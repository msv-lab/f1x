# Developer documentation #

The system consists of three main parts:

1. Repair module (f1x) 
2. Instrumentation module (f1x-instrument)
3. Meta-program runtime (libf1xrt.so)

The repair module is responsible for running tests, maintaining search space and partitioning, and synthesizing meta-program. f1x-instrument is responsible for instrumenting buggy code, extracting suspicious expressions and applying patches. The mata-program runtime library is responsible for computing semantic partitions.

## Search space representation ##

The entire search space is explicitly represented as a C++ vector `searchSpace` in the SearchEngine.cpp. The search space can be arbitrarily manipulated and efficiently traversed in an arbitrary order.

## Runtime ##

f1x runtime (meta-program) is generated automatically and dynamically linked to the buggy program. The runtime is responsible for computing semantic partitions. It takes a candidate and a search space to partition as the arguments and outputs a subset of the given search space that have the same semantic impact as the given candidate.

The format of communication is the following. The runtime reads a list of candidate IDs from the last line of the input file, in which the first ID corresponds to the evaluated candidate and the rest is the search space to partition:

    123 12 3 942 23 44 3 2 ...
    
After partitioning, it writes a new line in the end of file starting from the evaluated candidate ID and containing all patches with equivalent output value:

    123 12 3 942 23 44 3 2 ...
    123 3 942 44 ...

This protocol ensures correct partitioning when the candidate is executed multiple times or in multiple runtimes.

## Repair/Instrumentation interaction ##

f1x and f1x-instrument communicate using JSON format.

f1x-instrument represents the extracted expressions in the following way:

    [
        {
            "fileId": 1,
            "beginLine": 1,
            "beginColumn": 2,
            "endLine": 1,
            "endColumn": 10,
            "defect": "assignment",
            "type": "int",
            "expression": {...}
        },
        ...
    ]

Where each expression is represented as follows:

    {
        "node": "operator",
        "operator": ">",
        "args": [
            {
                "node": "constant",
                "type": "int",
                "value": 1
            },
            {
                "node": "variable",
                "type": "int",
                "name": "x"
            }
        ]
    }
    
## TODO ##

- support expressions inside case
- support expressions with side-effects (with eager evaluation)
- support project-scale localization with gcov
- what to do if evaluation causes segmentation fault?
