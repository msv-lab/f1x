add_library (f1xInstrument
  TransformationUtil.cpp
  SearchSpaceMatchers.cpp
  Transformer.cpp
  )

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (f1xInstrument PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library (f1xRepair
  Expression.cpp
  MetaProgram.cpp
  SearchEngine.cpp
  )

target_include_directories (f1xRepair PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
