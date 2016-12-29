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
#include <stdexcept>
#include <map>

#include <boost/filesystem.hpp>

#include "F1XConfig.h"


enum class Kind {
  OPERATOR, VARIABLE, CONSTANT, PARAMETER, 
  BV2, INT2, BOOL2, BOOL3 // auxiliary kinds
};


enum class Type {
  BOOLEAN, INTEGER, POINTER, BITVECTOR
};


enum class Operator {
  EQ, NEQ, LT, LE, GT, GE, OR, AND, ADD, SUB, MUL, DIV, MOD, NEG, NOT,
  BV_AND, BV_XOR, BV_OR, BV_SHL, BV_SHR, BV_NOT,
  BV_TO_INT, INT_TO_BV // auxiliary operators
};

Type operatorType(const Operator &op);

Operator binaryOperatorByString(const std::string &repr);

Operator unaryOperatorByString(const std::string &repr);

std::string operatorToString(const Operator &op);


struct Expression {
  Kind kind;
  
  Type type;

  /*
    only if kind is OPERATOR
   */
  Operator op;

  /* 
     Either 
       char, unsinged char, unsigned short, unsigned int, unsigned long, signed char, short, int, long
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

std::string expressionToString(const Expression &expression);


enum class DefectClass {
  CONDITION,  // existing program conditions in if, for, while, etc.
  EXPRESSION, // right side of assignments, call arguments
  GUARD       // adding guard for existing statement
};


struct Location {
  uint fileId;
  uint locId;  
  uint beginLine;
  uint beginColumn;
  uint endLine;
  uint endColumn;
};


struct CandidateLocation {
  DefectClass defect;
  Location location;
  Expression original;
  std::vector<Expression> components;
};


enum class Transformation {
  NONE,
  ALTERNATIVE,    // alternative operator e.g. > --> >=
  SWAPING,        // swaping arguments
  SIMPLIFICATION, // simplifying expression
  GENERALIZATION, // e.g. 1 --> x
  CONCRETIZATION, // e.g. x --> 1
  SUBSTITUTION,   // (generic) substution of subnode
  WIDENING,       // adding "|| something"
  NARROWING       // adding "&& something"
};


struct PatchMeta {
  Transformation transformation;
  uint distance;
};


struct SearchSpaceElement {
  std::shared_ptr<CandidateLocation> buggy;
  uint id;
  Expression patch;
  PatchMeta meta;
};


std::string visualizeElement(const SearchSpaceElement &el, const boost::filesystem::path &file);


class FromDirectory {
 public:
  FromDirectory(const boost::filesystem::path &path);
  ~FromDirectory();

 private:
  boost::filesystem::path original;
};


class InEnvironment {
 public:
  InEnvironment(const std::map<std::string, std::string> &env);
  ~InEnvironment();

 private:
  std::map<std::string, std::string> original;
};


class parse_error : public std::logic_error {
 public:
  using std::logic_error::logic_error;
};


std::vector<std::shared_ptr<CandidateLocation>> loadCandidateLocations(const boost::filesystem::path &path);


bool isExecutable(const char *file);
