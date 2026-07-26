# tsan
<!-- bd54ef05-b7a0-4b30-9374-b0b8f948b07e -->


Happens-Before 关系传递
线程间的 happens-before 通过同步操作建立：
- 锁的释放和获取：释放锁的线程将自己的 Epoch 写入锁的“release 存储”，获取锁的线程读取该 Epoch 并更新自己的时钟为 max(自身时钟, 锁中 Epoch + 1)。
- 线程创建/join：子线程继承创建者的 Epoch；父线程 join 子线程时合并子线程的 Epoch。
- 原子操作、信号、条件变量、屏障等也进行类似的时钟传递。
- 这样，运行时能判断任意两个内存访问之间是否存在 happens-before 路径。

shadow unit 记录变量的使用:
- 最近一次写的 Epoch（和可能的 lockset）；
- 最近多次（通常 ≤4）读的 Epoch。


基本检测算法:

*写操作 Write(addr)*

获取当前线程的 Epoch 和 lockset。
读取 addr 的影子单元：
	对其中记录的每一个读 Epoch，检查：
		若该读 happens-before 当前写 → 不冲突，跳过。
		否则，检查读/写的锁集交集是否为空。若为空 → 报告数据竞争。
将该影子单元更新为“写”状态，记录本次写的 Epoch 和当前锁集。
如果写操作本身有锁保护，可能把锁集编码进影子单元（或溢出块）。

*读操作 Read(addr)*

获取当前线程 Epoch 和锁集。
读取影子单元：
	若存在一个写记录，且该写 不 happens-before 当前读，且锁集无交集 → 报告竞争。
把本次读的 Epoch（以及锁集）添加到影子单元的读历史中（若空间不足则创建溢出块）。

整个过程在 hotpath 上仅通过位运算和少数比较完成，极快。有锁集或历史溢出时才走慢路径。

## 死锁检测
能检测：由互斥锁获取顺序不一致导致的潜在死锁（最经典的 ABBA 死锁）。
不能检测：
- 实际已发生的死锁——如果程序真的卡死了，TSan 自身也会被阻塞，无法主动打印报告（除非被杀掉时 dump 状态，但不是常规功能）。
- 由其他同步对象引起的死锁：如信号量、条件变量、读写锁（某些情况下）、自旋锁等引起的死锁，TSan 通常不跟踪。
- 不含互斥锁顺序反转的死锁：比如某个线程永久等待一个从未被 signal 的条件变量，这种不属于锁顺序问题。


会漏。即使满足：

* 所有代码都用 `-fsanitize=thread` 编译；
* 每个相关 load/store 都实际执行；
* 同步操作也被 TSan 识别；

LLVM TSan 仍然不是一个“保存完整执行历史的精确 HB 判定器”。它为了性能使用了有界、近似的数据结构。

不过需要先区分两个问题：

> **两个访问都执行了**，不等于这次执行中它们构成 data race。

如果这一次运行恰好存在 HB 链，TSan 不报告是正确的；程序可能只在另一种调度中发生竞争。

---

## 1. 最主要的真实漏报：shadow memory 只能保存少量旧访问

当前 LLVM TSan 把每个对齐的 8 字节应用内存映射为一个 shadow cell，而一个 shadow cell 只能保存 **4 条访问记录**：

```cpp
const uptr kShadowCnt = 4;
const uptr kShadowCell = 8;
```

当 4 个槽都满了，又找不到可以直接覆盖的记录时，TSan 会选择一个槽替换掉。当前实现实际上根据线程 trace 位置选一个槽覆盖。([GitHub][1])

所以它并不是：

```text
x 的所有历史访问：
W1, R2, R3, R4, R5, ...
```

而更像：

```text
x 最近保留下来的四个候选访问：
[R2, R3, R4, R5]
```

原始的 `W1` 可能已经被驱逐。

### 一个可能漏报的序列

设 `W0` 是一个写：

```text
T0: W(x)
```

之后多个线程读取 `x`，但这些读取都通过各自的同步操作与 `W0` 建立了 HB：

```text
T1: R(x)    W0 HB R1
T2: R(x)    W0 HB R2
T3: R(x)    W0 HB R3
T4: R(x)    W0 HB R4
T5: R(x)    W0 HB R5
```

这些读取都不是 race，但它们会不断更新 `x` 的 shadow cell。最终 `W0` 可能被驱逐。

然后另一个线程执行：

```text
Tbad: R(x)
```

它与 `W0` 没有 HB，因此真实存在：

```text
W0(x)  vs  Rbad(x)
```

但此时 shadow cell 中可能只剩下：

```text
R2, R3, R4, R5
```

当前访问也是 read，于是 TSan 只看到 read-read，不报告。

TSan 的算法说明明确写到：shadow 状态只保存有限个访问；没有槽位时会驱逐一条记录，因此可能漏掉 data race。([GitHub][2])

### 重要区别

`TSAN_OPTIONS=history_size=7` 不能解决这个问题。

`history_size` 主要控制每个线程用于恢复旧访问栈信息的 trace history；它不会把：

```cpp
kShadowCnt = 4
```

变成更大的值。因此它通常解决的是：

```text
failed to restore the stack
```

而不是“某地址的旧访问被 shadow cell 驱逐”。([Clang][3])

---

## 2. 两个访问恰好同时更新 shadow，TSan 自己可能错过比较

为了速度，普通内存访问的 fast path 不会给每个 shadow cell 加锁。

考虑两个线程第一次同时写一个干净地址：

```text
T1                         T2
load shadow: empty         load shadow: empty
没有发现旧访问              没有发现旧访问
store T1 record            store T2 record
```

两边在检查时都没有看到对方：

```text
T1 没有比较 T2
T2 没有比较 T1
```

于是这一次 `W/W` race 可能没有被报告。

TSan 使用原子 shadow load/store 来保证其内部状态不会损坏，但不会把整个“读取 shadow—检查—写回 shadow”过程串行化。官方算法说明因此明确提到，存在一个很小的漏报概率。([GitHub][2])

这也是为什么重复运行 TSan、扰动调度仍然有价值。

---

## 3. 原子同步模型可能建立过强的 HB

TSan 不可能完整保存每个原子对象的：

```text
modification order
read-from relation
每个 release sequence
每次 load 实际读到了哪次 store
```

它对每个原子对象维护一个聚合的同步 clock。release 操作向该 clock 发布，acquire 操作从该 clock 获取。

当前实现中，acquire load 大致执行：

```cpp
thr->clock.Acquire(s->clock);
```

release store 执行：

```cpp
thr->clock.ReleaseStore(&s->clock);
```

这是一种压缩建模，不是完整的 C++ 原子执行图。

### 一个非常具体的例子

```cpp
int data;
std::atomic<int> flag{0};

// T1
data = 42;
flag.store(1, std::memory_order_release);

// T2
while (flag.load(std::memory_order_relaxed) != 1) {
}
flag.store(2, std::memory_order_relaxed);

// T3
while (flag.load(std::memory_order_acquire) != 2) {
}
int value = data;
```

在 C++ 内存模型中：

```text
T3 的 acquire load 读到的是 T2 写入的 2
```

而 T2 的 store 是 relaxed。它不会把 T1 的 release 同步传递给 T3：

```text
T1 data=42
     X 没有 HB
T3 read data
```

所以 `data` 上存在 data race。

但是当前 TSan 的 relaxed-store fast path 不会重置原子对象关联的同步 clock。源码甚至有明确注释：

```cpp
// Strictly saying even relaxed store cuts off release sequence,
// so must reset the clock.
if (!IsReleaseOrder(mo)) {
    NoTsanAtomic(mo, a, v);
    return;
}
```

也就是说，`flag` 的同步 clock 中可能仍保存着 T1 的 release clock。T3 做 acquire load 时可能错误地 acquire 到 T1 的 clock，从而建立一个实际 C++ 语义中不存在的 HB，最终把 `data` 上的 race 隐藏掉。

这是典型的：

> 为降低原子操作的运行时成本，TSan 对原子同步进行过近似，从而可能产生 false negative。

---

## 4. Fiber、协程和用户态调度器的线程身份建模错误

假设所有普通内存访问都插桩了，但程序使用：

```text
fiber
ucontext
用户态线程
可迁移协程
自制 scheduler
```

普通 TSan 默认看到的是 OS thread。

如果两个逻辑 fiber 先后运行在同一个 OS thread 上，且没有调用 TSan fiber API，TSan 可能把它们看成同一个线程：

```text
Fiber A access x
Fiber B access x
```

被错误理解为：

```text
同一个 TSan thread 内的两个访问
```

同线程访问不会被判定为 data race，因此可能漏报。

LLVM 为此提供：

```cpp
__tsan_create_fiber()
__tsan_switch_to_fiber()
__tsan_destroy_fiber()
```

而且 `__tsan_switch_to_fiber` 默认还可能在 fiber 间建立同步；如果实际调度语义不应建立 HB，需要使用：

```cpp
__tsan_switch_to_fiber_no_sync
```

错误地使用默认同步也会产生过强 HB，从而隐藏 race。

---

## 5. Shadow 被主动清空

如果使用了：

```text
TSAN_OPTIONS=flush_memory_ms=...
TSAN_OPTIONS=memory_limit_mb=...
__tsan_flush_memory()
```

TSan 会清空 shadow memory，以降低内存占用。

假设：

```text
T1: write x
    ↓
TSan flushes shadow
    ↓
T2: read x
```

即使两个访问都被插桩并执行了，第二次访问也找不到第一次访问的记录。

TSan 的参数文档明确说明，已经被 flush 掉的访问无法再参与 race 检测；当前运行时的 `FlushShadowMemory()` 也直接走全局 reset。([GitHub][4])

当前实现使用有限数量的 thread slot 和有限宽度 epoch。极长时间运行、极高线程 churn 等情况下，slot/epoch 耗尽也会触发全局 shadow reset；跨越 reset 的旧访问自然无法再被检测。([GitHub][1])

---

## 6. 16 字节原子访问存在明确的表示精度损失

当前源码对 16 字节 atomic 只使用 8 字节大小的 memory-access 记录：

```cpp
// For 16-byte atomics we also use 8-byte memory access,
// this leads to false negatives only in very obscure cases.
```

例如，一个 16 字节原子对象的高 8 字节与另一个非原子访问重叠时，TSan 可能没有准确表示该重叠区域。

这是源码直接承认的少见 false-negative 情况。

---

## 7. “所有语句都执行了”仍然不等于“竞争调度执行了”

考虑：

```cpp
// T1
data = 42;
ready.store(true, std::memory_order_release);

// T2
if (ready.load(std::memory_order_acquire))
    use(data);
else
    use(data);
```

两个 `use(data)` 分支可能在不同运行中执行。在某次运行中，acquire 读到了 release 写入的值：

```text
data write HB data read
```

这次没有 race。

另一种运行中，load 读到旧值，走另一个分支：

```text
data write 没有 HB data read
```

才存在 race。

即使通过不同测试让“每一行代码都至少执行过一次”，也没有覆盖：

```text
每个 load 的 read-from 选择
每个锁竞争结果
每个条件变量唤醒关系
每种线程交错
```

TSan 是动态检测器，只分析**当前这一条执行轨迹**，不是静态证明器。Clang 新增的 adaptive delay 也是通过扰动同步点附近的调度，提高暴露不同交错的概率，而不是提供完整性保证。([Clang][3])

---

## 8. 一些“不报告”并不属于漏报

下面这些常被误认为 TSan 漏报，但它们本身不是 C++ data race：

### 全部是 atomic

```cpp
std::atomic<int> x;

x.store(1, std::memory_order_relaxed);
int v = x.load(std::memory_order_relaxed);
```

可能有算法或 memory-ordering bug，但两个 atomic 访问之间没有 C++ data race。TSan 不负责证明读取结果是否符合业务预期。

### 并发不变量被破坏

```cpp
atomic_balance.fetch_sub(100);
atomic_inventory.fetch_sub(1);
```

两个操作各自都是原子的，但整体事务可能不一致。这是 invariant violation，不是 data race。

### ABA、lost wakeup、错误的 CAS 循环

这些可能完全由原子操作构成，也不会产生 data race。

所以：

> TSan 无报告只能较有把握地说明“没有观察到它能够表示的 data race”，不能说明并发算法正确。

---

## 可以形成一个准确的结论

在理想条件下：

```text
完整插桩
+ 精确同步建模
+ 无限 shadow history
+ shadow 更新原子化
+ 不清空元数据
```

一个精确 HB 检测器应当发现当前执行中的所有 C/C++ data race。

但实际 LLVM TSan 使用：

```text
每 8 字节只有 4 条 shadow 记录
无锁的 shadow fast path
压缩的 atomic HB 模型
有限 thread slot / epoch
可被 flush 的元数据
有限访问宽度表示
```

因此即使两个冲突访问都执行并插桩，仍然可能漏报。

实践中最值得警惕的三个来源是：

```text
1. 旧访问被四槽 shadow history 驱逐；
2. 原子同步模型建立了过强的 HB；
3. fiber、自制同步或运行时上下文被错误建模。
```

[1]: https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/tsan/rtl/tsan_defs.h?utm_source=chatgpt.com "llvm-project/compiler-rt/lib/tsan/rtl/tsan_defs.h at main"
[2]: https://github.com/google/sanitizers/wiki/threadsanitizeralgorithm "ThreadSanitizerAlgorithm · google/sanitizers Wiki · GitHub"
[3]: https://clang.llvm.org/docs/ThreadSanitizer.html "ThreadSanitizer — Clang 24.0.0git documentation"
[4]: https://github.com/google/sanitizers/wiki/ThreadSanitizerFlags "ThreadSanitizerFlags · google/sanitizers Wiki · GitHub"


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
