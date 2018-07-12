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

#pragma once

#ifdef __cplusplus
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include "Util.h"
#include "Project.h"
#include "Runtime.h"
#include "FaultLocalization.h"

struct SearchStatistics {
  unsigned long explorationCounter;
  unsigned long executionCounter;
  unsigned long timeoutCounter;
  unsigned long nonTimeoutCounter;
  unsigned long nonTimeoutTestTime;
};

class SearchEngine {
 public:
  SearchEngine(const std::vector<Patch> &searchSpace,
               const std::vector<std::string> &tests,
               TestingFramework &tester,
               Runtime &runtime,
               std::shared_ptr<std::unordered_map<unsigned long, std::unordered_set<PatchID>>> partitionable,
               std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes);

  unsigned long findNext(const std::vector<Patch> &searchSpace, unsigned long fromIdx);
  std::unordered_map<std::string, std::unordered_map<PatchID, std::shared_ptr<Coverage>>> getCoverageSet();
  SearchStatistics getStatistics();
  void showProgress(unsigned long current, unsigned long total);
  unsigned long evaluatePatchWithNewTest(__string &test);
  const char* getWorkingDir();
  void getPatchLoc(int &length, int *& array);

 private:
  bool executeCandidate(const Patch elem, __string &test, int index);
  void prioritizeTest(std::vector<unsigned> &testOrder, unsigned index);
  std::vector<std::string> tests;
  TestingFramework tester;
  Runtime runtime;
  SearchStatistics stat;
  const std::vector<Patch> searchSpace;
  unsigned long progress;
  std::shared_ptr<std::unordered_map<unsigned long, std::unordered_set<PatchID>>> partitionable;
  std::unordered_set<PatchID> failing;
  std::unordered_map<std::string, std::unordered_set<PatchID>> passing;
  std::unordered_map<std::string, std::unordered_map<PatchID, std::shared_ptr<Coverage>>> coverageSet;
  std::unordered_map<Location, std::vector<unsigned>> relatedTestIndexes;
  boost::filesystem::path coverageDir;
  std::unordered_set<Location> vLocs;
};
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct C_SearchEngine;
const char* c_getWorkingDir(struct C_SearchEngine*);
unsigned long c_fuzzPatch(struct C_SearchEngine*, char*);
void c_getPatchLoc(struct C_SearchEngine* engine, int* length, int ** array);
#ifdef __cplusplus
}
#endif
#define SearchEngine_TO_CPP(o) (reinterpret_cast<SearchEngine*>(o))
#define SearchEngine_TO_C(o)   (reinterpret_cast<struct C_SearchEngine*>(o))
