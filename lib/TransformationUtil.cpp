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

#include "TransformationUtil.h"
#include "clang/Lex/Preprocessor.h"
#include "llvm/Support/raw_ostream.h"

#include "Config.h"

using namespace clang;
using namespace llvm;
using std::string;
using std::pair;


uint globalFileId;
uint globalFromLine;
uint globalToLine;
std::string globalOutputFile;
uint globalBeginLine;
uint globalBeginColumn;
uint globalEndLine;
uint globalEndColumn;
std::string globalPatch;
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
  rapidjson::Document document;
  clang::PrinterHelper* Helper;
  PrintingPolicy Policy;

public:
  StmtToSMTLIB2(PrinterHelper* helper, const PrintingPolicy &Policy):
    Helper(helper), Policy(Policy) {}

  void PrintExpr(Expr *E) {
    if (E)
      Visit(E);
    else
      OS << "<null expr>";
  }

  void Visit(Stmt* S) {
    if (Helper && Helper->handledStmt(S, OS))
      return;
    else StmtVisitor<StmtToSMTLIB2>::Visit(S);
  }

  void VisitBinaryOperator(BinaryOperator *Node) {
    std::string opcode_str = BinaryOperator::getOpcodeStr(Node->getOpcode()).lower();
    // if (opcode_str == "!=") {
    //   OS << "(not (= ";
    //   PrintExpr(Node->getLHS());
    //   OS << " ";
    //   PrintExpr(Node->getRHS());
    //   OS << "))";
    //   return;
    // }

    replaceStr(opcode_str, "==", "=");
    replaceStr(opcode_str, "!=", "neq");
    replaceStr(opcode_str, "||", "or");
    replaceStr(opcode_str, "&&", "and");

    OS << "(" << opcode_str << " ";
    PrintExpr(Node->getLHS());
    OS << " ";
    PrintExpr(Node->getRHS());
    OS << ")";
  }

  void VisitUnaryOperator(UnaryOperator *Node) {
    std::string op = UnaryOperator::getOpcodeStr(Node->getOpcode());
    if (op == "!") {
      op = "not";
    }

    OS << "(" << op << " ";
    PrintExpr(Node->getSubExpr());
    OS << ")";
  }

  void VisitImplicitCastExpr(ImplicitCastExpr *Node) {
    PrintExpr(Node->getSubExpr());
  }

  void VisitCastExpr(CastExpr *Node) {
    PrintExpr(Node->getSubExpr()); // TODO: this may not always work
  }

  void VisitParenExpr(ParenExpr *Node) {
    PrintExpr(Node->getSubExpr());
  }

  void VisitMemberExpr(MemberExpr *Node) {
    // this is copied from somewhere
    PrintExpr(Node->getBase());

    MemberExpr *ParentMember = dyn_cast<MemberExpr>(Node->getBase());
    FieldDecl  *ParentDecl   = ParentMember
      ? dyn_cast<FieldDecl>(ParentMember->getMemberDecl()) : nullptr;

    if (!ParentDecl || !ParentDecl->isAnonymousStructOrUnion())
      OS << (Node->isArrow() ? "->" : ".");

    if (FieldDecl *FD = dyn_cast<FieldDecl>(Node->getMemberDecl()))
      if (FD->isAnonymousStructOrUnion())
        return;

    if (NestedNameSpecifier *Qualifier = Node->getQualifier())
      Qualifier->print(OS, Policy);
    if (Node->hasTemplateKeyword())
      OS << "template ";
    OS << Node->getMemberNameInfo();
    if (Node->hasExplicitTemplateArgs())
      TemplateSpecializationType::PrintTemplateArgumentList(OS, Node->getTemplateArgs(), Node->getNumTemplateArgs(), Policy);
  }

  void VisitIntegerLiteral(IntegerLiteral *Node) {
    bool isSigned = Node->getType()->isSignedIntegerType();
    OS << Node->getValue().toString(10, isSigned);
  }

  void VisitCharacterLiteral(CharacterLiteral *Node) {
    unsigned value = Node->getValue();

    //TODO: it most likely does not produce valid SMTLIB2:
    switch (value) {
    case '\\':
      OS << "'\\\\'";
      break;
    case '\'':
      OS << "'\\''";
      break;
    case '\a':
      // TODO: K&R: the meaning of '\\a' is different in traditional C
      OS << "'\\a'";
      break;
    case '\b':
      OS << "'\\b'";
      break;
      // Nonstandard escape sequence.
      /*case '\e':
        OS << "'\\e'";
        break;*/
    case '\f':
      OS << "'\\f'";
      break;
    case '\n':
      OS << "'\\n'";
      break;
    case '\r':
      OS << "'\\r'";
      break;
    case '\t':
      OS << "'\\t'";
      break;
    case '\v':
      OS << "'\\v'";
      break;
    default:
      if (value < 256 && isPrintable((unsigned char)value))
        OS << value;
      else if (value < 256)
        OS << "'\\x" << llvm::format("%02x", value) << "'";
      else if (value <= 0xFFFF)
        OS << "'\\u" << llvm::format("%04x", value) << "'";
      else
        OS << "'\\U" << llvm::format("%08x", value) << "'";
    }
  }

  void VisitDeclRefExpr(DeclRefExpr *Node) {
    ValueDecl* decl = Node->getDecl();
    if (isa<EnumConstantDecl>(decl)) {
      EnumConstantDecl* ecdecl = cast<EnumConstantDecl>(decl);
      if (ecdecl->getInitVal() < 0) {
        OS << "(- ";
        OS << (-(ecdecl->getInitVal())).toString(10);
        OS << ")";
      } else {
        OS << ecdecl->getInitVal().toString(10);
      }
    } else {
      OS << Node->getNameInfo();
    }
  }

  void VisitArraySubscriptExpr(ArraySubscriptExpr *Node) {
    PrintExpr(Node->getLHS());
    OS << "_LBRSQR_";
    PrintExpr(Node->getRHS());
    OS << "_RBRSQR_";
  }

};


rapidjson::Document stmtToJSON(const clang::Stmt* stmt) {
    PrintingPolicy pp = PrintingPolicy(LangOptions());
    StmtToJSON T(nullptr, pp);
    T.Visit(const_cast<Stmt*>(stmt));
    return T.getJSON();
}
