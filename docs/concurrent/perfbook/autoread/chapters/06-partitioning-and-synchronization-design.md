# Partitioning and Synchronization Design

## 一、章节核心思想概述

FIXME 为什么总是感觉 data ownership 哪一个章节和这个是讲的同一个问题。
我还以为我在读 data ownership 呢?

本章的开篇格言来自马其顿的腓力二世："Divide and rule"（分而治之）。这句话精准概括了整章的核心主题——通过合理的数据分区（Partitioning）来设计可扩展的并发程序。作者Paul E. McKenney强调了一个关键的设计顺序：**Partition first, batch second, weaken third, and code fourth**（先分区，再批处理，然后弱化同步，最后编码）。改变这个顺序往往会导致性能差、可扩展性低和巨大的挫败感。

本章讨论的是如何为现代多核系统设计软件，使用设计模式（design patterns）来平衡性能、可扩展性和响应时间。正确分区的问题会导致简单、可扩展、高性能的解决方案；而错误分区则会导致缓慢且复杂的解决方案。

## 二、分区练习（Partitioning Exercises）

### 2.1 哲学家就餐问题（Dining Philosophers Problem）

这是并发编程中最经典的问题之一。五个哲学家坐在圆桌旁，每人需要左右两个叉子才能吃"非常难对付的意大利面"。关键约束是：所有哲学家必须能够同时就餐，不能有任何一个人饿死。

**教科书解法的问题**：传统的解法是给叉子编号，每个哲学家先拿编号较小的叉子。这虽然能防止死锁，但在最坏情况下，五个哲学家中只有一个能同时吃饭。例如：P2拿叉子1、P3拿叉子2、P4拿叉子3、P5拿叉子4，然后P5再拿叉子5开始吃，吃完后P4才能吃——形成了串行化。

**分区解法**：本章提出了两种更优方案：
1. **部分分区**：将哲学家分成两组，上方和右方的哲学家共享一对叉子，下方和左方的哲学家共享另一对。这样至少有两个哲学家能同时吃。
2. **完全分区**：干脆给每个哲学家配一副独立的叉子（共5对）。这样所有五个哲学家可以同时吃，同步开销几乎为零。这就是"水平并行"（horizontal parallelism）或"数据并行"（data parallelism）的典范。

作者在Quick Quiz中半开玩笑地说这种"作弊"是解决很多并发问题的关键。我的C代码实现（`dining_philosophers.out`）对比了教科书解法和完全分区解法：

```
--- Textbook (numbered forks) ---
textbook                  time=0.0325s total_eats=500000 throughput=15370913.99 eats/sec
--- Fully Partitioned (one pair per philosopher) ---
partitioned               time=0.0146s total_eats=500000 throughput=34259206.67 eats/sec
```

完全分区版本的吞吐量是教科书版本的两倍多，因为不存在任何锁竞争。这完美诠释了分区带来的性能收益。

### 2.2 双端队列（Double-Ended Queue）

双端队列允许从两端进行插入和删除操作。本章展示了三种实现方式：

**左右手锁（Left- and Right-Hand Locks）**：使用双向链表，左端操作加左锁，右端操作加右锁。问题在于当队列元素少于4个时，两个锁的保护域必须重叠（因为删除一个元素会影响其左右邻居）。这导致至少5种特殊情况，并发活动可能随时将队列从一种特殊情况转换到另一种，极其复杂。

**复合双端队列（Compound Double-Ended Queue）**：运行两个独立的DEQ，各受自己的锁保护。当一侧为空时，需要从另一侧搬运元素。这使用简单的锁层次（先左后右）避免死锁。`locktdeq.c`实现了这个方案。虽然比hash方案简单，但最多只能达到2倍加速（因为同时只有两个锁可以被持有）。

**哈希双端队列（Hashed Double-Ended Queue）**：通过哈希将队列分区。给每个元素分配一个基于位置的序列号，左端入队递减（-1, -2, -3...），右端入队递增（2, 3, 4...）。使用一个锁保护左端索引，一个保护右端索引，每个哈希链一个锁。`lockhdeq.c`实现了这个方案。锁域不重叠，死锁通过获取顺序避免（先索引锁后链锁）。当哈希桶足够大时，左端入队和右端入队之间的锁竞争概率可以降到任意低水平。

作者指出一个关键洞察：对于严格FIFO的队列，无论哪种并行实现都有显著开销。如果一个程序的所有数据都通过单个队列传递，那应该重新考虑整体设计。更好的方法是"水平扩展"——使用多个队列并行。

## 三、设计标准（Design Criteria）

本章提出了五个评估并发算法质量的核心标准：

### 3.1 Speedup（加速比）
加速比定义为串行版本运行时间与并行版本运行时间的比率。这是并行化的主要目的。根据Amdahl定律，要达到无界线性可扩展性，程序在任意独占锁临界区内花费的时间比例必须随CPU数量增加而减少。例如，程序要扩展到10个CPU，在最受限的独占锁临界区内花费的时间必须远少于1/10。

### 3.2 Contention（争用）
当应用到并行程序的CPU数量超过程序能保持忙碌的数量时，多余的CPU会因争用而无法做有用工作。争用可以是锁争用、内存争用、缓存溢出、热节流等。实际加速比与可用CPU数量之间的差距越大，CPU使用效率越低。

### 3.3 Work-to-Synchronization Ratio（工作与同步比率）
单处理器、单线程、不可抢占、不可中断版本的程序不需要任何同步原语。因此，任何被这些原语消耗的时间（包括通信缓存未命中、消息延迟、锁原语、原子指令、内存屏障）都是开销。重要的是同步开销与临界区内代码开销之间的关系——更大的临界区能容忍更大的同步开销。

### 3.4 Read-to-Write Ratio（读写比率）
很少更新的数据结构可以通过复制而非分区来保护，并且可以使用非对称同步原语（如读写锁）来减少读者的同步开销，以牺牲写者为代价。对于频繁更新的数据结构，则有相应的优化（见第5章计数）。

### 3.5 Complexity（复杂度）
并行程序比等效串行程序更复杂，因为状态空间更大。复杂度会转化为更高的开发和维护成本。更糟糕的是，增加的复杂度实际上可能降低性能和可扩展性。因此，超过某一点后，可能存在比并行化更便宜、更有效的顺序优化。

**这些标准之间的关系**：
1. 程序在独占锁临界区内花费的时间越少，潜在加速比越大（Amdahl定律）
2. 当实际加速比小于可用CPU数量时，争用效应会消耗多余的CPU时间
3. 如果同步原语开销高，应通过批处理、数据所有权或非对称原语来减少调用次数
4. 如果临界区开销高，应通过读写锁、数据锁定或数据所有权增加并行度
5. 如果读远多于写，应使用读写锁或非对称原语

## 四、同步粒度（Synchronization Granularity）

本章用hash table搜索作为贯穿例子，展示了从粗到细的不同同步粒度。

### 4.1 顺序程序（Sequential Program）
如果程序在单处理器上运行得足够快，且与其他进程/线程/中断处理程序没有交互，应该移除同步原语。顺序优化带来的加速比可能是任意大的（例如选择更好的数据结构），而并行加速比通常受限于CPU数量。

### 4.2 代码锁定（Code Locking）
使用全局锁。很容易将现有程序改造为使用代码锁定。如果程序只有一个共享资源，代码锁定甚至能提供最优性能。但对于大型复杂程序，如果大量执行发生在临界区内，代码锁定会严重限制可扩展性。

代码锁定本质上是"分区时间"——给每个请求CPU分配一个时间片来拥有hash table。在良好设计的算法中，应该存在大量没有CPU拥有hash table的时间分区。

我的测试数据（`hash_table.out`）显示，在4线程100万次操作下：

```
sequential            threads=4 ops=1000000 time=0.1334s throughput=7494546.22 ops/sec
code_lock             threads=4 ops=1000000 time=0.5841s throughput=1711915.47 ops/sec
data_lock             threads=4 ops=1000000 time=0.1509s throughput=6628574.33 ops/sec
rw_lock               threads=4 ops=1000000 time=0.1385s throughput=7219154.84 ops/sec
```

代码锁定版本的吞吐量只有顺序版本的约23%，因为所有线程都在竞争同一个锁。更有趣的是，4线程的代码锁定版本（1.71M ops/sec）比单线程顺序版本假设值（约7.5M ops/sec）慢得多——这展示了锁竞争的开销。

### 4.3 数据锁定（Data Locking）
将数据结构分区，每个分区有自己的锁。不同分区的临界区可以并行执行。例如hash table的每个桶一个锁。数据锁定减少了争用，但增加了复杂度（需要额外的数据结构如`struct bucket`）。

数据锁定同时分区地址空间（每个桶一个分区）和时间（使用每个桶的锁），因此可以称为"分区时空"。

但数据锁定也有问题：数据倾斜（data skew）。Linux内核的dcache中，根目录及其直接子目录的条目比其他条目更常被遍历，导致许多CPU竞争这些热门条目的锁。Linux内核通过RCU等技术解决了这个问题。

数据锁定在动态分配结构上的关键挑战是：获取锁时确保结构仍然存在（existence guarantee）。解决方案包括：
1. 使用静态分配的锁（如hash桶中的锁）
2. 使用锁数组，通过结构地址哈希选择锁
3. 使用垃圾收集器
4. 使用引用计数
5. 使用Hazard Pointers
6. 使用事务内存（TM）
7. 使用RCU

### 4.4 数据所有权（Data Ownership）
将数据结构分区给线程/CPU，每个线程访问自己的子集而无需任何同步开销。如果一线程需要访问另一线程的数据，必须通过通信让另一线程代为执行，或迁移数据。

数据所有权可能比数据锁定更简单（代码可以像顺序程序一样简单），达到理想性能。只读数据可以通过复制让所有线程"拥有"。数据所有权只分区地址空间，不必分区时间，因为给定线程被永久分配了给定地址空间分区的所有权。

常见的数据所有权例子：
- 仅一个CPU/线程可访问的变量（如C/C++的auto变量）
- 用户界面实例拥有对应用户的上下文
- 参数模拟中每个线程拥有一块参数空间

### 4.5 锁定粒度与性能（数学分析）

本章使用M/M/1排队模型分析同步效率。设到达率lambda = n * lambda0（n为CPU数），服务率mu为无竞争时的同步操作速率。期望等待时间 T = 1/(mu - lambda)。效率 e = (f - n) / (f - (n - 1))，其中f = mu/lambda0是事务处理时间与同步开销的比值。

关键结论：基于对单个全局共享变量的原子操作的同步机制，在当前商品硬件上重度使用时无法良好扩展。

矩阵乘法的例子进一步说明：即使是没有冲突的操作（理论上效率应为1.0），实际效率也远低于理想值。64x64矩阵乘法在单线程下效率都不到0.3。这是因为线程创建开销、缓存未命中等因素。如果必须承担同步开销，就要让每次同步"物有所值"——即增大批处理量。

## 五、并行快速路径（Parallel Fastpath）

细粒度设计通常比粗粒度设计更复杂。但很多程序中，大部分开销由一小部分代码引起（Knuth的"90-10规则"）。并行快速路径设计模式的思想是：积极并行化常见情况（fastpath），而不承担并行化整个算法的复杂度。

### 5.1 读写锁（Reader/Writer Locking）
如果同步开销可忽略，且只有一小部分临界区修改数据，允许多个读者并行可以极大提高可扩展性。写者排除读者和其他写者。

我的测试数据显示，在只读工作负载下，读写锁（7.22M ops/sec）接近顺序版本（7.49M ops/sec），远超代码锁定（1.71M ops/sec）和数据锁定（6.63M ops/sec）。这是因为读者之间不互斥。但在写者频繁的场景下，读写锁的优势会减弱。

### 5.2 层次锁（Hierarchical Locking）
先获取粗粒度锁，只持有足够长的时间来确定要获取哪个细粒度锁，然后释放粗粒度锁并获取细粒度锁。对于hash table搜索例子，层次锁的弱点是：支付了获取第二个锁的开销，但只持有很短时间。在本例中，数据锁定更简单且可能性能更好。但如果比较操作非常耗时，层次锁可能更有优势。

### 5.3 资源分配器缓存（Resource Allocator Caches）

这是并行快速路径最重要的实例之一，也是本章最实用的设计模式。

**问题**：并行内存分配器需要在常见情况下提供极快的分配/释放，同时在不利分配模式下高效分发内存。

**简单数据所有权的问题**：将内存切分给每个CPU。例如12 CPU系统有64GB内存，每个CPU分配5GB。但如果CPU 0只分配、CPU 1只释放（简单的生产者-消费者模式），这个方案就失败了。

**代码锁定的问题**：全局锁导致过度锁争用。

**解决方案（Parallel Fastpath）**：每个CPU拥有一个适度的块缓存，外加一个大型代码锁定的共享池。当CPU释放块时本地缓存满了，就将块发送到全局池；当CPU分配块时本地缓存空了，就从全局池获取。

我的实现（`allocator_cache.out`）展示了这一模式：

```c
struct memblock *memblock_alloc(void)
{
    struct perthreadmempool *pcpp = &perthreadmem;
    if (pcpp->cur < 0) {          /* slowpath: local pool empty */
        pthread_mutex_lock(&globalmem.mutex);
        /* refill from global pool */
        pthread_mutex_unlock(&globalmem.mutex);
    }
    if (pcpp->cur >= 0) {         /* fastpath: local pool has block */
        return pcpp->pool[pcpp->cur--];
    }
    return NULL;
}
```

测试显示，2线程runlength=1时吞吐量约8.63亿对分配/释放每秒，runlength=6时约6.74亿。运行长度小于等于TARGET_POOL_SIZE（3）时完全在本地进行，无需锁；大于3时需要定期访问全局池。

## 六、超越分区（Beyond Partitioning）

本章以迷宫求解为例，展示了比"尴尬并行"（embarrassingly parallel）更强的"羞辱并行"（humiliatingly parallel）——添加线程不仅不增加总计算成本，反而显著降低了总计算成本，产生算法级的超线性加速。

### 6.1 三种迷宫求解算法

**SEQ（顺序算法）**：基于2D数组和visited工作队列的广度优先搜索变体。

**PWQ（并行工作队列）**：直接并行化SEQ。使用fetch-and-add来仲裁对visited数组的并发访问，使用CAS循环标记cell为已访问。但存在一个关键弱点：最多只有一个线程能沿解路径前进。

**PART（分区并行算法）**：从迷宫两端各启动一个子线程。每个子线程有自己的visited数组。当一个线程发现另一个线程已访问过的cell时，设置done标志。PART的中位求解时间（17ms）比SEQ（79ms）快4倍以上——尽管只用了两个线程。

### 6.2 超线性加速的原因

PART有时只访问不到2%的迷宫cell，而SEQ和PWQ从不低于约9%。原因是：如果左上角的线程到达某个"瓶颈"cell，右下角的线程就无法到达迷宫的右上部分；反之亦然。因此PART可能访问极少的非解路径cell。

这与数十年的并行编程经验相反：过去人们努力让线程**不**互相干扰，而这里线程**故意**互相干扰来剪枝搜索空间。这是"羞辱并行"——添加线程显著减少了总工作量。

### 6.3 顺序优化 vs 并行优化

虽然PART的加速比令人印象深刻，但不应忽视顺序优化。用-O3编译时，SEQ比未优化的PWQ快约两倍，接近未优化的PART。

更有趣的是，PART的存在启发了协程算法（COPART）：在单线程上手动切换两个搜索上下文。COPART在单线程上的性能接近PART在两线程上的性能（差距约30%）。这表明：**将并行作为一级设计技术（而非事后微优化）可以为顺序算法带来改进**。

### 6.4 关键教训

对于搜索问题，搜索策略比并行设计更重要。是的，对于这个特定类型的迷宫，智能地应用并行化识别出了更优的搜索策略，但这种运气不能替代对搜索策略本身的清晰关注。

并行化只是众多性能优化中的一种，而且主要适用于基于CPU的瓶颈。成功的设计需要关注最重要的优化——那可能是并行化，也可能不是。

## 七、与Linux内核源码的关联

### 7.1 SLUB分配器：资源分配器缓存的真实实现

Linux内核的SLUB（SLab Unified Block）分配器是本章"资源分配器缓存"模式的工业级实现。其架构完美诠释了本章表格"Schematic of Real-World Parallel Allocator"中的四层结构：

| Level             | Locking        | Purpose                           |
|-------------------|----------------|-----------------------------------|
| Per-thread pool   | Data ownership | High-speed allocation             |
| Global block pool | Data locking   | Distributing blocks among threads |
| Coalescing        | Data locking   | Combining blocks into pages       |
| System memory     | Code locking   | Memory from/to system             |

在`mm/slub.c`中，`slab_alloc_node()`是内联fastpath：

```c
static __fastpath_inline void *slab_alloc_node(struct kmem_cache *s, ...)
{
    void *object;
    object = alloc_from_pcs(s, gfpflags, node);  /* fastpath: per-cpu */
    if (!object)
        object = __slab_alloc_node(s, ...);      /* slowpath: global */
    /* ... */
    return object;
}
```

`alloc_from_pcs()`从per-cpu缓存（per_cpu_pageset，PCS）分配对象。这是数据所有权的fastpath，无锁。如果本地缓存为空，才进入慢路径`___slab_alloc()`，从节点级别的partial list获取或分配新slab，这需要锁。

### 7.2 dcache：数据锁定与数据倾斜

本章提到Linux内核的dcache（目录项缓存）作为数据锁定的例子。每个dentry有自己的`d_lock`，但根目录及其直接子目录的条目被频繁遍历，导致锁争用。

在`fs/dcache.c`中可以看到锁层次结构：
```
dentry->d_inode->i_lock
  dentry->d_lock
    dentry->d_sb->s_dentry_lru_lock
    dcache_hash_bucket lock
```

Linux通过RCU（Read-Copy Update）路径查找（pathname lookup）来缓解这个问题。在只读路径上，查找可以完全无锁进行；只在需要修改dcache时才获取锁。这对应于本章提到的"弱化"（weaken）同步的策略。

### 7.3 任务列表：数据锁定的另一个例子

本章提到Linux内核的任务列表（task list）是数据锁定的例子，每个task_struct有自己的`alloc_lock`和`pi_lock`。这允许不同进程的操作在很大程度上并行进行，只在访问同一进程时才需要串行化。

## 八、对用户疑问的回应

用户的笔记在`~/data/vn/docs/concurrent/todo.md`中记录了大量关于并发编程的疑问。本章内容对其中一些疑问有直接帮助：

### 8.1 "感觉锁的问题没有一个清晰的体系"

本章恰好提供了一个清晰的体系框架。五个设计标准（speedup, contention, work-to-sync ratio, read-to-write ratio, complexity）和四种同步粒度（sequential, code locking, data locking, data ownership）构成了评估和选择同步策略的决策树：

1. **先判断是否需要并行**：如果单线程够快，不要加同步
2. **如果需要 modest scaling 或临界区很小**：代码锁定
3. **如果需要减少争用且数据结构可分区**：数据锁定
4. **如果没有共享**：数据所有权（尴尬并行）
5. **如果读远多于写**：读写锁/RCU/序列锁
6. **如果常见情况可预测**：并行快速路径

### 8.2 "memory barrier 需要斟酌一下"

本章虽然没有深入讨论memory barrier，但在迷宫求解的PART算法中使用了`READ_ONCE()`来防止编译器优化合并连续加载或重新加载值。这触及了内存序问题的一个侧面：在多线程共享标志位（如`mp->done`）时，即使不需要强同步，也需要阻止编译器优化。

### 8.3 "如何正确使用 RCU"

本章多次提到RCU作为存在性保证（existence guarantee）的解决方案，以及作为读写锁的高性能替代。RCU将在第9章"Deferred Processing"中详细讨论。本章的关联在于：当数据结构读多写少且需要存在性保证时，RCU是比数据锁定更优的选择。

### 8.4 "观察各个系统中的 spin lock 和 mutex 才是重点"

本章虽然没有专门讨论spinlock vs mutex的选择，但所有的代码示例都使用了锁原语。关键洞察是：**锁的类型（spinlock/mutex）远不如锁的粒度（granularity）和范围（scope）重要**。一个全局mutex的灾难性影响远大于一个细粒度spinlock的轻微开销。

### 8.5 本章未解决的疑问

以下疑问本章未直接涉及，需要后续章节：
- `ACCESS_ONCE` / `READ_ONCE` / `WRITE_ONCE` 的精确语义和数据竞争的关系
- Memory model 的形成原因、CPU如何保证、对编程者的影响
- Cache coherence 和 memory model 的关系
- DMA的cache一致性由谁处理
- Wait-free / lock-free / obstruction-free 的具体区别和性能影响
- Hazard pointer 的详细实现

### 8.6 本章解决的具体疑问

用户的笔记中有一条："为什么感觉锁的问题没有一个清晰的体系"。本章通过以下方式回答了这个问题：

1. **提供了分层的同步粒度模型**：从sequential -> code locking -> data locking -> data ownership，每一层都有明确的适用场景
2. **提供了量化的设计标准**：用speedup、contention、work-to-sync ratio、read-to-write ratio、complexity五个维度评估设计
3. **提供了具体的决策规则**：例如"如果临界区开销高相比原语开销，应增加并行度"；"如果同步原语开销高，应批处理"
4. **用实际案例展示**：hash table的4种实现、deq的3种实现、哲学家问题的2种解法、内存分配器的fastpath设计

## 九、代码实现总结

本次分析包含三个可运行的C程序，位于当前目录：

1. **hash_table.c** / **hash_table.out**：实现了4种hash table搜索变体（sequential、code locking、data locking、reader-writer locking），支持多线程基准测试。

2. **allocator_cache.c** / **allocator_cache.out**：实现了并行快速路径内存分配器，包含per-thread缓存和全局池，展示fastpath/slowpath的分离。

3. **dining_philosophers.c** / **dining_philosophers.out**：实现了哲学家就餐问题的教科书解法和完全分区解法，对比两者的吞吐量。

使用 `make test` 可以运行全部测试。

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
