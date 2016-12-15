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
#include <memory>
#include <cstdlib>
#include <string>

#include <boost/filesystem/fstream.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include "RepairUtil.h"
#include "Config.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;

using std::string;
using std::vector;
using std::shared_ptr;


FromDirectory::FromDirectory(const boost::filesystem::path &path):
  original(path) {
  fs::current_path(path);
}
  
FromDirectory::~FromDirectory() {
  fs::current_path(original);    
}

InEnvironment::InEnvironment(const std::map<std::string, std::string> &env) {
  for (auto &entry : env) {
    string orig;
    if (getenv(entry.first.c_str())) {
      original[entry.first] = getenv(entry.first.c_str());
    }
    setenv(entry.first.c_str(), entry.second.c_str(), true);
  }
}

InEnvironment::~InEnvironment() {
  // TODO: possibly can also erase previously undefined
  for (auto &entry : original) {
    setenv(entry.first.c_str(), entry.second.c_str(), true);
  }
}

Kind kindByString(const string &kindStr) {
  if (kindStr == "operator") {
    return Kind::OPERATOR;
  } else if (kindStr == "variable") {
    return Kind::VARIABLE;
  } else if (kindStr == "constant") {
    return Kind::CONSTANT;
  } else {
    throw parse_error("unsupported kind: " + kindStr);
  }
}

DefectClass defectClassByString(const string &defectStr) {
  if (defectStr == "expression") {
    return DefectClass::EXPRESSION;
  } else if (defectStr == "condition") {
    return DefectClass::CONDITION;
  } else if (defectStr == "guard") {
    return DefectClass::GUARD;
  } else {
    throw parse_error("unsupported defect: " + defectStr);
  }
}


Operator binaryOperatorByString(const string &repr) {
  if (repr == "==") {
    return Operator::EQ;
  } else if (repr == "!=") {
    return Operator::NEQ;
  } else if (repr == "<") {
    return Operator::LT;
  } else if (repr == "<=") {
    return Operator::LE;
  } else if (repr == ">") {
    return Operator::GT;
  } else if (repr == "<=") {
    return Operator::GE;
  } else if (repr == "||") {
    return Operator::OR;
  } else if (repr == "&&") {
    return Operator::AND;
  } else if (repr == "+") {
    return Operator::ADD;
  } else if (repr == "-") {
    return Operator::SUB;
  } else if (repr == "/") {
    return Operator::DIV;
  } else if (repr == "%") {
    return Operator::MOD;
  } else if (repr == "&") {
    return Operator::BV_AND;
  } else if (repr == "|") {
    return Operator::BV_OR;
  } else if (repr == "^") {
    return Operator::BV_XOR;
  } else if (repr == "<<") {
    return Operator::BV_SHL;
  } else if (repr == ">>") {
    return Operator::BV_SHR;
  } else {
    throw parse_error("unsupported binary operator: " + repr);
  }
}


Operator unaryOperatorByString(const string &repr) {
  if (repr == "-") {
    return Operator::NEG;
  } else if (repr == "!") {
    return Operator::NOT;
  } else if (repr == "~") {
    return Operator::BV_NOT;
  } else {
    throw parse_error("unsupported unary operator: " + repr);
  }
}


string operatorToString(const Operator &op) {
  switch (op) {
  case Operator::EQ:
    return "==";
  case Operator::NEQ:
    return "!=";
  case Operator::LT:
    return "<";
  case Operator::LE:
    return "<=";
  case Operator::GT:
    return ">";
  case Operator::GE:
    return ">=";
  case Operator::OR:
    return "||";
  case Operator::AND:
    return "&&";
  case Operator::ADD:
    return "+";
  case Operator::SUB:
    return "-";
  case Operator::MUL:
    return "*";
  case Operator::DIV:
    return "/";
  case Operator::MOD:
    return "%";
  case Operator::NEG:
    return "-";
  case Operator::NOT:
    return "!";
  case Operator::BV_AND:
    return "&";
  case Operator::BV_OR:
    return "|";
  case Operator::BV_XOR:
    return "^";
  case Operator::BV_SHL:
    return "<<";
  case Operator::BV_SHR:
    return ">>";
  case Operator::BV_NOT:
    return "~";
  case Operator::BV_TO_INT:
    return ""; // implicit
  case Operator::INT_TO_BV:
    return ""; // implicit
  }
}


Type operatorType(const Operator &op) {
  switch (op) {
  case Operator::EQ:
    return Type::BOOLEAN;
  case Operator::NEQ:
    return Type::BOOLEAN;
  case Operator::LT:
    return Type::BOOLEAN;
  case Operator::LE:
    return Type::BOOLEAN;
  case Operator::GT:
    return Type::BOOLEAN;
  case Operator::GE:
    return Type::BOOLEAN;
  case Operator::OR:
    return Type::BOOLEAN;
  case Operator::AND:
    return Type::BOOLEAN;
  case Operator::ADD:
    return Type::INTEGER;
  case Operator::SUB:
    return Type::INTEGER;
  case Operator::MUL:
    return Type::INTEGER;
  case Operator::DIV:
    return Type::INTEGER;
  case Operator::MOD:
    return Type::INTEGER;
  case Operator::NEG:
    return Type::INTEGER;
  case Operator::NOT:
    return Type::BOOLEAN;
  case Operator::BV_AND:
    return Type::BITVECTOR;
  case Operator::BV_OR:
    return Type::BITVECTOR;
  case Operator::BV_XOR:
    return Type::BITVECTOR;
  case Operator::BV_SHL:
    return Type::BITVECTOR;
  case Operator::BV_SHR:
    return Type::BITVECTOR;
  case Operator::BV_NOT:
    return Type::BITVECTOR;
  case Operator::BV_TO_INT:
    return Type::INTEGER;
  case Operator::INT_TO_BV:
    return Type::BITVECTOR;
  }
}


std::string expressionToString(const Expression &expression) {
  if (expression.args.size() == 0) {
    return expression.repr;
  } else if (expression.args.size() == 1) {
    return expression.repr + " " + expressionToString(expression.args[0]);
  } if (expression.args.size() == 2) {
    return "(" + expressionToString(expression.args[0]) + " " +
           expression.repr + " " +
           expressionToString(expression.args[1]) + ")";
  }
  return "<unsupported>";
}


Expression convertExpression(const json::Value &json) {
  Kind kind = kindByString(json["kind"].GetString());
  Type type;
  string rawType = json["type"].GetString();
  Operator op;
  string repr = json["repr"].GetString();
  vector<Expression> args;
  if (json.HasMember("args")) {
    const auto &arguments = json["args"].GetArray();
    assert(kind == Kind::OPERATOR);
    if (arguments.Size() == 1) {
      op = unaryOperatorByString(repr);
      type = operatorType(op);
    } else if (arguments.Size() == 2) {
      op = binaryOperatorByString(repr);
      type = operatorType(op);      
    } else {
      throw parse_error("unsupported arguments size: " + arguments.Size());
    }
    for (auto &arg : arguments) {
      args.push_back(convertExpression(arg));
    }
  } else {
    type = Type::INTEGER;
  }
  
  return Expression{kind, type, op, rawType, repr, args};
}


vector<shared_ptr<CandidateLocation>> loadCandidateLocations(const fs::path &path) {
  vector<shared_ptr<CandidateLocation>> result;
  json::Document d;
  {
    fs::ifstream ifs(path);
    json::IStreamWrapper isw(ifs);
    d.ParseStream(isw);
  }

  for (auto &loc : d.GetArray()) {
    DefectClass defect = defectClassByString(loc["defect"].GetString());

    Location location {
      loc["location"]["fileId"].GetUint(),
      loc["location"]["locId"].GetUint(),
      loc["location"]["beginLine"].GetUint(),
      loc["location"]["beginColumn"].GetUint(),
      loc["location"]["endLine"].GetUint(),
      loc["location"]["endColumn"].GetUint()
    };

    Expression expression = convertExpression(loc["expression"]);

    vector<Expression> components;
    for (auto &c : loc["components"].GetArray()) {
      components.push_back(convertExpression(c));
    }

    shared_ptr<CandidateLocation> cl(new CandidateLocation{defect, location, expression, components});
    result.push_back(cl);
  }

  return result;
}
