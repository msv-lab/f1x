/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Shin Hwei Tan, Abhik Roychoudhury

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

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

enum class LocalizationFormula {
  OCHIAI,
  JACCARD,
  TARANTULA  
};


enum class CandidateSelection {
  FIXED_ORDER,         // order defined by search space ordering
  PREDICATE_SWITCHING, // predicate switching for failing test
  MAX_PASSING,         // prioritize candidates that pass maximim number of tests
  LEAST_COVERED        // prioritize candidates that are evaluated with least number of tests
};


enum class TestSelection {
  FIXED_ORDER,      // failing, then passing
  MAX_FAILING,      // prioritize tests that failed maximum number of times
  LARGEST_PARTITION // prioritize tests with largest average partition size
};


enum class DefectClass {
  CONDITION,  // existing program conditions (e.g. if, for, while, ...)
  EXPRESSION, // right side of assignments, call arguments
  GUARD       // adding guard for existing statement
};


class ProjectFile {
public:
  ProjectFile(std::string _p);
  fs::path getPath() const;
  unsigned getId() const;

private:
  static unsigned next_id;
  unsigned id;
  fs::path path;
};


class RepairLocation {
public:
  RepairLocation(DefectClass _dc,
                 ProjectFile _f,
                 unsigned _bl,
                 unsigned _bc,
                 unsigned _el,
                 unsigned _ec);
  DefectClass getDefectClass() const;
  ProjectFile getProjectFile() const;
  unsigned getBeginLine() const;
  unsigned getBeginColumn() const;
  unsigned getEndLine() const;
  unsigned getEndColumn() const;

private:
  DefectClass defectClass;
  ProjectFile file;
  unsigned beginLine;
  unsigned beginColumn;
  unsigned endLine;
  unsigned endColumn;
};
