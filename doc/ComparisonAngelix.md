# Comparison with Angelix #

f1x is the successor of [Angelix](http://angelix.io). f1x realizes a theoretically and practically superior search space exploration algorithm, however it may not surpass Angelix in all use cases.

The advantages of f1x:

- f1x generates more patches (e.g. for Genprog benchmark);
- f1x is significantly faster (e.g. for Genprog benchmark);
- f1x provides higher reliability guarantees: it finds the most reliable patch (global maximum) according to a given prioritization strategy;
- f1x is easier to install and use;
- f1x does not require manual source code instrumentation;
- f1x does not rely on symbolic execution and therefore can be applied to a wider variety of programs;
- explicit representation of search space makes it easier to extend (e.g. change prioritization strategy). 

The advantages of Angelix:

- Angelix can generate multi-line patches;
- Angelix uses more informative (symbolic) representation of search space and specification.
