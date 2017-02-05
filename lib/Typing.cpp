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

#include "Typing.h"
#include <string>


Type operatorOutputType(const Operator &op) {
  switch (op) {
  case Operator::EQ:
    return Type::BOOLEAN;
  case Operator::NEQ:
    return Type::BOOLEAN;
  case Operator::LT:
    return Type::BOOLEAN;
  case Operator::LE:
    return Type::BOOLEAN;
  case Operator::GT:
    return Type::BOOLEAN;
  case Operator::GE:
    return Type::BOOLEAN;
  case Operator::OR:
    return Type::BOOLEAN;
  case Operator::AND:
    return Type::BOOLEAN;
  case Operator::ADD:
    return Type::INTEGER;
  case Operator::SUB:
    return Type::INTEGER;
  case Operator::MUL:
    return Type::INTEGER;
  case Operator::DIV:
    return Type::INTEGER;
  case Operator::MOD:
    return Type::INTEGER;
  case Operator::NEG:
    return Type::INTEGER;
  case Operator::NOT:
    return Type::BOOLEAN;
  case Operator::BV_AND:
    return Type::BITVECTOR;
  case Operator::BV_OR:
    return Type::BITVECTOR;
  case Operator::BV_XOR:
    return Type::BITVECTOR;
  case Operator::BV_SHL:
    return Type::BITVECTOR;
  case Operator::BV_SHR:
    return Type::BITVECTOR;
  case Operator::BV_NOT:
    return Type::BITVECTOR;
  case Operator::BV_IMPLICIT_CAST:
    return Type::INTEGER;
  case Operator::INT_IMPLICIT_CAST:
    return Type::BITVECTOR;
  case Operator::INT_EXPLICIT_CAST:
    return Type::INTEGER;
  }
  throw std::invalid_argument("unsupported operator: " + std::to_string((ulong)op));
}

Type operatorInputType(const Operator &op) {
  switch (op) {
  case Operator::EQ:
    return Type::ANY;
  case Operator::NEQ:
    return Type::ANY;
  case Operator::LT:
    return Type::INTEGER;
  case Operator::LE:
    return Type::INTEGER;
  case Operator::GT:
    return Type::INTEGER;
  case Operator::GE:
    return Type::INTEGER;
  case Operator::OR:
    return Type::BOOLEAN;
  case Operator::AND:
    return Type::BOOLEAN;
  case Operator::ADD:
    return Type::INTEGER;
  case Operator::SUB:
    return Type::INTEGER;
  case Operator::MUL:
    return Type::INTEGER;
  case Operator::DIV:
    return Type::INTEGER;
  case Operator::MOD:
    return Type::INTEGER;
  case Operator::NEG:
    return Type::INTEGER;
  case Operator::NOT:
    return Type::BOOLEAN;
  case Operator::BV_AND:
    return Type::BITVECTOR;
  case Operator::BV_OR:
    return Type::BITVECTOR;
  case Operator::BV_XOR:
    return Type::BITVECTOR;
  case Operator::BV_SHL:
    return Type::BITVECTOR;
  case Operator::BV_SHR:
    return Type::BITVECTOR;
  case Operator::BV_NOT:
    return Type::BITVECTOR;
  case Operator::BV_IMPLICIT_CAST:
    return Type::ANY;
  case Operator::INT_IMPLICIT_CAST:
    return Type::ANY;
  case Operator::INT_EXPLICIT_CAST:
    return Type::ANY;
  }
  throw std::invalid_argument("unsupported operator: " + std::to_string((ulong)op));
}
