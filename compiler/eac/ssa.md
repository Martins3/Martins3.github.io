# static single assignment

1. 为什么需要使用 dominance frontier 来实现 ?
2. SSA 带来的好处是什么 ? // 从循环的操作到
3. 求解 SSA 的基本方法是什么 ? phi的方法 ?

## https://en.wikipedia.org/wiki/Static_single_assignment_form

**which requires that each variable is assigned exactly once.**

One can expect to find SSA in a compiler for Fortran or C,
whereas in functional language compilers, such as those for Scheme, ML and Haskell, continuation-passing style (CPS) is generally used.
SSA is formally equivalent to a well-behaved subset of *CPS* excluding non-local control flow,
which does not occur when CPS is used as intermediate representation. So optimizations and transformations formulated in terms of one immediately apply to the other.
> CPS ??

Compiler optimization algorithms which are either enabled or strongly enhanced by the use of SSA include:
- Constant propagation
- Value range propagation[3]
- Sparse conditional constant propagation
- Dead code elimination
- Global value numbering
- Partial redundancy elimination
- Strength reduction
- Register allocation
> @todo 逐个阅读，体会SSA 的作用是什么 ?

Given an arbitrary control flow graph, *it can be difficult to tell where to insert `Φ` functions,
and for which variables.* This general question has an efficient solution that can be computed using a concept called dominance frontiers (see below).

Now we can define the **dominance frontier**: a node B is in the dominance frontier of a node A if A does not *strictly* dominate B, but does dominate some immediate predecessor of B.
> 就是一个dominate，然后一个strictly dominate的，@todo 没有细品

https://en.wikipedia.org/wiki/Dominator_(graph_theory)

> @todo 计算方法


## https://www.cs.cmu.edu/~fp/courses/15411-f08/lectures/09-ssa.pdf
