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

#include <sstream>
#include "TransformationUtil.h"
#include "SearchSpaceMatchers.h"
#include "InstrumentTransformer.h"

using namespace clang;
using namespace ast_matchers;


void InstrumentRepairableAction::EndSourceFileAction() {
  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (INPLACE_MODIFICATION) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }
}

std::unique_ptr<ASTConsumer> InstrumentRepairableAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<InstrumentationASTConsumer>(TheRewriter);
}


InstrumentationASTConsumer::InstrumentationASTConsumer(Rewriter &R) : ExpressionHandler(R), StatementHandler(R) {
  Matcher.addMatcher(RepairableExpression, &ExpressionHandler);    
  Matcher.addMatcher(RepairableStatement, &StatementHandler);
}

void InstrumentationASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  Matcher.matchAST(Context);
}


InstrumentationStatementHandler::InstrumentationStatementHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void InstrumentationStatementHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(stmt, srcMgr, langOpts))
      return;

    if(!isTopLevelStatement(stmt, Result.Context))
      return;
      
    SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

    unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
    unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
    unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
    unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

    llvm::errs() << beginLine << " "
                 << beginColumn << " "
                 << endLine << " "
                 << endColumn << "\n"
                 << toString(stmt);

    // FIXME: this instrumentation is incorrect for cases
    // if (condition)
    //   statement;
    // because it can create dangling else brach.
    // wrapping it with {} will not work because break/continue are matched without semicolon

    std::ostringstream stringStream;
    stringStream << "if ("
                 << "__f1x_"
                 << beginLine << "_" << beginColumn << "_" << endLine << "_" << endColumn
                 << "(" << ")"
                 << ") "
                 << toString(stmt);
    std::string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc, replacement);
  }
}


InstrumentationExpressionHandler::InstrumentationExpressionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void InstrumentationExpressionHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>("repairable")) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(expr, srcMgr, langOpts))
      return;

    SourceRange expandedLoc = getExpandedLoc(expr, srcMgr);

    unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
    unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
    unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
    unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

    llvm::errs() << beginLine << " "
                 << beginColumn << " "
                 << endLine << " "
                 << endColumn << "\n"
                 << toString(expr);

    std::ostringstream stringStream;
    stringStream << "__f1x_"
                 << beginLine << "_" << beginColumn << "_" << endLine << "_" << endColumn
                 << "(" << ")";
    std::string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc, replacement);
  }
}
