# simple.txt 中文译解

源文件：`tools/memory-model/Documentation/simple.txt`

## 文档定位

`simple.txt` 的目标不是把 LKMM 讲全，而是告诉你如何尽量避免直接掉进复杂的内存序推理。它提供的是“先把并发问题简化”的策略。

## 逐节译解

### Single-threaded code

如果代码真的是单线程执行的，就没有跨 CPU 重排问题。但内核中“看起来像单线程”的代码很容易后来被放到并发上下文里，所以不能靠想当然。最直接的办法是用全局锁把整段代码串行化，这叫 code locking。缺点是性能和可扩展性差，因此只适合低频路径。

### Packaged code

如果不能完全单线程化，就尽量复用内核已有 API，把并发复杂性委托给库维护者。例子包括链表 API、workqueue、`smp_call_function()`、哈希表和树结构等。核心思想是：优先使用成熟抽象，而不是自己发明同步协议。

### Data locking

与其给整段代码上一个大锁，不如把锁绑定到数据实例上。哈希桶锁就是经典例子。这样不同桶上的操作可以并行，随着 CPU 和数据规模增长，也更容易伸缩。

### Per-CPU processing

把数据和处理拆到每个 CPU 本地，可以同时获得单线程推理的简单性和良好的性能。代价是更高的内存占用，以及偶尔仍然需要同步一个全局视图。

### Packaged primitives: Sequence locking

顺序锁是少数“已经打包好”的无锁模式之一。文档给出的简化规则是：读侧不要写。如果读侧写入，就会进入更复杂的无锁语义，通常不再适合靠经验直接判断。

### Packaged primitives: RCU

RCU 的简化规则有三条：读侧不要写、不要修改读者可见且正在被读者访问的内容、更新侧用锁保护。也就是说，RCU 适合“发布新版本对象，再延迟回收旧版本”这类模式。

### Packaged primitives: Atomic operations

原子操作不是一个单一强度的集合。传统上有无返回值且无序的原子操作，也有返回值且全序的操作；现在又加入了 `_relaxed`、`_acquire`、`_release` 等细分变体。读代码时不能只看到“atomic”就默认它已经帮你排好了序。

### Lockless, fully ordered

如果你必须在不持锁的情况下访问共享变量，而又想保持推理简单，那么文档建议只使用“全序”的操作。代价通常是性能，但换来的是更稳定的心智模型。

### Lockless statistics and heuristics

对于统计信息和启发式参数，可以使用 `atomic_read()`、`atomic_set()`、`READ_ONCE()`、`WRITE_ONCE()` 这类无序原语。前提是：你真的能容忍“读到旧值、错值、非同步值”。

### Don’t let the compiler trip you up

Plain C 访问并不是天然安全的。现代编译器会融合、拆分、发明、删除读写。只有当你非常清楚 C 规则和编译器行为时，才应依赖普通读写做无锁共享访问。

### More complex use cases

如果以上简化路径都不够，就要进入 `ordering.txt`、`recipes.txt`、`locking.txt`、`explanation.txt` 这些更正式的材料。

## 这篇文档真正想传达的原则

先选一个更简单的并发结构，再讨论内存序。很多 LKMM 问题不是靠背 barrier 解决的，而是靠换一种设计避免问题。

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
