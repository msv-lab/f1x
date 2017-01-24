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

#include <algorithm>
#include <map>
#include <stack>
#include <sstream>

#include "clang/Lex/Preprocessor.h"
#include "clang/AST/Type.h"
#include "llvm/Support/raw_ostream.h"

#include "TransformationUtil.h"
#include "F1XConfig.h"

namespace json = rapidjson;

using namespace clang;
using namespace llvm;
using std::string;
using std::pair;
using std::stack;
using std::vector;
using std::map;


const BuiltinType::Kind DEFAULT_NUMERIC_TYPE = BuiltinType::Long;
const string DEFAULT_POINTEE_TYPE = "void";


uint globalFileId;
uint globalFromLine;
uint globalToLine;
string globalOutputFile;
string globalProfileFile;
uint globalBeginLine;
uint globalBeginColumn;
uint globalEndLine;
uint globalEndColumn;
string globalPatch;
uint globalBaseLocId = 0;

const uint F1XLOC_WIDTH = 32;
const uint F1XLOC_VALUE_BITS = 10;

/*
  __f1x_loc is a F1XID_WIDTH bit transparent location ID. The left F1XID_VALUE_BITS bits of this id is the file ID.
 */

uint f1xloc(uint baseId, uint fileId) {
  assert(baseId < (1 << (F1XLOC_WIDTH - F1XLOC_VALUE_BITS)));
  uint result = fileId;
  result <<= (F1XLOC_WIDTH - F1XLOC_VALUE_BITS);
  result += baseId;
  return result;
}


bool inRange(uint line) {
  if (globalFromLine || globalToLine) {
    return globalFromLine <= line && line <= globalToLine;
  } else {
    return true;
  }
}


uint getDeclExpandedLine(const Decl* decl, SourceManager &srcMgr) {
  SourceLocation startLoc = decl->getLocStart();
  if(startLoc.isMacroID()) {
    // Get the start/end expansion locations
    pair<SourceLocation, SourceLocation> expansionRange = srcMgr.getExpansionRange(startLoc);
    // We're just interested in the start location
    startLoc = expansionRange.first;
  }

  return srcMgr.getExpansionLineNumber(startLoc);
}


bool insideMacro(const Stmt* expr, SourceManager &srcMgr, const LangOptions &langOpts) {
  SourceLocation startLoc = expr->getLocStart();
  SourceLocation endLoc = expr->getLocEnd();

  if(startLoc.isMacroID() && !Lexer::isAtStartOfMacroExpansion(startLoc, srcMgr, langOpts))
    return true;
  
  if(endLoc.isMacroID() && !Lexer::isAtEndOfMacroExpansion(endLoc, srcMgr, langOpts))
    return true;

  return false;
}


SourceRange getExpandedLoc(const Stmt* expr, SourceManager &srcMgr) {
  SourceLocation startLoc = expr->getLocStart();
  SourceLocation endLoc = expr->getLocEnd();

  if(startLoc.isMacroID()) {
    // Get the start/end expansion locations
    pair<SourceLocation, SourceLocation> expansionRange = srcMgr.getExpansionRange(startLoc);
    // We're just interested in the start location
    startLoc = expansionRange.first;
  }
  if(endLoc.isMacroID()) {
    // Get the start/end expansion locations
    pair<SourceLocation, SourceLocation> expansionRange = srcMgr.getExpansionRange(endLoc);
    // We're just interested in the start location
    endLoc = expansionRange.second; // TODO: I am not sure about it
  }
      
  SourceRange expandedLoc(startLoc, endLoc);

  return expandedLoc;
}


string toString(const Stmt *stmt) {
  /* Special case for break and continue statement
     Reason: There were semicolon ; and newline found
     after break/continue statement was converted to string
  */
  if (dyn_cast<BreakStmt>(stmt))
    return "break";
  
  if (dyn_cast<ContinueStmt>(stmt))
    return "continue";

  LangOptions LangOpts;
  PrintingPolicy Policy(LangOpts);
  string str;
  raw_string_ostream rso(str);

  stmt->printPretty(rso, nullptr, Policy);

  string stmtStr = rso.str();
  return stmtStr;
}


bool overwriteMainChangedFile(Rewriter &TheRewriter) {
  bool AllWritten = true;
  FileID id = TheRewriter.getSourceMgr().getMainFileID();
  const FileEntry *Entry = TheRewriter.getSourceMgr().getFileEntryForID(id);
  std::error_code err_code;
  auto buffer = TheRewriter.getRewriteBufferFor(id);
  if (buffer) {// if there are modifications
    raw_fd_ostream out(Entry->getName(), err_code, sys::fs::F_None);
    buffer->write(out);
    out.close();
  }
  return !AllWritten;
}

bool isTopLevelStatement(const Stmt *stmt, ASTContext *context) {
  auto it = context->getParents(*stmt).begin();

  if(it == context->getParents(*stmt).end())
    return true;

  const CompoundStmt* cs;
  if ((cs = it->get<CompoundStmt>()) != NULL) {
    return true;
  }

  const IfStmt* is;
  if ((is = it->get<IfStmt>()) != NULL) {
    return stmt != is->getCond();
  }

  const ForStmt* fs;
  if ((fs = it->get<ForStmt>()) != NULL) {
    return stmt != fs->getCond() && stmt != fs->getInc() && stmt != fs->getInit();
  }

  const WhileStmt* ws;
  if ((ws = it->get<WhileStmt>()) != NULL) {
    return stmt != ws->getCond();
  }
    
  return false;
}

bool shouldAddBrackets(const Stmt *stmt, ASTContext *context)
{
  auto it = context->getParents(*stmt).begin();
  const IfStmt* is;
  if ((is = it->get<IfStmt>()) != NULL) {
    return true;
  }
  const ForStmt* fs;
  if ((fs = it->get<ForStmt>()) != NULL) {
    return true;
  }
  const WhileStmt* ws;
  if ((ws = it->get<WhileStmt>()) != NULL) {
    return true;
  }
  return false;
}

BuiltinType::Kind getBuiltinKind(const QualType &type) {
  QualType canon = type.getCanonicalType();
  assert(canon.getTypePtr());
  if (const BuiltinType *bt = canon.getTypePtr()->getAs<BuiltinType>()) {
    return bt->getKind();
  } else {
    llvm::errs() << "error: non-builtin type " << type.getAsString() << "\n";
    return DEFAULT_NUMERIC_TYPE;
  }
}

bool isPointerType(const QualType &type) {
  QualType canon = type.getCanonicalType();
  assert(canon.getTypePtr());
  return canon.getTypePtr()->isPointerType();
}

string getPointeeType(const QualType &type) {
  QualType canon = type.getCanonicalType();
  assert(canon.getTypePtr());
  if (const PointerType *pt = canon.getTypePtr()->getAs<PointerType>()) {
    return pt->getPointeeType().getCanonicalType().getAsString();
  } else {
    llvm::errs() << "error: non-pointer type " << type.getAsString() << "\n";
    return DEFAULT_POINTEE_TYPE;
  }
}

// TODO: support C++ types, compiler extensions
string kindToString(const BuiltinType::Kind kind) {
  switch (kind) {
  case BuiltinType::Char_U:
    return "char";
  case BuiltinType::UChar:
    return "unsigned char";
  case BuiltinType::WChar_U:
    return "wchar_t";
  case BuiltinType::UShort:
    return "unsigned short";
  case BuiltinType::UInt:
    return "unsigned int";
  case BuiltinType::ULong:
    return "unsigned long";
  case BuiltinType::ULongLong:
    return "unsigned long long";
  case BuiltinType::Char_S:
    return "char";
  case BuiltinType::SChar:
    return "char";
  case BuiltinType::WChar_S:
    return "wchar_t";
  case BuiltinType::Short:
    return "short";
  case BuiltinType::Int:
    return "int";
  case BuiltinType::Long:
    return "long";
  case BuiltinType::LongLong:
    return "long long";
  default:
    llvm::errs() << "warning: unsupported builtin type " << kind << "\n";
    return kindToString(DEFAULT_NUMERIC_TYPE);
  }
}

class StmtToJSON : public StmtVisitor<StmtToJSON> {
  json::Document::AllocatorType *allocator;
  PrintingPolicy policy;
  stack<json::Value> path;

public:
  StmtToJSON(const PrintingPolicy &policy,
             json::Document::AllocatorType *allocator):
    policy(policy),
    allocator(allocator) {}

  json::Value getValue() {
    assert(path.size() == 1);
    return std::move(path.top());
  }

  void Visit(Stmt* S) {
    StmtVisitor<StmtToJSON>::Visit(S);
  }

  void VisitBinaryOperator(BinaryOperator *Node) {
    json::Value node(json::kObjectType);
    
    node.AddMember("kind", json::Value().SetString("operator"), *allocator);
    string t = kindToString(getBuiltinKind(Node->getType()));
    node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    json::Value repr;
    string opcode_str = BinaryOperator::getOpcodeStr(Node->getOpcode()).lower();
    repr.SetString(opcode_str.c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    json::Value args(json::kArrayType);
    Visit(Node->getLHS());
    Visit(Node->getRHS());
    json::Value right = std::move(path.top());
    path.pop();
    json::Value left = std::move(path.top());
    path.pop();
    args.PushBack(left, *allocator);
    args.PushBack(right, *allocator);
    
    node.AddMember("args", args, *allocator);

    path.push(std::move(node));
  }

  void VisitUnaryOperator(UnaryOperator *Node) {
    json::Value node(json::kObjectType);

    node.AddMember("kind", json::Value().SetString("operator"), *allocator);
    string t = kindToString(getBuiltinKind(Node->getType()));
    node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    json::Value repr;
    string opcode_str = UnaryOperator::getOpcodeStr(Node->getOpcode());
    repr.SetString(opcode_str.c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    llvm::errs() << opcode_str << "\n";

    json::Value args(json::kArrayType);
    Visit(Node->getSubExpr());
    json::Value arg = std::move(path.top());
    path.pop();
    args.PushBack(arg, *allocator);
    node.AddMember("args", args, *allocator);

    path.push(std::move(node));
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitCastExpr(CastExpr *Node) {
    // NOTE: currently, two types of cases are supported: 
    // *  casts of atoms (variables and memeber exprs)
    // *  special case for NULL
    // I need to save the cast, since it affect the semantics, but without side effects
   json::Value node(json::kObjectType);
   if (isPointerType(Node->getType())) {
     node.AddMember("kind", json::Value().SetString("pointer"), *allocator);
     string t = getPointeeType(Node->getType());
     node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
   } else {
     node.AddMember("kind", json::Value().SetString("object"), *allocator);
     string t = kindToString(getBuiltinKind(Node->getType()));
     node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
   }
   json::Value repr;
   repr.SetString(toString(Node).c_str(), *allocator);
   node.AddMember("repr", repr, *allocator);
   
   path.push(std::move(node));
  }

  void VisitParenExpr(ParenExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitMemberExpr(MemberExpr *Node) {
    json::Value node(json::kObjectType);

    if (isPointerType(Node->getType())) {
      node.AddMember("kind", json::Value().SetString("pointer"), *allocator);
      string t = getPointeeType(Node->getType());
      node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    } else {
      node.AddMember("kind", json::Value().SetString("object"), *allocator);
      string t = kindToString(getBuiltinKind(Node->getType()));
      node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    }
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {
    json::Value node(json::kObjectType);

    node.AddMember("kind", json::Value().SetString("constant"), *allocator);
    string t = kindToString(getBuiltinKind(Node->getType()));
    node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitCharacterLiteral(CharacterLiteral *Node) {
    json::Value node(json::kObjectType);

    node.AddMember("kind", json::Value().SetString("constant"), *allocator);
    string t = kindToString(getBuiltinKind(Node->getType()));
    node.AddMember("type", json::Value().SetString("char"), *allocator);
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    json::Value node(json::kObjectType);

    if (isPointerType(Node->getType())) {
      node.AddMember("kind", json::Value().SetString("pointer"), *allocator);
      string t = getPointeeType(Node->getType());
      node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    } else {
      node.AddMember("kind", json::Value().SetString("object"), *allocator);
      string t = kindToString(getBuiltinKind(Node->getType()));
      node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    }
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    json::Value node(json::kObjectType);

    if (isPointerType(Node->getType())) {
      node.AddMember("kind", json::Value().SetString("pointer"), *allocator);
      string t = getPointeeType(Node->getType());
      node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    } else {
      node.AddMember("kind", json::Value().SetString("object"), *allocator);
      string t = kindToString(getBuiltinKind(Node->getType()));
      node.AddMember("type", json::Value().SetString(t.c_str(), *allocator), *allocator);
    }
    json::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

};


json::Value stmtToJSON(const clang::Stmt *stmt,
                       json::Document::AllocatorType &allocator) {
    PrintingPolicy policy = PrintingPolicy(LangOptions());
    StmtToJSON T(policy, &allocator);
    T.Visit(const_cast<Stmt*>(stmt));
    return T.getValue();
}


json::Value locToJSON(uint fileId, uint bl, uint bc, uint el, uint ec,
                      json::Document::AllocatorType &allocator) {
  json::Value entry(json::kObjectType);
  entry.AddMember("fileId", json::Value().SetInt(fileId), allocator);
  entry.AddMember("beginLine", json::Value().SetInt(bl), allocator);
  entry.AddMember("beginColumn", json::Value().SetInt(bc), allocator);
  entry.AddMember("endLine", json::Value().SetInt(el), allocator);
  entry.AddMember("endColumn", json::Value().SetInt(ec), allocator);
  return entry;
}


bool isSuitableComponentType(const QualType &type) {
  return type.getTypePtr()->isIntegerType()
      || type.getTypePtr()->isCharType()
      || type.getTypePtr()->isPointerType();
}


class CollectComponents : public StmtVisitor<CollectComponents> {
private:
  json::Document::AllocatorType *allocator;
  bool ignoreCasts;
  bool suitableTypes;
  vector<json::Value> collected;

public:
  CollectComponents(json::Document::AllocatorType *allocator, bool ignoreCasts, bool suitableTypes):
    allocator(allocator),
    ignoreCasts(ignoreCasts),
    suitableTypes(suitableTypes) {}

  vector<json::Value> getCollected() {
    return std::move(collected);
  }

  void Visit(Stmt* S) {
    StmtVisitor<CollectComponents>::Visit(S);
  }

  void VisitBinaryOperator(BinaryOperator *Node) {
    Visit(Node->getLHS());
    Visit(Node->getRHS());
  }

  void VisitUnaryOperator(UnaryOperator *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitCastExpr(CastExpr *Node) {
    if (! ignoreCasts)
      collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitParenExpr(ParenExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {}

  void VisitCharacterLiteral(CharacterLiteral *Node) {}

  void VisitMemberExpr(MemberExpr *Node) {
    if (!suitableTypes || isSuitableComponentType(Node->getType()))
      collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    if (!suitableTypes || isSuitableComponentType(Node->getType()))
      collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    if (!suitableTypes || isSuitableComponentType(Node->getType()))
      collected.push_back(stmtToJSON(Node, *allocator));
  }

};


vector<json::Value> collectFromExpression(const Stmt *stmt,
                                          json::Document::AllocatorType &allocator,
                                          bool ignoreCasts,
                                          bool suitableTypes) {
  CollectComponents T(&allocator, ignoreCasts, suitableTypes);
  T.Visit(const_cast<Stmt*>(stmt));
  return T.getCollected();
}


json::Value varDeclToJSON(VarDecl *vd, json::Document::AllocatorType &allocator) {
  json::Value node(json::kObjectType);
  if (isPointerType(vd->getType())) {
    node.AddMember("kind", json::Value().SetString("pointer"), allocator);
    string t = getPointeeType(vd->getType());
    node.AddMember("type", json::Value().SetString(t.c_str(), allocator), allocator);
  } else {
    node.AddMember("kind", json::Value().SetString("object"), allocator);
    string t = kindToString(getBuiltinKind(vd->getType()));
    node.AddMember("type", json::Value().SetString(t.c_str(), allocator), allocator);
  }
  json::Value repr;
  string name = vd->getName();
  repr.SetString(name.c_str(), allocator);
  node.AddMember("repr", repr, allocator);
  return node;
}


vector<json::Value> collectVisible(const ast_type_traits::DynTypedNode &node,
                                   uint line,
                                   ASTContext* context,
                                   json::Document::AllocatorType &allocator) {
  vector<json::Value> result;

  const FunctionDecl* fd;
  if ((fd = node.get<FunctionDecl>()) != NULL) {

    // adding function parameters
    for (auto it = fd->param_begin(); it != fd->param_end(); ++it) {
      auto vd = cast<VarDecl>(*it);
      if (isSuitableComponentType(vd->getType())) {
        result.push_back(varDeclToJSON(vd, allocator));
      }
    }

    if (USE_GLOBAL_VARIABLES) {
      auto parents = context->getParents(node);
      if (parents.size() > 0) {
        const ast_type_traits::DynTypedNode parent = *(parents.begin()); // FIXME: for now only first
        const TranslationUnitDecl* tu;
        if ((tu = parent.get<TranslationUnitDecl>()) != NULL) {
          for (auto it = tu->decls_begin(); it != tu->decls_end(); ++it) {
            if (isa<VarDecl>(*it)) {
              VarDecl* vd = cast<VarDecl>(*it);
              unsigned beginLine = getDeclExpandedLine(vd, context->getSourceManager());
              if (line > beginLine && isSuitableComponentType(vd->getType())) {
                result.push_back(varDeclToJSON(vd, allocator));
              }
            }
          }
        }
      }
    }
    
  } else {

    const CompoundStmt* cstmt;
    if ((cstmt = node.get<CompoundStmt>()) != NULL) {
      for (auto it = cstmt->body_begin(); it != cstmt->body_end(); ++it) {

        if (isa<BinaryOperator>(*it)) {
          BinaryOperator* op = cast<BinaryOperator>(*it);
          SourceRange expandedLoc = getExpandedLoc(op, context->getSourceManager());
          uint beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          // FIXME: support augmented assignments:
          // FIXME: is it redundant if we use collect funnction on whole statement
          if (line > beginLine &&
              BinaryOperator::getOpcodeStr(op->getOpcode()).lower() == "=" &&
              isa<DeclRefExpr>(op->getLHS())) {
            DeclRefExpr* dref = cast<DeclRefExpr>(op->getLHS());
            VarDecl* vd;
            if ((vd = cast<VarDecl>(dref->getDecl())) != NULL && isSuitableComponentType(vd->getType())) {
              result.push_back(varDeclToJSON(vd, allocator));
            }
          }
        }

        if (isa<DeclStmt>(*it)) {
          DeclStmt* dstmt = cast<DeclStmt>(*it);
          SourceRange expandedLoc = getExpandedLoc(dstmt, context->getSourceManager());
          uint beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          for (auto it = dstmt->decl_begin(); it != dstmt->decl_end(); ++it) {
            Decl* d = *it;
            if (isa<VarDecl>(d)) {
              VarDecl* vd = cast<VarDecl>(d);
              // NOTE: hasInit because don't want to use garbage
              if (line > beginLine && vd->hasInit() && isSuitableComponentType(vd->getType())) {
                result.push_back(varDeclToJSON(vd, allocator));
              }
            }
          }
        }

        Stmt* stmt = cast<Stmt>(*it);
        SourceRange expandedLoc = getExpandedLoc(stmt, context->getSourceManager());
        unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
        if (line > beginLine) {
          vector<json::Value> fromExpr = collectFromExpression(*it, allocator, true, true);
          for (auto &c : fromExpr) {
            result.push_back(std::move(c));
          }
          
          // FIXME: is it redundant?
          // TODO: should be generalized for other cases:
          if (isa<IfStmt>(*it)) {
            IfStmt* ifStmt = cast<IfStmt>(*it);
            Stmt* thenStmt = ifStmt->getThen();
            if (isa<CallExpr>(*thenStmt)) {
              CallExpr* callExpr = cast<CallExpr>(thenStmt);
              for (auto a = callExpr->arg_begin(); a != callExpr->arg_end(); ++a) {
                auto e = cast<Expr>(*a);
                vector<json::Value> fromParamExpr = collectFromExpression(e, allocator, true, true);
                for (auto &c : fromParamExpr) {
                  result.push_back(std::move(c));
                }
              }
            }
          }
        }
        
      }
    }
    
    auto parents = context->getParents(node);
    if (parents.size() > 0) {
      const ast_type_traits::DynTypedNode parent = *(parents.begin()); // TODO: for now only first
      vector<json::Value> parentResult = collectVisible(parent, line, context, allocator);
      for (auto &c : parentResult) {
        result.push_back(std::move(c));
      }
    }
  }
  
  return result;
}


vector<json::Value> collectComponents(const Stmt *stmt,
                                      uint line,
                                      ASTContext *context,
                                      json::Document::AllocatorType &allocator) {
  vector<json::Value> fromExpr = collectFromExpression(stmt, allocator, false, false);

  const ast_type_traits::DynTypedNode node = ast_type_traits::DynTypedNode::create(*stmt);
  vector<json::Value> visible = collectVisible(node, line, context, allocator);

  vector<json::Value> result;

  for (auto &c : fromExpr) {
    bool newComponent = true;
    for (auto &old : result) {
      if (c["repr"] == old["repr"])
        newComponent = false;
    }
    if (newComponent)
      result.push_back(std::move(c));
  }  

  for (auto &c : visible) {
    bool newComponent = true;
    for (auto &old : result) {
      if (c["repr"] == old["repr"])
        newComponent = false;
    }
    if (newComponent)
      result.push_back(std::move(c));
  }  

  return result;
}


string makeArgumentList(vector<json::Value> &components) {
  std::ostringstream result;

  map<string, vector<json::Value*>> typesToValues;
  vector<json::Value*> pointers;
  vector<string> types;
  for (auto &c : components) {
    string kind = c["kind"].GetString();
    if (kind == "pointer") {
      pointers.push_back(&c);
    } else {
      string type = c["type"].GetString();
      if (std::find(types.begin(), types.end(), type) == types.end()) {
        types.push_back(type);
      }
      if (typesToValues.find(type) == typesToValues.end()) {
        typesToValues.insert(std::make_pair(type, vector<json::Value*>()));
      }
      typesToValues[type].push_back(&c);
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
    result << "(" << type << "[]){";
    bool firstElement = true;
    for (auto c : typesToValues[type]) {
      if (firstElement) {
        firstElement = false;
      } else {
        result << ", ";
      }
      result << (*c)["repr"].GetString();
    }
    result << "}";
  }
  if (! pointers.empty()) {
    if (firstArray) {
      firstArray = false;
    } else {
      result << ", ";
    }
    result << "(void*[]){";
    bool firstElement = true;
    for (auto c : pointers) {
      if (firstElement) {
        firstElement = false;
      } else {
        result << ", ";
      }
      result << (*c)["repr"].GetString();
    }
    result << "}";
  }

  return result.str();
}
