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

f1x expression synthesizer is bit-precise; it supports all signed and unsigned builtin (C99) integer types.

### Prioritization ###

f1x employs change minimality as the default patch prioritization strategy. It guarantees to generate the syntactically minimal patch in the entire search space. The syntactical change is measured in the number of changed AST nodes. For certain situations (e.g. how to compare `x - y ---> x + y` and `x - y ---> y - x`), the score for a patch is defined in a heuristical manner.

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

### Command-line interface ###

f1x command-line tool accepts user options, executes the repair algorithm and saves the generated patch into a patch file in unidiff format. f1x prints log messages on the standard error output and terminates with zero exit code if and only if it finds a patch.

f1x accepts one positional argument:

Argument | Description
-------- | -----------
`PATH` | The source directory of your buggy program.

f1x accepts the following options:

Argument | Description
-------- | -----------
`-f [ --files ] RELPATH...` | The list of buggy files. The paths should be relative to the root of the source directory.
`-t [ --tests ] ID...` | The list of unique test identifiers.
`-T [ --test-timeout ] MS` | The test execution timeout in milliseconds.
`-d [ --driver ] PATH` | The path to the test driver. The test driver is executed from the root of the source directory.
`-b [ --build ] CMD` | The build command. If omitted, `make -e` is used. The build command is executed from the root of the source directory.
`-o [ --output ] PATH` | The path to the output patch (or directory when used with `--all`). If omitted, the patch is generated in the current directory with the name `<SRC>-<TIME>.patch` (or in the direcotry `<SRC>-<TIME>` when used with `--all`)
`-a [ --all ]` | Enables generation of all patches
`-v [ --verbose ]` | Enables extended output for troubleshooting
`-h [ --help ]` | Prints help message and exits
`--version` | Prints version and exits

### Advanced configuration ###

f1x allows to restrict the search space to certain parts of the source code files. In the following example, the candidate locations will be restricted to the line 20 of `main.c` and from the line 5 to the line 45 (inclusive) of `lib.c`:

    --files main.c:20 lib.c:5-45

Various algorithm parameters can be modified in the `Config.h.in` file (requires reconfiguration and recompilation).
    
## Related publications ##

**Angelix: Scalable Multiline Program Patch Synthesis via Symbolic Analysis.** [\[pdf\]](http://www.comp.nus.edu.sg/~abhik/pdf/ICSE16-angelix.pdf)  
S. Mechtaev, J. Yi, A. Roychoudhury.  
International Conference on Software Engineering (ICSE) 2016.  

**DirectFix: Looking for Simple Program Repairs.**  [\[pdf\]](https://www.comp.nus.edu.sg/~abhik/pdf/ICSE15-directfix.pdf)  
S. Mechtaev, J. Yi, A. Roychoudhury.  
International Conference on Software Engineering (ICSE) 2015.  

**SemFix: Program Repair via Semantic Analysis.** [\[pdf\]](https://www.comp.nus.edu.sg/~abhik/pdf/ICSE13-SEMFIX.pdf)  
H.D.T. Nguyen, D. Qi, A. Roychoudhury, S. Chandra.  
International Conference on Software Engineering (ICSE) 2013.  
