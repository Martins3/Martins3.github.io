# Engineering a Compiler : Introduction to Optimization

##  8.1 INTRODUCTION
This chapter
introduces the problems and techniques of code optimization and presents
key concepts through a series of example optimizations.

It can also mean an executable that
uses less energy when it runs or that occupies less space in memory

For each source of inefficiency, the
compiler writer must choose from multiple techniques that claim to improve
efficiency.

Section 8.3 lays out the different **granularities**, or scopes, over which optimization occurs.

A major source of
inefficiency arises from the implementation of source-language abstractions.
With contextual knowledge, the optimizer can often determine that the code does not need that full
generality;
> 高级语言的抽象会导致 inefficiency , 但是持有 contextual knowledge 之后，该问题可以被消除, how ?

A second significant source of opportunity for the optimizer lies with the
target machine.

Issues such as the number of functional units and their capabilities, the latency and bandwidth to various
levels of the memory hierarchy, the various addressing modes supported in
the instruction set, and the availability of unusual or complex operations
all affect the kind of code that the compiler should generate for a given
application.
> 本来以为只要持有指令集即可!

Chapter 9 presents an overview of static analysis.
It describes some of the analysis problems that an optimizing compiler
must solve and presents practical techniques that have been used to solve
them. Chapter 10 examines so-called scalar optimizations—those intended
for a uniprocessor—in a more systematic way


## 8.2 BACKGROUND

This led to a distinction between debugging compilers and
optimizing compilers. A debugging compiler emphasized quick compilation
at the expense of code quality

Modern optimizers assume
that the back end will handle resource allocation; thus, they typically target
an idealized machine that has an unlimited supply of registers, memory, and
functional units. This, in turn, places more pressure on the techniques used
in the compiler’s back end.

#### 8.2.1 Example

* **Improving an Array-Address Calculation**

In Chapter 7, we saw the calculation for row-major order; fortran’s
column-major order is similar
> emmmm 看来第七章讲了一些不得了的事情


## 8.6 GLOBAL OPTIMIZATI

This section presents two examples of global analysis and optimization.
- The first, finding **uninitialized variables** with live information, is not strictly an
optimization. *Rather, it uses global data-flow analysis to discover useful
information about the flow of values in a procedure*. 
We will use the discussion to introduce the computation of **live variables information**, which
plays a role in many optimization techniques, including *tree-height balancing* (Section 8.4.2), the construction of ssa information (Section 9.3), and
*register allocation* (Chapter 13). 
- The second, **global code placement**, uses profile information gathered from running the compiled code to rearrange
the layout of the executable code.


#### 8.6.1 Finding Uninitialized Variables with Live Information
A variable `v` is live at point `p` if and only if there
exists a path in the cfg from `p` to a use of `v` along which `v` is not redefined.
> redefined 还是 re-assigned ?
> live : 如果在p 的位置，v 在使用之前redefine 了，那么意味着p处检测的变量就是蛇皮
> 这难道不就是SSA 中间的内容 ?

We encode live information by computing, for each block b in the procedure,
a set `LiveOut(b)` that contains all the variables that are live on exit
from b. Given a LiveOut set for the cfg’s entry node n0, each variable in
`LiveOut(n0)` has a potentially uninitialized use.
> LiveOut : 在block 的离开位置，该变量依旧为live的，endpoint of block 就是 live 定义中间的 p

Problems in data-flow analysis are typically posed as a
set of *simultaneous equations* over sets associated with the nodes and edges of a graph.
> simultaneous equations 是啥 ?
> graph ?
It can be referenced in m before it is redefined in m, in which case v ∈ UEVar(m)

Kill : 重新赋值，即为kill
UE :  使用过之前的block 的变量，当然在kill 之前使用，否则就不算了。
> 总体来说，计算 LiveOut 的公式非常科学

> 接下来分析 LiveOut 的作用是什么 ?
> 未初始化变量其实并不是 declaration 的没有初始化就不行，显然依赖于data-flow的
> 如果对于 uninitialized 在某一条路径上没有找到 LiveOut 集合中间 ? 路径，显然不可能随意确定的 ?

* ***Finding Uninitialized Variables***

If `v ∈ LiveOut(n0)`, where n0 is the
entry node of the procedure’s cfg, then, by the construction of LiveOut(n0),
there exists a path from n0 to a use of v along which v is not defined.
> 是不是可以说，从该变量定义所在的block来分析 uninitialized variable ?

> 从当前的内容分析里面，无论entry 是做什么，对于其liveout 分析都是没有影响的

> liveOut 表示从此处之后，会有变量依赖于此
> liveOut 求解出来的内容会比本来申明含有的内容更加多(申明，本身就被弱化了)
> uninitialized 分析，其实可以简化成为添加一个新的节点在开始，将变量的declaration的语句删除，仅仅分析assign 的内容.

However, this approach may yield false positives for several reasons.

- If v is accessible through another name and initialized through that ...
name, live analysis will not connect the initialization and the use. This
situation can arise when a pointer is set to the address of a local
variable, as in the code fragment shown in the margin.

```c
p = &x;
*p = 0; // 从LiveOut 的角度，本语句的存在其无法检查，进而UEvar无法判断
...
x = x + 1;
```

- 静态变量，在当前函数分析之前就存在，看上去是 uninitialized 初始化，但是其实早就可以使用了。
- live variable 分析过于保守，但是其实无法确定。

To discover that s is initialized on the first
iteration of the for loop, the compiler would need to combine an analysis
that tracked individual paths with some form of constant propagation and
with live analysis. 

* ***Other Uses for Live Variables***
> 列举三个，不记得就看书吧，傻狗!

#### 8.6.2 Global Code Placement
> skip , although interesting
