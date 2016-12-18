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
#include <fstream>
#include <iostream>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include "Config.h"
#include "F1XID.h"
#include "TransformationUtil.h"
#include "SearchSpaceMatchers.h"
#include "InstrumentTransformer.h"

using namespace clang;
using namespace ast_matchers;

namespace json = rapidjson;

using std::vector;
using std::string;

json::Document candidateLocations;


bool inRange(uint line) {
  if (globalFromLine || globalToLine) {
    return globalFromLine <= line && line <= globalToLine;
  } else {
    return true;
  }
}


bool InstrumentRepairableAction::BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
  candidateLocations.SetArray();
  return true;
}

void InstrumentRepairableAction::EndSourceFileAction() {
  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (INPLACE_MODIFICATION) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }

  std::ofstream ofs(globalOutputFile);
  json::OStreamWrapper osw(ofs);
  json::Writer<json::OStreamWrapper> writer(osw);
  candidateLocations.Accept(writer);
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
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {//Bound = repairable
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(stmt, srcMgr, langOpts))
      return;

    if(!isTopLevelStatement(stmt, Result.Context))
      return;

    uint locId = f1xloc(globalBaseLocId, globalFileId);
    globalBaseLocId++;
      
    SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

    uint beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
    uint beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
    uint endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
    uint endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

    if (!inRange(beginLine))
      return;
                 
    llvm::errs() << beginLine << " "
                 << beginColumn << " "
                 << endLine << " "
                 << endColumn << "\n"
                 << locId << "\n"
                 << toString(stmt) << "\n";

    json::Value candidateLoc(json::kObjectType);
    candidateLoc.AddMember("defect", json::Value().SetString("guard"), candidateLocations.GetAllocator());
    json::Value exprJSON(json::kObjectType);
    exprJSON.AddMember("kind", json::Value().SetString("constant"), candidateLocations.GetAllocator());
    exprJSON.AddMember("type", json::Value().SetString("int"), candidateLocations.GetAllocator());
    exprJSON.AddMember("repr", json::Value().SetString("1"), candidateLocations.GetAllocator());
    candidateLoc.AddMember("expression", exprJSON, candidateLocations.GetAllocator());
    json::Value locJSON = locToJSON(globalFileId, locId, beginLine, beginColumn, endLine, endColumn, candidateLocations.GetAllocator());
    candidateLoc.AddMember("location", locJSON, candidateLocations.GetAllocator());
    json::Value componentsJSON(json::kArrayType);    
    vector<json::Value> components = collectComponents(stmt, beginLine, Result.Context, candidateLocations.GetAllocator());
    string arguments = makeArgumentList(components);
    for (auto &component : components) {
      componentsJSON.PushBack(component, candidateLocations.GetAllocator());
    }
    candidateLoc.AddMember("components", componentsJSON, candidateLocations.GetAllocator());
    candidateLocations.PushBack(candidateLoc, candidateLocations.GetAllocator());

    // FIXME: this instrumentation is incorrect for cases
    // if (condition)
    //   statement;
    // because it can create dangling else branch.
    // wrapping it with {} will not work because break/continue are matched without semicolon

	unsigned origLength = Rewrite.getRangeSize(expandedLoc);
    std::ostringstream stringStream;
    
    bool addBrackets = shouldAddBrackets(stmt, Result.Context);
    if(addBrackets)
    	stringStream << "{\n\t";
    stringStream << "if ("
                 << "!(__f1x_loc == " << locId << "ul) || "
                 << "__f1x_" << globalFileId << "_" << beginLine << "_" << beginColumn << "_" << endLine << "_" << endColumn
                 << "(" << arguments << ")"
                 << ") "
                 << toString(stmt);
    if(addBrackets)
    {
    	stringStream << ";\n}";
    	const char *followingData = srcMgr.getCharacterData(expandedLoc.getBegin());
    	int followingDataSize = strlen(followingData);
    	origLength = 0;
		for(int i=0; i<followingDataSize; i++)
		{
			origLength++;
			char curChar = *(followingData+i);
			if(curChar == ';')
				break;
			/*else if(curChar == ' ' || curChar == '\t' || curChar == '\n')
			{
				continue;
			}*/
		}
    }
    
    string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc.getBegin(), origLength, replacement);
  }
}


InstrumentationExpressionHandler::InstrumentationExpressionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void InstrumentationExpressionHandler::run(const MatchFinder::MatchResult &Result) {
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

    llvm::errs() << beginLine << " "
                 << beginColumn << " "
                 << endLine << " "
                 << endColumn << "\n"
                 << locId << "\n"
                 << toString(expr) << "\n";

    json::Value candidateLoc(json::kObjectType);
    //FIXME: condition is more precise defect class:
    candidateLoc.AddMember("defect", json::Value().SetString("expression"), candidateLocations.GetAllocator());
    json::Value exprJSON = stmtToJSON(expr, candidateLocations.GetAllocator());
    candidateLoc.AddMember("expression", exprJSON, candidateLocations.GetAllocator());
    json::Value locJSON = locToJSON(globalFileId, locId, beginLine, beginColumn, endLine, endColumn, candidateLocations.GetAllocator());
    candidateLoc.AddMember("location", locJSON, candidateLocations.GetAllocator());
    json::Value componentsJSON(json::kArrayType);
    vector<json::Value> components = collectComponents(expr, beginLine, Result.Context, candidateLocations.GetAllocator());
    string arguments = makeArgumentList(components);
    for (auto &component : components) {
      componentsJSON.PushBack(component, candidateLocations.GetAllocator());
    }
    candidateLoc.AddMember("components", componentsJSON, candidateLocations.GetAllocator());
    candidateLocations.PushBack(candidateLoc, candidateLocations.GetAllocator());
    
    std::ostringstream stringStream;
    stringStream << "(__f1x_loc == " << locId << "ul ? "
                 << "__f1x_" << globalFileId << "_" << beginLine << "_" << beginColumn << "_" << endLine << "_" << endColumn
                 << "(" << arguments << ")"
                 << " : " << toString(expr) << ")";
    string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc, replacement);
  }
}
