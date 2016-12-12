# Developer documentation #

The system consists of three main parts:

1. Repair module (f1x) 
2. Code transformation module (f1x-transform)
3. Meta-program runtime (libf1xrt.so)

The repair module is responsible for running tests, maintaining search space and partitioning, and synthesizing meta-program. f1x-transform is responsible for instrumenting buggy code, extracting suspicious expressions and applying patches. The mata-program runtime library is responsible for computing semantic partitions.

## Search space representation ##

The entire search space is explicitly represented as a C++ vector `searchSpace` in the SearchEngine.cpp. The search space can be freely manipulated and efficiently traversed in an arbitrary order.

## Runtime ##

f1x runtime (meta-program) is generated automatically and dynamically linked to the buggy program. The runtime is responsible for computing semantic partitions. It takes a candidate and a search space to partition as the arguments and outputs a subset of the given search space that have the same semantic impact as the given candidate.

The format of communication is the following. The runtime reads a list of candidate IDs from the last line of the input file, in which the first ID corresponds to the evaluated candidate and the rest is the search space to partition:

    123 12 3 942 23 44 3 2 ...
    
After partitioning, it writes a new line in the end of file starting from the evaluated candidate ID and containing all patches with equivalent output value:

    123 12 3 942 23 44 3 2 ...
    123 3 942 44 ...

This protocol ensures correct partitioning when the candidate is executed multiple times or in multiple runtimes.

Each element of the search space is transparently identified by a 32bit ID called `__f1x_id`. A part of the bits used in this ID can encode some information, that is ID is not arbitrary. Apart from that, each candidate has a 32bit location ID `__f1x_loc` that transparently identifies its location in the buggy program. A part of the bits in this ID encode the buggy file.

## Transformation ##

f1x and f1x-transform communicate using JSON format.

f1x-transform represents the extracted expressions in the following way:

    [
        {
            "defect": "assignment",
            "location" : {...},
            "expression": {...},
            "components": [...]
        },
        ...
    ]
    
Where each location is represented as follows:

    {
        "fileId": 0,
        "locId": 1;
        "beginLine": 1,
        "beginColumn": 2,
        "endLine": 1,
        "endColumn": 10
    }

Each expression is represented as follows:

    {
        "kind": "operator",
        "type": "int",
        "repr": ">",
        "args": [
            {
                "kind": "constant",
                "type": "int",
                "repr": 1
            },
            {
                "kind": "variable",
                "type": "unsigned int",
                "repr": "x"
            }
        ]
    }
    
## TODO ##

- support expressions inside case
- support expressions with side-effects (with eager evaluation)
- support project-scale localization with gcov
- what to do if evaluation causes segmentation fault?
