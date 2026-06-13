# Chapter 14: Advanced Synchronization 详细分析

## 1. 章节概述

第 14 章 "Advanced Synchronization" 分为两大部分：

1. **无锁同步 (Lockless Synchronization)**：涵盖避免锁的技术、非阻塞同步 (Non-Blocking Synchronization, NBS) 的理论分类与实用算法。
2. **并行实时计算 (Parallel Real-Time Computing)**：从实时系统的定义出发，讨论 Linux 内核如何支持微秒级甚至亚微秒级的实时响应。

作者 Paul E. McKenney 的核心观点是：**不要过早追求无锁算法**。在考虑 NBS 之前，应该充分运用分区 (partitioning)、批处理 (batching) 和成熟的延迟处理 API (如 RCU)。只有当这些手段无法满足极端需求时，才应该诉诸本章介绍的高级技术。

---

## 2. 避免锁 (Avoiding Locks)

虽然锁是并行编程的主力军，但在很多场景下无锁技术能显著提升性能、可扩展性和实时响应。本章回顾了书中此前介绍过的多种无锁技术：

- 统计计数器 (statistical counters)：连原子操作和内存屏障都不需要。
- 资源分配器缓存 (resource allocator caches) 的快速路径。
- 数据所有权 (Data Ownership) 技术。
- 引用计数、危险指针 (hazard pointers) 和 RCU。
- 各种数据结构查找路径的免锁实现。

关键忠告是：**无锁技术应该隐藏在良好定义的 API 后面** (如 `inc_count()`, `rcu_read_lock()` 等)。无约束地使用无锁原语是制造难以调试的 bug 的捷径。

---

## 3. 非阻塞同步 (Non-Blocking Synchronization)

### 3.1 NBS 的八个前进进度保证级别
<!-- 4fd46a74-8f7a-4d66-9712-64633cd037c6 -->

NBS 描述的是八类具有不同**前进进度保证 (forward-progress guarantee)** 的线性化 (linearizable) 算法：

| 级别 | 名称 | 保证内容 |
|------|------|----------|
| 1 | Bounded population-oblivious wait-free | 每个线程在有限时间内前进，且该时间与线程数无关 |
| 2 | Bounded wait-free | 每个线程在特定有限时间内前进 |
| 3 | Wait-free | 每个线程在有限时间内前进 |
| 4 | Lock-free | 至少有一个线程在有限时间内前进 |
| 5 | Obstruction-free | 每个线程在无竞争时有限时间内前进 |
| 6 | Clash-free | 至少有一个线程在无竞争时有限时间内前进 |
| 7 | Starvation-free | 每个线程在无故障时有限时间内前进 |
| 8 | Deadlock-free | 至少有一个线程在无故障时有限时间内前进 |

**重要辨析**：wait-free 并不等同于"实时"。wait-free 只保证"有限时间"，而这个"有限时间"可能是 100 年。实时系统需要的是**有界**的、通常是微秒级的延迟保证。


再次引用一次这个: https://quant67.com/post/linux/lockfree-lies.html

### 3.2 简单 NBS 算法

#### 3.2.1 NBS 集合与计数器

- **NBS Set**：用数组或位图表示集合，索引即值，元素内容表示成员关系。如果不需要线性化，简单的 `READ_ONCE()` / `WRITE_ONCE()` 即可。
- **NBS Counter**：统计计数器通过巧妙的定义（允许近似和），可以被归类为 bounded wait-free。这是 Linux 内核中最广泛使用的 NBS 算法。

#### 3.2.2 半 NBS 队列 (Half-NBS Queue)

基于 userspace-RCU 库中的 `___cds_wfcq_append()` 实现，入队使用原子交换 (atomic exchange) 指令：

```c
old_tail = uatomic_xchg(&tail->p, new_tail);
CMM_STORE_SHARED(old_tail->next, new_head);
return old_tail != &head->node;
```

入队是 wait-free 的，但**单元素出队需要互斥锁**（因此是 blocking 的）。不过，**整个队列的非阻塞清空**是可能的：先原子交换 `head` 为 NULL，再原子交换 `tail`。这种 "half-NBS" 算法在实践中被大量使用。

#### 3.2.3 NBS 栈 (Lock-Free Stack)

基于 CAS (compare-and-swap) 的 LIFO push 和 `xchg` 的 `pop_all`：

```c
void list_push(value_t v) {
    struct node_t *newnode = malloc(sizeof(*newnode));
    struct node_t *oldtop;
    newnode->val = v;
    oldtop = READ_ONCE(top);
    do {
        newnode->next = oldtop;
    } while (!cmpxchg(&top, oldtop, newnode) &&
             (oldtop = READ_ONCE(top), 1));
}

struct node_t *list_pop_all(void) {
    return xchg(&top, NULL);
}
```

本目录下的 `nbs_stack.c` 实现了该算法，并通过 4 线程各 10 万次 push 的测试验证了其正确性。测试输出：

```
NBS Stack Test: 4 threads, 100000 pushes each
Total nodes pushed: 400000
Total nodes popped: 400000
Result: PASS
```

**ABA 问题与 Zombie Pointer**：该算法有一个不寻常的特性——即使节点 A 被 `pop_all` 释放后，另一个 push 恰好分配到了同样的地址，CAS 仍然能成功。这在纯汇编中是安全的，但在 C/C++ 中存在问题：编译器有权假设由不同 `malloc()` 返回的指针不相等。指向已释放但内存被重新分配对象的指针被称为 **zombie pointer**。C 和 C++ 标准委员会正在努力解决此问题。

### 3.3 NBS 的适用性讨论

#### 3.3.1 NBS 的前进进度保证 vs 实时需求

NBS 的前进进度保证与实时需求存在诸多差异：

1. **时间界定**：实时通常有确定的时间界（如"100 微秒内"），而 NBS 只说"有限时间"。
2. **概率性保证**：软实时常用概率保证（如 99.9% 的时间小于 100 微秒），NBS 常提供无条件保证，反而带来不必要的复杂性和性能损失。
3. **调度器假设**：NBS 假设最坏情况的"恶魔调度器"，而实时系统假设调度器在尽力满足约束。
4. **过载行为**：NBS 在过载时最有用，但许多实时系统在过载时会被终止。
5. **底层操作是否阻塞**：NBS 算法常依赖内存分配、I/O 或系统调用，而这些在 Linux 中通常是用锁实现的。

#### 3.3.2 NBS 的底层操作问题

即使是最简单的 load/store 指令，在现代系统上也不一定是 wait-free 的：

- **Cache miss**：可能消耗数千个 CPU 周期，且延迟与 CPU 数量相关。
- **Page fault**：触发内核页错误处理程序，可能获取锁甚至执行 I/O。
- **中断**：任何时候都可能发生中断，用户态的"非阻塞"算法在此期间完全停止运行。

缓解方法包括：裸机运行、使用非阻塞内核、`mlockall()` 预分配内存、Linux 的 `NO_HZ_FULL` 模式。

#### 3.3.3 NBS 的故障停止容忍 (Fail-Stop Tolerance)

wait-free、lock-free、obstruction-free 和 clash-free 即使在某个线程无限期被抢占的情况下也能保证前进进度。但需要注意：**组合多个 fail-stop 容忍机制不等于整个系统 fail-stop 容忍**。例如，一个元素从 wait-free 队列中取出后，处理它的线程 fail-stop，元素就丢失了——队列的 fail-stop 容忍不能延伸到使用队列的代码。

#### 3.3.4 NBS 的线性化 (Linearizability)

线性化在分析简单 NBS 算法时非常有用，但对于复杂 NBS 库的实现，线性化点往往深埋在算法内部，库用户并不能直接受益。现代并发代码的证明技术（如第 12 章讨论的形式化验证）很多并不依赖线性化。

---

## 4. 并行实时计算 (Parallel Real-Time Computing)
FIXME 我靠，第一次听到这个词汇

### 4.1 什么是实时计算？

#### 软实时 (Soft Real Time)

简单地称"偶尔错过截止时间"为软实时是没有意义的。必须有量化指标，例如"99.9% 的时间响应时间小于 20 微秒"。错过截止时间后的处理策略也很重要：可能是使用快速但不精确的回退方案。

#### 硬实时 (Hard Real Time)

严格地说，没有任何系统能永远保证不失败（因为总有更大的锤子）。因此，**硬实时是整个系统的属性，不仅仅是软件**。现实世界的实时规范必须包含：

- **环境约束**：温度、空气质量、电磁干扰、震动等级。
- **工作负载约束**：CPU 利用率上限、中断频率限制。实时系统常将 CPU 利用率保持在 80% 空闲。
- **应用约束**：哪些操作需要有界延迟，非实时部分能否使用实时 CPU。
- **操作延迟规格**：最大响应时间、最小响应时间、满足概率。

### 4.2 谁需要并行实时？

许多实时系统类似于生物神经系统：

- **实时反射 (Real-Time Reflexes)**：在单个 CPU 或 FPGA 上运行，读取传感器并控制执行器。
- **非实时策略与规划**：在剩余 CPU 上运行统计分析、校准、用户界面等。

例如，在胶合板旋切机控制中，一个 CPU 负责高速实时切削，其他 CPU 可以提前分析下一根原木的尺寸和形状。

### 4.3 实现并行实时系统

#### 4.3.1 Linux 内核的实时演进

Linux 内核的实时能力经历了三个阶段：

1. **非抢占内核 (CONFIG_PREEMPT=n)**：内核代码不可抢占，延迟受限于最长内核代码路径。
2. **可抢占内核**：进程级代码可抢占，但 RCU 读临界区、spinlock 临界区、中断处理程序、关中断/关抢占区域仍不可抢占。
3. **RT patchset**：中断处理线程化（threaded interrupts），允许中断处理程序被抢占；关中断区域被锁替代；关抢占被关迁移替代。

当前 Linux 内核源码 (`~/data/kernel/linux-build`) 中：

- **中断线程化**：`kernel/irq/manage.c` 中的 `irq_thread()` 函数实现了中断处理线程。设备中断处理程序只运行足够长的时间来唤醒 IRQ 线程，真正的处理在可抢占的线程上下文中完成。

```c
/* kernel/irq/manage.c */
static int irq_thread(void *data)
{
    ...
    if (force_irqthreads() && test_bit(IRQTF_FORCED_THREAD, &action->thread_flags))
        handler_fn = irq_forced_thread_fn;
    else
        handler_fn = irq_thread_fn;
    ...
}
```

- **优先级继承 (Priority Inheritance)**：`kernel/locking/rtmutex.c` 中的 `rt_mutex_adjust_prio_chain()` 实现了优先级链式提升。当高优先级线程试图获取被低优先级线程持有的 rt_mutex 时，低优先级线程的优先级被临时提升到高优先级线程的级别，防止优先级反转无限延长。

```c
/* kernel/locking/rtmutex.c */
static int __sched rt_mutex_adjust_prio_chain(struct task_struct *task, ...)
{
    ...
    if (++depth > max_lock_depth)
        return -EDEADLK;
    ...
}
```

#### 4.3.2 Timer Wheel 与 HRTIMER

Linux 内核的经典定时器数据结构是 **timer wheel** (`kernel/time/timer.c`)：

```c
/* kernel/time/timer.c */
#define LVL_BITS  6
#define LVL_SIZE  (1UL << LVL_BITS)  /* 64 */
#define LVL_DEPTH 9
#define WHEEL_SIZE (LVL_SIZE * LVL_DEPTH)

struct timer_base {
    raw_spinlock_t lock;
    ...
    DECLARE_BITMAP(pending_map, WHEEL_SIZE);
    struct hlist_head vectors[WHEEL_SIZE];
};
```

Timer wheel 通过多层数组层级提供 O(1) 的插入和删除，但对实时系统有两个问题：

1. **精度与开销的权衡**：1kHz 的时钟中断提供毫秒级精度但开销低；100kHz 提供十微秒级精度但 CPU 可能无暇做其他工作。
2. **级联 (cascading)**：高层数组的定时器需要在到期前级联到低层，级联大量定时器可能导致延迟抖动。

现代内核已**移除级联**：定时器直接在其所在层级处理，利用粒度差异提供隐式批处理。对于需要精确时间的场景，使用基于红黑树的 **hrtimer** (high-resolution timer)。

RT patchset 的解决方案是区分两类定时器：

- **错误处理超时 (timeouts)**：用 timer wheel 处理，毫秒级精度足够，且大多在级联前被取消。
- **定时事件 (timers)**：用树结构（hrtimer）处理，提供 O(log n) 但精确的调度。

#### 4.3.3 可抢占 RCU (Preemptible RCU)

`kernel/rcu/tree_plugin.h` 中的 `__rcu_read_lock()` 和 `__rcu_read_unlock()` 实现了可抢占 RCU：

```c
/* kernel/rcu/tree_plugin.h */
void __rcu_read_lock(void)
{
    rcu_preempt_read_enter();
    barrier();
}

void __rcu_read_unlock(void)
{
    struct task_struct *t = current;
    barrier();
    if (rcu_preempt_read_exit() == 0) {
        barrier();
        if (unlikely(READ_ONCE(t->rcu_read_unlock_special.s)))
            rcu_read_unlock_special(t);
    }
}
```

关键机制：

- 使用 per-task 的 `rcu_read_lock_nesting` 计数器支持嵌套。
- 如果线程在 RCU 读临界区内被抢占，`rcu_read_unlock_special` 标志被设置，`rcu_read_unlock()` 会调用 `rcu_read_unlock_special()` 将自己从阻塞当前 grace period 的列表中移除。
- 最高优先级的线程不会被抢占，因此它们永远不会在 `rcu_read_unlock()` 中尝试获取锁。
- `CONFIG_RCU_NOCB_CPU=y` 可以将 RCU callback 执行卸载到 housekeeping CPU，避免在实时 CPU 上引入抖动。

#### 4.3.4 CPU 隔离与 NO_HZ_FULL

Linux 3.10 引入的 `CONFIG_NO_HZ_FULL` 是实现接近裸机性能的关键：

- 当某个非 housekeeping CPU 上只有一个可运行任务时，**调度时钟中断被关闭**。
- 这会消除一个重要的抖动来源（OS jitter）。
- 需要至少一个 housekeeping CPU 运行内核守护进程。
- 当前内核源码 `kernel/time/tick-sched.c` 中维护 `tick_nohz_full_mask` 和 `tick_nohz_full_running` 标志。

最新研究（如 Zhou et al. 2025）甚至提出延迟所有 IPI 到实时应用的安全点，从而将 Linux 实时延迟降低到 **0.5 微秒**。

### 4.4 实时应用实现要点

本目录下的 `rt_timer.c` 演示了编写实时定时等待应用的三个关键步骤：

1. **使用 `CLOCK_MONOTONIC`**：而非 `CLOCK_REALTIME`，后者受 NTP 调整影响。
2. **提升实时优先级**：使用 `sched_setscheduler(0, SCHED_FIFO, &param)`。
3. **锁定内存**：使用 `mlockall(MCL_CURRENT | MCL_FUTURE)` 防止缺页中断。

测试输出示例（非 root 运行时无法设置 SCHED_FIFO）：

```
mlockall: OK
Warning: cannot set real-time priority (need root?)
Running 1000 iterations of 1000000 ns sleep...

Results (1000 iterations, requested 1000000 ns):
  Min extra latency: 8931 ns
  Max extra latency: 82076 ns
  Avg extra latency: 59744 ns
```

若使用 root 运行并绑定到隔离的 `nohz_full` CPU，延迟通常可降低一个数量级以上。

---

## 5. Linux 内核源码关联总结

| 本章概念 | Linux 内核源码位置 | 关键文件/函数 |
|----------|-------------------|---------------|
| Timer Wheel | `kernel/time/timer.c` | `struct timer_base`, `enqueue_timer()` |
| HRTIMER | `kernel/time/hrtimer.c` | 红黑树实现的精确定时器 |
| Threaded IRQs | `kernel/irq/manage.c` | `irq_thread()`, `setup_irq_thread()` |
| Priority Inheritance | `kernel/locking/rtmutex.c` | `rt_mutex_adjust_prio_chain()` |
| Preemptible RCU | `kernel/rcu/tree_plugin.h` | `__rcu_read_lock()`, `__rcu_read_unlock()` |
| Preemptible Spinlocks | `kernel/locking/spinlock_rt.c` | RT patchset 中的可抢占 spinlock |
| NO_HZ_FULL | `kernel/time/tick-sched.c` | `tick_nohz_full_mask` |
| Per-CPU 变量 | 内核广泛使用 | `get_cpu_var()`, `put_cpu_var()` |
| Statistical Counters | 内核多处 | `percpu_counter` 等 |

---

## 6. 用户笔记疑问解答

用户在 `~/data/vn/docs/concurrent/` 中记录的笔记提出了多个与第 14 章相关的问题，以下是针对性回答：

### 6.1 wait-free / lock-free / obstruction-free 的区分

笔记中的 `lock-free.md` 已经准确总结了：

- **wait-free**：每个线程都在有限步内完成。
- **lock-free**：至少有一个线程在有限步内完成，所有线程最终都会前进。
- **obstruction-free**：单个线程在其他线程暂停时可以完成。

第 14 章补充了更细粒度的分类（bounded wait-free 等），并强调：**wait-free 不等于实时**。wait-free 的"有限时间"可能是任意长，而实时需要的是**有界且可预测**的延迟。此外，第 14 章明确指出，在公平调度器下，lock-free 算法在实践中与 wait-free 几乎没有区别。

### 6.2 为什么要使用 wait-free？

笔记中质疑：CAS 失败后的重试循环和 spinlock 的自旋似乎没有区别。第 14 章的回答是：

1. **防止死锁**：持有锁的线程被抢占后，其他线程只能等待；而 NBS 中所有线程都在持续执行。
2. **fail-stop 容忍**：某些 NBS 算法在线程突然停止时仍能保证系统前进。
3. **但性能未必更好**：简单 NBS 算法性能优秀，复杂 NBS 往往因为过度设计而性能下降。

### 6.3 Linearizability (线性化)

笔记 `linearizability.md` 详细分析了 quiescent consistency、sequential consistency 和 linearizability 的关系。第 14 章对此的补充非常关键：

- 线性化在分析**简单 NBS** 和**严格锁 + 全序原子操作**时很有用。
- 但对于**复杂 NBS 库**，线性化点深埋在实现内部，库用户并不能直接受益。
- 现代证明技术（如针对 Linux RCU 的验证）已经不需要依赖线性化。

### 6.4 Stack 是否可以是并发的？

笔记 `notes.md` 中提问："stack 有什么并发可言吗？"

第 14 章的 NBS Stack 给出了明确答案：**可以**。LIFO push 使用 CAS 实现 lock-free，`pop_all` 使用 `xchg` 实现 bounded wait-free。但单元素的 lock-free pop 更复杂，需要考虑 ABA 问题。本目录的 `nbs_stack.c` 实现了并发安全的 push 和 pop_all。

### 6.5 seqlock 的实现与使用场景

虽然第 14 章没有直接讨论 seqlock，但 NBS 和实时章节的相关讨论可以延伸理解：seqlock 属于"读取无锁、写入上锁"的设计，与 statistical counters 和 RCU 属于同一家族。它的好处是读端完全无锁，坏处是读端可能需要重试。第 14 章对"什么时候用无锁、什么时候用锁"的分析直接适用于 seqlock 的选型。

### 6.6 RCU 链表/哈希表与普通链表的区别

第 14 章通过 `Real-Time Calibration Using RCU` 的例子展示了 RCU 在实时系统中的核心优势：

- **读端确定性的延迟**：`rcu_dereference()` 只是一个依赖数据加载，没有锁、没有重试、没有阻塞。
- **写端通过 `rcu_assign_pointer()` 原子发布新版本**。
- **旧版本通过 grace period 安全释放**。

这与普通链表需要锁保护读写有本质区别，也与 hazard pointers 需要读端写共享状态不同。

---

## 7. 代码文件说明

本目录包含以下可编译运行的代码示例：

| 文件 | 说明 | 编译目标 |
|------|------|----------|
| `nbs_stack.c` | Lock-free LIFO stack (CAS push, xchg pop_all) | `nbs_stack.out` |
| `nbs_queue.c` | Half-NBS queue (xchg enqueue, mutex-protected dequeue) | `nbs_queue.out` |
| `rt_timer.c` | Real-time timer latency test (CLOCK_MONOTONIC, SCHED_FIFO, mlockall) | `rt_timer.out` |
| `Makefile` | 一键编译、测试、清理 | - |

使用方式：

```bash
make all    # 编译
make test   # 运行测试
make clean  # 清理
```

---

## 8. 结论

第 14 章是一章极具实践指导意义的内容。它没有盲目推崇 NBS 或实时技术，而是反复强调：

1. **先分区、再批处理、再用成熟 API，最后才考虑 NBS。**
2. **NBS 的 theory 和 practice 之间存在巨大鸿沟**：底层内存操作、页错误、中断都可能破坏非阻塞保证。
3. **实时是系统级属性**：涉及硬件、固件、操作系统、应用设计、工作负载控制的全面协同。
4. **Linux 内核通过 RT patchset、NO_HZ_FULL、threaded interrupts、priority inheritance、preemptible RCU 等机制**，正在将通用操作系统的实时能力推向亚微秒级。

对于并发编程的学习者而言，本章最大的收获是建立**正确的技术选型思维**：理解各种同步机制的能力边界、成本开销和适用场景，而不是简单地将"无锁"或"实时"视为终极目标。

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
