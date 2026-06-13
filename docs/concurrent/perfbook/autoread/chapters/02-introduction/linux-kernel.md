# 结合 Linux 内核源码分析第 2 章

## 1. 工作划分 (Work Partitioning) —— 调度器负载均衡

Linux 内核调度器是工作划分的经典实现。

### 1.1 CFS 调度与负载均衡

文件：`kernel/sched/fair.c`

内核中的 `load_balance()` 函数实现了跨 CPU 的任务迁移，以解决不均匀划分导致的负载失衡问题：

```c
/* kernel/sched/fair.c 中负载均衡相关代码 */
static unsigned long __read_mostly max_load_balance_interval = HZ/10;

/* active_load_balance_cpu_stop 由 CPU stopper 执行，将任务从繁忙 CPU 推送到空闲 CPU */
static int active_load_balance_cpu_stop(void *data)
{
    // ... 将任务从 busiest CPU 推送到目标 CPU
}
```

调度器会周期性检查各 runqueue 的负载，通过 `active_load_balance` 和 `idle_balance` 等机制在 CPU 之间迁移任务，这正是自动化的"负载均衡"实现。

### 1.2 工作队列 (Workqueue)

文件：`kernel/workqueue.c`

内核的工作队列将工作项 (work item) 划分为 per-CPU 队列和 unbound 队列：

- **bound workqueue**：工作项绑定到特定 CPU，减少缓存失效
- **unbound workqueue**：工作项可在任意 CPU 上执行，用于可能睡眠的任务

这是工作划分与缓存局部性权衡的工程实践。

## 2. 并行访问控制 (Parallel Access Control)

Linux 内核提供了丰富的同步机制，对应本章提到的各种协调方式。

### 2.1 锁机制

| 机制 | 适用场景 | 内核实现 |
|------|---------|---------|
| 互斥锁 | 进程上下文长期持有 | `struct mutex` (`kernel/locking/mutex.c`) |
| 自旋锁 | 短期持有、不可睡眠 | `raw_spinlock_t` (`include/linux/spinlock.h`) |
| 读写锁 | 读多写少 | `rwlock_t`, `percpu-rwsem` |
| RCU | 读极多写极少 | `rcu_read_lock()` (`kernel/rcu/`) |

### 2.2 顺序锁 (Seqlock)

文件：`include/linux/seqlock.h`

seqlock 是"显式时序"访问控制的一个变体，读者无锁读取，通过序列号检测写者干扰：

```c
do {
    seq = read_seqbegin(&lock);
    /* 读取数据 */
} while (read_seqretry(&lock, seq));
```

这对应本章提到的 "explicit timing" 类别，但更加灵活和安全。

## 3. 资源划分与复制 (Resource Partitioning and Replication)

这是 Linux 内核中应用最广泛的并行优化策略。

### 3.1 Per-CPU 变量

文件：`include/linux/percpu.h`, `arch/x86/include/asm/percpu.h`

```c
DEFINE_PER_CPU(int, my_counter);

/* 当前 CPU 的本地计数器加 1 */
this_cpu_inc(my_counter);

/* 读取当前 CPU 的计数器 */
int val = this_cpu_read(my_counter);
```

x86 架构通过段寄存器或普通内存寻址实现高效的 per-CPU 访问，无需原子操作或锁。

### 3.2 Per-CPU Counter

文件：`lib/percpu_counter.c`, `include/linux/percpu_counter.h`

`percpu_counter` 是本章"资源划分"思想的完美体现：

```c
struct percpu_counter {
    raw_spinlock_t lock;
    s64 count;
    s32 __percpu *counters;  /* 每个 CPU 的本地计数器 */
};
```

**快速路径**：每个 CPU 修改自己的本地计数器（无锁、无原子操作）
**慢速路径**：当本地计数器超过 `batch` 阈值时，用自旋锁将累积值刷到全局 `count`

```c
void percpu_counter_add_batch(struct percpu_counter *fbc, s64 amount, s32 batch)
{
    s64 count;
    unsigned long flags;

    count = this_cpu_read(*fbc->counters);
    do {
        if (unlikely(abs(count + amount) >= batch)) {
            raw_spin_lock_irqsave(&fbc->lock, flags);
            count = __this_cpu_read(*fbc->counters);
            fbc->count += count + amount;
            __this_cpu_sub(*fbc->counters, count);
            raw_spin_unlock_irqrestore(&fbc->lock, flags);
            return;
        }
    } while (!this_cpu_try_cmpxchg(*fbc->counters, &count, count + amount));
}
```

内核中的 `vm_committed_as`（虚拟内存提交计数）、ext4 文件系统的块计数等都使用 `percpu_counter`。

### 3.3 NUMA 内存划分

文件：`mm/page_alloc.c`

内核将物理内存按 NUMA 节点划分：

```c
/* 每个节点有自己的空闲页列表 */
struct pglist_data *node_data[MAX_NUMNODES];
```

内存分配优先从本地 NUMA 节点分配，减少跨节点访问延迟。这是"按 NUMA 节点划分资源"的体现。

### 3.4 Read-Mostly 数据的复制

RCU (Read-Copy-Update) 是"复制 read-mostly 资源"的极致实现：

文件：`kernel/rcu/`

- 读者完全无锁
- 写者创建副本并修改，然后通过原子指针切换发布
- 旧副本在所有读者退出后释放

这对应本章"复制 frequently accessed read-mostly resources"的策略。

## 4. 顺序设计的限制 (Existing Sequential Designs)

### 4.1 内核中的顺序 API 改造

本章提到 hash table API 返回精确计数在并行环境下会成为瓶颈。Linux 内核中类似的问题广泛存在：

**进程计数**：早期 Linux 用全局变量记录进程数，每次 fork/exit 都要原子修改。现代内核使用近似计数或 per-CPU 计数。

**文件偏移量**：`file->f_pos` 的读写需要锁保护（`file_pos_read/file_pos_write`），因为顺序文件模型假设单线程访问。

### 4.2 从全局到分布式的演变

| 子系统 | 顺序/粗粒度实现 | 并行优化实现 |
|--------|----------------|-------------|
| 定时器 | 全局定时器中断 | per-CPU `tick_cpu_device` |
| 软中断 | 全局 `__softirq_pending` | per-CPU `irq_stat.__softirq_pending` |
| 网络 backlog | 全局队列 | per-CPU `softnet_data` |
| 页表缓存 | 全局 TLB | per-CPU TLB, invlpg |

## 5. 性能 vs 顺序优化

本章强调"并行化只是性能优化的一种"。Linux 内核中也有大量顺序优化远超并行化的例子：

### 5.1 Radix Tree -> XArray

文件：`lib/xarray.c`

内核将老旧的 Radix Tree 替换为 XArray，单线程性能显著提升，而不需要引入额外的并行化。

### 5.2 哈希表替代链表

`pid_hash` 用哈希表替代链表查找进程，将 `find_task_by_vpid()` 从 O(n) 降到 O(1)。

## 6. 本章在 Linux 内核中的映射总结

```
perfbook 第 2 章概念          Linux 内核实现
-----------------------------------------------------------------
工作划分 (Work Partitioning)   -> 调度器负载均衡、workqueue
并行访问控制                   -> mutex, spinlock, RCU, seqlock
资源划分与复制                 -> percpu vars, percpu_counter, NUMA
与硬件交互                     -> CPU topology, cache geometry (sched domain)
顺序设计限制                   -> f_pos, 全局计数器改造
顺序优化优先于并行化           -> XArray, hash table, radix tree
```

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
