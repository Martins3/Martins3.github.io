# Locking

## 一、章节核心内容概述

FIXME 这个似乎以为自己知道了，但是其实一直都理解啊

Perfbook 第7章以 "Locking: Villain or Hero?" 为核心议题，系统性地探讨了锁机制在并发编程中的双重角色。作者开宗明义地指出：锁在学术界常被视为引发死锁、车队效应、饥饿、不公平和数据竞争的罪魁祸首；但在工业界，锁却是最广泛使用的同步原语，是支撑生产级共享内存并行软件的"工作马"。这种矛盾源于多个因素：许多锁的"罪行"都有成熟的工程解决方案；某些问题只在设计不良的代码高争用场景下才会暴露；通过与其他同步机制（如 RCU、引用计数、危险指针等）配合使用，可以有效规避锁的缺陷；以及大量生产代码是在保密环境下开发的，导致最佳实践未能广泛传播。

本章的结构非常清晰，分为四大部分：首先是"Staying Alive"，即如何在锁的使用中避免死锁、活锁、饥饿等致命问题；其次是"Types of Locks"，介绍独占锁、读写锁、多角色锁和作用域锁；接着是"Locking Implementation Issues"，深入锁的实现原理；最后是"Locking: Hero or Villain?"，从不同应用场景的视角审视锁的适用性。此外，还有一个独立的节 "Lock-Based Existence Guarantees"，讨论如何通过锁来保证动态数据结构的生存期。

## 二、Staying Alive：避免死锁与活锁

### 2.1 死锁的本质与避免策略

死锁的定义是：一组线程中的每个成员都持有至少一个锁，同时又在等待该组中另一个成员持有的锁。值得注意的是，即使是单线程也可能死锁——当一个线程试图获取它已经持有的非递归锁时。作者用有向图来形式化描述死锁：节点是线程和锁，从锁指向线程的箭头表示线程持有该锁，从线程指向锁的箭头表示线程正在等待该锁。死锁场景必然包含至少一个死锁循环。

本章提出了七种避免死锁的策略，构成了一个完整的工具箱：

**锁层次结构 (Locking Hierarchies)**：给所有锁定义一个全局顺序，禁止逆序获取。对于同类型锁的多个实例（如搜索树中每个节点的锁），传统做法是按锁地址的升序获取。Linux 内核的 lockdep 工具正是用来强制检查锁层次结构的。

**局部锁层次结构 (Local Locking Hierarchies)**：这对库函数设计至关重要。库函数无法预知调用者的锁层次，因此必须遵循"黄金法则"——在调用未知代码（如回调函数）之前释放所有锁。qsort() 的比较函数就是一个经典例子：如果 qsort() 在持有内部锁 C 时调用比较函数，而比较函数又试图获取调用者持有的锁 B，就会发生死锁。通过在调用比较函数前释放锁 C，qsort() 就避免了参与任何死锁循环。

但作者也警告了这种策略的危险：释放锁后，被保护的状态可能发生改变。本章给出的递归树迭代器示例（`rec_tree_itr.c`）展示了这个风险：树锁在回调前释放，回调返回后重新获取，但此时当前节点的子节点数量可能已变，`realloc()` 可能已移动 `children` 数组，导致迭代器访问无效内存。

**分层锁层次结构 (Layered Locking Hierarchies)**：当无法释放所有锁再调用未知代码时，可以构造多层锁层次。例如，锁 A 和 B 在第一层，锁 C 在第二层，锁 D 在第三层。这种设计允许在持有列表迭代器锁的同时调用可能获取其他锁的回调，只要这些锁位于更低的层次。

**时间锁层次结构 (Temporal Locking Hierarchies)**：通过延迟获取某个冲突锁来避免死锁。Linux 内核 RCU 的 `call_rcu()` 就是一个例子：它在被调度器调用时（调度器持有其锁）不能安全地唤醒调度器，因此将启动新 grace period 的工作推迟到定时器中断或调度器时钟中断中执行，那时没有调度器锁被持有。

**条件锁 (Conditional Locking)**：当无法定义锁层次时（如网络协议栈中包可能双向流动），使用 `spin_trylock()` 尝试获取锁。如果失败，释放已持有的锁，按正确顺序重新获取。但作者强调，`layer_2_processing()` 必须是幂等的，因为路由决策改变时可能需要重试。同时警告了这种转换可能引入活锁。

**先获取所有需要的锁 (Acquire Needed Locks First)**：一种重要的条件锁特例，在修改共享数据前先获取所有需要的锁。如果无法按顺序获取，就全部释放重试。这与数据库的两阶段锁密切相关。

**单次持锁设计 (Single-Lock-at-a-Time Designs)**：如果问题完全可划分，每个分区一个锁，任何时刻线程只持有一个锁，死锁就不可能发生。

**信号/中断处理器**：在信号处理器中获取锁是危险的。如果某个锁可能在信号处理器中被获取，那么在线程上下文中获取该锁时必须屏蔽相应信号。Linux 内核允许在中断处理器中获取锁，因此获取任何可能被中断处理器获取的锁时都必须关中断。

### 2.2 活锁与饥饿

条件锁的滥用会导致活锁。作者给出了一个对称条件锁的经典例子：两个线程分别持有 lock1 和 lock2，同时尝试条件获取对方的锁，失败后释放自己的锁并重试，形成无限循环。活锁可以被视为一种极端的饥饿——不是单个线程挨饿，而是一组线程集体挨饿。

解决活锁的一个简单有效方法是**指数退避 (Exponential Backoff)**：每次重试前引入指数增长的延迟。这给了竞争线程完成工作并释放锁的时间窗口。但作者也指出，更好的方法是重新审视锁设计，或者结合 Doug Lea 的建议：如果重试次数过多，无条件获取一个全局锁，再无条件获取所有需要的锁。

### 2.3 不公平与低效

**不公平 (Unfairness)** 是饥饿的温和形式。在共享缓存或 NUMA 架构的系统中，当 CPU 0 释放锁时，与 CPU 0 共享互联的 CPU 1 比其他 CPU 更容易获取锁。如果 CPU 0 和 CPU 1 交替持锁，CPU 2-7 就会被持续绕过。这解释了为什么在高度争用场景下，简单的 test-and-set 锁会导致严重的不公平。

**低效 (Inefficiency)** 源于锁本身的开销。锁使用原子指令和内存屏障实现，通常还伴随缓存未命中，开销大约是简单指令的两个数量级。如果用一个锁保护单条指令，就需要一百个 CPU 才能追上一个无锁 CPU 的性能。本章用"锯木屑"作比喻：就像锯子会把一部分木头变成锯末一样，锁会把一部分执行时间浪费在同步开销上。过粗的粒度限制扩展性，过细的粒度导致同步开销过大。Rust 语言通过编译器可见的锁/数据关联，试图在编译期防止无锁访问受保护数据的错误。

## 三、锁的类型

### 3.1 独占锁

独占锁是最基本的锁类型。本章特别提到了一个有趣的话题：空临界区（获取锁后立即释放）是否有意义？答案是肯定的——独占锁的语义包含两个部分：数据保护和消息传递。释放锁会通知正在等待该锁的获取者。2.4 Linux 内核的 `brlock`（大读者锁）就是利用空临界区来近似 RCU 语义的历史案例。

独占锁的获取策略也多种多样：严格 FIFO、近似 FIFO、按优先级 FIFO、随机、不公平等。更强的顺序保证通常意味着更高的开销，这解释了为什么生产环境中有如此多样的锁实现。

### 3.2 读者-写者锁

读者-写者锁允许多个读者并发持有，或单个写者独占持有。理论上这对读多写少的数据应该有很好的扩展性。但经典的实现使用原子操作操纵计数器和标志，开销同样很大。一种更利于读者的设计是使用 per-thread 独占锁：读者只获取自己的锁，写者获取所有锁。没有写者时，读者只有原子指令和内存屏障开销，没有缓存未命中。但写者需要获取所有锁，成本随线程数线性增长。

读者-写者锁还面临公平性抉择：如果大量读者持锁，等待的写者是否应该被允许无限期阻塞？Linux 内核的 `qrwlock`（queued rwlock）使用队列来保证一定的公平性。

### 3.3 多角色锁与 VAX/VMS DLM

作者介绍了 VAX/VMS 分布式锁管理器 (DLM) 的六模式策略：Null、并发读、并发写、保护读、保护写、独占。这比简单的独占锁和读者-写者锁提供了更细粒度的并发控制。现代数据库管理系统甚至有超过三十种锁模式。

### 3.4 作用域锁 (Scoped Locking / RAII)

C++ 的 RAII 模式将锁获取和释放与对象的作用域绑定，构造函数获取锁，析构函数释放锁。这消除了在每个退出路径上手动释放锁的负担，但也带来了限制：难以实现锁获取和释放分离的封装（如迭代器在 start 中获锁、在 end 中放锁），也禁止了重叠的临界区。C++17 的 `unique_lock` 提供了逃逸机制来应对这些限制。Linux 内核 RCU 的 `force_quiescent_state()` 函数展示了条件锁锦标赛模式——CPU 条件获取叶子 `rcu_node` 的锁，成功后尝试获取父节点锁，同时检查全局标志判断是否有其他 CPU 已经"获胜"。这种模式很难用严格的 RAII 表达。

## 四、锁的实现问题

### 4.1 基于原子交换的锁

本章给出了一个教学用的 test-and-set 锁实现（`xchglock.c`）：

```c
typedef int xchglock_t;

void xchg_lock(xchglock_t *xp)
{
    while (xchg(xp, 1) == 1) {
        while (READ_ONCE(*xp) == 1)
            continue;
    }
}

void xchg_unlock(xchglock_t *xp)
{
    (void)xchg(xp, 0);
}
```

内层循环（自旋在本地缓存的锁值上）的设计至关重要：如果所有竞争者都在外循环反复做原子交换，会导致包含锁的缓存行在 CPU 之间来回"乒乓"，给互联带来巨大压力。内层循环让线程在自己的缓存中自旋，几乎不产生互联流量。

### 4.2 Linux 内核的 Queued Spinlock (qspinlock)

test-and-set 锁和 ticket lock 在高争用下都有性能问题：释放锁时需要更新单一内存位置，必须使所有等待 CPU 的只读缓存副本失效，CPU 越多开销越大。Linux 内核目前使用 queued spinlock（`kernel/locking/qspinlock.c`），它基于经典的 MCS 锁（Mellor-Crummey and Scott）。

MCS 锁的核心思想是：每个等待线程在自己的队列节点上自旋，而不是在全局锁变量上自旋。释放锁时，锁持有者只需使下一个等待者节点中的 `locked` 字段失效，极大减少了缓存一致性流量。

Linux 的 qspinlock 对 MCS 做了精巧的压缩：传统 MCS 需要 8 字节 tail 指针 + 每个节点 8 字节 next 指针，而 qspinlock 将整个状态压缩到 32 位：1 字节锁值、3 字节 tail（编码 CPU 号和嵌套层级）。它还为每个 CPU 预分配了最多 4 个队列节点（对应 task、softirq、hardirq、nmi 四种嵌套上下文）。

qspinlock 的状态机非常精巧：
- `(0,0,0)` 空闲 -> `(0,0,1)` 无竞争获取
- `(0,1,1)` pending 状态 -> `(0,1,0)` -> `(0,0,1)` 快速交接
- `(n,x,y)` 队列非空 -> MCS 队列等待

qspinlock 还结合了乐观自旋（optimistic spinning）：在真正入队前，先尝试设置 pending 位并自旋等待当前持有者释放锁，如果持有者很快释放就避免了昂贵的队列操作。

### 4.3 其他实现问题

本章还讨论了 ticket lock（严格 FIFO，Linux 曾使用）、优先级反转及其解决方案（优先级继承、禁止持锁时抢占）、以及 futex（Linux 用户空间快速互斥锁）等主题。

## 五、基于锁的存在性保证

这是 `locking-existence.tex` 的主题。动态分配的数据结构在并发环境下如何安全释放？一个直接的方法是将锁放在元素内部（per-element locking），但这有一个微妙的竞态条件：

```c
p = hashtable[b];
spin_lock(&p->lock);    /* BUG: p 可能已被释放 */
```

修复方法是使用全局/桶级别的锁：

```c
sp = &locktable[b];
spin_lock(sp);          /* 先锁桶 */
p = hashtable[b];       /* 再拿指针，保证存在 */
```

这种锁提供了显式的存在性保证：只要持有该锁，对应元素就一定存在。但对于跨越多个哈希表或树、图等复杂结构的数据元素，这种方法会失效。更复杂的解决方案（如 RCU、引用计数、危险指针）将在第13章介绍。

## 六、Linux 源码对应分析

Linux 内核的锁子系统位于 `kernel/locking/`，包含以下核心文件：

**`spinlock.c`**：实现了 `_raw_spin_lock()` 等函数。在 `BUILD_LOCK_OPS` 宏中可以看到一个经典模式：`preempt_disable()` 后尝试 `trylock`，失败则 `preempt_enable()` 并自旋。`raw_spin_lock_irqsave()` 还会保存/恢复中断状态。这对应了本章关于信号/中断处理器中锁安全的讨论——在可能和中断处理器共享的锁上，必须关中断。

**`mutex.c`**：实现了可睡眠的互斥锁。与自旋锁不同，mutex 在无法获取时会将线程置于睡眠状态。`__mutex_trylock_fast()` 使用 `atomic_long_try_cmpxchg_acquire()` 进行乐观的快速路径尝试。`mutex` 还实现了自适应自旋（`CONFIG_MUTEX_SPIN_ON_OWNER`）：在真正睡眠前，先自旋等待当前锁持有者释放锁，如果持有者在另一个 CPU 上运行且即将释放锁，这比立即睡眠更高效。

**`qspinlock.c` / `mcs_spinlock.h`**：如前所述，qspinlock 是 Linux 内核当前使用的自旋锁实现。`mcs_spinlock.h` 中的 `mcs_spin_lock()` 使用 `xchg()` 将当前节点链接到队列尾部，如果队列非空则在 `node->locked` 上自旋；`mcs_spin_unlock()` 则将锁传递给下一个等待者。`arch_mcs_spin_lock_contended()` 在 x86 上可能是简单的 `cpu_relax()` 循环，在 ARM64 上可能使用 `wfe/sev` 事件机制来节能等待。

**`lockdep.c`**：lock dependency checker，用于在运行时检测锁层次违反和潜在死锁。这是本章多次提到的核心工具。

**`locktorture.c`**：锁的 torture test，用于验证锁实现在各种压力场景下的正确性。

## 七、用户疑问分析

用户的并发编程笔记位于 `~/data/vn/docs/concurrent/`，其中与锁和本章内容相关的疑问包括：

### 已解决的疑问

1. **"观察各个系统中的 spin lock 和 mutex 才是重点"**：本章及配套代码直接回应了这一需求。我们编写的 `xchglock_demo.c` 展示了自旋锁的核心原理，并结合 Linux `kernel/locking/qspinlock.c` 和 `mutex.c` 分析了真实实现。

2. **"锁是靠什么保证这些的 (memory barrier)？是靠 lock 的代码实现，还是靠硬件实现的？"**：本章的 `xchglock.c` 示例和 Linux qspinlock 源码明确回答了这一点。锁的实现同时依赖两者：硬件提供原子指令（如 x86 的 `xchg`、`lock cmpxchg`），但仅靠硬件原子性不足以保证内存顺序。锁的代码必须正确使用内存屏障（acquire/release 语义）来防止编译器和 CPU 重排序。例如 qspinlock 中 `smp_wmb()`、`smp_cond_load_acquire()`、`cmpxchg_release()` 等的使用，都是软件层面对内存顺序的显式控制。

3. **"cache coherence 和 memory model 的关系"**：虽然本章没有直接深入 memory model，但在讨论锁的不公平性和低效时，反复涉及缓存一致性协议（如缓存行失效、缓存行乒乓、false sharing）。锁的公平性和扩展性本质上受缓存一致性机制制约。

4. **"各种数据结构的 wait free 转化的基本方法是什么？"**：本章明确指出了锁的局限，并引导读者关注后续章节（第13章的 RCU、引用计数、危险指针等）。

### 未解决或仅部分解决的疑问

1. **"access once / data_race 完全不懂"**：本章没有深入讨论 `READ_ONCE`/`WRITE_ONCE` 或 data race 的形式化定义。这些内容更多在第4章（Hardware and its Habits）和第14章（Advanced Synchronization）中。

2. **"memory barrier 需要斟酌一下" / "总结一个如何使用 memory barrier 的基本方法"**：本章虽然多次提到内存屏障（如 `smp_mb()`、`smp_wmb()`、`smp_cond_load_acquire`），但没有系统性地讲解 memory barrier 的使用方法。这是第14章的核心内容。

3. **"为什么内核有定义了一个 LKMM (Linux Kernel Memory Model)？"**：本章没有涉及 LKMM 的形式化模型。LKMM 是理解内核中复杂同步模式的基础，但属于更高级的主题。

4. **"什么时候应该使用 seqlock 而不是 rcu"**：seqlock 和 RCU 的详细对比在第9章（Deferred Processing）中。

5. **"slab 中的 system_has_freelist_aba 是做什么的？"**：ABA 问题与无锁数据结构密切相关，本章几乎没有涉及。这也是第13章的内容。

6. **"formally verified / 形式化验证指的是什么"**：本章仅在脚注中提到形式化验证工具（如 herd），没有展开。

7. **"DMA 的 cache 一致性是谁处理的"**：与缓存一致性相关，但属于硬件/驱动层面，超出本章范围。

## 八、代码说明与测试

本章配套编写了五个可运行程序：

- **`deadlock_demo.out`**：演示死锁场景、锁层次结构避免死锁、条件锁（trylock+retry）、信号处理器安全、局部锁层次结构（回调前释放锁）。
- **`livelock_demo.out`**：演示活锁（对称条件锁）、指数退避解决方案、不公平性（一个线程因不休眠而大量获取锁）、锁粒度效率对比（粗粒度 vs 无锁分区）。
- **`xchglock_demo.out`**：实现并测试基于 C11 `atomic_exchange` 的教学用自旋锁，与 pthread mutex 做正确性和性能对比。
- **`rwlock_demo.out`**：使用 `pthread_rwlock_t` 和简化的 per-thread 锁方案演示读者-写者锁的行为。
- **`existence_guarantee.out`**：演示基于桶锁的存在性保证，避免 "先拿指针再锁元素" 的经典竞态。

所有程序均通过 `make test` 在本地物理机（Fedora 43, kernel 6.19.13）上编译和运行验证。

## 九、总结

Perfbook 第7章是一章内容极其丰富的"锁使用指南"。它没有把锁当作一个简单的原语来介绍，而是从工程实践的角度，全面审视了锁在真实系统中可能遇到的所有问题及其解决方案。从死锁避免的策略工具箱，到活锁和饥饿的缓解方法，从锁类型的选择，到实现层面的缓存一致性优化，再到库设计中锁层次结构的难题——这些内容对于任何需要编写并发代码的工程师都是必读材料。

然而，本章的定位是"使用锁"而非"理解同步的一切"。对于那些更深层的疑问——如 memory barrier 的精确语义、LKMM 的形式化模型、无锁数据结构的 ABA 问题、RCU 与 seqlock 的取舍——本章只是给出了方向指引，真正的答案分散在本书的其他章节中。阅读完本章后，最自然的下一步是进入第9章（Deferred Processing）学习 RCU 和引用计数，以及第13章（Data Structures）和第14章（Advanced Synchronization）来补全无锁编程和 memory model 的知识拼图。

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
