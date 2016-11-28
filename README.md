![workflow](doc/logo.png)

f1x is a test-driven automated program repair tool for C/C++ program. It can automatically find and fix bugs in program expressions based on given passing and failing tests. f1x was designed for the purpose of large scale experimentation, which is critical for the future of automated program repair research. Specifically, f1x aims to be efficient, reliable and easy-to-use.

f1x implements an efficient search space exploration algorithm that performs semantic search space partitioning during test execution. Essentially, it combines ideas from existing syntax-based and semantics-based techniques in a mutually reinforcing fashion. First, this enables f1x to achieve X times efficiency boost when repairing large programs (such as PHP, Libtiff, etc) compared to previous algorithms. Second, f1x is the first system that guarantees that the generated patch is always the most reliable in the search space according to a given static prioretization strategy. Specifically, in the current implementation f1x guarantees to find a modification that is syntactically minimal in the entire search space. More details about f1x algorithm can be found in the publication.

## Installation ##

f1x currently supports Linux-based systems; it was tested on Ubuntu 14.04.

Install dependencies (Ubuntu):

    sudo apt-get install build-essential cmake bear libboost-program-options-dev libboost-log-dev libboost-filesystem-dev
    
Install LLVM and Clang 3.8.1. You can use the provided installation script to download, build and install it locally:

    cd <my-llvm-dir>
    /path/to/f1x/scripts/download-and-build-llvm38.sh
    
To compile f1x, create `build` directory and execute:
    
    mkdir build
    cd build
    cmake -DLLVM_DIR=<my-llvm-dir>/install/share/llvm/cmake/ -DClang_DIR=<my-llvm-dir>/install/share/clang/cmake/ ..
    make
    
To install f1x, add the `tools` directory into your `PATH`:

    export PATH=$PWD/tools:$PATH
    
To test f1x, execute `./tests/runall.sh`.
    
## Usage ##
    
Before using f1x:

- Create a copy of your project (f1x can corrupt your source files)
- Configure your project using f1x compiler wrapper (e.g. `CC=f1x-cc ./configure`)
- Clean binaries (e.g. `make clean`)

Example:

    $ f1x grep-1.4.5/src/ -d tests/run.sh -t 1 2 3
    run time:     0:12:54
    total execs:  1401
    exec speed:   5.4/sec
    evaluated:    120042
    
## Algorithm ##

Initialization:

1. Build, construct compilation database with bear
2. Get specified tests/run failing test and gcov to identify suspicious tests
3. Generate and compile search space for suspicious files, instrument suspicious files
4. Execute all tests, collect coverage and initial patritions
5. Prioritize locations

Validation (for single location):

1. Three sets of patches: grey, green and red. green is empty, red has only original program, grey contains the rest. each grey patch corresponds to a set of tests is passes.
2. Pick grey patch according to heuristic
3. If the patch passes all tests, add it to the green set, remove from grey
4. Pick not evaluated test according to heuristic
5. Evaluate the patch on the test
6. If the patch passes, add the corresponding partition to the test
7. If the patch fails, remove partition from grey, add to red
8. Goto 2

## Documentation ##

TODO: Tutorial for small programs, tutorial for large programs, manual, troubleshooting
TODO: acknowledgement

TODO: support expression inside case
