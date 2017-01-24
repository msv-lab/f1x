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

#include <sstream>
#include <fstream>
#include <iostream>

#include "F1XConfig.h"
#include "TransformationUtil.h"
#include "SearchSpaceMatchers.h"
#include "ProfileTransformer.h"

using namespace clang;
using namespace ast_matchers;

static bool alreadyTransformed = false;

bool ProfileAction::BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
  //NOTE: this is a wierd problem: sometimes this action is called two times that causes crash
  if (alreadyTransformed) {
    return false;
  }
  alreadyTransformed = true;
  return true;
}

void ProfileAction::EndSourceFileAction() {
  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (INPLACE_MODIFICATION) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }
}

std::unique_ptr<ASTConsumer> ProfileAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<ProfileASTConsumer>(TheRewriter);
}


ProfileASTConsumer::ProfileASTConsumer(Rewriter &R) : ExpressionHandler(R), StatementHandler(R) {
  Matcher.addMatcher(RepairableExpression, &ExpressionHandler);    
  Matcher.addMatcher(RepairableStatement, &StatementHandler);
}

void ProfileASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  Matcher.matchAST(Context);
}


ProfileStatementHandler::ProfileStatementHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ProfileStatementHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();
      
      const LangOptions &langOpts = Rewrite.getLangOpts();
      if (insideMacro(stmt, srcMgr, langOpts))
        return;

      if(!isTopLevelStatement(stmt, Result.Context))
        return;

      SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

      uint beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      uint beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      uint endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      uint endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());
      
      if (!inRange(beginLine))
        return;

      // NOTE: to avoid extracting locations from headers:
      std::pair<FileID, unsigned> decLoc = srcMgr.getDecomposedExpansionLoc(expandedLoc.getBegin());
      if (srcMgr.getMainFileID() != decLoc.first)
        return;

      std::ostringstream replacement;
      replacement << "({ __f1x_trace(" << globalFileId << ", "
                                       << beginLine << ", "
                                       << beginColumn << ", "
                                       << endLine << ", "
                                       << endColumn << "); "
                  << toString(stmt) << "; })";

      Rewrite.ReplaceText(expandedLoc, replacement.str());
  }
}


ProfileExpressionHandler::ProfileExpressionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ProfileExpressionHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>(BOUND)) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(expr, srcMgr, langOpts))
      return;

    SourceRange expandedLoc = getExpandedLoc(expr, srcMgr);

    uint beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
    uint beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
    uint endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
    uint endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

    if (!inRange(beginLine))
      return;

    // NOTE: to avoid extracting locations from headers:
    std::pair<FileID, unsigned> decLoc = srcMgr.getDecomposedExpansionLoc(expandedLoc.getBegin());
    if (srcMgr.getMainFileID() != decLoc.first)
      return;

    std::ostringstream stringStream;
    stringStream << "({ __f1x_trace(" << globalFileId << ", " 
                                      << beginLine << ", "
                                      << beginColumn << ", " 
                                      << endLine << ", "
                                      << endColumn << "); "
                 << toString(expr) << "; })";
    
    Rewrite.ReplaceText(expandedLoc, stringStream.str());
  }
}

