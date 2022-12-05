# Quiescent consistency，Sequential consistency 和 Linearizability
以下使用`QC` `SC` 和 `L` 代指三者。


## 动机
多线程执行，如何描述线程之间的相互关联的方法和强度。

1. `QC`适合描述计数器，只要保证计数不重不漏就可以了。
2. `SC`是现代架构读写内存的一种模式(使用 fence 或者 memory barrier)。

> In the end, the architectures do implement sequential
consistency, but only on demand. We discuss further issues related to sequential
consistency and the Java programming language in detail in Section 3.8.

3. `L`则是最强的要求，也是最常见的要求，比如 queue stack 等数据结构的操作。


## 定义
1. `QC`要求当出现时间点没有任何程序执行的时候，那么该时间点之前和时间点之后的程序执行的顺序不可以改变。

2. 而`SC`则要求，每一个线程函数调用都是按照程序顺序执行的，但是程序之间的顺序没有关系。 因为两个线程的函数调用的先后关系没有要求，所以，下面两种情况对于`SC`是等价的

```txt
// A:
P1 -- q.enq(x) -----------------------------
P2 ---------------------------- q.deq():x --

// B:
P1 ----------------------q.enq(x) ----------
P2 ---q.deq():x ----------------------------
```

3. 当`L`在`SC`的要求，如果两个函数调用没有重叠，那么函数调用顺序不可改变。所以对于上面举得例子，在`L`看来就是不同的，而且例子 B 没有就不满足`L`，因为 q 看到自己首先 dequeu 出来了 x 然后才 enqueue 了一个 x。还有一个很容易理解的说法，`L`希望程序的执行是瞬间发生，所以重叠的两个函数的顺序没有要求，但是不重叠的函数的顺序关系必须保持。

> A sequential history H is legal if each object subhistory is legal for that object.

为什么要强调函数调用的移动，因为给定一个历史(History)，进行函数调用的移动总是为了将该历史变化为一个合法历史，如果先后顺序的约束强，那么意味着不能更加难以转换为合法历史。

## 关系
1. Linearizability 是 sequential consistency　的子集，因为 linearizability 是在 sequential consistency 的基础上添加了一个保持没有 overlapping 的顺序的要求。
2. quiescent consistency 和 sequential consistency 没有包含关系，其实容易理解，
    1. [这里](https://stackoverflow.com/questions/19209274/example-of-execution-which-is-sequentially-consistent-but-not-quiescently-consis)举出一个是 sequential consistency 但是不是 quiescent concurrency 的例子。
    2. [这里](https://stackoverflow.com/questions/48935256/example-of-a-program-order-which-is-quiescently-consistent-but-sequentially-inco)是一个对称的例子
    3. 两者都是 : 对于一个原子寄存器读写。
    4. 两者都不是的例子 : 一个线程 enqueue 一个 1,另个一线程 dequeu 一个 2 出来。

3. 仅仅满足`SC`但是不满足`L`，


## Compositional
何为组合: A correctness property is compositional if, whenever each **object** in the system satisfies , the system as a whole satisfies .

- quiescent consistency is compositional

> Can we, in fact, compose a collection of independently implemented quiescently consistent objects to construct a quiescently consistent system? The answer is, yes: quiescent consistency is compositional, so quiescently consistent objects can be composed to construct more complex quiescently consistent objects

- Figure 3.8 说明 sequential consistency 不是 compositional 的。
-  `L`也是可组合的，证明方法是递归的方法(假设对于任意小于 k 个函数调用的历史是组合的)

## Limit Concurrency
在当前上下文中间，not limit concurrency 其实只是说当前历史是符合某种一致性，比如顺序一致性，并且含有 pending 的 inv，那么当其获得 response 的时候，可以保证历史还是满足该一致性。

> Linearizability’s nonblocking property states that any pending invocation has a correct response, but does not talk about how to compute such a response.

三者都不会 limit concurrency

> - How much does *quiescent consistency* limit concurrency? Specifically, under what circumstances does quiescent consistency require one method call to block waiting for another to complete? Surprisingly, the answer is (essentially), never.
> - How much does linearizability limit concurrency? Linearizability, like sequential consistency, is *nonblocking*. Moreover, like quiescent consistency, but unlike sequential consistency, linearizability is compositional; the result of composing linearizable objects is linearizable.
> - Sequential consistency, like quiescent consistency, is nonblocking: any pending call to a total method can always be completed.

虽然三者都是不会 limit concurrency 的，但是没有保证获得返回需要的步数，所以这三个一致性描述和 wait free 之类 blocking 描述其是对于一个函数或者对象两个维度的描述。
> For now, it suffices to recall the **two key properties** of their design: safety, defined by **consistency conditions**, and liveness, defined by **progress conditions**

## 杂谈
1. 证明一个函数是可线性化的一般方法是什么 ?

> The usual way to show that a concurrent object implementation is linearizable is
to identify for each method a linearization point where the method takes effect.
For lock-based implementations, each method’s critical section can serve as its
linearization point. For implementations that do not use locking, the linearization point is typically a single step where the effects of the method call become
visible to other method calls.

2. 对于可线性化的形式化证明体系，其基本的策略是什么 ?
> TODO

## 参考
- [周刊（第 22 期）：图解一致性模型](https://www.codedump.info/post/20220710-weekly-22/)

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
