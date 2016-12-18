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

#include <string>
#include <sstream>
#include <unordered_map>

#include "Synthesis.h"

using std::pair;
using std::vector;
using std::string;
using std::shared_ptr;
using std::unordered_map;


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

vector<string> mutateOperator(Operator op) {
  switch (op) {
  case Operator::EQ:
    return { "!=", "<", "<=", ">", ">=" };
  case Operator::NEQ:
    return { "==", "<", "<=", ">", ">=" };
  case Operator::LT:
    return { "==", "!=", "<=", ">", ">=" };
  case Operator::LE:
    return { "==", "!=", "<", ">", ">=" };
  case Operator::GT:
    return { "==", "!=", "<", "<=", ">=" };
  case Operator::GE:
    return { "==", "!=", "<", "<=", ">" };
  case Operator::OR:
    return {};
  case Operator::AND:
    return {};
  case Operator::ADD:
    return {};
  case Operator::SUB:
    return {};
  case Operator::MUL:
    return {};
  case Operator::DIV:
    return {};
  case Operator::MOD:
    return {};
  case Operator::NEG:
    return {};
  case Operator::NOT:
    return {};
  case Operator::BV_AND:
    return {};
  case Operator::BV_OR:
    return {};
  case Operator::BV_XOR:
    return {};
  case Operator::BV_SHL:
    return {};
  case Operator::BV_SHR:
    return {};
  case Operator::BV_NOT:
    return {};
  case Operator::BV_TO_INT:
    return {};
  case Operator::INT_TO_BV:
    return {};
  }
}


vector<Expression> mutate(const Expression &expr, const vector<Expression> &components) {
  vector<Expression> result;
  if (expr.args.size() == 0) {
    result = components;
  } else {
    vector<string> oms = mutateOperator(expr.op);
    for (auto &m : oms) {
      Expression e = expr;
      e.repr = m;
      result.push_back(std::move(e));
    }

    if (expr.args.size() == 1) {
      vector<Expression> argMutants = mutate(expr.args[0], components);
      for (auto &m : argMutants) {
        result.push_back(Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {m}});
      }
    } else if (expr.args.size() == 2) {
      vector<Expression> leftMutants = mutate(expr.args[0], components);
      for (auto &m : leftMutants) {
        result.push_back(Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {m, expr.args[1]}});
      }
      vector<Expression> rightMutants = mutate(expr.args[1], components);
      for (auto &m : rightMutants) {
        result.push_back(Expression{expr.kind, expr.type, expr.op, expr.rawType, expr.repr, {expr.args[0], m}});
      }
    }
  }
  return result;
}


void substituteRealNames(Expression &expression,
                         unordered_map<string, unsigned> &indexByName) {
  if (expression.args.size() == 0 && expression.kind == Kind::VARIABLE) {
    expression.repr = "i[" + std::to_string(indexByName[expression.repr]) + "]";
  } else {
    for (auto &arg : expression.args) {
      substituteRealNames(arg, indexByName);
    }
  }
}


void generateExpressions(shared_ptr<CandidateLocation> cl,
                         uint &id,
                         std::ostream &OS,
                         vector<SearchSpaceElement> &ss) {
  unordered_map<string, unsigned> indexByName;
  for (int index = 0; index < cl->components.size(); index++) {
    indexByName[cl->components[index].repr] = index;
  }

  vector<Expression> mutants = mutate(cl->original, cl->components);
  
  for (auto &candidate : mutants) {
    uint currentId = ++id;
    Expression runtimeRepr = candidate;
    substituteRealNames(runtimeRepr, indexByName);
    OS << "case " << currentId << ":" << "\n"
       << "return " << expressionToString(runtimeRepr) << ";" << "\n";
    PatchMeta meta{Transformation::NONE, 0};
    ss.push_back(SearchSpaceElement{cl, currentId, candidate, meta});
  }
}


vector<SearchSpaceElement> generateSearchSpace(const vector<shared_ptr<CandidateLocation>> &candidateLocations, std::ostream &OS, std::ostream &OH) {
  
  // header
  OH << "#ifdef __cplusplus" << "\n"
     << "extern \"C\" {" << "\n"
     << "#endif" << "\n"
     << "extern unsigned long __f1x_loc;" << "\n"
     << "extern unsigned long __f1x_id;" << "\n";

  for (auto cl : candidateLocations) {
    OH << "int __f1x_" 
       << cl->location.fileId << "_"
       << cl->location.beginLine << "_"
       << cl->location.beginColumn << "_"
       << cl->location.endLine << "_"
       << cl->location.endColumn
       << "(int i[])"
       << ";" << "\n";
  }

  OH << "#ifdef __cplusplus" << "\n"
     << "}" << "\n"
     << "#endif" << "\n";

  // source
  OS << "#include \"rt.h\"" << "\n"
     << "#include <stdlib.h>" << "\n";

  addRuntimeLoader(OS);

  vector<SearchSpaceElement> searchSpace;

  uint id = 0;

  for (auto cl : candidateLocations) {
    OS << "int __f1x_" 
       << cl->location.fileId << "_"
       << cl->location.beginLine << "_"
       << cl->location.beginColumn << "_"
       << cl->location.endLine << "_"
       << cl->location.endColumn
       << "(int i[])"
       << "{" << "\n";

    OS << "switch (__f1x_id) {" << "\n";
    generateExpressions(cl, id, OS, searchSpace);
    OS << "}" << "\n";

    OS << "}" << "\n";
  }

  return searchSpace;
}
