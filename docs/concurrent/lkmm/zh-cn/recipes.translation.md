# recipes.txt 中文译解

源文件：`tools/memory-model/Documentation/recipes.txt`

## 文档定位

这篇文档不是定义术语，而是给出常见并发模式的“做法模板”和“反例模板”。它适合在你已经知道 barrier 名字之后，用来建立“什么模式该配什么原语”的直觉。

## 逐节译解

### Single CPU or single memory location

只有一个 CPU，或者只操作一个对齐且完整访问的共享变量时，顺序问题简单很多。但文档仍提醒三点：

- C 语言里有些求值顺序本来就是未定义的。
- 编译器能按 as-if 规则重写普通访问。
- 多 CPU 上即便只有一个变量，也要考虑撕裂、融合和编译器发明访问。

### Locking

这里先给出锁的基本规则，并展示它比很多人直觉中的“临界区内可见”更强。随后又给出一个反例：锁建立的顺序不一定会自然扩展到没持锁的第三个 CPU，因此在这种传播链上可能需要 `smp_mb__after_spinlock()`。

### Message passing (MP)

MP 是最经典模式：生产者先写数据，再发布标志；消费者先读标志，再读数据。无序访问时，消费者可能看到“标志已发布但数据还是旧的”。最标准修法是 `smp_store_release()` 配 `smp_load_acquire()`。

### Assign and dereference

这是 MP 在线程安全指针发布上的 RCU 版本：更新侧用 `rcu_assign_pointer()`，读侧用 `rcu_dereference()`。适用于“发布一个新对象，再让读者跟着指针访问内容”。

### Write and read memory barriers

有些 MP 也可用 `smp_wmb()` 与 `smp_rmb()` 组合实现。但 acquire/release 往往更直观，因为它们把“同步点”绑在访问本身上。

### Load buffering (LB)

LB 模式常常揭示“两个 CPU 都先读后写”时的反直觉结果。它提醒你：某些你在 x86 上从未见过的结果，在更弱的架构上是允许的。

### Release-acquire chains

单条 release-acquire 边只是局部同步，链式传播能不能得到你想要的全局结论，需要更细致地分析。不要把“我有一条 acquire/release 链”直接等同于“所有 CPU 都同意一个总顺序”。

### Store buffering

SB 模式说明：每个 CPU 先写自己的变量，再读对方变量时，双方都读到旧值在很多模型下是允许的。它是理解“为什么必须有显式同步”的经典反例。

### Rules of thumb

文档最后给出实用经验：优先用现成配方；必要时写 litmus test；看到 lockless 模式先想同步边在哪里，而不是先想“某架构上应该没问题”。

## 这篇文档的价值

如果 `ordering.txt` 告诉你“工具箱里有什么”，那 `recipes.txt` 告诉你“这些工具一般怎么搭配使用”。

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
