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

#include <boost/filesystem/fstream.hpp>

#include "Runtime.h"

namespace fs = boost::filesystem;
using std::vector;


Runtime::Runtime(const fs::path &workDir): 
  workDir(workDir) {};

void Runtime::setPartiotion(uint locId, uint candidateId, vector<uint> space) {
  fs::ofstream out(workDir / std::to_string(locId));
  out << candidateId;
  for (auto &el : space) {
    out << " " << el;
  }
}

vector<uint> Runtime::getPartiotion(uint locId) {
  vector<uint> result;
  fs::ifstream in(workDir / std::to_string(locId));
  uint id;
  in >> id;
while (in >> id) {
    result.push_back(id);
  }
  return result;
}

fs::path Runtime::getWorkDir() {
  return workDir;
}
