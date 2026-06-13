# Chapter 15: Advanced Synchronization: Memory Ordering

## 1. 章节概述

本章是 perfbook 中最抽象、最反直觉的一章，核心目标是让读者理解现代多核系统中"内存顺序"(Memory Ordering) 的本质。作者 Paul E. McKenney 开篇即指出：人类对因果性和顺序的直觉在复杂并发代码中会完全失效，尤其是在 Linux 内核这种广泛使用弱序原子操作和内存屏障的系统中。

本章的结构大致分为四个层次：

1. **直觉与经验法则** (Memory-Model Intuitions)：为不想深入 LKMM (Linux Kernel Memory Model) 细节的读者提供快速上手的规则。
2. **为什么需要排序以及如何实现** (Ordering: Why and How?)：从硬件 store buffer 的物理机制出发，解释内存乱序的根源，并给出 Linux 内核提供的各类屏障原语。
3. **陷阱与技巧** (Tricks and Traps)：通过大量 litmus test 展示反直觉的硬件行为，包括变量同时具有多个值、各类重排序、地址/数据/控制依赖等。
4. **高层原语与硬件细节** (Higher-Level Primitives / Hardware Specifics)：讨论锁、RCU、内存分配器等高层原语背后的内存序语义，以及 x86/ARM/PowerPC 等架构的具体实现差异。

---

## 2. 核心概念详解

### 2.1 Store Buffer 与内存乱序的根源

现代 CPU 为了弥合主存速度远低于核心频率的差距，引入了 store buffer。当一个 CPU 执行 store 指令时，如果被写入的数据不在本地 cache 中，CPU 不会等待 cache line 从其他核心那里失效并取回，而是先将数据放入 store buffer，然后继续执行后续指令。

这个机制直接导致了经典的 **Store-Buffering (SB)** 乱序：

```
CPU 0                      CPU 1
x0 = 2;                    x1 = 2;
r2 = x1;  // may read 0   r2 = x0;  // may read 0
```

在 x86 上，即使该架构被称为"强序模型"(TSO)，上述两个 load 同时读到 0 的情况仍然是允许的。本章给出的测试数据显示，在一亿次试验中，这种反直觉的结果出现了 314 次。本章附带的 `store_buffering.c` 在本机测试中也验证了这一点：100 万次迭代中触发了 3 次（触发率约 0.0003%）。

**关键结论**：x86 的 TSO (Total Store Order) 模型只保证 store-store 和 load-load 的顺序，以及同地址访问的顺序，但**允许 store-load 的乱序**。这是唯一一类在 x86 上会被硬件重排序的内存操作组合。

### 2.2 Linux 内核内存排序速查表 (memory model api)
<!-- 94f3dc41-062d-40fe-a69e-49adcb89e9f3 -->

本章提供了一个极为重要的速查表 (Linux-Kernel Memory-Ordering Cheat Sheet)，总结了各类原语提供的排序保证。对日常编程最有价值的几行如下：

| 原语                  | 对之前操作排序 | 对之后操作排序 | 备注                             |
|-----------------------|----------------|----------------|----------------------------------|
| `WRITE_ONCE()`        | -              | -              | 仅保证单变量顺序，防止编译器优化 |
| `READ_ONCE()`         | -              | -              | 同上，防止 load 融合/发明        |
| `smp_rmb()`           | Load           | Load           | 读内存屏障                       |
| `smp_wmb()`           | Store          | Store          | 写内存屏障                       |
| `smp_mb()`            | Load+Store     | Load+Store     | 全内存屏障                       |
| `smp_store_release()` | Load+Store     | Store          | release 语义                     |
| `smp_load_acquire()`  | Load           | Load+Store     | acquire 语义                     |
| `void atomic RMW`     | -              | -              | 无返回值的原子 RMW 不保证排序    |
| `non-void atomic RMW` | Load+Store     | Load+Store     | 有返回值/条件的 RMW 提供全序     |

我这里解释说明一下，其中 smp_store_release() 和 smp_load_acquire() 语义是最清晰的，
也就是单向的，只有读或者写。

这张表是理解内核并发代码的基石。例如，为什么 `atomic_inc()` 不提供任何排序？因为它属于 void RMW。为什么 `atomic_add_return()` 提供全序？因为它有返回值，内核要求其成功时必须具备完整屏障语义。

### 2.3 经验法则 (Rules of Thumb)

本章给出了四条核心经验法则，足以覆盖绝大多数内核并发编程场景：

1. **仅在至少两个线程共享至少两个变量时才需要内存序操作**。单变量或单线程不需要。
2. **如果循环中所有线程间通信链接都是 store-to-load**，则最弱排序（依赖、acquire/release）就足够了。
3. **如果循环中除了一条链接外其余都是 store-to-load**，则对每条 store-to-load 使用 release-acquire 对即可。
4. **如果循环中存在两条或更多非 store-to-load 链接**（即 load-to-store 或 store-to-store），则每对非 store-to-load 链接之间需要至少一个 `smp_mb()` 级别的全屏障。

这里的"store-to-load 是特殊的"，原因在于它的 temporal 性质：如果一个 load 读到了一个 store 写入的值，那么从物理时间上讲，这个 store 必然在 load 之前完成（因为光速有限、系统尺寸非零）。而 load-to-store 和 store-to-store 则可能呈现 counter-temporal（反时间顺序）的特性，即后执行的操作可能先被其他 CPU 观察到。

### 2.4 依赖关系：地址依赖、数据依赖、控制依赖

除了显式的内存屏障，CPU 还会隐式地尊重某些依赖关系，这可以在不插入昂贵屏障的情况下获得免费的排序：

- **地址依赖 (Address Dependency)**：一个 load 的结果被用作后续内存访问的地址。例如 `p = READ_ONCE(head); v = READ_ONCE(p->val);`。除 DEC Alpha 外，所有现代 CPU 都会尊重这种依赖。
- **数据依赖 (Data Dependency)**：一个 load 的结果被用作后续 store 的数据。例如 `v = READ_ONCE(x); WRITE_ONCE(y, v);`。
- **控制依赖 (Control Dependency)**：一个 load 的结果决定后续 store 是否执行。例如 `if (READ_ONCE(x)) WRITE_ONCE(y, 1);`。**注意**：控制依赖仅保证 load->store 的顺序，不保证 load->load。而且编译器极易破坏控制依赖（例如通过将条件分支优化掉）。

**编译器破坏依赖的陷阱**是本章反复强调的重点。例如，如果编译器能够证明一个指针等于 `&reserve_int`，它可能直接用这个已知地址替代指针变量，从而切断硬件依赖链。又例如，如果 `if` 的两条分支对同一变量做相同的 store，编译器可能将 store 提升到 `if` 之外，彻底摧毁控制依赖。

### 2.5 多拷贝原子性 (Multicopy Atomicity)

- **完全多拷贝原子 (Full multicopy atomic)**：所有线程对所有 store 的顺序达成一致。IBM z Systems 是唯一主流商用架构具备此属性。
- **其他多拷贝原子 (Other-multicopy atomic)**：除执行 store 的 CPU 自身外，其他所有 CPU 对 store 顺序达成一致。x86 和 ARMv8 属于此类。
- **非多拷贝原子 (Non-multicopy atomic)**：即使不同 CPU 之间也可能对 store 顺序有不同看法。PowerPC 属于此类。

这一差异直接导致了 **WRC (Write-to-Read Causality)** 这类 litmus test 在不同平台上的行为不同。在非多拷贝原子平台上，即使使用了数据依赖和 `smp_rmb()`，`exists` 子句仍可能触发。

### 2.6 累积性 (Cumulativity) 与传播性 (Propagation)

Release 操作（如 `smp_store_release()`）具有**累积性**：它不仅排序当前线程之前的访问，还排序其他线程对同一变量的先前访问。这使得 release-acquire 链能够跨越多线程传播可见性。

然而累积性有其极限。当存在 load-to-store 链接时，release-acquire 链的排序可能无法传播过去。此时需要更强的 **传播性**，即 `smp_mb()` 提供的全屏障语义。具体规则是：release-acquire 链可以容忍**一个**非 store-to-load 链接，但如果有两个或以上，就必须使用 `smp_mb()`。

---

## 3. 代码实现与测试

本章配套了四个 C 语言示例程序，使用 C11 atomics 模拟内核中的内存序原语，并通过百万级循环尝试触发乱序行为。

### 3.1 Store-Buffering 测试 (`store_buffering.c`)

```c
P0:
    atomic_store_explicit(&x0, 1, memory_order_relaxed);
    r2 = atomic_load_explicit(&x1, memory_order_relaxed);

P1:
    atomic_store_explicit(&x1, 1, memory_order_relaxed);
    r2 = atomic_load_explicit(&x0, memory_order_relaxed);
```

测试目标：`exists (0:r2=0 /
 1:r2=0)`

**本机测试结果**（Intel i9-13900K，x86_64）：

```
Iterations: 1000000
Triggered 3 times (both loads saw 0)
Rate: 0.000300%
```

结果明确表明，即使在 x86 这种强序架构上，store-buffering 导致的 store-load 乱序也确实存在，只是触发概率极低。本章原文指出在 x86 笔记本上 1 亿次试验中触发了 314 次，与本机的观察一致。

### 3.2 Message-Passing 测试 (`message_passing.c`)

```c
P0:
    WRITE_ONCE(x0, 42);
    smp_wmb();
    WRITE_ONCE(x1, 1);

P1:
    while ((r2 = READ_ONCE(x1)) == 0)
        ;
    smp_rmb();
    r3 = READ_ONCE(x0);
```

测试目标：`exists (1:r2=1 /
 1:r3=0)`

**本机测试结果**：

```
Iterations: 100000
Triggered 0 times
```

在 x86 上未触发是预期的，因为 x86 本身不会重排序 P1 的两个 load（load-load 顺序有保证），且 `smp_wmb()` 和 `smp_rmb()` 在 x86 上虽然只是编译器屏障，但结合硬件的强序特性，足以阻止这种错误结果。

### 3.3 Release-Acquire 链测试 (`release_acquire.c`)

该测试模拟了三线程的 release-acquire 链：P0 release-stores 到 A，P1 acquire-loads A 后 release-stores 到 B，P2 acquire-loads B 后读取 P0 的原始数据。100000 次迭代中未触发乱序，验证了 release-acquire 链的跨线程传播能力。

### 3.4 控制依赖测试 (`control_dependency.c`)

该测试主要用于教育目的，展示了控制依赖的语法形式。由于在 x86 上 load->store 顺序天然被保证，此测试不会触发乱序，但注释中强调了在弱序 CPU（如 ARM）上控制依赖 load->load 是不被保证的。

---

## 4. 结合 Linux 内核源码分析

### 4.1 x86 内存屏障实现

Linux 内核源码 `arch/x86/include/asm/barrier.h` 对 x86 的内存序原语实现非常精简：

```c
#define __smp_mb()   asm volatile("lock addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
#define __smp_rmb()  dma_rmb()
#define __smp_wmb()  barrier()

#define __smp_store_release(p, v)    \
do {                                 \
    compiletime_assert_atomic_type(*p); \
    barrier();                       \
    WRITE_ONCE(*p, v);               \
} while (0)

#define __smp_load_acquire(p)        \
({                                   \
    typeof(*p) ___p1 = READ_ONCE(*p); \
    compiletime_assert_atomic_type(*p); \
    barrier();                       \
    ___p1;                           \
})
```

**关键点分析**：

- `__smp_mb()` 使用 `lock; addl $0, -4(%rsp)` 指令。`lock` 前缀在 x86 上具有全屏障效果，且比 `mfence` 性能更优（2017 年内核提交 `76e079fefc8f` 将 `mfence` 替换为 `lock; addl`，获得了 60% 性能提升）。
- `__smp_rmb()` 在 x86 上退化为 `barrier()`，因为 x86 不会重排序 load-load。
- `__smp_wmb()` 在 x86 上同样只是 `barrier()`，因为 x86 不会重排序 store-store。
- `__smp_store_release()` 仅插入一个编译器 `barrier()` 后执行 `WRITE_ONCE()`。在 x86 上，这已足够，因为 TSO 保证 store-store 顺序，且 `barrier()` 防止编译器重排序。
- `__smp_load_acquire()` 执行 `READ_ONCE()` 后插入编译器 `barrier()`。同样在 x86 上已足够。

### 4.2 通用屏障头文件

`include/asm-generic/barrier.h` 提供了跨架构的通用定义：

```c
#ifndef __smp_store_release
#define __smp_store_release(p, v)    \
do {                                 \
    compiletime_assert_atomic_type(*p); \
    __smp_mb();                      \
    WRITE_ONCE(*p, v);               \
} while (0)
#endif

#ifndef __smp_load_acquire
#define __smp_load_acquire(p)        \
({                                   \
    __unqual_scalar_typeof(*p) ___p1 = READ_ONCE(*p); \
    compiletime_assert_atomic_type(*p); \
    __smp_mb();                      \
    (typeof(*p))___p1;               \
})
#endif
```

注意通用定义中使用了 `__smp_mb()`，这比 x86 的专用实现重得多。这体现了 Linux 内核的"最小公分母"设计哲学：在弱序架构（如 ARM、PowerPC）上需要全屏障，而在 x86 上可以通过架构特定代码优化掉不必要的硬件屏障。

### 4.3 READ_ONCE / WRITE_ONCE

内核中 `include/asm-generic/rwonce.h` 定义：

```c
#define __READ_ONCE(x)  (*(const volatile __unqual_scalar_typeof(x) *)&(x))
#define READ_ONCE(x)    ({ compiletime_assert_rwonce_type(x); __READ_ONCE(x); })
#define __WRITE_ONCE(x, val)    (*(volatile typeof(x) *)&(x) = (val))
```

`volatile` 在这里的作用是告诉编译器：
1. 每次访问必须生成实际的内存读写指令（防止 load fusion/store fusion）。
2. 不允许假设变量的值不会在两次访问之间改变。

但 `volatile` **不**提供 CPU 级别的内存排序，也不保证操作的原子性（对于大于机器字长的类型）。因此 `READ_ONCE`/`WRITE_ONCE` 必须与适当的屏障（如 `smp_mb()`、`smp_store_release()` 等）配合使用。

---

## 5. 对用户笔记中疑问的回应

结合本章内容和 Linux 内核源码，对用户笔记 ~/data/vn/docs/concurrent 中的相关疑问进行分析和解答：

### 5.1 "能不能稳定复现 memory model 的效果"

本章明确给出了答案：**可以，但触发率极低，且高度依赖硬件平台和负载**。例如 store-buffering 在 x86 上 1 亿次试验仅触发 314 次。WRC 在 Power8 上 10 亿次试验触发 4 次。这意味着：

- 不能因为没有在测试中触发乱序，就假定代码是正确的。
- 应该使用 litmus 测试工具（如 `herd`、`litmus7`）进行形式化验证，而非依赖物理测试。
- 本章附带的 `store_buffering.c` 在本机 100 万次迭代中触发了 3 次，证实了现象存在但罕见。

### 5.2 "既然存在 smp_load_acquire，为什么还需要 smp_mb"

这是理解内存序层次结构的关键问题。本章速查表和经验法则给出了清晰答案：

- `smp_load_acquire()` / `smp_store_release()` 只提供单向的"半屏障"语义，且它们**不干预 store buffer**。它们仅保证：release 之前的所有访问不会重排到 release 之后；acquire 之后的所有访问不会重排到 acquire 之前。
- 当一个同步循环中包含**两个或更多非 store-to-load 链接**时，release-acquire 无法传播足够强的顺序。此时必须使用 `smp_mb()`，因为它具有**传播性 (propagation)**，能够强制清空 store buffer 并在全局建立顺序。
- x86 上 `smp_mb()` 实现为 `lock; addl`，这是一个昂贵的全屏障；而 `smp_load_acquire()` 只是 `READ_ONCE` + 编译器屏障。两者的性能差距可达数量级。

### 5.3 "barrier() 和 __sync_synchronize 是一个东西吗"

**不是**。`barrier()` 是 Linux 内核中的编译器屏障：

```c
#define barrier() __asm__ __volatile__("": : :"memory")
```

它只阻止编译器重排序，不生成任何 CPU 指令。而 `__sync_synchronize()` 是 GCC 内置的全内存屏障，会生成实际的硬件屏障指令（在 x86 上类似 `mfence` 或 `lock` 前缀操作）。本章明确区分了编译器排序和硬件排序，并指出编译器重排序通常比硬件重排序更激进。

### 5.4 "为什么 x86 需要三个 fence (mfence/lfence/sfence)"

这个问题在 x86 语境下需要澄清：Linux 内核的 `smp_mb()` 在 x86 上实际使用的是 `lock; addl`，而非 `mfence`。但确实 x86 提供了三种 fence 指令：

- `lfence`：load fence，阻止 load 重排序。主要用于防止推测执行（如 Spectre 缓解），在常规内存序中很少需要，因为 x86 本身不会乱序 load-load。
- `sfence`：store fence，阻止 store 重排序。主要用于非临时存储 (non-temporal stores) 的排序。
- `mfence`：全 fence，阻止 load 和 store 的所有重排序。

对于普通的 `smp_mb()`，`lock; addl` 比 `mfence` 性能更好，因此内核选择了前者。`lfence`/`sfence` 主要在驱动代码或特定 SIMD 指令场景中使用。

### 5.5 "smp_store_release 的实现是不是有点出乎意料"

用户在笔记中贴出了 x86 和 ARM64 的实现对比，并感到困惑。本章的速查表和硬件细节章节解释了这一差异：

- **x86**：`smp_store_release()` 仅插入编译器 `barrier()` 后执行 `WRITE_ONCE()`。这是因为 x86 的 TSO 已经保证了 store-store 顺序，release 语义所需的"之前操作不后移"由编译器屏障保证即可。
- **ARM64**：需要使用 `STLR` (Store-Release) 指令。ARM 是弱序架构，没有专门的硬件保证，必须通过指令显式声明 release 语义。

这种差异正是 Linux 内核内存模型"抽象层"的价值所在：内核代码编写者只需要使用 `smp_store_release()`，无需关心底层是 x86 的零开销实现还是 ARM 的专用指令。

### 5.6 "WRITE_ONCE 和 READ_ONCE 的使用时机"

本章 `sec:memorder:Compile-Time Consternation` 专门讨论了这个问题。总结如下：

- **必须使用 READ_ONCE/WRITE_ONCE 的场景**：
  - 访问可能被其他 CPU 并发修改的变量（防止编译器优化掉重读）。
  - 防止 load/store 被拆分为多次访问（tearing）。
  - 在数据竞争 (data race) 的代码路径中标记共享变量。
- **可以不使用的场景**：
  - 持有锁保护下的变量访问（因为锁已经提供了排他性，编译器不会跨临界区优化）。
  - 初始化阶段，其他 CPU/线程尚无法访问该变量时。

用户笔记中提到的 `__list_add()` 使用 `WRITE_ONCE(prev->next, new)` 的例子，内核注释解释得很清楚：这是链表操作的"发布点"(publication point)，必须确保在该写操作之前对 `new` 节点的所有初始化对其他 CPU 可见。虽然 `spin_lock()` 已经提供了足够的屏障，但 `WRITE_ONCE` 明确地标记了这一个发布语义点，使得代码意图更清晰，也便于 KCSAN 等工具检测竞争。

### 5.7 "try_to_wake_up 为什么大量使用 smp_mb__after_spinlock"

这个问题触及了本章关于锁的排序语义的核心发现。本章通过 litmus test 证明：

- `spin_lock()` 和 `spin_unlock()` 单独或合起来**都弱于**全屏障。
- 一个 CPU 上先 unlock 再 lock 同一个锁，并不保证这两个临界区之间的访问被其他 CPU 观察到是有序的。
- `smp_mb__after_spinlock()` 的作用正是：在 lock 获取之后插入一个全屏障，确保**所有 CPU**（包括不持有该锁的 CPU）都能看到之前临界区的访问发生在该屏障之后。

调度器中的 `try_to_wake_up` 需要将一个任务从等待队列移出并唤醒，这个操作涉及多个数据结构（task_struct、runqueue、waitqueue）的复杂交互。由于其他 CPU 可能在无锁路径上读取这些状态（例如通过 `schedule()` 的快速路径），必须确保状态变更的全局可见性，因此需要使用 `smp_mb__after_spinlock()`。

---

## 6. 总结

本章的核心教诲可以概括为一句话：**不要依赖直觉，要依赖模型和工具**。

Linux 内核内存模型 (LKMM) 的出现，使得并发代码的正确性不再依赖于程序员对硬件的模糊理解，而是可以通过形式化工具（`herd`、`litmus7`）进行机械验证。对于绝大多数内核开发者，遵循本章的四条经验法则，配合 `READ_ONCE`/`WRITE_ONCE` 和适当的 `smp_*` 原语，就足以写出正确的并发代码。

对于更深层的实现者（如编写锁、RCU、无锁数据结构），则需要深入理解：

1. Store buffer 导致的 store-load 乱序是性能与正确性的根本张力。
2. Release-acquire 链是绝大多数高性能同步的基石，但它只能容忍一个非 store-to-load 链接。
3. 编译器比 CPU 更危险，`READ_ONCE`/`WRITE_ONCE` 和 `barrier()` 是防止编译器破坏并发语义的基础工具。
4. 不同架构的内存序差异巨大，Linux 内核的通用原语抽象是成功跨平台的关键。

本章的代码示例和测试进一步验证了：即使在 x86 这种强序架构上，内存乱序也确实存在（store-buffering 已在测试中被观察到）。这意味着无论目标平台是什么，正确的内存序使用都不是可选项，而是并发编程的必选项。

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
