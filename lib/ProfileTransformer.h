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
#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang;
using namespace ast_matchers;


class ProfileStatementHandler : public MatchFinder::MatchCallback {
public:
  ProfileStatementHandler(Rewriter &Rewrite);

  virtual void run(const MatchFinder::MatchResult &Result);

private:
  Rewriter &Rewrite;
};


class ProfileExpressionHandler : public MatchFinder::MatchCallback {
public:
  ProfileExpressionHandler(Rewriter &Rewrite);

  virtual void run(const MatchFinder::MatchResult &Result);

private:
  Rewriter &Rewrite;
};


class ProfileASTConsumer : public ASTConsumer {
public:
  ProfileASTConsumer(Rewriter &R);

  void HandleTranslationUnit(ASTContext &Context) override;

private:
  ProfileExpressionHandler ExpressionHandler;
  ProfileStatementHandler StatementHandler;
  MatchFinder Matcher;
};


class ProfileAction : public ASTFrontendAction {
public:
  ProfileAction(){}

  bool BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) override;
  void EndSourceFileAction() override;

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override;

private:
  Rewriter TheRewriter;
};
