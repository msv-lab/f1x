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

#include <boost/program_options.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include "Config.h"
#include "Repair.h"

namespace po = boost::program_options;
namespace logging = boost::log;

using std::vector;
using std::string;

int main (int argc, char *argv[])
{
  po::positional_options_description positional;
  positional.add("source", -1);
  
  // Declare supported options.
  po::options_description general("Usage: f1x PATH OPTIONS\n\nSupported options");
  general.add_options()
    ("files,f", po::value<vector<string>>()->multitoken()->value_name("RELPATH..."), "list of source files to repair")
    ("tests,t", po::value<vector<string>>()->multitoken()->value_name("ID..."), "list of test IDs")
    ("test-timeout,T", po::value<unsigned>()->value_name("MS"), "test execution timeout (default: none)")
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

  if (vm.count("verbose")) {
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::debug);    
  } else {
    logging::core::get()->set_filter(logging::trivial::severity >= logging::trivial::info);
  }

  if (!vm.count("source")) {
    BOOST_LOG_TRIVIAL(error) << "source directory is not specified (use --help)\n";
    return 1;
  }
  fs::path root(vm["source"].as<string>());

  unsigned testTimeout = 0;
  if (vm.count("test-timeout")) {
    testTimeout = vm["test-timeout"].as<unsigned>();
  }

  vector<fs::path> files;
  if (!vm.count("files")) {
    BOOST_LOG_TRIVIAL(error) << "files are not specified (use --help)\n";
    return 1;
  }
  vector<string> fileNames = vm["files"].as<vector<string>>();
  for(string &name : fileNames) {
    fs::path file;
    // FIXME: check if file exists
    files.push_back(file);
  }

  if (!vm.count("tests")) {
    BOOST_LOG_TRIVIAL(error) << "tests are not specified (use --help)\n";
    return 1;
  }
  vector<string> tests = vm["tests"].as<vector<string>>();

  if (!vm.count("driver")) {
    BOOST_LOG_TRIVIAL(error) << "test driver is not specified (use --help)\n";
    return 1;
  }
  // FIXME: check if driver exists and executable
  fs::path driver(vm["driver"].as<string>());

  string buildCmd = "make -e";
  if (vm.count("build")) {
    buildCmd = vm["build"].as<string>();
  }

  string patch;
  bool found = repair(root, files, tests, testTimeout, driver, buildCmd, patch);

  if (found) {
    std::cout << "SUCCESS" << std::endl;
    return 0;
  } else {
    std::cout << "FAIL" << std::endl;
    return 1;
  }
}
