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
#include <sstream>
#include <cstdlib>
#include <sys/wait.h>

// for shared memory:
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <boost/log/trivial.hpp>

#include "Util.h"
#include "Global.h"
#include "Runtime.h"
#include "Config.h"

namespace fs = boost::filesystem;
using std::vector;
using std::unordered_set;


Runtime::Runtime() {

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
  unsigned long index = 0;
  for (auto &id : ids) {
    partition[index] = id;
    index++;
  }
  partition[index] = INPUT_TERMINATOR;
}

unordered_set<F1XID> Runtime::getPartition() {
  unordered_set<F1XID> result;
  unsigned long index = 0;
  while (!(partition[index] == OUTPUT_TERMINATOR)) {
    if (partition[index] == INPUT_TERMINATOR) {
      BOOST_LOG_TRIVIAL(debug) << "wrongly terminated partition";
      return unordered_set<F1XID>();
    }
    if (index > MAX_PARTITION_SIZE) {
      BOOST_LOG_TRIVIAL(debug) << "unterminated partition";
      return unordered_set<F1XID>();
    }
    result.insert(partition[index]);
    index++;
  }
  return result;
}

boost::filesystem::path Runtime::getHeader() {
return fs::path(cfg.dataDir) / RUNTIME_HEADER_FILE_NAME;
}

boost::filesystem::path Runtime::getSource() {
return fs::path(cfg.dataDir) / RUNTIME_SOURCE_FILE_NAME;
}

// FIXME: this should probably be built using F1X_PROJECT_CC instead of hard-coded compiler
bool Runtime::compile() {
  BOOST_LOG_TRIVIAL(info) << "compiling analysis runtime";
  FromDirectory dir(fs::path(cfg.dataDir));
  std::stringstream cmd;
  cmd << RUNTIME_COMPILER
      << " " << RUNTIME_OPTIMIZATION
      << " -fPIC"
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
  unsigned long status = std::system(cmd.str().c_str());
  return WEXITSTATUS(status) == 0;
}
