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

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "F1XConfig.h"
#include "TransformationUtil.h"
#include "SearchSpaceMatchers.h"
#include "ProfileTransformer.h"

using namespace clang;
using namespace ast_matchers;

namespace json = rapidjson;
json::Document profileLocations;

std::ostringstream ProfileInstr_CPP;
std::ostringstream ProfileInstr_H;

bool ProfileAction::BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
  profileLocations.SetArray();
  
  ProfileInstr_CPP << globalOutputFile << ".cpp";
  ProfileInstr_H << globalOutputFile << ".h";
  std::ofstream out_CPP(ProfileInstr_CPP.str(), std::ios::app);
  std::ofstream out_H(ProfileInstr_H.str(), std::ios::app);

  // source
  out_CPP << "#include<stdio.h>\n"
          << "#include \"" << ProfileInstr_H.str() << "\"\n\n";
  out_CPP << "void f1x_RecordStmt(int beginLine, int beginColumn, int endLine, int endColumn, int globalFileId){\n"
          << "  FILE * file = fopen(\""<< globalOutputFile << "\", \"a+\");\n"
          << "  fprintf(file, \"%d_%d_%d_%d_%d\\n\", beginLine, beginColumn, endLine, endColumn, globalFileId);\n"
          << "  fclose(file);\n"
          << "}\n\n";
  
  // header
  out_H << "#ifdef __cplusplus" << "\n"
        << "extern \"C\" {" << "\n"
        << "#endif" << "\n";
  out_H << "void f1x_RecordStmt(int beginLine, int beginColumn, int endLine, int endColumn, int globalFileId);\n";
  
  return true;
}

void ProfileAction::EndSourceFileAction() {
  
  std::ofstream out_H(ProfileInstr_H.str(), std::ios::app);
  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (INPLACE_MODIFICATION) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }
  out_H << "#ifdef __cplusplus" << "\n"
        << "}" << "\n"
        << "#endif" << "\n";
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
      replacement << "({f1x_RecordStmt(" << beginLine << ", " << beginColumn << ", " 
                  << endLine << ", " << endColumn << ", " << globalFileId <<");"
                  << toString(stmt) << ";})";

      Rewrite.ReplaceText(expandedLoc, replacement.str());
  }
}


ProfileExpressionHandler::ProfileExpressionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void ProfileExpressionHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>("repairable")) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(expr, srcMgr, langOpts))
      return;

    uint locId = f1xloc(globalBaseLocId, globalFileId);
    globalBaseLocId++;
    
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

    std::ofstream out_CPP(ProfileInstr_CPP.str(), std::ios::app);
    std::ofstream out_H(ProfileInstr_H.str(), std::ios::app);
    
    json::Value exprJSON = stmtToJSON(expr, profileLocations.GetAllocator());
    const char* type = exprJSON["type"].GetString();
    
    std::ostringstream stringStream;
    stringStream << "({f1x_RecordStmt(" << beginLine << ", " << beginColumn << ", " 
                  << endLine << ", " << endColumn << ", " << globalFileId <<");"
                  << toString(expr) << ";})";
    
    /*std::ostringstream functionName;
    functionName << "__f1x_" <<  beginLine << "_" << beginColumn << "_" << endLine << "_" << endColumn 
                 << "_" << "_" << globalBaseLocId;
                 
    //source
    out_CPP << type << " " << functionName.str() 
        << "( int beginLine, int  beginColumn, int endLine, int endColumn, int globalFileId, " 
        << type <<" value" << "){\n"
        <<"\tf1x_RecordStmt( beginLine, beginColumn, endLine, endColumn, globalFileId);\n"
        <<"\treturn value;" << "\n}\n";
    //head
    out_H << type << " " << functionName.str() 
          << "( int beginLine, int  beginColumn, int endLine, int endColumn, int globalFileId, " 
          << type <<" value" << ");\n";
        
    std::ostringstream stringStream;
    stringStream << functionName.str() << "(" << beginLine << ", " << beginColumn
                 << ", " << endLine << ", " << endColumn << ", " << globalFileId
                 << ", " << toString(expr) << ")";*/
                 

    Rewrite.ReplaceText(expandedLoc, stringStream.str());
  }
}

