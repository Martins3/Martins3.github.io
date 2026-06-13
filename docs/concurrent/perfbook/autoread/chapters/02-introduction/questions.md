# 第 2 章是否解决了用户的疑问？

## 用户笔记中的核心疑问清单

通过分析 `~/data/vn/docs/concurrent/` 下的笔记，用户的主要疑问集中在：

1. **Memory barrier / Memory model**
   - "cache coherence 和 memory model 的关系"
   - "memory barrier 需要斟酌一下"
   - "总结 memory model 形成的原因，CPU 如何保证，对于编程者的影响"
   - "构建一个 memory model 测试程序"

2. **RCU**
   - "如何正确使用 RCU，以及 Linux 内核中的 ART"
   - "RCU 和 memory barrier 的问题，都是两章，这是重点"

3. **Lock / Atomic**
   - "lock 是靠什么保证这些的（是 lock 的代码实现，还是靠硬件实现的）"
   - "access once / data_race 完全不懂"
   - "观察各个系统中的 spin lock 和 mutex 才是重点"
   - "原子性和锁不是一个事情"

4. **Wait-free / Lock-free**
   - "各种数据结构的 wait free 转化的基本方法是什么"
   - "wait-free lock-free && obstruction free 的区别"

5. **Cache Coherency**
   - "关于 Cache Coherency，仔细阅读 chapter 2 就可以了"（用户以为本章会讲，但实际上本章是 Introduction）

6. **形式化验证**
   - "形式化验证指的是什么"

7. **工程实践**
   - "为什么感觉锁的问题没有一个清晰的体系"
   - "思考问题的本质是，这个资源不可以被同时做什么操作然后操作有什么特点"
   - "page table 的 lock 和 fs 目录的 lock"

---

## 本章 (Chapter 2: Introduction) 解决了哪些疑问？

### 已解决的疑问

| 疑问 | 解决程度 | 说明 |
|------|---------|------|
| **为什么并行编程被认为困难？** | 完全解决 | 本章明确指出困难主要来自历史原因（硬件昂贵稀有），而非技术不可逾越 |
| **锁的问题有没有清晰的体系？** | 部分解决 | 将并行访问控制归纳为：访问形式（隐式/显式）+ 协调机制（锁/消息/事务/引用计数/时序/原子变量/所有权） |
| **性能优化的优先级** | 完全解决 | 明确并行化只是性能优化的一种，顺序优化（如链表改哈希表）有时收益更大 |
| **什么时候应该并行化？** | 完全解决 | "如果你的程序已经够快，就不要并行化"；要先比较最优顺序算法 |
| **资源竞争的根本解决思路** | 完全解决 | "划分写密集型资源，复制读多写少资源" |
| **铁三角权衡** | 完全解决 | 性能/生产力/通用性不可兼得，帮助理解为什么不同场景选用不同技术 |

### 未解决的疑问

| 疑问 | 未解决原因 | 预期解决章节 |
|------|-----------|-------------|
| **Memory barrier 细节** | 本章是引言，不涉及具体硬件内存模型 | Chapter 3 (Hardware and its Habits) / Chapter 15 (Advanced Synchronization) |
| **RCU 使用方法** | 仅在"资源复制"一节提及 read-mostly 复制，未展开 RCU | Chapter 9 (RCU) |
| **Spinlock / Mutex 实现** | 仅列举为访问控制手段，未涉及具体实现 | Chapter 7 (Locking) / Chapter 8 (Data Ownership) |
| **Cache coherence 协议** | 用户笔记误以为本章会讲，实际本章是概念性 Introduction | Chapter 3 (Hardware and its Habits) |
| **Wait-free / Lock-free 数据结构** | 未涉及 | Chapter 10 (Data Structures) / Chapter 14 (Deferred Processing) |
| **形式化验证** | 未涉及 | Chapter 12 (Formal Verification) |
| **data_race / READ_ONCE** | 未涉及 | Chapter 4 (Tools of the Trade) / Chapter 15 |
| **Lock 是靠软件还是硬件实现？** | 本章提到锁是协调机制之一，未深入实现 | Chapter 7 (Locking) |
| **原子性和锁的区别** | 未深入 | Chapter 4 / Chapter 7 |

---

## 针对性建议

基于本章内容，给用户如下阅读建议：

### 1. 立即纠正一个误解

用户笔记中写道："关于 Cache Coherency，仔细阅读 chapter 2 就可以了"。

**实际情况**：Chapter 2 (Introduction) 完全不涉及 Cache Coherency 的具体内容。Cache coherence、内存屏障、存储缓冲区等硬件机制在 **Chapter 3 (Hardware and its Habits)** 中才会详细讨论。

### 2. 本章对"锁的体系"的启示

虽然本章没有讲具体锁实现，但提供了一个高维度的分类框架：

```
并行访问控制 = 访问形式（隐式/显式） + 协调机制
```

用户的困惑"为什么感觉锁的问题没有一个清晰的体系"，本质上是因为只看到了"锁"这一个坐标轴。本章提示应从更宽的视角看待同步：

- **数据所有权**：完全避免锁（如 per-thread 计数器）
- **资源划分**：减少锁竞争（如分区锁）
- **消息传递**：避免共享内存（如 MPI 模型）
- **事务内存**：自动化的乐观并发

### 3. 本章回答了一个用户的根本问题

用户在 `why-parallel-is-hard.md` 中写道："当从软件的角度来思考的时候...哪一个环节实现并行最复杂?"

本章的答案是：**并行访问控制（协调）和资源划分**是软件层面最复杂的部分。但更重要的是，本章指出：

> "并行编程 involves two-way communication, with a program's performance and scalability being the communication from the machine to the human."

这意味着并行编程的困难不仅是技术问题，更是**人机交互问题**——程序通过性能和可扩展性"反馈"给开发者，开发者据此调整策略。

### 4. 阅读路线图

根据用户笔记中的疑问，建议按以下顺序阅读 perfbook：

| 优先级 | 章节 | 解决疑问 |
|--------|------|---------|
| P0 | Chapter 3 (Hardware) | Cache coherence, memory barrier, store buffer |
| P0 | Chapter 4 (Tools) | RCU 基础、atomic、KCSAN、data race 检测 |
| P0 | Chapter 7 (Locking) | Spinlock/mutex 实现、锁的粒度、死锁 |
| P1 | Chapter 9 (RCU) | RCU 详细用法、read-mostly 优化 |
| P1 | Chapter 10 (Data Structures) | Hash table、lock-free 结构 |
| P1 | Chapter 12 (Formal Verification) | 形式化验证概念 |
| P2 | Chapter 14 (Deferred Processing) | Sequence locks、reference counting |
| P2 | Chapter 15 (Advanced Synchronization) | Memory ordering、non-blocking sync |

---

## 结论

Chapter 2 是一个**宏观概述**，它为并行编程建立了正确的认知框架：
- 并行编程没有想象中那么难
- 并行化只是众多性能优化手段之一
- 资源划分和复制比加锁更可扩展
- 性能、生产力、通用性不可兼得

但它**没有解决**用户笔记中列出的任何技术细节问题。这些问题的答案分散在后续章节中，尤其是 Chapter 3 (Hardware) 和 Chapter 7 (Locking)。

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
