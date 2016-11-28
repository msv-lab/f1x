#include "SearchSpaceMatchers.h"

#define ID "repairable"


StatementMatcher RepairableOperator = 
  anyOf(binaryOperator(anyOf(hasOperatorName("=="),
                             hasOperatorName("!="),
                             hasOperatorName("<="),
                             hasOperatorName(">="),
                             hasOperatorName(">"),
                             hasOperatorName("<"),
                             hasOperatorName("||"),
                             hasOperatorName("&&"))).bind(ID),
        binaryOperator(anyOf(hasOperatorName("+"),
                             hasOperatorName("-"),
                             hasOperatorName("*"),
                             hasOperatorName("%"),
                             hasOperatorName("/"))).bind(ID),
        unaryOperator(hasOperatorName("!")).bind(ID),
        binaryOperator(anyOf(hasOperatorName("&"),
                             hasOperatorName("|"),
                             hasOperatorName("~"),
                             hasOperatorName("<<"),
                             hasOperatorName(">>"))).bind(ID),
        unaryOperator(hasOperatorName("~")).bind(ID));

// FIXME: currently support only integer arrays:
StatementMatcher RepairableArraySubscript =
  arraySubscriptExpr(hasIndex(ignoringImpCasts(anyOf(integerLiteral(),
                                                     declRefExpr(),
                                                     memberExpr()))),
                     hasBase(implicitCastExpr(hasSourceExpression(declRefExpr(hasType(arrayType(hasElementType(isInteger())))))))).bind(ID);

StatementMatcher RepairableAtom =
  anyOf(declRefExpr(to(varDecl(anyOf(hasType(isInteger()),
                                     hasType(pointerType()))))).bind(ID),
        declRefExpr(to(enumConstantDecl())).bind(ID),
        declRefExpr(to(namedDecl())), // NOTE: no binding because it is only for member expression
        integerLiteral().bind(ID),
        characterLiteral().bind(ID),
        memberExpr().bind(ID), // TODO: I need to make sure that base is a variable here
        castExpr(hasType(asString("void *")), hasDescendant(integerLiteral(equals(0)))).bind(ID)); // NULL
               
StatementMatcher RepairableNode =
  anyOf(RepairableOperator,
        RepairableArraySubscript,
        RepairableAtom);

StatementMatcher NonRepairableNode =
  unless(RepairableNode);

StatementMatcher BaseRepairableExpression =
  allOf(RepairableNode,
        unless(hasDescendant(expr(ignoringParenImpCasts(NonRepairableNode)))));

StatementMatcher NonBaseRepairableExpression =
  anyOf(unless(RepairableNode),
        hasDescendant(expr(ignoringParenImpCasts(NonRepairableNode))));

StatementMatcher Splittable =
  anyOf(binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(BaseRepairableExpression)),
                       hasRHS(ignoringParenImpCasts(NonBaseRepairableExpression))),
        binaryOperator(anyOf(hasOperatorName("||"), hasOperatorName("&&")),
                       hasLHS(ignoringParenImpCasts(NonBaseRepairableExpression)),
                       hasRHS(ignoringParenImpCasts(BaseRepairableExpression))));

auto hasSplittableCondition =
  anyOf(hasCondition(ignoringParenImpCasts(BaseRepairableExpression)),
        eachOf(hasCondition(Splittable), hasCondition(forEachDescendant(Splittable))));

StatementMatcher RepairableCondition = 
  anyOf(ifStmt(hasSplittableCondition),
        whileStmt(hasSplittableCondition),
        forStmt(hasSplittableCondition));

// FIXME: why to restrict to variables, members and arrays?
StatementMatcher RepairableAssignment =
  binaryOperator(anyOf(hasOperatorName("="),
                       hasOperatorName("+="),
                       hasOperatorName("-="),
                       hasOperatorName("*="),
                       hasOperatorName("/="),
                       hasOperatorName("%="),
                       hasOperatorName("&="),
                       hasOperatorName("|="),
                       hasOperatorName("^="),
                       hasOperatorName("<<="),
                       hasOperatorName(">>=")),
                 anyOf(hasLHS(ignoringParenImpCasts(declRefExpr())),
                       hasLHS(ignoringParenImpCasts(memberExpr())),
                       hasLHS(ignoringParenImpCasts(arraySubscriptExpr()))),
                 hasRHS(ignoringParenImpCasts(BaseRepairableExpression)));

StatementMatcher RepairableExpression =
  anyOf(RepairableCondition,
        RepairableAssignment);

StatementMatcher RepairableStatement =
  anyOf(callExpr().bind(ID),
        breakStmt().bind(ID),
        continueStmt().bind(ID));
