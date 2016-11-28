/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Shin Hwei Tan, Abhik Roychoudhury

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
