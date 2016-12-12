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

#include <memory>
#include <boost/filesystem.hpp>
#include "Config.h"


void addClangHeadersToCompileDB(boost::filesystem::path projectRoot);


enum class Kind {
  OPERATOR, VARIABLE, CONSTANT, PARAMETER
};


enum class Type {
  BOOLEAN, INTEGER, POINTER, BITVECTOR
};


enum class Operator {
  EQ, NEQ, LT, LE, GT, GE, OR, AND, ADD, SUB, MUL, DIV, MOD, NOT,
  BV_AND, BV_OR, BV_SHL, BV_SHR, BV_NOT,
  BV_TO_INT, INT_TO_BV // auxiliary operators
};


class Expression {
  Kind kind;
  
  Type type;

  /* 
     Either 
       char
       unsinged char
       unsigned short
       unsigned int
       unsigned long
       signed char
       short
       int
       long
     or
       the base type of pointer
   */
  std::string rawType;

  /*
    1, 2,... for constants; "x", "y",... for variables; ">=",... for ops
  */
  std::string repr;

  std::vector<Expression> args;
};


enum class DefectClass {
  CONDITION,  // existing program conditions in if, for, while, etc.
  EXPRESSION, // right side of assignments, call arguments
  GUARD       // adding guard for existing statement
};


class ProjectFile {
public:
  ProjectFile(std::string _p);
  boost::filesystem::path getRelativePath() const;
  uint getId() const;

private:
  static uint next_id;
  uint id;
  boost::filesystem::path path;
};


class Location {
  ProjectFile file;
  uint beginLine;
  uint beginColumn;
  uint endLine;
  uint endColumn;
  uint id;
};


class CandidateLocation {
  DefectClass defect;
  Location loc;
  Expression original;
  std::vector<Expression> components;
};


enum class Transformation {
  ALTERNATIVE,    // alternative operator e.g. > --> >=
  SWAPING,        // swaping arguments
  GENERALIZATION, // e.g. 1 --> x
  CONCRETIZATION, // e.g. x --> 1
  SUBSTITUTION,   // (generic) substution of subnode
  WIDENING,       // adding "|| something"
  NARROWING       // adding "&& something"
};


class PatchMeta {
  Transformation transformation;
  uint distance;
};


class SearchSpaceElement {
  std::shared_ptr<CandidateLocation> buggy;
  Expression patch;
  PatchMeta meta;
};
