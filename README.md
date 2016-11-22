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

    sudo apt-get install build-essential cmake libboost-program-options-dev libboost-log-dev libboost-filesystem-dev libgtest-dev rapidjson-dev libclang-3.8-dev bear
    
To compile, create and change to `build` directory and execute:

    cmake -DLLVM_DIR=/home/sergey/lsym/install/share/llvm/cmake/ -DClang_DIR=/home/sergey/lsym/install/share/clang/cmake/ -G Ninja ..
    
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
                            
Defect classes:

- side effect free conditions (solver-based grouping engine)
- side effect free assignments (enumeration-based grouping engine)
- conditions with side effects (generate-and-validate for common bugs)
- assignments with side effects (generate and validate for common bugs)
- guards (with small side-effect free conditions)
- pointers (type-based enumeration)
- new assignments before variable use
                            
f1x creates `/tmp/f1x-DATE-TIME` directory containing generated patches and logs. Patches have names

    1-source_name-30-12-30-15.patch
    2-source_name-30-12-30-15.patch
    ...
    
f1x also creates `1-30-12-30-15.meta` files with additional information about generated patches. You can then use `f1x-prioritize /tmp/f1x-DATE-TIME | head -3` to select there best patches.

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
