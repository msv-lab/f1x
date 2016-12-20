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

#include "SearchEngine.h"


bool search(const std::vector<SearchSpaceElement> &searchSpace,
            const std::vector<std::string> tests,
            TestingFramework &tester,
            Runtime &runtime,
            SearchSpaceElement &patch) {
  BOOST_LOG_TRIVIAL(info) << "search space: " << searchSpace.size();

  uint candidateCounter = 0;
  uint testCounter = 0;
  bool found = false;
  
  setenv("F1X_WORKDIR", runtime.getWorkDir().string().c_str(), true);
  for (auto &elem : searchSpace) {
    candidateCounter++;
    setenv("F1X_ID", std::to_string(elem.id).c_str(), true);
    setenv("F1X_LOC", std::to_string(elem.buggy->location.locId).c_str(), true);
    bool passAll = true;
    for (auto &test : tests) {
      BOOST_LOG_TRIVIAL(debug) << "executing candidate " << elem.id << " with test " << test;
      if (passAll) {
        testCounter++;
        passAll = tester.isPassing(test);
      }
      if (!passAll)
        break;
    }
    if (passAll) {
      patch = elem;
      found = true;
      break;
    }
  }

  BOOST_LOG_TRIVIAL(info) << "candidates evaluated: " << candidateCounter;
  BOOST_LOG_TRIVIAL(info) << "tests executed: " << testCounter;

  return found;
}
