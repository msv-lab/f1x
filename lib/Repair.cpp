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

#include <memory>
#include <iostream>
#include <sstream>

#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include "Repair.h"
#include "RepairUtil.h"
#include "Project.h"
#include "Runtime.h"
#include "Synthesis.h"
#include "SearchEngine.h"

namespace fs = boost::filesystem;
using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;

const string CANDADATE_LOCATIONS_FILE_NAME = "cl.json";

bool repair(Project &project,
            TestingFramework &tester,
            const std::vector<std::string> &tests,
            const boost::filesystem::path &workDir,
            const boost::filesystem::path &patchFile,
            bool verbose) {

  BOOST_LOG_TRIVIAL(info) << "repairing project " << project.getRoot();

  bool buildSucceeded = project.initialBuild();
  if (! buildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation returned non-zero exit code";
  }

  project.backupOriginalFiles();

  fs::path clFile = workDir / fs::path(CANDADATE_LOCATIONS_FILE_NAME);

  bool instrSuccess = project.instrumentFile(project.getFiles()[0], clFile);
  if (! instrSuccess) {
    BOOST_LOG_TRIVIAL(warning) << "transformation returned non-zero exit code";
  }
  if (! fs::exists(clFile)) {
    BOOST_LOG_TRIVIAL(error) << "failed to extract candidate locations";
    project.restoreOriginalFiles();
    return false;
  }

  project.backupInstrumentedFiles();

  BOOST_LOG_TRIVIAL(debug) << "loading candidate locations";
  vector<shared_ptr<CandidateLocation>> cls = loadCandidateLocations(clFile);

  vector<SearchSpaceElement> searchSpace;

  Runtime runtime(workDir, verbose);

  BOOST_LOG_TRIVIAL(info) << "generating search space";
  {
    fs::ofstream os(runtime.getSource());
    fs::ofstream oh(runtime.getHeader());
    searchSpace = generateSearchSpace(cls, os, oh);
  }

  bool runtimeSuccess = runtime.compile();

  if (! runtimeSuccess) {
    BOOST_LOG_TRIVIAL(warning) << "runtime compilation failed";
  }

  bool rebuildSucceeded = project.buildWithRuntime(runtime.getHeader());

  if (! rebuildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation with runtime returned non-zero exit code";
  }

  SearchSpaceElement patch;

  bool found = search(searchSpace, tests, tester, runtime, patch);

  if (found) {
    project.restoreOriginalFiles();

    bool appSuccess = project.applyPatch(patch);
    if (! appSuccess) {
      BOOST_LOG_TRIVIAL(warning) << "patch application failed";
    }

    project.computeDiff(project.getFiles()[0], patchFile);
  }

  project.restoreOriginalFiles();

  return found;
}
