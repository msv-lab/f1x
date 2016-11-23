add_library (f1xInstrument
  TransformationUtil.cpp
  SearchSpaceMatchers.cpp
  MetaProgram.cpp
  )

# Make sure the compiler can find include files for our Hello library
# when other libraries or executables link to Hello
target_include_directories (f1xInstrument PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
