![logo](doc/logo.png)

f1x [ɛf-wʌn-ɛks] is a test-driven patch generation engine for C/C++ programs. It automatically finds and fixes software bugs by analyzing behaviour of passing and failing tests. f1x aims to be efficient, predictable and easy-to-use.

## Why f1x? ##

- f1x is **user/hacker friendly**. It requires virtually no configuration, has a straightforward implementation and minimum dependencies (Clang, Pin).
- f1x comes with a **large experimentation infrastructure**. We provide environments for executing GenProg ICSE'12 benchmark, IntroClass benchmark and more coming soon.
- f1x generates fixes **10 times faster** than previous systems (for GenProg ICSE'12 benchmark).
- f1x **can fix more bugs** since it implements a larger number of transformations compared with previous systems.
- f1x is designed to operate on search spaces with an arbitrary cost function and therefore it is a perfect **platform for AI-based program repair**.

## Documentation ##

To install f1x, you can either use the docker image `docker pull mechtaev/f1x` or [build f1x from source](doc/BuildFromSource.md).

To get started, go though our [Tutorial](doc/Tutorial.md). More detailed information about the tool and the underlying methodology is given in [Manual](doc/Manual.md). If you encounter a problem while using f1x, please consult [Troubleshooting guide](doc/Troubleshooting.md) or ask us by email (contact: Sergey Mechtaev, `mechtaev@comp.nus.edu.sg`). If you plan to modify f1x, please refer to [Developer guide](doc/Development.md).

f1x has been evaluated on several automated program repair benchmarks. We provide tested and instantly available environments for reproducing

- [experiments with Genprog ICSE'12 benchmark](https://github.com/mechtaev/f1x-genprog-icse12);
- [experiments with IntroClass benchmark](https://github.com/stan6/f1x-introclass);
- more coming soon.

## People ##

* Abhik Roychoudhury, Professor, Principal investigator
* Sergey Mechtaev, PhD student, Developer
* Gao Xiang, PhD student, Developer
* Shin Hwei Tan, PhD Student, Contributor
