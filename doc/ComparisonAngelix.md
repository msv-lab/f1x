# Comparison with Angelix #

f1x is the successor of [Angelix](http://angelix.io). We recommend f1x for general use, since it represents a more practical trade-off between efficiency, effectiveness and usability. However, Angelix also provides several unique capabilities not available in f1x.

The advantages of f1x:

- f1x generates more patches (e.g. for Genprog ICSE'12 benchmark);
- f1x is significantly faster (e.g. for Genprog ICSE'12 benchmark);
- f1x supports patch prioritization (due to explicit search space representation and the global maximum guarantee);
- f1x is easier to install and use;
- f1x does not require manual source code instrumentation;
- f1x does not rely on symbolic execution and therefore can be applied to a wider variety of programs;
- f1x is more accurate (its expression synthesizer is bit-precise);
- f1x is not restricted to theories supported by SMT solver (e.g. it supports non-linear arithmetic, etc.)

The advantages of Angelix:

- Angelix can generate multi-line patches;
- Angelix can generate more complex patches (e.g. for Heartbleed vulnerability)
- Angelix can be potentially more reliable if its symbolic search space representation is combined with verification.
