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

#include <cstdint>

typedef unsigned long ulong;


enum class Exploration {
  GENERATE_AND_VALIDATE, // evaluate patches one-by-one
  SEMANTIC_PARTITIONING  // group patches into semantic partitions
};


enum class LocalizationFormula {
  OCHIAI,
  JACCARD,
  TARANTULA  
};


enum class TestPrioritization {
  FIXED_ORDER,      // first run failing, then passing tests
  MAX_FAILING       // dynamically prioritize tests based on previous failures
};


const bool USE_GLOBAL_VARIABLES = false;


struct Config {
  bool verbose;
  bool validatePatches;
  bool generateAll;
  bool dumpSearchSpace;
  bool outputPatchMetadata; /* UNSUPPORTED */
  bool removeIntermediateData;
  ulong maxConditionParameter;
  ulong maxExpressionParameter;
  Exploration exploration;
  TestPrioritization testPrioritization;
  std::string runtimeCompiler;
};


static Config DEFAULT_CONFIG = {
  false,   /* verbose */
  true,    /* validatePatches */
  false,   /* generateAll */
  false,   /* dumpSearchSpace */
  false,   /* outputPatchMetadata */
  false,   /* removeIntermediateData */
  64,      /* maxConditionParameter */
  1,       /* maxExpressionParameter */
  Exploration::SEMANTIC_PARTITIONING,
  TestPrioritization::MAX_FAILING,
  "g++"    /* runtimeCompiler */
};
