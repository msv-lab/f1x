/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Abhik Roychoudhury

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
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <map>

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

const string CANDADATE_LOCATIONS_FILE_NAME = "extracted.json";

const std::string PROFILE_FILE_NAME = "Recoder";
const std::string PROFILE_SOURCE_FILE_NAME = "Recoder.cpp";
const std::string PROFILE_HEADER_FILE_NAME = "Recoder.h";

const std::string RUNTIME_SOURCE_FILE_NAME = "rt.cpp";
const std::string RUNTIME_HEADER_FILE_NAME = "rt.h";

std::map<string, int> interestedLine;

double score(const SearchSpaceElement &el) {
  double result = (double) el.meta.distance;
  const double GOOD = 0.3;
  const double OK = 0.2;
  const double BAD = 0.1;
  switch (el.meta.transformation) {
  case Transformation::ALTERNATIVE:
    result -= OK;
    break;
  case Transformation::SWAPING:
    result -= GOOD;
    break;
  case Transformation::SIMPLIFICATION:
    result -= GOOD;
    break;
  case Transformation::GENERALIZATION:
    result -= GOOD;
    break;
  case Transformation::SUBSTITUTION:
    result -= BAD;
    break;
  case Transformation::WIDENING:
    result -= BAD;
    break;
  case Transformation::NARROWING:
    result -= BAD;
    break;
  default:
    break; // everything else is very bad
  }
  return result;
}

void mergeInterestedLine(const char* tempFile)
{
  std::ifstream infile(tempFile);
  std::string line;
  BOOST_LOG_TRIVIAL(debug) << "merging profiling recode:" << tempFile << "\n";
  while(std::getline(infile, line))
  {
    if(interestedLine.find(line) == interestedLine.end())
      interestedLine[line] = 1;
  }
}

void recordinterestedLine(const char* interestingLocationFile)
{
  BOOST_LOG_TRIVIAL(info) << "writing profiling data into " << interestingLocationFile << "\n";
  std::ofstream outfile(interestingLocationFile, std::ios::app);
  std::map<string, int>::iterator it = interestedLine.begin();
  while(it!=interestedLine.end())
  {
    outfile << it->first << "\n";
    it++;
  }
}

bool comparePatches(const SearchSpaceElement &a, const SearchSpaceElement &b) {
  return score(a) < score(b);
}


void prioritize(std::vector<SearchSpaceElement> &searchSpace) {
  std::stable_sort(searchSpace.begin(), searchSpace.end(), comparePatches);
}


void dumpSearchSpace(std::vector<SearchSpaceElement> &searchSpace, const fs::path &file, const vector<ProjectFile> &files) {
  fs::ofstream os(file);
  for (auto &el : searchSpace) {
    os << std::setprecision(3) << score(el) << " " << visualizeElement(el, files[el.buggy->location.fileId].relpath) << "\n";
  }
}


bool repair(Project &project,
            TestingFramework &tester,
            const std::vector<std::string> &tests,
            const boost::filesystem::path &workDir,
            const boost::filesystem::path &patchOutput,
            const Config &cfg) {

  BOOST_LOG_TRIVIAL(info) << "repairing project " << project.getRoot();

  pair<bool, bool> initialStatus = project.initialBuild();
  if (! initialStatus.first) {
    BOOST_LOG_TRIVIAL(warning) << "compilation returned non-zero exit code";
  }
  if (! initialStatus.second) {
    BOOST_LOG_TRIVIAL(error) << "failed to infer compile commands";
    return false;
  }

/**********************************************/
//this part is used to profile
  fs::path ilFile = workDir / fs::path(PROFILE_FILE_NAME);
  BOOST_LOG_TRIVIAL(info) << "instrumenting for profile\n";
  //instrument candidate file to record the expressions and statements excuted by a test case
  bool profileInstSuccess = project.instrumentFile(project.getFiles()[0], ilFile, true);
  if (! profileInstSuccess) {
    BOOST_LOG_TRIVIAL(warning) << "profiling instrumentation returned non-zero exit code";
  }

  project.saveProfileInstFiles();

//compile and excute all tests @waiting to implement
  Runtime profileRuntime(workDir, cfg, PROFILE_SOURCE_FILE_NAME, PROFILE_HEADER_FILE_NAME);
  bool profileSuccess = profileRuntime.compile();

  if (! profileSuccess) {
    BOOST_LOG_TRIVIAL(error) << "profile compilation failed";
    return false;
  }

  bool profileRebuildSucceeded = project.buildWithRuntime(profileRuntime.getHeader());

  if (! profileRebuildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation with runtime returned non-zero exit code";
  }

  bool pass = false;
  //fs::path tempFile;//the excution path will be saved in this file
  //tempFile  = workDir / fs::path(PROFILE_FILE_NAME);
  BOOST_LOG_TRIVIAL(info) << "profiling all the test cases\n";
  for (auto &test : tests) {
  
    pass = tester.isPassing(test);
    if(!pass)
    {
      mergeInterestedLine(ilFile.c_str());
    }
    std::ostringstream cmd;
    cmd << "rm " << ilFile.c_str();
    system(cmd.str().c_str());
  }
  
  //the excution path of the failing test case will be saved in this file
  //fs::path interestiongLocationFile;
  //interestiongLocationFile = workDir / fs::path(INTRESTING_LOCATIONS_FILE_NAME);
  recordinterestedLine(ilFile.c_str());
  
  project.restoreOriginalFiles();

/**********************************************/

  fs::path clFile = workDir / fs::path(CANDADATE_LOCATIONS_FILE_NAME);

  bool instrSuccess = project.instrumentFile(project.getFiles()[0], clFile, false);
  if (! instrSuccess) {
    BOOST_LOG_TRIVIAL(warning) << "transformation returned non-zero exit code";
  }
  if (! fs::exists(clFile)) {
    BOOST_LOG_TRIVIAL(error) << "failed to extract candidate locations";
    return false;
  }

  project.saveInstrumentedFiles();

  BOOST_LOG_TRIVIAL(debug) << "loading candidate locations";
  vector<shared_ptr<CandidateLocation>> cls = loadCandidateLocations(clFile);

  vector<SearchSpaceElement> searchSpace;

  Runtime runtime(workDir, cfg, RUNTIME_SOURCE_FILE_NAME, RUNTIME_HEADER_FILE_NAME);

  BOOST_LOG_TRIVIAL(info) << "generating search space";
  {
    fs::ofstream os(runtime.getSource());
    fs::ofstream oh(runtime.getHeader());
    searchSpace = generateSearchSpace(cls, workDir, os, oh, cfg);
  }

  bool runtimeSuccess = runtime.compile();

  if (! runtimeSuccess) {
    BOOST_LOG_TRIVIAL(error) << "runtime compilation failed";
    return false;
  }

  bool rebuildSucceeded = project.buildWithRuntime(runtime.getHeader());

  if (! rebuildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation with runtime returned non-zero exit code";
  }

  project.restoreOriginalFiles();

  BOOST_LOG_TRIVIAL(info) << "prioritizing search space";
  prioritize(searchSpace);

  if (cfg.dumpSearchSpace) {
    BOOST_LOG_TRIVIAL(info) << "dumping search space: " << (workDir / "searchspace.txt");
    dumpSearchSpace(searchSpace, workDir / "searchspace.txt", project.getFiles());
  }

  BOOST_LOG_TRIVIAL(info) << "search space size: " << searchSpace.size();
  
  SearchEngine engine(tests, tester, runtime, cfg);

  uint last = 0;
  uint patchCount = 0;

  while (last < searchSpace.size()) {
    last = engine.findNext(searchSpace, last);

    if (last < searchSpace.size()) {

      fs::path relpath = project.getFiles()[searchSpace[last].buggy->location.fileId].relpath;
      BOOST_LOG_TRIVIAL(info) << "found patch: " << visualizeElement(searchSpace[last], relpath);
      
      bool appSuccess = project.applyPatch(searchSpace[last]);
      if (! appSuccess) {
        BOOST_LOG_TRIVIAL(warning) << "patch application failed";
      }

      project.savePatchedFiles();

      bool valid = true;

      if (cfg.validatePatches) {
        BOOST_LOG_TRIVIAL(info) << "validating patch";

        bool rebuildSuccess = project.build();
        if (! rebuildSuccess) {
          BOOST_LOG_TRIVIAL(warning) << "compilation with patch returned non-zero exit code";
        }

        vector<string> failing = getFailing(tester, tests);

        if (!failing.empty()) {
          valid = false;
          BOOST_LOG_TRIVIAL(warning) << "generated patch failed validation";
          for (auto &t : failing) {
            BOOST_LOG_TRIVIAL(debug) << "failed test: " << t;
          }
        }

        if (cfg.generateAll) {
          project.restoreInstrumentedFiles();
          project.buildWithRuntime(runtime.getHeader());
        }
        
        project.restoreOriginalFiles();
      }

      if (!valid)
        continue;

      fs::path patchFile = patchOutput;
      if (cfg.generateAll) {
        if (! fs::exists(patchFile)) {
          fs::create_directory(patchFile);
        }
        patchFile = patchFile / (std::to_string(patchCount) + ".patch");
      }

      patchCount++;

      project.computeDiff(project.getFiles()[0], patchFile);
      
      if (!cfg.generateAll)
        break;
    }

    last++;
  }

  BOOST_LOG_TRIVIAL(info) << "candidates evaluated: " << engine.getCandidateCount();
  BOOST_LOG_TRIVIAL(info) << "tests executed: " << engine.getTestCount();

  return patchCount > 0;
}
