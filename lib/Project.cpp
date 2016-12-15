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

#include <boost/filesystem/fstream.hpp>

#include "Project.h"
#include "RepairUtil.h"

namespace fs = boost::filesystem;

using std::string;
using std::vector;


Project::Project(const boost::filesystem::path &root,
                 const std::vector<boost::filesystem::path> &files,
                 const boost::filesystem::path &workDir):
  root(root),
  files(files),
  workDir(workDir) {}

fs::path Project::getRoot() {
  return root;
}

void Project::backupFiles() {
  for (int i = 0; i < files.size(); i++) {
    fs::copy(root / files[i], workDir / fs::path(std::to_string(i) + ".c"));
  }
}

void Project::restoreFiles() {
  for (int i = 0; i < files.size(); i++) {
    if(fs::exists(root / files[i])) {
      fs::remove(root / files[i]);
    }
    fs::copy(workDir / fs::path(std::to_string(i) + ".c"), root / files[i]);
  }
}

void Project::computeDiff(const fs::path &file,
                          const fs::path &output) {
  {
    fs::path a = fs::path("a") / file;
    fs::path b = fs::path("b") / file;
    fs::ofstream ofs(output);
    ofs << "--- " << a.string() << "\n"
        << "+++ " << b.string() << "\n";
  }
  uint id = std::find(files.begin(), files.end(), file) - files.begin();
  assert(id < files.size());
  
  fs::path fromFile = workDir / fs::path(std::to_string(id) + ".c");
  fs::path toFile = root / file;
  string cmd = "diff " + fromFile.string() + " " + toFile.string() + " >> " + output.string();
  std::system(cmd.c_str());
}


TestingFramework::TestingFramework(const Project &project,
                                   const boost::filesystem::path &driver):
  project(project),
  driver(driver) {}

bool TestingFramework::isPassing(const std::string &testId) {
  FromDirectory dir(project.getRoot());
  string cmd = driver.string() + " " + testId;
  uint status = std::system(cmd.c_str());
  return (status == 0);
}
