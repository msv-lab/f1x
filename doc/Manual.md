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
    
In order to repair a program, f1x requires a special build configuration and an interface to the testing framework.

### Build system ###

It order to let f1x transform and compile your application, you need to substitute C compiler in your build system with the `f1x-cc` tool. For instance, autotools-based projects require

- configuring using f1x compiler wrapper (e.g. `CC=f1x-cc ./configure`)
- removing binaries before executing f1x (e.g. `make clean`)
- ensuring that the build command passed to f1x uses compiler from the `CC` environment variable (e.g. use `make` with the `-e` option)

### Testing framework ###

f1x needs to be able to execute an arbitrary test and to identify if this test passes or fails. To abstract over testing frameworks, f1x uses the following concepts:

- A set of unique test identifiers (e.g. "test1", "test2", ...)
- A test driver executable that accepts a test identifier as the only argument, runs the corresponding test, and terminates with zero exit code if and only if the test passes.

Note that when executing tests f1x appends a path to its runtime library (libf1xrt.so) to the `LD_LIBRARY_PATH` environment variable. Therefore, your testing framework should not overwrite this variable.

### Options ###

f1x accepts one positional argument:

Argument | Description
-------- | -----------
**PATH** | The source directory of your buggy program.

f1x accepts the following options:

Argument | Description
-------- | -----------
**-f [ --files ] RELPATH...** | The list of buggy files. The paths should be relative to the root of the source directory. If omitted, the files are localized automatically.
**-t [ --tests ] ID...** | The list of unique test identifiers.
**-T [ --test-timeout ] MS** | The test execution timeout in milliseconds.
**-d [ --driver ] PATH** | The path to the test driver. The test driver is executed from the root of the source directory.
**-b [ --build ] CMD** | The build command. It omitted, the `make -e` is used. The build command is executed from the root of the source directory.
**-o [ --output ] PATH** | The path to the generated patch. If omitted, the patch is generated in the current directory with the name `<SRC>-<TIME>.patch`
**-v [ --verbose ]** | Enables extended output for troubleshooting
**-h [ --help ]** | Prints help message and exits
**--version** | Prints version and exits
    
## Related publications ##
