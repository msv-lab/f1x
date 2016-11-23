#pragma once


#include "clang/AST/AST.h"
#include "clang/Rewrite/Core/Rewriter.h"


using namespace clang;
using namespace llvm;
using std::string;
using std::pair;


#define INPLACE_MODIFICATION 1


unsigned getDeclExpandedLine(const Decl* decl, SourceManager &srcMgr);

bool insideMacro(const Stmt* expr, SourceManager &srcMgr, const LangOptions &langOpts);

SourceRange getExpandedLoc(const Stmt* expr, SourceManager &srcMgr);

string toString(const Stmt* stmt);

bool overwriteMainChangedFile(Rewriter &TheRewriter);
