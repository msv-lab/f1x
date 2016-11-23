#include "SearchSpaceMatchers.h"


StatementMatcher RepairableNode =
  anyOf(unaryOperator(hasOperatorName("!")).bind("repairable"),
        // TODO: for pointer type we only interested if they are equal to 0 or other pointers:
        // pointers of various types are used in x->y expressions
        declRefExpr(to(varDecl(anyOf(hasType(isInteger()),
                                     hasType(pointerType()))))).bind("repairable"),
        declRefExpr(to(enumConstantDecl())).bind("repairable"),
        declRefExpr(to(namedDecl())), // no binding because it is only for member expression
        arraySubscriptExpr(hasIndex(ignoringImpCasts(anyOf(integerLiteral(), declRefExpr(), memberExpr()))),
                           hasBase(implicitCastExpr(hasSourceExpression(declRefExpr(hasType(arrayType(hasElementType(isInteger())))))))).bind("repairable"),
        integerLiteral().bind("repairable"),
        characterLiteral().bind("repairable"),
        // TODO: I need to make sure that base is a variable here:
        memberExpr().bind("repairable"), 
        castExpr(hasType(asString("void *")), hasDescendant(integerLiteral(equals(0)))).bind("repairable"), // NULL
        binaryOperator(anyOf(hasOperatorName("=="),
                             hasOperatorName("!="),
                             hasOperatorName("<="),
                             hasOperatorName(">="),
                             hasOperatorName(">"),
                             hasOperatorName("<"),
                             hasOperatorName("+"),
                             hasOperatorName("-"),
                             hasOperatorName("*"),
                             hasOperatorName("/"),
                             hasOperatorName("||"),
                             hasOperatorName("&&"))).bind("repairable"));


StatementMatcher NonRepairableNode =
  unless(RepairableNode);


StatementMatcher TrivialNode =
  anyOf(declRefExpr(), integerLiteral(), characterLiteral(), memberExpr());


StatementMatcher RepairableExpression =
  allOf(RepairableNode, unless(hasDescendant(expr(ignoringParenImpCasts(NonRepairableNode)))));


StatementMatcher NonTrivialRepairableExpression =
  allOf(RepairableExpression, unless(TrivialNode));


StatementMatcher NonRepairableExpression =
  anyOf(unless(RepairableNode), hasDescendant(expr(ignoringParenImpCasts(NonRepairableNode))));


StatementMatcher Splittable =
  anyOf(binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(RepairableExpression)),
                       hasRHS(ignoringParenImpCasts(NonRepairableExpression))),
        binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(NonRepairableExpression)),
                       hasRHS(ignoringParenImpCasts(RepairableExpression))));


StatementMatcher NonTrivialSplittable =
  anyOf(binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(NonTrivialRepairableExpression)),
                       hasRHS(ignoringParenImpCasts(NonRepairableExpression))),
        binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(NonRepairableExpression)),
                       hasRHS(ignoringParenImpCasts(NonTrivialRepairableExpression))));


auto hasSplittableCondition =
  anyOf(hasCondition(ignoringParenImpCasts(RepairableExpression)),
        eachOf(hasCondition(Splittable), hasCondition(forEachDescendant(Splittable))));


auto hasNonTrivialSplittableCondition =
  anyOf(hasCondition(ignoringParenImpCasts(NonTrivialRepairableExpression))
        eachOf(hasCondition(NonTrivialSplittable),
               hasCondition(forEachDescendant(NonTrivialSplittable))));


StatementMatcher RepairableIfCondition =
  ifStmt(hasSplittableCondition);


StatementMatcher NonTrivialRepairableIfCondition =
  ifStmt(hasNonTrivialSplittableCondition);


StatementMatcher RepairableLoopCondition = 
  anyOf(whileStmt(hasSplittableCondition),
        forStmt(hasSplittableCondition));


StatementMatcher NonTrivialRepairableLoopCondition = 
  anyOf(whileStmt(hasNonTrivialSplittableCondition),
        forStmt(hasNonTrivialSplittableCondition));


auto isTopLevelStatement = 
  allOf(anyOf(hasParent(compoundStmt()),
              hasParent(ifStmt()),
              hasParent(labelStmt()),
              hasParent(whileStmt()),
              hasParent(forStmt())),
        unless(has(forStmt())),
        unless(has(whileStmt())),
        unless(has(ifStmt())));


StatementMatcher RepairableAssignment =
  binaryOperator(isTopLevelStatement,
                 anyOf(hasOperatorName("="), hasOperatorName("+="), hasOperatorName("-="), hasOperatorName("*=")),
                 anyOf(hasLHS(ignoringParenImpCasts(declRefExpr())),
                       hasLHS(ignoringParenImpCasts(memberExpr())),
                       hasLHS(ignoringParenImpCasts(arraySubscriptExpr()))),
                 hasRHS(ignoringParenImpCasts(RepairableExpression)));


StatementMatcher NonTrivialRepairableAssignment =
  binaryOperator(isTopLevelStatement,
                 anyOf(hasOperatorName("="), hasOperatorName("+="), hasOperatorName("-="), hasOperatorName("*=")),
                 anyOf(hasLHS(ignoringParenImpCasts(declRefExpr())),
                       hasLHS(ignoringParenImpCasts(memberExpr())),
                       hasLHS(ignoringParenImpCasts(arraySubscriptExpr()))),
                 hasRHS(ignoringParenImpCasts(NonTrivialRepairableExpression)));

//TODO: currently these selectors are not completely orthogonal
// for example, if RHS of assignment contains if condition like here:
// x = ({ if (...) {...}; 1; });
// it may fail


StatementMatcher InterestingRepairableCondition =
  anyOf(RepairableIfCondition,
        RepairableLoopCondition);


StatementMatcher InterestingRepairableExpression =
  anyOf(InterestingRepairableCondition,
        RepairableAssignment);


auto hasAngelixOutput =
  anyOf(hasDescendant(callExpr(callee(functionDecl(matchesName("angelix_ignore"))))), callExpr(callee(functionDecl(matchesName("angelix_ignore")))));


StatementMatcher InterestingCondition =
  anyOf(ifStmt(hasCondition(expr(unless(hasAngelixOutput)).bind("repairable"))),
        whileStmt(hasCondition(expr(unless(hasAngelixOutput)).bind("repairable"))),
        forStmt(hasCondition(expr(unless(hasAngelixOutput)).bind("repairable"))));

//TODO: do I need it?
StatementMatcher InterestingIntegerAssignment =
  binaryOperator(isTopLevelStatement,
                 anyOf(hasOperatorName("="), hasOperatorName("+="), hasOperatorName("-="), hasOperatorName("*=")),
                 anyOf(hasLHS(ignoringParenImpCasts(declRefExpr(hasType(isInteger())))),
                       hasLHS(ignoringParenImpCasts(memberExpr(hasType(isInteger()))))),
                 hasRHS(expr().bind("repairable")),
                 unless(hasAngelixOutput));


StatementMatcher InterestingAssignment =
  binaryOperator(isTopLevelStatement,
                 anyOf(hasOperatorName("="), hasOperatorName("+="), hasOperatorName("-="), hasOperatorName("*=")),
                 anyOf(hasLHS(ignoringParenImpCasts(declRefExpr())),
                       hasLHS(ignoringParenImpCasts(memberExpr())),
                       hasLHS(ignoringParenImpCasts(arraySubscriptExpr()))),
                 unless(hasAngelixOutput)).bind("repairable");


StatementMatcher InterestingCall =
  callExpr(isTopLevelStatement,
           unless(hasAngelixOutput)).bind("repairable");


StatementMatcher InterestingStatement =
  anyOf(InterestingAssignment, InterestingCall, breakStmt().bind("repairable"),continueStmt().bind("repairable"));
