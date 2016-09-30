                  __                       
             ...-'  |`.                    
             |      |  |                   
         _.._....   |  |                   
       .' .._| -|   |  |                   
       | '      |   |  | ____     _____    
     __| |__ ...'   `--'`.   \  .'    /    
    |__   __||         |`.`.  `'    .'     
       | |   ` --------\ |  '.    .'       
       | |    `---------'   .'     `.      
       | |                .'  .'`.   `.    
       | |              .'   /    `.   `.  
       |_|             '----'       '----' 

f1x is an efficient automated progam repair tool. It fixes bugs manifested by failing tests by traversing a search space of candidate patches. f1x achieves high throughput (number of candidate patches per a unit of time) by representing the search space symbolically, and dynamically grouping semantically equivalent patches. It also ranks generated patches based on two criteria: (1) syntactical distance from the original program and (2) matching pre-defined set of anti-patterns.

TODO: tell a bit more about test-driven automated program repair, difference from human patches.

## Installation ##

f1x currently supports only Linux-based systems. It was tested on Ubuntu 14.04, Ubuntu 16.04 and Fedora 24.

Install dependencies (Ubuntu):

    sudo apt-get install build-essential cmake libboost-all-dev libclang-3.8-dev bear
    
To compile, create and change to `build` directory and execute:

    cmake -DLLVM_DIR=/home/sergey/lsym/install/share/llvm/cmake/ -DClang_DIR=/home/sergey/lsym/install/share/clang/cmake/ -G Ninja ..
    
Before using f1x:

- Create a copy of your project (f1x can corrupt your source files)
- Configure your project using f1x compiler wrapper (e.g. `CC=f1x-cc ./configure`)
- Clean binaries (e.g. `make clean`)

f1x can be executed with the following options:

    f1x path/to/project --files src/lib_a.c src/lib_b.c
                        --tests n1 n2 p1 p2
                        --generic-driver /full/path/to/test.sh
                        --test-timeout 5
                        --make 'make -e'
                        --term-on-first-found
                        --verbose
                        --version
                        --help

Example:

    $ f1x grep-1.4.5/src/ --generic-driver tests/run.sh --tests 1 2 3
    run time:     0:12:54
    total execs:  1401
    exec speed:   5.4/sec
    evaluated:    120042
    patches:      34
    uniq patches: 3
    
By default, the repair process will continue until you press Ctrl-C or the search space is exhausted. You can use `--term-on-first-found` if you are interested in a single patch. However, it is not recommended for general usage. f1x prioretizes patches based on several criteria such as syntactical distance, and the more patches are found, the higher the expected quality of the best found patch.
                            
Defect classes:

- side effect free conditions (solver-based grouping engine)
- side effect free assignments (enumeration-based grouping engine)
- conditions with side effects (generate-and-validate for common bugs)
- assignments with side effects (generate and validate for common bugs)
- guards (with small side-effect free conditions)
- pointers (type-based enumeration)
- new assignments before variable use
                            
f1x creates `f1x-out-N` directory containing generated patches and logs. Patches have names

    X_Y_Z.patch
    
where `X` is the corresponding defect class, `Y` indicates the order in which the search space is explored, `Z` is higher for syntactically more complex modifications.

TODO: Tutorial for small programs, tutorial for large programs, manual, troubleshooting

TODO: acknowledgement
