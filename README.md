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

f1x is an efficient patch generation tool. It traverses a search space of side-effect free expression modifications and statement deletions. f1x achieves high throughput (number of candidate patches per a unit of time) by using dynamic semantic search space partitioning and symbolic representation of search space. It also ranks generated patches based on two criteria: syntactical similarity with the original program and anti-patterns (repairs that are likely incorrect).

Install dependencies:

    sudo apt-get install libboost-program-options-dev libboost-log-dev libboost-filesystem-dev
    
To compile, create `build` directory and execute:

    cmake -DLLVM_DIR=/home/sergey/lsym/build/share/llvm/cmake/  -G Ninja ..

f1x can be run with the following options:

    f1x path/to/project --files src/lib_a.c src/lib_b.c
                            --tests n1 n2 p1 p2
                            --google-test-driver src/test
                            --generic-driver /full/path/to/test.sh
                            --stdstreams-driver /path/to/io/files
                            --make 'make -e'
                            --test-timeout 5
                            --first-found
                            --verbose
                            --version
                            --help
                            
f1x supports three test drivers: Generic, Google Test, and Stdstreams.
                            
Defect classes:

    - side effect free conditions (solver-based grouping engine)
    - side effect free assignments (enumeration-based grouping engine)
    - conditions with side effects (generate-and-validate for common bugs)
    - assignments with side effects (generate and validate for common bugs)
    - guards (with small side-effect free conditions)
    - pointers (type-based enumeration)
    - new assignments before variable use
                            
Note that f1x performs source transformation and can corrupt your source code. For this reason, it is recommended to run it on a separate copy of the source tree.

f1x creates `f1x-out-N` directory containing generated patches and logs. Patches have names

    X_Y_Z.patch
    
where `X` is the corresponding defect class, `Y` indicates the order in which the search space is explored, `Z` is higher for syntactically more complex modifications.

Logs are stored in `f1x-out-N/log.txt` file. Execution statistics is available in `f1x-out-N/statistics.txt`:

    space: 100000
    explored: 50000
    locations: 60
    runs: 1000
    patches: 300

Search space:
    
1. Modifying side-effect free conditions and adding conjuncts and disjuncts to non-side-effect free conditions
2. Modifying side-effect free integer expressions
3. Replacing pointers with pointers to a different object of the same type
4. Deleting program statements
