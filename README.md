![workflow](doc/logo.png)

f1x is a test-driven automated program repair engine for C/C++ programs. It can automatically find and fix bugs in program expressions based on given passing and failing tests. f1x was designed for the purpose of large scale experimentation, which is critical for the future of automated program repair research. Specifically, f1x aims to be reliable, efficient and easy-to-use.

f1x combines ideas from existing syntax-based and semantics-based techniques in a mutually reinforcing fashion by performing semantic search space partitioning during test execution. This enables f1x to traverse the search space in an arbitrary order without sacrificing efficiency. As a result, f1x is the first system that guarantees to always generate the most reliable patch in the search space according to a given static prioritization strategy. Specifically, in the current implementation f1x finds the syntactically minimal modification in the entire search space. Apart from that, f1x explores search space X times faster compared to previous approaches when repairing large programs such as PHP, Libtiff, etc.

More details about f1x can be found in our publication.

## Installation ##

f1x currently supports Linux-based systems; it was tested on Ubuntu 14.04 and Ubuntu 16.04.

Install dependencies (GCC, G++, Make, Boost.Filesystem, Boost.Program_options, Boost.Log, diff, patch):

    # Ubuntu:
    sudo apt-get install build-essential libboost-filesystem-dev libboost-program-options-dev libboost-log-dev
    
Install a new version of `cmake` (3.4.3 or higher).

Install LLVM and Clang 3.8.1. You can use the provided installation script to download, build and install it locally:

    cd <my-llvm-dir>
    /path/to/f1x/scripts/download-and-build-llvm381.sh
    
To compile f1x, create `build` directory and execute:
    
    mkdir build
    cd build
    cmake -DF1X_LLVM=<my-llvm-dir>/install/ ..
    make
    
To install f1x, add the `tools` directory into your `PATH`:

    export PATH=$PWD/tools:$PATH
    
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
