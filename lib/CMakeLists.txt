add_library (f1xInstrument
  TransformationUtil.cpp
  SearchSpaceMatchers.cpp
  Transformer.cpp
  )

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (f1xInstrument PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(f1xInstrument PROPERTIES COMPILE_FLAGS "-fno-rtti -fno-exceptions" ) # this is to be compatible with llvm libraries

add_library (f1xRepair
  F1XCommon.cpp
  Expression.cpp
  MetaProgram.cpp
  SearchEngine.cpp
  )

target_include_directories (f1xRepair PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
