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
#include <boost/log/trivial.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "Project.h"
#include "RepairUtil.h"
#include "Config.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;

using std::string;
using std::vector;
using std::map;

const string BACKUP_PREFIX = "backup";


void addClangHeadersToCompileDB(fs::path projectRoot) {
  fs::path compileDB("compile_commands.json");
  fs::path path = projectRoot / compileDB;
  json::Document db;
  {
    fs::ifstream ifs(path);
    json::IStreamWrapper isw(ifs);
    db.ParseStream(isw);
  }

  for (auto& entry : db.GetArray()) {
    // assume there is always a first space in which we insert our include
    // FIXME: add escape for the spaces in the include path
    string command = entry.GetObject()["command"].GetString();
    uint index = command.find(" ");
    string newCommand = command.substr(0, index) + " -I" + F1X_CLANG_INCLUDE + " " + command.substr(index);
    entry.GetObject()["command"].SetString(newCommand.c_str(), db.GetAllocator());
  }
  
  {
    fs::ofstream ofs(path);
    json::OStreamWrapper osw(ofs);
    json::Writer<json::OStreamWrapper> writer(osw);
    db.Accept(writer);
  }
}


Project::Project(const boost::filesystem::path &root,
                 const std::vector<boost::filesystem::path> &files,
                 const std::string &buildCmd,
                 const boost::filesystem::path &workDir):
  root(root),
  files(files),
  buildCmd(buildCmd),
  workDir(workDir) {}

fs::path Project::getRoot() const {
  return root;
}

vector<fs::path> Project::getFiles() const {
  return files;
}

bool Project::initialBuild() {
  BOOST_LOG_TRIVIAL(info) << "building project using \"" << buildCmd << "\"";

  FromDirectory dir(root);
  InEnvironment env(map<string, string> { {"CC", "f1x-cc"} });

  string cmd = "f1x-bear " + buildCmd;
  uint status = std::system(cmd.c_str());

  // FIXME: check that compilation database is non-empty
  addClangHeadersToCompileDB(root);

  return (status == 0);
}

bool Project::buildWithRuntime(const fs::path &header) {
  BOOST_LOG_TRIVIAL(info) << "rebuilding project with f1x runtime";
  FromDirectory dir(root);
  // FIXME: I need to append LD_LIBRARY_PATH
  InEnvironment env({ {"F1X_RUNTIME_H", header.string()},
                      {"F1X_RUNTIME_LIB", workDir.string()},
                      {"LD_LIBRARY_PATH", workDir.string()} });
  string cmd = buildCmd;
  uint status = std::system(cmd.c_str());
  return (status == 0);
}

void Project::saveFilesWithPrefix(const string &prefix) {
  for (int i = 0; i < files.size(); i++) {
    fs::copy(root / files[i], workDir / fs::path(prefix + std::to_string(i) + ".c"));
  }
}

void Project::backupFiles() {
  saveFilesWithPrefix(BACKUP_PREFIX);
}

void Project::restoreFiles() {
  for (int i = 0; i < files.size(); i++) {
    if(fs::exists(root / files[i])) {
      fs::remove(root / files[i]);
    }
    fs::copy(workDir / fs::path(BACKUP_PREFIX + std::to_string(i) + ".c"), root / files[i]);
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
  
  fs::path fromFile = workDir / fs::path(BACKUP_PREFIX + std::to_string(id) + ".c");
  fs::path toFile = root / file;
  string cmd = "diff " + fromFile.string() + " " + toFile.string() + " >> " + output.string();
  std::system(cmd.c_str());
}


TestingFramework::TestingFramework(const Project &project,
                                   const boost::filesystem::path &driver,
                                   const uint testTimeout):
  project(project),
  driver(driver),
  testTimeout(testTimeout) {}

bool TestingFramework::isPassing(const std::string &testId) {
  // FIXME: respect timeout
  FromDirectory dir(project.getRoot());
  string cmd = driver.string() + " " + testId;
  uint status = std::system(cmd.c_str());
  return (status == 0);
}
