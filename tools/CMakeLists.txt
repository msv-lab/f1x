add_executable(f1x F1X.cpp)
# Link against LLVM libraries
target_link_libraries(f1x ${Boost_LIBRARIES})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs option support)

add_executable(f1x-instrument F1XInstrument.cpp)

target_link_libraries(f1x-instrument
  ${llvm_libs}
  clangAST
  clangBasic
  clangDriver
  clangFrontend
  clangTooling
  )

set_target_properties(f1x-instrument PROPERTIES COMPILE_FLAGS "-fno-rtti -fno-exceptions" ) # this is to be compatible with llvm libraries
