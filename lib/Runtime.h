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

#include <unordered_set>
#include <boost/filesystem.hpp>
#include "F1XConfig.h"
#include "RepairUtil.h"
#include <string>
#include <sstream>

const std::string PARTITION_IN = "partition.in";
const std::string PARTITION_OUT = "partition.out";

class Runtime {
 public:
  Runtime(const boost::filesystem::path &workDir, const Config &cfg, const std::string source, const std::string header);

  void setPartition(std::unordered_set<F1XID> ids);
  std::unordered_set<F1XID> getPartition();
  boost::filesystem::path getWorkDir();
  boost::filesystem::path getSource();
  boost::filesystem::path getHeader();
  bool compile();

 private:
  boost::filesystem::path workDir;
  Config cfg;
  
  std::string source;
  std::string header;
};
