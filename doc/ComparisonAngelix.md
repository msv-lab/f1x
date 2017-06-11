# f1x vs Angelix #

### Search methodology ###

From the point of view of search methodology, f1x is an __improvement__ of Angelix that addresses the path explosion problem. The corresponding tradeoffs:

- f1x scales to larger search spaces and therefore can repair more bugs [TBA];
- f1x search is complete (exhaustive), while Angelix search is not [TBA].

### Underlying analysis ###

Angelix is symbolic (uses constraint solving), while f1x is enumerative. Therefore, f1x is a __counterpart__ of Angelix from the point view of underlying analysis. The corresponding tradeoffs: 

- f1x supports arbitrary patch prioritization [TBA];
- f1x is not restricted to theories supported by SMT solver (e.g. supports non-linear arithmetic, etc.);
- f1x does not depend on symbolic execution and therefore can be applied to a wider variety of programs;
- f1x performance is more consistent [NO DOCUMENTED EVIDENCE];
- Angelix is better in synthesizing constants [NO DOCUMENTED EVIDENCE];
- Angelix is better in repairing integer expressions [NO DOCUMENTED EVIDENCE].

### Implementation ###

The implementation tradeoffs:

- f1x does not require manual source code instrumentation;
- f1x is easier to install and use;
- f1x implementation is deterministic, while Angelix implementation is not;
- f1x synthesizer is bit-precise, while Angelix synthesizer is not;
- Angelix can generate multi-line patches, while f1x cannot;
- Angelix contain more complex patches in its search space (e.g. can fix Heartbleed vulnerability).
