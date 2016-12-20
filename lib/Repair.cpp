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
            const boost::filesystem::path &patchOutput,
            bool verbose,
            bool all) {

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

  BOOST_LOG_TRIVIAL(info) << "search space: " << searchSpace.size();
  
  SearchEngine engine(tests, tester, runtime);

  uint last = 0;
  uint patchCount = 0;

  while (last < searchSpace.size()) {
    last = engine.findNext(searchSpace, last);

    if (last < searchSpace.size()) {
      patchCount++;
      fs::path patchFile = patchOutput;
      if (all) {
        if (! fs::exists(patchFile)) {
          fs::create_directory(patchFile);
        }
        patchFile = patchFile / (std::to_string(patchCount) + ".patch");
      }

      project.restoreOriginalFiles();
      
      bool appSuccess = project.applyPatch(searchSpace[last]);
      if (! appSuccess) {
        BOOST_LOG_TRIVIAL(warning) << "patch application failed";
      }

      project.computeDiff(project.getFiles()[0], patchFile);
      
      if (!all)
        break;
    }

    last++;
  }

  BOOST_LOG_TRIVIAL(info) << "candidates evaluated: " << engine.getCandidateCount();
  BOOST_LOG_TRIVIAL(info) << "tests executed: " << engine.getTestCount();

  project.restoreOriginalFiles();

  return patchCount > 0;
}
