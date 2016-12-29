/*
  This file is part of f1x.
  Copyright (C) 2016  Sergey Mechtaev, Abhik Roychoudhury

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

#include <memory>

#include "RepairUtil.h"


/*

  Currently, we compute distance in the following way:
  
  var -> var = 1
  var -> tree2 = 2
  tree2 -> tree2 = 3
  node -> node && tree3 = 3
  
  Some calculation regadring the size of search space:
  
  n - number of variables
  p - max parameter value

  Subcomponents:
  * BoolTree2
  * IntTree2
  * Cond3

  BoolTree2:
  6 comparison operators * n variables * (n + p) variables + parameter
  = 6n^2 + 6np

  IntTree2:
  either (n + p)
  5 arithmetic operators * n variables * (n + p) variables + parameter
  = 5n^2 + 5np

  Cond3: (X + X) > Y:
  (5n^2 + 5np) * 6 * n
  = 30n^3 + 30n^2p

  For n=15, 100000 + 7000 * p, for p = 256, around 2000000 millions candidates

  For a condition, consider the following search space:
  1. substitute each node with IntTree2 or BoolTree2 depending on type
  2. add || Cond3
  3. add && Cond3

  For 5 nodes, we have: 5 * (6n^2 + 6np) + 2 * (30n^3 + 30n^2p)
  = 30n^2 + 30np + 60n^3 + 60n^2p

  For n=15, 7000 + 500p + 2000000 +  15000p
  For p=256, 6000000

  Also need: swap, simplify, change parenthesis
 */


std::vector<SearchSpaceElement> generateSearchSpace(const std::vector<std::shared_ptr<CandidateLocation>> &candidateLocations, std::ostream &OS, std::ostream &OH, const Config &cfg);
