# ordering.txt 中文译解

源文件：`tools/memory-model/Documentation/ordering.txt`

## 文档定位

这份文档按“原语类别”而不是按“硬件架构”来讲 LKMM。它回答的问题是：Linux 内核到底提供了哪些排序工具，它们各自约束什么。

## 三大类排序手段

原文把 LKMM 原语分成三层：

- `Barriers`：显式栅栏，直接把前后访问隔开。
- `Ordered memory accesses`：带有排序语义的访问，比如 acquire/release。
- `Unordered accesses`：没有排序承诺的访问，但有时在特定条件下会和其他原语配合产生效果。

## Barriers

### Full memory barriers

`smp_mb()` 是最强的 SMP 栅栏，它让一个 CPU 上所有“之前的访问”在所有 CPU 看来都先于“之后的访问”。返回值型、且不是 `_relaxed/_acquire/_release` 的 RMW 操作，也常常提供全序。RCU 的 grace period 原语也提供强排序，但成本远高于一般 barrier。

### RMW ordering augmentation barriers

像 `atomic_inc()` 这类无返回值 RMW 在 LKMM 中默认不保证排序。`smp_mb__before_atomic()` 和 `smp_mb__after_atomic()` 是给这类原语补强排序用的，避免在某些架构上多付出一个完整 `smp_mb()` 的代价。

### Write memory barrier

`smp_wmb()` 只保证写前的访问先于写后的写访问。文档强调：很多场景下，你其实更应该用 `smp_store_release()`，因为语义更直接，开销也通常更低。

### Read memory barrier

`smp_rmb()` 只保证读前的访问先于读后的读访问。很多时候更推荐 `smp_load_acquire()`，因为 acquire 不只约束后续读，也约束后续写。

### Compiler barrier

`barrier()` 只约束编译器，不约束硬件。它的用途是禁止编译器把访问跨越某个点移动，但 CPU 仍可能在运行时乱序执行。

## Ordered memory accesses

### Release operations

`smp_store_release()`、`atomic_set_release()`、`rcu_assign_pointer()` 等，把“这个 store”排在本 CPU 之前的访问之后。它非常适合实现“先写数据，再发布标志/指针”。

### Acquire operations

`smp_load_acquire()`、`atomic_read_acquire()` 等，把“这个 load”排在本 CPU 后续访问之前。它适合实现“先读标志/指针，再读数据”。

### Release/acquire 配对

如果 acquire load 读到了 release store 写入的那个值，那么 release 之前的操作就能通过这条同步边传递给 acquire 之后的操作。这是消息传递模式最常见的写法。

### RCU read-side ordering

RCU 读侧标记和 `rcu_dereference()` 提供的是专门面向 RCU 的排序，不应和一般 acquire/release 完全混为一谈。它依赖地址依赖和 grace period 语义。

### Control dependencies

控制依赖只对“先读后写”的特定模式可靠，而且非常容易被编译器优化破坏。文档明确把它当作高级技巧，而不是默认方案。

## Unordered accesses

`READ_ONCE()`、`WRITE_ONCE()`、某些 relaxed 原子操作都属于无序访问。它们的主要价值是限制编译器做危险优化，并让访问在 LKMM 中变成“可见、可讨论的事件”，而不是自动获得跨 CPU 排序。

## 工程上的直接含义

- 想表达“发布/订阅”，优先用 acquire/release。
- 想表达“所有前后访问都不能穿越”，用 `smp_mb()`。
- 只想让编译器别乱来，不够，还得看 CPU 是否也可能乱序。
- 看到 `READ_ONCE()` 不要误以为已经同步成功。

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
