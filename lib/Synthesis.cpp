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
 - synthesis for everything specific to expression synthesis 
 - generator for runtime code generation
 */

namespace synthesis {

  // size of simple (atomic) modification
  const ulong ATOMIC_EDIT = 1;

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


  vector<pair<Expression, PatchMetadata>> baseModifications(const Expression &expr,
                                                        const vector<Expression> &components) {
    vector<pair<Expression, PatchMetadata>> result;
    if (expr.args.size() == 0) {
      if (expr.kind == NodeKind::CONSTANT) {
        if (expr.type == Type::INTEGER) {
          for (auto &c : components) {
            if (c.type == Type::INTEGER)
              result.push_back(make_pair(c, PatchMetadata{ModificationKind::GENERALIZATION, ATOMIC_EDIT}));
          }
          result.push_back(make_pair(PARAMETER_NODE,
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
          result.push_back(make_pair(NULL_NODE,
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
        vector<pair<Expression, PatchMetadata>> argMods = baseModifications(expr.args[0], components);
        for (auto &m : argMods) {
          result.push_back(make_pair(makeArgSubs(expr, m.first), m.second));
        }
        if (isSimplifiable(expr)) {
          Expression argCopy = expr.args[0];
          // FIXME: instead of ATOMIC_EDIT simplification should depend on the size of deleted subexpression
          result.push_back(make_pair(argCopy, PatchMetadata{ModificationKind::SIMPLIFICATION, ATOMIC_EDIT}));
        }
      } else if (expr.args.size() == 2) {
        vector<pair<Expression, PatchMetadata>> leftMods = baseModifications(expr.args[0], components);
        for (auto &m : leftMods) {
          result.push_back(make_pair(makeLeftSubs(expr, m.first), m.second));
        }
        vector<pair<Expression, PatchMetadata>> rightMods = baseModifications(expr.args[1], components);
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

namespace generator {

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

  void runtimeLoader(std::ostream &OUT, const Config &cfg) {
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
        << ID_TYPE << " __f1xid_param = strtoul(getenv(\"F1X_ID_PARAM\"), (char **)NULL, 10);" << "\n"
        << "__f1xid_t *__f1xids = NULL;" << "\n";

    OUT << "void __f1x_init_runtime() {" << "\n";
    if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
      OUT << "int fd = shm_open(\"" << PARTITION_FILE_NAME << "\", O_RDWR, 0);" << "\n"
          << "struct stat sb;" << "\n"
          << "fstat(fd, &sb);" << "\n"
          << "__f1xids = (__f1xid_t*) mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);" << "\n"
          << "close(fd);" << "\n";
    }
    OUT << "}" << "\n";
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

  bool isAbstractExpression(const Expression &expression) {
    if (isAbstractNode(expression.kind)) {
      return true;
    } else {
      for (auto &arg : expression.args) {
        if(isAbstractExpression(arg))
          return true;
      }
    }
    return false;
  }

  bool substituteAbstractNode(Expression &expression,
                              NodeKind abstractKind, 
                              const Expression &substitution) {
    if (expression.kind == abstractKind) {
      expression = substitution;
      return true;
    } else {
      for (auto &arg : expression.args) {
        if(substituteAbstractNode(arg, abstractKind, substitution))
          return true;
      }
    }
    return false;
  }

  void modificationsAndDispatch(shared_ptr<SchemaApplication> sa,
                                ulong &baseId,
                                std::ostream &OS,
                                vector<SearchSpaceElement> &ss,
                                const Config &cfg) {
    unordered_map<string, string> runtimeReprBySource = runtimeRenaming(sa);

    vector<pair<Expression, PatchMetadata>> modifications =
      synthesis::baseModifications(sa->original, sa->components);
  
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

      if (isAbstractExpression(runtimeExpr)) {
        ulong paramBound;
        if (sa->context == LocationContext::CONDITION) {
          paramBound = cfg.maxConditionParameter;
        } else {
          paramBound = cfg.maxExpressionParameter;
        }
        for (int i = 0; i <= paramBound; i++) {
          F1XID f1xid{0}; //FIXME: should assign all ids
          f1xid.base = baseId;
          f1xid.param = i;
          Expression instance = candidate.first;
          substituteAbstractNode(instance, NodeKind::PARAMETER, getIntegerExpression(i));
          ss.push_back(SearchSpaceElement{f1xid, sa, instance, candidate.second});
        }
      } else {
        F1XID f1xid{0}; //FIXME: should assign all ids
        f1xid.base = baseId;
        ss.push_back(SearchSpaceElement{f1xid, sa, candidate.first, candidate.second});
      }

      baseId++;
    }
  }

  void partitioningFunctions(const vector<shared_ptr<SchemaApplication>> &schemaApplications,
                             const fs::path &workDir,
                             std::ostream &OS,
                             vector<SearchSpaceElement> &searchSpace,
                             const Config &cfg) {

    OS << "#include \"rt.h\"" << "\n"
       << "#include <stdlib.h>" << "\n"
       << "#include <vector>" << "\n"
       << "#include <cstddef>" << "\n"
       << "#include <unistd.h>" << "\n"
       << "#include <fcntl.h>" << "\n"
       << "#include <sys/stat.h>" << "\n"
       << "#include <sys/mman.h>" << "\n";


    generator::runtimeLoader(OS, cfg);

    ulong baseId = 1; // because 0 is reserved:

    for (auto sa : schemaApplications) {
      string outputType;
      if (sa->original.type == Type::POINTER) {
        outputType= "void*";
      } else {
        outputType = sa->original.rawType;
      }

      OS << outputType << " __f1x_"
         << locationNameSuffix(sa->location)
         << "(" << generator::parameterList(sa) << ")"
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
         << "bool output_initialized = false;" << "\n"
         << "unsigned long input_index = 0;" << "\n"
         << "unsigned long output_index = 0;" << "\n";

      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        OS << "if (__f1xids == NULL) __f1x_init_runtime();" << "\n";
      }

      OS << "label_" << locationNameSuffix(sa->location) << ":" << "\n";

      OS << "param_value = id.param;" << "\n";

      OS << "switch (id.base) {" << "\n";
      generator::modificationsAndDispatch(sa, baseId, OS, searchSpace, cfg);
      OS << "}" << "\n";

      OS << "if (!output_initialized) {" << "\n"
         << "output_value = base_value;" << "\n"
         << "output_initialized = true;" << "\n"
         << "} else if (output_value == base_value) {" << "\n";
      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        OS << "__f1xids[output_index] = id;" << "\n"
           << "output_index++;" << "\n";
      }
      OS << "}" << "\n";

      OS << "if (__f1xids && __f1xids[input_index].base != 0) {" << "\n"
         << "id = __f1xids[input_index];" << "\n"
         << "input_index++;" << "\n"
         << "goto " << "label_" << locationNameSuffix(sa->location) << ";" << "\n"
         << "}" << "\n";

      if (cfg.exploration == Exploration::SEMANTIC_PARTITIONING) {
        // output terminator:
        OS << "__f1xids[output_index] = __f1xid_t{0, 0, 0, 0, 1};" << "\n";
      }
      OS << "return output_value;" << "\n";

      OS << "}" << "\n";
    }

  }

}


vector<SearchSpaceElement> 
generateSearchSpace(const vector<shared_ptr<SchemaApplication>> &schemaApplications,
                    const fs::path &workDir,
                    std::ostream &OS,
                    std::ostream &OH,
                    const Config &cfg) {
  
  // header

  OH << "#ifdef __cplusplus" << "\n"
     << "extern \"C\" {" << "\n"
     << "#endif" << "\n"
     << "extern " << generator::ID_TYPE << " __f1xapp;" << "\n";

  for (auto sa : schemaApplications) {
    string outputType;
    if (sa->original.type == Type::POINTER) {
      outputType= "void*";
    } else {
      outputType = sa->original.rawType;
    }

    OH << outputType << " __f1x_" 
       << generator::locationNameSuffix(sa->location)
       << "(" << generator::parameterList(sa) << ")"
       << ";" << "\n";
  }

  OH << "#ifdef __cplusplus" << "\n"
     << "}" << "\n"
     << "#endif" << "\n";

  // source

  vector<SearchSpaceElement> searchSpace;
  
  generator::partitioningFunctions(schemaApplications, workDir, OS, searchSpace, cfg);  

  return searchSpace;
}
