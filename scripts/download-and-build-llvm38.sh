#!/bin/bash
set -euo pipefail

download () {
    wget --retry-connrefused --waitretry=1 --read-timeout=20 --timeout=15 --tries=5 --continue "$1"
}

require () {
    hash "$1" 2>/dev/null || { echo >&2 "I require $1 but it's not installed. Aborting."; exit 1; }
}

require wget
require cmake

LLVM_URL="http://llvm.org/releases/3.8.1/llvm-3.8.1.src.tar.xz"
LLVM_ARCHIVE="llvm-3.8.1.src.tar.xz"
CLANG_URL="http://llvm.org/releases/3.8.1/cfe-3.8.1.src.tar.xz"
CLANG_ARCHIVE="cfe-3.8.1.src.tar.xz"
COMPILER_RT_URL="http://llvm.org/releases/3.8.1/compiler-rt-3.8.1.src.tar.xz"
COMPILER_RT_ARCHIVE="compiler-rt-3.8.1.src.tar.xz"
CLANG_TOOLS_EXTRA_URL="http://llvm.org/releases/3.8.1/clang-tools-extra-3.8.1.src.tar.xz"
CLANG_TOOLS_EXTRA_ARCHIVE="clang-tools-extra-3.8.1.src.tar.xz"

INSTALL_DIR="$PWD/install"

download "$LLVM_URL"
download "$CLANG_URL"
download "$COMPILER_RT_URL"
download "$CLANG_TOOLS_EXTRA_URL"

mkdir -p "src"
tar xf "$LLVM_ARCHIVE" --directory "src" --strip-components=1

mkdir -p "src/tools/clang"
tar xf "$CLANG_ARCHIVE" --directory "src/tools/clang" --strip-components=1

mkdir -p "src/projects/compiler-rt"
tar xf "$COMPILER_RT_ARCHIVE" --directory "src/projects/compiler-rt" --strip-components=1

mkdir -p "src/tools/clang/tools/extra"
tar xf "$CLANG_TOOLS_EXTRA_ARCHIVE" --directory "src/tools/clang/tools/extra" --strip-components=1

mkdir -p "build"
mkdir -p "install"

(
    cd "build"
    cmake "../src" -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR"
    make
    make install
)






