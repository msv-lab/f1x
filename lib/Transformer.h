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


class InstrumentationStatementHandler : public MatchFinder::MatchCallback {
public:
  InstrumentationStatementHandler(Rewriter &Rewrite);

  virtual void run(const MatchFinder::MatchResult &Result);

private:
  Rewriter &Rewrite;
};


class InstrumentationExpressionHandler : public MatchFinder::MatchCallback {
public:
  InstrumentationExpressionHandler(Rewriter &Rewrite);

  virtual void run(const MatchFinder::MatchResult &Result);

private:
  Rewriter &Rewrite;
};


class InstrumentationASTConsumer : public ASTConsumer {
public:
  InstrumentationASTConsumer(Rewriter &R);

  void HandleTranslationUnit(ASTContext &Context) override;

private:
  InstrumentationExpressionHandler ExpressionHandler;
  InstrumentationStatementHandler StatementHandler;
  MatchFinder Matcher;
};


// class ApplicationASTConsumer : public ASTConsumer {
// public:
//   ApplicationASTConsumer(Rewriter &R);

//   void HandleTranslationUnit(ASTContext &Context) override;

// private:
//   ApplicationExpressionHandler ExpressionHandler;
//   ApplicationStatementHandler StatementHandler;
//   MatchFinder Matcher;
// };


class InstrumentRepairableAction : public ASTFrontendAction {
public:
  InstrumentRepairableAction() {}

  void EndSourceFileAction() override;

  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override;

private:
  Rewriter TheRewriter;
};


// class ApplyPatchAction : public ASTFrontendAction {
// public:
//   ApplyPatchAction() {}

//   void EndSourceFileAction() override;

//   std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override;

// private:
//   Rewriter TheRewriter;
// };
