#include <iostream>
#include <sstream>
#include "Transformer.h"
#include "TransformationUtil.h"
#include "SearchSpaceMatchers.h"


void InstrumentRepairableAction::EndSourceFileAction() {
  FileID ID = TheRewriter.getSourceMgr().getMainFileID();
  if (INPLACE_MODIFICATION) {
    overwriteMainChangedFile(TheRewriter);
    // I am not sure what the difference is, but this case requires explicit check:
    //TheRewriter.overwriteChangedFiles();
  } else {
      TheRewriter.getEditBuffer(ID).write(llvm::outs());
  }
}

std::unique_ptr<ASTConsumer> InstrumentRepairableAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
    TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
    return llvm::make_unique<InstrumentationASTConsumer>(TheRewriter);
}


// void ApplyPatchAction::EndSourceFileAction() {
//   FileID ID = TheRewriter.getSourceMgr().getMainFileID();
//   if (INPLACE_MODIFICATION) {
//     overwriteMainChangedFile(TheRewriter);
//     // I am not sure what the difference is, but this case requires explicit check:
//     //TheRewriter.overwriteChangedFiles();
//   } else {
//       TheRewriter.getEditBuffer(ID).write(llvm::outs());
//   }
// }

// std::unique_ptr<ASTConsumer> ApplyPatchAction::CreateASTConsumer(CompilerInstance &CI, StringRef file) {
//     TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
//     return llvm::make_unique<ApplicationASTConsumer>(TheRewriter);
// }


InstrumentationASTConsumer::InstrumentationASTConsumer(Rewriter &R) : ExpressionHandler(R), StatementHandler(R) {
  Matcher.addMatcher(RepairableExpression, &ExpressionHandler);    
  Matcher.addMatcher(RepairableStatement, &StatementHandler);
}

void InstrumentationASTConsumer::HandleTranslationUnit(ASTContext &Context) {
  Matcher.matchAST(Context);
}


// ApplicationASTConsumer::ApplicationASTConsumer(Rewriter &R) : ExpressionHandler(R), StatementHandler(R) {
//   Matcher.addMatcher(RepairableExpression, &ExpressionHandler);    
//   Matcher.addMatcher(RepairableStatement, &StatementHandler);
// }

// void ApplicationASTConsumer::HandleTranslationUnit(ASTContext &Context) {
//   Matcher.matchAST(Context);
// }


InstrumentationStatementHandler::InstrumentationStatementHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void InstrumentationStatementHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Stmt *stmt = Result.Nodes.getNodeAs<clang::Stmt>(BOUND)) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(stmt, srcMgr, langOpts))
      return;

    if(!isTopLevelStatement(stmt, Result.Context))
      return;
      
    SourceRange expandedLoc = getExpandedLoc(stmt, srcMgr);

    unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
    unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
    unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
    unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

    std::cout << beginLine << " "
              << beginColumn << " "
              << endLine << " "
              << endColumn << "\n"
              << toString(stmt) << "\n";

    // FIXME: this instrumentation is incorrect for cases
    // if (condition)
    //   statement;
    // because it can create dangling else brach.
    // wrapping it with {} will not work because break/continue are matched without semicolon

    std::ostringstream stringStream;
    stringStream << "if ("
                 << "f1x("
                 << 1 << ", "
                 << beginLine << ", "
                 << beginColumn << ", "
                 << endLine << ", "
                 << endColumn
                 << ")"
                 << ") "
                 << toString(stmt);
    std::string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc, replacement);
  }
}


InstrumentationExpressionHandler::InstrumentationExpressionHandler(Rewriter &Rewrite) : Rewrite(Rewrite) {}

void InstrumentationExpressionHandler::run(const MatchFinder::MatchResult &Result) {
  if (const Expr *expr = Result.Nodes.getNodeAs<clang::Expr>("repairable")) {
    SourceManager &srcMgr = Rewrite.getSourceMgr();
    const LangOptions &langOpts = Rewrite.getLangOpts();

    if (insideMacro(expr, srcMgr, langOpts))
      return;

    SourceRange expandedLoc = getExpandedLoc(expr, srcMgr);

    unsigned beginLine = srcMgr.getExpansionLineNumber(expandedLoc.getBegin());
    unsigned beginColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getBegin());
    unsigned endLine = srcMgr.getExpansionLineNumber(expandedLoc.getEnd());
    unsigned endColumn = srcMgr.getExpansionColumnNumber(expandedLoc.getEnd());

    std::cout << beginLine << " "
              << beginColumn << " "
              << endLine << " "
              << endColumn << "\n"
              << toString(expr) << "\n";

    std::ostringstream stringStream;
    stringStream << "f1x("
                 << toString(expr) << ", "
                 << beginLine << ", "
                 << beginColumn << ", "
                 << endLine << ", "
                 << endColumn
                 << ")";
    std::string replacement = stringStream.str();

    Rewrite.ReplaceText(expandedLoc, replacement);
  }
}
