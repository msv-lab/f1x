![logo](doc/logo.png)

f1x [ɛf-wʌn-ɛks] is a test-driven patch generation engine for C/C++ programs. It automatically finds and fixes software bugs by analyzing behaviour of passing and failing tests. f1x aims to be efficient, robust and easy-to-use.

## Why f1x? ##

* ***User/hacker-friendly***. f1x requires virtually no configuration, has a simple interface and a compact implementation.
* ***Experimentation***. f1x comes with environments for executing main program repair benchmarks.
* ***Efficiency***. f1x generates fixes up to 10 times faster than previous program repair systems.
* ***Effectiveness***. f1x can repair more defects since it explores larger spaces of candidate patches than previous systems.
* ***Quality***. f1x searches for patches that optimize a given cost function to prioritize high quality repairs.

## How does it work? ##

f1x generates bug fixes by enumerating and testing a large number of source code changes. Suppose we have a program `p` and a set of tests `[t1, t2, t3, ...]`. f1x generates a space of modifications `[p1, p2, p3, ...]` and test each of these programs until it finds one that passes all the tests. In the other words, it fills the following table:

|    | t1 | t2 | t3 | t4 | t5 | ...
|----|----|----|----|----|----|----
| p1 | Pass | Pass | Pass | Fail | Fail
| p2 | Fail | Fail | Pass | Fail | Fail
| p3 | Pass | Pass | Pass | Pass | Pass
| p4 | Pass | Fail | Fail | Fail | Pass
| p5 | Pass | Fail | Pass | Fail | Pass
|... |

As has been shown in previous research, this approach has two shortcomings:

* ***Efficiency***. Testing a large number of patches can be very time-consuming.
* ***Overfitting***. Even if a patch passes all the tests, it may still be incorrect.

In order to address the efficiency issue, f1x uses test-equivalence analyses in order to group patches that produce the same results on a given test. For example, after executing the program `p4` with the test `t2`, it can detect that `p5` has the same result on `t2` as `p4` and skip redundant executions. As a result, f1x tests around 1000 different patches in a single test execution.

In order to address the overfitting issue, f1x allows to prioritize patches. It assigns a cost (a rational number) to each patch and search for a patch with the lowest cost. For example, if both `p3` and `p6` pass all the tests, but `cost(p6) < cost(p3)`, then f1x outputs `p6` as the result. f1x supports both static (source code-based) and dynamic (execution-based) cost functions.

## Documentation ##

To install f1x, you can either use [our docker image](doc/Docker.md) or [build it from source](doc/BuildFromSource.md).
To get started, please go though [Tutorial](doc/Tutorial.md). More detailed information about the tool is given in [Manual](doc/Manual.md). If you encounter a problem while using f1x, please consult [Troubleshooting guide](doc/Troubleshooting.md) or ask us by email (contact: Sergey Mechtaev, `mechtaev@comp.nus.edu.sg`). If you plan to modify f1x, please refer to [Developer guide](doc/Development.md).

## Evaluation ##

f1x has been evaluated on main automated program repair benchmarks.
We provide a [benchmarking infrastructure](doc/Benchmarking.md) and instantly available environments for reproducing the results.
The following data is obtained with `f1x 0.1`:

| Benchmark | Defects | Plausible patches | Correct patches | Avg. time | Reproduction package 
|----------|-------------------|-----------------|-----------|---------------------|---
| Genprog ICSE'12 | 105 | 49 | 16 | 5 min | [Repository+docker](https://github.com/mechtaev/f1x-genprog-icse12)
| IntroClass | 572 | 118 | 47 | 10 sec | [Repository+docker](https://github.com/mechtaev/f1x-introclass)

## People ##

* Abhik Roychoudhury, Professor, Principal investigator
* Sergey Mechtaev, PhD student, Developer
* Gao Xiang, PhD student, Developer
* Shin Hwei Tan, PhD Student, Contributor
