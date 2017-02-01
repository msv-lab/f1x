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
#include <sstream>
#include <cstdlib>
#include <sys/wait.h>

// for shared memory:
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <boost/log/trivial.hpp>

#include "RepairUtil.h"
#include "Runtime.h"

namespace fs = boost::filesystem;
using std::vector;
using std::unordered_set;


Runtime::Runtime(const fs::path &workDir, const Config &cfg): 
  workDir(workDir),
  cfg(cfg) {

  size_t size = sizeof(F1XID) * MAX_PARTITION_SIZE;
  int fd = shm_open(PARTITION_FILE_NAME.c_str(),
                    O_CREAT | O_RDWR,
                    S_IRUSR | S_IWUSR);
  ftruncate(fd, size);
  partition = (F1XID*) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED , fd, 0);
  close(fd);
  };

void Runtime::setPartition(std::unordered_set<F1XID> ids) {
  assert(ids.size() < MAX_PARTITION_SIZE);
  ulong index = 0;
  for (auto &id : ids) {
    partition[index] = id;
    index++;
  }
  partition[index] = INPUT_TERMINATOR;
}

unordered_set<F1XID> Runtime::getPartition() {
  unordered_set<F1XID> result;
  ulong index = 0;
  while (!(partition[index] == OUTPUT_TERMINATOR)) {
    if (partition[index] == INPUT_TERMINATOR) {
      BOOST_LOG_TRIVIAL(warning) << "wrongly terminated partition";
      return unordered_set<F1XID>();
    }
    if (index > MAX_PARTITION_SIZE) {
      BOOST_LOG_TRIVIAL(warning) << "unterminated partition";
      return unordered_set<F1XID>();
    }
    result.insert(partition[index]);
    index++;
  }
  return result;
}

boost::filesystem::path Runtime::getHeader() {
  return workDir / RUNTIME_HEADER_FILE_NAME;
}

boost::filesystem::path Runtime::getSource() {
  return workDir / RUNTIME_SOURCE_FILE_NAME;
}

bool Runtime::compile() {
  BOOST_LOG_TRIVIAL(info) << "compiling meta program runtime";
  FromDirectory dir(workDir);
  std::stringstream cmd;
  cmd << cfg.runtimeCompiler << " -O2 -fPIC"
      << " " << RUNTIME_SOURCE_FILE_NAME
      << " -shared"
      << " -lrt" // this is for shared memory
      << " -std=c++11" // this is for initializers
      << " -o libf1xrt.so";
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  ulong status = std::system(cmd.str().c_str());
  return WEXITSTATUS(status) == 0;
}
