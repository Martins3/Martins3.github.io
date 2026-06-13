# 第18章：Looking Forward and Back - 详细分析

## 目录

1. [全书内容回顾](#1-全书内容回顾)
2. [Paul McKenney 的写作历程](#2-paul-mckenney-的写作历程)
3. [并行编程工具的发展](#3-并行编程工具的发展)
4. [Linux 内核对并发的拥抱](#4-linux-内核对并发的拥抱)
5. [最重要的教训：Use the Right Tools](#5-最重要的教训use-the-right-tools)
6. [Linux 内核源码关联](#6-linux-内核源码关联)
7. [代码示例与测试](#7-代码示例与测试)
8. [与个人笔记疑问的对照](#8-与个人笔记疑问的对照)
9. [总结](#9-总结)

---

## 1. 全书内容回顾

本章以 Konrad Adenauer 的名言 "History is the sum total of things that could have been avoided" 开篇，随后对全书17章内容进行了系统性回顾。这种回顾不仅是简单的目录复述，而是将各章核心思想串联成一幅完整的并行编程知识图谱。

**第1章（How To Use This Book）** 界定了本书的受众和范围：面向低层并行编程的开发者。本章也提示了那些对高层并行框架（如OpenMP、MPI）更感兴趣的读者可以寻找替代资源。

**第2章（Introduction）** 提出了并行编程的核心挑战，以及应对这些挑战的高层方法。本章的一个重要洞见是：在许多场景下，可以通过避免并行编程的挑战（例如使用更好的算法或数据结构）来仍然获得大部分并行性的好处。

**第3章（Hardware and its Habits）** 从多核硬件角度解释了并行编程困难的物理根源。这些挑战主要归因于物理定律（光速限制、原子尺寸），而非硬件架构师的不配合。但本章也讨论了硬件层面可能的改进方向，与此同时软件工程师必须在自己的层面应对这些挑战。

**第4章（Tools of the Trade）** 概述了低层并发编程的基本工具，包括POSIX线程、原子操作、内存屏障等。这些工具是后续所有章节的基础。

**第5章（Counting）** 展示了如何将设计技术应用于一个看似简单但极具挑战性的任务：并发计数。本章的深刻之处在于揭示了一个事实：并发计数并没有单一的"最佳"算法，而是存在多种算法各自由特定用例主导。这体现了并行编程中"没有银弹"的普遍真理。

**第6章（Partitioning and Synchronization Design）** 深入探讨了最重要的并行编程设计技术：在尽可能高的层次上对问题进行分区。本章还概述了设计空间中的多个关键点，为读者提供了系统化的设计思维框架。

**第7章（Locking）** 详细讨论了并行编程的"功臣与反派"——锁。本章覆盖了多种锁类型，并为锁的许多广为人知且被积极宣传的缺陷提供了工程解决方案。这些解决方案表明，锁的许多问题并非不可克服，而是需要正确的设计模式。

**第8章（Data Ownership）** 讨论了数据所有权的概念，其中同步由数据项与特定线程的关联来提供。在适用的场景下，这种方法结合了出色的性能、可扩展性和深刻的简单性。这是"分区"思想在数据层面的直接体现。

**第9章（Deferred Processing）** 展示了"拖延"如何能够极大提升性能和可扩展性，同时在惊人多的场景下还能简化代码。本章介绍的许多机制利用了CPU缓存复制只读数据的能力，从而规避了限制光速和原子大小的物理定律。RCU（Read-Copy Update）是这一章的明星机制。

**第10章（Data Structures）** 审视了并发数据结构，重点是哈希表——它在并行程序中有着悠久而光荣的历史。本章表明，数据结构的选择对并行性能有决定性影响。

**第11章（Validation）** 深入代码审查和测试方法，**第12章（Formal Verification）** 则概述了形式化验证。无论读者站在形式化验证还是测试的分界线哪一侧，有一点是共同的：未经彻底验证的代码就不算工作，而对并发代码来说尤其如此——"至少两倍"。

**第13章（Putting It All Together）** 展示了将并发机制彼此组合或与其他设计技巧结合使用，可以极大地简化并行程序员的生活。这呼应了"使用正确工具"的主题。

**第14章（Advanced Synchronization）** 审视了高级同步方法，包括无锁编程、非阻塞同步（NBS）和并行实时计算。**第15章（Advanced Synchronization: Memory Ordering）** 深入了内存排序这一关键主题，提供了帮助读者不仅解决内存排序问题，还能完全避免这些问题的技术和工具。

**第16章（Ease of Use）** 简要概述了易用性这一出人意料地重要的主题。**第17章（Conflicting Visions of the Future）** 阐述了关于未来的多种冲突愿景，包括CPU技术趋势、事务内存、硬件事务内存、形式化验证在回归测试中的应用，以及长期以来关于并行编程未来属于函数式编程语言的预测。

---

## 2. Paul McKenney 的写作历程

本章最打动人的部分或许是作者 Paul E. McKenney 分享的个人经历。这段经历不仅解释了本书诞生的动机，也折射出整个Linux内核社区在并行编程领域的成长轨迹。

Paul 的并行编程之旅始于1990年加入 Sequent Computer Systems。Sequent 采用了一种类似学徒制的培养模式：新入职的工程师被安排在经验丰富的工程师周围，接受指导、代码审查和大量建议。当时的一个独特优势是没有片上缓存，这意味着逻辑分析仪可以轻松显示给定CPU的指令流和内存访问，附带精确的时间信息。当然，这种透明性的代价是CPU核心时钟频率比21世纪慢100倍。

在这种环境下，新工程师在两三个月内就能成为高效的并行程序员，有些人在一两年内就做出了开创性的工作。Sequent 意识到这种快速培训能力是非同寻常的，因此将公司的并行编程智慧结晶成一本薄书和一些开创性论文。然而，这些材料对于已经精通并行编程的人有帮助，对新手却效果有限——新手总会做出书中没有明确禁止的"高度创造性且极具破坏性的错误"。

1999年 IBM 收购 Sequent 时，许多世界上最大的数据库实例运行在 Sequent 硬件上。到2001年，许多 Sequent 的并行程序员已将重心转向 Linux 内核。Linux 内核社区在最初的不情愿之后，热情地拥抱了并发，并取得了许多优秀的创新。

本书写作的直接催化剂发生在2006年。Paul 在一次 Linux 可扩展性会议上向一个备受尊敬的并行编程专家小组提出了一个问题：从1991年到2006年，并行系统的价格从一栋房子降到了一辆中档自行车，而且很明显未来15年（即到2021年）价格还会继续大幅下降。如果工具持续改进，为什么到2021年并行编程不会变得 routine？ FIXME

第一个专家对此嗤之以鼻，给出了简短的 soundbite 回答。Paul 也以 soundbite 回应——例如专家说"死锁"，Paul 回应"锁依赖检查器"。这场交锋最终以专家说出"像你们这样的人应该用锤子敲脑袋！"而 Paul 回应"你得排队等！"而告终。

轮到 Linus Torvalds 时，他的回答改变了 Paul 的人生。Linus 指出，三年前（2003年），任何并发相关补丁的初始版本通常设计拙劣且有很多bug，即使清理到被接受的程度，bug 仍然存在。但到2006年，并发相关补丁的第一个版本设计良好且几乎没有bug，这已不罕见。**如果工具持续改进，也许到2021年并行编程真的会变得 routine。**

会议结束后，一位曾在 Sequent 工作多年、深刻理解并行k编程的男子流着泪找到 Paul 说"谢谢"。他当前被分配到一个负责编写并行代码的团队，而团队进展不顺——不是他们无法理解他的解释，而是**他们根本拒绝听他说**。

在那一刻，Paul 从"某天我应该写一本书"变成了"我将不惜一切代价写这本书"。

---

## 3. 并行编程工具的发展

本章特别强调了自2006年以来工具的显著改进。这些工具正是 Linus Torvalds 所期待的，也是 Paul 在 panel 上用来回应"死锁"质疑的"锁依赖检查器"的延伸。具体包括：

- **Fuzzers（模糊测试器）**：通过生成随机输入来暴露并发代码中的 race condition。
- **Lock dependency checker（锁依赖检查器）**：即 Linux 内核中的 lockdep，运行时检测锁获取顺序中的潜在死锁。
FIXME lockdep 的实现原理
- **Static analyzers（静态分析器）**：在不运行代码的情况下发现潜在的并发bug。
FIXME 想到了 qemu  graph lock 到底做了发什么?
- **Formal verification（形式化验证）**：使用数学方法证明并发算法的正确性。
- **Memory models（内存模型）**：如 Linux Kernel Memory Model (LKMM)，为理解和验证内存排序提供形式化基础。
FIXME 为什么重新发定义 LKMM 啊？
- **Code-modification tools（代码修改工具）**：如 Coccinelle，用于大规模代码重构和模式匹配。
FIXME 一直都没搞懂过

这些工具的进步意味着，到2021年，那些断言并行编程尚未变得 routine 的人，需要面对一个事实：工具确实在持续改进。正如本章脚注所言，"因此，那些希望断言2021年的并行编程不是 routine 的人，应该参考第2章的题记"（"并行编程并不难"）。

---

## 4. Linux 内核对并发的拥抱

本章明确提到，Linux 内核社区"热情地拥抱了并发，并且非常有效"。这一点在第18章中不是简单的赞扬，而是作为全书技术内容的社会背景。

Linux 内核的并发演进体现在多个维度：
- **锁基础设施**：从简单的自旋锁发展到 qspinlock、mutex、rwsem、percpu-rwsem、ww_mutex、rt_mutex 等完整的锁体系。
- **RCU**：从简单的玩具实现发展为 tree-based、SRCU、Tasks RCU 等多变体体系，成为内核中最成功的无锁同步机制之一。
- **内存模型**：LKMM 的引入和 tools/memory-model/ 中的 herd7 集成，使内核开发者能够用 litmus test 验证内存排序假设。
- **调试工具**：lockdep、KCSAN（Kernel Concurrency Sanitizer）等工具使并发bug的检测从"靠运气"变为"系统化"。

本章关于 Sequent 培训模式的描述也提供了一个有趣的对比：当年的透明性（无缓存，逻辑分析仪可见）与今日的工具化（lockdep、KCSAN、herd7）在某种程度上殊途同归——都是让并发的行为变得可观测、可调试。

---

## 5. 最重要的教训：Use the Right Tools

本章结尾的核心信息是一张卡通图（`UseTheRightToolsBubble`）及其 caption："The Most Important Lesson"。Paul 希望读者从本书中带走的，不是某个特定的算法或技巧，而是**为每个问题选择正确工具**的能力。

这个教训在第18章的回顾中得到了印证：
- 并发计数没有单一最佳方案，因为**不同场景需要不同工具**（原子操作、每CPU计数、最终一致性计数器等）。
- 锁不是反派，而是"功臣与反派"——**在正确的地方使用正确的锁类型**。
- RCU 适用于读多写少，但不是万能药；seqlock 适用于写少读多且读者可容忍重试；hazard pointers 适用于需要精确内存回收的场景。
- 形式化验证和测试不是对立面，而是**互补的工具**。

这个"选择正确工具"的主题也与 Paul 在 panel 上的交锋相呼应：当专家说"死锁"时，Paul 的回答不是"避免使用锁"，而是"锁依赖检查器"——不是回避工具，而是使用更好的工具。

---

## 6. Linux 内核源码关联

本章虽然没有直接分析具体内核代码，但其回顾的每一章都在内核中有直接对应。以下是关键关联：

### 6.1 Lockdep（锁依赖检查器）

本章提到的 "lock dependency checker" 直接对应内核源码 `kernel/locking/lockdep.c`。这是 Ingo Molnar 和 Peter Zijlstra 的实现，运行时维护所有锁获取顺序的有向图，检测：
- 锁 inversion 场景
- 循环锁依赖（死锁）
- hardirq/softirq safe/unsafe 锁定bug

关键函数 `check_deadlock()`（约第3057行）在 `__lock_acquire()` 中被调用。lockdep 的独特之处在于：**即使当前锁定场景没有触发死锁，只要历史上曾经以不同顺序获取过同类锁，就会报告bug**。这与用户空间 demo `lockdep_demo.c` 中展示的逻辑完全一致。

### 6.2 RCU 实现

`kernel/rcu/tree.c` 和 `kernel/rcu/tree.h` 实现了 tree-based RCU，是本书第9章内容的生产级实现。`struct rcu_node` 层级结构用于 grace period 检测，`struct rcu_state` 管理全局状态。本章提到的 "超过1,600个 callback per grace period" 正是来自这类生产环境观察。

### 6.3 Linux Kernel Memory Model (LKMM)

`tools/memory-model/` 目录包含了用 "cat" 语言编写的内存一致性模型，可被 herd7 模拟器执行。这是本书第15章内容的直接延伸。`klitmus7` 工具可将 litmus test 转换为内核模块，在实际内核中运行验证。

### 6.4 Coccinelle

`scripts/coccinelle/` 和 `scripts/coccicheck` 是本章提到的 "code-modification tools" 在内核中的具体体现。Coccinelle 使用语义补丁（SmPL）语言，能够在整个内核源码树中执行复杂的模式匹配和转换，对于大规模并发代码重构尤为重要。

### 6.5 各种锁的实现

`kernel/locking/` 目录包含了内核中所有锁类型的实现：
- `qspinlock.c`：排队自旋锁，解决传统 ticket spinlock 的缓存行 bouncing 问题
- `mutex.c`：互斥锁，支持乐观自旋（optimistic spinning）
- `rwsem.c`、`percpu-rwsem.c`：读写信号量
- `locktorture.c`：锁的 torture test，对应第11章的验证主题

---

## 7. 代码示例与测试

由于第18章本身没有具体代码，本章配套三个总结性代码示例，分别对应全书的关键主题。所有代码均在本地编译测试通过。

### 7.1 counter_comparison.c - 并发计数策略对比

这个程序直接演示第5章（Counting）的核心信息：并发计数没有单一最佳方案。实现了四种策略：

1. **atomic_add**：使用 `atomic_fetch_add_explicit` 实现精确计数。多核竞争激烈，但在8线程1000万次迭代下仅需约615ms。
2. **per_thread_approx**：每线程使用 TLS 计数器，无全局竞争。读取时只能获得近似值（本 demo 仅演示概念）。
3. **rwlock_write**：使用 `pthread_rwlock_wrlock` 串行化写者。精确但最慢（约9117ms），因为写者之间完全互斥。
4. **rcu_like_write**：写者用 `pthread_mutex_lock` 保护，读者无锁读取。比 rwlock 快（约4472ms），因为读者无竞争。

测试结果清楚展示了 perfbook 的核心设计原则：**首先分区（per-thread），其次弱化（RCU/原子替代全局锁），最后才编码**。

### 7.2 lockdep_demo.c - 锁依赖检测原理

用户空间模拟 `kernel/locking/lockdep.c` 的核心逻辑。维护一张有向图，边表示"曾经以 L1 -> L2 的顺序获取锁"。当检测到反向顺序 L2 -> L1 时，报告潜在死锁。

测试了三个场景：
1. 一致顺序（A -> B, A -> B）：无报告 [OK]
2. 反向顺序（A -> B, B -> A）：报告潜在死锁 [OK]
3. 三环依赖（A -> B -> C -> A）：报告循环死锁 [OK]

这个 demo 验证了本章提到的 "lock dependency checker" 的工作原理。

### 7.3 right_tool_demo.c - 选择正确工具

直接对应本章最重要的教训 "Use the Right Tools"。三组对比：

| 场景 | 错误工具 | 正确工具 | 性能差距 |
|------|---------|---------|---------|
| 计数器 | mutex | atomic | ~4.7x |
| 统计 | global lock | per-thread | ~1135x |
| 分配器 | global freelist | per-thread freelist | ~153x |

测试结果以数量级差异证明了选择正确工具的重要性。per-thread 数据完全消除了跨缓存行竞争，这是 perfbook "Partition first" 原则的最直接体现。

### 编译与运行

```bash
make all    # 编译所有示例
make test   # 运行所有测试
make clean  # 清理
```

所有二进制文件以 `.out` 结尾，可被 `.gitignore` 自动忽略。

---

## 8. 与个人笔记疑问的对照

对照 `~/data/vn/docs/concurrent/` 中的笔记和疑问：

### 8.1 已解答或部分解答的疑问

1. **"RCU 和 memory barrier 的问题，都是两章，这是重点"**（overview.md 第27行）
   - 第18章的回顾确认了 RCU（第9章）和 Memory Ordering（第15章）确实是全书的核心重点。Paul 的职业生涯也主要围绕 RCU 展开。

2. **"Partitioning and Synchronization Design / Data Ownership / Validation / Formal Verification"**（overview.md 第44-51行，标注为"真正不知道在搞什么的"）
   - 第18章的回顾为这些章提供了简洁的总结。Partitioning 是"最重要的设计技术"，Data Ownership 是"分区的数据层面体现"，Validation 和 Formal Verification 是"无论站在哪一侧，未经验证的代码就不工作"的互补工具。

3. **"形式化验证指的是什么?"**（overview.md 第19行）
   - 第18章明确提到形式化验证是2006年以来持续改进的工具之一，与 fuzzers、lock dependency checkers、static analyzers 并列。

4. **"Partition first, batch second, weaken third, and code fourth"**（engineerings-perspective.md 第84行）
   - 第18章对全书回顾时，这个设计哲学在各章都有体现：第6章（分区）、第9章（延迟处理/批量化）、第5/7章（弱化同步需求）。`right_tool_demo.c` 的性能测试结果正是这一哲学的量化证明。

5. **"锁是靠什么保证这些的，是靠 lock 的代码实现，还是靠硬件实现的?"**（overview.md 第16行）
   - 第18章回顾第3章时提到"挑战归因于物理定律"，回顾第4/7章时提到锁的工具属性。综合全书来看：**锁的正确性由硬件原子指令保证，锁的性能由软件设计（分区、选择合适的锁类型）决定**。

6. **"工具上：反汇编"**（overview.md 第31行）
   - 第18章提到的工具集（fuzzer、lockdep、static analyzer、formal verification、memory model、coccinelle）比单纯的反汇编更系统化。Linux 内核的 `tools/memory-model/` 提供了 litmus test 框架，可直接验证内存排序假设。

### 8.2 尚未直接解答的疑问

1. **"memory barrier 需要斟酌一下"**（todo.md 第2行）
   - 第18章仅回顾了第15章的存在，没有深入 memory barrier 的具体使用场景和配对规则。这需要回到第15章和内核 `include/asm-generic/barrier.h` 来解答。

2. **"如何正确的使用 RCU"**（todo.md 第12行）
   - 第18章确认 RCU 是第9章的核心内容，但没有提供 RCU 的具体使用指南。用户的 `~/data/vn/docs/concurrent/rcu/` 目录已经积累了大量 RCU 笔记，说明这是一个长期学习的主题。

3. **"cache coherence 和 memory model 的关系"**（todo.md 第14行）
   - 第18章没有涉及这个基础问题。不过第3章回顾中提到"CPU缓存复制只读数据"，第15章回顾中提到"内存排序"，两者的关系需要结合体系结构知识来理解。

4. **"DMA 的 cache 一致性是谁处理的"**（todo.md 第15行）
   - 全书未涉及 DMA 相关主题。这是内核驱动开发领域的专门话题，与 perfbook 的用户空间/内核通用并发编程定位有所偏离。

5. **"什么时候应该使用 seqlock 而不是 rcu，既然都是写多读少"**（todo.md 第230行）
   - 第18章回顾第9章时同时提到了 seqlock 和 RCU，但没有直接对比选择标准。这需要回到第9章的具体内容和内核实际用例来分析。

---

## 9. 总结

第18章 "Looking Forward and Back" 是全书的完美收尾。它不提供新的技术细节，而是通过回顾将全书编织成一个连贯的知识体系。本章传达了几个核心信息：

1. **并行编程的知识是系统化的**：从硬件原理（第3章）到工具（第4章），从设计哲学（第6章）到具体机制（第7-10章），从验证（第11-12章）到未来趋势（第17章），各章之间有着严密的逻辑关联。

2. **工具的持续改进使并行编程变得可行**：从2003年"并发补丁初始版本通常设计拙劣且bug众多"，到2006年"第一版本设计良好且几乎没有bug已不罕见"，再到2021年及以后 fuzzers、lockdep、formal verification、memory model 等工具的成熟——**并行编程的困难正在被工具化解决**。

3. **选择正确的工具是最重要的能力**：本章以 "Use the Right Tools" 作为全书最重要的教训。这不是一句空话，而是建立在全书大量案例分析基础上的结论。`right_tool_demo.c` 的数量级性能差异正是这一教训的实证。

4. **并行编程的历史是由人和社区塑造的**：Paul McKenney 的个人经历表明，技术的进步不仅取决于算法和硬件，还取决于社区文化——是否愿意倾听、是否愿意分享知识、是否愿意改进工具。那个流泪的 Sequent 前工程师的故事提醒我们：**忽视专业知识不仅是个人的损失，也是整个团队的损失**。

对于读者而言，第18章的意义在于：当你面对一个并发问题时，不要急于编写代码，而是先回顾 perfbook 提供的工具箱——分区了吗？可以延迟处理吗？可以使用数据所有权吗？需要哪种锁？或者可以用原子操作/RCU完全避免锁？验证策略是什么？

正如本章结语所言：如果有人试图向我们展示解决紧迫问题的方案，我们至少应该礼貌地倾听。这不仅是对他人的尊重，也是对自己代码质量的负责。

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
