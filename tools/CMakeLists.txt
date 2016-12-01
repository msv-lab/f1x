#  This file is part of f1x.
#  Copyright (C) 2016  Sergey Mechtaev, Shin Hwei Tan, Abhik Roychoudhury
#
#  f1x is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.


## f1x target:

add_executable(f1x F1X.cpp)

target_link_libraries(f1x
  f1xRepair
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})


## f1x-instrument target:

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs option support)

add_executable(f1x-instrument F1XInstrument.cpp)

target_link_libraries(f1x-instrument
  f1xInstrument
  ${llvm_libs}
  clangTooling
  )

set_target_properties(f1x-instrument PROPERTIES COMPILE_FLAGS "-fno-rtti -fno-exceptions" ) # this is to be compatible with llvm libraries

## f1x-cc target
configure_file(f1x-cc f1x-cc COPYONLY)
