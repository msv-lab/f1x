add_executable(f1x F1X.cpp)
# Link against LLVM libraries
target_link_libraries(f1x ${Boost_LIBRARIES})

# Find the libraries that correspond to the LLVM components
# that we wish to use
llvm_map_components_to_libnames(llvm_libs support) #  clangTooling clangBasic clangAST)

add_executable(f1x-instrument F1XInstrument.cpp)

target_link_libraries(f1x-instrument ${llvm_libs})
