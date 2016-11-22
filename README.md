![workflow](doc/logo.png)

f1x is an efficient automated progam repair tool. It fixes bugs manifested by failing tests by traversing a search space of candidate patches. f1x achieves high throughput (number of candidate patches per a unit of time) by dynamically grouping patches that have equivalent semantic impact. It also ranks generated patches based on two criteria: (1) syntactical distance from the original program and (2) matching pre-defined set of anti-patterns.

## Installation ##

f1x currently supports only Linux-based systems. It was tested on Ubuntu 14.04.

Install dependencies (Ubuntu):

    sudo apt-get install build-essential cmake libboost-all-dev bear
    
Install LLVM and Clang 3.8. You can use a provided installation script to download and build it automatically:

    cd $MY_LLVM_DIR
    /path/to/f1x/scripts/download-and-build-llvm38.sh
    
To compile, create and change to `build` directory and execute:

    cmake -DLLVM_DIR=$MY_LLVM_DIR/install/share/llvm/cmake/ -DClang_DIR=$MY_LLVM_DIR/install/share/clang/cmake/ ..
    
## Usage ##
    
Before using f1x:

- Create a copy of your project (f1x can corrupt your source files)
- Configure your project using f1x compiler wrapper (e.g. `CC=f1x-cc ./configure`)
- Clean binaries (e.g. `make clean`)

f1x can be executed with the following options:

    Usage: f1x PATH OPTIONS
    
    -f | --files FILE...       buggy source files
    -t | --tests TEST...       tests
    -T | --test-timeout MS     timeout for single test execution (default: none)
    -d | --driver DRIVER       test driver
    -D | --driver-type TYPE    driver type (default: generic)
    -b | --build               build command (default: make -e)
    -o | --output DIR          output directory (default: $PWD/f1x-out-N)
    -v | --verbose LEVEL       verbosity level (default: none)
    -h | --help                help
    --first-found              terminate search on first found patch

Example:

    $ f1x grep-1.4.5/src/ -d tests/run.sh -t 1 2 3
    run time:     0:12:54
    total execs:  1401
    exec speed:   5.4/sec
    evaluated:    120042
    patches:      34
    uniq patches: 3
    
By default, the repair process will continue until you press Ctrl-C or the search space is exhausted. You can use the `--first-found` option if you are interested in a single patch, however it is not recommended, since f1x can select best found patch based on a number of huristics such as syntactical distance. Therefore the more patches are found, the higher the expected quality of the best found patch.

## Interpreting output ##
                            
f1x creates `f1x-out-N` directory containing generated patches and logs. Patches have names

    1.patch
    2.patch
    ...
    
f1x also creates `N.meta` files with additional information about generated patches. You can then use `f1x-prioritize f1x-out-1 | head -3` to select there best patches.

TODO: Tutorial for small programs, tutorial for large programs, manual, troubleshooting

TODO: acknowledgement
