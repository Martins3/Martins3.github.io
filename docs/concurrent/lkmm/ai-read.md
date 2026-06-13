# Linux Kernel Memory Model (LKMM) 完全指南
<!-- 7f3d96f1-f58e-4435-9cdb-7bc7dab33145 -->

(似乎是这么回事啊，这个总结的东西)

## 2. LKMM 核心概念

### 2.1 Events（事件）

LKMM 将程序执行抽象为事件的集合。事件类型定义在 `linux-kernel.bell` 中：

**Memory Accesses（内存访问）**

| 标记 | 含义 | 示例 |
|------|------|------|
| `R[once]` | 普通读 | `READ_ONCE(x)` |
| `W[once]` | 普通写 | `WRITE_ONCE(x, v)` |
| `R[acquire]` | acquire 读 | `smp_load_acquire(&x)` |
| `W[release]` | release 写 | `smp_store_release(&x, v)` |
| `R[mb]` / `W[mb]` | 全屏障访问 | `xchg()`, `cmpxchg()` |
| `R[noreturn]` | noreturn 读 | 不会返回的读取 |

**Barriers（屏障）**

| 屏障 | 含义 |
|------|------|
| `F[wmb]` | 写内存屏障 `smp_wmb()` |
| `F[rmb]` | 读内存屏障 `smp_rmb()` |
| `F[mb]` | 全内存屏障 `smp_mb()` |
| `F[release]` | release 屏障 `smp_release_fence()` |
| `F[acquire]` | acquire 屏障 `smp_acquire_fence()` |
| `F[rcu_lock]` / `F[rcu_unlock]` | RCU 读临界区起止 |
| `F[sync]` | RCU grace period `synchronize_rcu()` |

**Lock Operations（锁操作）**

| 事件 | 含义 |
|------|------|
| `LKR` | 锁获取 (Lock Read) |
| `LKW` | 锁获取 (Lock Write) |
| `UL` | 锁释放 (Unlock) |
| `LF` | 锁释放失败 |
| `RL` | 读锁获取 |
| `RU` | 读锁释放 |

### 2.2 Relations（关系）

LKMM 定义了多种事件之间的关系：

**Program Order (po)**

同一线程内，按程序代码顺序排列的事件关系。

**Reads-From (rf)**

写事件 W 和读事件 R 之间的关系，表示 R 读取了 W 写入的值。

**Coherence Order (co)**

同一内存位置的所有写事件之间的全序关系。

**From-Reads (fr)**

读事件 R 和写事件 W 之间的关系，表示 R 读取的值来自 W 之前的某个写。

**Happens-Before (hb)**

由以下关系组合而成的偏序：

- `po` (program order)
- `rf` (reads-from)
- `co` (coherence order)
- 各种屏障和同步原语引入的排序

**Propagation (pb)**

跨 CPU 的传播顺序，确保写操作对所有 CPU 可见。

### 2.3 Axioms（公理）

`linux-kernel.cat` 中定义的核心公理：

```cat
(* Coherence *)
acyclic po-loc | com as coherence

(* Atomicity *)
empty rmw & (fre ; coe) as atomic

(* Happens-Before *)
acyclic hb as happens-before

(* Propagation *)
acyclic pb as propagation

(* RCU *)
irreflexive rb as rcu
```

| 公理 | 含义 |
|------|------|
| `coherence` | 同一位置的访问必须满足一致性顺序（无循环） |
| `atomic` | RMW 操作必须是原子的（不允许中间有其他写） |
| `happens-before` | happens-before 关系必须是无环的 |
| `propagation` | 传播关系必须是无环的 |
| `rcu` | RCU 读临界区和 grace period 的关系约束 |


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
