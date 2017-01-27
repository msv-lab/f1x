# Developer documentation #

The system consists of three main parts:

1. Repair module (f1x) 
2. Code transformation module (f1x-transform)
3. Meta-program runtime (libf1xrt.so)

The repair module is responsible for running tests, maintaining search space and partitioning, and synthesizing meta-program. f1x-transform is responsible for instrumenting buggy code, extracting suspicious expressions and applying patches. The mata-program runtime library is responsible for computing semantic partitions.

## Search space representation ##

The entire search space is explicitly represented as a C++ vector `searchSpace` in SearchEngine.cpp. The search space can be freely manipulated and efficiently traversed in an arbitrary order.

## Runtime ##

f1x runtime (meta-program) is generated automatically and dynamically linked to the buggy program. The runtime is responsible for computing semantic partitions. It takes a candidate and a search space to partition as the arguments and outputs a subset of the given search space that have the same semantic impact as the given candidate.

TODO: describe communication protocol

## Transformation ##

f1x and f1x-transform communicate using JSON format.

f1x-transform represents transformation schemas applications to program locations in the following way:

    [
        {
            "appId": 1,
            "schema": "if_guard",
            "context": "condition",
            "location" : {...},
            "expression": {...},
            "components": [...]
        },
        ...
    ]
    
Where each location is represented as follows:

    {
        "fileId": 0,
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
                "kind": "object",
                "type": "unsigned int",
                "repr": "x"
            }
        ]
    }
