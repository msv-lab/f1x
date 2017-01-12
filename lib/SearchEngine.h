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

#pragma once

#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include "RepairUtil.h"
#include "Project.h"
#include "Runtime.h"

class SearchEngine {
 public:
  SearchEngine(const std::vector<std::string> &tests,
               TestingFramework &tester,
               Runtime &runtime,
               const Config &cfg,
               std::map<std::string, std::vector<int>> relatedTestIndex);

  uint findNext(const std::vector<SearchSpaceElement> &searchSpace, uint indexFrom);
  uint getCandidateCount();
  uint getTestCount();

 private:
 
  std::vector<int> changeSensitivity(std::vector<int> iVec, int index);
  std::vector<std::string> tests;
  TestingFramework tester;
  Runtime runtime;
  uint candidateCounter;
  uint testCounter;
  Config cfg;
  std::unordered_set<uint> failing;
  std::unordered_map<std::string, std::unordered_set<uint>> passing;
  std::map<std::string, std::vector<int>> testCaseSensitivity;
};
