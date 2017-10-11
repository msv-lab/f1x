/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Gao Xiang, Shin Hwei Tan, Abhik Roychoudhury

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

#include "Config.h"
#include "Core.h"
#include "Global.h"


struct Config cfg = {
  /* runtimeCompiler        = */ DEFAULT_RUNTIME_COMPILER,
  /* runtimeOptimization    = */ DEFAULT_RUNTIME_OPTIMIZATION,
  /* globalVariables        = */ DEFAULT_GLOBAL_VARIABLES,
  /* verbose                = */ DEFAULT_VERBOSE,
  /* validatePatches        = */ DEFAULT_VALIDATE_PATCHES,
  /* generateAll            = */ DEFAULT_GENERATE_ALL,
  /* searchSpaceFile        = */ "",
  /* statisticsFile         = */ "",
  /* dataDir                = */ "",
  /* outputPatchMetadata    = */ DEFAULT_OUTPUT_PATCH_METADATA,
  /* removeIntermediateData = */ DEFAULT_REMOVE_INTERMEDIATE_DATA,
  /* insertAssignments      = */ DEFAULT_INSERT_ASSIGNMENTS,
  /* repairCPP              = */ DEFAULT_REPAIR_CPP,
  /* maxConditionParameter  = */ DEFAULT_MAX_CONDITION_PARAMETER,
  /* maxExpressionParameter = */ DEFAULT_MAX_EXPRESSION_PARAMETER,
  /* valueTEQ               = */ DEFAULT_VALUE_TEQ,
  /* dependencyTEQ          = */ DEFAULT_DEPENDENCY_TEQ,
  /* testPrioritization     = */ (DEFAULT_TEST_PRIORITIZATION ? TestPrioritization::MAX_FAILING : TestPrioritization::FIXED_ORDER)
};


