# Comparison with Angelix #

f1x is the successor of [Angelix](http://angelix.io), but it also borrows ideas from other systems such as [GenProg](http://dijkstra.cs.virginia.edu/genprog/) and [Prophet](http://groups.csail.mit.edu/pac/patchgen/). f1x is the result of deep reflection, extreme simplification and balancing trade-offs. f1x was designed for the purpose of large scale experimentation: it is reliable (it guarantees to generate the most reliable patch in the search space according to a given static prioritization strategy), it is efficient (it traverses search space X times faster than previous tools), and it is simple (zero configuration, simple and extensible architecture, relatively easy-to-use). However, f1x does not surpass Angelix in all aspects.

Choose f1x if you

- Need more patches
- Need more correct patches
- Need to generate patches faster
- Want to spend less effort for installation and configuration

Choose Angelix if you

- Need more complex patches (e.g. multi-line patches)
- Research symbolic techniques (symbolic execution, SMT solving)
