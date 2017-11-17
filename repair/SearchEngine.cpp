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
#include "Global.h"
#include "SearchEngine.h"
#include "Util.h"

using std::unordered_set;
using std::unordered_map;
using std::shared_ptr;
using std::string;
using std::to_string;

namespace fs = boost::filesystem;

const unsigned SHOW_PROGRESS_STEP = 10;

SearchEngine::SearchEngine(const std::vector<std::string> &tests,
                           TestingFramework &tester,
                           Runtime &runtime,
                           shared_ptr<unordered_map<unsigned long, unordered_set<PatchID>>> partitionable,
                           std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes):
  tests(tests),
  tester(tester),
  runtime(runtime),
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

  coverageDir = fs::path(cfg.dataDir) / "patch-coverage";
  fs::create_directory(coverageDir);

}


void SearchEngine::showProgress(unsigned long current, unsigned long total) {
      if ((100 * current) / total >= progress) {
      BOOST_LOG_TRIVIAL(info) << "exploration progress: " << progress << "%";
      progress += SHOW_PROGRESS_STEP;
    }
}


SearchStatistics SearchEngine::getStatistics() {
  return stat;
}


std::unordered_map<std::string, std::unordered_map<PatchID, std::shared_ptr<Coverage>>> SearchEngine::getCoverageSet() {
  return coverageSet;
}


//FIXME: this probably does not work. Tests should be prioritized based on global score
void SearchEngine::prioritizeTest(std::vector<unsigned> &testOrder, unsigned index) {
    std::vector<unsigned>::iterator it = testOrder.begin() + index;
    unsigned temp = testOrder[index];
    testOrder.erase(it);
    testOrder.insert(testOrder.begin(), temp);
}


unsigned long SearchEngine::findNext(const std::vector<Patch> &searchSpace,
                                     unsigned long from) {

  unsigned long index = from;
  for (; index < searchSpace.size(); index++) {
    stat.explorationCounter++;
    showProgress(index, searchSpace.size());

    const Patch &elem = searchSpace[index];

    if (cfg.valueTEQ) {
      if (failing.count(elem.id))
        continue;
    }

    InEnvironment env({ { "F1X_APP", to_string(elem.app->id) },
                        { "F1X_ID_BASE", to_string(elem.id.base) },
                        { "F1X_ID_INT2", to_string(elem.id.int2) },
                        { "F1X_ID_BOOL2", to_string(elem.id.bool2) },
                        { "F1X_ID_COND3", to_string(elem.id.cond3) },
                        { "F1X_ID_PARAM", to_string(elem.id.param) } });

    bool passAll = true;

    std::vector<unsigned> testOrder = relatedTestIndexes[elem.app->location];

    for (unsigned orderIndex = 0; orderIndex < testOrder.size(); orderIndex++) {
      auto test = tests[testOrder[orderIndex]];

      if (cfg.valueTEQ) {
        if (passing[test].count(elem.id))
          continue;

        //FIXME: select only unexplored candidates
        runtime.setPartition((*partitionable)[elem.app->id]);
      }

      BOOST_LOG_TRIVIAL(debug) << "executing candidate " << visualizePatchID(elem.id) 
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

      if (cfg.valueTEQ) {
        unordered_set<PatchID> partition = runtime.getPartition();
        if (partition.empty()) {
          //NOTE: it should contain at least the current element
          BOOST_LOG_TRIVIAL(warning) << "partitioning failed for "
                                     << visualizePatchID(elem.id)
                                     << " with test " << test;
        }

        if (cfg.patchPrioritization == PatchPrioritization::SEMANTIC_DIFF) {
          fs::path coverageFile = coverageDir / (test + "_" + std::to_string(index) + ".xml");
          std::shared_ptr<Coverage> curCoverage(new Coverage(extractAndSaveCoverage(coverageFile)));

          if (!coverageSet.count(test))
            coverageSet[test] = std::unordered_map<PatchID, std::shared_ptr<Coverage>>();

          coverageSet[test][elem.id] = curCoverage;
          for (auto &id : partition)
            coverageSet[test][id] = curCoverage;
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
          prioritizeTest(relatedTestIndexes[elem.app->location], orderIndex);
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
