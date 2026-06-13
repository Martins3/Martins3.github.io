# Chapter 12: Formal Verification - 详细分析

## 1. 章节概述

Perfbook 第 12 章 "Formal Verification" 围绕一个核心命题展开：**并行算法难以编写，更难调试；测试虽然必不可少，但不足以发现所有竞态条件（race condition）**。形式化验证（Formal Verification）作为验证工具箱中的重要补充，可以在设计阶段对小规模、高复杂度的并行代码片段进行数学意义上的正确性证明。

本章以 Donald Knuth 的名言开篇："Beware of bugs in the above code; I have only proved it correct, not tried it." 这句话精准地概括了形式化验证的局限与价值——它能证明代码在模型层面的正确性，却无法替代实际测试，更不能发现需求错误、假设偏差或硬件原语理解错误。

本章依次介绍了以下几类形式化验证工具与方法：

- **Promela / Spin**：通用的状态空间搜索工具
- **PPCMEM / herd**：针对 PowerPC / ARM / x86 内存模型的专用验证工具
- **Axiomatic Approaches（公理化方法）**：基于 LKMM（Linux Kernel Memory Model）的 litmus test 验证
- **SAT Solvers（SAT 求解器）**：以 CBMC 为代表，将 C 代码转换为布尔可满足性问题
- **Stateless Model Checkers（无状态模型检测器）**：以 Nidhugg 为代表，牺牲数据非确定性以换取更大规模的验证能力

最后，本章以 "Choosing a Validation Plan" 收尾，将形式化验证定位为"保险"——需要持续投入，但不应过度投资，最佳实践是优先测试，对测试难以覆盖且鲁棒性要求极高的代码片段辅以形式化验证。

Section 12.1 provides an introduction to the general-purpose state-space search tools Promela and Spin,
Section 12.2 similarly introduces the special purpose `ppcmem` and `cppmem` tools,
Section 12.3 looks at an example axiomatic approach,
Section 12.4 briefly overviews SAT solvers,
Section 12.5 briefly overviews stateless model checkers,
Section 12.6 sums up use of formal-verification tools for verifying parallel algorithms, and finally Section 12.7 discusses how to decide how much and what type of validation to apply to a given software projec

---

## 2. Promela 与 Spin：状态空间搜索

### 2.1 基本原理

Promela 是一种专门用于验证协议的类 C 语言，Spin 则将其翻译为 C 程序后进行完整的状态空间搜索（full state-space search）。与压力测试不同，Promela/Spin 不需要循环执行竞态操作——它通过非确定性调度（nondeterministic interleaving）自动探索所有可能的执行序列，从而验证断言（assertion）或发现反例（counter-example）。

这种全状态搜索是一把双刃剑：如果模型过于复杂或状态变量过多，状态空间会以指数级膨胀，导致内存耗尽或运行时间超过宇宙寿命。因此，Promela/Spin 适用于"紧凑但复杂"的并行算法，而非大规模代码库（如整个 Linux 内核）。

### 2.2 非原子增量（Warm-Up）

本章的第一个示例是经典的非原子增量竞态条件。两个进程分别执行 `temp = counter; counter = temp + 1;`，由于 Promela 中每条简单语句默认是原子的，因此必须显式拆分为两条语句来模拟非原子行为。

Spin 运行此模型后会报告断言失败（assertion violated），并生成 `.trail` 文件记录导致失败的精确执行序列。通过 `spin -t -p` 可以 human-readable 地回溯该序列：两个进程都先读取 counter（例如读到 0），然后各自加一并写回，最终 counter 为 1 而非 2，丢失了一次更新。

我们编写了对应的 C 语言示例 `race_increment.c`，使用 pthread 创建两个线程，每个线程对共享变量执行 100000 次非原子增量。由于实际硬件上的竞态条件具有概率性，小循环次数可能无法触发 bug，但增大迭代次数后几乎必然出现计数丢失。修复方法是使用 `atomic_fetch_add()`，见 `atomic_increment.c`。

### 2.3 原子增量与状态空间爆炸

将增量操作放入 `atomic { }` 块（或直接使用 `counter = counter + 1`，因为单条 Promela 语句是原子的）即可消除竞态。但随着进程数增加，状态空间急剧膨胀：1 个增量器仅需 11 个状态，2 个需 52 个，3 个需 372 个，到 7 个时已达到 850 万个状态，内存消耗接近 1GB。这直观地说明了状态空间爆炸问题。

### 2.4 Spinlock 验证

本章提供了 `spin_lock()` 和 `spin_unlock()` 的 Promela 宏定义，并通过 3 个 locker 进程无限循环地获取/释放锁来验证互斥性。断言 `assert(sum <= 1)` 确保任何时候最多只有一个进程持有锁。

对应的 C 语言实现 `spinlock_demo.c` 使用 `atomic_exchange` 实现 test-and-set 自旋锁。我们通过原子变量 `in_critical` 跟踪当前处于临界区的线程数，实验验证了在 3 个线程各迭代 50000 次的情况下，`max_concurrent` 始终为 1，证明了互斥性。

### 2.5 QRCU 验证

QRCU（Quick RCU）是 SRCU 的一种变体，以稍高的读端开销（全局变量的原子增减）换取极低的 grace period 延迟（无读者时小于 1 微秒）。本章用 Promela 建模了 QRCU 的读者（`qrcu_reader`）和更新者（`qrcu_updater`）进程，并通过 `sum_unordered` 宏显式模拟弱内存序下的非确定性读取顺序。

该模型展示了形式化验证中的典型挑战：2 个读者 + 2 个更新者产生约 100 万个状态，2 个读者 + 3 个更新者产生约 3300 万个状态，3 个读者 + 3 个更新者则需要约 500GB 内存。为解决此问题，作者使用 `-DMA=96` 编译选项进行激进的状态压缩，将内存降至 6.5GB，但运行时间长达 3 天。

本章还提供了一个非形式化的手工证明（proof of correctness），通过分析计数器翻转（counter flip）和读者并发数的约束，论证了 fastpath 的安全性。这引出了一个重要观点：**不是所有算法都需要机械化的形式化验证，清晰的数学证明往往更实用**。

### 2.6 Dynticks 与 Preemptible RCU：一个真实案例

这是本章最详细、最具教学价值的案例。2008 年，preemptible RCU 被合入主线内核以支持实时工作负载，但旧的实现要求每个 grace period 在每个 CPU 上执行工作，即使该 CPU 处于低功耗的 dynticks-idle 状态。Steve Rostedt 的新 dyntick 实现与 preemptible RCU 集成后导致系统启动挂死。

Paul McKenney 从 2007 年 10 月到 2008 年 2 月反复人工审查代码，几乎每次都"发现"至少一个 bug——但所有 bug 都是幻觉。最终他转向 Promela/Spin，构建了 7 个逐步逼近真实实现的模型：

1. **Basic Model**：仅建模任务级 dynticks 进入/退出和 grace-period 处理
2. **Safety Assertions**：添加 `grace_period_state` 变量验证 RCU 安全属性（grace period 不能在读侧临界区完成前结束）
3. **Liveness Checks**：验证 grace period 不会无限期阻塞，由此发现了一个真实 bug——`rcu_try_flip_waitack_needed()` 中错误地检查了 `snap` 而非 `curr` 的奇偶性
4. **Interrupt Handlers**：用单独的 `dyntick_irq()` 进程建模中断，通过 `EXECUTE_MAINLINE` 宏实现主代码与中断的原子互斥
5. **Nested Interrupts**：用两个计数器 `i`（进入）和 `j`（退出）建模嵌套中断
6. **NMI Handlers**：NMIs 不嵌套，因此模型更简单

修正后的 C 代码逻辑如下（对应旧内核 v2.6.25-rc4）：

```c
static inline int
rcu_try_flip_waitack_needed(int cpu)
{
	long curr = per_cpu(dynticks_progress_counter, cpu);
	long snap = per_cpu(rcu_dyntick_snapshot, cpu);
	smp_mb();
	if ((curr - snap) >= 2 || (curr & 0x1) == 0)
		return 0;
	return 1;
}
```

该验证最终消耗约 40GB 内存，但成功发现了一个极其隐蔽的 bug。

#### 2.6.1 与当前 Linux Kernel 源码的对比

在当前内核（`~/data/kernel/default`，较新版本）中，dynticks 机制已经经历了多次重写。旧版的 `dynticks_progress_counter` 和 `rcu_dyntick_snapshot` 已被新的 **context tracking** 框架取代。

当前内核中，核心状态通过 `context_tracking.state` 原子变量管理，RCU 子系统使用 `ct_rcu_watching()` 和 `ct_rcu_watching_cpu_acquire()` 接口查询 CPU 是否处于 extended quiescent state（EQS）。`kernel/rcu/tree.c` 中的 `rcu_watching_snap_save()` 和 `rcu_watching_snap_recheck()` 对应旧版的 `dyntick_save_progress_counter()` 和 `rcu_try_flip_waitack_needed()`，但实现更为简洁：

```c
static int rcu_watching_snap_save(struct rcu_data *rdp)
{
	rdp->watching_snap = ct_rcu_watching_cpu_acquire(rdp->cpu);
	if (rcu_watching_snap_in_eqs(rdp->watching_snap)) {
		/* CPU is in dynticks idle */
		return 1;
	}
	return 0;
}

static bool rcu_watching_snap_stopped_since(struct rcu_data *rdp, int snap)
{
	return snap != ct_rcu_watching_cpu_acquire(rdp->cpu);
}
```

这一演化的核心思想在章节末尾被总结为 **"Simplicity Avoids Formal Verification"**：将 IRQ 和 NMI 的代码路径分离（使用独立的 `dynticks` 和 `dynticks_nmi` 变量），避免了共享状态带来的验证复杂度。当前内核的 context tracking 进一步将这一思想推广到整个内核的 idle/user/idle 状态管理。

---

## 3. PPCMEM：专用状态空间搜索

### 3.1 为什么需要专用工具？

Promela 假设顺序一致性（sequential consistency），不直接理解内存模型或重排序语义。虽然可以通过非确定性的 `if-fi` 分支手动模拟重排序（如 QRCU 中的 `sum_unordered`），但这要求开发者完全理解目标系统的内存模型——而现代 CPU（Power、ARM、x86）的内存模型极其复杂，很少有开发者能完全掌握。

PPCMEM 工具由剑桥大学等机构开发，形式化了 Power、ARM、x86 以及 C/C++11 的内存模型，并基于这些形式化定义进行验证。

### 3.2 Litmus Test 示例

PPCMEM 接受 **litmus test** 作为输入。本章展示了一个 PowerPC litmus test，其 C 语言等价物为：

```c
void P0(void)
{
	x = 1;
	atomic_add_return(&z, 0);  /* 旧版 PowerPC 实现不完整屏障 */
	r3 = y;
}

void P1(void)
{
	y = 1;
	smp_mb();
	r3 = x;
}
```

断言关注的是：执行完成后，是否可能出现 `P0.r3 == 0 && P1.r3 == 0`？由于 `atomic_add_return()` 旧版 PowerPC 实现使用 `lwsync` + `isync` 而非完整的 `sync`，PPCMEM 报告该结果是 **Sometimes**（可能发生）。修复方法是将 `isync` 替换为 `sync`，此时 PPCMEM 报告 **Never**。

我们编写了 `litmus_sb.c`，使用 C11 `memory_order_relaxed` 演示 Store Buffering（SB）模式：两个线程分别写 `x` 和 `y`，然后读对方的变量。在 x86 等较强内存序的硬件上，该程序在 10 万次试验中未观测到 `r1=0 && r2=0`，但这不代表该结果在架构层面被禁止——它只是在当前硬件实现上未出现。

### 3.3 PPCMEM 的局限

本章明确列出了 PPCMEM 的若干局限：

1. 研究原型，无官方支持
2. 不能替代真实硬件的压力测试
3. 仅支持字长（32 位）对齐访问
4. 仅适用于小片段、无循环、少线程的代码
5. 不提供反例的到达路径（但可通过交互式工具查找）
6. 不支持复杂数据结构、MMIO、设备寄存器
7. 只能检测已编码断言的问题

---

## 4. 公理化方法（Axiomatic Approaches）

### 4.1 从 PPCMEM 到 herd

PPCMEM 基于 trace 的全状态空间搜索非常耗时。例如，经典的 IRIW（Independent Reads of Independent Writes）litmus test 需要 14 个 CPU 小时和 10GB 状态空间。

公理化方法（以 `herd` 工具为代表）则创建一组公理来表示内存模型，将 litmus test 转换为定理进行证明或反证。`herd` 处理 IRIW 仅需 17 毫秒——6 个数量级的加速。当然，问题的指数性质并未改变，增加写入次数或线程数仍会导致指数级 slowdown。

### 4.2 LKMM 与高级同步原语建模

Linux Kernel Memory Model（LKMM）允许在 C 语言层面直接编写 litmus test，并使用 `herd` 验证。本章展示了两个关键示例：

1. **Locking**：`C-Lock1.litmus` 使用同一 spinlock 保护 `x` 的读写，`herd` 输出 `Never`。若使用不同锁（`C-Lock2.litmus`），则输出 `Sometimes`。直接建模 `spin_lock()` / `spin_unlock()` 比用原子操作模拟锁快 5 倍到 2 个数量级。

2. **RCU**：`C-RCU-remove.litmus` 展示了经典的 RCU 链表删除场景。`P0()` 移除元素并等待 grace period，`P1()` 在 RCU 读侧临界区内遍历链表。`herd` 输出 `Never`；若移除 `synchronize_rcu()`，则输出 `Sometimes`。

更复杂的 `C-RomanPenyaev-list-rcu-rr.litmus` 展示了读者"泄漏"指针到临界区外的情况，更新者需要两次 `synchronize_rcu()` 来确保安全。这验证了 RCU 使用中一个高级但危险的技巧。

---

## 5. SAT 求解器：CBMC

### 5.1 基本原理

任何有限程序（有界循环和递归）都可以转换为逻辑表达式。如果输入表示为布尔变量组合，那么判断是否存在导致断言失败的输入组合，就是经典的 SAT（可满足性）问题。

CBMC（C Bounded Model Checker）是一款前端工具，可自动将 C 代码转换为逻辑表达式，并生成数组越界等错误的断言。使用极为简单：`cbmc test.c` 即可完成验证。这种易用性使其有望融入回归测试框架。

### 5.2 并行代码验证

近年来 SAT 求解器开始支持并行代码：将输入代码转为单静态赋值（SSA）形式，然后生成所有允许的访问顺序。2016 年的工作成功将 CBMC 应用于 Linux-kernel Tree RCU，验证了少量线程的小配置场景。生成的逻辑表达式高达 9000 万个变量、4.5 亿个子句，占用数十 GB 内存，SAT 求解耗时最多 80 小时。

作者对此持审慎态度：即使工具报告"验证通过"，也应通过注入已知 bug（mutation testing）来检验工具本身。Paul McKenney 维护了一个包含 20 个变异分支的 git 仓库，供形式化验证工具测试其发现注入 bug 的能力。

---

## 6. 无状态模型检测器：Nidhugg

### 6.1 设计权衡

SAT 求解器跟踪所有可能执行的全部状态，开销巨大。无状态模型检测器（如 Nidhugg）放弃跟踪完整状态，仅通过调度重排来探索执行路径，从而处理更大的程序。

Nidhugg 在某些 Linux-kernel RCU 验证场景中比 CBMC 快一个数量级以上。但其速度优势建立在**不处理数据非确定性**的前提下——如果 bug 依赖于具体的输入数据值，Nidhugg 可能无法发现。

### 6.2 现状

与 CBMC 类似，Nidhugg 尚未发现 RCU 维护者未知的新 bug，但曾揭示一个历史 bug 实际上是由另一个 commit 修复的——这至少说明此类工具有潜力在将来发现新的并发 bug。

---

## 7. 总结与验证计划的选择

本章的核心结论可以概括为以下几点：

1. **形式化验证不会取代测试**。对于大规模并行系统（如 Linux 内核），测试仍是验证的主力。形式化验证应聚焦于"小而复杂"的代码片段。

2. **寻找 bug 的层次**：第一个 bug 通常通过代码审查或编译器诊断发现；第一个相关 bug 可能仍通过审查发现；大部分生产环境相关的 bug 通过测试发现；但**最后一个 bug**（尤其是安全相关的）无法通过测试找到，这正是形式化验证的潜在价值所在。

3. **复杂度是敌人**。如果代码复杂到必须依赖形式化验证工具（尤其是需要手工翻译到专用语言时），应该重新思考设计。本章的 dynticks 案例最终通过简化设计（分离 IRQ/NMI 路径）避免了复杂的验证。

4. **验证的投资如同保险**。应根据软件的关键性分配验证预算。Linux-kernel RCU 的测试代码占比随版本演进不断变化，体现了验证策略的动态调整。

5. **组合策略是最佳实践**。Linux 内核的 lockdep、KCSAN 等工具将形式化验证技术与运行时分析结合。当前最佳实践是：优先测试，对测试难以覆盖且鲁棒性要求极高的模块辅以形式化验证。

---

## 8. 结合 Linux Kernel 源码的分析

### 8.1 RCU Dynticks 的演化

本章详细分析的 `dynticks_progress_counter` 机制是 Linux v2.6.25-rc4 时代的实现。在当前内核源码中，这一机制已被彻底重写：

- **旧机制**：每个 CPU 维护一个 `dynticks_progress_counter` 计数器，偶数表示 idle，奇数表示 active。RCU grace period 通过比较当前值和快照值（`curr - snap >= 2 || (curr & 1) == 0`）判断 CPU 是否经历了 quiescent state。

- **新机制**：基于 `context_tracking` 框架，使用 `ct_rcu_watching()` 查询 CPU 的 RCU 观察状态。`kernel/rcu/tree.c` 中的 `rcu_watching_snap_save()` 和 `rcu_watching_snap_recheck()` 使用 `atomic_read_acquire` 来保证内存序，不再需要开发者手工维护奇偶计数器的复杂逻辑。

这一演化验证了本章的核心观点：简单的设计不仅减少 bug，也降低验证负担。

### 8.2 LKMM 在内核源码中的位置

当前 Linux 内核源码在 `tools/memory-model/` 目录下包含了完整的 LKMM 定义和 `herd` 工具的配置文件。开发者可以直接在内核源码树中运行 litmus test，验证自己的并发代码是否符合内核内存模型。这与本章介绍的 axiomatic approach 完全一致。

### 8.3 Lockdep 与 KCSAN：运行时形式化技术

本章在 "Choosing a Validation Plan" 中提到，lockdep 和 KCSAN 是形式化验证技术与运行时分析结合的优秀例子。Lockdep 通过构建锁依赖图并检测循环来预测死锁；KCSAN 通过编译器插桩检测数据竞争。它们不是传统意义上的"证明"工具，但将形式化方法的思想应用到了可扩展的运行时场景中。

---

## 9. 用户疑问分析（基于 ~/data/vn/docs/concurrent）

阅读用户的并发编程笔记后，发现其主要疑问集中在以下几个方面：

### 9.1 Memory Barrier 与 Memory Model

用户的笔记中有大量关于 memory barrier（`smp_mb`, `smp_rmb`, `smp_wmb`, `smp_load_acquire`, `smp_store_release`）的疑问，特别是 ARM64 和 x86 的实现差异、DSB/DMB/ISB 的区别、以及何时使用何种屏障。

**本章是否解答了这些疑问？**

本章**没有直接回答** memory barrier 的使用方法和具体语义。它关注的是如何用形式化工具验证使用了 memory barrier 的代码是否正确。例如：
- 在 Promela 中，内存屏障**不需要建模**，因为 Promela 假设顺序一致性
- 要模拟**缺乏**内存屏障的情况，才需要显式编写非确定性分支（如 `sum_unordered`）
- PPCMEM 和 herd 则**内置**了对各种内存屏障语义的理解

对于用户想了解的"memory barrier 基本使用方法"和"各种使用案例"，本章提供的更多是**验证视角**而非**编程视角**。建议用户结合 perfbook 第 14 章（Advanced Synchronization: Memory Ordering）和 `Documentation/memory-barriers.txt` 来系统学习。

### 9.2 RCU 的正确使用

用户笔记中有"如何正确使用 RCU"以及 "seqlock vs RCU" 的疑问。

本章通过 QRCU 和 dynticks 的 Promela 建模，以及 LKMM 的 RCU litmus test，**从验证角度深化了对 RCU 语义的理解**：
- `synchronize_rcu()` 是 grace period 的边界
- 读者在 `rcu_read_lock()` / `rcu_read_unlock()` 之间访问的数据，在 grace period 结束前不会被释放
- 通过 `herd` 运行 `C-RCU-remove.litmus` 可以直观验证"移除 `synchronize_rcu()` 会导致 `Sometimes`"

但这仍然是**底层机制**的验证，而非**上层使用模式**的指导。对于"什么时候用 seqlock 而不是 RCU"这类工程决策问题，需要参考 perfbook 其他章节或内核文档。

### 9.3 Lock-Free / Wait-Free

用户笔记详细记录了 lock-free、wait-free、obstruction-free 的定义和性能疑问。本章**未涉及**这些无锁编程范式，因为形式化验证工具（尤其是 Promela/Spin）更擅长建模基于锁或基于 RCU 的算法，而非复杂的 lock-free 数据结构。

### 9.4 Data Race 与 ACCESS_ONCE / READ_ONCE

用户提到"access once / data_race 完全不懂"。本章在 Promela 建模中实际上隐含了 data race 的概念（非原子增量导致断言失败），并通过原子块来消除 race。但本章没有讨论 C11 `memory_order_relaxed`、Linux 内核的 `READ_ONCE`/`WRITE_ONCE` 与编译器优化的关系。这些问题属于第 4 章（What Makes Parallel Programming Hard?）和第 14 章的范畴。

### 9.5 工具链疑问

用户笔记中提到了 `herd`、PPCMEM、CBMC、Nidhugg 等工具。本章**系统性地介绍了这些工具**的原理、用法和局限，这是与用户笔记直接对接的部分。特别是：
- `herd` 可以验证用户关心的 LKMM litmus test
- CBMC 的易用性（`cbmc test.c`）降低了形式化验证的门槛
- Nidhugg 的速度优势适合更大规模的模型

### 9.6 结论

本章**部分解答**了用户的以下疑问：
- [x] 形式化验证工具是什么、能做什么、不能做什么
- [x] RCU 的底层语义和验证方法
- [x] 内存模型验证的基本思路（通过 litmus test）

本章**未直接解答**的疑问：
- [ ] memory barrier 的具体使用方法和场景
- [ ] seqlock vs RCU 的工程选择
- [ ] lock-free 数据结构的实现与验证
- [ ] `READ_ONCE` / `WRITE_ONCE` 与编译器优化的细节
- [ ] cache coherence 与 memory model 的关系

---

## 10. 代码说明与运行方式

本章提取的 C 语言示例代码和 Makefile 位于当前目录下：

| 文件 | 说明 |
|------|------|
| `race_increment.c` | 非原子增量竞态条件演示 |
| `atomic_increment.c` | 使用 `atomic_fetch_add` 修复 race |
| `spinlock_demo.c` | test-and-set 自旋锁互斥性验证 |
| `dynticks_like.c` | 模拟旧内核 dynticks 计数器机制 |
| `litmus_sb.c` | Store Buffering litmus test 的 C11 relaxed 版本 |
| `Makefile` | 一键编译、测试、清理 |

编译与测试命令：

```bash
make all   # 编译所有示例
make test  # 运行所有测试
make clean # 清理二进制文件
```

所有二进制文件以 `.out` 结尾，会被 `.gitignore` 自动忽略。

---

## 11. 关键引用与推荐阅读

1. **Promela/Spin**: https://spinroot.com/spin/whatispin.html
2. **PPCMEM**: https://www.cl.cam.ac.uk/~pes20/ppcmem/
3. **herd / LKMM**: Linux 内核 `tools/memory-model/README`
4. **CBMC**: 多数 Linux 发行版可直接安装 `cbmc` 包
5. **Nidhugg**: https://github.com/nidhugg/nidhugg
6. **Linux kernel dynticks 演进**: `kernel/rcu/tree.c`, `include/linux/context_tracking_state.h`
7. **Verification Challenge 6**: Paul McKenney 维护的 20-branch mutation 测试集

---

*分析完成时间: 2026-04-27*

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
