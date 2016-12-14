/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Abhik Roychoudhury

  f1x is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <ctime>
#include <sstream>
#include <sys/stat.h>

#include <boost/program_options.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "Config.h"
#include "Repair.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace logging = boost::log;

using std::vector;
using std::string;


bool isExecutable(const char *file)
{
  struct stat  st;

  if (stat(file, &st) < 0)
    return false;
  if ((st.st_mode & S_IEXEC) != 0)
    return true;
  return false;
}


int main (int argc, char *argv[])
{
  po::positional_options_description positional;
  positional.add("source", -1);
  
  // Declare supported options.
  po::options_description general("Usage: f1x PATH OPTIONS\n\nSupported options");
  general.add_options()
    ("files,f", po::value<vector<string>>()->multitoken()->value_name("RELPATH..."), "list of source files to repair")
    ("tests,t", po::value<vector<string>>()->multitoken()->value_name("ID..."), "list of test IDs")
    ("test-timeout,T", po::value<uint>()->value_name("MS"), "test execution timeout (default: none)")
    ("driver,d", po::value<string>()->value_name("PATH"), "test driver")
    ("build,b", po::value<string>()->value_name("CMD"), "build command (default: make -e)")
    ("output,o", po::value<string>()->value_name("PATH"), "output patch file (default: SRC-TIME.patch)")
    ("verbose,v", "produce extended output")
    ("help,h", "produce help message and exit")
    ("version", "print version and exit")
    ;

  po::options_description hidden("Hidden options");
  hidden.add_options()  
    ("source", po::value<string>(), "source directory")
    ;

  po::options_description all("All options");
  all.add(general).add(hidden);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(all).positional(positional).run(), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << general << std::endl;
    return 1;
  }

  if (vm.count("version")) {
    std::cout << "f1x " << F1X_VERSION_MAJOR <<
                    "." << F1X_VERSION_MINOR <<
                    "." << F1X_VERSION_PATCH << std::endl;
    return 1;
  }

  logging::add_common_attributes();
  logging::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
  logging::add_console_log(std::cerr, logging::keywords::format = "[%TimeStamp%] [%Severity%]\t%Message%");
  if (vm.count("verbose")) {
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::debug);    
  } else {
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
  }

  if (!vm.count("source")) {
    BOOST_LOG_TRIVIAL(error) << "source directory is not specified (use --help)";
    return 1;
  }
  fs::path root(vm["source"].as<string>());
  root = fs::absolute(root);
  if (! (fs::exists(root) && fs::is_directory(root))) {
    BOOST_LOG_TRIVIAL(error) << "source directory " << root.string() << " does not exist";
    return 1;
  }

  if (!vm.count("test-timeout")) {
    BOOST_LOG_TRIVIAL(error) << "test execution timeout is not specified (use --help)";
    return 1;
  }
  uint testTimeout = vm["test-timeout"].as<uint>();

  vector<fs::path> files;
  if (!vm.count("files")) {
    BOOST_LOG_TRIVIAL(error) << "files are not specified (use --help)";
    return 1;
  }
  vector<string> fileNames = vm["files"].as<vector<string>>();
  for(string &name : fileNames) {
    fs::path file(name);
    fs::path fullPath = root / file;
    if (! fs::exists(fullPath)) {
      BOOST_LOG_TRIVIAL(error) << "source file " << fullPath.string() << " does not exist";
      return 1;
    }
    files.push_back(file);
  }

  if (!vm.count("tests")) {
    BOOST_LOG_TRIVIAL(error) << "tests are not specified (use --help)";
    return 1;
  }
  vector<string> tests = vm["tests"].as<vector<string>>();

  if (!vm.count("driver")) {
    BOOST_LOG_TRIVIAL(error) << "test driver is not specified (use --help)";
    return 1;
  }
  fs::path driver(vm["driver"].as<string>());
  driver = fs::absolute(driver);
  if (! fs::exists(driver)) {
    BOOST_LOG_TRIVIAL(error) << "driver " << driver.string() << " does not exist";
    return 1;
  }
  if (! isExecutable(driver.string().c_str())) {
    BOOST_LOG_TRIVIAL(error) << "driver " << driver.string() << " is not executable";
    return 1;
  }

  string buildCmd = "make -e";
  if (vm.count("build")) {
    buildCmd = vm["build"].as<string>();
  }

  fs::path output;
  if (!vm.count("output")) {
    std::time_t now = std::time(0);
    struct std::tm tstruct;
    char timeRepr[80];
    tstruct = *localtime(&now);
    strftime(timeRepr, sizeof(timeRepr), "%y%m%d_%H%M%S", &tstruct);
    std::stringstream name;
    name << fs::basename(root) << "-" << timeRepr << ".patch";
    output = fs::path(name.str());
  } else {
    output = fs::path(vm["output"].as<string>());
  }
  output = fs::absolute(output);

  bool found = repair(root, files, tests, testTimeout, driver, buildCmd, output);

  if (found) {
    BOOST_LOG_TRIVIAL(info) << "patch successfully generated: " << output.string();
    return 0;
  } else {
    BOOST_LOG_TRIVIAL(info) << "failed to generated a patch";
    return 1;
  }
}
