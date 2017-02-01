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
extern ulong globalFileId;
extern ulong globalFromLine;
extern ulong globalToLine;
extern std::string globalProfileFile;
extern std::string globalOutputFile;
extern ulong globalBeginLine;
extern ulong globalBeginColumn;
extern ulong globalEndLine;
extern ulong globalEndColumn;
extern std::string globalPatch;
extern ulong globalBaseAppId;

ulong getDeclExpandedLine(const clang::Decl *decl, clang::SourceManager &srcMgr);

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

bool isChildOfNonblock(const clang::Stmt *stmt, clang::ASTContext *context);

bool inConditionContext(const clang::Stmt *stmt, clang::ASTContext *context);

rapidjson::Value stmtToJSON(const clang::Stmt *stmt,
                            rapidjson::Document::AllocatorType &allocator);


rapidjson::Value locToJSON(ulong fileId, ulong bl, ulong bc, ulong el, ulong ec,
                           rapidjson::Document::AllocatorType &allocator);


std::vector<rapidjson::Value> collectComponents(const clang::Stmt *stmt,
                                                ulong line,
                                                clang::ASTContext *context,
                                                rapidjson::Document::AllocatorType &allocator);


std::string makeArgumentList(std::vector<rapidjson::Value> &components);

ulong f1xapp(ulong baseId, ulong fileId);
bool inRange(ulong line);
