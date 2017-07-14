# f1x benchmarking infrastructure #

This documents describes scripts and format specifications for running various benchmarks with f1x

## Interface ##

- `f1x-bench DEFECT` - reproduce (fetch + setUp + run + tearDown) a repair for DEFECT
- `f1x-bench` - reproduce repairs for all defects

Options:

- `--root` - specify $BENCHMARK_ROOT (current directory by default)
- `--fetch` - only fetch (should create directory DEFECT in current directory)
- `--setUp` - only setUp (e.g. configure)
- `--run` - only run (execute f1x, save results in output directory)
- `--tearDown` - only tearDown (remove DEFECT directory, possibly something else)
- `--print` - only print f1x command for specified DEFECT
- `--list` - list all defects
- `--help` - print help message
- `--timeout` - running timeout
- `--output DIR` - specify output directory

## Format ##

In the root benchmark  directory, there should be `benchmark.json` and `tests.json` files. The content of `benchmark.json` should be as follows (`fetch`, `setUp`, `tearDown` are shell commands, `$BENCHMARK_ROOT` is provided):

    [
        "defect1": {
            "fetch": "$BENCHMARK_ROOT/util/fetch defect1",
            "setUp": "$BENCHMARK_ROOT/util/configure defect1",
            "tearDown": "rm -rf defect1",
            "source": "defect1/src",
            "files": [ "lib/source.c" ],
            "build": "make -e && cd lib1 && make -e",
            "test-timeout": 1000,
            "driver": "util/driver"
        },
        ...
    ]
    
Note that `source` and `driver` are relative to the benchmark root. `build` is optional.

The content of `tests.json` should be as follows

    [
        "defect1": {
            "negative": [ "1", "2" ],
            "positive": [ "3", "4", "5"]
        },
        ...
    ]
