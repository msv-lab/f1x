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
using std::to_string;


/*
 This module is split into two namespaces:
 - synthesize for everything specific to expression synthesis 
 - generate for runtime code generation
 */

namespace synthesize {

  // size of simple (atomic) modification
  const uint ATOMIC_EDIT = 1;

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

  // simplifiable are those that have arguments of the same type as the result
  bool isSimplifiable(const Expression &expr) {
    //TODE: not and bv_not are not here, because they represent a separate modification kind
    switch (expr.op) {
    case Operator::OR:
    case Operator::AND:
    case Operator::ADD:
    case Operator::SUB:
    case Operator::MUL:
    case Operator::DIV:
    case Operator::MOD:
    case Operator::NEG:
    case Operator::BV_AND:
    case Operator::BV_OR:
    case Operator::BV_XOR:
    case Operator::BV_SHL:
    case Operator::BV_SHR:
      return true;
    default:
      return false;
    }
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


  vector<pair<Expression, PatchMetadata>> modifications(const Expression &expr,
                                                        const vector<Expression> &components) {
    vector<pair<Expression, PatchMetadata>> result;
    if (expr.args.size() == 0) {
      if (expr.kind == NodeKind::CONSTANT) {
        if (expr.type == Type::INTEGER) {
          for (auto &c : components) {
            if (c.type == Type::INTEGER)
              result.push_back(make_pair(c, PatchMetadata{ModificationKind::GENERALIZATION, ATOMIC_EDIT}));
          }
          result.push_back(make_pair(getIntegerExpression(0),
                                     PatchMetadata{ModificationKind::SUBSTITUTION, ATOMIC_EDIT}));
          result.push_back(make_pair(getIntegerExpression(1),
                                     PatchMetadata{ModificationKind::SUBSTITUTION, ATOMIC_EDIT}));
        }
      }
      if (expr.kind == NodeKind::VARIABLE) {
        if (expr.type == Type::INTEGER) {
          for (auto &c : components) {
            if (c.type == Type::INTEGER)
              result.push_back(make_pair(c, PatchMetadata{ModificationKind::SUBSTITUTION, ATOMIC_EDIT}));
          }
          result.push_back(make_pair(getIntegerExpression(0),
                                     PatchMetadata{ModificationKind::CONCRETIZATION, ATOMIC_EDIT}));
          result.push_back(make_pair(getIntegerExpression(1),
                                     PatchMetadata{ModificationKind::CONCRETIZATION, ATOMIC_EDIT}));
        }
        if (expr.type == Type::POINTER) {
          for (auto &c : components) {
            if (c.type == Type::POINTER && expr.rawType == c.rawType)
              result.push_back(make_pair(c, PatchMetadata{ModificationKind::SUBSTITUTION, ATOMIC_EDIT}));
          }
          result.push_back(make_pair(getNullPointer(),
                                     PatchMetadata{ModificationKind::CONCRETIZATION, ATOMIC_EDIT}));
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
        result.push_back(make_pair(std::move(e), PatchMetadata{ModificationKind::OPERATOR, ATOMIC_EDIT}));
      }

      // FIXME: need to avoid division by zero
      if (expr.args.size() == 1) {
        vector<pair<Expression, PatchMetadata>> argMods = modifications(expr.args[0], components);
        for (auto &m : argMods) {
          result.push_back(make_pair(makeArgSubs(expr, m.first), m.second));
        }
        if (isSimplifiable(expr)) {
          Expression argCopy = expr.args[0];
          // FIXME: instead of ATOMIC_EDIT simplification should depend on the size of deleted subexpression
          result.push_back(make_pair(argCopy, PatchMetadata{ModificationKind::SIMPLIFICATION, ATOMIC_EDIT}));
        }
      } else if (expr.args.size() == 2) {
        vector<pair<Expression, PatchMetadata>> leftMods = modifications(expr.args[0], components);
        for (auto &m : leftMods) {
          result.push_back(make_pair(makeLeftSubs(expr, m.first), m.second));
        }
        vector<pair<Expression, PatchMetadata>> rightMods = modifications(expr.args[1], components);
        for (auto &m : rightMods) {
          result.push_back(make_pair(makeRightSubs(expr, m.first), m.second));
        }
        if (isSimplifiable(expr)) {
          Expression leftCopy = expr.args[0];
          Expression rightCopy = expr.args[1];
          // FIXME: instead of ATOMIC_EDIT simplification should depend on the size of deleted subexpression
          result.push_back(make_pair(leftCopy, PatchMetadata{ModificationKind::SIMPLIFICATION, ATOMIC_EDIT}));
          result.push_back(make_pair(rightCopy, PatchMetadata{ModificationKind::SIMPLIFICATION, ATOMIC_EDIT}));
        }
      }
    }
    return result;
  }

}

namespace generate {

  const string POINTER_ARG_NAME = "__ptr_vals";
  const string ID_TYPE = "unsigned long";
  const string PARAM_TYPE = "int";
  const string INT2_TYPE = "int"; //NOTE: need to cast INT2 explicitly inside patches

  void substituteWithRuntimeRepr(Expression &expression,
                                 unordered_map<string, string> &runtimeReprBySource) {
    if (expression.kind == NodeKind::VARIABLE) {
      expression.repr = runtimeReprBySource[expression.repr];
    } else {
      for (auto &arg : expression.args) {
        substituteWithRuntimeRepr(arg, runtimeReprBySource);
      }
    }
  }

  string locationNameSuffix(Location loc) {
    std::ostringstream result;
    result << loc.fileId << "_"
           << loc.beginLine << "_"
           << loc.beginColumn << "_"
           << loc.endLine << "_"
           << loc.endColumn;
    return result.str();
  }

  string argNameByNonPtrType(const string &typeName) {
    string result = typeName;
    std::replace(result.begin(), result.end(), ' ', '_');
    return "__" + result + "_vals";
  }

  void runtimeLoader(std::ostream &OUT, const fs::path &partitionFile, const Config &cfg) {
    OUT << "struct __f1xid_t {" << "\n"
        << ID_TYPE << " base;" << "\n"
        << ID_TYPE << " int2;" << "\n"
        << ID_TYPE << " bool2;" << "\n"
        << ID_TYPE << " cond3;" << "\n"
        << ID_TYPE << " param;" << "\n"
        << "};" << "\n";

    OUT << ID_TYPE << " __f1xapp = strtoul(getenv(\"F1X_APP\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_base = strtoul(getenv(\"F1X_ID_BASE\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_int2 = strtoul(getenv(\"F1X_ID_INT2\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_bool2 = strtoul(getenv(\"F1X_ID_BOOL2\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_cond3 = strtoul(getenv(\"F1X_ID_COND3\"), (char **)NULL, 10);" << "\n"
        << ID_TYPE << " __f1xid_param = strtoul(getenv(\"F1X_ID_PARAM\"), (char **)NULL, 10);" << "\n";

    OUT << "std::vector<__f1xid_t> __f1x_init_runtime() {" << "\n";
    if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
      OUT << "std::ifstream ifs(\"" << partitionFile.string() << "\");" << "\n"
          << "std::vector<__f1xid_t> ids;" << "\n"
          << "__f1xid_t id;" << "\n"
          << "while (ifs >> id.base) {" << "\n"
          << "ifs >> id.int2 >> id.bool2 >> id.cond3 >> id.param;" << "\n"
          << "ids.push_back(id);" << "\n"
          << "}" << "\n"
          << "return ids;" << "\n";
    }
    OUT << "}" << "\n"
        << "std::vector<__f1xid_t> __f1xids = __f1x_init_runtime();" << "\n";
  }


  string parameterList(shared_ptr<SchemaApplication> sa) {
    std::ostringstream result;

    vector<string> types;
    bool hasPointers = false;
    for (auto &c : sa->components) {
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
      result << type << " " << argNameByNonPtrType(type) << "[]";
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


  unordered_map<string, string> runtimeRenaming(shared_ptr<SchemaApplication> sa) {
    vector<string> nonPtrTypes;
    vector<Expression> pointers;
    for (auto &c : sa->components) {
      switch (c.type) {
      case Type::INTEGER:
        if(std::find(nonPtrTypes.begin(), nonPtrTypes.end(), c.rawType) == nonPtrTypes.end())
          nonPtrTypes.push_back(c.rawType);
        break;
      case Type::POINTER:
        pointers.push_back(c);
        break;
      default:
        throw std::invalid_argument("unsupported component type");
      }
    }

    std::stable_sort(nonPtrTypes.begin(), nonPtrTypes.end());

    unordered_map<string, vector<string>> reprsByType;
    for (auto &type : nonPtrTypes) {
      reprsByType[type] = vector<string>();
    }
    for (auto &c : sa->components) {
      if (c.type == Type::INTEGER) {
        reprsByType[c.rawType].push_back(c.repr);
      }
    }

    unordered_map<string, string> runtimeReprBySource;
    for (auto &type : nonPtrTypes) {
      auto reprs = reprsByType[type];
      for (int index = 0; index < reprs.size(); index++) {
        runtimeReprBySource[reprs[index]] =
          argNameByNonPtrType(type) + "[" + to_string(index) + "]";
      }
    }
    for (int index = 0; index < pointers.size(); index++) {
      runtimeReprBySource[pointers[index].repr] =
        POINTER_ARG_NAME + "[" + to_string(index) + "]";
    }
    
    return runtimeReprBySource;
  }


  void modificationsAndDispatch(shared_ptr<SchemaApplication> sa,
                                uint &baseId,
                                std::ostream &OS,
                                vector<SearchSpaceElement> &ss) {
    unordered_map<string, string> runtimeReprBySource = runtimeRenaming(sa);

    vector<pair<Expression, PatchMetadata>> modifications =
      synthesize::modifications(sa->original, sa->components);
  
    string outputType;
    if (sa->original.type == Type::POINTER) {
      outputType= "void*";
    } else {
      outputType = sa->original.rawType;
    }

    for (auto &candidate : modifications) {
      Expression runtimeExpr = candidate.first;
      substituteWithRuntimeRepr(runtimeExpr, runtimeReprBySource);
      string castStr;
      if (runtimeExpr.type == Type::POINTER && sa->original.type != Type::POINTER)
        castStr = "(std::size_t)"; //FIXME: this is temporary, type inference makes it irrelevent

      OS << "case " << baseId << ":" << "\n"
         << "base_value = " << castStr << expressionToString(runtimeExpr) << ";" << "\n"
         << "break;" << "\n";
      
      F1XID f1xid{0}; //FIXME: should assign all ids
      f1xid.base = baseId;
      ss.push_back(SearchSpaceElement{f1xid, sa, candidate.first, candidate.second});

      baseId++;
    }
  }

  void partitioningFunctions(const vector<shared_ptr<SchemaApplication>> &schemaApplications,
                             const fs::path &workDir,
                             std::ostream &OS,
                             vector<SearchSpaceElement> &searchSpace,
                             const Config &cfg) {

    fs::path partitionInputFile = workDir / PARTITION_IN;
    fs::path partitionOutputFile = workDir / PARTITION_OUT;

    OS << "#include \"rt.h\"" << "\n"
       << "#include <stdlib.h>" << "\n"
       << "#include <vector>" << "\n"
       << "#include <fstream>" << "\n";

    generate::runtimeLoader(OS, partitionInputFile, cfg);

    uint baseid = 0;

    for (auto sa : schemaApplications) {
      string outputType;
      if (sa->original.type == Type::POINTER) {
        outputType= "void*";
      } else {
        outputType = sa->original.rawType;
      }

      OS << outputType << " __f1x_"
         << locationNameSuffix(sa->location)
         << "(" << generate::parameterList(sa) << ")"
         << "{" << "\n";

      OS << "__f1xid_t id;" << "\n"
         << "id.base = __f1xid_base;" << "\n"
         << "id.int2 = __f1xid_int2;" << "\n"
         << "id.bool2 = __f1xid_bool2;" << "\n"
         << "id.cond3 = __f1xid_cond3;" << "\n"
         << "id.param = __f1xid_param;" << "\n";

      OS << outputType << " base_value;" << "\n"
         << INT2_TYPE << " int2_value;" << "\n"
         << "bool bool2_value;" << "\n"
         << "bool cond3_value;" << "\n"
         << PARAM_TYPE << " param_value;" << "\n";

      OS << outputType << " output_value = 0;" << "\n"
         << "bool output_initialized = false;" << "\n";

      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        OS << "unsigned long index = 0;" << "\n";
        OS << "std::ofstream ofs;" << "\n"
           << "ofs.open(" << partitionOutputFile << ", std::ofstream::out | std::ofstream::app);" << "\n";
      } else {
        OS << "unsigned long index = __f1xids.size();" << "\n";
      }

      OS << "label_" << locationNameSuffix(sa->location) << ":" << "\n";

      OS << "param_value = id.param;" << "\n";

      OS << "switch (id.base) {" << "\n";
      generate::modificationsAndDispatch(sa, baseid, OS, searchSpace);
      OS << "}" << "\n";

      OS << "if (!output_initialized) {" << "\n"
         << "output_value = base_value;" << "\n"
         << "output_initialized = true;" << "\n"
         << "} else if (output_value == base_value) {" << "\n";
      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        OS << "ofs << \" \" << id.base << \" \" << id.int2 << \" \" << id.bool2 << \" \" << id.cond3 << \" \" << id.param;" << "\n";
      }
      OS << "}" << "\n";

      OS << "if (__f1xids.size() > index) {" << "\n"
         << "id = __f1xids[index];" << "\n"
         << "index++;" << "\n"
         << "goto " << "label_" << locationNameSuffix(sa->location) << ";" << "\n"
         << "}" << "\n";

      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        OS << "ofs << '\\n';" << "\n";
        OS << "ofs.close();" << "\n";
      }
      OS << "return output_value;" << "\n";

      OS << "}" << "\n";
    }

  }

}


vector<SearchSpaceElement> generateSearchSpace(const vector<shared_ptr<SchemaApplication>> &schemaApplications,
                                               const fs::path &workDir,
                                               std::ostream &OS,
                                               std::ostream &OH,
                                               const Config &cfg) {
  
  // header

  OH << "#ifdef __cplusplus" << "\n"
     << "extern \"C\" {" << "\n"
     << "#endif" << "\n"
     << "extern " << generate::ID_TYPE << " __f1xapp;" << "\n";

  for (auto sa : schemaApplications) {
    string outputType;
    if (sa->original.type == Type::POINTER) {
      outputType= "void*";
    } else {
      outputType = sa->original.rawType;
    }

    OH << outputType << " __f1x_" 
       << generate::locationNameSuffix(sa->location)
       << "(" << generate::parameterList(sa) << ")"
       << ";" << "\n";
  }

  OH << "#ifdef __cplusplus" << "\n"
     << "}" << "\n"
     << "#endif" << "\n";

  // source

  vector<SearchSpaceElement> searchSpace;
  
  generate::partitioningFunctions(schemaApplications, workDir, OS, searchSpace, cfg);  

  return searchSpace;
}
