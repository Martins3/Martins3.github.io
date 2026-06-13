# 第 8 章：Data Ownership（数据所有权）

FIXME 总体来说，这章原文写的也比较简单，引用的内容都是之前的内容。
## 1. 章节核心思想

Data Ownership 是避免同步开销的最简单方式之一：将数据划分给各个线程（或 CPU），使得某一块数据只被一个线程访问和修改。它同时覆盖了并行设计的"三大支柱"：
- **Partitioning（划分）**：按线程划分数据
- **Batching（批处理）**：本地操作批量执行
- **Weakening（弱化同步）**：将消除同步操作推向逻辑极端

正因为其简单直观，数据所有权被广泛使用，甚至新手都会本能地使用它。本章没有引入全新示例，而是回顾前面章节的代码，从不同视角阐释数据所有权的各种形式。

---

## 2. 各节详细分析

### 2.1 Multiple Processes（多进程）

这是数据所有权的逻辑极端：每个线程拥有自己独立的地址空间。通过 shell 的 `&` 和 `wait`，或者 C 的 `fork()`/`wait()`，可以创建完全独立的进程。

```c
compute_it 1 > compute_it.1.out &
compute_it 2 > compute_it.2.out &
wait
cat compute_it.1.out
cat compute_it.2.out
```

由于进程之间不共享内存（除非显式使用 `shmget()` 或 `mmap()`），进程内的所有数据都被该进程"所有"。这种方式几乎完全消除了同步开销，兼具极端的简单性和最优的性能。

**同步残留**：仅剩的同步是 shell 的 `&`（创建线程）和 `wait`（等待线程结束）。如果进程显式共享内存，则需要额外的同步机制（System V 信号量、消息队列、UNIX domain socket、文件锁等）。

**代码实现**：见本目录 `process_ownership.c`，使用 `fork()` 创建两个子进程，各自累加私有计数器，父进程通过 `waitpid()` 等待完成，无需任何锁或原子操作。

```bash
make process_ownership.out
./process_ownership.out
# 输出：
# process 1: counter = 10000000
# process 2: counter = 10000000
# parent: both children completed, no shared data needed sync
```

这种朴素形式的并行绝非"作弊"或"逃避责任"，而是快速、可扩展、易于编程和维护的优雅方案。开发者可以把节省下来的时间用于单线程优化，或者将复杂并行模式应用于代码中不适用此方案的部分。

### 2.2 Partial Data Ownership and pthreads（部分数据所有权）

在共享内存并行程序中，数据所有权可以更加细腻：线程可以读取其他线程拥有的数据，但不允许修改。第 5 章的并发计数器就大量使用了这种模式。

典型的例子是 per-thread 统计计数器：
- `inc_count()` 只更新本线程的 `counter` 实例
- `read_count()` 读取但不修改所有线程的 `counter` 实例

这种模式在内核中极为常见。例如，某个 CPU 可能被允许在禁用中断时读取自己的 per-CPU 变量；另一个 CPU 只有在持有对应的 per-CPU 锁时才能读取第一个 CPU 的 per-CPU 变量；而该 CPU 自己只有在同时禁用中断并持有 per-CPU 锁时才能更新这些变量。这种安排可以看作一种 reader-writer 锁的变体，允许每个 CPU 以极低开销访问自己的变量集。

纯数据所有权同样常见且有用，例如 per-thread 内存分配器缓存（见第 5 章资源分配器缓存），每个线程的缓存完全私有。

**代码实现**：见 `partial_ownership.c`。每个线程只写自己的 `counters[tid]`，而 `read_count()` 遍历所有线程的计数器进行读取。注意这里没有使用任何锁，因为：
- 每个线程只写自己的 cache line（通过 `__attribute__((__aligned__(CACHE_LINE_SIZE)))` 避免 false sharing）
- `read_count()` 读取时可能看到稍旧的数据，但对于统计计数器这是可以接受的

```bash
make partial_ownership.out
./partial_ownership.out
# 输出：
# expected total: 4000000
# read_count:     4000000
```

### 2.3 Function Shipping（函数托运）

前面一节描述的是一种弱形式的数据所有权：线程"伸出手"去访问其他线程的数据。这可以看作把数据带到需要它的函数。另一种替代方案是把函数发送到数据所在的地方。

第 5 章的信号盗窃限制计数器（Signal-Theft Limit Counter）就是一个典型例子：
- `flush_local_count_sig()` 是信号处理函数，扮演"被托运的函数"
- `pthread_kill()` 在 `flush_local_count()` 中发送信号——即"托运函数"
- 然后等待被托运的函数执行完毕

这种托运函数需要与并发的 `add_count()`/`sub_count()` 交互，增加了实现复杂度，但避免了直接跨线程访问数据。

**代码实现**：见 `function_shipping.c`。主线程创建一个工作线程，该线程累加自己的私有计数器。当工作完成后，主线程通过 `pthread_kill(tid, SIGUSR1)` 向工作线程发送信号。信号处理函数 `flush_sig_handler` 在工作线程的上下文中执行，读取该线程的 `thread_counter` 并写回全局变量 `signal_result`。主线程等待 `signal_received` 标志后，将结果累加到 `global_count`。

```bash
make function_shipping.out
./function_shipping.out
# 输出：
# global_count after collection: 5000000
# function shipping demo completed
```

这种模式的本质是：不直接触碰别人的数据，而是请数据所有者在自己的上下文中执行操作。除了 POSIX 信号，还可以使用 System V 消息队列、共享内存邮箱、UNIX domain socket、TCP/IP、RPC 等机制实现函数托运。

### 2.4 Designated Thread（指定线程）

前面各节描述的是让每个线程保留自己的数据副本或数据分片。本节描述的是功能分解方法：指定一个特殊线程拥有执行某项工作所需数据的权利。

第 5 章的"最终一致性计数器"提供了例子。该实现有一个指定线程运行 `eventual()` 函数，它周期性地将 per-thread 计数拉取到全局计数器中，使得对全局计数器的访问最终会收敛到实际值。

**关键点**：`eventual()` 线程并非"拥有" per-thread 的 `counter` 变量（那是各工作线程的私有数据），而是拥有**访问这些数据的权利**。这类似于部分数据所有权的概念。而 `eventual()` 线程确实拥有自己的数据：它内部定义的局部变量 `t` 和 `sum`。

Linux 内核中的 kernel thread（通过 `kthread_create()` 和 `kthread_run()` 创建）也是指定线程的典型例子。例如：
- `khungtaskd`：检测 D 状态任务的守护线程
- `audit_send_list`：审计日志发送线程
- `irq_thread`：中断处理线程

这些内核线程通常负责某个特定的子系统功能，拥有该功能相关的数据结构访问权。

**代码实现**：见 `designated_thread.c`。主线程创建一个"eventual"线程，它每隔 1ms 扫描所有工作线程的 per-thread 计数器并累加到 `global_count`。工作线程只管累加自己的计数器，完全不参与全局同步。由于 `stopflag` 被主线程和工作线程并发访问，代码使用 C11 的 `<stdatomic.h>` 中的 `atomic_int` 和 `atomic_load`/`atomic_store`/`atomic_fetch_add` 保证正确的同步语义。

```bash
make designated_thread.out
./designated_thread.out
# 输出：
# expected total: 2000000
# global_count:   2000000
```

### 2.5 Privatization（私有化）

私有化是将共享数据转换为被特定线程私有的数据，从而提升性能和可扩展性。

**哲学家就餐问题**是最直观的例子：
- 经典版本：5 位哲学家围坐，每对相邻哲学家之间有一把叉子，最多 2 人可以同时用餐
- 私有化版本：再提供 5 把叉子，每位哲学家拥有自己的私有叉子对，5 人可同时用餐，且减少了某些疾病的传播

本目录 `privatization.c` 实现了这个对比测试：
- 经典方案使用 5 个 pthread_mutex_t 作为共享叉子，通过不对称的加锁顺序（最后一位哲学家先拿右边）避免死锁
- 私有化方案完全不需要锁

```bash
make privatization.out
./privatization.out
# 输出示例：
# classic:     166.970 ms (with 5 mutexes)
# privatized:  0.144 ms (zero synchronization)
# speedup:     1159.51x
```

私有化也有代价。例如第 5 章的简单限制计数器中，线程可以读取彼此的数据，但只能更新自己的数据。`read_count()` 中的跨线程求和循环如果去掉，就得到了更高效的纯数据所有权，但 `read_count()` 的精度会下降。

部分私有化也是可能的，只需要比完全共享更少的同步。第 15 章（Deferred Processing）将通过 RCU 等机制引入时间维度上的数据所有权，提供安全地将公共数据结构私有化的方法。

### 2.6 Other Uses of Data Ownership（其他应用场景）

数据所有权在以下环境中作为一等公民存在：

1. **MPI 和 BOINC**：所有消息传递环境天然就是数据所有权模型
2. **Map-Reduce**：map 阶段数据分片，reduce 阶段聚合
3. **客户端-服务器系统**：RPC、Web 服务、后端数据库
4. **Shared-nothing 数据库系统**
5. **Fork-join 系统**：独立的 per-process 地址空间
6. **基于进程的并行**：如 Erlang 语言
7. **私有变量**：C 语言栈上的 auto 变量在线程环境中默认私有
8. **GPGPU 并行线性代数**：许多算法天然适合数据划分
9. **操作系统网络栈**：如 IX 操作系统，每个网络连接（flow）分配给特定线程

---

## 3. Linux 内核源码分析

### 3.1 Per-CPU 变量：内核数据所有权的基石

Linux 内核通过 `DEFINE_PER_CPU(type, name)` 和 `DECLARE_PER_CPU(type, name)` 宏定义 per-CPU 变量。这些变量被放置在特殊的 per-CPU 段中，每个 CPU 拥有独立的副本。

在 `include/linux/percpu-defs.h` 中：

```c
#define DECLARE_PER_CPU(type, name) \
    DECLARE_PER_CPU_SECTION(type, name, "")

#define DEFINE_PER_CPU(type, name) \
    DEFINE_PER_CPU_SECTION(type, name, "")
```

在 x86 架构上，`arch/x86/include/asm/percpu.h` 使用段寄存器（gs/fs）实现高效的 per-CPU 访问：

```c
#define PER_CPU_VAR(var)    %__percpu_seg:(var)__percpu_rel
```

访问接口包括：
- `this_cpu_read(pcp)` / `this_cpu_write(pcp, val)`：当前 CPU 读写自己的副本
- `per_cpu(var, cpu)`：访问指定 CPU 的副本（通常需要配合 CPU hotplug 锁）
- `raw_cpu_read(pcp)` / `raw_cpu_write(pcp, val)`：无抢占检查的原始访问

这些宏在 `include/linux/percpu-defs.h` 中定义：

```c
#define this_cpu_read(pcp)     __pcpu_size_call_return(this_cpu_read_, pcp)
#define this_cpu_write(pcp, val) __pcpu_size_call(this_cpu_write_, pcp, val)
```

**为什么 per-CPU 变量访问不需要锁？** 因为每个 CPU 只访问自己的副本，天然满足数据所有权。其他 CPU 如果需要读取，通常需要持有 `cpus_read_lock()` 或使用 `READ_ONCE()` 等机制。

### 3.2 vmstat：部分数据所有权的典型例子

在 `mm/vmstat.c` 中，内核定义了 per-CPU 的 vm 事件计数器：

```c
DEFINE_PER_CPU(struct vm_event_state, vm_event_states) = { {0}};
EXPORT_PER_CPU_SYMBOL(vm_event_states);
```

更新时（`include/linux/vmstat.h`）：

```c
static inline void count_vm_event(enum vm_event_item item)
{
    this_cpu_inc(vm_event_states.event[item]);
}
```

这里 `this_cpu_inc` 直接对当前 CPU 的 `vm_event_states` 进行自增，无需任何锁。这是典型的部分数据所有权：每个 CPU 拥有写权限，而读取（如 `all_vm_events()`）需要遍历所有 CPU：

```c
void all_vm_events(unsigned long *ret)
{
    cpus_read_lock();
    sum_vm_events(ret);
    cpus_read_unlock();
}
```

`sum_vm_events` 内部使用 `per_cpu(vm_event_states, cpu)` 访问其他 CPU 的数据，此时需要 `cpus_read_lock` 保护，防止 CPU 热插拔导致 per-CPU 数组变化。

### 3.3 SLUB 分配器：per-CPU cache 的纯数据所有权

`mm/slub.c` 中的 SLUB 分配器使用 `slub_percpu_sheaves` 结构实现 per-CPU 快速路径：

```c
struct slub_percpu_sheaves {
    ...
};

pcs = per_cpu_ptr(s->cpu_sheaves, cpu);
```

每个 CPU 有自己的 sheaf（捆），分配和释放时优先操作本地 sheaf，使用 `spin_trylock` 保护（快速路径失败时才走全局慢路径）。这是纯数据所有权在内存分配中的经典应用，避免了全局锁竞争。

### 3.4 内核线程：Designated Thread 的实现

`kernel/kthread.c` 提供了 `kthread_create()` 和 `kthread_run()`：

```c
struct kthread_create_info {
    int (*threadfn)(void *data);
    void *data;
    ...
    struct task_struct *result;
    struct completion *done;
};
```

内核中的 designated thread 实例：
- `kernel/hung_task.c`：`kthread_run(watchdog, NULL, "khungtaskd")`
- `kernel/irq/manage.c`：`kthread_create(irq_thread, new, "irq/%d-%s", ...)`
- `kernel/kprobes.c`：`kthread_run(kprobe_optimizer_thread, NULL, ...)`

这些线程各自负责特定的内核子系统功能，拥有相关的数据结构和处理逻辑，其他内核代码通过工作队列、信号量或 completion 等机制与它们交互，而不是直接操作它们的数据。

---

## 4. 用户笔记中的疑问分析

在 `~/data/vn/docs/concurrent/` 目录下的笔记中，用户提出了几个与数据所有权密切相关的问题。本章内容对这些问题的解答如下：

### 4.1 "per_cpu 读取为什么不需要 READ_ONCE？"

用户笔记中提到了 `all_vm_events` 中的读取：

```c
struct vm_event_state *this = &per_cpu(vm_event_states, cpu);
```

用户问："这里为什么不需要 READ_ONCE？"

**解答**：
- 在 `sum_vm_events()` 中，调用者已经持有 `cpus_read_lock()`，这意味着在遍历期间不会有 CPU 被热插拔，per-CPU 数组的地址是稳定的
- 更重要的是，被读取的 `vm_event_states` 是目标 CPU 的**私有数据**。目标 CPU 通过 `this_cpu_inc()` 更新自己的计数器，而 `this_cpu_inc` 在 x86 上通常是单个原子指令或基于段寄存器的直接内存操作
- 对于读取端，`per_cpu(vm_event_states, cpu)` 获取的是该 CPU 对应内存区域的地址。虽然目标 CPU 可能正在并发更新该区域，但由于是统计计数器，读取一个稍旧或稍新的值都是可接受的（ eventual consistency ）
- 如果场景要求严格的顺序一致性（例如读取后要根据该值做决策），则需要 `READ_ONCE()` 或更重的屏障。但对于统计累加，`READ_ONCE` 的语义保证并非必需

本章的"部分数据所有权"一节明确说明：线程可以读取其他线程的数据，但不修改它。这种读取通常不需要与写入方同步，除非对数据新鲜度有严格要求。

### 4.2 "global_node_page_state 为什么用 atomic？"

```c
unsigned long global_node_page_state_pages(enum node_stat_item item)
{
    long x = atomic_long_read(&vm_node_stat[item]);
    ...
}
```

**解答**：
- `vm_node_stat` 是**全局**统计数组，不是 per-CPU 的。多个 CPU 可能并发更新同一节点的同一统计项，因此必须使用原子操作
- 这与 `vm_event_states`（per-CPU）形成鲜明对比：后者通过数据所有权消除了竞争，前者由于数据必须全局共享，不得不回退到原子操作
- 这正是本章强调的核心思想：如果能把数据所有权划分好，就可以避免原子操作的开销；如果必须共享，就要承受同步成本

### 4.3 "READ_ONCE 只是保证 read 不会消失，还需要保证指令重排吗？"

用户笔记中追问 READ_ONCE 的语义：是否还需要保证指令重排？

**解答**：
- `READ_ONCE` 在 Linux 内核中的实现（对应本章 `api-pthreads.h` 中的定义）使用了 `volatile` 强制编译器不做优化消除，也不对访问进行重排
- `READ_ONCE` / `WRITE_ONCE` 提供的是**编译器屏障**级别的保证：防止编译器重排内存访问，并保证访问不会被优化掉
- 但它**不提供 CPU 内存序保证**。如果需要防止 CPU 乱序执行，需要配合 `smp_rmb()`、`smp_wmb()`、`smp_mb()` 等内存屏障，或者使用 `smp_load_acquire()` / `smp_store_release()`
- 本章中的 `count_stat_eventual.c` 示例就混合使用了 `READ_ONCE`/`WRITE_ONCE` 和 `smp_load_acquire()`/`smp_store_release()`，前者处理编译器层面，后者处理 CPU 层面

### 4.4 "什么叫 weakly ordered atomic operations？"

用户笔记中提到对 weakly ordered atomic operations 的困惑。

**解答**：
- "Weakly ordered" 指的是弱内存序模型，如 ARM、RISC-V、POWER 等架构。在这些架构上，内存访问的可见顺序可能与程序顺序不同，除非显式使用屏障或具有顺序语义的指令
- x86 是强序架构（strongly ordered），大多数内存访问默认具有 TSO（Total Store Order）语义，因此普通程序员较少直接面对弱序问题
- Linux 内核的 `smp_load_acquire()` / `smp_store_release()`、`smp_rmb()` / `smp_wmb()` / `smp_mb()` 等抽象就是为了屏蔽不同架构的内存序差异
- 在数据所有权模型中，由于大部分操作是本地 CPU 对自己的数据的操作，天然避免了跨 CPU 的序问题。只有当执行跨 CPU 读取（如 `read_count()` 读取其他线程的计数器）或指定线程汇总数据时，才需要考虑内存序和屏障

---

## 5. 总结

数据所有权或许是现存最被低估的同步机制。当使用得当时，它提供了无与伦比的简单性、性能和可扩展性。也许正是因为它的简单，才让它没有得到应有的尊重。

本章通过六种视角展示了数据所有权：

1. **多进程**：完全独立的地址空间，零共享零同步
2. **部分数据所有权**：写私有，读共享，适用于统计计数器等场景
3. **函数托运**：不直接触碰他人数据，而是请数据所有者在自己的上下文中执行操作
4. **指定线程**：将特定功能和数据所有权赋予特定线程
5. **私有化**：将共享资源复制为私有资源，消除竞争
6. **应用场景**：MPI、Map-Reduce、客户端-服务器、Erlang、GPGPU、网络操作系统等

Linux 内核是数据所有权的集大成者：per-CPU 变量、`vmstat` 统计、SLUB per-CPU cache、内核线程等处处体现了这一思想。理解数据所有权，是理解高性能并发编程和内核设计的关键一步。

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
