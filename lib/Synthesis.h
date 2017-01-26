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

#include <memory>

#include <boost/filesystem.hpp>

#include "RepairUtil.h"


/*
  The meta-program structure:

  static bool __f1x_id_initialized = false;
  static vector<F1XID> __f1x_ids;

  TYPE __f1x_LOCID(ARGS) {
    if (!__f1x_id_initialized) {
      // read from file partition.in
      __f1x_id_initialized = true;
    }

    id = __f1x_id;
    id_int2 = __f1x_id_int2;
    id_bool2 = __f1x_id_bool2;
    id_cond3 = __f1x_id_cond3;
    id_param = __f1x_id_param;

    int int2 = 0;
    bool bool2 = 0;
    bool cond3 = 0;
    int param = 0;
    
    eval_result = 0;

    output_value = 0;
    output_initialized = false;
    idx = 0;
    
  eval_LOCID:
    if (id0) {
      switch () {
      ...
      }
    }
    ...
    // here compute int2, bool2, cond3, eval_result

    if (!output_initialized) {
      output_value = eval_result;
      output_initialized = true;
    } else if (output_value == eval_result) {
      // print id0, id1, id2, id3, id4 to partition.out
    }

    if (__f1x_ids.size() > idx) {
      id0 = __f1x_ids[idx].id0;
      ...
      idx++
      goto eval_LOCID;
    }

    return output_value;
  }

  ---------------------------------------------------------------------
  Currently, we compute distance in the following way:
  
  var -> var = 1
  var -> tree2 = 2
  tree2 -> tree2 = 3
  node -> node && tree3 = 3
  
  Some calculation regadring the size of search space:
  
  n - number of variables
  p - max parameter value

  Subcomponents:
  * BOOL2 - boolean tree with 2 leaves
  * INT2 - integer tree with 2 leaves
  * COND3 - boolean tree with 3 leaves

  BOOL2:
  6 comparison operators * n variables * (n + p) variables + parameter
  = 6n^2 + 6np

  INT2:
  either (n + p)
  5 arithmetic operators * n variables * (n + p) variables + parameter
  = 5n^2 + 5np

  COND3: (X + X) > Y:
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


std::vector<SearchSpaceElement> generateSearchSpace(const std::vector<std::shared_ptr<SchemaApplication>> &schemaApplications,
                                                    const boost::filesystem::path &workDir,
                                                    std::ostream &OS,
                                                    std::ostream &OH,
                                                    const Config &cfg);
