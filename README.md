![logo](doc/logo.png)

f1x [ɛf-wʌn-ɛks] is a test-driven patch generation engine for C/C++ programs. It automatically finds and fixes software bugs by analyzing behaviour of passing and failing tests. f1x aims to be efficient, predictable and easy-to-use.

## Motivation ##

* ***User/hacker-friendly***. f1x requires virtually no configuration, has a simple interface and a compact implementation.
* ***Experimentation***. f1x comes with environments for executing GenProg ICSE'12 and IntroClass benchmarks.
* ***Efficiency***. f1x generates fixes up to 10 times faster than previous systems (for GenProg ICSE'12 benchmark).
* ***Effectiveness***. f1x can repair more defects since it explores larger spaces of candidate patches than previous systems.
* ***Quality***. f1x searches for patches that optimize a given cost function to prioritize high quality repairs.

## Documentation ##

To install f1x, you can either use [our docker image](doc/Docker.md) or [build it from source](doc/BuildFromSource.md).

To get started, please go though [Tutorial](doc/Tutorial.md). More detailed information about the tool is given in [Manual](doc/Manual.md). If you encounter a problem while using f1x, please consult [Troubleshooting guide](doc/Troubleshooting.md) or ask us by email (contact: Sergey Mechtaev, `mechtaev@comp.nus.edu.sg`). If you plan to modify f1x, please refer to [Developer guide](doc/Development.md).

f1x has been evaluated on several automated program repair benchmarks. We provide a [benchmarking infrastructure](doc/Benchmarking.md) and instantly available environments for reproducing:

* [Experiments with Genprog ICSE'12 benchmark](https://github.com/mechtaev/f1x-genprog-icse12).
* [Experiments with IntroClass benchmark](https://github.com/mechtaev/f1x-introclass).

## People ##

* Abhik Roychoudhury, Professor, Principal investigator
* Sergey Mechtaev, PhD student, Developer
* Gao Xiang, PhD student, Developer
* Shin Hwei Tan, PhD Student, Contributor
