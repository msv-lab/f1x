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

#include <string>
#include <cstdlib>
#include <sstream>

#include <boost/log/trivial.hpp>

#include "F1XConfig.h"
#include "SearchEngine.h"
#include "RepairUtil.h"

using std::unordered_set;
using std::unordered_map;
using std::string;

SearchEngine::SearchEngine(const std::vector<std::string> &tests,
                           TestingFramework &tester,
                           Runtime &runtime,
                           const Config &cfg,
                           std::map<string, std::vector<int>> relatedTestIndex):
  tests(tests),
  tester(tester),
  runtime(runtime),
  cfg(cfg),
  candidateCounter(0),
  testCounter(0) {

  failing = {};
  passing = {};
  for (auto &test : tests) {
    passing[test] = {};
  }
  testCaseSensitivity = relatedTestIndex;
}

uint SearchEngine::findNext(const std::vector<SearchSpaceElement> &searchSpace, uint indexFrom) {

  uint index = indexFrom;

  for (; index < searchSpace.size(); index++) {
    candidateCounter++;
    auto elem = searchSpace[index];
    if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
      if (failing.count(elem.id))
        continue;
    }
    setenv("F1X_ID", std::to_string(elem.id).c_str(), true);
    setenv("F1X_LOC", std::to_string(elem.buggy->location.locId).c_str(), true);
    bool passAll = true;
    
    std::shared_ptr<CandidateLocation> cl = elem.buggy;
    
    std::ostringstream repairLoc;
    repairLoc << cl->location.beginLine << "_"
                 << cl->location.beginColumn << "_"
                 << cl->location.endLine << "_"
                 << cl->location.endColumn << "_"
                 << cl->location.fileId;
    std::vector<int> testOrder = testCaseSensitivity[repairLoc.str()];
    
    BOOST_LOG_TRIVIAL(debug) << "***************before sorting********************";
    for (int i=0; i<testOrder.size(); i++)
      BOOST_LOG_TRIVIAL(debug) << testOrder[i] << " ";
    BOOST_LOG_TRIVIAL(debug) << "*************************************************";
    
    for (int i=0; i<testOrder.size(); i++) {
      auto test = tests[testOrder[i]];
      BOOST_LOG_TRIVIAL(debug) << "executing candidate " << elem.id << " with test " << test;
      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        if (passing[test].count(elem.id))
          continue;
        runtime.cleanPartition(elem.buggy->location.locId);
      }
      testCounter++;
      passAll = tester.isPassing(test);
      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        unordered_set<uint> partition = runtime.getPartition(elem.buggy->location.locId);
        for (auto &pel: partition) {
          BOOST_LOG_TRIVIAL(debug) << "same partition " << pel;
        }
        if (passAll) {
          passing[test].insert(elem.id);
          passing[test].insert(partition.begin(), partition.end());
        } else {
          failing.insert(elem.id);
          failing.insert(partition.begin(), partition.end());
        }
      }
      if (!passAll)
      {
        testCaseSensitivity[repairLoc.str()] = changeSensitivity(testOrder,i);
        break;
      }
    }
    BOOST_LOG_TRIVIAL(debug) << "*****************after sorting*******************";
    testOrder = testCaseSensitivity[repairLoc.str()];
    for (int i=0; i<testOrder.size(); i++)
      BOOST_LOG_TRIVIAL(debug) << testOrder[i];
    BOOST_LOG_TRIVIAL(debug) << "*************************************************";
    if (passAll) {
      return index;
    }
  }

  return index;
}

uint SearchEngine::getCandidateCount() {
  return candidateCounter;
}

uint SearchEngine::getTestCount() {
  return testCounter;
}

std::vector<int> SearchEngine::changeSensitivity(std::vector<int> iVec, int index)
{
    std::vector<int>::iterator it = iVec.begin()+index;
    int temp = iVec[index];
    iVec.erase(it);
    iVec.insert(iVec.begin(), temp);
    return iVec;
}
