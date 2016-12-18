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

#pragma once

#include <string>

#include <boost/filesystem.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "Config.h"
#include "RepairUtil.h"
#include "Project.h"


void initializeTrivialLogger(bool verbose) {
  boost::log::add_common_attributes();
  boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");
  boost::log::add_console_log(std::cerr, boost::log::keywords::format = "[%TimeStamp%] [%Severity%]\t%Message%");
  if (verbose) {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);    
  } else {
    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
  }
}


std::vector<ProjectFile> parseFilesArg(const boost::filesystem::path &root,
                                       const std::vector<std::string> &args) {
  std::vector<ProjectFile> files;
  for (auto &arg : args) {
    boost::filesystem::path file;
    uint fromLine = 0;
    uint toLine = 0;
    auto colonIndex = arg.find(":");
    if (colonIndex == std::string::npos) {
      file = boost::filesystem::path(arg);
    } else {
      std::string pathStr = arg.substr(0, colonIndex);
      file = boost::filesystem::path(pathStr);
      std::string rangeStr = arg.substr(colonIndex + 1);
      auto dashIndex = rangeStr.find("-");
      if (dashIndex == std::string::npos) {
        try {
          fromLine = std::stoi(rangeStr);
          toLine = fromLine;
        } catch (...) {
          throw parse_error("wrong range format: " + rangeStr);
        }
      } else {
        std::string fromStr = rangeStr.substr(0, dashIndex);
        std::string toStr = rangeStr.substr(dashIndex + 1);
        try {
          fromLine = std::stoi(fromStr);
          toLine = std::stoi(toStr);
        } catch (...) {
          throw parse_error("wrong range format: " + rangeStr);
        }
      }
    }
    boost::filesystem::path fullPath = root / file;
    if (! boost::filesystem::exists(fullPath)) {
      throw parse_error("source file does not exist: " + fullPath.string());
    }
    files.push_back(ProjectFile{file, fromLine, toLine});    
  }
  return files;
}
