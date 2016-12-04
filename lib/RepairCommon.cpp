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

#include "RepairCommon.h"

namespace fs = boost::filesystem;


ProjectFile::ProjectFile(std::string _p): 
  path(_p) {
  id = next_id;
  next_id++;
}

fs::path ProjectFile::getPath() const {
  return path;
}

unsigned ProjectFile::getId() const {
  return id;
}

unsigned ProjectFile::next_id = 0;


RepairLocation::RepairLocation(DefectClass _dc,
                                   ProjectFile _f,
                                   unsigned _bl,
                                   unsigned _bc,
                                   unsigned _el,
                                   unsigned _ec):
  defectClass(_dc),
  file(_f),
  beginLine(_bl),
  beginColumn(_bc),
  endLine(_el),
  endColumn(_ec) {}

DefectClass RepairLocation::getDefectClass() const {
  return defectClass;
}

ProjectFile RepairLocation::getProjectFile() const {
  return file;
}

unsigned RepairLocation::getBeginLine() const {
  return beginLine;
}

unsigned RepairLocation::getBeginColumn() const {
  return beginColumn;
}

unsigned RepairLocation::getEndLine() const {
  return endLine;
}

unsigned RepairLocation::getEndColumn() const {
  return endColumn;
}
