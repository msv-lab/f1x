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

#include <boost/program_options.hpp>

#include <boost/log/trivial.hpp>

#include "Config.h"
#include "ToolsCommon.h"
#include "Repair.h"
#include "RepairUtil.h"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using std::vector;
using std::string;


int main (int argc, char *argv[])
{
  po::positional_options_description positional;
  positional.add("source", -1);
  
  // Declare supported options.
  po::options_description general("Usage: f1x PATH OPTIONS\n\nSupported options");
  general.add_options()
    ("patch,p", po::value<string>()->value_name("PATH"), "patch file")
    ("files,f", po::value<vector<string>>()->multitoken()->value_name("RELPATH..."), "list of source files to repair")
    ("tests,t", po::value<vector<string>>()->multitoken()->value_name("ID..."), "list of test IDs")
    ("test-timeout,T", po::value<uint>()->value_name("MS"), "test execution timeout (default: none)")
    ("driver,d", po::value<string>()->value_name("PATH"), "test driver")
    ("build,b", po::value<string>()->value_name("CMD"), "build command (default: make -e)")
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

  if(vm.count("verbose")){
    initializeTrivialLogger(true);
  } else {
    initializeTrivialLogger(false);
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

  if (!vm.count("files")) {
    BOOST_LOG_TRIVIAL(error) << "files are not specified (use --help)";
    return 1;
  }
  vector<string> fileArgs = vm["files"].as<vector<string>>();
  vector<ProjectFile> files;
  try {
    files = parseFilesArg(root, fileArgs);
  } catch (const parse_error& e) {
    BOOST_LOG_TRIVIAL(error) << e.what();
    return 1;
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
  
  if (!vm.count("patch")) {
    BOOST_LOG_TRIVIAL(error) << "patch file is not specified (use --help)";
    return 1;
  }
  fs::path patch = fs::path(vm["patch"].as<string>());
  patch = fs::absolute(patch);
  if (! fs::exists(patch)) {
    BOOST_LOG_TRIVIAL(error) << "patch " << driver.string() << " does not exist";
    return 1;
  }

  fs::path workDir = fs::temp_directory_path() / fs::unique_path();
  fs::create_directory(workDir);
  BOOST_LOG_TRIVIAL(debug) << "working directory: " + workDir.string();

  Project project(root, files, buildCmd, workDir);
  TestingFramework tester(project, driver, testTimeout);

  project.backupFiles();

  BOOST_LOG_TRIVIAL(debug) << "applying patch";
  {
    FromDirectory dir(project.getRoot());
    std::stringstream cmd;
    cmd << "patch -p1 < " << patch.string();
    uint status = std::system(cmd.str().c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(error) << "patch application failed";
      return 1;
    }
  }

  project.initialBuild();

  vector<string> failingTests;

  for (auto &test : tests) {
    if (! tester.isPassing(test)) {
      failingTests.push_back(test);
    }
  }

  project.restoreFiles();

  if (failingTests.empty()) {
    BOOST_LOG_TRIVIAL(info) << "patch successfully validated";
    return 0;
  } else {
    for (auto &test : failingTests) {
      BOOST_LOG_TRIVIAL(info) << "failing test: " << test;
    }
    return 1;
  }
}
