# Building f1x from source #

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
