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

#pragma once

#include "clang/AST/AST.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <rapidjson/document.h>


const bool INPLACE_MODIFICATION = true;

// FIXME: better to pass these variable to constructors, but it requires a lot of boilerplate
extern uint globalFileId;
extern uint globalFromLine;
extern uint globalToLine;
extern std::string globalProfileFile;
extern std::string globalOutputFile;
extern uint globalBeginLine;
extern uint globalBeginColumn;
extern uint globalEndLine;
extern uint globalEndColumn;
extern std::string globalPatch;
extern uint globalBaseAppId;

uint getDeclExpandedLine(const clang::Decl *decl, clang::SourceManager &srcMgr);

bool insideMacro(const clang::Stmt *expr, clang::SourceManager &srcMgr, const clang::LangOptions &langOpts);

clang::SourceRange getExpandedLoc(const clang::Stmt *expr, clang::SourceManager &srcMgr);

std::string toString(const clang::Stmt *stmt);

bool overwriteMainChangedFile(clang::Rewriter &TheRewriter);

/*
  The purpose of this function is to determine if the statement is a child of 
  - compound statment
  - if/while/for statement
  - label statement (inside switch)
  it should not be, for example, the increment of for loop
 */
bool isTopLevelStatement(const clang::Stmt *stmt, clang::ASTContext *context);
bool shouldAddBrackets(const clang::Stmt *stmt, clang::ASTContext *context);

rapidjson::Value stmtToJSON(const clang::Stmt *stmt,
                            rapidjson::Document::AllocatorType &allocator);


rapidjson::Value locToJSON(uint fileId, uint bl, uint bc, uint el, uint ec,
                           rapidjson::Document::AllocatorType &allocator);


std::vector<rapidjson::Value> collectComponents(const clang::Stmt *stmt,
                                                uint line,
                                                clang::ASTContext *context,
                                                rapidjson::Document::AllocatorType &allocator);


std::string makeArgumentList(std::vector<rapidjson::Value> &components);

uint f1xapp(uint baseId, uint fileId);
bool inRange(uint line);
