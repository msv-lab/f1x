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

#include <boost/filesystem/fstream.hpp>

#include <boost/log/trivial.hpp>

#include "RepairUtil.h"
#include "Profiler.h"

namespace fs = boost::filesystem;
using std::string;
using std::vector;
using std::unordered_set;
using std::unordered_map;

Profiler::Profiler(const fs::path &workDir, const Config &cfg): 
  workDir(workDir),
  cfg(cfg) {};


boost::filesystem::path Profiler::getHeader() {
  return workDir / PROFILE_HEADER_FILE_NAME;
}

boost::filesystem::path Profiler::getSource() {
  return workDir / PROFILE_SOURCE_FILE_NAME;
}

unordered_map<Location, vector<int>> Profiler::getRelatedTestIndexes() {
  return relatedTestIndexes;
}

bool Profiler::compile() {
  BOOST_LOG_TRIVIAL(debug) << "compiling profile runtime";
  {
    fs::ofstream source(getSource());
    source << "#include <stdio.h>\n"
           << "#include \"" << PROFILE_HEADER_FILE_NAME << "\"\n";
    source << "void __f1x_trace(int fileId, int beginLine, int beginColumn, int endLine, int endColumn) {\n"
           << "  FILE * file = fopen(\""<< (workDir / TRACE_FILE_NAME).string() << "\", \"a+\");\n"
           << "  fprintf(file, \"%d %d %d %d %d\\n\", fileId, beginLine, beginColumn, endLine, endColumn);\n"
           << "  fclose(file);\n"
           << "}\n";

    fs::ofstream header(getHeader());
    header << "#ifdef __cplusplus" << "\n"
           << "extern \"C\" {" << "\n"
           << "#endif" << "\n";
    header << "void __f1x_trace(int fileId, int beginLine, int beginColumn, int endLine, int endColumn);\n";
    header << "#ifdef __cplusplus" << "\n"
           << "}" << "\n"
           << "#endif" << "\n";
  }
  FromDirectory dir(workDir);
  std::stringstream cmd;
  cmd << cfg.runtimeCompiler << " -O2 -fPIC"
      << " " << PROFILE_SOURCE_FILE_NAME
      << " -shared"
      << " -o libf1xrt.so";
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  uint status = std::system(cmd.str().c_str());
  return status == 0;
}

void Profiler::clearTrace() {
  fs::path traceFile = workDir / TRACE_FILE_NAME;
  fs::ofstream out;
  out.open(traceFile, std::ofstream::out | std::ofstream::trunc);
  out.close();
}

void Profiler::mergeTrace(int testIndex, bool isPassing) {
  fs::path traceFile = workDir / TRACE_FILE_NAME;
  fs::ifstream infile(traceFile);
  unordered_set<Location> covered;
  if(infile) {
    string line;
    while (std::getline(infile, line)) {
      Location loc;
      std::istringstream iss(line);
      iss >> loc.fileId >> loc.beginLine >> loc.beginColumn >> loc.endLine >> loc.endColumn;
      if(! relatedTestIndexes.count(loc)) {
        relatedTestIndexes[loc] = vector<int>();
      }
      vector<int> current = relatedTestIndexes[loc];
      if (std::find(current.begin(), current.end(), testIndex) == current.end()) {
        relatedTestIndexes[loc].push_back(testIndex);
      }
      covered.insert(loc);
    }
  }
  if (covered.empty()) {
    BOOST_LOG_TRIVIAL(debug) << "test no. " << testIndex << " produces empty trace";
  }

  if (!isPassing) {
    if (interestingLocations.empty()) {
      interestingLocations.insert(covered.begin(), covered.end());
    } else {
      //NOTE: here is intentionally intersection:
      unordered_set<Location> newInteresting;
      for (auto &loc : interestingLocations) {
        if (covered.count(loc)) {
          newInteresting.insert(loc);
        }
      }
      std::swap(interestingLocations, newInteresting);
    }
  }
}

fs::path Profiler::getProfile() {
  fs::path profileFile = workDir/ PROFILE_FILE_NAME;
  fs::ofstream outfile(profileFile, std::ios::app);

  //NOTE: removing uninteresting test indexes
  unordered_map<Location, vector<int>>::iterator it = relatedTestIndexes.begin();
  while(it != relatedTestIndexes.end()) {
    if(interestingLocations.find(it->first) == interestingLocations.end()) {
      relatedTestIndexes.erase(it);
    }
    it++;
  }

  for (auto &loc : interestingLocations) {
    outfile << loc.fileId << " "
            << loc.beginLine << " "
            << loc.beginColumn << " "
            << loc.endLine << " "
            << loc.endColumn << "\n";
  }

  return profileFile;
}
 
