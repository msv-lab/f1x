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

#pragma once

#include "Config.h"


/*
  __f1x_id is a 32 bit transparent candidate ID. The left 10 bits of this id is the parameter value.
 */
uint f1xid(uint baseId, uint parameter) {
  return 0;
}

/*
  __f1x_loc is a 32 bit transparent location ID. The left 10 bits of this id is the file ID.
 */

uint f1xloc(uint baseId, uint fileId) {
  uint result = fileId;
  result <<= 22;
  result += baseId;
  return result;
}

