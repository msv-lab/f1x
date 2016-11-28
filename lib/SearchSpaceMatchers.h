#pragma once

#include "clang/ASTMatchers/ASTMatchers.h"

#define BOUND "repairable"


using namespace clang;
using namespace ast_matchers;

/*
  Matches 
  - integer and pointer variables
  - literals
  - arrays subscripts
  - memeber expressions
  - supported binary and pointer operators
 */
extern StatementMatcher BaseRepairableExpression;

/*
  Matches condition (if, while, for) a part of which is BaseRepairableExpression
 */
extern StatementMatcher RepairableCondition;

/*
  Matches RHS of assignments and compound assignments if it is BaseRepairableExpression
  Note: need to manually check if is it top level statement
 */
extern StatementMatcher RepairableAssignment;

/*
  BaseRepairableExpression in relevant context
 */
extern StatementMatcher RepairableExpression;

/*
  Matches call, break and continue statements
  Note: need to manually check if is it top level statement
 */
extern StatementMatcher RepairableStatement;
