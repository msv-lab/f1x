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

#include <stack>
#include <sstream>

#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/raw_ostream.h"

#include "TransformationUtil.h"
#include "Config.h"

using namespace clang;
using namespace llvm;
using std::string;
using std::pair;
using std::stack;
using std::vector;


uint globalFileId;
uint globalFromLine;
uint globalToLine;
string globalOutputFile;
uint globalBeginLine;
uint globalBeginColumn;
uint globalEndLine;
uint globalEndColumn;
string globalPatch;
uint globalBaseLocId = 0;


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
  raw_fd_ostream out(Entry->getName(), err_code, sys::fs::F_None);
  TheRewriter.getRewriteBufferFor(id)->write(out);
  out.close();
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


class StmtToJSON : public StmtVisitor<StmtToJSON> {
  rapidjson::Document::AllocatorType *allocator;
  PrintingPolicy policy;
  stack<rapidjson::Value> path;

public:
  StmtToJSON(const PrintingPolicy &policy,
             rapidjson::Document::AllocatorType *allocator):
    policy(policy),
    allocator(allocator) {}

  rapidjson::Value getValue() {
    assert(path.size() == 1);
    return std::move(path.top());
  }

  void Visit(Stmt* S) {
    StmtVisitor<StmtToJSON>::Visit(S);
  }

  void VisitBinaryOperator(BinaryOperator *Node) {
    rapidjson::Value node(rapidjson::kObjectType);
    
    node.AddMember("kind", rapidjson::Value().SetString("operator"), *allocator);
    // FIXME: should set real type
    // possibly from Expr::getType()
    node.AddMember("type", rapidjson::Value().SetString("int"), *allocator);
    rapidjson::Value repr;
    string opcode_str = BinaryOperator::getOpcodeStr(Node->getOpcode()).lower();
    repr.SetString(opcode_str.c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    rapidjson::Value args(rapidjson::kArrayType);
    Visit(Node->getLHS());
    Visit(Node->getRHS());
    rapidjson::Value right = std::move(path.top());
    path.pop();
    rapidjson::Value left = std::move(path.top());
    path.pop();
    args.PushBack(left, *allocator);
    args.PushBack(right, *allocator);
    
    node.AddMember("args", args, *allocator);

    path.push(std::move(node));
  }

  void VisitUnaryOperator(UnaryOperator *Node) {
    rapidjson::Value node(rapidjson::kObjectType);

    node.AddMember("kind", rapidjson::Value().SetString("operator"), *allocator);
    // FIXME: should set real type
    node.AddMember("type", rapidjson::Value().SetString("int"), *allocator);
    rapidjson::Value repr;
    string opcode_str = UnaryOperator::getOpcodeStr(Node->getOpcode());
    repr.SetString(opcode_str.c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    llvm::errs() << opcode_str << "\n";

    rapidjson::Value args(rapidjson::kArrayType);
    Visit(Node->getSubExpr());
    rapidjson::Value arg = std::move(path.top());
    path.pop();
    args.PushBack(arg, *allocator);
    node.AddMember("args", args, *allocator);

    path.push(std::move(node));
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitCastExpr(CastExpr *Node) {
    // FIXME: this may not always work, and I need to save explicit cast as operator
    Visit(Node->getSubExpr());
  }

  void VisitParenExpr(ParenExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitMemberExpr(MemberExpr *Node) {
    rapidjson::Value node(rapidjson::kObjectType);

    node.AddMember("kind", rapidjson::Value().SetString("variable"), *allocator);
    // FIXME: should set real type
    node.AddMember("type", rapidjson::Value().SetString("int"), *allocator);
    rapidjson::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {
    rapidjson::Value node(rapidjson::kObjectType);

    node.AddMember("kind", rapidjson::Value().SetString("constant"), *allocator);
    // FIXME: should set real type
    node.AddMember("type", rapidjson::Value().SetString("int"), *allocator);
    rapidjson::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitCharacterLiteral(CharacterLiteral *Node) {
    rapidjson::Value node(rapidjson::kObjectType);

    node.AddMember("kind", rapidjson::Value().SetString("constant"), *allocator);
    // FIXME: should set real type
    node.AddMember("type", rapidjson::Value().SetString("char"), *allocator);
    rapidjson::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    rapidjson::Value node(rapidjson::kObjectType);

    node.AddMember("kind", rapidjson::Value().SetString("variable"), *allocator);
    // FIXME: should set real type
    node.AddMember("type", rapidjson::Value().SetString("int"), *allocator);
    rapidjson::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    rapidjson::Value node(rapidjson::kObjectType);

    node.AddMember("kind", rapidjson::Value().SetString("constant"), *allocator);
    // FIXME: should set real type
    node.AddMember("type", rapidjson::Value().SetString("int"), *allocator);
    rapidjson::Value repr;
    repr.SetString(toString(Node).c_str(), *allocator);
    node.AddMember("repr", repr, *allocator);

    path.push(std::move(node));
  }

};


rapidjson::Value stmtToJSON(const clang::Stmt *stmt,
                            rapidjson::Document::AllocatorType &allocator) {
    PrintingPolicy policy = PrintingPolicy(LangOptions());
    StmtToJSON T(policy, &allocator);
    T.Visit(const_cast<Stmt*>(stmt));
    return T.getValue();
}


rapidjson::Value locToJSON(uint fileId, uint locId, uint bl, uint bc, uint el, uint ec,
                           rapidjson::Document::AllocatorType &allocator) {
  rapidjson::Value entry(rapidjson::kObjectType);
  entry.AddMember("fileId", rapidjson::Value().SetInt(fileId), allocator);
  entry.AddMember("locId", rapidjson::Value().SetInt(locId), allocator);
  entry.AddMember("beginLine", rapidjson::Value().SetInt(bl), allocator);
  entry.AddMember("beginColumn", rapidjson::Value().SetInt(bc), allocator);
  entry.AddMember("endLine", rapidjson::Value().SetInt(el), allocator);
  entry.AddMember("endColumn", rapidjson::Value().SetInt(ec), allocator);
  return entry;
}


class CollectComponents : public StmtVisitor<CollectComponents> {
  rapidjson::Document::AllocatorType *allocator;
  vector<rapidjson::Value> collected;

public:
  CollectComponents(rapidjson::Document::AllocatorType *allocator):
    allocator(allocator) {}

  vector<rapidjson::Value> getCollected() {
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

  void VisitParenExpr(ParenExpr *Node) {
    Visit(Node->getSubExpr());
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {}

  void VisitCharacterLiteral(CharacterLiteral *Node) {}

  void VisitMemberExpr(MemberExpr *Node) {
    collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    collected.push_back(stmtToJSON(Node, *allocator));
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    collected.push_back(stmtToJSON(Node, *allocator));
  }

};


vector<rapidjson::Value> collectFromExpression(const Stmt *stmt,
                                                      rapidjson::Document::AllocatorType &allocator) {
  CollectComponents T(&allocator);
  T.Visit(const_cast<Stmt*>(stmt));
  return T.getCollected();
}


//TODO: these functions are taken from Angelix and need to be refactored
bool suitableVarDecl(VarDecl *vd) {
  return (vd->getType().getTypePtr()->isIntegerType() ||
          vd->getType().getTypePtr()->isCharType());
  // vd->getType().getTypePtr()->isPointerType();
}


rapidjson::Value varDeclToJSON(VarDecl *vd, rapidjson::Document::AllocatorType &allocator) {
  rapidjson::Value node(rapidjson::kObjectType);
  node.AddMember("kind", rapidjson::Value().SetString("variable"), allocator);
  // FIXME: should set real type
  node.AddMember("type", rapidjson::Value().SetString("int"), allocator);
  rapidjson::Value repr;
  string name = vd->getName();
  repr.SetString(name.c_str(), allocator);
  node.AddMember("repr", repr, allocator);
  return node;
}


vector<rapidjson::Value> collectVisible(const ast_type_traits::DynTypedNode &node,
                                     uint line,
                                     ASTContext* context,
                                     rapidjson::Document::AllocatorType &allocator) {
  vector<rapidjson::Value> result;

  const FunctionDecl* fd;
  if ((fd = node.get<FunctionDecl>()) != NULL) {

    // adding function parameters
    for (auto it = fd->param_begin(); it != fd->param_end(); ++it) {
      auto vd = cast<VarDecl>(*it);
      if (suitableVarDecl(vd)) {
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
              if (line > beginLine && suitableVarDecl(vd)) {
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
          if (line > beginLine &&
              BinaryOperator::getOpcodeStr(op->getOpcode()).lower() == "=" &&
              isa<DeclRefExpr>(op->getLHS())) {
            DeclRefExpr* dref = cast<DeclRefExpr>(op->getLHS());
            VarDecl* vd;
            if ((vd = cast<VarDecl>(dref->getDecl())) != NULL && suitableVarDecl(vd)) {
              result.push_back(varDeclToJSON(vd, allocator));
            }
          }
        }

        if (isa<DeclStmt>(*it)) {
          DeclStmt* dstmt = cast<DeclStmt>(*it);
          SourceRange expandedLoc = getExpandedLoc(dstmt, context->getSourceManager());
          uint beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
          if (dstmt->isSingleDecl()) {
            Decl* d = dstmt->getSingleDecl();
            if (isa<VarDecl>(d)) {
              VarDecl* vd = cast<VarDecl>(d);
              if (line > beginLine && vd->hasInit() && suitableVarDecl(vd)) {
                result.push_back(varDeclToJSON(vd, allocator));
              }
            }
          }
        }

        Stmt* stmt = cast<Stmt>(*it);
        SourceRange expandedLoc = getExpandedLoc(stmt, context->getSourceManager());
        unsigned beginLine = context->getSourceManager().getExpansionLineNumber(expandedLoc.getBegin());
        if (line > beginLine) {
          vector<rapidjson::Value> fromExpr = collectFromExpression(*it, allocator);
          for (auto &c : fromExpr) {
            result.push_back(std::move(c));
          }
          
          //TODO: should be generalized for other cases:
          if (isa<IfStmt>(*it)) {
            IfStmt* ifStmt = cast<IfStmt>(*it);
            Stmt* thenStmt = ifStmt->getThen();
            if (isa<CallExpr>(*thenStmt)) {
              CallExpr* callExpr = cast<CallExpr>(thenStmt);
              for (auto a = callExpr->arg_begin(); a != callExpr->arg_end(); ++a) {
                auto e = cast<Expr>(*a);
                vector<rapidjson::Value> fromParamExpr = collectFromExpression(e, allocator);
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
      vector<rapidjson::Value> parentResult = collectVisible(parent, line, context, allocator);
      for (auto &c : parentResult) {
        result.push_back(std::move(c));
      }
    }
  }
  
  return result;
}


vector<rapidjson::Value> collectComponents(const Stmt *stmt,
                                        uint line,
                                        ASTContext *context,
                                        rapidjson::Document::AllocatorType &allocator) {
  vector<rapidjson::Value> fromExpr = collectFromExpression(stmt, allocator);

  const ast_type_traits::DynTypedNode node = ast_type_traits::DynTypedNode::create(*stmt);
  vector<rapidjson::Value> visible = collectVisible(node, line, context, allocator);

  vector<rapidjson::Value> result;

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


string makeArgumentList(const vector<rapidjson::Value> &components) {
  std::ostringstream result;

  result << "(int[]){";
  bool first = true;
  for (auto &c : components) {
    if (first) {
      first = false;
    } else {
      result << ", ";
    }
    result << c["repr"].GetString();
  }
  result << "}";

  return result.str();
}
