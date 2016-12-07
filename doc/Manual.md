# Manual #

f1x is a test-driven automated program repair engine for C/C++ programs. It can automatically find and fix bugs in program expressions based on given passing and failing tests. f1x was designed for the purpose of large scale experimentation, which is critical for the future of automated program repair research. Specifically, f1x aims to be reliable, efficient and easy-to-use.

f1x combines ideas from existing syntax-based and semantics-based techniques in a mutually reinforcing fashion by performing semantic search space partitioning during test execution. This enables f1x to traverse the search space in an arbitrary order without sacrificing efficiency. As a result, f1x is the first system that guarantees to always generate the most reliable patch in the search space according to a given static prioritization strategy. Specifically, in the current implementation f1x finds the syntactically minimal modification in the entire search space. Apart from that, f1x explores search space X times faster compared to previous approaches when repairing large programs such as PHP, Libtiff, etc.

## Characteristics ##

The three main characteristics of a repair tool are the search space (syntactical changes that can be generated), the prioritization strategy (which patch is selected from multiple plausible ones) and exploration speed (the number of candidate patches evaluated within a unit of time).

### Search space ###

The search space of f1x is the following:

1. Modification of conditional expressions (inside if, for, while)
2. Modification of RHS of assignments (integer and pointer types)
3. Modification of return arguments (integer and pointer types)
4. Inserting if-guards for break, continue, function calls

f1x expression synthesizer is bit-precise. The following integer types are supported:

* char
* unsinged char
* unsigned short
* unsigned int
* unsigned long
* signed char
* short
* int
* long

All other integer types are currently casted to long.

f1x can operate in two modes: with specified buggy file(s) and when the buggy files are identified automatically using statistical fault localization.

### Prioritization ###

f1x employs change minimality as the default patch prioritization strategy. Its guarantees to generate the syntactically minimal patch in the entire search space. Since the minimality is subjective (e.g. how to compare `x - y ---> x + y` and `x - y ---> y - x`), the score for a patch is defined in a heuristical manner.

### Performance analysis ###

TODO

## Usage ##

**Warning** f1x executes arbitrary modifications of your source code which may lead to undesirable side effects. Therefore, it is recommended to run f1x in an isolated environment. Apply f1x to a copy of your application, since it can corrupt the source code.
    
Before using f1x:

- Create a copy of your project (f1x can corrupt your source files)
- Configure your project using f1x compiler wrapper (e.g. `CC=f1x-cc ./configure`)
- Clean binaries (e.g. `make clean`)

## Related publications ##
