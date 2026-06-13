# 第 9 章：Deferred Processing（延迟处理）详细分析

## 1. 章节概述与核心思想

第 9 章 "Deferred Processing" 是 perfbook 中极为关键的一章，它探讨了在并发编程中如何通过"延迟执行"来降低同步开销、提升可扩展性。核心洞察在于：**在并行编程中，"懒惰"（laziness）往往比"勤劳"更具性能和扩展性优势**。这种优势来源于延迟工作可以弱化同步原语的需求，从而减少同步开销。

本章围绕一个贯穿始终的示例展开：一个简化的 Pre-BSD 包路由表。该路由表本质上是一个链表，包含地址到网络接口的映射。这个示例被故意简化（搜索键从四元组降到简单整数），目的是为了在直观的环境中突出并行特有的问题，而不被复杂的算法细节分散注意力。

本章介绍的延迟处理技术主要包括四类：
1. **引用计数（Reference Counting）**
2. **Hazard Pointers（危险指针）**
3. **Sequence Locks（顺序锁）**
4. **Read-Copy Update (RCU)**

此外，本章还讨论了如何在不同场景下选择这些技术，以及更新操作的策略。

---

## 2. 运行示例：Pre-BSD 包路由表

### 2.1 顺序实现

顺序版本的路由表使用简单的单向链表实现，包含三个核心操作：

- `route_lookup(addr)`: 顺序遍历链表，返回对应接口号或 `ULONG_MAX`
- `route_add(addr, iface)`: 分配新节点并添加到链表头部
- `route_del(addr)`: 查找并删除指定节点，释放内存

顺序实现作为后续并发实现的基准（prototype），也代表了理想情况下的性能和扩展性上限。

### 2.2 并发化挑战

将顺序实现改造为并发安全版本时，核心挑战在于**读多写少**场景下如何保护共享的链表结构：
- 多个读者同时遍历链表
- 更新者可能插入或删除节点
- 最棘手的问题：删除节点时，如何确保没有读者仍在访问该节点？

这正是本章各种延迟处理技术试图解决的核心问题。

---

## 3. 引用计数（Reference Counting）

### 3.1 原理与历史

引用计数通过跟踪指向对象的引用数量来防止对象被过早释放。其历史可以追溯到 1960 年代的早期论文，甚至更早的工业安全实践（如维修大型危险机器前用挂锁锁定电源开关，每个工人加一把锁，相当于一次引用计数）。

### 3.2 并发陷阱：The Reference-Counting Trap

本章展示了一个看似合理的并发引用计数实现，但它实际上存在**严重的 use-after-free 漏洞**。问题出在以下时序：

1. **线程 A** 执行 `route_lookup(42)`，读取到链表第一个节点的指针 `rep`，但尚未执行 `atomic_cmpxchg` 增加引用计数
2. **线程 B** 执行 `route_del(42)`，成功删除该节点，发现引用计数为 1，于是调用 `re_free()` 释放节点
3. **线程 A** 继续执行，尝试对已经释放的节点执行 `atomic_cmpxchg`，导致未定义行为（实际实现中通过 `re_freed` 检查触发 `abort()`）

**核心问题**：引用计数存储在被保护的对象内部，这意味着在获取引用计数本身的瞬间，对象处于无保护状态！这是并发引用计数最经典的陷阱（Gamsa et al. 早在 1999 年就指出了类似问题）。

### 3.3 性能灾难

即使引用计数在正确性上可以被修复（使用极其复杂的算法，如 Valois 95a、MagedMichael 95a），其性能仍然糟糕。在 448 线程的系统上，引用计数的读性能几乎与 x 轴重合（即接近零），因为它将频繁的共享内存写操作（原子递增/递减）引入了一个原本只读的工作负载，导致严重的缓存行伪共享和缓存行弹跳（cache-line bouncing）。

### 3.4 与 Linux 内核的关联

Linux 内核中大量使用引用计数（`kref`、`atomic_t` 等），但内核通过以下手段来缓解上述问题：
- 结合 RCU 使用：RCU 提供短期、无条件的存在保证，引用计数仅用于需要长期持有引用的少数对象
- 使用 per-CPU 计数器（如 `percpu_ref`）来减少缓存行争用
- 专门的引用计数获取原语（如 `kref_get_unless_zero`）避免 use-after-free

内核源码参考：
- `include/linux/kref.h`: `kref` 引用计数基础结构
- `include/linux/percpu-refcount.h`: per-CPU 引用计数实现

---

## 4. Hazard Pointers（危险指针）

### 4.1 核心思想：Inside-Out Reference Counting

Hazard Pointers 由 Maged Michael 于 2004 年提出（同时期其他人也有独立发明）。它的核心洞察是将引用计数"翻转"：不在对象内部存储计数器，而是在每个线程的本地存储中记录当前正在访问的对象指针。

每个线程维护一组 **hazard pointers**。当线程遍历链表时，每访问一个节点，就将其指针记录到自己的 hazard pointer 中。更新者删除节点后不能立即释放，而是将其放入一个"待释放"列表；只有当扫描确认没有任何 hazard pointer 指向该节点时，才能安全释放。

### 4.2 关键机制

**记录与清除（Record & Clear）**：
- `hp_try_record(p, hp)`: 尝试将指针 `*p` 记录到 hazard pointer `hp` 中。使用 `READ_ONCE` 读取指针，写入 hazard pointer，执行 `smp_mb()`，然后再次读取 `*p` 验证指针未变。如果变化，返回 `HAZPTR_POISON` 表示竞争失败。
- `hp_record(p, hp)`: 循环调用 `hp_try_record` 直到成功（非 `HAZPTR_POISON`）。
- `hp_clear(hp)`: 清除 hazard pointer，前后都有内存屏障。

**延迟释放（Scan & Free）**：
- `hazptr_free_later()`: 将被删除节点加入线程本地的待释放列表。当列表长度达到阈值 `R` 时触发扫描。
- `hazptr_scan()`: 收集所有线程的所有 hazard pointer（非 NULL 者），排序后对待释放列表中的每个节点执行二分查找。若找到，说明仍有读者引用，放回待释放列表；否则安全释放。

### 4.3 读者重试（Restart Requirement）

Hazard pointers 的一个重要约束是：**读者在遍历过程中如果遇到并发删除（`hp_try_record` 返回 `HAZPTR_POISON`），必须从头重新遍历链表**。这是因为删除操作会将节点的 `next` 指针标记为 `HAZPTR_POISON`，而后续节点可能已经被释放。若读者继续沿着 `next` 前进，将访问已释放内存。

这种重试要求看似负担，但实际上是正确性的关键保障。它意味着 hazard-pointer 保护的算法在遍历期间不能对数据结构做任何修改，直到所有需要的 hazard pointer 都已成功记录。

### 4.4 性能与优势

与引用计数相比，hazard pointers 的性能有质的飞跃：
- **读者无共享写**：读者只写自己的 per-thread hazard pointer，不修改被遍历的对象
- **无原子 RMW**：每个对象遍历只需要内存屏障和条件检查，不需要 `atomic_cmpxchg`
- **正确性保证**：能够安全处理并发更新，不会出现 use-after-free

但 hazard pointers 仍不完美：每个遍历的对象都需要一次内存屏障，对于长链表仍有不可忽视的开销。此外，读者需要重试，且需要维护待释放列表，内存 footprint 大于 RCU。

### 4.5 C++26 标准

值得注意的是，hazard pointers 于 2023 年 6 月 17 日被 ISO C++ 标准委员会投票纳入 C++26。Facebook 的 Folly 库、MongoDB 的 WiredTiger 存储引擎等都使用了 hazard pointers。

---

## 5. Sequence Locks（顺序锁）

### 5.1 核心思想

Sequence locks 是 Linux 内核中广泛使用的一种机制，用于保护**读多写少**且读者需要看到一致状态的数据。与读写锁不同，顺序锁的读者**不排斥写者**。相反，它通过版本号检测并发更新，若检测到则强制读者重试。

关键组件是**序列号（sequence counter）**：
- 无更新时，序列号为偶数
- 写者开始更新时，将序列号加 1（变为奇数）
- 写者完成更新时，再加 1（变回偶数，且数值比原来大 2）

读者在访问数据前后分别读取序列号。如果任一读取值为奇数，或两次读取值不同，说明发生了并发更新，读者必须丢弃结果并重试。

### 5.2 实现解析

```c
typedef struct {
    unsigned long seq;
    spinlock_t lock;
} seqlock_t;

unsigned long read_seqbegin(seqlock_t *slp) {
    unsigned long s = READ_ONCE(slp->seq);
    smp_mb();
    return s & ~0x1UL;
}

int read_seqretry(seqlock_t *slp, unsigned long oldseq) {
    smp_mb();
    unsigned long s = READ_ONCE(slp->seq);
    return s != oldseq;
}

void write_seqlock(seqlock_t *slp) {
    spin_lock(&slp->lock);
    WRITE_ONCE(slp->seq, READ_ONCE(slp->seq) + 1);
    smp_mb();
}

void write_sequnlock(seqlock_t *slp) {
    smp_mb();
    WRITE_ONCE(slp->seq, READ_ONCE(slp->seq) + 1);
    spin_unlock(&slp->lock);
}
```

注意其中的内存屏障：
- `read_seqbegin` 中的 `smp_mb()` 确保序列号快照在读者临界区之前
- `read_seqretry` 中的 `smp_mb()` 确保读者临界区在第二次序列号读取之前
- `write_seqlock` 和 `write_sequnlock` 中的屏障确保写者临界区被严格包围在序列号变更之间

### 5.3 致命缺陷：无存在保证

本章明确指出，顺序锁应用于 Pre-BSD 路由表的实现是 **BUGGY** 的。原因在于：顺序锁**不提供指针遍历的存在保证**。读者可能在 `read_seqretry()` 检测到竞争之前，就已经解引用了一个已经被释放的指针，导致段错误（segmentation fault）。

具体来说：
1. 读者读取到指针 `rep` 指向节点 A
2. 写者删除节点 A 并释放内存
3. 读者解引用 `rep->addr` 或 `rep->iface`
4. 此时内存可能已经被重新分配为其他数据结构（如浮点数），导致崩溃
5. 读者直到调用 `read_seqretry()` 才发现竞争，但为时已晚

### 5.4 使用场景与内核应用

顺序锁适用于**静态分配的数据**或**不包含指针遍历**的场景。Linux 内核中的经典用例包括：
- `gettimeofday()` 系统调用中的时间校准数据（x86 的 vsyscall 实现）
- 内存热插拔中的 zone span 信息（`memory_hotplug.h`）
- 路径名遍历中检测并发的重命名操作

内核源码参考：
- `include/linux/seqlock.h`: 完整的 seqlock 实现，包括 `seqcount_t`（无锁的纯序列计数器）、`seqlock_t`（带锁的版本），以及各种变体（`_bh`、`_irq`、`_irqsave`）
- 内核中的 `read_seqbegin()` / `read_seqretry()` / `write_seqlock()` / `write_sequnlock()` 实现与 perfbook 中的教学版本基本一致，但增加了 lockdep 支持和更严格的类型检查

---

## 6. Read-Copy Update (RCU)

RCU 是本章最重磅的内容，占据了绝大部分篇幅。它被描述为一种允许读者执行与单线程程序完全相同的机器指令序列来遍历并发更新的链式数据结构的同步原语。

### 6.1 RCU 的核心 API

Linux 内核 RCU API 超过 100 个成员，但核心概念只需要 6 个原语：

| 角色 | 原语 | 作用 |
|------|------|------|
| 读者 | `rcu_read_lock()` | 开始 RCU 读端临界区 |
| 读者 | `rcu_read_unlock()` | 结束 RCU 读端临界区 |
| 读者 | `rcu_dereference()` | 安全加载 RCU 保护指针 |
| 更新者 | `rcu_assign_pointer()` | 安全发布（更新）RCU 保护指针 |
| 更新者 | `synchronize_rcu()` | 等待所有已存在的 RCU 读者完成 |
| 更新者 | `call_rcu()` | 异步注册回调，在 grace period 后执行 |

### 6.2 发布-订阅机制（Publish-Subscribe）

RCU 使用发布-订阅语义来安全地插入新数据：

**插入过程**：
1. 分配新结构（初始为未初始化的垃圾数据）
2. 初始化新结构的所有字段
3. 使用 `rcu_assign_pointer()`（本质是 `smp_store_release`）将指针指向新结构

`rcu_assign_pointer()` 确保编译器和 CPU 不会将初始化操作重排到指针发布之后。在弱序架构（如 ARM、RISC-V）上，这需要内存屏障或 store-release 指令；在 x86（TSO）上，普通 store 即可，但使用 `rcu_assign_pointer` 提供了可移植性和静态分析支持。

**读取过程**：
读者使用 `rcu_dereference()`（本质是带屏障的 `READ_ONCE` 或 C11 acquire load）加载指针。这确保编译器和 CPU 不会将指针的解引用重排到加载操作之前，从而避免看到未初始化的垃圾数据。

在大多数现代 CPU 上，`rcu_dereference()` 只生成一条 load 指令——与单线程代码完全相同！这正是 RCU 读者零开销（或接近零开销）的关键。

### 6.3 等待读者：Grace Period

RCU 最核心的机制是**等待所有已存在的读者完成**。这段时间称为 **grace period（宽限期）**。

**为什么不能用简单的引用计数？** 因为引用计数的扩展性极差（如第 3 节所示）。RCU 采用间接方式：不跟踪每个读者，而是等待所有 CPU/线程通过一个**静默状态（quiescent state）**。

在非抢占式 Linux 内核中，上下文切换就是一个天然的 quiescent state。因为 RCU 读者不允许阻塞（不允许睡眠或调用 `schedule()`），所以任何执行了上下文切换的 CPU 必然已经完成了所有之前的 RCU 读者临界区。

QSBR（Quiescent-State-Based Reclamation）就是基于这一思想的实现。一个玩具级实现极其简单：

```c
void synchronize_rcu(void) {
    for_each_online_cpu(cpu)
        sched_setaffinity(current->pid, cpumask_of(cpu));
}
```

通过依次在每个 CPU 上执行，强制每个 CPU 发生上下文切换，从而保证所有之前的读者已经完成。

### 6.4 多版本维护

RCU 允许并发读者和更新者看到数据的不同版本。这是 RCU 高性能的代价，也是其设计哲学的核心：

- 插入后：一些读者看到旧版本（空列表），一些看到新版本（包含新节点）
- 删除后：新读者看不到被删除节点，但已存在的读者仍可能引用它（直到 grace period 结束）

这导致一个令人惊讶的现象：一个读者遍历链表时，可能看到一个**从未在任何时刻实际存在过**的版本。例如链表初始为 {A,B,C}，读者遍历到 B 时，并发更新者删除了 A 并追加了 E。最终读者可能看到 {A,B,C,D,E}——即使这个序列从未作为一个整体存在过。

perfbook 指出，这在很多场景下是可以接受的：
- 互联网路由表的更新本身就有秒到分钟的延迟，内部几微秒的不一致无关紧要
- 单元素查找（而非全表遍历）是最常见的操作，无法检测到这种不一致
- 现实世界中的数据本来就是陈旧的（光速限制）

如果应用确实需要强一致性，可以结合 sequence lock（检测更新并重试）或使用更昂贵的同步机制。

### 6.5 Linux 内核 RCU API 家族

Linux 内核提供了五种 RCU "风味（flavors）"，以适应不同的工作负载和约束：

1. **RCU (Classic)**: 最常用的版本，读者通过 `rcu_read_lock()` / `rcu_read_unlock()` 标记临界区，不允许阻塞
2. **SRCU (Sleepable RCU)**: 允许读者睡眠，但需要每个使用点定义 `srcu_struct`，`srcu_read_lock()` 返回一个必须传递给 `srcu_read_unlock()` 的 cookie
3. **Tasks RCU**: 没有显式的读者标记，以自愿上下文切换作为 quiescent state。用于释放 tracing trampoline
4. **Tasks RCU Rude**: 更激进的变体，强制每个 CPU 上下文切换。可抢占区域和自愿切换都是 quiescent state
5. **Tasks RCU Trace**: 类似 SRCU，但读者标记开销极小（无内存屏障），以牺牲实时响应为代价

内核源码参考：
- `include/linux/rcupdate.h`: RCU 核心 API，`rcu_read_lock()` / `rcu_read_unlock()` 定义
- `kernel/rcu/tree.c`: Tree RCU 实现，`synchronize_rcu()`、`call_rcu()` 的核心逻辑
- `include/linux/srcu.h`: SRCU API
- `include/linux/list.h` 和 `include/linux/rculist.h`: RCU 保护的链表操作（`list_add_rcu()`、`list_del_rcu()`、`list_for_each_entry_rcu()` 等）

### 6.6 RCU 的使用场景

本章列举了 RCU 的 11 种使用模式：

1. **Pre-BSD 路由表**: 最经典的读多写少链表场景
2. **等待已有事物完成**: 如 NMI handler 的动态切换
3. **分阶段状态变更**: 维护操作期间让常规操作自动切换快慢路径
4. **只增列表**: 纯发布-订阅，无删除
5. **类型安全内存**: `SLAB_TYPESAFE_BY_RCU`，允许节点被释放后重新分配为同类型
6. **存在保证**: 读者临界区内对象保证存在，可安全获取 per-element lock
7. **轻量级垃圾回收**: 自动化的 reclamation timing
8. **只删列表**: 初始化后 universe 已知，只能删除
9. **准读写锁**: 替代 rwlock，提供性能、死锁免疫和实时延迟优势
10. **准引用计数**: 每个 `rcu_read_lock()` 可视为对所有 RCU 对象的批量引用
11. **准 MVCC**: 多版本并发控制，可容忍不一致数据

### 6.7 RCU 与 Reader-Writer Locking 的对比

RCU 和读写锁的关键语义差异：

- **读写锁**: 写者等待所有读者释放锁；后续读者等待写者完成。保证内部一致性，但可能看到对外部世界的陈旧数据。
- **RCU**: 写者只等待**已存在**的读者完成，不阻塞新读者。读者和写者真正并发运行，可能看到不一致数据，但响应更快。

性能对比（448-CPU x86 系统）：
- 单 CPU 上，RCU 比读写锁快一个数量级
- 192 CPU 上，RCU 比读写锁快**四个数量级**
- 即使 `CONFIG_PREEMPT=y` 的抢占式内核，RCU 仍有 7x 到 3 个数量级的优势

死锁免疫：RCU 读者原语不阻塞、不自旋、不回跳，执行时间是确定性的，因此不可能参与死锁循环。这也允许无条件地将 RCU 读者升级为更新者（获取 spinlock 并修改数据），而在读写锁中这会导致死锁。

---

## 7. 如何选择？

本章提供了两张详细的对比表。

### 7.1 高层概览

| 特性 | 引用计数 | Hazard Pointers | Sequence Locks | RCU |
|------|----------|-----------------|----------------|-----|
| 读者速度 | 慢且不可扩展 | 快且可扩展 | 快且可扩展 | 快且可扩展 |
| 内存开销 | 每个对象一个计数器 | 每个读者每个对象一个指针 | 无保护 | 无 |
| 保护时长 | 可长期 | 可长期 | N/A | 用户必须限制时长 |
| 遍历重试 | 对象被删时需重试 | 对象被删时需重试 | 任何更新都重试 | 永不 |
| 回收时机 | 立即 | 批量延迟 | N/A | 已有读者完成 + 批量延迟 |

### 7.2 详细对比

| 特性 | 引用计数 | Hazard Pointers | Sequence Locks | RCU |
|------|----------|-----------------|----------------|-----|
| 存在保证 | 复杂 | 是 | 否 | 是 |
| 读者/更新者并发前进 | 是 | 是 | 否 | 是 |
| 读者间争用 | 高 | 无 | 无 | 无 |
| 每临界区开销 | N/A | N/A | 两个 `smp_mb()` | 从无到两个 `smp_mb()` |
| 每对象遍历开销 | 原子 RMW + 屏障 + 缓存缺失 | 一个 `smp_mb()` | 无（但不安全） | 无（volatile load） |
| 读者前进进度保证 | 无锁 | 无锁 | 阻塞 | 有界等待自由（QSBR） |
| 引用获取 | 可能失败（条件式） | 可能失败（条件式） | 不安全 | 永不失败（无条件） |
| 内存占用 | 有界 | 有界 | 有界 | 无界（除非限制更新） |
| 回收前进进度 | 无锁 | 无锁 | N/A | 阻塞 |
| 自动回收 | 是 | 视用例 | N/A | 视用例 |
| 代码行数 | 94 | 79 | 79 | 73 |

关键结论：
- 如果需要**存在保证**且读者必须**无条件获取引用**，RCU 是最佳选择
- 如果需要**检测更新并重试**，且数据是静态分配的，sequence lock 简单有效
- 如果需要**长期持有引用**且内存占用必须严格有界，hazard pointer 更合适
- 如果读者**本身就在写被遍历的对象**，引用计数的开销可能被摊平

---

## 8. 更新操作怎么办？

本章最后简要讨论了更新操作的性能。对于读多写少场景，延迟处理技术已经很好。但对于写密集型负载：

- **OpLog**: 将更新记录到 per-thread log，批量应用。Silas Boyd-Wickizer 将其应用于 Linux 内核的路径名查找、VM 反向映射和 `stat()` 系统调用
- **Disruptor**: 基于单生产者-单消费者 FIFO 队列，最小化同步需求
- **完全分区/分片**: 如果数据可以完全分区，更新无需同步

---

## 9. 代码实现与测试

本章随附的代码实现了一个自包含的 C 程序集合，对应五种路由表实现：

| 文件 | 说明 |
|------|------|
| `route_seq.c` | 顺序版本（基准） |
| `route_refcnt.c` | 引用计数版本（**故意包含 bug，会崩溃**） |
| `route_seqlock.c` | 顺序锁版本（**可能崩溃，因无存在保证**） |
| `route_hazptr.c` | Hazard Pointer 版本（正确） |
| `route_rcu.c` | RCU 玩具实现版本（正确） |
| `common.h` | 共享宏和原子操作封装 |
| `Makefile` | 一键编译和测试 |

### 9.1 构建与运行

```bash
make all   # 编译所有示例
make test  # 运行测试（引用计数版本预期崩溃）
make clean # 清理
```

所有二进制文件以 `.out` 结尾，自动被 `.gitignore` 忽略。

### 9.2 关键代码结构

**Hazard Pointer 实现要点**：
- 使用固定大小的全局 hazard pointer 数组 `HP[H]`
- 每个线程使用其中 `K=2` 个槽位
- `hp_try_record()` 使用双重检查避免 ABA 问题：先读指针，记录到 HP，执行 `smp_mb()`，再验证指针未变
- 删除时将节点加入 per-thread 的 `rlist`，达到阈值后调用 `hazptr_scan()` 进行垃圾回收

**RCU 玩具实现要点**：
- 使用 per-thread 的原子嵌套计数器 `rcu_reader_nesting[]`
- `rcu_read_lock()` / `rcu_read_unlock()` 只是简单的原子递增/递减
- `synchronize_rcu()` 等待所有计数器归零（即所有读者退出）
- `call_rcu()` 将回调加入链表，`rcu_invoke_callbacks()` 在 grace period 后执行释放
- 这虽然极其简单，但完整演示了 RCU 的"等待已有读者"核心语义

---

## 10. 与 Linux 内核源码的对应关系

| perfbook 概念 | Linux 内核源码位置 | 说明 |
|---------------|-------------------|------|
| `seqlock_t` | `include/linux/seqlock.h` | 内核 seqlock 实现，包含 `seqcount_t` 和相关变体 |
| `read_seqbegin` / `read_seqretry` | `include/linux/seqlock.h` | 与教学实现基本一致，增加了 KCSAN 和 lockdep 支持 |
| `rcu_read_lock` / `rcu_read_unlock` | `include/linux/rcupdate.h` | 抢占式内核中可能涉及 `preempt_disable`/`enable` |
| `rcu_assign_pointer` | `include/linux/rcupdate.h` | 基于 `smp_store_release` 或 `__rcu` 标记 |
| `rcu_dereference` | `include/linux/rcupdate.h` | 基于 `READ_ONCE` + volatile，Alpha 上需内存屏障 |
| `synchronize_rcu` / `call_rcu` | `kernel/rcu/tree.c` | Tree RCU 的核心 grace period 引擎 |
| `list_add_rcu` / `list_del_rcu` | `include/linux/rculist.h` | RCU 保护的标准双向链表操作 |
| `hlist_add_head_rcu` | `include/linux/rculist.h` | 用于哈希桶的线性链表 RCU 操作 |
| `SLAB_TYPESAFE_BY_RCU` | `include/linux/slab.h` | slab 分配器的 RCU 类型安全内存支持 |
| `kref` | `include/linux/kref.h` | 内核引用计数基础 |

内核中的 seqlock 实现比 perfbook 的教学版本复杂得多：
- 支持 `seqcount_t`（纯计数器，无锁）和 `seqlock_t`（计数器+锁）
- 提供 `_bh`、`_irq`、`_irqsave` 变体以适配不同的中断上下文
- 集成 KCSAN（Kernel Concurrency Sanitizer）以检测数据竞争
- 集成 lockdep 以验证写者序列化

内核中的 RCU 实现更是工程奇迹：
- **Tree RCU**: 使用层次化的 CPU 树结构来高效检测 quiescent state
- **Tiny RCU**: 用于单 CPU 或极小系统的极简实现
- **SRCU**: 允许睡眠，使用 per-CPU 计数器数组
- **Tasks RCU**: 专门用于 BPF 和 tracing 子系统
- **rcutorture**: 极为严苛的测试框架，通过注入故障和随机延迟验证正确性

---

## 11. 对用户笔记疑问的回应

用户笔记 `~/data/vn/docs/concurrent/perfbook/overview.md` 中提到的疑问：

### 11.1 "RCU 和 memory barrier 的问题，都是两章，这是重点"

第 9 章确实深入覆盖了 RCU 和 memory barrier 的交互。关键收获：
- `rcu_assign_pointer()` 和 `rcu_dereference()` 的本质就是**发布-订阅语义**的内存序控制
- `rcu_assign_pointer()` 使用 release 语义（或更强），确保初始化在指针发布前完成
- `rcu_dereference()` 使用 acquire 语义（或编译器屏障 + volatile），确保指针解引用在加载之后
- 在 x86 上这些通常是无开销的（普通 store/load 即可），但在 ARM/RISC-V 等弱序架构上需要显式屏障或 special instructions
- RCU 的 grace period 保证本身也构成了强大的内存序约束：grace period 开始前的写对所有 grace period 结束后的读者可见

### 11.2 "单生产者/单消费者有 lockless queue 吗？多个 consumer 和 producer 呢？"

这个问题**未在本章直接回答**，但本章的技术为构建无锁队列奠定了基础：
- **单生产者/单消费者队列**：可以完全无锁，只需要 `READ_ONCE` / `WRITE_ONCE` 和适当的内存屏障。这是第 9 章 "What About Updates?" 节提到的 Disruptor 模式的基础
- **多生产者/多消费者队列**：通常需要原子操作（如 `atomic_cmpxchg`）或本章介绍的延迟处理技术。例如：
  - 使用 hazard pointers 保护节点，允许无锁出队
  - 使用 RCU 保护队列结构，更新者通过 `call_rcu` 安全释放被弹出的节点
  - Michael-Scott 无锁队列就是基于 CAS 和 hazard pointers 思想的经典实现

如需深入研究无锁队列，可以参考 `perfbook` 后续的数据结构章节（第 12 章）。

### 11.3 "锁是靠什么保证这些的？是靠 lock 的代码实现，还是靠硬件实现的？"

本章通过引用计数的失败案例间接回答了这个问题：
- 锁（和 RCU、memory barrier）的语义**最终由硬件内存模型和编译器共同保证**
- 软件层面的 `spin_lock()` 在 x86 上可能只是一条 `lock` 前缀的原子指令，在 ARM 上可能是 LDXR/STXR 循环加上屏障
- RCU 的 `rcu_read_lock()` 在 `CONFIG_PREEMPT=n` 的内核中曾经是**完全空操作**（零指令），仅靠上下文切换的语义来界定 grace period；后来因为编译器优化会导致 page fault 被重排到 RCU 临界区内，Linus Torvalds 在 2019 年强制加入了 `barrier()`（仍无指令，但约束编译器优化）
- 因此，同步原语的保证是**软硬件协同**的结果：软件通过 barrier/atomic API 向编译器和 CPU 传达意图，硬件通过其内存一致性模型提供底层保证

### 11.4 "形式化验证指的是什么？"

本章末尾的 "RCU Related Work" 节介绍了大量形式化验证工作：
- CBMC（C Bounded Model Checker）用于机械证明 Tree RCU 的正确性
- Nidhugg 状态less model checker 用于验证更大范围的 RCU 行为
- LKMM（Linux Kernel Memory Model）litmus tests 用于验证 RCU 的内存序保证
- 这些工具验证的核心是：RCU 的实现是否满足"grace period 等待所有已有读者"的形式化规范

---

## 12. 总结

第 9 章 "Deferred Processing" 展示了并发编程中最优雅的一类技术：通过**延迟危险操作**（特别是内存释放）来换取读者的**零开销或极低开销**。从引用计数的失败教训，到 hazard pointers 的巧妙翻转，再到 sequence locks 的版本号检测，最终到 RCU 的 grace period 哲学，这些技术构成了现代高性能并发系统（尤其是 Linux 内核）的基石。

核心收获可以概括为以下几点：

1. **不要在共享数据上做不必要的写操作**：引用计数之所以失败，是因为读者不得不修改共享对象。Hazard pointers 和 RCU 都将读者的写操作移到了 per-thread 的私有数据或完全消除。

2. **存在保证是昂贵的，但有时不可或缺**：Sequence locks 不提供存在保证，因此无法安全遍历指针。RCU 和 hazard pointers 通过不同的机制提供存在保证，代价分别是内存占用和重试。

3. **RCU 的核心简单到令人惊讶**：就是"等待已有读者完成"。但其应用却极为丰富（读写锁替代、引用计数、存在保证、MVCC、垃圾回收...），这正印证了 Butler Lampson 将其归类为"easy concurrency"。

4. **选择合适的工具**：没有银弹。RCU 最适合读多写少且可容忍轻微不一致的场景；hazard pointers 适合需要长期引用且内存占用敏感的场景；sequence locks 适合静态数据的一致性快照；引用计数则适合对象生命周期明确且争用不激烈的场景。

5. **Linux 内核是这些技术的最佳教科书**：从 `seqlock.h` 中简洁而强大的顺序锁实现，到 `kernel/rcu/tree.c` 中工程化的 Tree RCU，内核代码将本章的理论付诸实践。阅读内核源码时，关注 `rcu_dereference()` 和 `rcu_assign_pointer()` 的使用模式，以及 `synchronize_rcu()` / `call_rcu()` 的调用点，可以快速掌握这些技术在实际系统中的用法。

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
