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

#include <string>
#include <cstdlib>

#include <boost/log/trivial.hpp>

#include "Config.h"
#include "SearchEngine.h"


SearchEngine::SearchEngine(const std::vector<std::string> &tests,
                           TestingFramework &tester,
                           Runtime &runtime):
  tests(tests),
  tester(tester),
  runtime(runtime),
  candidateCounter(0),
  testCounter(0) {}

uint SearchEngine::findNext(const std::vector<SearchSpaceElement> &searchSpace, uint indexFrom) {

  setenv("F1X_WORKDIR", runtime.getWorkDir().string().c_str(), true);

  uint index = indexFrom;

  for (; index < searchSpace.size(); index++) {
    candidateCounter++;
    auto elem = searchSpace[index];
    setenv("F1X_ID", std::to_string(elem.id).c_str(), true);
    setenv("F1X_LOC", std::to_string(elem.buggy->location.locId).c_str(), true);
    bool passAll = true;
    for (auto &test : tests) {
      BOOST_LOG_TRIVIAL(debug) << "executing candidate " << elem.id << " with test " << test;
      testCounter++;
      passAll = tester.isPassing(test);
      if (!passAll)
        break;
    }
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
