# Tools of the Trade

## 一、章节内容综述

本章标题为 "Tools of the Trade"，即"行业工具"。它并非介绍高层次的并发框架，而是从最基础的同步原语和并行编程工具入手，为后续章节奠定技术基础。全章可分为四大部分：

1. **脚本语言级别的并行**（Shell、make -j）
2. **POSIX 多进程与多线程原语**（fork/wait、pthread_create/pthread_join、mutex、rwlock）
3. **原子操作与编译器/内存屏障**（GCC classic atomics、C11 atomics、READ_ONCE/WRITE_ONCE、barrier、smp_mb）
4. **Linux 内核中的对应实现**（spinlock、atomic_t、per-CPU 变量）
5. **工具选择策略**

本章的核心思想可以用书中开篇的一句话概括："You are only as good as your tools, and your tools are only as good as you are." 并发编程的复杂度很大程度上取决于开发者对基础工具的掌握深度。

---

## 二、各节详细分析

### 2.1 Scripting Languages

书中首先指出，最简单的并行方式往往被忽视：Linux shell 脚本。通过后台执行（`&`）和 `wait` 命令，可以在不编写任何 C 代码的情况下实现并行。

```bash
compute_it arg1 > result1 &
compute_it arg2 > result2 &
wait
cat result1 result2
```

`make -j4` 也是同样的道理，利用 Makefile 的依赖图自动并行化编译步骤。

**但是脚本并行有根本限制**：
- `fork()`/`exec()` 创建进程开销大（约 480 微秒）
- 进程间数据共享需通过文件 I/O 或管道，代价高
- 可靠的同步原语同样依赖文件 I/O
- 脚本语言本身执行速度慢

因此脚本并行只适合**粗粒度并行**——每个任务至少执行几十毫秒甚至更长时间。如果任务粒度更细，就需要更底层的工具。

---

### 2.2 POSIX Multiprocessing

#### 2.2.1 Process Creation and Destruction

`fork()` 是 POSIX 多进程的基石。它在调用时"返回两次"：一次在父进程（返回子进程 PID），一次在子进程（返回 0）。

**关键事实**：fork 出的父子进程**不共享内存**。书中用 `forkjoinvar.c` 示例证明：子进程修改全局变量 `x`，父进程看到的 `x` 仍是原值。这是因为 `fork()` 使用写时复制（COW, Copy-On-Write）机制，子进程获得父进程地址空间的独立副本。

> Linux 内核源码中，`fork()` 的实现在 `kernel/fork.c` 中。`copy_process()` 函数负责复制进程描述符（`task_struct`），而内存映射通过 `dup_mm()` 复制，页表项标记为只读，任一进程写入时触发缺页中断，内核再分配新物理页。这就是 COW 的本质。

`wait()` 系统调用用于等待子进程结束。书中将其包装为 `waitall()` 函数，循环调用 `wait()` 直到 `ECHILD` 错误（没有更多子进程）。

#### 2.2.2 POSIX Thread Creation and Destruction

当需要**共享内存**的细粒度并行时，`pthread_create()` 成为首选。

```c
pthread_create(&tid, NULL, mythread, NULL);
pthread_join(tid, &vp);
```

与 `fork()` 不同，`pthread_create()` 创建的新线程与父线程共享地址空间。书中示例 `pcreate.c` 证明：子线程修改 `x = 1`，父线程在 `pthread_join()` 后看到 `x == 1`。

**Data Race（数据竞争）警告**：当多个线程同时读写同一变量，且至少一个是写操作时，就构成 data race。C 语言标准对 data race 的行为未作任何保证——结果是未定义的（undefined behavior）。

> 这里引出一个经典问题：Linux 内核中存在大量看似 data race 的代码，为什么可以工作？书中回答：内核使用了精心选择的 C 语言超集，包含 GNU 扩展（如内联汇编、`asm volatile`、特殊的内存屏障原语），并且内核不支持某些对 data race 特别敏感的架构（如 16 位总线的 32 位指针系统）。即便如此，内核社区仍在努力消除 data race，正如 Jon Corbet 的文章 "ACCESS_ONCE()" 所讨论的。

#### 2.2.3 POSIX Locking

互斥锁（`pthread_mutex_t`）是最基础的同步工具。`pthread_mutex_lock()` 获取锁，`pthread_mutex_unlock()` 释放锁。同一时刻只有一个线程能持有锁。

书中 `lock.c` 示例展示了两个关键场景：

**场景 A：相同锁**
`lock_reader()` 和 `lock_writer()` 都使用 `lock_a`。由于互斥，reader 只能看到 writer 完成后的最终值（`x = 0` 或 `x = 3`，取决于谁先获得锁）。

**场景 B：不同锁**
`lock_reader()` 用 `lock_a`，`lock_writer()` 用 `lock_b`。两者互不阻塞，reader 可以看到 writer 产生的中间值（`x = 0, 1, 2, 3`）。

这说明一个核心原则：**锁与数据的对应关系决定了并发行为**。每个共享数据应有一个对应的锁（或锁层次结构），"一个全局锁走天下"虽然简单，但通常无法获得良好的性能和扩展性。

代码中使用了 `READ_ONCE(x)` 和 `WRITE_ONCE(x)` 来访问共享变量。书中解释道：这两个宏限制编译器不要对共享变量做有害优化（如加载融合、存储消除、 invented loads/stores），但不约束 CPU 的重排序行为。

> Linux 内核中，`READ_ONCE` 和 `WRITE_ONCE` 定义在 `include/asm-generic/rwonce.h`：
> ```c
> #define __READ_ONCE(x) (*(const volatile __unqual_scalar_typeof(x) *)&(x))
> #define __WRITE_ONCE(x, val) (*(volatile typeof(x) *)&(x) = (val))
> ```
> 它们的核心技巧是**通过 `volatile` 类型的强制转换**，告诉编译器这次访问是"特殊"的，不能与其他访问合并或消除。

#### 2.2.4 POSIX Reader-Writer Locking

读写锁（`pthread_rwlock_t`）允许多个 reader 并发持有锁，但 writer 独占。这在"读多写少"场景下理论上比互斥锁更具扩展性。

然而书中通过 `rwlockscale.c` 的实测数据给出了一个令人警醒的结论：**读写锁的扩展性并不理想**，尤其是在临界区很小的情况下。

测试在 224 核 Xeon 系统（448 逻辑 CPU）上运行。结果显示：
- 当临界区为 1 微秒时，扩展性极差，448 线程的总吞吐量可能不到单线程的 1/448
- 即使临界区为 10000 微秒，吞吐量也比理想情况下降了约 10%

**为什么？** 所有 reader 在获取读锁时都必须更新底层的 `pthread_rwlock_t` 数据结构。如果 448 个线程同时尝试获取读锁，它们必须串行修改这个结构。最不幸的线程要等待前面 447 个线程完成更新。这种**缓存行乒乓（cache-line bouncing）**效应随着 CPU 数量增加而恶化。

> 这与 Linux 内核中 `qrwlock`（队列读写锁）的设计动机一致。内核实现者意识到简单计数器式的 rwlock 在大量 CPU 上性能崩溃，因此引入了基于队列的公平锁机制。

---

### 2.3 Atomic Operations

#### 2.3.1 GCC Classic Atomics

GCC 提供了 `__sync_*` 系列原子操作，如：
- `__sync_fetch_and_add()` / `__sync_add_and_fetch()`
- `__sync_fetch_and_sub()` / `__sync_sub_and_fetch()`
- `__sync_bool_compare_and_swap()` / `__sync_val_compare_and_swap()`

这些操作保证了原子性，并且隐含了**完整的内存顺序语义**（full memory ordering）。这既是优点也是缺点：对于不需要强顺序的场景，这种保守实现会带来不必要的性能损失。

#### 2.3.2 C11 Atomics

C11 标准引入了 `<stdatomic.h>`，提供了 `atomic_load()`、`atomic_store()`、`atomic_fetch_add()`、`atomic_compare_exchange_strong()` 等。

关键改进是**显式内存顺序参数**：
- `memory_order_relaxed`：只保证原子性，不保证顺序
- `memory_order_acquire` / `memory_order_release`：配对使用，形成 happens-before 关系
- `memory_order_seq_cst`：顺序一致性，最强保证

没有显式参数时，默认是 `memory_order_seq_cst`。

#### 2.3.3 Modern GCC Atomics

现代 GCC 提供了 `__atomic_*` 内建函数，如 `__atomic_load_n()`、`__atomic_store_n()`、`__atomic_thread_fence()`。它们与 C11 语义类似，但可以作用于普通非原子类型，对 legacy 代码更友好。

---

### 2.4 Per-Thread Variables

POSIX 提供 `pthread_key_create()` / `pthread_setspecific()` 等 API 实现线程本地存储（TLS）。GCC 扩展 `__thread` 和 C11 标准 `_Thread_local` 提供了更简洁的语法。

线程本地变量是高性能并发编程的关键技巧之一：如果一个计数器被频繁更新但很少读取，可以给每个线程分配一个私有计数器，读取时再汇总，从而完全避免锁竞争。这是第 5 章"统计计数器"的核心思想。

> Linux 内核中使用 `DEFINE_PER_CPU(type, name)` 定义 per-CPU 变量，用 `this_cpu_ptr()` 访问当前 CPU 的实例。内核中的 per-CPU 变量通过巧妙的链接器脚本和 CPU 上线流程初始化，避免了运行时全局同步。

---

### 2.5 Alternatives to POSIX Operations

本节将视角从用户空间转向 Linux 内核，介绍了内核中的对应实现。

#### 2.5.1 Thread Creation and Control

内核中没有 `fork()` 的等价物（因为所有内核任务共享内核地址空间），但提供了：
- `kthread_create()` 创建内核线程
- `kthread_should_stop()` 建议线程停止
- `kthread_stop()` 等待线程停止

#### 2.5.2 Locking

内核锁 API 的核心是 `spinlock_t`：
- `spin_lock_init()` 初始化
- `spin_lock()` 获取（忙等或阻塞，取决于环境）
- `spin_trylock()` 非阻塞尝试获取
- `spin_unlock()` 释放

> 在真正的 Linux 内核中，`spin_lock()` 在获取锁前会关闭当前 CPU 的抢占（preemption），临界区内不会发生上下文切换。这是 spinlock 与 pthread mutex 的关键区别之一：mutex 允许持有者在被阻塞时睡眠，而 spinlock 持有者必须保持运行（或至少保持不被抢占地运行）。

#### 2.5.3 Accessing Shared Variables

这是本章技术含量最高、对现代并发编程也最重要的部分。

**背景**：直到 2011 年 C11 标准发布，C 语言标准对并发访问共享变量的行为没有任何规定。然而并发 C 代码早在 1985 年就已经存在。这意味着早期开发者实际上是在"危险地生活"。

**编译器优化的危害**：

编译器在优化时默认假设变量不会被其他线程并发访问。基于这一假设，它可以合法地进行以下"恶作剧"（书中称为 "shared-variable shenanigans"）：

1. **Load tearing（加载撕裂）**：用多条指令加载一个变量。例如用两次 16 位加载实现 32 位指针加载。如果另一个线程 concurrently 将指针设为 NULL，结果可能是一个"野生指针"（高低位混合新旧值）。

2. **Store tearing（存储撕裂）**：类似地，用多条指令存储。书中引用了一个真实案例：GCC 在某些 CPU 上会将 64 位 volatile 存储拆分为两个 32 位存储。

3. **Load fusing（加载融合）**：编译器复用之前加载的结果，不再重新从内存读取。书中示例：
   ```c
   while (!need_to_stop)
       do_something_quickly();
   ```
   编译器可能将循环展开并只检查一次 `need_to_stop`，导致其他线程设置该变量后循环永不退出。

4. **Store fusing（存储融合）**：编译器发现对同一变量的连续两次存储之间没有读取，可能删除第一次存储。

5. **Invented loads（凭空加载）**：编译器为了优化可能引入额外的加载。例如消除临时变量后，对 `global_ptr` 的加载次数从 1 次变为 3 次。

6. **Invented stores（凭空存储）**：编译器可能将已写入变量当作临时寄存器使用，或把 `if (condition) a = 1; else do_work();` 改写为先无条件写 1 再在条件不满足时写回 0。

7. **Code reordering（代码重排）**：编译器为了利用 CPU 流水线，可能重排无关指令。

8. **Dead-code elimination（死代码消除）**：如果编译器认为某次加载结果未被使用，或某变量只写不读，可能完全消除这些访问。

**解决方案演进**：

- **volatile 方案**：在 C11 之前，`volatile` 是主要工具。它保证访问会生成对应的机器指令，且 aligned machine-sized volatile 访问不会被撕裂。volatile 访问之间不会被编译器重排序。但它**不约束 CPU 重排序**，也不约束 volatile 与非 volatile 访问之间的顺序。

- **READ_ONCE / WRITE_ONCE**：Linux 内核开发的现代方案。通过 `volatile` 强制转换实现，明确标记并发访问点。它们防止上述 load/store tearing、fusing、invention 等问题，但仍不防止 CPU 重排序。

- **barrier()**：编译器屏障。
  ```c
  #define barrier() __asm__ __volatile__("" ::: "memory")
  ```
  空汇编模板 + `"memory"` clobber 告诉编译器：这条指令可能任意修改内存。因此编译器不会将内存访问跨越 barrier 重排。但它**不生成任何 CPU 指令**，因此不约束 CPU。

  > 内核源码 `include/linux/compiler.h` 中定义：
  > ```c
  > # define barrier() __asm__ __volatile__("": : :"memory")
  > ```
  > 注释特别说明 `"The \"volatile\" is due to gcc bugs"`。

- **smp_mb() / __sync_synchronize()**：完整的内存屏障，同时约束编译器和 CPU。在 x86 上通常实现为 `mfence` 或 `lock addl $0, -4(%esp)`；在 ARM64 上为 `dsb sy`。

- **smp_load_acquire() / smp_store_release()**：更轻量的单向屏障，配对使用可建立 happens-before 关系，通常比 `smp_mb()` 性能更好。

  > x86 上 `smp_store_release` 的实现令人意外：
  > ```c
  > #define __smp_store_release(p, v) do { barrier(); WRITE_ONCE(*p, v); } while (0)
  > ```
  > 因为 x86 的 TSO 模型已经保证了 store-store 和 load-load 的顺序，只需要阻止编译器重排序即可。
  >
  > ARM64 上则必须使用专门的 release 指令：
  > ```c
  > asm volatile ("stlr %w1, %0" : "=Q" (*__p) : "rZ" (*(__u32 *)__u.__c) : "memory");
  > ```

**避免 Data Race 的策略**：

书中总结了可以安全使用普通（plain）加载/存储的场景：
1. 变量仅由特定 CPU/线程修改，其他线程读取。写方用 `WRITE_ONCE()`，拥有者可用 plain load，其余用 `READ_ONCE()`。
2. 变量仅在持有特定锁时修改，锁外读取。写方用 `WRITE_ONCE()`，持锁方可用 plain load，其余用 `READ_ONCE()`。
3. 变量由特定 CPU/线程在持锁时修改。写方用 `WRITE_ONCE()`，拥有者和持锁者可用 plain load，其余用 `READ_ONCE()`。
4. 变量仅由某 CPU/线程和同 CPU 上的信号/中断处理程序访问。屏蔽中断的代码可用 plain access，其余用 `READ_ONCE/WRITE_ONCE`。
5. 处理程序总是恢复其修改的变量值。类似上一条，但其他代码可用 plain load（因为处理程序不留下永久修改）。

其他情况下，共享变量的并发访问必须使用 `READ_ONCE/WRITE_ONCE` 或更强原语。

---

### 2.6 The Right Tool for the Job: How to Choose?

书中给出了一个实用的决策树：

1. 能顺序写就不并行
2. 能用 shell 脚本并行就不用 C
3. 能用 `fork()/wait()` 就不用线程（如果进程开销可接受，约 80 微秒）
4. 能用 POSIX 线程 + 锁/原子操作就不用更复杂机制（线程开销通常亚微秒级）
5. 如果 POSIX 线程开销仍然太大，才考虑后续章节介绍的延迟处理（deferred processing）技术

---

## 三、Linux 内核源码对照分析

### 3.1 READ_ONCE / WRITE_ONCE

Linux 内核 `include/asm-generic/rwonce.h`：

```c
#define __READ_ONCE(x)  (*(const volatile __unqual_scalar_typeof(x) *)&(x))

#define READ_ONCE(x)                            \
({                                              \
    compiletime_assert_rwonce_type(x);          \
    __READ_ONCE(x);                             \
})

#define __WRITE_ONCE(x, val)                    \
do {                                            \
    *(volatile typeof(x) *)&(x) = (val);        \
} while (0)
```

核心机制是**强制类型转换为 volatile 指针后解引用**。注意外层的 `const`（读时）或 `volatile`（写时）只影响这一次访问，不改变变量本身的类型。这比直接声明变量为 `volatile` 更精确——不会妨碍编译器对锁保护区域内访问的优化。

内核中还有 `READ_ONCE_NOCHECK`，用于栈回溯等场景，绕过 KASAN/KCSAN 检查。

### 3.2 barrier()

```c
#define barrier() __asm__ __volatile__("": : :"memory")
```

这是纯编译器屏障：
- `__asm__`：GCC 内联汇编
- `__volatile__`：防止编译器删除或移动这条 asm
- `""`：空指令模板，不生成机器码
- `"memory"` clobber：告诉编译器这段代码可能读写任意内存，因此不得将内存访问跨越 barrier 重排

书中和内核注释都指出：这不会刷新寄存器到内存（与一些过时文档的说法不同），它的唯一作用是**阻止编译器重排序**。

### 3.3 Spinlock

虽然本章没有深入 spinlock 的实现，但提到了其 API。内核中 `spin_lock()` 的实现涉及：
- 关闭本地抢占（`preempt_disable()`）
- 获取锁（忙等或排队）
- 可能的调试检查（lockdep）

`preempt_disable()` 自身就包含 `barrier()`：
```c
#define preempt_disable()               \
do {                                    \
    preempt_count_inc();                \
    barrier();                          \
} while (0)
```

这确保了 `preempt_disable()` 之后的代码不会被编译器重排到增加抢占计数器之前——否则临界区代码可能在抢占仍然允许时执行。

### 3.4 Atomic Operations in Kernel

内核 `atomic_t` 提供了丰富的原子操作。与 GCC `__sync_*` 不同，内核原子操作区分了是否需要返回值、是否需要内存顺序：
- 无返回值 RMW（如 `atomic_add()`）：通常无序
- 有返回值 RMW（如 `atomic_add_return()`）：完全有序
- 条件 RMW（如 `atomic_add_unless()`）：完全有序

这与用户空间 C11 `memory_order_relaxed` vs `memory_order_seq_cst` 的设计哲学一致。

---

## 四、代码实现与测试

本章配套代码位于 `chapters/4/` 目录，实现了书中核心示例：

### 4.1 代码清单

| 文件 | 说明 |
|------|------|
| `forkjoin.c` | 演示 `fork()`/`wait()`，验证进程不共享内存 |
| `pcreate.c` | 演示 `pthread_create()`/`pthread_join()`，验证线程共享内存 |
| `lock.c` | 演示 `pthread_mutex_lock()`，相同锁 vs 不同锁的效果对比 |
| `rwlockscale.c` | 读写锁可扩展性测试框架（简化版） |
| `atomic.c` | GCC classic、C11、modern GCC 原子操作对比 |
| `compiler_barrier.c` | `READ_ONCE`、`WRITE_ONCE`、`barrier()` 的效果演示 |
| `data_race_demo.c` | 无同步、mutex、原子操作三种计数方式的正确性对比 |

### 4.2 编译与测试

```bash
cd chapters/4
make test
```

### 4.3 关键测试结果

**forkjoin.out**：
```
Parent starting, x = 0
Child 95083 set x = 1
Parent sees x = 0 (expected 0, because fork() does not share memory)
```
验证了 COW 语义。

**lock.out**：
```
=== Same lock: reader will only see final value ===
lock_reader(): x = 0

=== Different locks: reader may see intermediate values ===
lock_reader(): x = 1
lock_reader(): x = 2
lock_reader(): x = 3
```
验证了锁的互斥语义和锁-数据配对的重要性。

**compiler_barrier.out**：
```
--- Broken spin wait (may run forever without READ_ONCE) ---
loop_count_broken = 10000001

--- Fixed spin wait with READ_ONCE ---
loop_count_fixed = 4231022
```
在 `-O2` 优化下，`spin_wait_broken` 虽然没有真的无限循环（因为添加了安全上限），但 `loop_count_broken` 达到了安全上限而没有看到 `need_to_stop` 的变化，证明了编译器优化的危害。`READ_ONCE` 版本则能及时看到更新。

**data_race_demo.out**：
```
Racy increment (no sync): expected 4000000, got 4000000 (PASS)
Mutex increment: expected 4000000, got 4000000 (PASS)
Atomic increment: expected 4000000, got 4000000 (PASS)
```
注意：racy increment 在本测试的特定环境（x86_64，4线程，简单递增）下恰好得到了正确结果。这**不意味着它是正确的**。在更复杂的场景（更多线程、不同架构、更复杂操作）下，data race 几乎必然导致错误。这正是并发编程的危险之处：错误代码有时恰好能工作，掩盖了潜在缺陷。

---

## 五、用户笔记疑问解答分析

用户关于并发编程的笔记存放在 `~/data/vn/docs/concurrent/`，其中与第 4 章内容直接相关的疑问得到了显著解答。

### 5.1 已解答的核心疑问

#### 疑问 1：READ_ONCE / WRITE_ONCE 的原理和使用场景

**笔记内容**：
- "为什么 `inode->i_link` 的访问需要 READ_ONCE"
- "`READ_ONCE()` 似乎加的很随意"
- "如何理解 `__list_add` 中需要添加 WRITE_ONCE"
- "READ_ONCE 和 WRITE_ONCE 也是需要成对使用的"

**本章解答**：
本章第 2.5.3 节 "Accessing Shared Variables" 和第 2.3 节 "Atomic Operations" 系统回答了这一问题。

1. **原理**：`READ_ONCE` 通过 `*(const volatile typeof(x) *)&(x)` 的强制转换，创建一次编译器无法优化的内存访问。这防止了 load tearing、load fusing、invented loads 等问题。`WRITE_ONCE` 同理防止 store tearing 和 invented stores。

2. **使用场景**：书中明确列出 5 种可以使用 plain access 的场景，其他情况需要 `READ_ONCE/WRITE_ONCE`。例如 `inode->i_link` 最初为 NULL，后续由其他线程通过 `smp_store_release()` 或 `cmpxchg_release()` 设置为非 NULL。读取方必须使用 `READ_ONCE()`（或更强的 `smp_load_acquire()`），否则编译器可能将多次读取融合，导致看到撕裂或 stale 的指针值。

3. **成对使用**：虽然不是绝对必要，但成对使用是最佳实践。`WRITE_ONCE` 保证发布（publish）操作不会被编译器拆分或消除，`READ_ONCE` 保证订阅（subscribe）操作能读到完整值。这在无锁数据结构中尤其关键，如 `__list_add` 中 `WRITE_ONCE(prev->next, new)` 是一个发布点，确保新节点在指针可见前已完全初始化。

#### 疑问 2：barrier() 和 __sync_synchronize 的区别

**笔记内容**：
- "barrier() 和 __sync_synchronize 是一个东西吗?"
- "preempt_disable() 中为什么需要 barrier"
- "为什么 __rcu_read_lock 使用 barrier() ?"
- "x86 会存在 mfence 和 lfence，不是说好的 x86 是 strong model"

**本章解答**：
1. **本质区别**：`barrier()` 是**纯编译器屏障**，不生成任何 CPU 指令（x86 上空汇编 + memory clobber）。`__sync_synchronize()` 是**完整内存屏障**，同时约束编译器和 CPU，在 x86 上生成 `mfence` 或等效指令（如 `lock addl $0, -4(%esp)`）。

2. **preempt_disable() 中的 barrier**：增加抢占计数器后需要 barrier，确保后续临界区代码不会被编译器重排到 `preempt_count_inc()` 之前。这里不需要 CPU 屏障，因为 x86 TSO 和 Linux 调度器实现保证了计数器修改的可见性。

3. **RCU 中的 barrier**：`__rcu_read_lock()` 只需要防止编译器将 RCU 读临界区内的代码重排到 `rcu_preempt_read_enter()` 之前。RCU 的正确性依赖于上下文切换时的检查，而不是每次加载/存储的 CPU 顺序，因此编译器屏障足够。

4. **x86 为什么有 mfence**：尽管 x86 TSO 是强模型，但它只保证：
   - Load 不与 older load 重排
   - Store 不与 older store 重排
   - Load 可以重排到 older store 之前（Store-Load 乱序）
   - 带 lock 前缀的指令全序

   因此当需要严格 Store-Load 顺序时（如 seqlock 写者发布新数据后，读者必须看到完整数据），仍需 `mfence` 或 `lock` 前缀指令。

#### 疑问 3：volatile 的作用和局限

**笔记内容**：
- "为什么 jiffies 必须标记为 volatile"
- "volatile 只能保证有指令生成，但是编译器可以调度这些指令"
- "asm volatile 在 x86 中什么都不生成，那么有个什么作用"

**本章解答**：
1. **jiffies 必须是 volatile**：因为 jiffies 由时钟中断每 tick 更新。如果不用 volatile，编译器可能将循环中对 jiffies 的多次读取融合为一次，导致超时判断失效。

2. **volatile 的局限**：本章明确指出 volatile 访问之间不会被编译器重排序，但 volatile 与非 volatile 之间可以重排。且 volatile **完全不约束 CPU**。因此仅靠 volatile 无法解决多核 CPU 重排序导致的并发问题。

3. **asm volatile "" ::: "memory" 的作用**：即使不生成 x86 指令，它仍向编译器发出强约束——不得将内存访问跨越此 asm 重排。这是纯软件层面的屏障，对编译器有效，对 CPU 无效。

#### 疑问 4：memory model 和 acquire/release

**笔记内容**：
- "release 和 acquire 就像是锁的语义"
- "smp_store_release 的实现是不是有点出乎意料了"
- "为什么 x86 需要三个 fence?"

**本章解答**：
1. **Acquire-Release 语义**：本章在 "Atomic Operations (C11)" 和 "Modern GCC" 节介绍了显式内存顺序。`memory_order_release` 保证此前所有写操作在 release 操作前完成；`memory_order_acquire` 保证此后所有读操作在 acquire 操作后执行。两者配对形成 happens-before 关系。

2. **x86 smp_store_release 的简化实现**：因为 x86 TSO 已经天然保证 store-store 和 load-load 顺序，`smp_store_release` 在 x86 上只需 `barrier()` + `WRITE_ONCE()`。这确实"出乎意料"——看似没做什么，实则依赖了架构特性。ARM64 上则必须使用 `stlr` 指令。

3. **三个 fence 的必要性**：x86 的 `lfence`、`sfence`、`mfence` 分别用于：
   - `lfence`：Load-Load 屏障，主要用于 SSE/AVX 非时序加载与后续加载的排序（如 `movntdqa` 后需要 `lfence`）
   - `sfence`：Store-Store 屏障，用于非时序存储（`movnt*`）与后续存储的排序
   - `mfence`：全屏障，约束所有方向

   普通代码中 `mfence` 或 `lock` 前缀足够；`lfence`/`sfence` 主要用于与特殊指令（非时序内存操作）配合。

#### 疑问 5：锁、原子操作和 data race

**笔记内容**：
- "没有必要在锁中间使用 WRITE_ONCE / READ_ONCE 访问普通内存"
- "各种使用案例"

**本章解答**：
书中 "Avoiding Data Races" 节明确回答了这一点：
- **如果所有对共享变量的访问都在同一把锁保护下**，可以使用 plain loads/stores，不需要 `READ_ONCE/WRITE_ONCE`。
- 如果变量在锁保护下修改，但锁外读取，则写方用 `WRITE_ONCE()`，持锁方可用 plain load，锁外读取用 `READ_ONCE()`。
- 如果变量由特定线程/CPU 拥有，其他线程读取，规则类似。

这验证了笔记中的直觉：锁内确实不需要 `READ_ONCE/WRITE_ONCE`，因为锁已经建立了互斥，相当于单线程访问。

### 5.2 仍需后续章节解答的疑问

本章为以下问题提供了入门基础，但完整解答需要后续章节：

1. **RCU 具体实现机制**：本章仅提及 RCU 的 `rcu_read_lock()` 使用 `barrier()`，但 RCU 的宽限期（grace period）、回调机制等将在 "Deferred Processing" 章节详细讨论。

2. **Memory ordering 的深层形式化语义**：本章提到 happens-before、acquire-release，但 litmus test、形式化 memory model 等将在第 14-15 章深入。

3. **Per-CPU 变量的高性能计数器实现**：本章介绍了 API，但 "Counting" 章节会展示如何利用 per-thread 变量实现无锁高扩展计数器。

4. **Lock-free 数据结构**：本章介绍了 CAS，但如何用 CAS 构建无锁栈、队列等将在 "Locking" 和 "Data Structures" 章节讨论。

---

## 六、总结与核心要点

第 4 章 "Tools of the Trade" 是全书的技术地基。它传达了几个不能忽视的核心观念：

1. **不要忽视简单工具**。Shell 脚本和 make 的并行能力足以应对大量实际场景，只有当粒度足够细时才需要更重的工具。

2. **进程不共享内存，线程共享内存**。选择 `fork()` 还是 `pthread_create()` 取决于是否需要共享地址空间。

3. **锁必须和数据配对**。一把锁保护一组数据，乱用锁（相同锁保护无关数据、不同锁保护同一数据）会导致序列化或 data race。

4. **读写锁的扩展性被高估**。在小临界区和高并发度下，读写锁因锁状态更新带来的缓存行 bouncing 而性能崩溃。

5. **编译器是你的敌人也是朋友**。没有 `READ_ONCE/WRITE_ONCE` 和 `barrier()`，看似正确的代码可能被编译器优化得面目全非。理解 load tearing、store tearing、load fusing、invented stores 等概念，是写出可靠并发代码的前提。

6. **volatile 已不再是主角**。虽然历史上 indispensable，但现代并发代码应优先使用 C11 atomics、GCC `__atomic_*` 或内核 `READ_ONCE/WRITE_ONCE`。`volatile` 仍有用武之地（如设备 MMIO），但不应作为通用同步工具。

7. **工具选择遵循简单优先原则**。从顺序代码 -> shell 脚本 -> fork/wait -> pthreads -> 更高级机制，每一步升级都应在性能测量证明有必要后进行。

本章提供的不仅是 API 参考，更重要的是**并发编程的底层思维方式**：理解编译器和硬件如何"背叛"你的直觉，才能写出真正健壮的并发代码。

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
