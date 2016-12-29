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

typedef std::uint_least32_t uint;


enum class Exploration {
  GENERATE_AND_VALIDATE,
  SEMANTIC_PARTITIONING
};


enum class LocalizationFormula {
  OCHIAI,
  JACCARD,
  TARANTULA  
};


enum class TestSelection {
  FIXED_ORDER,      // failing, then passing
  MAX_FAILING,      // prioritize tests that failed maximum number of times
  LARGEST_PARTITION // prioritize tests with largest average partition size
};


const bool USE_GLOBAL_VARIABLES = false;


struct Config {
  bool verbose;
  bool validatePatches;
  bool generateAll;
  bool dumpSearchSpace;
  uint maxConditionParameter;       /* UNSUPPORTED */
  uint maxExpressionParameter;      /* UNSUPPORTED */
  uint maxCandidatesPerLocation;    /* UNSUPPORTED */
  uint maxExecutionsPerLocation;    /* UNSUPPORTED */
  uint maxLocations;                /* UNSUPPORTED */
  Exploration exploration;          /* UNSUPPORTED */
  LocalizationFormula localization; /* UNSUPPORTED */
  TestSelection testSelection;      /* UNSUPPORTED */
  std::string runtimeCompiler;
};


static Config DEFAULT_CONFIG = {
  false,   /* verbose */
  true,    /* validatePatches */
  false,   /* generateAll */
  false,   /* dumpSearchSpace */
  0,       /* maxConditionParameter */
  0,       /* maxExpressionParameter */
  0,       /* maxCandidatesPerLocation */
  0,       /* maxExecutionsPerLocation */
  0,       /* maxLocations */
  Exploration::SEMANTIC_PARTITIONING,
  LocalizationFormula::JACCARD,
  TestSelection::FIXED_ORDER,
  "g++" /* runtimeCompiler */
};
