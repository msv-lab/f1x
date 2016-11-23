#pragma once

#include "clang/ASTMatchers/ASTMatchers.h"

/*

  The goal of these matchers is to define repair search space.

  Particularly:
  1. boolean/integer/pointer expressions in conditions, assignments, function call arguments, return
  1.1 expression with no side effects
  1.2 expression with side effects and eager evaluation
  2. if guards for certain types of statements (break, continue, function calls, assignments)
  Note that there can be intersection with 1.

 */
