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
  it should not be the, for example, the increment of for loop
 */
bool isTopLevelStatement(const Stmt* stmt, SourceManager &srcMgr);

