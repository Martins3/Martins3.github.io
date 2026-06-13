# Hardware and its Habits

## 章节核心主旨

本章的核心论点是：**并行程序员必须理解底层硬件特性，因为并行编程的唯一目的是提升性能，而性能直接取决于硬件的详细行为。** 作者用一句话精辟地概括了这一点："You might not care about the laws of physics, but the laws of physics cares deeply about your code!"

本章系统地分析了现代共享内存并行系统中，各种硬件特性如何成为性能障碍（obstacles），并给出了这些障碍的实际开销数据，最终推导出软件设计的基本原则。

---

## 一、现代 CPU 的性能障碍 (Section 3.1 Overview)

### 1.1 流水线 CPU (Pipelined CPUs)

1980 年代的微处理器采用简单的"取指-译码-执行"三步流程，每条指令至少消耗 3 个时钟周期。而现代 CPU 使用流水线（pipeline）、超标量（superscalar）、乱序执行（out-of-order）、推测执行（speculative execution）以及超线程（SMT/HT）等技术，同时执行大量指令。

**关键洞察**：这些优化都依赖于**高度可预测的控制流**。当程序运行在紧凑循环中（如大型矩阵运算）时，CPU 的分支预测器能正确预测循环末尾的分支，保持流水线满载。但当控制流不可预测时（如通过虚函数指针调用、二叉搜索中的随机分支），预测失败会导致**流水线冲刷（pipeline flush）**，已推测执行的指令被丢弃，性能急剧下降。

超线程进一步复杂化了这个问题：同一核心上的多个硬件线程共享寄存器、缓存、执行单元等资源，一个线程的行为会干扰另一个线程。作者特别提到一个反直觉的现象：在紧凑循环中增加一条指令，有时反而可能**加速**执行，这是因为执行单元选择的变化缓解了后续指令的资源争用。

**Linux 内核关联**：Linux 调度器 aware 了 SMT 的影响。在 `kernel/sched/core.c` 中，调度器通过 `sched_domain` 层次结构区分物理核心和逻辑核心（超线程），优先将任务分散到不同物理核心，避免同一核心内的资源争用。

### 1.2 内存引用 (Memory References)

1980 年代，从内存加载数据比执行一条指令更快。今天，CPU 可能在一次内存访问的时间里执行数百甚至数千条指令。这种差距来自摩尔定律对 CPU 性能的提升远大于对内存延迟的改善。

现代 CPU 用大型缓存来缓解这个问题。但缓存需要**高度可预测的访问模式**才能有效工作。不幸的是，常见操作如链表遍历具有极不可预测的访问模式——如果模式可预测，我们就不需要指针了。因此，内存引用成为现代 CPU 面临的严重障碍。

### 1.3 原子操作 (Atomic Operations)

原子操作与 CPU 流水线的"逐段装配线"特性存在根本冲突。硬件设计师使用巧妙的技巧使原子操作**看起来**是原子的：识别包含目标数据的所有缓存行，确保这些缓存行由执行原子操作的 CPU 独占持有，然后在该 CPU 拥有这些缓存行期间执行操作。

这个过程可能导致流水线延迟甚至冲刷，因为 CPU 必须等待缓存行所有权的确立。非原子操作则可以从缓存中直接读取值并存入存储缓冲，无需等待缓存行所有权。

**Linux 内核关联**：x86 上的原子操作通过 `lock` 前缀实现。在内核源码 `arch/x86/include/asm/atomic.h` 中：

```c
static __always_inline void arch_atomic_add(int i, atomic_t *v)
{
    asm_inline volatile(LOCK_PREFIX "addl %1, %0"
             : "+m" (v->counter)
             : "ir" (i) : "memory");
}
```

`LOCK_PREFIX` 在 SMP 系统上展开为 `lock` 前缀，该前缀保证对内存总线的独占访问，实现原子性。而在 `arch/x86/include/asm/cmpxchg.h` 中，`cmpxchg` 指令配合 `lock` 前缀实现了比较并交换（CAS）：

```c
asm_inline volatile(lock "cmpxchgl %[new], %[ptr]"
             : "=@ccz" (success),
               [ptr] "+m" (*__ptr),
               [old] "+a" (__old)
             : [new] "r" (__new)
             : "memory");
```

### 1.4 内存屏障 (Memory Barriers)

考虑一个简单的加锁临界区：

```c
spin_lock(&mylock);
a = a + 1;
spin_unlock(&mylock);
```

如果 CPU 不受约束地按自己认为最优的顺序执行这些语句，变量 `a` 的增量可能发生在锁保护之外。为了防止这种破坏性重排序，锁原语包含显式或隐式的内存屏障。

内存屏障的目的是阻止 CPU（和编译器）为了性能而进行的重排序，因此**几乎总是降低性能**。但它们是保证正确性所必需的。

**Linux 内核关联**：x86 的内存屏障实现在 `arch/x86/include/asm/barrier.h` 中：

```c
#define __smp_mb()	asm volatile("lock addl $0,-4(%%" _ASM_SP ")" ::: "memory", "cc")
```

x86 是强一致性架构（TSO），大部分读写操作不会乱序，因此 `smp_rmb()` 和 `smp_wmb()` 可以简化为编译器屏障（`barrier()`），但全内存屏障 `smp_mb()` 仍需要 `lock addl $0` 这种自增零操作来冲刷存储缓冲和加载队列。

ARM64 则是弱一致性架构，需要显式的屏障指令：

```c
#define __smp_mb()	__asm__ __volatile__("dmb ish" ::: "memory")
```

ARM64 还提供了 `ldar`（load acquire）和 `stlr`（store release）指令，可以直接在加载/存储操作中嵌入屏障语义，这比单独的 `dmb` 更高效。

### 1.5 功能单元不匹配 (Functional Unit Failings)

现代超标量 CPU 拥有多种功能单元：整数 ALU、向量单元、浮点单元、分支单元、加载/存储单元等。不同应用在不同执行阶段需要不同的功能单元组合。有时 CPU 必须"单腿跳跃"，仅使用其丰富功能单元阵列中的少数几个。

在超线程核心中，共享功能单元会导致另一种形式的争用，限制可扩展性。

### 1.6 热节流 (Thermal Throttling)

对关键代码路径进行微优化后，有时反而发现墙上时钟时间**增加**了。这就是现代热节流（thermal throttling）：更有效地使用功能单元会增加功耗和散热，如果散热系统能力不足，CPU 会降低时钟频率。

**实践建议**：如果无法改进散热，应考虑算法优化而非硬件层面的微优化；或者将工作负载并行化到多个核心上分散热量。

### 1.7 缓存未命中 (Cache Misses)

缓存对频繁共享的变量反而是有害的。当某个 CPU 要修改变量时，该变量很可能在另一个 CPU 的缓存中而不在本 CPU 缓存中，导致昂贵的缓存未命中。这种缓存未命中是 CPU 性能的主要障碍。

**Linux 内核关联**：内核中大量使用 `____cacheline_aligned` 属性来避免 false sharing。例如 per-CPU 变量的定义就确保不同 CPU 的数据落在不同缓存行上。

### 1.8 中断 (Interrupts) 与 I/O 操作

中断会打断 CPU 当前执行的指令流。CPU 需要"找到一个好的停止点"，这可能耗时，因为 CPU 可能正在执行数百条乱序指令。处理完中断后，重新填充指令执行流水线也需要时间。

I/O 操作（网络、存储）比缓存未命中的延迟更大。共享内存并行程序通常只需处理缓存未命中级别的延迟，而分布式系统还要面对网络通信延迟。

---

## 二、开销的实际数据 (Section 3.2 Overheads)

### 2.1 硬件系统架构

perfbook 展示了八核系统的简化架构：每颗 die 有两个 CPU 核心，各有自己的缓存，通过片上互联通信；系统互联让四颗 die 之间以及与主内存通信。

数据以**缓存行（cache line）**为单位在系统中移动，通常是 32-256 字节的 2 的幂次大小的对齐内存块。

作者用一个简化的场景展示了"简单存储"的完整生命周期：CPU 1 写一个变量，其缓存行在 CPU 6 的缓存中。这个过程涉及：存储缓冲记录、多级互联转发、缓存行查找、缓存行转发与冲刷、最终写入本地缓存。这仅仅是一个简化的 cache-coherency 协议流程，实际生产中的协议要复杂得多。

**关键洞察**：如果某个变量在一段时间内被频繁读取而从不会更新，它可以被复制到所有 CPU 的缓存中。这种**只读复制（read-mostly replication）**允许所有 CPU 极快地访问该变量。

### 2.2 操作成本

perfbook 提供了三张表格，展示了不同规模系统上同步机制的开销。核心数据（以 Intel Xeon Platinum 8176 八插槽系统为例）：

| 操作 | 延迟 (ns) | 时钟周期 |
|------|-----------|----------|
| Clock period | 0.5 | 1 |
| Same-CPU CAS | 7.0 | 14.6 |
| Same-CPU lock | 15.4 | 32.3 |
| On-Core blind CAS | 7.2 | 15.2 |
| On-Core CAS | 18.0 | 37.7 |
| Off-Core blind CAS | 47.5 | 99.8 |
| Off-Core CAS | 101.9 | 214.0 |
| Off-Socket blind CAS | 148.8 | 312.5 |
| Off-Socket CAS | 442.9 | 930.1 |
| Cross-Interconnect CAS | 944.8 | 1984.2 |

**关键洞察**：
1. **同一 CPU 的 CAS** 约 7ns，已经比时钟周期长十倍以上
2. **跨核心 blind CAS** 约 47.5ns，近 100 个时钟周期
3. **跨插槽 CAS** 超过 400ns，近 1000 个时钟周期——在此期间 CPU 可以执行超过 200 条普通指令
4. 这展示了**细粒度锁**的局限性，以及任何依赖细粒度全局一致性的同步机制的问题

这些数据验证了 Stephen Hawking 对硬件设计师提出的两个基本问题：**光速有限**和**物质的原子性**。光速限制了原始速度，原子性限制了微型化，进而限制了频率。

有趣的是，笔记本级的小规模系统（如 Intel Core i7-8750H）的延迟要合理得多：Off-Core blind CAS 仅 22.2ns。这提醒我们，空间和时间局部性即使在相对较小的系统上也是首要考虑因素。

### 2.3 硬件优化

硬件设计师提供了多种优化：

1. **大缓存行**：对顺序访问有巨大提升，但带来 **false sharing** 问题——不同 CPU 更新同一缓存行中的不同变量时，会导致缓存行来回弹跳。软件可以通过编译器的对齐指令避免 false sharing。

2. **缓存预取**：硬件对连续访问进行预取，但对复杂访问模式可能失效。某些 CPU 提供显式预取指令。

3. **存储缓冲（store buffer）**：允许快速执行一连串存储指令，即使目标缓存行不在缓存中。副作用是内存乱序，需要内存屏障来处理。

4. **推测执行**：有效利用存储缓冲而不导致内存乱序。副作用包括能量效率降低和 Spectre/Meltdown 等侧信道攻击。

5. **大缓存**：允许 CPU 在更大的数据集上工作而不产生昂贵的缓存未命中。

6. **只读复制**：频繁读取但很少更新的数据可以存在于所有 CPU 的缓存中。

---

## 三、硬件的免费午餐？ (Section 3.3 Hardware Free Lunch?)

并发受到关注的主要原因是摩尔定律带来的单线程性能提升（"免费午餐"）已经结束。硬件设计师可能通过以下途径恢复免费午餐：

1. **新材料和新工艺**：高 K 电介质、量子点、真空沟道晶体管等，但都面临制造挑战。

2. **用光代替电子**：光在真空中的速度是硬限制，但半导体中的电波只有光速的 3%-30%。芯片内/芯片间的光纤实验正在进行，但光电转换效率是问题。

3. **3D 集成**：通过垂直堆叠硅片缩短路径长度。约 2020 年的 chiplet 技术已在此方向上取得实质进展。

4. **专用加速器**：GPU、向量指令（SSE/AVX）、加密/压缩硬件等。通用 CPU 在处理专用问题时，大量时间和能量浪费在解码指令、递增循环计数器等与核心问题无关的操作上。

5. **现有并行软件**：并行操作系统、线程库、数据库、数值软件。使用现有并行软件（如关系数据库）可以让单线程程序无需直接处理并行性。

---

## 四、软件设计启示 (Section 3.4 Software Design Implications)

本章的核心启示极其明确：**并行算法必须在设计时就充分考虑这些硬件特性。**

具体策略：

1. **运行近乎独立的线程**：线程间通信（原子操作、锁、显式消息）越不频繁，应用的性能和可扩展性越好。

2. **确保共享是只读的**：这允许 CPU 缓存复制只读数据，让所有 CPU 都能快速访问。

3. **追求尴尬的并行（embarrassingly parallel）**：无论是通过仔细选择数据结构和算法、使用现有并行应用和环境，还是将问题转化为尴尬并行形式。

作者以一个思想实验强化了这个观点：假设并行应用使用 CAS 操作在线程间通信，每个 CAS 涉及缓存未命中（因为线程主要彼此通信而非与自己通信），而每个 CAS 对应的计算工作量为 300ns。那么大约一半的执行时间将消耗在 CAS 通信操作上！这意味着双 CPU 系统运行这样的并行程序，速度不会比单 CPU 的串行实现更快。

---

## 五、配套 C 语言演示程序

本章提供了 6 个配套的 C 语言演示程序，所有程序均包含 Makefile，二进制文件以 `.out` 结尾，符合项目规范。

### 5.1 cache_miss_demo.c

演示缓存未命中的开销。比较：
- 单线程本地计数器
- 两个线程各自操作独立的计数器（不同缓存行）
- 两个线程操作同一个共享计数器（缓存行弹跳）

本地测试结果显示：共享计数器的操作明显更慢，因为每次修改都需要在多核之间迁移缓存行的独占所有权。

### 5.2 false_sharing_demo.c

演示 false sharing 问题。比较：
- 两个字段紧密打包在同一缓存行的结构体
- 两个字段通过填充（padding）放置在不同缓存行的结构体

当不同 CPU 更新同一缓存行上的不同变量时，硬件缓存一致性协议会错误地将其视为同一变量的争用，导致缓存行来回弹跳。

### 5.3 atomic_overhead_demo.c

演示原子操作的开销。比较：
- 普通递增
- 原子递增（`__atomic_fetch_add`）
- CAS 循环递增
- 有争用的原子递增

本地测试显示：普通递增约 1.3ns/iter，原子递增约 3.3ns/iter，CAS 循环约 5.9ns/iter，有争用的原子递增约 8.4ns/iter。这验证了原子操作的额外开销以及争用时缓存行弹跳的代价。

### 5.4 branch_prediction_demo.c

演示分支预测对流水线的影响。比较：
- 随机数据（50/50 正负）上的条件分支
- 排序后数据上的条件分支

当数据可预测时，CPU 的分支预测器能正确预测，避免流水线冲刷。

### 5.5 memory_barrier_demo.c

演示内存屏障在锁实现中的作用。实现了：
- 带有 acquire/release 语义的票证锁（ticket lock）
- 缺少屏障的"破损"锁

acquire 语义确保临界区内的内存操作不会被重排序到获取锁之前；release 语义确保它们不会被重排序到释放锁之后。

### 5.6 read_mostly_demo.c

演示只读复制的好处。比较：
- 纯只读访问模式
- 读多写少模式
- 频繁写模式

只读数据可以被复制到所有 CPU 缓存中，实现极快的并行读取；频繁写则导致缓存行所有权频繁迁移。

---

## 六、Linux 内核源码关联分析

### 6.1 原子操作实现

Linux 内核在 `arch/x86/include/asm/atomic.h` 和 `arch/x86/include/asm/cmpxchg.h` 中定义了 x86 架构的原子操作。核心机制是使用 `lock` 前缀确保内存操作的原子性。`LOCK_PREFIX` 宏在 SMP 系统上展开为 `lock` 前缀，在 UP 系统上为空，体现了内核对不同硬件配置的适应性。

### 6.2 内存屏障实现

x86 的强一致性模型（TSO）使得许多屏障操作可以简化为编译器屏障（`barrier()`，即 `asm volatile("" ::: "memory")`）。这是因为 x86 保证：
- Loads 不会被重排序到其他 loads 之前
- Stores 不会被重排序到其他 stores 之后
- Stores 不会被重排序到其他 loads 之前（但 load-store 可能重排序）

因此 x86 只需要：
- `smp_mb()`：`lock addl $0`（全屏障）
- `smp_rmb()`：`dma_rmb()`，即 `barrier()`
- `smp_wmb()`：`barrier()`

而 ARM64 的弱一致性模型需要显式指令：
- `smp_mb()`：`dmb ish`
- `smp_rmb()`：`dmb ishld`
- `smp_wmb()`：`dmb ishst`

ARM64 还利用 `ldar`/`stlr` 指令将对普通变量的加载/存储与屏障语义结合，这是比单独屏障指令更高效的实现。

### 6.3 避免 False Sharing

内核中广泛使用缓存行对齐来避免 false sharing。例如 per-CPU 变量、RCU 的 per-CPU 计数器、各种锁结构体等，都通过 `____cacheline_aligned` 或显式 padding 确保热点数据分布在不同的缓存行上。

---

## 七、对用户笔记中疑问的回应

### 7.1 cache-coherency.md 中的疑问

**Q: ccNUMA 中 cache coherence 如何设计？**

本章虽然没有深入讲解具体的 cache coherency 协议（如 MESI/MOESI），但在 "Hardware System Architecture" 小节中通过简化流程展示了其基本思想：当 CPU 1 写入一个缓存行时，如果该缓存行在 CPU 6 的缓存中，系统需要经过多级互联将该缓存行迁移到 CPU 1，并使其在 CPU 6 中失效。这揭示了 ccNUMA 系统中跨节点缓存一致性的基本开销来源。

**Q: cache 中的三级 cache 同步时，需要每一个层级都同步吗？**

本章的 Table 3.4 展示了缓存几何结构：L0/L1/L2 是 per-core 的，L3 是 per-socket 的。数据以缓存行为单位移动。虽然本章没有详细讨论各级缓存间的精确同步协议，但 "Lifetime of a Simple Store" 图示展示了缓存行从远程 CPU 缓存迁移到本地 CPU 缓存的过程，暗示了 coherence 是在缓存行级别而非整个缓存层级同步的。

**Q: 中断时是否需要 flush write buffer？**

本章 "Interrupts" 小节讨论了中断处理的延迟来源，但没有直接回答 write buffer 的问题。不过结合 Linux 内核的实现来看，x86 的 `lock` 前缀和内存屏障指令（如 `mfence`）会隐式地确保 store buffer 的排空，因此在中断处理的关键路径上，如果代码中已使用了适当的锁或屏障，store buffer 的一致性是有保证的。

### 7.2 memory-model.md 中的疑问

**Q: 能不能稳定复现 memory model 的效果？**

本章的 `memory_barrier_demo.c` 提供了一个起点，展示了 acquire/release 语义如何保证临界区内操作不被重排序到锁外。虽然在 x86 这种强一致性架构上难以复现乱序导致的数据损坏（因为 `smp_rmb()` 和 `smp_wmb()` 退化为编译器屏障），但在 ARM 等弱一致性架构上，缺少屏障的"破损锁"确实可能导致重排序引发的错误。本章的代码框架可以作为构建更复杂 memory model 测试程序的基础。

**Q: 为什么需要 `smp_mb` 而不仅是 `smp_load_acquire` / `smp_store_release`？**

本章 "Memory Barriers" 小节解释了屏障的目的：防止 CPU 和编译器重排序。`smp_load_acquire` 和 `smp_store_release` 分别只保证单向的排序约束（acquire 阻止后续操作前移，release 阻止前面操作后移），而 `smp_mb()` 提供全屏障，阻止所有方向的重排序。在需要同时保证加载和存储顺序的场景（如双重检查锁定），必须使用全屏障。

**Q: `smp_store_mb` 的实现差异？**

本章没有直接分析这个宏，但结合内核源码：x86 的 `smp_store_mb()` 使用 `xchg` 指令（自带 `lock` 前缀），因为 x86 的 `lock` 指令同时提供原子性和全屏障语义。ARM64 则先 `WRITE_ONCE` 再 `__smp_mb()`，因为 ARM64 需要显式的 `dmb` 屏障指令。

### 7.3 engineerings-perspective.md 中的疑问

**Q: 数组中分配资源如何使用原子操作？**

本章的 "Atomic Operations" 小节和 `atomic_overhead_demo.c` 展示了 CAS 的基本用法。对于数组中资源的分配，可以使用 test-and-set 模式：用 `__atomic_test_and_set` 或 `cmpxchg` 原子地标记数组元素为"已分配"状态。本章的数据表明，如果这种分配是频繁操作，应该让每个 CPU 优先使用本地资源（per-CPU 分配器），避免跨 CPU 的原子操作。

### 7.4 perfbook/overview.md 中的疑问

**Q: Lock 是靠代码实现还是硬件实现的？**

本章明确回答了这个问题：锁的正确性同时依赖**硬件提供的原子操作**（如 x86 的 `lock` 前缀、ARM 的 LDXR/STXR）和**软件实现的内存屏障**。硬件提供底层的原子性保证，软件（内核/库）利用这些硬件原语构建更高层次的同步原语（如自旋锁、互斥锁）。

---

## 八、总结

本章是 perfbook 中承上启下的关键一章。它没有停留在抽象层面，而是用具体的数字和物理原理告诉读者：并行程序的性能天花板在哪里，以及这些天花板来自何处。

核心 takeaway：

1. **好消息**：多核系统廉价且普及；现代同步操作的开销远低于 2000 年代初的并行系统。
2. **坏消息**：缓存未命中的开销仍然很高，尤其是在大型系统上。
3. **行动指南**：并行算法必须显式地为硬件特性而设计——减少通信、追求只读共享、向尴尬并行靠拢。

正如本章开篇所言："Premature abstraction is the root of all evil." 在理解这些硬件特性之前，过早的抽象会让并行程序陷入不可预测的性能陷阱。

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
