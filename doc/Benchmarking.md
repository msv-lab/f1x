# f1x benchmarking infrastructure #

This repository contains scripts and format specifications for running various benchmarks with f1x

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
- `--repaired` - perform command only for repaired versions
- `--output DIR` - specify output directory

## Format ##

In the root directory, there should be `benchmark.json` file. The content of `benchmark.json` should be as follows ($BENCHMARK_ROOT is provided):

    [
	"defect1": {
	    "repaired": True,
	    "fetch": "$BENCHMARK_ROOT/fetch defect1",
	    "setUp": "$BENCHMARK_ROOT/configure defect1",
	    "tearDown": "rm -rf defect1",
	    "files": [ "lib/source.c" ],
	    "negative-tests": [ "1", "2" ],
	    "positive-tests": [ "3", "4", "5"],
	    "test-timeout": 1000,
	    "driver": "$BENCHMARK_ROOT/driver"
	},
	...
    ]
