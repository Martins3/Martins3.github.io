# Introduction

## 1
If some thread attempts to acquire the lock, then some thread will succeed in acquiring the lock. Note: the two “some thread” may not be the same

Note that a system can still deadlock even if each of the locks it uses satisfies the deadlock-freedom property.


| starvation free | deadlock free |
|-----------------|-----------------|
| everyone        | some            |


lockOne's proof:
1. Satisfies Mutual Exclusion
2. LockOne fails deadlock-freedom


A **safety** property states that some
“bad thing” never happens. For example, a traffic light never displays green in all
directions, even if the power fails. Naturally, concurrent correctness is also concerned with safety, but the problem is much, much harder, because safety must be
ensured despite the vast number of ways that the steps of concurrent threads can
be interleaved. Equally important, concurrent correctness encompasses a variety
of liveness properties that have no counterparts in the sequential world. A **liveness** property states that a particular good thing will happen. For example, a red
traffic light will eventually turn green. A final goal of the part of the book dealing
with principles is to introduce a variety of metrologies and approaches for reasoning about concurrent programs, which will later serve us when discussing the
correctness of real-world objects and programs

## 2
1. perterson 算法 : 真的可能避免死锁吗 ? @todo 找到哪一个经典教程来打脸，前提是赋值操作是原子性的
2. filter 算法 :  将 peterson 算法扩展到多个，
3. bakery alogrithm : 实现先进先出的算法
> bakery 中间max 已经while 中间的比较都不是原子性的，如何保证？

At least N MRSW (multi-reader/single-writer) registers are needed to solve deadlock-free mutual exclusion. 
1. 假设有一个算法使用 n-1 个内存单元实现了 n 个线程无死锁互斥。
首先注意到，任何一个线程在进入临界区前都必须写至少一个共享内存单元。否则其它
线程无法知道它在临界区，不可能实现互斥。如果只用多读单写的内存单元（如 Bakery　算法），那直接可以推出至少需要 n 个这样的内存单元才能实现无死锁互斥。
2. 
从上面的证明可以看到读、写操作存在的一个局限：一个线程写在一个内存单元中的信
息在还没有被任何其它线程读取之前就可能被抹去。现代的多核架构一般都提供专门的指令
以克服这个局限。

> covering state : 在进入临界区的之前设置的状态


## 3
> 描述历史： 线程 对象 动作

> 动作和回应 : 一对动作和回应必须出现在同一个线程上 ?

> 为什么需要引出来可顺序化，其作用是什么?

产生效果的pending  和 不产生效果的pending

well-formed history : 在每一个线程上的投影都是顺序的历史

equivalent history(历史等价) : 不同的线程上投影的顺序都是相等的

顺序规约（Sequential Specifications）: 描述单线程、单对象的执行历史是否正确的、合法.

一个**顺序历史** H 是**合法**的，如果对于每个对象 x，H | x 满足对象的**顺序规约**。

可线性化（Linearizability）: 补充或者删除之后，顺序关系不可以减少，各个线程上，单对象的正确

> 线程 和 对象

可组合性定理（Compositionality Theorem）历史 H 是可线性化的当且仅当对于每个对象x  H|x 是可线性化的

由于可线性化性质是可组合的，在证明一个系统可线性化时，我们可以对每个对象分别
加以证明，这给证明带来了极大的便利。另外，我们也可以将独立实现的可线性化对象组合
起来，得到的系统也是可线性化的。

历史 H 的延拓是通过在 H 的末尾添加若干个（可以是零个）与 H 中（已经产生效果的)
待结启动相匹配的回应而得到的历史。complete(H)是通过删去 H 中的（还没有产生效果的）
待结启动得到的完整子历史

> 以后只考虑well formed history，因为well formed 历史相当于单线程执行，线程a 执行函数调用，显然不可能在线程 b 中间返回，同样的，线程
> 顺序历史 要求比 well formed history 要求更加严格!


> 顺序一致性不要求不同线程的方法调用保持它们的实时次序.

> @todo 什么样的历史是不可顺序化的:

https://zh.wikipedia.org/wiki/%E7%BA%BF%E6%80%A7%E4%B8%80%E8%87%B4%E6%80%A7
1. 顺序历史的定义
2. 一个历史是well-formed 不一定是可顺序化的，但是可顺序化一定是 well-formed !
3. 简单的介绍其中的历史
> @todo 完备化函数的含义是什么:

https://en.wikipedia.org/wiki/Linearizability

Informally, this means that the unmodified list of events is linearizable if and only if its invocations were serializable, but some of the responses of the serial schedule have yet to return
> 除非稍微修改一下就是 serializable 的，否则!

Concurrent Method Calls Take Overlapping Time

An execution of an object is correct if this “sequential” behavior is correct

A method is wait-free if it guarantees that every call finishes its execution in a finite number of steps.
A method is lock-free if it guarantees that infinitely often some method call finishes in a finite number of steps.
> method 指的是在不同的线程中间的，出现pending ?
> 定义执行上，还是定义在call的数目上 ?

We say that an object is wait-free if its
methods are wait-free, and in an object oriented language, we say that a class is
wait-free if all instances of its objects are wait-free. 

The usual way to show that a concurrent object implementation is linearizable is
to identify for each method a linearization point where the method takes effect.
For lock-based implementations, each method’s critical section can serve as its
linearization point. For implementations that do not use locking, the linearization point is typically a single step where the effects of the method call become
visible to other method calls.
For example, let us recall the single-enqueuer/single-dequeuer queue of
Fig. 3.3. This implementation has no critical sections, and yet we can identify
its linearization points. Here, the linearization points depend on the execution.
If it returns an item, the deq() method has a linearization point when the head
field is updated (Line 17). If the queue is empty, the deq() method has a linearization point when it throws EmptyException (Line 15). The enq() method is
similar

# 4
safe regular atomic 三种类型的寄存器。
1. safe 如果写操作和读操作不重叠，读操作会正确读出之
前（最近）一个写操作的值；如果写操作和读操作重叠，读操作可以读到该寄存
器所允许的任意值。
2. Regular 寄存器在此基础上增加了一条限制：当读、写操作
重叠时，**读操作要么读到当前重叠的写操作的值，要么读到在读操作之前完成的
最近一个写操作的值**。Regular 寄存器的问题在于，对于连续的两个读操作，当
前一个读操作已读到新值时，后一个读操作仍然会读到旧值，

它类型的寄存器对象可以完全基于可单独单写的安全布尔寄存器对象
来实现，而不需要引入额外的互斥机制。也就是说，仅通过读写共享内存，能够
多线程（可线性化地）“互斥”或者“原子”访问共享寄存器对象，在方法级的
粗粒度层面实现宏观并发。

SRSW safe Boolean => MRSW safe Boolean => MRSW regular Boolean =>
MRSW regular => MRSW atomic => MRMW atomic。在构建过程中，要求构建的所
有方法必须是无等待（wait free）的，这样所有寄存器相关的动作可以做到与系
统的调度策略相独立。

1. SRSW safe Boolean => MRSW safe Boolean 用单读单写寄存器数组构建一个多读单写寄存器，当写入的时候，对于整个数组写入，当读出的时候，从特定的数组中间的读出。
    1. 单个寄存器，为什么不能满足SRSW ? 可能是假设访问寄存器多个线程无法同时访问
2. MRSW safe Boolean => MRSW regular Boolean 通过预存所写的最近一个值来避免这个问题。当写线程试图再次写相同的值时，不对寄存器进行写操作即可
    1. 由于使用的是bool 寄存器，所以通过记录上一次写入的数值，只有刷入新的数值的时候才会写入。
    2. 写入新的数值的时候，*同时*读，safe 读任意值，而 regular 读出新的值或者旧的值，当寄存器是binary 的时候，两者等价
3. MRSW regular Boolean => MRSW regular M-Valued 使用数值
    1. 这种方法的核心思想是利用写的可覆盖性对不同写操作的结果进行排序，保证读操作的正规性
    2. 虽然读写可以相交，但是写和写不能重叠
    3. 读，从左向右。写，从右向左。 => 可以保证要么上一个数值，要么当前数值，如果读写冲突。

4. Regular SRSW  => Atomic SRSW : 引入时间戳来实现原子操作，因为regular 已经可以保证只有new 和 old两个value，所以通过time stamp 加以区分，实现读出数值不会出现交叉。
    0. regular 的问题在于，如果两个read 同时和一个write 重合，可能出现先来的读到new 后来读到old 
        1. 可以构造一种非常诡异的可能，新值 5 写，然后一直停留在该节点上，然后read 操作滑动到5 读到为新的值1，返回5，第二个read 滑动到5 读到旧的值0，然后继续滑动到后面的6，返回6
    1. 为了原子性的要求，我们需要引入对逻辑时间进行计量的数据结构：时间戳（time stamp）。其直觉在于：原子寄存器就是可线性化的寄存器，需要用逻辑时
间确定各种操作的先后顺序（线性化点）。
    2. 以确保 lastStamp 和 stamp 的值一定是单调增长的
    3. 读线 程此时会和自己的局部变量 *lastRead* 所保存的值进行比对。如果发现读到旧值，
那就用 lastRead 所保存的新值。这样可以确保每次读操作返回的都是新写的值。
    4. SRSW 的含义必须是 : 总共只有一个reader 和 一个 writer，不然 read 逻辑无法解释。不是只有一个reader 可以同时访问，reader是自由的。
        1. 如果紧跟着的读操作是另一个reader， 那么比较thread local 变量 lastread 毫无意义

5. Atomic SRSW => Atomic MRSW
    1. 解决办法是扩充成二维数组。
    2. *每个读操作读取自己对应索引的整个行，然后比较哪个是最新的，返回最新的值*
    3. *写操作写入自己对应的整个列，保证后续的读操作在其自己的行内一定能读到最近写操作的值.* 讲义自己都错了，我们还是不要浪费时间了吧!
    4. 实际上形成的n^2 的数组

> SRSW 的含义 : 只有两个thread 一个reader 一个 writer
> MRSW 的含义 : 含有多个read thread，一个write thread， 但是不同的read 依旧不能同时发生

# 5
1. 共识协议由一个共享并发对象（称为共识对象）和在其上定义的输入（propose）和决定（decide）方法构成
2. wait‐free 的共识协议可以保证每个线程都能在有穷步内终止。一致且有效的
共识协议可以保证所有线程在终止时都能得到（即决定）一个相同的值（一致性），
而且该值一定是其中某个线程的输入（有效性）。
3. 只通过读写寄存器不能实现无等待的 n‐线程共识协议
4. 定义了关于bivalent和univalent的含义.
5. 定理 2：只通过读写寄存器不能实现无等待的 2‐线程共识协议
> 定理的证明需要看一下，后面２

6. 可以用双出队 FIFO 队列和原子寄存器来实现 2‐线程共识协议。
7. 双出队是指允许两个线程并发地对 FIFO 队列执行出队操作。

8. concensus number
> 有一个理论

9. 多赋值数组: 写操作对于多个内容进行赋值
10. RMW寄存器 : 根据 mumble(x)的不同，可以把 RMW 对象分为 getAndSet、getAndIncrement 和 getAndAdd 等多种原语
11. 非平凡 RMW 对象的共识数至少为 2。
> 非平凡 ?
> 后面还有若干定理的证明
> 分析了一个绝对关键的问题，原子操作到底支持多少个并发度(concensus number ?)

并发计算中有没有
一种并发对象，可以用来构建其它所有无等待的并发对象？上面已经给出了部分
答案，可以用 **compareAndSet 原语和原子寄存器构建任意并发对象的无等待实现**.


> 无锁通用构造:
> 不知道在说什么东西，为什么又是需要使用链表，关于wait free 和 lock free 的论证

无等待通用构造:由无锁通用构造与 announce 数组构成。
> 好的，我心态崩溃了!



# 6
在性能方面，主要有两个重要指标：**延迟**和**吞吐率**。延迟是一个方法从调用
开始到结束所经历的时间；吞吐率是在单位时间内完成的方法调用的个数。并发
系统一般更关心吞吐率指标

影响锁的性能主要有两个因素，
1. 一是热点竞争造成的性能损失，多个线程申
请同一把锁，在竞争中相互干扰，这些无用功和开销会导致共享锁的性能下降，
应尽量避免；
2. 另一个因素来自**锁本身的顺序特性**，即使在没有热点竞争的情况下，
获得锁的线程也只能顺序访问临界区。这个顺序瓶颈因素对性能的影响是无法避
免的。在设计锁时，主要考察热点竞争的根源以及如何消除热点竞争的影响

1. TAS 锁:
  1. lock : getAndSet(true) 只要返回为true 就GG 如果两个processor 同时getAndSet(true) 显然不可能同时返回为true !
  2. unlock setFalse

2. TTAS 锁: 改进在上锁的时候不要在使用 getAndSet 如果上锁了就不去，知道可能了getAndSe
  1. 上锁问题不大
  2. unlock : 所有cache 中间的内容全部作废，需要lock的全部需要进入到getAndSet中间，全部锁住总线。

> （cache coherence protocol）角度解释: getAndSet会锁住总线。unlock invalid 也会导致总线被锁。

3. 回退锁（Backoff Lock）: 处理方法类似于网络中间的方法。

4. 队列锁（Queue Lock）
    1. 有效解决争用的办法仍然是将单个锁变量扩充为锁数组或者队列，以资源换效率
    2. unlock : 清空下一个单元的内容
    3. lock : getSlot 然后等待自己的内容
    4. 实现等待的数据总是自己的cache 
    5. 相对于TTAS中间的，关键的改动是unlock 只是修改了一个项目，所以只有一个processor 会去锁住总线!

5. CLH 队列锁
    1. 其链接是隐式的，由线程的局部变量和 Qnode 结点共同构成
    2. Qnode 结点记录了锁的状态。如果其状态域为 true，则相应的线程要么已经获得锁，要么正在等待锁；如果该域为 false，
则相应的线程已经释放了锁
    3. *Alock 锁的空间开销大，同步 L 个不同的锁对象需要 O(LN)大小的空间。CLH 锁只需要 O(L+N)。其中，L 是锁的个数，N 是线程个数* 不能理解啊!
        1. *而且并不清楚其是如何回收的*
        2. 应该是上锁的时候，就会添加一个新的节点。unlock 的时候，Mynode = pred 导致内存释放

    4. 原理: 第一进入，后者阻塞，类似于队列，然后依次打开
    5. 问题: 忙等待NUMA其他节点的Qnode很难受!
    6. 链表是隐式的:

6. MCS
    1. 忙等待自己的节点的内容，其他其前面的节点来修改。
        1. 和LHS不同的地方在于，LHS总是在询问前面的节点内容

7. 超时锁
    1. 和LHS 类似，处理等待超时的问题

> 这些问题显然可以通过google 解决
