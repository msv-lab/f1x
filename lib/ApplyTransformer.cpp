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
#include <iostream>
#include "TransformationUtil.h"
#include "SearchSpaceMatchers.h"
#include "ApplyTransformer.h"
#include "F1XConfig.h"

using namespace clang;
using namespace ast_matchers;

static bool alreadyTransformed = false;

void ApplyPatchAction::EndSourceFileAction() {
  //NOTE: this is a wierd problem: sometimes this action is called two times that causes crash
  if (alreadyTransformed) {
    return;
  }
  alreadyTransformed = true;

  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (INPLACE_MODIFICATION) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }
}

std::unique_ptr<ASTConsumer> ApplyPatchAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<ApplicationASTConsumer>(TheRewriter);
}


ApplicationASTConsumer::ApplicationASTConsumer(Rewriter &R) : ExpressionHandler(R), StatementHandler(R) {
  Matcher.addMatcher(RepairableExpression, &ExpressionHandler);    
  Matcher.addMatcher(RepairableStatement, &StatementHandler);
}

void ApplicationASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  Matcher.matchAST(Context);
}


ApplicationStatementHandler::ApplicationStatementHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ApplicationStatementHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();

      SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

      uint beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      uint beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      uint endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      uint endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

      if (beginLine == globalBeginLine &&
          beginColumn == globalBeginColumn &&
          endLine == globalEndLine &&
          endColumn == globalEndColumn) {

        std::stringstream replacement;
        
        unsigned origLength = Rewrite.getRangeSize(expandedLoc);
        bool addBrackets = shouldAddBrackets(stmt, Result.Context);
        if(addBrackets)
    	    replacement << "{ ";
    	    
        replacement << "if (" << globalPatch << ") " << toString(stmt);

        llvm::errs() << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"
          << "<   " << toString(stmt) << "\n"
          << ">   " << replacement.str() << "\n";
        
        if(addBrackets)
        {
        	replacement << "; }";
        	const char *followingData = srcMgr.getCharacterData(expandedLoc.getBegin());
        	int followingDataSize = strlen(followingData);
          for(int i=origLength; i<followingDataSize; i++)
          {
            origLength++;
            char curChar = *(followingData+i);
            if(curChar == ';')
              break;
            else if(curChar == ' ' || curChar == '\t' || curChar == '\n')
                    continue;
            else return;
          }
        }

        Rewrite.ReplaceText(expandedLoc.getBegin(), origLength, replacement.str());
      }
  }
}


ApplicationExpressionHandler::ApplicationExpressionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ApplicationExpressionHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>(BOUND)) {
      SourceManager &srcMgr = Rewrite.getSourceMgr();

      SourceRange expandedLoc = getExpandedLoc(expr, srcMgr);

      uint beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
      uint beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
      uint endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
      uint endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

      // FIXME: do I need to cast it to the original type (because this is the type if runtime function)
      if (beginLine == globalBeginLine &&
          beginColumn == globalBeginColumn &&
          endLine == globalEndLine &&
          endColumn == globalEndColumn) {
        llvm::errs() << beginLine << " " << beginColumn << " " << endLine << " " << endColumn << "\n"
          << "<   " << toString(expr) << "\n"
          << ">   " << globalPatch << "\n";
        
        Rewrite.ReplaceText(expandedLoc, globalPatch);
      }
  }
}

