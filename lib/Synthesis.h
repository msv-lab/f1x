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
  Expression synthesizer:

  BOOL2 is "x > y", "p != NULL" (there cannot be BOOL1 in our type system)
  INT2 is "x + y"
  COND3 is either BOOL2 or "x > INT2"

  Note that currently we cast all INT2 to EXPLICIT_INT_CAST_TYPE

  narrowing/tightening is done with COND3
 */

std::vector<SearchSpaceElement>
generateSearchSpace(const std::vector<std::shared_ptr<SchemaApplication>> &schemaApplications,
                    const boost::filesystem::path &workDir,
                    std::ostream &OS,
                    std::ostream &OH,
                    const Config &cfg);
