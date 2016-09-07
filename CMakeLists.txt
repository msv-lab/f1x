cmake_minimum_required(VERSION 3.4.3)
project(f1x)

# LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set your project compile flags.
# E.g. if using the C++ header files
# you will need to enable C++11 support
# for your compiler.

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Clang
find_package(Clang REQUIRED CONFIG)

# Boost
FIND_PACKAGE( Boost 1.40 COMPONENTS program_options log REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

ADD_DEFINITIONS(-DBOOST_LOG_DYN_LINK)

# Compiler
set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

# The version number.
set (F1X_VERSION_MAJOR 1)
set (F1X_VERSION_MINOR 0)

# configure a header file to pass some of the CMake settings
# to the source code
configure_file (
  "${PROJECT_SOURCE_DIR}/F1XConfig.h.in"
  "${PROJECT_BINARY_DIR}/F1XConfig.h"
  )

# add the binary tree to the search path for include files
# so that we will find MinifixConfig.h
include_directories("${PROJECT_BINARY_DIR}")


#add_subdirectory(lib)
add_subdirectory(tools)
