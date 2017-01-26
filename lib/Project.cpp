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

#include <sstream>
#include <iomanip>

#include <boost/filesystem/fstream.hpp>
#include <boost/log/trivial.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "SystemConfig.h"
#include "F1XConfig.h"
#include "Project.h"
#include "RepairUtil.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;

using std::string;
using std::vector;
using std::map;
using std::shared_ptr;


bool projectFilesInCompileDB(fs::path root, vector<ProjectFile> files) {
  FromDirectory dir(root);
  
  fs::path compileDB("compile_commands.json");
  json::Document db;
  {
    fs::ifstream ifs(compileDB);
    json::IStreamWrapper isw(ifs);
    db.ParseStream(isw);
  }

  for (auto &file : files) {
    bool present = false;

    for (auto& entry : db.GetArray()) {
      string dbFile = entry.GetObject()["file"].GetString();
      if (fs::equivalent(fs::path(dbFile), file.relpath)) {
        present = true;
      }
    }

    if (! present) {
      BOOST_LOG_TRIVIAL(warning) << "file " << file.relpath
                                 << " is not in compilation database";
      return false;
    }
  }

  return true; 
}

/* 
   1. Add Clang headers.
      By default a Clang tool searches for headers in a strange place, so we add them explicitly
   2. Add default definitions
      Sometime if the transformed program contains undefined symbolic, transformation fails
      (e.g. gmp-13420-13421, it may depend on compiler flags)
 */
void adjustCompileDB(fs::path projectRoot) {
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
    string includeCmd =  "-I" + F1X_CLANG_INCLUDE;
    if (command.find(includeCmd) == std::string::npos) {
      uint index = command.find(" ");
      command = command.substr(0, index) + " " + includeCmd + " " + command.substr(index);
    }
    string defineCmd =  "-D__f1x_app=0ul";
    if (command.find(defineCmd) == std::string::npos) {
      uint index = command.find(" ");
      command = command.substr(0, index) + " " + defineCmd + " " + command.substr(index);
    }
    entry.GetObject()["command"].SetString(command.c_str(), db.GetAllocator());
  }
  
  {
    fs::ofstream ofs(path);
    json::OStreamWrapper osw(ofs);
    json::Writer<json::OStreamWrapper> writer(osw);
    db.Accept(writer);
  }
}


Project::Project(const boost::filesystem::path &root,
                 const std::vector<ProjectFile> &files,
                 const std::string &buildCmd,
                 const boost::filesystem::path &workDir,
                 const Config &cfg):
  root(root),
  files(files),
  buildCmd(buildCmd),
  workDir(workDir),
  cfg(cfg) {
  saveOriginalFiles();
  }

Project::~Project() {
  restoreOriginalFiles();
}

fs::path Project::getRoot() const {
  return root;
}

vector<ProjectFile> Project::getFiles() const {
  return files;
}

bool Project::buildInEnvironment(const std::map<std::string, std::string> &environment,
                                 const std::string &baseCmd) {
  FromDirectory dir(root);
  InEnvironment env(environment);

  std::stringstream cmd;
  cmd << baseCmd;
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }

  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  uint status = std::system(cmd.str().c_str());

  return (status == 0);
}

std::pair<bool, bool> Project::initialBuild() {
  BOOST_LOG_TRIVIAL(info) << "building project and inferring compile commands";

  std::stringstream cmd;
  if (fs::exists(root / "compile_commands.json")) {
    BOOST_LOG_TRIVIAL(info) << "using existing compilation database (compile_commands.json)";
    cmd << buildCmd;
  } else {
    cmd << "f1x-bear " << buildCmd;
  }
  bool compilationSuccess = buildInEnvironment({ {"CC", "f1x-cc"} }, cmd.str());

  bool inferenceSuccess = fs::exists(root / "compile_commands.json");

  if (inferenceSuccess) {
    if (projectFilesInCompileDB(root, files)) {
      adjustCompileDB(root);
    } else {
      inferenceSuccess = false;
    }
  }

  return std::make_pair(compilationSuccess, inferenceSuccess);
}

bool Project::build() {
  BOOST_LOG_TRIVIAL(info) << "building project";

  bool success = buildInEnvironment({ {"CC", "f1x-cc"} }, buildCmd);

  return success;
}

bool Project::buildWithRuntime(const fs::path &header) {
  BOOST_LOG_TRIVIAL(info) << "building project with f1x runtime";

  bool success = buildInEnvironment({ {"CC", "f1x-cc"},
                                      {"F1X_RUNTIME_H", header.string()},
                                      {"F1X_RUNTIME_LIB", workDir.string()},
                                      {"LD_LIBRARY_PATH", workDir.string()} },
                                    buildCmd);

  return success;
}

void Project::saveFilesWithPrefix(const string &prefix) {
  for (int i = 0; i < files.size(); i++) {
    auto destination = workDir / fs::path(prefix + std::to_string(i) + ".c");
    if(fs::exists(destination)) {
      fs::remove(destination);
    }
    fs::copy(root / files[i].relpath, destination);
  }
}

void Project::restoreFilesWithPrefix(const string &prefix) {
  for (int i = 0; i < files.size(); i++) {
    if(fs::exists(root / files[i].relpath)) {
      fs::remove(root / files[i].relpath);
    }
    fs::copy(workDir / fs::path(prefix + std::to_string(i) + ".c"), root / files[i].relpath);
  }
}

void Project::saveOriginalFiles() {
  saveFilesWithPrefix("original");
}

void Project::saveInstrumentedFiles() {
  saveFilesWithPrefix("instrumented");
}

void Project::saveProfileInstumentedFiles() {
  saveFilesWithPrefix("profile_instrumented");
}

void Project::savePatchedFiles() {
  saveFilesWithPrefix("patched");
}

void Project::restoreOriginalFiles() {
  restoreFilesWithPrefix("original");
}

void Project::restoreInstrumentedFiles() {
  restoreFilesWithPrefix("instrumented");
}

void Project::computeDiff(const ProjectFile &file,
                          const fs::path &output) {
  {
    fs::path a = fs::path("a") / file.relpath;
    fs::path b = fs::path("b") / file.relpath;
    fs::ofstream ofs(output);
    ofs << "--- " << a.string() << "\n"
        << "+++ " << b.string() << "\n";
  }
  uint id = getFileId(file);
  
  fs::path fromFile = workDir / fs::path("original" + std::to_string(id) + ".c");
  fs::path toFile = workDir / fs::path("patched" + std::to_string(id) + ".c");
  string cmd = "diff " + fromFile.string() + " " + toFile.string() + " >> " + output.string();
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd;
  std::system(cmd.c_str());
}

bool Project::instrumentFile(const ProjectFile &file,
                             const boost::filesystem::path &outputFile,
                             const boost::filesystem::path *profile) {
  BOOST_LOG_TRIVIAL(info) << "instrumenting source files";
  
  uint id = getFileId(file);

  FromDirectory dir(root);
  std::stringstream cmd;
  cmd << "f1x-transform " << file.relpath.string();
  
  if(! profile)
    cmd << " --profile";
  else
    cmd << " --instrument " << *profile;

  cmd << " --from-line " << file.fromLine
      << " --to-line " << file.toLine
      << " --file-id " << id
      << " --output " + outputFile.string();
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  uint status = std::system(cmd.str().c_str());
  return status == 0;
}

uint Project::getFileId(const ProjectFile &file) {
  uint id = 0;
  for (auto &f : files) {
    if (file.relpath != f.relpath)
      id++;
  }
  assert(id < files.size());
  return id;
}

bool Project::applyPatch(const SearchSpaceElement &patch) {
  BOOST_LOG_TRIVIAL(debug) << "applying patch";
  FromDirectory dir(root);
  uint beginLine = patch.app->location.beginLine;
  uint beginColumn = patch.app->location.beginColumn;
  uint endLine = patch.app->location.endLine;
  uint endColumn = patch.app->location.endColumn;
  std::stringstream cmd;
  cmd << "f1x-transform " << files[patch.app->location.fileId].relpath.string() << " --apply"
      << " --bl " << beginLine
      << " --bc " << beginColumn
      << " --el " << endLine
      << " --ec " << endColumn
      << " --patch " << "\"" << expressionToString(patch.modified) << "\"";
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  uint status = std::system(cmd.str().c_str());
  return status == 0;
}


TestingFramework::TestingFramework(const Project &project,
                                   const boost::filesystem::path &driver,
                                   const uint testTimeout,
                                   const boost::filesystem::path &workDir,
                                   const Config &cfg):
  project(project),
  driver(driver),
  testTimeout(testTimeout),
  workDir(workDir),
  cfg(cfg) {}

bool TestingFramework::isPassing(const std::string &testId) {
  FromDirectory dir(project.getRoot());
  InEnvironment env(map<string, string>{{"LD_LIBRARY_PATH", workDir.string()}});
  std::stringstream cmd;
  cmd << "timeout " << std::setprecision(3) << ((double) testTimeout) / 1000.0 << "s"
      << " " << driver.string() << " " << testId;
  if (cfg.verbose) {
    cmd << " >&2";
  } else {
    cmd << " >/dev/null 2>&1";
  }
  BOOST_LOG_TRIVIAL(debug) << "cmd: " << cmd.str();
  uint status = std::system(cmd.str().c_str());
  return (status == 0);
}


vector<string> getFailing(TestingFramework &tester, const vector<string> &tests) {

  vector<string> failingTests;

  for (auto &test : tests) {
    if (! tester.isPassing(test)) {
      failingTests.push_back(test);
    }
  }

  return failingTests;
}
