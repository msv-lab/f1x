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

#include <string>
#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "Synthesis.h"
#include "Runtime.h"

namespace fs = boost::filesystem;

using std::pair;
using std::make_pair;
using std::vector;
using std::string;
using std::shared_ptr;
using std::unordered_map;

// size of simple (atomic) modification
const uint ATOMIC_EDIT = 1;
const string POINTER_ARG_NAME = "__ptr_vals";
const string ID_TYPE = "unsigned long";
const string PARAM_TYPE = "int";
const string INT2_TYPE = "int"; //FIXME: this a serious assumption, need to cast explicitly inside patches

string argNameByType(const std::string &typeName) {
  std::string result = typeName;
  std::replace(result.begin(), result.end(), ' ', '_');
  return "__" + result + "_vals";
}

vector<Operator> mutateNumericOperator(Operator op) {
  switch (op) {
  case Operator::EQ:
    return { Operator::NEQ, Operator::LT, Operator::LE, Operator::GT, Operator::GE };
  case Operator::NEQ:
    return { Operator::EQ, Operator::LT, Operator::LE, Operator::GT, Operator::GE };
  case Operator::LT:
    return { Operator::EQ, Operator::NEQ, Operator::LE, Operator::GT, Operator::GE };
  case Operator::LE:
    return { Operator::EQ, Operator::NEQ, Operator::LT, Operator::GT, Operator::GE };
  case Operator::GT:
    return { Operator::EQ, Operator::NEQ, Operator::LT, Operator::LE, Operator::GE };
  case Operator::GE:
    return { Operator::EQ, Operator::NEQ, Operator::LT, Operator::LE, Operator::GT };
  case Operator::OR:
    return { Operator::AND };
  case Operator::AND:
    return { Operator::OR };
  case Operator::ADD:
    return { Operator::SUB, Operator::MUL }; // NOTE: no Operator::DIV and Operator::MOD
  case Operator::SUB:
    return { Operator::ADD, Operator::MUL };
  case Operator::MUL:
    return { Operator::ADD, Operator::SUB };
  case Operator::DIV:
    return { Operator::ADD, Operator::SUB, Operator::MUL };
  case Operator::MOD:
    return { Operator::ADD, Operator::SUB, Operator::MUL };
  case Operator::NEG:
    return {};
  case Operator::NOT:
    return {};
  case Operator::BV_AND:
    return { Operator::BV_OR, Operator::BV_XOR };
  case Operator::BV_OR:
    return { Operator::BV_AND, Operator::BV_XOR };
  case Operator::BV_XOR:
    return { Operator::BV_AND, Operator::BV_OR };
  case Operator::BV_SHL:
    return { Operator::BV_SHR };
  case Operator::BV_SHR:
    return { Operator::BV_SHL };
  case Operator::BV_NOT:
    return {};
  }
  return {};
}

vector<Operator> mutatePointerOperator(Operator op) {
  switch (op) {
  case Operator::EQ:
    return { Operator::NEQ };
  case Operator::NEQ:
    return { Operator::EQ };
  }
  return {};
}



Expression makeArgSubs(const Expression &expr, const Expression &subs) {
  return Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {subs}};
}


Expression makeLeftSubs(const Expression &expr, const Expression &subs) {
  return Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {subs, expr.args[1]}};
}


Expression makeRightSubs(const Expression &expr, const Expression &subs) {
  return Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {expr.args[0], subs}};
}

// simplifiable are those that have arguments of the same type as the result
bool simplifiable(const Expression &expr) {
  switch (expr.op) {
  case Operator::OR:
  case Operator::AND:
  case Operator::ADD:
  case Operator::SUB:
  case Operator::MUL:
  case Operator::DIV:
  case Operator::MOD:
  case Operator::NEG:
  case Operator::NOT: //TODE: shouldn't I move it to a separate schema?
  case Operator::BV_AND:
  case Operator::BV_OR:
  case Operator::BV_XOR:
  case Operator::BV_SHL:
  case Operator::BV_SHR:
  case Operator::BV_NOT:
    return true;
  default:
    return false;
  }
}

vector<pair<Expression, PatchMeta>> mutate(const Expression &expr, const vector<Expression> &components) {
  vector<pair<Expression, PatchMeta>> result;
  if (expr.args.size() == 0) {
    if (expr.kind == Kind::CONSTANT) {
      if (expr.type == Type::INTEGER) {
        for (auto &c : components) {
          if (c.type == Type::INTEGER)
            result.push_back(make_pair(c, PatchMeta{Transformation::GENERALIZATION, ATOMIC_EDIT}));
        }
        result.push_back(make_pair(getIntegerExpression(0),
                                   PatchMeta{Transformation::SUBSTITUTION, ATOMIC_EDIT}));
        result.push_back(make_pair(getIntegerExpression(1),
                                   PatchMeta{Transformation::SUBSTITUTION, ATOMIC_EDIT}));
      }
    }
    if (expr.kind == Kind::VARIABLE) {
      if (expr.type == Type::INTEGER) {
        for (auto &c : components) {
          if (c.type == Type::INTEGER)
            result.push_back(make_pair(c, PatchMeta{Transformation::SUBSTITUTION, ATOMIC_EDIT}));
        }
        result.push_back(make_pair(getIntegerExpression(0),
                                   PatchMeta{Transformation::CONCRETIZATION, ATOMIC_EDIT}));
        result.push_back(make_pair(getIntegerExpression(1),
                                   PatchMeta{Transformation::CONCRETIZATION, ATOMIC_EDIT}));
      }
      if (expr.type == Type::POINTER) {
        for (auto &c : components) {
          if (c.type == Type::POINTER && expr.rawType == c.rawType)
            result.push_back(make_pair(c, PatchMeta{Transformation::SUBSTITUTION, ATOMIC_EDIT}));
        }
        result.push_back(make_pair(getNullPointer(),
                                   PatchMeta{Transformation::CONCRETIZATION, ATOMIC_EDIT}));
      }
    }
  } else {
    vector<Operator> oms;
    if (expr.args[0].type == Type::POINTER) {
      oms = mutatePointerOperator(expr.op);
    } else {
      oms = mutateNumericOperator(expr.op);
    }
    for (auto &m : oms) {
      Expression e = expr;
      e.op = m;
      e.repr = operatorToString(m);
      result.push_back(make_pair(std::move(e), PatchMeta{Transformation::ALTERNATIVE, ATOMIC_EDIT}));
    }

    // FIXME: need to avoid division by zero
    if (expr.args.size() == 1) {
      vector<pair<Expression, PatchMeta>> argMutants = mutate(expr.args[0], components);
      for (auto &m : argMutants) {
        result.push_back(make_pair(makeArgSubs(expr, m.first), m.second));
      }
      if (simplifiable(expr)) {
        Expression argCopy = expr.args[0];
        // FIXME: instead of ATOMIC_EDIT simplification should depend on the size of deleted subexpression
        result.push_back(make_pair(argCopy, PatchMeta{Transformation::SIMPLIFICATION, ATOMIC_EDIT}));
      }
    } else if (expr.args.size() == 2) {
      vector<pair<Expression, PatchMeta>> leftMutants = mutate(expr.args[0], components);
      for (auto &m : leftMutants) {
        result.push_back(make_pair(makeLeftSubs(expr, m.first), m.second));
      }
      vector<pair<Expression, PatchMeta>> rightMutants = mutate(expr.args[1], components);
      for (auto &m : rightMutants) {
        result.push_back(make_pair(makeRightSubs(expr, m.first), m.second));
      }
      if (simplifiable(expr)) {
        Expression leftCopy = expr.args[0];
        Expression rightCopy = expr.args[1];
        // FIXME: instead of ATOMIC_EDIT simplification should depend on the size of deleted subexpression
        result.push_back(make_pair(leftCopy, PatchMeta{Transformation::SIMPLIFICATION, ATOMIC_EDIT}));
        result.push_back(make_pair(rightCopy, PatchMeta{Transformation::SIMPLIFICATION, ATOMIC_EDIT}));
      }
    }
  }
  return result;
}


void substituteRealNames(Expression &expression,
                         unordered_map<string, string> &accessByName) {
  if (expression.args.size() == 0 && expression.kind == Kind::VARIABLE) {
    expression.repr = accessByName[expression.repr];
  } else {
    for (auto &arg : expression.args) {
      substituteRealNames(arg, accessByName);
    }
  }
}


void generateExpressions(shared_ptr<CandidateLocation> cl,
                         uint &id,
                         std::ostream &OS,
                         vector<SearchSpaceElement> &ss) {
  vector<string> types;
  vector<Expression> pointers;
  for (auto &c : cl->components) {
    if (c.type == Type::INTEGER) {
      if(std::find(types.begin(), types.end(), c.rawType) == types.end()) {
        types.push_back(c.rawType);
      }
    } else {
      pointers.push_back(c);
    }
  }

  std::stable_sort(types.begin(), types.end());

  unordered_map<string, vector<string>> namesByType;
  for (auto &type : types) {
    namesByType[type] = vector<string>();
  }

  for (auto &c : cl->components) {
    if (c.type == Type::INTEGER) {
      namesByType[c.rawType].push_back(c.repr);
    }
  }

  unordered_map<string, string> accessByName;
  for (auto &type : types) {
    auto names = namesByType[type];
    for (int index = 0; index < names.size(); index++) {
      accessByName[names[index]] = argNameByType(type) + "[" + std::to_string(index) + "]";
    }
  }

  for (int index = 0; index < pointers.size(); index++) {
    accessByName[pointers[index].repr] = POINTER_ARG_NAME + "[" + std::to_string(index) + "]";
  }

  vector<pair<Expression, PatchMeta>> mutants = mutate(cl->original, cl->components);
  
  uint topId = id;

  string outputType;
  if (cl->original.type == Type::POINTER) {
    outputType= "void*";
  } else {
    outputType = cl->original.rawType;
   }

  for (auto &candidate : mutants) {
    uint currentId = id;

    Expression runtimeRepr = candidate.first;
    substituteRealNames(runtimeRepr, accessByName);
    OS << "case " << currentId << ":" << "\n"
       << "if (! evaluated) { " << "\n";
    // FIXME: this is a temporary hack
    string castStr;
    if (runtimeRepr.type == Type::POINTER && cl->original.type != Type::POINTER)
      castStr = "(std::size_t)";
    OS << "candidate_value = " << castStr << expressionToString(runtimeRepr) << ";" << "\n";
    OS << "evaluated = true;" << "\n"
       << "next = " << topId << ";" << "\n"
       << "break;" << "\n"
       << "}" << "\n";
    // FIXME: this is very imprecise, the comparison should depend on type
    castStr = "";
    if (runtimeRepr.type == Type::POINTER && cl->original.type != Type::POINTER)
      castStr = "(std::size_t)";
    // FIXME: currently fill with zeros:
    OS << "if (candidate_value == " << castStr << expressionToString(runtimeRepr) << ") ofs << " << currentId << " << \" 0 0 0 0 \";" << "\n";
    if (currentId == topId + mutants.size() - 1) {
      OS << "partitioned = true;" << "\n";
    } else {
      OS << "next = " << (currentId + 1) << ";" << "\n";
    }
    OS << "break; " << "\n";
    F1XID f1xid{0};
    f1xid.base = currentId; //FIXME: this should be more complete
    ss.push_back(SearchSpaceElement{cl, f1xid, candidate.first, candidate.second});

    id++;
  }
}


void addRuntimeLoader(std::ostream &OH) {
  OH << "unsigned long __f1x_id;" << "\n"
     << "unsigned long __f1x_loc;" << "\n"
     << "class F1XRuntimeLoader {" << "\n"
     << "public:" << "\n"
     << "F1XRuntimeLoader() {" << "\n"
     << "__f1x_id = strtoul(getenv(\"F1X_ID\"), (char **)NULL, 10);" << "\n"
     << "__f1x_loc = strtoul(getenv(\"F1X_LOC\"), (char **)NULL, 10);" << "\n"
     << "}" << "\n"
     << "};" << "\n"
     << "static F1XRuntimeLoader __loader;" << "\n";
}

string makeParameterList(shared_ptr<CandidateLocation> cl) {
  std::ostringstream result;

  vector<string> types;
  bool hasPointers = false;
  for (auto &c : cl->components) {
    if (c.type == Type::INTEGER) {
      if(std::find(types.begin(), types.end(), c.rawType) == types.end()) {
        types.push_back(c.rawType);
      }
    } else {
      hasPointers = true;
    }
  }

  std::stable_sort(types.begin(), types.end());

  bool firstArray = true;
  for (auto &type : types) {
    if (firstArray) {
      firstArray = false;
    } else {
      result << ", ";
    }
    result << type << " " << argNameByType(type) << "[]";
  }
  if (hasPointers) {
    if (firstArray) {
      firstArray = false;
    } else {
      result << ", ";
    }
    result << "void *" << POINTER_ARG_NAME << "[]";
  }
  
  return result.str();
}

vector<SearchSpaceElement> generateSearchSpace(const vector<shared_ptr<CandidateLocation>> &candidateLocations,
                                               const fs::path &workDir,
                                               std::ostream &OS,
                                               std::ostream &OH,
                                               const Config &cfg) {
  
  // header
  OH << "#ifdef __cplusplus" << "\n"
     << "extern \"C\" {" << "\n"
     << "#endif" << "\n"
     << "extern unsigned long __f1x_loc;" << "\n"
     << "extern unsigned long __f1x_id;" << "\n";

  for (auto cl : candidateLocations) {
    string outputType;
    if (cl->original.type == Type::POINTER) {
      outputType= "void*";
    } else {
      outputType = cl->original.rawType;
    }

    OH << outputType << " __f1x_" 
       << cl->location.fileId << "_"
       << cl->location.beginLine << "_"
       << cl->location.beginColumn << "_"
       << cl->location.endLine << "_"
       << cl->location.endColumn
       << "(" << makeParameterList(cl) << ")"
       << ";" << "\n";
  }

  OH << "#ifdef __cplusplus" << "\n"
     << "}" << "\n"
     << "#endif" << "\n";

  // source
  OS << "#include \"rt.h\"" << "\n"
     << "#include <stdlib.h>" << "\n"
     << "#include <fstream>" << "\n";

  addRuntimeLoader(OS);

  vector<SearchSpaceElement> searchSpace;

  uint id = 0;

  for (auto cl : candidateLocations) {
    string outputType;
    if (cl->original.type == Type::POINTER) {
      outputType= "void*";
    } else {
      outputType = cl->original.rawType;
    }

    OS << outputType << " __f1x_"
       << cl->location.fileId << "_"
       << cl->location.beginLine << "_"
       << cl->location.beginColumn << "_"
       << cl->location.endLine << "_"
       << cl->location.endColumn
       << "(" << makeParameterList(cl) << ")"
       << "{" << "\n";

    fs::path partitionFile = workDir / PARTITION_OUT;
    
    OS << "std::ofstream ofs;" << "\n"
       << "ofs.open (" << partitionFile << ", std::ofstream::out | std::ofstream::app);" << "\n";

    OS << outputType << " candidate_value;" << "\n"
       << "bool evaluated = false;" << "\n"
       << "unsigned long next = __f1x_id;" << "\n";

    if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
      OS << "bool partitioned = false;" << "\n";
    } else {
      OS << "bool partitioned = true;" << "\n";
    }

    OS << "while (!evaluated || !partitioned) {" << "\n"
       << "switch (next) {" << "\n";
    generateExpressions(cl, id, OS, searchSpace);
    OS << "}" << "\n";
    OS << "}" << "\n";

    OS << "ofs << '\\n';" << "\n";
    OS << "ofs.close();" << "\n";
    OS << "return candidate_value;" << "\n";

    OS << "}" << "\n";
  }

  return searchSpace;
}
