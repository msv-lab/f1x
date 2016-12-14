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
#include "MetaProgram.h"
#include "SearchEngine.h"

namespace fs = boost::filesystem;
using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;


bool repair(const fs::path &root,
            const vector<fs::path> &files,
            const vector<string> &tests,
            const uint testTimeout,
            const fs::path &driver,
            const string &buildCmd,
            const fs::path &patchFile) {
  BOOST_LOG_TRIVIAL(info) << "repairing project " << root;
  BOOST_LOG_TRIVIAL(debug) << "with timeout " << testTimeout;

  setenv("CC", "f1x-cc", true);

  BOOST_LOG_TRIVIAL(info) << "building with " << buildCmd;
  {
    FromDirectory dir(root);
    string cmd = "bear -- " + buildCmd;
    uint status = std::system(cmd.c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(warning) << "compilation failed";
    }
  }

  BOOST_LOG_TRIVIAL(info) << "adjusting compilation database";
  addClangHeadersToCompileDB(root);

  fs::path workDir = fs::temp_directory_path() / fs::unique_path();
  fs::create_directory(workDir);
  BOOST_LOG_TRIVIAL(info) << "working directory: " + workDir.string();

  backupSource(workDir, root, files);

  fs::path candidateLocationsFile = workDir / fs::path("cl.json");

  BOOST_LOG_TRIVIAL(info) << "instrumenting";
  {
    FromDirectory dir(root);
    string cmd = "f1x-transform " + files[0].string() + " --instrument --file-id 0 --output " + candidateLocationsFile.string();
    uint status = std::system(cmd.c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(warning) << "transformation failed";
    }
  }

  BOOST_LOG_TRIVIAL(info) << "loading candidate locations";
  vector<shared_ptr<CandidateLocation>> cls = loadCandidateLocations(candidateLocationsFile);

  fs::path runtimeSourceFile = workDir / fs::path("rt.cpp");
  fs::path runtimeHeaderFile = workDir / fs::path("rt.h");

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

  setenv("F1X_RUNTIME_H", runtimeHeaderFile.string().c_str(), true);
  setenv("F1X_RUNTIME_LIB", workDir.string().c_str(), true);
  setenv("LD_LIBRARY_PATH", workDir.string().c_str(), true);

  BOOST_LOG_TRIVIAL(info) << "rebuilding project";
  {
    FromDirectory dir(root);
    string cmd = buildCmd;
    uint status = std::system(cmd.c_str());
    if (status != 0) {
      BOOST_LOG_TRIVIAL(warning) << "compilation failed";
    }
  }

  TestingFramework tester(root, driver);

  SearchSpaceElement patch;
  bool found = search(searchSpace, tests, tester, patch);

  restoreSource(workDir, root, files);

  if (found) {
    BOOST_LOG_TRIVIAL(info) << "applying patch";
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
        BOOST_LOG_TRIVIAL(warning) << "transformation failed";
      }
    }

    computeDiff(root, files[0], 0, patchFile);
  }

  restoreSource(workDir, root, files);

  return found;
}
