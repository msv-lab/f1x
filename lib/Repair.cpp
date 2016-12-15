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

#include <cstdlib>
#include <memory>
#include <iostream>
#include <sstream>

#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include "Repair.h"
#include "RepairUtil.h"
#include "Project.h"
#include "MetaProgram.h"
#include "SearchEngine.h"

namespace fs = boost::filesystem;
using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;

const string CANDADATE_LOCATIONS_FILE_NAME = "cl.json";
const string RUNTIME_SOURCE_FILE_NAME = "rt.cpp";
const string RUNTIME_HEADER_FILE_NAME = "rt.h";


bool repair(const fs::path &root,
            const vector<fs::path> &files,
            const vector<string> &tests,
            const uint testTimeout,
            const fs::path &driver,
            const string &buildCmd,
            const fs::path &patchFile) {
  BOOST_LOG_TRIVIAL(info) << "repairing project " << root;

  fs::path workDir = fs::temp_directory_path() / fs::unique_path();
  fs::create_directory(workDir);
  BOOST_LOG_TRIVIAL(debug) << "working directory: " + workDir.string();

  Project project(root, files, buildCmd, workDir);

  bool buildSucceeded = project.initialBuild();

  if (! buildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation returned non-zero exit code";
  }

  project.backupFiles();

  fs::path clFile = workDir / fs::path(CANDADATE_LOCATIONS_FILE_NAME);

  BOOST_LOG_TRIVIAL(debug) << "instrumenting";
  {
    FromDirectory dir(root);
    std::stringstream s;
    s << "f1x-transform " << files[0].string() << " --instrument"
      << " --file-id 0"
      << " --output " + clFile.string();
    uint status = std::system(s.str().c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(warning) << "transformation failed";
    }
  }

  project.saveFilesWithPrefix("instrumented");

  BOOST_LOG_TRIVIAL(debug) << "loading candidate locations";
  vector<shared_ptr<CandidateLocation>> cls = loadCandidateLocations(clFile);

  fs::path runtimeSourceFile = workDir / fs::path(RUNTIME_SOURCE_FILE_NAME);
  fs::path runtimeHeaderFile = workDir / fs::path(RUNTIME_HEADER_FILE_NAME);

  vector<SearchSpaceElement> searchSpace;

  BOOST_LOG_TRIVIAL(info) << "generating search space";
  {
    fs::ofstream os(runtimeSourceFile);
    fs::ofstream oh(runtimeHeaderFile);
    searchSpace = generateSearchSpace(cls, os, oh);
  }

  BOOST_LOG_TRIVIAL(info) << "compiling search space";
  {
    FromDirectory dir(workDir);
    string cmd = RUNTIME_COMPILER + " -O2 -fPIC rt.cpp -shared -o libf1xrt.so";
    uint status = std::system(cmd.c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(warning) << "runtime compilation failed";
    }
  }


  bool rebuildSucceeded = project.buildWithRuntime(runtimeHeaderFile);

  if (! rebuildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation with runtime returned non-zero exit code";
  }

  TestingFramework tester(project, driver);

  SearchSpaceElement patch;
  bool found = search(searchSpace, tests, tester, patch);

  if (found) {
    project.restoreFiles();

    BOOST_LOG_TRIVIAL(debug) << "applying patch";
    {
      FromDirectory dir(root);
      uint beginLine = patch.buggy->location.beginLine;
      uint beginColumn = patch.buggy->location.beginColumn;
      uint endLine = patch.buggy->location.endLine;
      uint endColumn = patch.buggy->location.endColumn;
      std::stringstream cmd;
      cmd << "f1x-transform " << files[0].string() << " --apply" 
          << " --bl " << beginLine 
          << " --bc " << beginColumn
          << " --el " << endLine 
          << " --ec " << endColumn 
          << " --patch " << "\"" << expressionToString(patch.patch) << "\"";
      uint status = std::system(cmd.str().c_str());
      if (status != 0) {
        BOOST_LOG_TRIVIAL(warning) << "patch application failed";
      }
    }

    project.computeDiff(files[0], patchFile);
  }

  project.restoreFiles();

  return found;
}
