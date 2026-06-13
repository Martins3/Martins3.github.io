# Appendix

## 1. 概述

Perfbook 附录 A 是全书内容的重要补充，涵盖四个主要部分：

1. **Important Questions** (A.1): 以问答形式澄清 SMP 编程中的七个核心概念问题
2. **Toy RCU Implementations** (A.2): 从简单到复杂逐步构建 RCU 的玩具实现，揭示 RCU 的设计原理
3. **Why Memory Barriers?** (A.3): 从 CPU 缓存、MESI 协议、Store Buffer、Invalidate Queue 等硬件机制出发，解释内存屏障为何必要
4. **Style Guide** (A.4): perfbook 的 LaTeX 排版和编码风格规范

本章的核心价值在于：它不只给出结论，而是通过**递进式实现**和**硬件机制剖析**，让读者真正理解"为什么"，而非仅仅记住"怎么做"。

---

## 2. A.1 Important Questions

本节以七个问题引导读者思考并行编程的本质。

### 2.1 Why Aren't Parallel Programs Always Faster?

核心答案：**并行执行需要通信，而通信不是免费的**。这个问题在第 2 章（Hardware and its Habits）和第 6 章（Partitioning and Synchronization Design）有更深入讨论。本章提醒读者：并行只是众多优化手段之一，有些场景下其他优化（如更好的缓存局部性、算法改进）可能带来更大收益。

### 2.2 Why Not Remove Locking?

虽然无锁（lockless）算法在某些场景下表现优异（如第 14.2 节讨论的 Non-Blocking Synchronization），但**无锁不保证性能和可扩展性**。一个典型反例是原子递增操作在 x86 上的可扩展性极差（多个 CPU 竞争同一缓存行）。

更一般地，**算法越复杂，将锁定与精选的无锁技术结合的优势就越大**。这一观点对当前流行的"锁是坏东西"这种简单口号式认知是一个很好的纠偏。作者的结论是：对并行编程采取 sound-bite（口号式）方法通常不会有好结果。

### 2.3 What Time Is It?

多核系统上的时间是一个复杂话题。读取时间戳涉及多个不确定因素：

- 读取硬件时钟可能需要跨核甚至跨 Socket
- 读取后还有格式转换、NTP 调整等计算
- 线程可能被中断或抢占
- 从读时间到使用时间之间还有代码执行

Linux 内核提供了多种时钟源，各有取舍：

| 时钟源 | 速度 | 精度 | 排序保证 |
|--------|------|------|----------|
| jiffies | 极快 | 毫秒级 | 弱 |
| x86 HPET | 慢 | 高 | 无特别 |
| x86 TSC | 较快 | 高 | 需要内存屏障 |

一个有趣的发现是：`clock_gettime(CLOCK_REALTIME)` 和紧接着的 `clock_gettime(CLOCK_MONOTONIC)` 之间可能出现负偏差（即前者比后者还小），这是因为 NTP 调整或读取过程中的并发干扰。这提醒我们：**时间不是理所当然的确定量**。

FIXME 真的是这样的吗？

### 2.4 What Does "After" Mean?

这是本节最直观也最有深度的部分。作者通过一个 Producer-Consumer 实验来展示"after"的复杂性：

Producer 循环执行：

```c
ss.t = dgettimeofday();
ss.a = ss.c + 1;
ss.b = ss.a + 1;
ss.c = ss.b + 1;
```

Consumer 循环执行：

```c
curssc.tc = dgettimeofday();
curssc.t = ss.t;
curssc.a = ss.a;
curssc.b = ss.b;
curssc.c = ss.c;
```

直觉上，consumer 的时间戳 `tc` 应该总是晚于 producer 的时间戳 `t`，因为 consumer 是在 producer 写入之后才读取的。但实验结果显示了**时间倒流**现象：delta（`tc - t`）可以为负，甚至达到数十微秒。

原因分析：

1. Consumer 获取时间戳 `tc`
2. Consumer 被抢占，任意长时间过去
3. Producer 获取时间戳 `t`
4. Consumer 恢复运行，读取 producer 的时间戳 `t`

此时 `t` 可以任意晚于 `tc`，造成"时间倒流"的假象。这个问题的根源在于**没有同步机制保证操作的原子性和顺序性**。

解决方案是使用锁：在 producer 读取时间戳前加锁，在 consumer 读取时间戳前也加锁。这样两个临界区互斥执行，consumer 的时间戳一定在 producer 的临界区结束之后获取。加锁后的实验不再出现负 delta，只有较大的正间隔（这是因为 consumer 可能被长时间抢占或延迟）。

**核心启示**：在并行程序中，"after"的真正含义是**操作的排序（ordering）**，而非物理时间的流逝。如果你发现自己纠结于"这段代码之前/之后发生了什么"，这通常是一个信号——你应该更好地使用标准同步原语，让它们替你操心。

### 2.5 How Much Ordering Is Needed?

本节讨论强排序与弱排序之间的权衡。一个常见误区是：要么全程强排序（性能差），要么全程弱排序（bug 多）。作者提供了几个判断标准：

1. **数据定义在哪里？** 跟踪外部世界状态的数据通常可以使用弱排序，因为光速延迟已经导致系统内部状态滞后于外部。但银行账户余额这种内部定义的数据需要强排序。

2. **数据是否被一致地使用？** 如果在持锁期间计算的数据在释放锁后使用，那这个值本身就变成了近似值，暗示可能使用更弱的排序。

3. **问题是否可分区？** 通过分区（partitioning）可以将问题分解为更小的独立部分，每个部分内部使用强排序，部分之间减少同步。

4. **都不行？** 如果以上方法都不适用，才考虑第 9 章和第 15 章的高级同步技术。

### 2.6 What is the Difference Between "Concurrent" and "Parallel"?

作者整理了三种常见视角：

**视角一（数据依赖）**：Parallel = 数据并行（data parallel），各分区完全独立，无需通信；Concurrent = 其他一切，可能有紧耦合的锁、事务等同步。

**视角二（调度器）**：并行程序因为无依赖，可以在最简单的调度器上高效运行（甚至可以被任意切分和交错到单处理器上）；并发程序可能需要调度器的高度 sophistication。

**视角三（逻辑 vs 物理）**：Concurrency 是源代码的逻辑 manifestation；Parallelism 是在实际硬件上运行的物理 manifestation。但这个视角在实践中会产生很多悖论（比如一个程序在单核上是 concurrent，多核上就是 parallel 吗？）。

作者的务实态度是：**没有一种人类视角能对抗客观宇宙。在适用的地方使用某种视角，不适用的地方忽略它**。

### 2.7 Why Is Software Buggy?

简短回答：因为人都会犯错。自动化代码生成也不解决问题，因为代码生成器也是人写的。最大的挑战往往是弄清楚软件**应该做什么**——而这个任务至今难以自动化。

尽管如此，自动化（编译器、验证工具）仍是减少 bug 的重要手段。

---

## 3. A.2 Toy RCU Implementations

这是附录 A 中篇幅最大、技术含量最高的部分。作者通过一系列逐步改进的玩具实现，展示了 RCU 的设计空间和权衡。

### 3.1 Lock-Based RCU

最简单实现：`rcu_read_lock()` 获取全局锁，`rcu_read_unlock()` 释放，`synchronize_rcu()` 获取再释放。

```c
static void rcu_read_lock(void)
{
    spin_lock(&rcu_gp_lock);
}

static void rcu_read_unlock(void)
{
    spin_unlock(&rcu_gp_lock);
}

void synchronize_rcu(void)
{
    spin_lock(&rcu_gp_lock);
    spin_unlock(&rcu_gp_lock);
}
```

优点：正确实现 RCU 语义，几乎可在任何用户态环境实现。
缺点：
- 只有一个 reader 能进入临界区，完全丧失 RCU 的读并发优势
- 读端开销巨大（100ns 到 17us）
- `rcu_read_lock()` 可能参与死锁循环
- 不支持嵌套
- 串行化 grace period

### 3.2 Per-Thread Lock-Based RCU

改进：`rcu_read_lock()` 获取当前线程的锁，`synchronize_rcu()` 依次获取并释放每个线程的锁。

```c
static void rcu_read_lock(void)
{
    spin_lock(&rcu_perthread_locks[tid()]);
}

static void rcu_read_unlock(void)
{
    spin_unlock(&rcu_perthread_locks[tid()]);
}

static void rcu_perthread_synchronize(void)
{
    for (int i = 0; i < nthreads; i++) {
        spin_lock(&rcu_perthread_locks[i]);
        spin_unlock(&rcu_perthread_locks[i]);
    }
}
```

优点：允许读并发；避免全局锁的死锁；读端开销固定（约 140ns）。
缺点：更新端随线程数线性增长（单 CPU 600ns，64 CPU 超 100us）；不支持嵌套；串行化 grace period。

### 3.3 Simple Counter-Based RCU

使用全局引用计数器 `rcu_refcnt`。`rcu_read_lock()` 原子递增，`rcu_read_unlock()` 原子递减，`synchronize_rcu()` 等待计数归零。

优点：允许并行读和嵌套；`rcu_read_lock()` 不死锁（不阻塞）。
缺点：原子操作开销大；全局计数器导致严重缓存竞争；大量 reader 可能饿死 updater。

### 3.4 Starvation-Free Counter-Based RCU

关键改进：使用**一对全局引用计数器** `rcu_refcnt[2]` 和全局索引 `rcu_idx`。

`synchronize_rcu()` 翻转 `rcu_idx`（`rcu_idx = !rcu_idx`），新 reader 使用另一个计数器，然后等待旧计数器归零。这确保了 updater 不会永远被阻塞。

但原子操作仍然重，且需要翻转两次（原因在 QuickQuiz 中详细解释：如果只翻转一次，在特定时序下可能出现 grace period 在 reader 开始之后结束的错误）。

### 3.5 Scalable Single-Counter RCU (Per-Thread Counter)

关键改进：使用**每线程计数器** `rcu_nesting[]`，读端使用非原子操作。

```c
static void rcu_read_lock(void)
{
    __thread_var(rcu_nesting[tid()])++;
    barrier();
}

static void rcu_read_unlock(void)
{
    barrier();
    __thread_var(rcu_nesting[tid()])--;
}
```

优点：读端开销极低且固定（无原子操作、无缓存竞争）。
缺点：`synchronize_rcu()` 仍需扫描所有线程；串行化 grace period；需要环境支持 per-thread 变量和线程枚举。

### 3.6 Scalable Counter-Based RCU With Shared Grace Periods

改进：允许并发的 `synchronize_rcu()` 调用共享 grace period。通过检查锁获取期间其他线程是否已执行了足够的计数翻转来决定是否需要额外等待。

这样，即使大量线程并发调用 `synchronize_rcu()`，也只需要三次等待计数归零。

### 3.7 RCU Based on Free-Running Counter

使用一个只取偶数值的全局自由运行计数器 `rcu_gp_ctr`。`rcu_read_lock()` 将 `rcu_gp_ctr + 1` 存入每线程变量（奇数表示在临界区内），`rcu_read_unlock()` 将当前 `rcu_gp_ctr` 的值存回（偶数表示不在临界区内）。`synchronize_rcu()` 将全局计数器加 2，然后等待所有线程的每线程值为偶数或大于全局计数器。

读端开销约 63ns，不随 CPU 数变化。但**不支持嵌套**。

### 3.8 Nestable RCU Based on Free-Running Counter

通过预留计数器的低 7 位作为嵌套深度计数，实现可嵌套的版本。`RCU_GP_CTR_BOTTOM_BIT` 是嵌套位之上的单个位，`RCU_GP_CTR_NEST_MASK` 覆盖嵌套区域。

```c
#define RCU_GP_CTR_NEST_MASK    0x7f
#define RCU_GP_CTR_BOTTOM_BIT   0x80
```

读端约 65ns。但仍存在计数器溢出的风险（32 位系统上尤其严重）。

### 3.9 RCU Based on Quiescent States

这是读端性能最高的实现。`rcu_read_lock()` 和 `rcu_read_unlock()` 是**空操作**！

RCU 的读侧临界区范围由**显式静止状态（quiescent state）**近似界定。线程需要定期调用 `rcu_quiescent_state()`，或在进入长休眠时调用 `rcu_thread_offline()` / `rcu_thread_online()`。

```c
static void rcu_read_lock(void) { }
static void rcu_read_unlock(void) { }

static void rcu_quiescent_state(void)
{
    smp_mb();
    __thread_var(rcu_reader_qs_gp) = READ_ONCE(rcu_gp_ctr) + 1;
    smp_mb();
}
```

读端往返开销约 50 皮秒（实际上是因为编译器优化掉了空函数，真实开销约等于单个 `rcu_quiescent_state()` 调用的千分之一）。

Linux 内核的服务器构建实际上就是这么做的：`rcu_read_lock()` 和 `rcu_read_unlock()` 编译为空，真正的开销在上下文切换时隐式报告静止状态。

**限制**：需要线程主动报告静止状态；不适合无法控制系统所有代码的库函数。

### 3.10 理想 RCU 的性质总结

作者列出了 9 条理想性质：

1. 必须有读端原语和 grace period 原语
2. 读端原语开销最小化（避免缓存缺失、原子指令、内存屏障、分支）
3. 读端原语必须是 O(1) 复杂度（支持实时使用）
4. 读端原语可在所有上下文使用（包括嵌套）
5. 读端原语必须无条件成功（无失败返回）
6. RCU 读临界区内允许任何非静止状态操作（包括 I/O）
7. 允许在 RCU 读临界区内更新 RCU 保护的数据结构
8. RCU 原语应与内存分配器设计独立
9. Grace period 不应被在临界区外休眠的线程阻塞

这些性质为我们评估 RCU 实现提供了清晰的框架。

---

## 4. A.3 Why Memory Barriers?

本节从硬件层面回答"为什么会有内存屏障"这个根本问题。

### 4.1 Cache Structure

现代 CPU 比内存快两个数量级以上。CPU 缓存以固定大小的 cache line（通常 64 字节，但文中例子用 256 字节）为单位传输数据。缓存 miss 分为：

- **warm-up miss**: 首次访问
- **capacity miss**: 缓存满了
- **associativity miss**: 哈希冲突（硬件哈希表无链表，直接映射）
- **write miss**: 写前需要使其他 CPU 缓存失效
- **communication miss**: 因其他 CPU 使缓存行失效导致

### 4.2 MESI Protocol

四状态缓存一致性协议：

- **Modified**: 独占且已修改，必须负责写回
- **Exclusive**: 独占但未修改，可直接丢弃
- **Shared**: 可能存在于多个缓存中，写前需协商
- **Invalid**: 空

MESI 通过五种消息协调：Read、Read Response、Invalidate、Invalidate Acknowledge、Read Invalidate、Writeback。

### 4.3 Store Buffers

写操作如果目标 cache line 在其他 CPU 缓存中，需要等待 invalidate 完成，这会导致 CPU 长时间 stall。Store Buffer 的引入允许 CPU 将写操作记入本地 buffer 后继续执行。

但 Store Buffer 带来两个问题：

**问题一：自一致性破坏**

```c
a = 1;
b = a + 1;
assert(b == 2);
```

如果 CPU 将 `a = 1` 写入 store buffer 但 cache line 还未到达，接着读取 `a` 可能读到旧值 0，导致断言失败。解决方案是 **Store Forwarding**：CPU 读操作同时检查 store buffer 和 cache。

**问题二：全局内存序破坏**

```c
/* CPU 0 */
a = 1;
b = 1;

/* CPU 1 */
while (b == 0) continue;
assert(a == 1);
```

`a = 1` 可能滞留在 CPU 0 的 store buffer 中，而 `b = 1` 直接写入 cache（因为 CPU 0 已独占该 cache line）。CPU 1 看到 `b == 1` 后读取 `a`，但 `a` 的更新还在 CPU 0 的 store buffer 中，断言失败。

解决方案：`smp_mb()` 强制 CPU 在后续 store 之前刷空 store buffer。

### 4.4 Invalidate Queues

 invalidate acknowledge 可能延迟很久（目标 CPU 缓存正忙）。引入 Invalidate Queue 后，CPU 收到 invalidate 消息可以立即 ack，将消息排队稍后处理。

但这又破坏了内存屏障：

```c
/* CPU 0 */
a = 1;
smp_mb();
b = 1;

/* CPU 1 */
while (b == 0) continue;
assert(a == 1);  /* 可能失败！ */
```

CPU 0 的 `smp_mb()` 只标记自己的 store buffer，不影响 CPU 1 的 invalidate queue。CPU 1 可能在处理队列中的 invalidate 之前就读取 `a`。

解决方案是双向内存屏障：CPU 执行 `smp_mb()` 时标记自己的 store buffer 和 invalidate queue；后续 load 必须等待所有标记的 invalidate 处理完毕。

### 4.5 Read/Write Memory Barriers

- **smp_rmb()**: 只标记 invalidate queue，只排序 load
- **smp_wmb()**: 只标记 store buffer，只排序 store
- **smp_mb()**: 同时标记两者，排序所有访问

### 4.6 内存屏障示例

作者给出了三个经典例子说明内存屏障的局限性：

**Example 1**: CPU 0 的 `smp_wmb()` 不能保证 CPU 2 在看到 CPU 1 的赋值之前看到 CPU 0 的赋值，因为 CPU 0 和 CPU 1 的消息队列独立。

**Example 2**: 类似地，即使 CPU 1 使用了 `smp_mb()`，CPU 2 仍可能先看到 CPU 1 的 store 再看到 CPU 0 的 store。

**Example 3**: 通过成对使用内存屏障（CPU 0 的 `smp_wmb()` 与 CPU 1/2 的 `smp_mb()`，以及 CPU 0 最后的 `smp_mb()`），可以实现跨 CPU 的传递性排序，保证断言不触发。Linux 内核的 `synchronize_rcu()` 就使用了类似算法。

---

## 5. A.4 Style Guide

这是 perfbook 的排版和编码规范，主要涉及：

- 标点符号（英国式引号、Oxford comma、斜体强调）
- NIST 单位符号规范（数值与单位之间用窄空格）
- LaTeX 代码片段提取机制（使用 fancyvrb 包和 fcvextract.pl 脚本）
- 交叉引用使用 cleveref 包
- 破折号、连字符、省略号的规范用法

对理解并行编程技术本身帮助不大，但对贡献 perfbook 很有参考价值。

---

## 6. 结合 Linux 内核源码

### 6.1 RCU 实现

Linux 内核的 RCU 实现远玩具实现复杂得多。关键文件：

- `kernel/rcu/tree.c`: Tree RCU 核心实现
- `kernel/rcu/tree.h`: 数据结构和宏定义
- `kernel/rcu/update.c`: 更新端和 grace period 管理
- `include/linux/rcupdate.h`: 用户可见的 RCU API

内核中 `rcu_read_lock()` 的实现（`include/linux/rcupdate.h`）：

```c
static inline void __rcu_read_lock(void)
{
    preempt_disable();
}
```

在非抢占内核中，这仅仅是 `preempt_disable()`——禁止抢占，几乎零开销。这正是 quiescent-state RCU 思想的体现：读端无需原子操作或内存屏障，靠调度器隐式报告静止状态。

内核的 `synchronize_rcu()` 使用层次化树结构追踪静止状态，`rcu_node` 的 `qsmask` 位图表示哪些 CPU 还未报告 QS。这比玩具实现中逐个扫描线程高效得多，且支持 grace period 共享。

### 6.2 Memory Barrier 实现

x86 的 `smp_mb()` 实现（`arch/x86/include/asm/barrier.h`）：

```c
#define __smp_mb() asm volatile("lock addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
```

使用 `lock` 前缀的 dummy 加法指令实现全内存屏障。x86 TSO 模型本身已经保证 store-store、load-load、load-store 顺序，只需要处理 store-load 重排序，但 `lock` 指令提供完整的屏障语义。

ARM64 则完全不同：

```c
#define __mb()    dsb(sy)
#define __rmb()   dsb(ld)
#define __wmb()   dsb(st)
```

使用显式的 barrier 指令，且需要为 `smp_load_acquire` / `smp_store_release` 使用 `ldar` / `stlr` 指令。这解释了为什么同样的 RCU 代码在 x86 上可能"看起来工作"而在 ARM 上可能崩溃——对弱排序架构的理解不能停留在"x86 上测试通过了"。

### 6.3 关于"After"的修复

内核中类似的 timekeeping 和 snapshot 问题通常使用 seqlock（`include/linux/seqlock.h`）解决：读端记录序列号，读完后检查序列号是否变化，如果变化则重试。这比简单加锁更高效，因为读端通常是无锁的。

---

## 7. 与我的笔记关联分析

我的并发编程笔记位于 `~/data/vn/docs/concurrent/`，附录 A 的内容解决了其中多个长期疑问：

### 7.1 Memory Model 笔记

笔记中记录了诸多问题：
- "能不能稳定复现 memory model 的效果" —— 附录 A.3 从硬件机制（store buffer、invalidate queue）给出了**可复现的根本原因**
- "smp_mb 的使用位置和实现方式是什么" —— 附录 A.3 详细解释了 smp_mb/smp_rmb/smp_wmb 的硬件语义
- "既然存在 smp_load_acquire，为什么还需要 smp_mb" —— A.3 的 Example 1/2/3 展示了单向屏障不足以保证跨 CPU 传递性排序的场景
- "x86 为什么只需要三个 fence" —— A.3 指出 x86 TSO 只存在 store-load 重排序，但 full memory barrier 仍需要 lock 指令
- "ARM 的 dsb/dmb/dsb 区别" —— A.3 解释了 read/write/full barrier 的硬件实现差异

### 7.2 Cache Coherency 笔记

笔记中问：
- "cache coherence 到底提供的保证是什么" —— A.3 明确回答：MESI 保证所有 CPU 对同一内存位置达成一致，但不保证**不同位置**的访问顺序
- "中断的时候需要把 write buffer 都 flush 掉吗" —— A.3 的 store buffer 和 invalidate queue 分析说明：单 CPU 内部的中断不需要 flush（因为单 CPU 看到自己的操作有序），但涉及多 CPU 的 DMA、IPI 等必须考虑缓存一致性

### 7.3 RCU 笔记

我的 RCU 笔记（`rcu/relearn-with-ai.md`）详细分析了内核 RCU 的树结构、stall 检测、grace period 机制。附录 A.2 的 toy RCU 实现恰好提供了**自下而上的理解路径**：从全局锁到每线程锁，从计数器到自由运行计数器，再到静止状态——这与内核从简单到复杂的演进历史一致（Linux 2.4 使用类似 per-thread lock 的实现，现代内核使用树 RCU 和 QS-based 优化）。

### 7.4 尚未完全解决的问题

附录 A 虽然解答了很多问题，但我的笔记中仍有一些未被直接回答的疑问：
- GPU 中的 memory model 是做什么的？（A.3 完全未涉及 GPU）
- RISC-V 的 memory model 细节（A.3 主要讨论通用原理，未深入具体架构）
- `try_to_wake_up` 中大量使用 `smp_mb__after_spinlock` 的具体场景分析（需要结合内核调度器源码）
- seqlock 与 locking 在 timekeeping 中的权衡（perfbook 其他章节有涉及）

---

## 8. 代码说明与测试

本目录包含四个可编译运行的演示程序：

| 程序 | 说明 | 编译 |
|------|------|------|
| `after_demo.c` | 演示无锁条件下时间倒流 | `make after_demo.out` |
| `after_demo_locked.c` | 使用 mutex 修复时间倒流 | `make after_demo_locked.out` |
| `rcu_demo.c` | 三种 toy RCU 实现对比 | `make rcu_demo.out` |
| `mb_demo.c` | 内存屏障必要性演示 | `make mb_demo.out` |

### 8.1 after_demo 测试

运行 `./after_demo.out`，可以看到输出中出现负的 delta 值（括号中的微秒数），如：

```
1/0: ... (-1.192) 34 35 36
```

这表示 consumer 获取自己的时间戳 `tc` 比 producer 的时间戳 `t` 早了约 1.2 微秒——**时间倒流**。

运行 `./after_demo_locked.out`，所有 delta 都是非负的，证明锁确实消除了时序异常。

### 8.2 rcu_demo 测试

运行 `./rcu_demo.out`，三种 RCU 实现都通过 100 次更新迭代和 4 个 reader 的测试。注意这只是一个功能测试，没有测量性能。真正的性能差异需要在多核高并发压力下才能显现。

### 8.3 mb_demo 测试

运行 `./mb_demo.out 100000`，在 x86 TSO 上"no barrier"和"with barrier"通常都不会失败，因为 x86 本身不允许 store-store 重排序。但这个代码在 ARM 或其他弱排序架构上，"no barrier"版本很可能观察到失败。程序使用 C11 `memory_order_release` / `memory_order_acquire` 来演示正确的配对用法。

---

## 9. 总结

Perfbook 附录 A 是全书的"压舱石"：

- **Important Questions** 纠偏了并行编程中的常见误解
- **Toy RCU** 以递进式实现揭示了生产级 RCU 的设计动机
- **Why Memory Barriers** 从硬件根源解释了同步原语为何必须如此设计

对于内核开发者而言，A.2 和 A.3 是最有价值的部分：理解了 store buffer 和 invalidate queue，就能明白为什么 `smp_mb()` 在 lock 之外仍然是必需的；理解了 toy RCU 的演进，就能更好地阅读 `kernel/rcu/tree.c` 中复杂的 grace period 算法。

附录 A 没有解决所有问题（如 GPU memory model、具体架构差异），但它建立了一个坚实的底层认知框架，使得后续阅读其他章节和内核源码时有据可依。

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
