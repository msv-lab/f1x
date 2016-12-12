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

#include <iostream>
#include <boost/filesystem/fstream.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "RepairUtil.h"
#include "Config.h"

namespace fs = boost::filesystem;

using namespace rapidjson;
using std::string;


void addClangHeadersToCompileDB(fs::path projectRoot) {
  fs::path compileDB("compile_commands.json");
  fs::path path = projectRoot / compileDB;
  Document db;
  {
    fs::ifstream ifs(path);
    IStreamWrapper isw(ifs);
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
    OStreamWrapper osw(ofs);
    Writer<OStreamWrapper> writer(osw);
    db.Accept(writer);
  }
}


ProjectFile::ProjectFile(std::string _p): 
  path(_p) {
  id = next_id;
  next_id++;
}

fs::path ProjectFile::getRelativePath() const {
  return path;
}

uint ProjectFile::getId() const {
  return id;
}

uint ProjectFile::next_id = 0;
