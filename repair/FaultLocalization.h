#pragma once

#include <unordered_map>
#include <unordered_set>

#include "Project.h"
#include "Util.h"


class FaultLocalization {
public:
  FaultLocalization(const std::vector<std::string> &tests, const TestingFramework &tester);
  std::vector<boost::filesystem::path> localize(std::vector<boost::filesystem::path> allFiles);

private:
  TestingFramework tester;
  std::vector<std::string> tests;
  boost::filesystem::path coverageDir;
  std::unordered_map<std::string, std::unordered_set<unsigned>> loadCoverage(std::string test);
};
