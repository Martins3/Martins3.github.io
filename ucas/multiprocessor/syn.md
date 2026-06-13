# 多核总结

## Read the book
We will see that in a system of n or more concurrent threads, it is impossible to construct
a wait-free or lock-free implementation of an object with consensus number n from an object with a lower consensus number.


## 问题
2. memory 和 cache 一致性 和　顺序一致性的关系
https://zh.wikipedia.org/wiki/%E7%BA%BF%E6%80%A7%E4%B8%80%E8%87%B4%E6%80%A7

https://juejin.im/post/5d328b94e51d45554877a698

线性一致性给我们这样一种错觉：并发进程的每一个操作是在调用和响应之间的某个点瞬时生效的。这也意味着并发对象的操作可以使用“先决条件”（pre-conditions）和“后置条件”（post-conditions）来定义

#### 各种数据结构的wait free 转化的基本方法是什么 ?
1. 读取，compareAndSet 失败，重复尝试 (linkedlist queue 的push 计数器)
2. help : atomic snapshot 和 通用 wait-free 构造 的方法


# 总结时间


## wait-free lock-free && obstruction free
1. https://stackoverflow.com/questions/46489805/what-is-an-example-of-obstruction-free-algorithm-that-is-not-lock-free
> The core difference between lock-freedom and obstruction-freedom is that the latter doesn't guarantee progress if two or more threads are running (but does if only one is running).

2. https://stackoverflow.com/questions/4432527/what-is-the-difference-between-lock-free-and-obstruction-free


1. 定义问题

https://stackoverflow.com/questions/37742306/examples-of-wait-lock-obstruction-freedom-and-ooc-methods
http://www.cs.swan.ac.uk/~csdavec/HPC/11LockFreedom.pdf --> ref:A
> 给出来 lock free 的定义 : 其中任意时间总有线程运行
> 1. 到底是不是 lock-free 是 obstruction free 的子集 ? (lock free 的条件下，加上其他线程都停止，显然该线程可以完成，不然该线程不能)
> 2. https://stackoverflow.com/questions/46489805/what-is-an-example-of-obstruction-free-algorithm-that-is-not-lock-free 举出了是 obstruction-free 但是不是 wait-free 的例子。

2. 真的可以带来性能提高吗 ?
ref:A 中间说了各种锁的问题，但是直觉上，从wait-free的snapshot 上，实现根本不可能效率高(比lock)

https://stackoverflow.com/questions/5680869/do-lock-free-algorithms-really-perform-better-than-their-lock-full-counterparts
> 可以确认的想法 : lock free 不停的失败的确像是新的lock
> lock free 的核心是，不在性能，取决于实现，而是防止防止出现一个thread 持有了lock 之后，其他人就没办法了


## java 语言
3. [reentrant lock](https://stackoverflow.com/questions/11821801/why-use-a-reentrantlock-if-one-can-use-synchronizedthis)
2. [atomicreference](https://stackoverflow.com/questions/3964211/when-to-use-atomicreference-in-java)

## https://zhuanlan.zhihu.com/p/20832611?from=singlemessage

## 扩展
https://github.com/iZhangHui/CCiA
https://book.douban.com/subject/4130141/


## 讲义总结
1. 证明可线性化 : 试图确定方法调用的可线性化点，即方法调用产生效果的原子步骤。通常可以考虑代码中离开临界区的点

有些情况下，在同一个执行中，同一个方法的不同调用中
产生效果的点不一样，有的调用中产生效果的点甚至可能落在其它方法的代码中。对这样的
方法需要确定多个可线性化点。

2. safe register 和 atomic 的定义
  1. 77 页 : 4.1.1 regular  4.1.1 safe 4.1.3 atomic
  2. 其实和讲义上一致 :
    1. safe 要求读到最新的，如果没有。
    2. regular 不会超前，就是最新或者重叠的。
    3. atomic 要求读不会重叠出现。

3. 不是重放日志的方法的方法。
  1. head[i]= after； 每一个节点对于共识对象仅仅访问一次，最简单的论证方法就是，第一使用before 下一次max出来必然是 head 的内容。
  2. 每一个线程仅仅使用 共识对象一次 是基本假设。
  3. 讲义提出的第一种方法构造 一个thread 重复使用 共识对象的情况的构造 :

4. lock的实现:
  1. CLH MCS : 前者实现的，询问list 中间的前面的节点，所以NUMA的问题

5. Diffraction tree
  1. 第一种情况，形成一个二叉树
  2. 第二种，二叉树开始位置，
  3. 扁平组合:
    1. 感觉和前面的计数tree非常的类似

## Homework
1. safety : 不会进入错误状态。
2. liveness : 会到达目的。
3. 不使用变好，变坏区分的，而是到底

Exercise 36. Consider the atomic MRSW construction shown in Fig. 4.12. True
or false: if we replace the atomic SRSW registers with regular SRSW registers,
then the construction still yields an atomic MRSW register. Justify your answer.

所有的共识数的证明: 到达临界区，然后两个操作产生的结果相同，虽然顺序不同，但是对于观察者来说，看不到任何区别。

Exercise 54. Suppose we augment the FIFO Queue class with a peek() method
that returns but does not remove the first element in the queue. Show that the
augmented queue has infinite consensus number.

> 首先每一个共识对象仅仅使用一次，所有的总是首先enq 然后 deq 那么peek 得到的总是
> 第一个加入的了



##
187

1. 在Refinable Hash Set的实现中，能否将owner的类型从AtomicMarkableReference改成AtomicReference？如果可以，请给出具体的修改方案。如果不行，请说明理由。

2. Hopscotch Hashing在add()方法中，如果能够发现空表项，并且所有表项的bitmap中都不是全满（全满是指所有位的值都为1），那么空表项的回跳过程一定能正常完成。该结论是否正确？如果认为正确，请给出证明思路。如果认为不正确，请给出反例。

两题二选一。

68. enqueue 根本没有办法得到想要的结果!

78. wait-free 的爆炸，因为其 help 的机制

138. 提供 wait-free 的 AtomicBoolean 方法，感觉需要处理ABA 问题以及 Prism 处理

hopscotch 的内容，

刚才的36题可以这么考虑：令a_table.length=1，也就是变成了由regular SRSW按照fig4.12能否构造出Atomic SRSW。很明显，这个构造不出来。因为正如fig.4.11中read方法表示的，需要比较寄存器写的时间戳和上一次读的时间戳。但是fig 4.12缺少这个比较。因此regular SRSW不能构造Atomic MRSW。
