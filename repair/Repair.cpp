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

#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <map>
#include <set>
#include <vector>

#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include "Repair.h"
#include "Global.h"
#include "Typing.h"
#include "Util.h"
#include "Project.h"
#include "Runtime.h"
#include "Profiler.h"
#include "Synthesis.h"
#include "SearchEngine.h"
#include "FaultLocalization.h"

namespace fs = boost::filesystem;
using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unordered_map;
using std::unordered_set;


string getSchemaApplicationsFileName(unsigned index) {
  std::stringstream name;
  name << "applications" << index << ".json";
  return name.str();
}

double simplicityScore(const SearchSpaceElement &el) {
  double result = (double) el.meta.distance;
  const double GOOD = 0.2;
  const double OK = 0.1;
  switch (el.app->schema) {
  case TransformationSchema::EXPRESSION:
    switch (el.meta.kind) {
    case ModificationKind::OPERATOR:
      result -= GOOD;
      break;
    case ModificationKind::SWAPING:
      result -= GOOD;
      break;
    case ModificationKind::SIMPLIFICATION:
      result -= GOOD;
      break;
    case ModificationKind::GENERALIZATION:
      result -= GOOD;
      break;
    case ModificationKind::SUBSTITUTION:
      result -= GOOD;
      break;
    case ModificationKind::LOOSENING:
      result -= OK;
      break;
    case ModificationKind::TIGHTENING:
      result -= OK;
      break;
    default:
      break; // everything else is bad
    }
    break;
  case TransformationSchema::IF_GUARD:
    result -= GOOD;
    break;
  default:
    break;
  }
  return result;
}


bool comparePatches(const SearchSpaceElement &a, const SearchSpaceElement &b) {
  return simplicityScore(a) < simplicityScore(b);
}


void prioritize(std::vector<SearchSpaceElement> &searchSpace) {
  std::stable_sort(searchSpace.begin(), searchSpace.end(), comparePatches);
}


void dumpSearchSpace(std::vector<SearchSpaceElement> &searchSpace, const fs::path &file, const vector<ProjectFile> &files) {
  fs::ofstream os(file);
  for (auto &el : searchSpace) {
    os << std::setprecision(3) << simplicityScore(el) << " " 
       << visualizeElement(el, files[el.app->location.fileId].relpath) << "\n";
  }
}


shared_ptr<unordered_map<unsigned long, unordered_set<F1XID>>> getPartitionable(const std::vector<SearchSpaceElement> &searchSpace) {
  shared_ptr<unordered_map<unsigned long, unordered_set<F1XID>>> result(new unordered_map<ulong, unordered_set<F1XID>>);
  for (auto &el : searchSpace) {
    unsigned long locId = el.app->appId;
    if (! result->count(locId)) {
      (*result)[locId] = unordered_set<F1XID>();
    }
    (*result)[locId].insert(el.id);
  }
  return result;
}


bool repair(Project &project,
            TestingFramework &tester,
            const std::vector<std::string> &tests,
            const boost::filesystem::path &patchOutput) {

  BOOST_LOG_TRIVIAL(info) << "repairing project " << project.getRoot();
  
  pair<bool, bool> initialBuildStatus = project.initialBuild();
  if (! initialBuildStatus.first) {
    BOOST_LOG_TRIVIAL(warning) << "compilation returned non-zero exit code";
  }
  if (! initialBuildStatus.second) {
    BOOST_LOG_TRIVIAL(error) << "failed to infer compile commands";
    return false;
  }
  std::vector<std::string> fFromJson;
  if (project.getFiles().empty())
  {
  /**
   * testing fault localization module
   */
    if (cfg.filesToLocalize > 0)
    {
      fFromJson = FaultLocalization::getFileFromJson(project.getRoot());
    }
    if (cfg.filesToLocalize < fFromJson.size())
    {
      int nGt = fFromJson.size() - cfg.filesToLocalize;
      fFromJson.erase(fFromJson.begin(),fFromJson.begin() + nGt);
    }
  }
  else
  {
	  for (auto &file : project.getFiles())
	  {
		  fFromJson.push_back(file.relpath.string());
	  }
  }
  if (!fFromJson.empty())
  {
	{
      for (auto &file : fFromJson)
      {
        FaultLocalization faultLocal(tests,tester,project,fs::path(file));
        vector<struct TarantulaScore> vFaultLocal = faultLocal.getFaultLocalization();
        if (!vFaultLocal.empty())
        {
          BOOST_LOG_TRIVIAL(info) << file;
          for (int i = 0 ; i < vFaultLocal.size(); i++)
          {
            BOOST_LOG_TRIVIAL(info) << vFaultLocal[i].line << "    " << vFaultLocal[i].score;
          }
        }
        else
        {
          BOOST_LOG_TRIVIAL(info) << "Does not find fault localize";
        }
      }
	}
  }

  fs::path traceFile = fs::path(cfg.dataDir) / TRACE_FILE_NAME;

  BOOST_LOG_TRIVIAL(info) << "instrumenting source files for profiling";
  for (auto &file : project.getFiles()) {
    bool profileInstSuccess = project.instrumentFile(file, traceFile);
    if (! profileInstSuccess) {
      BOOST_LOG_TRIVIAL(warning) << "profiling instrumentation of " << file.relpath << " returned non-zero exit code";
    }
  }
  project.saveProfileInstumentedFiles();

  Profiler profiler;

  bool profilerBuildSuccess = profiler.compile();
  if (! profilerBuildSuccess) {
    BOOST_LOG_TRIVIAL(error) << "profiler runtime compilation failed";
    return false;
  }

  bool profileRebuildSucceeded = project.buildWithRuntime(profiler.getHeader());
  if (! profileRebuildSucceeded) {
    BOOST_LOG_TRIVIAL(warning) << "compilation with profiler runtime returned non-zero exit code";
  }

  project.restoreOriginalFiles();

  BOOST_LOG_TRIVIAL(info) << "profiling project";
  vector<string> negativeTests;
  unsigned long numPositive = 0;
  unsigned long numNegative = 0;
  for (int i = 0; i < tests.size(); i++) {
    auto test = tests[i];
    profiler.clearTrace();
    TestStatus status = tester.execute(test);
    if (status == TestStatus::PASS)
      numPositive++;
    else {
      numNegative++;
      negativeTests.push_back(test);
    }
    if (status == TestStatus::TIMEOUT)
      BOOST_LOG_TRIVIAL(warning) << "test " << test << " timeout during profiling";
    profiler.mergeTrace(i, (status == TestStatus::PASS));
  }
  if (numNegative == 0) {
    BOOST_LOG_TRIVIAL(error) << "no negative tests";
    return false;
  }

  BOOST_LOG_TRIVIAL(info) << "number of positive tests: " << numPositive;
  const unsigned long MAX_PRINT_TESTS = 5;
  std::stringstream printTests;
  bool firstTest = true;
  for (int i=0; i<std::min(negativeTests.size(), MAX_PRINT_TESTS); i++) {
    if (!firstTest)
      printTests << ", ";
    firstTest = false;
    printTests << negativeTests[i];
  }
  if (MAX_PRINT_TESTS < negativeTests.size())
    printTests << ", ...";
  
  BOOST_LOG_TRIVIAL(info) << "number of negative tests: " << numNegative;
  BOOST_LOG_TRIVIAL(info) << "negative tests: [" << printTests.str() << "]";
  
  fs::path profile = profiler.getProfile();

  auto relatedTestIndexes = profiler.getRelatedTestIndexes();
  BOOST_LOG_TRIVIAL(info) << "number of locations: " << relatedTestIndexes.size();
  
  vector<fs::path> saFiles;

  BOOST_LOG_TRIVIAL(info) << "applying transfomation schemas to source files";
  for (int i=0; i<project.getFiles().size(); i++) {
    fs::path saFile = fs::path(cfg.dataDir) / getSchemaApplicationsFileName(i);
    saFiles.push_back(saFile);
    bool instrSuccess = project.instrumentFile(project.getFiles()[i], saFile, &profile);
    if (! instrSuccess) {
      BOOST_LOG_TRIVIAL(warning) << "transformation returned non-zero exit code";
    }
    if (! fs::exists(saFile)) {
      BOOST_LOG_TRIVIAL(error) << "failed to extract candidate locations";
      return false;
    }
  }

  project.saveInstrumentedFiles();

  BOOST_LOG_TRIVIAL(debug) << "loading candidate locations";
  vector<shared_ptr<SchemaApplication>> sas = loadSchemaApplications(saFiles);
  
  BOOST_LOG_TRIVIAL(debug) << "inferring types";
  for (auto sa : sas) {
    Type context;
    if (sa->context == LocationContext::CONDITION)
      context = Type::BOOLEAN;
    else
      context = Type::ANY;
    sa->original = correctTypes(sa->original, context);
  }

  vector<SearchSpaceElement> searchSpace;

  Runtime runtime;

  BOOST_LOG_TRIVIAL(info) << "generating search space";
  {
    fs::ofstream os(runtime.getSource());
    fs::ofstream oh(runtime.getHeader());
    searchSpace = generateSearchSpace(sas, os, oh);
  }

  BOOST_LOG_TRIVIAL(info) << "search space size: " << searchSpace.size();

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

  if (!cfg.searchSpaceFile.empty()) {
    auto path = fs::path(cfg.searchSpaceFile);
    BOOST_LOG_TRIVIAL(info) << "dumping search space: " << path;
    dumpSearchSpace(searchSpace, path, project.getFiles());
  }

  SearchEngine engine(tests, tester, runtime, getPartitionable(searchSpace), relatedTestIndexes);

  unsigned long last = 0;
  unsigned long patchCount = 0;
  unordered_set<unsigned long> fixLocations;

  while (last < searchSpace.size()) {
    last = engine.findNext(searchSpace, last);

    if (last < searchSpace.size()) {
      //FIXME: this logic with --all may be incorrect, needs to be simplified
      if (! cfg.generateAll && fixLocations.count(searchSpace[last].app->appId)) {
        last++;
        patchCount++;
        continue;
      }
      
      fs::path relpath = project.getFiles()[searchSpace[last].app->location.fileId].relpath;
      BOOST_LOG_TRIVIAL(info) << "plausible patch: " << visualizeChange(searchSpace[last]) 
                              << " with score " << std::setprecision(3) << simplicityScore(searchSpace[last])
                              << " in " << relpath.string() << ":" << searchSpace[last].app->location.beginLine;
      
      bool appSuccess = project.applyPatch(searchSpace[last]);
      if (! appSuccess) {
        BOOST_LOG_TRIVIAL(warning) << "patch application returned non-zero code";
      }

      project.savePatchedFiles();

      bool valid = true;

      if (cfg.validatePatches) {
        bool rebuildSuccess = project.build();
        if (! rebuildSuccess) {
          BOOST_LOG_TRIVIAL(warning) << "compilation with patch returned non-zero exit code";
        }

        BOOST_LOG_TRIVIAL(info) << "validating patch";
        vector<string> failing = getFailing(tester, tests);

        if (!failing.empty()) {
          valid = false;
          BOOST_LOG_TRIVIAL(warning) << "generated patch failed validation";
          for (auto &t : failing) {
            BOOST_LOG_TRIVIAL(info) << "failed test: " << t;
          }
        }

        if (cfg.generateAll) {
          project.restoreInstrumentedFiles();
          project.buildWithRuntime(runtime.getHeader());
        }
      }

      project.restoreOriginalFiles();

      if (!valid) {
        last++;
        continue;
      }

      fs::path patchFile = patchOutput;
      if (cfg.generateAll) {
        if (! fs::exists(patchFile)) {
          fs::create_directory(patchFile);
        }
        patchFile = patchFile / (std::to_string(patchCount) + ".patch");
      }

      patchCount++;
      fixLocations.insert(searchSpace[last].app->appId);

      unsigned fileId = searchSpace[last].app->location.fileId;

      project.computeDiff(project.getFiles()[fileId], patchFile);
      
      if (!cfg.generateAll)
        break;
    }

    last++;
  }

  SearchStatistics stat = engine.getStatistics();

  BOOST_LOG_TRIVIAL(info) << "candidates evaluated: " << stat.explorationCounter;
  BOOST_LOG_TRIVIAL(info) << "tests executed: " << stat.executionCounter;
  BOOST_LOG_TRIVIAL(info) << "executions with timeout: " << stat.timeoutCounter;
  if (stat.nonTimeoutTestTime != 0) {
    double executionsPerSec = (stat.nonTimeoutCounter * 1000.0) / stat.nonTimeoutTestTime;
    BOOST_LOG_TRIVIAL(info) << "execution speed: " << std::setprecision(3) << executionsPerSec << " exe/sec";
  }
  BOOST_LOG_TRIVIAL(info) << "plausible patches: " << patchCount;
  BOOST_LOG_TRIVIAL(info) << "fix locations: " << fixLocations.size();

  return patchCount > 0;
}

