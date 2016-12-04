# Manual #

f1x is a test-driven automated program repair tool for C/C++ program. It can automatically find and fix bugs in program expressions based on given passing and failing tests. f1x was designed for the purpose of large scale experimentation, which is critical for the future of automated program repair research. Specifically, f1x aims to be reliable, efficient and easy-to-use.

f1x combines ideas from existing syntax-based and semantics-based techniques in a mutually reinforcing fashion by performing semantic search space partitioning during test execution. This enables f1x to traverse the search space in arbitrary order without sacrificing efficiency. As a result, f1x is the first system that guarantees that it always generates the most reliable patch in the search space according to a given static prioritization strategy. Specifically, in the current implementation f1x guarantees to find a modification that is syntactically minimal in the entire search space. Apart from that, the exploration algorithm enables f1x to achieve X times efficiency boost when repairing large programs (such as PHP, Libtiff, etc) compared to previous approaches.

## Characteristics ##

The three main characteristics of a repair tool are the search space (syntactical changes that can be generated), the prioritization strategy (which patch is selected from multiple plausible ones) and exploration speed (the number of candidate patches evaluated within a unit of time).

### Search space ###

The search space of f1x is the following:

1. Modification of conditional expressions (inside if, for, while)
2. Modification of RHS of assignments (numeric and pointer types)
3. Modification of return arguments (numeric and pointer types)
4. Inserting if-guards for break, continue, function calls

The search space of f1x is not formally defined; it relies on approximations (e.g. which variables are selected for modifying expressions) and can evolve over time. f1x does not guarantee exhaustiveness.

f1x can operate in two modes: with specified buggy file(s) and when the buggy files are identified automatically using statistical fault localization. In the table below you can see the average size of the generated search spaces (the number of candidate patches) for the subjects of ManyBugs benchmark.

### Performance ###

According to our study on large subject programs, f1x explores search space X times faster then previous approaches. In the table below you can see the average exploration speed (candidates per second) and the average total repair time for subjects in the ManyBugs benchmark.

### Prioritization ###

f1x employs change minimality as the default patch prioritization strategy. Its guarantees to generate the syntactically minimal patch in the entire search space. Since the minimality is subjective (e.g. how to compare `x - y ---> x + y` and `x - y ---> y - x`), the score for a patch is defined in a heuristical and unsystematic manner.

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
