![logo](doc/logo.png)

f1x [ɛf-wʌn-ɛks] is a test-driven patch generation engine for C/C++ programs. It automatically finds and fixes software bugs by analyzing behaviour of passing and failing tests. f1x aims to be reliable, efficient and easy-to-use.

More details about f1x can be found in our publication.

**Status:** f1x is under active development and maintenance. It provides good support for C language (tested on real-world projects such as PHP, Python, etc.) and initial C++ support (tested on small programs). f1x currently works on Linux-based systems (tested on Ubuntu 14.04 and Ubuntu 16.04).

## Installation ##

Install dependencies (GCC, G++, Make, Boost.Filesystem, Boost.Program_options, Boost.Log, diff):

    # Ubuntu:
    sudo apt-get install build-essential zlib1g-dev libtinfo-dev
    sudo apt-get install libboost-filesystem-dev libboost-program-options-dev libboost-log-dev
    
Install a new version of CMake (3.4.3 or higher, version is important).

[Download](http://releases.llvm.org/download.html) LLVM and Clang 3.8.1 (version is important):

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

## Resources ##

* [Tutorial](doc/Tutorial.md)
* [Manual](doc/Manual.md)
* [Troubleshooting](doc/Troubleshooting.md)
* [Development](doc/Development.md)
* [Experiments with Genprog ICSE'12 benchmark](https://github.com/mechtaev/f1x-genprog-icse12)
* [Experiments with IntroClass benchmark](https://github.com/stan6/f1x-introclass)

## People ##

* Abhik Roychoudhury, Professor, Principal investigator
* Sergey Mechtaev, PhD student, Developer
* Gao Xiang, PhD student, Developer
* Shin Hwei Tan, PhD Student, Contributor
