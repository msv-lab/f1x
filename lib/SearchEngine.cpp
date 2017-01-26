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
#include <memory>

#include <boost/log/trivial.hpp>

#include "F1XConfig.h"
#include "SearchEngine.h"
#include "RepairUtil.h"

using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;
using std::string;
using std::to_string;

SearchEngine::SearchEngine(const std::vector<std::string> &tests,
                           TestingFramework &tester,
                           Runtime &runtime,
                           const Config &cfg,
                           shared_ptr<unordered_map<uint, unordered_set<F1XID>>> groupable,
                           std::unordered_map<Location, std::vector<int>> relatedTestIndexes):
  tests(tests),
  tester(tester),
  runtime(runtime),
  cfg(cfg),
  groupable(groupable),
  relatedTestIndexes(relatedTestIndexes) {
  
  stat.explorationCounter = 0;
  stat.executionCounter = 0;

  //FIXME: I should use evaluation table instead
  failing = {};
  passing = {};
  for (auto &test : tests) {
    passing[test] = {};
  }
}

uint SearchEngine::findNext(const std::vector<SearchSpaceElement> &searchSpace, uint indexFrom) {

  uint index = indexFrom;
  for (; index < searchSpace.size(); index++) {
    stat.explorationCounter++;
    const SearchSpaceElement &elem = searchSpace[index];
    uint appId = elem.app->appId;
    if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
      if (failing.count(elem.id))
        continue;
    }
    InEnvironment env({ { "F1X_ID", to_string(elem.id.base) },
                        { "F1X_ID_INT2", to_string(elem.id.int2) },
                        { "F1X_ID_BOOL2", to_string(elem.id.bool2) },
                        { "F1X_ID_COND3", to_string(elem.id.cond3) },
                        { "F1X_ID_PARAM", to_string(elem.id.param) },
                        { "F1X_APP", to_string(appId) } });
    bool passAll = true;
    
    std::shared_ptr<SchemaApplication> sa = elem.app;
    
    std::vector<int> testOrder = relatedTestIndexes[elem.app->location];
    
    for (int i = 0; i < testOrder.size(); i++) {
      auto test = tests[testOrder[i]];
      BOOST_LOG_TRIVIAL(debug) << "executing candidate " << visualizeF1XID(elem.id) 
                               << " with test " << test;
      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        if (passing[test].count(elem.id))
          continue;
        runtime.clearPartition();
        runtime.setPartition((*groupable)[appId]);
      }
      stat.executionCounter++;
      passAll = tester.isPassing(test);
      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        unordered_set<F1XID> partition = runtime.getPartition();
        for (auto &pel: partition) {
          BOOST_LOG_TRIVIAL(debug) << "same partition " << visualizeF1XID(pel);
        }
        if (passAll) {
          passing[test].insert(elem.id);
          passing[test].insert(partition.begin(), partition.end());
        } else {
          failing.insert(elem.id);
          failing.insert(partition.begin(), partition.end());
        }
      }
      if (!passAll) {
        if (cfg.testPrioritization == TestPrioritization::MAX_FAILING) {
          changeSensitivity(relatedTestIndexes[elem.app->location], i);
        }
        break;
      }
    }

    if (passAll) {
      return index;
    }
  }

  return index;
}

SearchStatistics SearchEngine::getStatistics() {
  return stat;
}

void SearchEngine::changeSensitivity(std::vector<int> &testOrder, int index)
{
    std::vector<int>::iterator it = testOrder.begin() + index;
    int temp = testOrder[index];
    testOrder.erase(it);
    testOrder.insert(testOrder.begin(), temp);
}
