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

#include <string>
#include <cstdlib>
#include <sstream>
#include <memory>
#include <chrono>

#include <boost/log/trivial.hpp>

#include "Config.h"
#include "SearchEngine.h"
#include "Util.h"

using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;
using std::string;
using std::to_string;

const unsigned SHOW_PROGRESS_STEP = 10;

SearchEngine::SearchEngine(const std::vector<std::string> &tests,
                           TestingFramework &tester,
                           Runtime &runtime,
                           const Config &cfg,
                           shared_ptr<unordered_map<unsigned long, unordered_set<F1XID>>> partitionable,
                           std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes):
  tests(tests),
  tester(tester),
  runtime(runtime),
  cfg(cfg),
  partitionable(partitionable),
  relatedTestIndexes(relatedTestIndexes) {
  
  stat.explorationCounter = 0;
  stat.executionCounter = 0;
  stat.timeoutCounter = 0;
  stat.nonTimeoutCounter = 0;
  stat.nonTimeoutTestTime = 0;

  progress = 0;

  //FIXME: I should use evaluation table instead
  failing = {};
  passing = {};
  for (auto &test : tests) {
    passing[test] = {};
  }
}

unsigned long SearchEngine::findNext(const std::vector<SearchSpaceElement> &searchSpace,
                                     unsigned long fromIdx) {

  unsigned long candIdx = fromIdx;
  for (; candIdx < searchSpace.size(); candIdx++) {
    stat.explorationCounter++;
    if ((100 * candIdx) / searchSpace.size() >= progress) {
      BOOST_LOG_TRIVIAL(info) << "exploration progress: " << progress << "%";
      progress += SHOW_PROGRESS_STEP;
    }

    const SearchSpaceElement &elem = searchSpace[candIdx];

    if (cfg.exploration == Exploration::TEST_EQUIVALENCE) {
      if (failing.count(elem.id))
        continue;
    }

    InEnvironment env({ { "F1X_APP", to_string(elem.app->appId) },
                        { "F1X_ID_BASE", to_string(elem.id.base) },
                        { "F1X_ID_INT2", to_string(elem.id.int2) },
                        { "F1X_ID_BOOL2", to_string(elem.id.bool2) },
                        { "F1X_ID_COND3", to_string(elem.id.cond3) },
                        { "F1X_ID_PARAM", to_string(elem.id.param) } });

    bool passAll = true;
    
    std::shared_ptr<SchemaApplication> sa = elem.app;
    
    std::vector<unsigned> testOrder = relatedTestIndexes[elem.app->location];
    
    for (unsigned orderIdx = 0; orderIdx < testOrder.size(); orderIdx++) {
      auto test = tests[testOrder[orderIdx]];

      if (cfg.exploration == Exploration::TEST_EQUIVALENCE) {
        if (passing[test].count(elem.id))
          continue;
        //FIXME: select unexplored candidates
        runtime.setPartition((*partitionable)[elem.app->appId]);
      }

      BOOST_LOG_TRIVIAL(debug) << "executing candidate " << visualizeF1XID(elem.id) 
                               << " with test " << test;

      std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

      TestStatus status = tester.execute(test);

      std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

      stat.executionCounter++;
      if (status != TestStatus::TIMEOUT) {
        stat.nonTimeoutCounter++;
        stat.nonTimeoutTestTime += std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
      } else {
        stat.timeoutCounter++;
      }

      switch (status) {
      case TestStatus::PASS:
        BOOST_LOG_TRIVIAL(debug) << "PASS";
        break;
      case TestStatus::FAIL:
        BOOST_LOG_TRIVIAL(debug) << "FAIL";
        break;
      case TestStatus::TIMEOUT:
        BOOST_LOG_TRIVIAL(debug) << "TIMEOUT";
        break;
      }

      passAll = (status == TestStatus::PASS);
        
      if (cfg.exploration == Exploration::TEST_EQUIVALENCE) {
        unordered_set<F1XID> partition = runtime.getPartition();
        if (partition.empty()) {
          BOOST_LOG_TRIVIAL(warning) << "partitioning failed for "
                                     << visualizeF1XID(elem.id)
                                     << " with test " << test;
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
          prioritizeTest(relatedTestIndexes[elem.app->location], orderIdx);
        }
        break;
      }
    }

    if (passAll) {
      return candIdx;
    }
  }

  return candIdx;
}

SearchStatistics SearchEngine::getStatistics() {
  return stat;
}

void SearchEngine::prioritizeTest(std::vector<unsigned> &testOrder, unsigned index) {
    std::vector<unsigned>::iterator it = testOrder.begin() + index;
    unsigned temp = testOrder[index];
    testOrder.erase(it);
    testOrder.insert(testOrder.begin(), temp);
}
