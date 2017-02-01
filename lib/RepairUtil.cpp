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

#include <iostream>
#include <memory>
#include <cstdlib>
#include <string>
#include <sys/stat.h>
#include <sstream>

#include <boost/filesystem/fstream.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>

#include "RepairUtil.h"
#include "F1XConfig.h"

namespace fs = boost::filesystem;
namespace json = rapidjson;

using std::string;
using std::vector;
using std::shared_ptr;


std::string visualizeF1XID(const F1XID &id) {
  std::stringstream result;
  result << id.base << ":" << id.int2 << ":" << id.bool2 << ":" << id.cond3 << ":" << id.param;
  return result.str();
}

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

bool isAbstractNode(NodeKind kind) {
  return kind == NodeKind::PARAMETER ||
    kind == NodeKind::INT2 ||
    kind == NodeKind::BOOL2 ||
    kind == NodeKind::COND3;
}

NodeKind kindByString(const string &kindStr) {
  if (kindStr == "operator") {
    return NodeKind::OPERATOR;
  } else if (kindStr == "object") {
    return NodeKind::VARIABLE;
  } else if (kindStr == "pointer") {
    return NodeKind::VARIABLE;
  } else if (kindStr == "constant") {
    return NodeKind::CONSTANT;
  } else {
    throw parse_error("unsupported kind: " + kindStr);
  }
}

TransformationSchema transformationSchemaByString(const string &str) {
  if (str == "expression") {
    return TransformationSchema::EXPRESSION;
  } else if (str == "if_guard") {
    return TransformationSchema::IF_GUARD;
  } else if (str == "array_init") {
    return TransformationSchema::INITIALIZATION;
  } else {
    throw parse_error("unsupported transformation schema: " + str);
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
  } else if (repr == ">=") {
    return Operator::GE;
  } else if (repr == "||") {
    return Operator::OR;
  } else if (repr == "&&") {
    return Operator::AND;
  } else if (repr == "+") {
    return Operator::ADD;
  } else if (repr == "-") {
    return Operator::SUB;
  } else if (repr == "*") {
    return Operator::MUL;
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
  case Operator::INT_CAST:
    return "(int)";
  case Operator::BV_TO_INT:
    return ""; // implicit
  case Operator::INT_TO_BV:
    return ""; // implicit
  }
  throw std::invalid_argument("unsupported operator");
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
  case Operator::INT_CAST:
    return Type::INTEGER;
  }
  throw std::invalid_argument("unsupported operator");
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
  throw std::invalid_argument("unsupported expression");
}


Expression getIntegerExpression(int n) {
  return Expression{ NodeKind::CONSTANT,
                     Type::INTEGER,
                     Operator::NONE,
                     "int",
                     std::to_string(n),
                     {} };
}

std::string visualizeTransformationSchema(const TransformationSchema &schema) {
  switch (schema) {
  case TransformationSchema::EXPRESSION:
    return "modify expression";
  case TransformationSchema::IF_GUARD:
    return "add if guard";
  case TransformationSchema::INITIALIZATION:
    return "add array initialization";
  default:
    throw std::invalid_argument("unsupported transformation schema");
  }
}


std::string visualizeModificationKind(const ModificationKind &kind) {
  switch (kind) {
  case ModificationKind::OPERATOR:
    return "replace operator";
  case ModificationKind::SWAPING:
    return "swap arguments";
  case ModificationKind::SIMPLIFICATION:
    return "simplify";
  case ModificationKind::GENERALIZATION:
    return "generalize";
  case ModificationKind::CONCRETIZATION:
    return "concretize";
  case ModificationKind::LOOSENING:
    return "loosen";
  case ModificationKind::TIGHTENING:
    return "tighten";
  case ModificationKind::NEGATION:
    return "negate";
  case ModificationKind::NULL_CHECK:
    return "null check";
  case ModificationKind::SUBSTITUTION:
    return "substitute";
  default:
    throw std::invalid_argument("unsupported modification kind");
  }
}


std::string visualizeChange(const SearchSpaceElement &el) {
  std::stringstream result;
  result << "\"" << expressionToString(el.app->original) << "\""
         << " ---> "
         << "\"" << expressionToString(el.modified) << "\"";
  return result.str();
}                                                          


std::string visualizeElement(const SearchSpaceElement &el,
                             const boost::filesystem::path &file) {
  std::stringstream result;
  result << visualizeF1XID(el.id) << " "
         << visualizeTransformationSchema(el.app->schema) 
         << " [" << visualizeModificationKind(el.meta.kind) << "] "
         << visualizeChange(el)
         << " in " << file.string() << ":" << el.app->location.beginLine;
  return result.str();
}                                                          


Expression convertExpression(const json::Value &json) {
  string kindStr = json["kind"].GetString();
  NodeKind kind = kindByString(kindStr);
  Type type;
  string rawType = json["type"].GetString();
  Operator op;
  string repr = json["repr"].GetString();
  vector<Expression> args;
  if (json.HasMember("args")) {
    const auto &arguments = json["args"].GetArray();
    assert(kind == NodeKind::OPERATOR);
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
    op = Operator::NONE;
    if (kindStr == "pointer") {
      type = Type::POINTER;
    } else {
      type = Type::INTEGER;
    }
  }
  
  return Expression{kind, type, op, rawType, repr, args};
}


vector<shared_ptr<SchemaApplication>> loadSchemaApplications(const fs::path &path) {
  vector<shared_ptr<SchemaApplication>> result;
  json::Document d;
  {
    fs::ifstream ifs(path);
    json::IStreamWrapper isw(ifs);
    d.ParseStream(isw);
  }

  for (auto &app : d.GetArray()) {
    ulong appId = app["appId"].GetUint();

    TransformationSchema schema = transformationSchemaByString(app["schema"].GetString());

    Location location {
      app["location"]["fileId"].GetUint(),
      app["location"]["beginLine"].GetUint(),
      app["location"]["beginColumn"].GetUint(),
      app["location"]["endLine"].GetUint(),
      app["location"]["endColumn"].GetUint()
    };

    LocationContext context;
    string contextStr = app["context"].GetString();
    if (contextStr == "condition") {
      context = LocationContext::CONDITION;
    } else {
      context = LocationContext::UNKNOWN;
    }

    Expression expression = convertExpression(app["expression"]);

    vector<Expression> components;
    for (auto &c : app["components"].GetArray()) {
      components.push_back(convertExpression(c));
    }

    shared_ptr<SchemaApplication> sa(new SchemaApplication{ appId,
                                                            schema,
                                                            location,
                                                            context,
                                                            expression,
                                                            components });
    result.push_back(sa);
  }

  return result;
}


bool isExecutable(const char *file)
{
  struct stat  st;

  if (stat(file, &st) < 0)
    return false;
  if ((st.st_mode & S_IEXEC) != 0)
    return true;
  return false;
}
