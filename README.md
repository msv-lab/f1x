![workflow](doc/logo.png)

f1x is a test-driven automated program repair engine for C/C++ programs. It can automatically find and fix bugs in program expressions based on given passing and failing tests. f1x was designed for the purpose of large scale experimentation, which is critical for the future of automated program repair research. Specifically, f1x aims to be reliable, efficient and easy-to-use.

f1x combines ideas from existing syntax-based and semantics-based techniques in a mutually reinforcing fashion by performing semantic search space partitioning during test execution. This enables f1x to traverse the search space in an arbitrary order without sacrificing efficiency. As a result, f1x is the first system that guarantees to always generate the most reliable patch in the search space according to a given static prioritization strategy. Apart from that, f1x explores search space X times faster compared to previous approaches when repairing large programs such as PHP, Libtiff, etc.

More details about f1x can be found in our publication.

## Installation ##

f1x currently supports Linux-based systems; it was tested on Ubuntu 14.04 and Ubuntu 16.04.

Install dependencies (GCC, G++, Make, Boost.Filesystem, Boost.Program_options, Boost.Log, diff):

    # Ubuntu:
    sudo apt-get install build-essential libboost-filesystem-dev libboost-program-options-dev libboost-log-dev
    
Install a new version of CMake (3.4.3 or higher, version is important).

Download LLVM and Clang 3.8.1 (version is important):

    # Ubuntu 14.04 x86_64:
    wget http://releases.llvm.org/3.8.1/clang+llvm-3.8.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz
    tar xf clang+llvm-3.8.1-x86_64-linux-gnu-ubuntu-14.04.tar.xz
    
Alternatively, you can use the provided script to download, build and install it locally from sources:

    cd <my-llvm-dir>
    /path/to/f1x/scripts/download-and-build-llvm-3.8.1.sh
    
To compile f1x, create `build` directory and execute:
    
    mkdir build
    cd build
    cmake -DF1X_LLVM=<my-llvm-dir> ..
    make
    
To install f1x, add the `build/tools` directory into your `PATH`:

    export PATH=$PWD/build/tools:$PATH
    
To test f1x, execute `./tests/runall.sh`.

## Documentation ##

* [Tutorial (small program)](doc/Tutorial.md)
* [Tutorial (Heartbleed)](doc/Heartbleed.md)
* [Manual](doc/Manual.md)
* [Troubleshooting](doc/Troubleshooting.md)
* [Development](doc/Development.md)

## People ##

* Abhik Roychoudhury, Professor, Principal investigator
* Sergey Mechtaev, PhD student, Developer
* Gao Xiang, PhD student, Developer
