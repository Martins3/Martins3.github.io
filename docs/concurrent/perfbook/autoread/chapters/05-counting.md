# Counting

## 一、章节概述与核心问题

Counting（计数）是计算机最基本的操作之一，但在大规模共享内存多处理器上实现高效、可扩展的并发计数却出奇地困难。本章以计数这个简单问题为切入点，展示了并行编程中的核心挑战：如何在保证正确性的前提下，获得接近单线程的性能和线性扩展性。

本章开篇提出了四个典型的计数应用场景：

1. **网络包统计问题**：每秒数百万次更新，但每五秒才读取一次，允许近似值。
2. **近似结构分配限制**：需要快速判断是否超过限制，允许一定误差。
3. **精确结构分配限制**：必须精确知道何时达到零，用于资源释放。
4. **可移除 I/O 设备引用计数**：平时不关心具体数值，只在卸载时需要精确知道是否为零。

这四个问题的共同点是：更新操作必须极快，而读取操作的频率和精度要求各不相同。这种不对称性正是设计高效并发计数算法的关键出发点。

---

## 二、为什么并发计数不简单

### 2.1 非原子计数：速度快但丢失计数

最简单的实现使用普通的加载-修改-存储操作：

```c
unsigned long counter = 0;

static __inline__ void inc_count(void)
{
    WRITE_ONCE(counter, READ_ONCE(counter) + 1);
}
```

这个实现在作者六核 x86 笔记本上，2.85 亿次调用中丢失了约 87% 的计数。原因是加载和存储是两个独立操作，多线程并发时会发生经典的 "read-modify-write" 竞争。即使编译器将 `READ_ONCE` + 加 1 + `WRITE_ONCE` 优化为单条 x86 `add` 指令，由于这不是原子操作（除非使用 `_Atomic` 变量），仍然存在竞争窗口。

在测试程序 `count_nonatomic.out` 中，4 个线程各执行 1000 万次递增，预期 4000 万，实际只得到约 1131 万（约 71.7% 丢失）。这验证了简单的非原子计数在并发下完全不可用。

### 2.2 原子计数：正确但扩展性差

使用原子操作可以保证正确性：

```c
atomic_t counter = ATOMIC_INIT(0);

static __inline__ void inc_count(void)
{
    atomic_inc(&counter);
}
```

但性能代价巨大：在单线程情况下就比非原子版本慢 20 倍以上，且随着 CPU 数量增加性能急剧下降。根本原因在于 **缓存行颠簸（cache-line bouncing）**：每次原子递增都需要使包含该计数器的缓存行在所有 CPU 之间传播，如图 5.2 所示。每个 CPU 必须轮流获得该缓存行的独占所有权才能执行递增，这导致大量时间浪费在缓存一致性协议上。

测试程序 `count_atomic.out` 显示 4 线程 4000 万次递增耗时约 0.338 秒。虽然正确，但在更多核心上性能会线性恶化。

---

## 三、统计计数器（Statistical Counters）

### 3.1 设计思想：数据所有权（Data Ownership）

统计计数器的特点是更新极其频繁，读取极少。解决方案是 **为每个线程分配独立的计数器**，每个线程只更新自己的计数器，读取时汇总所有线程的值。加法满足交换律和结合律，因此汇总顺序不影响结果。

这是 **Data Ownership** 模式的典型应用：每个线程拥有并只修改自己的数据，无需任何同步。

### 3.2 基于数组的实现

```c
DEFINE_PER_THREAD(unsigned long, counter);

static __inline__ void inc_count(void)
{
    unsigned long *p_counter = &__get_thread_var(counter);
    WRITE_ONCE(*p_counter, *p_counter + 1);
}

static __inline__ unsigned long read_count(void)
{
    int t;
    unsigned long sum = 0;

    for_each_thread(t)
        sum += READ_ONCE(per_thread(counter, t));
    return sum;
}
```

关键点：
- `WRITE_ONCE` 和 `READ_ONCE` 用于阻止编译器优化。例如，编译器可能将两次 `read_count()` 调用合并，认为两次读取同一位置的结果相同。`READ_ONCE` 明确告知编译器 "这段内存可能在任何时候被其他实体修改"。
- 写入和读取都不需要原子操作，因为每个元素只属于一个线程。

### 3.3 基于 _Thread_local 的实现

数组实现有线程数量上限。C11 的 `_Thread_local` 存储类提供了更优雅的方案：

```c
unsigned long _Thread_local counter = 0;
unsigned long *counterp[NR_THREADS] = { NULL };
unsigned long finalcount = 0;
DEFINE_SPINLOCK(final_mutex);
```

每个线程有自己独立的 `counter` 变量。`counterp[]` 数组用于让其他线程能够访问某个线程的计数器。`finalcount` 累积已退出线程的计数，因为线程退出后其 `_Thread_local` 变量会消失。

```c
static inline unsigned long read_count(void)
{
    spin_lock(&final_mutex);
    sum = finalcount;
    for_each_thread(t)
        if (counterp[t] != NULL)
            sum += READ_ONCE(*counterp[t]);
    spin_unlock(&final_mutex);
    return sum;
}
```

`read_count()` 需要加锁，原因是为了防止与线程退出并发：如果一个线程正在退出（正在将 `counter` 加到 `finalcount` 并将 `counterp[]` 设为 NULL），而另一个线程正在遍历 `counterp[]`，可能访问到已经释放的内存。

测试程序 `count_perthread.out` 显示 4 线程 4000 万次递增耗时几乎为 0.000 秒（实际约几毫秒，计时精度限制），说明更新侧性能几乎与非原子版本相同。

### 3.4 最终一致性实现（Eventually Consistent）

如果读取需要极快的速度，可以进一步弱化一致性保证。最终一致性计数器引入一个全局变量 `global_count`，由独立的后台线程定期将各 per-thread 计数汇总到全局变量中。读者直接读取 `global_count`。

```c
void *eventual(void *arg)
{
    while (READ_ONCE(stopflag) < 3) {
        sum = 0;
        for_each_thread(t)
            sum += READ_ONCE(per_thread(counter, t));
        WRITE_ONCE(global_count, sum);
        poll(NULL, 0, 1);  /* 1ms */
    }
    return NULL;
}
```

这个方案的代价是：读取的值可能不是精确的当前值，但在更新停止后会收敛到正确值。测试程序 `count_eventual.out` 在 4 线程下同样获得接近零的更新时间，读取时间也是常数级别。

**smp_load_acquire / smp_store_release 的作用**：`count_cleanup()` 和 `eventual()` 之间使用 `smp_store_release` 和 `smp_load_acquire` 来确保 `global_count` 的所有更新在 `count_cleanup()` 返回前对其他代码可见。这是典型的发布-订阅模式，建立了 happens-before 关系。

---

## 四、近似限制计数器（Approximate Limit Counters）

### 4.1 问题与核心设计

限制计数器需要维护一个当前值，并在达到上限时拒绝增加操作。简单地将上限除以线程数分配给每个线程的方案不适用于 "一个线程分配、另一个线程释放" 的场景（如内存分配器）。

核心思想是 **部分分区（partial partitioning）**：每个线程不仅有本地计数器 `counter`，还有一个本地上限 `countermax`。线程可以在本地自由增减，直到 `counter` 达到 `countermax`，此时需要将部分计数 "上交" 到全局计数器。

具体地，当线程的 `counter` 达到 `countermax` 时：
1. 获取全局锁
2. 将 `counter` 的一半加到 `globalcount`
3. 从 `counter` 减去相应数量
4. 释放锁
5. 继续本地操作

这大大减少了全局锁的竞争频率：每 `countermax` 次操作才需要一次全局锁。

### 4.2 变量关系与约束

```
globalcount + globalreserve <= globalcountmax
sum(all threads' countermax) <= globalreserve
each thread's counter <= countermax
```

`globalreserve` 记录所有线程 `countermax` 的总和，用于保守估计资源使用。在 `add_count()` 中，`globalreserve` 按 "所有线程的 counter 都满了" 来考虑，因此可能提前返回失败（假阴性）。在 `sub_count()` 中，它按 "所有线程的 counter 都空了" 来考虑，同样可能保守地返回失败。

### 4.3 globalize_count 与 balance_count

```c
static __inline__ void globalize_count(void)
{
    globalcount += counter;
    counter = 0;
    globalreserve -= countermax;
    countermax = 0;
}

static __inline__ void balance_count(void)
{
    countermax = (globalcountmax - globalcount - globalreserve) / num_threads;
    globalreserve += countermax;
    counter = countermax / 2;
    if (counter > globalcount)
        counter = globalcount;
    globalcount -= counter;
}
```

`globalize_count()` 将线程的本地状态转移到全局，不改变总计数值。
`balance_count()` 重新分配本地配额。将 `counter` 设为 `countermax / 2` 的目的是让线程既有增量空间也有减量空间，可以双向使用 fastpath。

这是 **Parallel Fastpath** 设计模式的典型示例：常见路径无锁、无原子操作，偶尔走慢路径获取全局锁。

---

## 五、精确限制计数器（Exact Limit Counters）

### 5.1 原子限制计数器

近似计数器的问题是可能过早地拒绝合法操作。精确限制计数器允许将计数跑到精确的上下限。

关键挑战：当一个线程需要从另一个线程 "窃取" 计数时，必须同时原子地操作 `counter` 和 `countermax`。解决方案是将两者打包到一个原子变量中：

```c
atomic_t counterandmax;  /* high 16 bits: counter, low 16 bits: countermax */
```

使用 CAS（compare-and-swap）循环来更新：

```c
do {
    old = atomic_read(&counterandmax);
    split_counterandmax(old, &c, &cm);
    if (cm - c < delta)
        goto slowpath;
    new = merge_counterandmax(c + delta, cm);
} while (atomic_cmpxchg(&counterandmax, old, new) != old);
```

`flush_local_count()` 使用 `atomic_xchg()` 原子地将某线程的 `counterandmax` 清零并获取旧值，从而将其计数转移到全局。即使此时该线程的 fastpath 正在运行，CAS 会失败并重试，保证正确性。

### 5.2 信号窃取（Signal-Theft）实现

如果原子指令仍然太慢（如在某些老旧的 x86 上），可以使用信号处理程序来实现无原子操作的窃取。核心是一个状态机：

- **IDLE**：初始状态
- **REQ**：慢路径请求窃取，设置状态为 REQ 并发送信号
- **ACK**：信号处理程序发现 fastpath 正在运行，设为 ACK
- **READY**：fastpath 完成或信号处理程序直接设为 READY

信号处理程序在目标线程的上下文中运行，可以直接访问其内存而无需原子操作（因为信号和被中断线程在同一个 CPU 上运行，如果迁移则线程一起迁移）。

```c
void flush_local_count_sig(int sig)
{
    if (theft != THEFT_REQ)
        return;
    if (counting) {
        theft = THEFT_ACK;
    } else {
        smp_store_release(&theft, THEFT_READY);
    }
}
```

`smp_store_release` 确保：如果 fastpath 看到 `theft == READY`，那么它一定也能看到 fastpath 中对 `counter` 的更新。

---

## 六、Linux 内核中的对应实现

### 6.1 percpu_counter

Linux 内核中的 `percpu_counter`（`lib/percpu_counter.c`）是本章思想的工程化实现，广泛应用于文件系统（ext2/ext3 超级块计数）、网络子系统等。

```c
struct percpu_counter {
    raw_spinlock_t lock;
    s64 count;
    s32 __percpu *counters;
};
```

- `count`：全局计数值
- `counters`：每个 CPU 的本地计数器（percpu 变量）

更新操作：

```c
void percpu_counter_add_batch(struct percpu_counter *fbc, s64 amount, s32 batch)
{
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

这与 perfbook 中的 "batching" 思想完全一致：每个 CPU 在本地累积计数，只有当绝对值超过 `batch` 时才刷新到全局。`batch` 默认值是 `max(32, nr_cpus * 2)`，即 CPU 越多，batch 越大，减少全局锁竞争。

读取操作有两种：

1. `percpu_counter_read()`：直接返回 `fbc->count`，可能遗漏本地计数（类似本章的 "approximate" 读取）。
2. `percpu_counter_sum()`：加锁并汇总所有 CPU 的本地计数（类似本章的精确读取）。

CPU 热插拔处理：当 CPU 下线时，`percpu_counter_cpu_dead()` 会将该 CPU 的本地计数刷新到全局：

```c
static int percpu_counter_cpu_dead(unsigned int cpu)
{
    list_for_each_entry(fbc, &percpu_counters, list) {
        raw_spin_lock(&fbc->lock);
        pcount = per_cpu_ptr(fbc->counters, cpu);
        fbc->count += *pcount;
        *pcount = 0;
        raw_spin_unlock(&fbc->lock);
    }
}
```

这与 perfbook 中 `count_unregister_thread()` 将线程计数转移到 `finalcount` 的逻辑完全一致。

### 6.2 内核中的 limit counter 应用

内核中的 `percpu_counter_limited_add` 实现了带限制的原子添加：

```c
bool __percpu_counter_limited_add(struct percpu_counter *fbc,
                                  s64 limit, s64 amount, s32 batch)
{
    /* fast check without full sum */
    if (abs(count + amount) <= batch &&
        ((amount > 0 && fbc->count + unknown <= limit) || ...)) {
        this_cpu_add(*fbc->counters, amount);
        return true;
    }
    /* slowpath: full sum with lock */
    ...
}
```

这体现了 "partial partitioning across time"：大部分时候在本地快速路径运行，偶尔走全局慢路径检查限制。

---

## 七、性能总结与核心教训

| 算法 | 精确? | 更新 (ns) | 1 CPU 读 | 8 CPU 读 | 64 CPU 读 |
|------|-------|-----------|----------|----------|-----------|
| stat (数组) | - | 6.3 | 294 | 303 | 315 |
| stat_eventual | - | 6.4 | 1 | 1 | 1 |
| end (_Thread_local) | - | 2.9 | 301 | 6309 | 147594 |
| lim (简单限制) | N | 3.2 | 435 | 6678 | 156175 |
| lim_atomic | Y | 19.7 | 513 | 7085 | 199957 |
| lim_sig | Y | 4.7 | 519 | 6805 | 120000 |

核心教训：

1. **分区（Partitioning）** 提升性能和扩展性。完全分区（统计计数器）或部分分区（限制计数器）都是关键。
2. **批处理（Batching）** 减少昂贵的同步操作频率。per-thread 计数器累积到一定数量再刷新全局。
3. **弱化同步（Weakening）** 在可接受时降低一致性要求。最终一致性让读操作无需锁。
4. **只读路径保持只读**：不要向只读路径引入写操作。`count_end.c` 的读锁竞争就是反例，可以用 RCU 优化（`count_end_rcu.c`）。
5. **延迟（Delay）** 有时能带来巨大收益。`eventual()` 线程的延迟汇总是典型的以延迟换吞吐量。

---

## 八、对用户疑问的解答

用户的并发编程笔记中记录了大量关于 `READ_ONCE` / `WRITE_ONCE`、memory barrier、memory model 的疑问。本章虽然没有直接解答所有问题，但提供了非常好的实践案例：

### 8.1 READ_ONCE / WRITE_ONCE 的必要性

本章反复使用 `READ_ONCE` 和 `WRITE_ONCE`，并明确解释了原因：
- 编译器可能将连续读取同一变量的操作合并（common subexpression elimination），认为值不会改变。
- 编译器可能将待写入的位置作为临时存储，在最终写入前先写入垃圾值。
- 本章的 Quick Quiz 明确提到：连续两次调用 `read_count()` 可能被内联和优化，导致第二次调用不重新读取内存。

这直接回应了用户笔记中的问题："为什么 `inode->i_link` 的访问需要 `READ_ONCE`"、"为什么 `__list_add` 中需要添加 `WRITE_ONCE`"。根本原因是：**只要存在多线程访问同一内存的可能，编译器优化就可能破坏语义，必须显式标记**。

### 8.2 memory barrier 的基本用法

本章中 `count_stat_eventual.c` 使用了 `smp_load_acquire` 和 `smp_store_release`：

```c
count_cleanup():
    WRITE_ONCE(stopflag, 1);
    while (smp_load_acquire(&stopflag) < 3)
        poll(NULL, 0, 1);

eventual():
    if (READ_ONCE(stopflag))
        smp_store_release(&stopflag, stopflag + 1);
```

这是一个典型的 **Release-Acquire 模式**：
- `smp_store_release` 保证其之前的所有内存操作（包括对 `global_count` 的写入）在该 store 之前完成。
- `smp_load_acquire` 保证其之后的所有内存操作（包括读取 `global_count`）在该 load 之后开始。

这建立了 happens-before 关系：如果 `count_cleanup()` 看到 `stopflag >= 3`，那么 `eventual()` 线程在此之前对 `global_count` 的所有更新都对 `count_cleanup()` 可见。

这为用户 "acquire and release" 的笔记提供了具体案例。

### 8.3 数据竞争（data race）

`count_nonatomic.c` 中的非原子递增就是典型的 data race：两个线程同时读取同一值，各自加 1，然后存储，导致一个更新被覆盖。测试程序显示超过 70% 的计数丢失。

而 per-thread 计数器中，由于每个线程只写自己的变量，不存在 data race，因此无需原子操作。这说明了 **避免 data race 的方法不一定是加锁，也可以是通过数据所有权消除共享**。

### 8.4 尚未完全解答的疑问

本章没有直接涉及以下问题：
- `barrier()` 和 `__sync_synchronize` 的区别（第 14 章更详细）
- x86 vs ARM 的 memory model 差异（第 3 章和第 14 章）
- RCU 的具体实现（第 9 章）
- `cmpxchg128` 的必要性（需要看具体使用场景）

---

## 九、代码文件说明

| 文件 | 说明 |
|------|------|
| `count_nonatomic.c` | 非原子计数，演示 data race 导致计数丢失 |
| `count_atomic.c` | 原子计数，演示正确但慢 |
| `count_perthread.c` | per-thread 统计计数器，Data Ownership 模式 |
| `count_eventual.c` | 最终一致性计数器，独立线程后台汇总 |
| `count_limit.c` | 近似限制计数器，Parallel Fastpath 模式 |
| `Makefile` | 一键编译和测试 |

运行 `make test` 即可看到各算法的输出对比。

---

## 十、结论

Perfbook 第 5 章通过计数这个看似简单的问题，深入展示了并行编程的复杂性。从非原子计数的数据竞争，到原子计数的缓存行颠簸，再到 per-thread 计数的数据所有权、批处理和最终一致性，每一步都是在特定约束下的权衡。

Linux 内核的 `percpu_counter` 是这些理论在真实系统中的杰出应用。理解这些计数算法，就是理解了并行系统中 "分区、批处理、弱化" 这三大性能优化原则的本质。

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
