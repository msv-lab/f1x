/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Shin Hwei Tan, Abhik Roychoudhury

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
#include <string>

#include <boost/filesystem.hpp>

#include <boost/program_options.hpp>

#include <boost/log/trivial.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "Core.h"
#include "Global.h"
#include "Repair.h"
#include "Util.h"


namespace po = boost::program_options;
namespace fs = boost::filesystem;

using std::vector;
using std::string;


void initializeTrivialLogger(bool verbose) {
  boost::log::add_common_attributes();
  boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
  boost::log::add_console_log(std::cerr, boost::log::keywords::format = "[%TimeStamp%] [%Severity%]\t%Message%");
  if (verbose) {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);    
  } else {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
  }
}


std::vector<ProjectFile> parseFilesArg(const boost::filesystem::path &root,
                                       const std::vector<std::string> &args) {
  std::vector<ProjectFile> files;
  for (auto &arg : args) {
    boost::filesystem::path file;
    unsigned fromLine = 0;
    unsigned toLine = 0;
    auto colonIndex = arg.find(":");
    if (colonIndex == std::string::npos) {
      file = boost::filesystem::path(arg);
    } else {
      std::string pathStr = arg.substr(0, colonIndex);
      file = boost::filesystem::path(pathStr);
      std::string rangeStr = arg.substr(colonIndex + 1);
      auto dashIndex = rangeStr.find("-");
      if (dashIndex == std::string::npos) {
        try {
          fromLine = std::stoi(rangeStr);
          toLine = fromLine;
        } catch (...) {
          throw parse_error("wrong range format: " + rangeStr);
        }
      } else {
        std::string fromStr = rangeStr.substr(0, dashIndex);
        std::string toStr = rangeStr.substr(dashIndex + 1);
        try {
          fromLine = std::stoi(fromStr);
          toLine = std::stoi(toStr);
        } catch (...) {
          throw parse_error("wrong range format: " + rangeStr);
        }
      }
    }
    boost::filesystem::path fullPath = root / file;
    if (! boost::filesystem::exists(fullPath)) {
      throw parse_error("source file does not exist: " + fullPath.string());
    }
    files.push_back(ProjectFile{file, fromLine, toLine});    
  }
  return files;
}


int main (int argc, char *argv[]) {
  po::positional_options_description positional;
  positional.add("root", -1);
  
  // Declare supported options.
  po::options_description general("Usage: f1x PATH OPTIONS\n\nSupported options");
  general.add_options()
    ("files,f", po::value<vector<string>>()->multitoken()->value_name("RELPATH..."), "list of source files to repair")
    ("tests,t", po::value<vector<string>>()->multitoken()->value_name("ID..."), "list of test IDs")
    ("test-timeout,T", po::value<unsigned>()->value_name("MS"), "test execution timeout")
    ("driver,d", po::value<string>()->value_name("PATH"), "test driver")
    ("build,b", po::value<string>()->value_name("CMD"), "build command (default: make -e)")
    ("output,o", po::value<string>()->value_name("PATH"), "output patch file or directory (default: SRC-TIME)")
    ("all,a", "generate all patches")
    ("cost,c", po::value<string>()->value_name("FUNCTION"), "patch prioritization (default: syntax-diff)")
    ("verbose,v", "produce extended output")
    ("help,h", "produce help message and exit")
    ("version", "print version and exit")
    ("dump-stat", po::value<string>()->value_name("PATH"), "output execution statistics")
    ("dump-space", po::value<string>()->value_name("PATH"), "[DEBUG] output search space")
    ("enable-cleanup", "remove intermediate data")
    ("enable-metadata", "output patch metadata")
    ("enable-validation", "validate found patches")
    ("enable-assignment", "synthesize assignments")
    ("disable-guard", "don't synthesize guards")
    ("disable-vteq", "[DEBUG] don't apply value-based analysis")
    ("disable-dteq", "[DEBUG] don't apply dependency-based analysis")
    ("disable-testprior", "[DEBUG] don't prioritize tests")
    ;

  po::options_description hidden("Hidden options");
  hidden.add_options()  
    ("root", po::value<string>(), "project root directory")
    ;

  po::options_description allOptions("All options");
  allOptions.add(general).add(hidden);

  po::variables_map vm;

  try {
    po::store(po::command_line_parser(argc, argv).options(allOptions).positional(positional).run(), vm);
    po::notify(vm);
  } catch(po::error& e) {
    BOOST_LOG_TRIVIAL(error) << e.what() << " (use --help)";
    return 0;
  }

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
    cfg.verbose = true;
  }
  initializeTrivialLogger(cfg.verbose);

  if (vm.count("all")) {
    cfg.generateAll = true;
  }

  if (vm.count("enable-metadata")) {
    cfg.outputPatchMetadata = true;
  }

  if (vm.count("enable-cleanup")) {
    cfg.removeIntermediateData = true;
  }

  if (vm.count("enable-validation")) {
    cfg.validatePatches = true;
  }

  if (vm.count("disable-vteq")) {
    cfg.valueTEQ = false;
  }

  if (vm.count("disable-testprior")) {
    cfg.testPrioritization = TestPrioritization::FIXED_ORDER;
  }

  if (vm.count("disable-guard")) {
    cfg.addGuards = false;
  }

  if (vm.count("dump-space")) {
    cfg.searchSpaceFile = fs::absolute(vm["dump-space"].as<string>()).string();
  }

  if (!vm.count("root")) {
    BOOST_LOG_TRIVIAL(error) << "project root directory is not specified (use --help)";
    return 1;
  }
  fs::path root(vm["root"].as<string>());
  root = fs::absolute(root);
  if (! (fs::exists(root) && fs::is_directory(root))) {
    BOOST_LOG_TRIVIAL(error) << "project root directory " << root.string() << " does not exist";
    return 1;
  }

  if (!vm.count("test-timeout")) {
    BOOST_LOG_TRIVIAL(error) << "test execution timeout is not specified (use --help)";
    return 1;
  }
  unsigned testTimeout = vm["test-timeout"].as<unsigned>();

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

  fs::path output;
  if (!vm.count("output")) {
    std::time_t now = std::time(0);
    struct std::tm tstruct;
    char timeRepr[80];
    tstruct = *localtime(&now);
    strftime(timeRepr, sizeof(timeRepr), "-%Y_%m_%d-%H_%M_%S", &tstruct);
    string dirname;
    for(auto& e : root)
      if (e.string() != ".")
        dirname = e.string();
    std::stringstream name;
    name << dirname << timeRepr;
    if (!cfg.generateAll)
      name << ".patch";
    output = fs::path(name.str());
  } else {
    output = fs::path(vm["output"].as<string>());
  }

  output = fs::absolute(output);
  if (fs::exists(output)) {
    BOOST_LOG_TRIVIAL(warning) << "existing " << output << " will be overwritten";
    fs::remove_all(output);
  }

  fs::path dataDir = fs::temp_directory_path() / fs::unique_path();
  fs::create_directory(dataDir);
  BOOST_LOG_TRIVIAL(info) << "intermediate data directory: " << dataDir;
  cfg.dataDir = dataDir.string();

  bool found = false;
  {
    Project project(root, files, buildCmd);
    TestingFramework tester(project, driver, testTimeout);
    
    found = repair(project, tester, tests, output);
  }

  // NOTE: project is already destroyed here
  if (cfg.removeIntermediateData) {
    fs::remove_all(dataDir);
  }

  if (found) {
    if (cfg.generateAll) {
      BOOST_LOG_TRIVIAL(info) << "patches successfully generated: " << output;
    } else {
      BOOST_LOG_TRIVIAL(info) << "patch successfully generated: " << output;
    }
    return SUCCESS_EXIT_CODE;
  } else {
    BOOST_LOG_TRIVIAL(info) << "failed to generated a patch";
    return FAILURE_EXIT_CODE;
  }
}
