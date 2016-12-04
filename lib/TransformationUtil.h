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

#pragma once


#include "clang/AST/AST.h"
#include "clang/Rewrite/Core/Rewriter.h"


using namespace clang;
using namespace llvm;
using std::string;
using std::pair;


const bool INPLACE_MODIFICATION = true;


unsigned getDeclExpandedLine(const Decl* decl, SourceManager &srcMgr);

bool insideMacro(const Stmt* expr, SourceManager &srcMgr, const LangOptions &langOpts);

SourceRange getExpandedLoc(const Stmt* expr, SourceManager &srcMgr);

string toString(const Stmt* stmt);

bool overwriteMainChangedFile(Rewriter &TheRewriter);

/*
  The purpose of this function is to determine if the statement is a child of 
  - compound statment
  - if/while/for statement
  - label statement (inside switch)
  it should not be, for example, the increment of for loop
 */
bool isTopLevelStatement(const Stmt* stmt, ASTContext* context);

