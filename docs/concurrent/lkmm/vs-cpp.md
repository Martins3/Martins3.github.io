## 8. LKMM vs C/C++ Memory Model

| 特性 | LKMM | C11/C++11 |
|------|------|-----------|
| RCU 支持 | 完整支持 | 不支持 |
| 锁语义 | spinlock、rwlock 等 | mutex、atomic_flag 等 |
| 屏障 | `smp_mb()` 等 | `atomic_thread_fence()` |
| Acquire/Release | `smp_load_acquire()` / `smp_store_release()` | `memory_order_acquire` / `memory_order_release` |
| Benign Data Races | 允许 | 视为 UB |
| 多架构 | x86/ARM/PPC/RISC-V/Alpha | 抽象硬件无关 |
| 形式化工具 | herd7 + klitmus7 | 无官方工具 |
| 适用范围 | Linux 内核 | 通用用户程序 |

**共同点**

- 都有 happens-before 关系
- 都支持 acquire/release 语义
- 都有原子操作和 RMW

**关键差异**

- LKMM 的 `propagation` 公理在 C11 中没有直接对应
- LKMM 的 `rcu` 公理是内核特有的
- LKMM 对 plain access 的处理更宽松


## 1. 为什么内核需要 LKMM (我现在都不知道什么是 LKMM 了)

### 1.1 核心问题

Linux 内核定义了自己的内存模型 (LKMM)，而不是直接使用 C11/C++11 的内存模型。关键原因如下：

**RCU 不支持**

C11/C++11 内存模型完全不覆盖 RCU (Read-Copy-Update)。RCU 是内核最核心的同步机制之一，广泛应用于网络协议栈、文件系统、调度器等关键子系统。LKMM 必须能够形式化地描述和验证 RCU 的语义。

**内核特有的同步原语**

内核使用大量标准 C 语言不存在的同步原语：

- `spinlock_t` / `rwlock_t` — 内核自旋锁
- `smp_mb()` / `smp_rmb()` / `smp_wmb()` — 内核屏障
- `smp_load_acquire()` / `smp_store_release()` — 内核 acquire/release
- `rcu_read_lock()` / `synchronize_rcu()` — RCU 操作
- `srcu_read_lock()` / `synchronize_srcu()` — SRCU 操作

**Benign Data Races**

内核有意使用一些 "良性数据竞争" (benign data races)。例如，一个 CPU 用 `READ_ONCE()` 读取，另一个 CPU 用 plain store 写入。C11 会将此视为未定义行为 (UB)，但内核需要允许这种用法。

**多架构支持**

LKMM 需要同时覆盖 x86、ARM64、PowerPC、RISC-V、Alpha 等多种硬件架构的内存模型。它是这些硬件模型的 "最大公约数"，既要足够宽松以允许各架构的优化，又要足够严格以保证正确性。

**历史原因**

C 语言直到 C11 才有正式的内存模型，而内核长期基于 C99 开发。即使 C11 发布后，其模型也不满足内核需求。


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
