## 核心结论

对于**位于单个 cache line 内、可缓存的普通内存**，现代 CPU 的原子 RMW 指令通常不是“锁住内存总线”，而是：

> 通过 cache coherence 协议，先取得该 cache line 的独占写权限，然后保证从读取旧值到写入新值期间，其他核心不能插入对该位置的修改。

这里的“原子”并不意味着整个操作必须在一个时钟周期完成，而是所有其他观察者只能看到：

```text
旧值
或
新值
```

永远看不到中间状态，也不能在 read-modify-write 的 read 和 write 之间插入另一个修改。

---

# 1. 普通 load/store 为什么可能天然原子

先看一个对齐的普通 64-bit store：

```c
*x = 42;
```

大致执行过程是：

```text
CPU pipeline
    │
    ▼
Store Buffer
    │
    ├─ 请求 x 所在 cache line 的写权限
    │
    ▼
L1 D-cache：该 line 进入 Modified/Exclusive 状态
    │
    ▼
写入 cache line
```

假设 `x` 所在 cache line 当前被 Core 0 和 Core 1 共享：

```text
Core 0 L1: line X, Shared
Core 1 L1: line X, Shared
```

Core 0 要写入时，会通过一致性协议发出类似：

```text
GetM / Read For Ownership / Upgrade
```

的请求：

```text
Core 0 ── GetM(X) ──► coherence fabric
                         │
                         ├── invalidate Core 1 的副本
                         │
                         └── 等待确认
```

完成后变成：

```text
Core 0 L1: line X, Modified
Core 1 L1: line X, Invalid
```

此时 Core 0 才能让写入对系统可见。

因此，对于架构明确保证原子的、满足大小和对齐要求的普通 load/store，其他核心观察到的是完整旧值或完整新值，而不是一半旧、一半新。

但它只有**读或写本身的原子性**，没有 read-modify-write 原子性。例如：

```c
*x = *x + 1;
```

普通指令可能展开为：

```asm
load  r0, [x]
add   r0, r0, 1
store [x], r0
```

两个核心可能同时读取旧值 10，最后都写入 11，丢失一次更新。

---

# 2. 原子 RMW 指令如何执行

以：

```c
atomic_fetch_add(x, 1);
```

为例，它在逻辑上包含：

```text
old = load(x)
new = old + 1
store(x, new)
return old
```

CPU 必须保证这三步相对于其他核心不可分割。

概念上大致经过以下步骤。

## 第一步：取得 cache line 独占权

如果该 line 不在本地 L1，先将它取入。

如果它处于 Shared 状态，则发送写权限请求：

```text
Core 0               Coherence fabric             Core 1
  │                         │                         │
  ├──── GetM(line X) ──────►│                         │
  │                         ├──── Invalidate X ─────►│
  │                         │◄──── Ack ───────────────┤
  │◄──── Grant Modified ────┤                         │
```

完成后，Core 0 是这个 cache line 唯一合法的写者。

## 第二步：在 cache/cache pipeline 中执行 RMW

CPU 对该 cache line 建立一个内部的 atomic transaction：

```text
读取旧值
    ↓
ALU 执行 add / xor / swap / compare
    ↓
写入新值
```

在此期间，针对同一 cache line 的一致性请求不能在读取和写回之间完成。

其他核心如果也请求这个 line，可能会：

* 被延迟；
* 被暂时拒绝并重试；
* 在内部队列中等待；
* 等本次原子操作完成后再获得 line。

具体实现依 CPU 而异。可能存在专门的 cache pipeline 状态、line reservation、atomic unit、snoop blocking 状态或 coherence transaction 状态。ISA 只规定外部可见行为，并不规定必须使用哪一种电路。

## 第三步：提交结果并解除内部保护

例如初始值为 10：

```text
load old = 10
calculate new = 11
store new = 11
```

操作完成后，其他核心才可以获得该 line。

因此另一个核心随后执行 `fetch_add` 时，读取到的必然是 11，而不是 10。

---

# 3. 两个核心同时 atomic_add 时会发生什么

假设：

```text
*x = 10
```

Core 0 和 Core 1 同时执行：

```c
atomic_fetch_add(x, 1);
```

一种可能顺序是：

```text
Core 0 获得 line X 的 Modified 权限
Core 1 的请求等待

Core 0:
    old = 10
    new = 11
    写入 11
    完成

line X 转移给 Core 1

Core 1:
    old = 11
    new = 12
    写入 12
    完成
```

最终值是：

```text
12
```

两个返回值分别为：

```text
Core 0 返回 10
Core 1 返回 11
```

也可能 Core 1 先执行，但所有核心最终必须同意某一个全序：

```text
Core 0 在前，Core 1 在后
```

或者：

```text
Core 1 在前，Core 0 在后
```

不能出现两者都读取 10 的情况。

所以原子 RMW 的一个本质是：

> 对同一个 coherence location 的所有修改必须被串行化。

---

# 4. x86 的 `LOCK` 指令

例如：

```asm
lock addq $1, [x]
lock xaddq %rax, [x]
lock cmpxchgq %rcx, [x]
```

以及 memory operand 形式的：

```asm
xchgq %rax, [x]
```

其中 `xchg` 对内存操作数隐含 locked 语义。

对于单个 cache line 内的、正常可缓存内存，现代 x86 通常使用 cache coherence 机制完成原子操作，而不是拉低全局 `LOCK#` 信号、锁住整个内存总线。Intel 文档明确说明，内部可缓存的内存通常不会使用外部 `LOCK#`，而是通过 cache/coherence 机制保证原子执行。([Intel][1])

可以把它抽象成：

```text
1. 获得 cache line 的独占所有权
2. 阻止其他一致性事务在 RMW 中间介入
3. 在本地 cache pipeline 中完成 load + ALU + store
4. 允许 line 再次参与一致性传输
```

这里常说“cache lock”，但不要理解成：

```text
锁住整个 L1 cache
```

更接近：

```text
锁定/保留这个 cache line 对应的原子事务
```

或者：

```text
在该原子事务结束前，不允许其他核心取得该 line 的可写所有权
```

当操作跨 cache line，或者访问不可缓存内存时，CPU 才可能需要更昂贵的 bus-lock 类机制。Intel 的性能事件文档也专门把 split-lock 和不可缓存内存访问列为可能产生 bus lock 的情况。([perfmon-events.intel.com][2])

你已经排除了跨 cache line，所以常规路径就是 coherence-based atomic。

---

# 5. ARM 的 LL/SC 实现

ARM 在没有 LSE atomic 指令时，经常使用：

```asm
retry:
    ldxr  x0, [addr]       // Load Exclusive
    add   x1, x0, #1
    stxr  w2, x1, [addr]   // Store Exclusive
    cbnz  w2, retry
```

其逻辑是：

```text
LDXR:
    读取 addr
    在本核心的 exclusive monitor 中记录这次访问

STXR:
    检查这段时间 exclusive reservation 是否仍然有效
    有效：执行 store，返回成功
    无效：不执行 store，返回失败
```

Arm 对 `LDXR` 的描述是：执行原子加载，并将该物理地址标记为 exclusive access，之后由 Store Exclusive 检查。([Arm开发者][3])

关键点是：

> `LDXR` 和 `STXR` 之间通常并没有一直锁住 cache line。

它不像下面这样：

```text
LDXR 时取得永久锁
执行任意代码
STXR 时释放锁
```

而是乐观并发：

```text
我先读
我记住“从读取到现在，这个位置似乎没有被别人破坏”
提交时再检查
```

如果另一个核心在中间修改了相关位置：

```text
Core 0: LDXR(x) -> 10
Core 1: 修改 x -> 20
Core 0: STXR(x, 11) -> 失败
```

Core 0 必须重试：

```text
Core 0: LDXR(x) -> 20
Core 0: STXR(x, 21) -> 成功
```

exclusive reservation 也允许因为中断、上下文切换、实现相关事件等原因失效，所以软件必须允许 `STXR` 偶发失败。

---

# 6. ARM LSE atomic 指令

Armv8.1 LSE 引入了直接的原子指令，例如：

```asm
cas
swp
ldadd
ldclr
ldeor
```

例如：

```asm
ldadd x0, x1, [x2]
```

表示对内存执行原子 add。Arm 文档将这些定义为原子 RMW 指令，并提供 acquire、release 和 acquire-release 变体。([Arm开发者][4])

编译器可以把：

```c
__atomic_fetch_add(p, 1, __ATOMIC_RELAXED);
```

直接编译成：

```asm
ldadd
```

而不是：

```asm
ldxr
add
stxr
失败则循环
```

它的微架构实现仍然依赖 cache coherence serialization，只是 ISA 直接表达了整个 RMW，硬件可以更有效地调度请求，特别是在多核高竞争场景中。Arm 官方也说明 LSE 的 CAS、LDADD 等可以替代 load-exclusive/store-exclusive 重试循环。([Arm开发者][5])

---

# 9. 原子性和内存序是两件事

这是最重要的区分之一。

## 原子性

保证对同一个对象：

```text
不会撕裂
不会看到中间结果
RMW 不会丢失更新
```

## 内存序

保证这个原子操作与其他地址访问之间的顺序。

例如：

```c
data = 123;
atomic_store_explicit(&ready, 1, memory_order_release);
```

另一个核心：

```c
if (atomic_load_explicit(&ready, memory_order_acquire))
    assert(data == 123);
```

这里 `ready` 的 store/load 原子性只保证 `ready` 自身不会被撕裂。

真正保证看到 `ready == 1` 后也能看到 `data == 123` 的，是 release/acquire 顺序。

在 ARM LSE 中，这经常直接编码到指令后缀中：

```asm
LDADD       // relaxed
LDADDA      // acquire
LDADDL      // release
LDADDAL     // acquire + release
```

Arm 官方文档将这些 ordering 选项对应到 acquire/release 语义。([Arm开发者][4])

---

# 10. 高竞争时为什么 atomic 很慢

无竞争时，如果 line 已经位于当前核心的 L1 且处于 Modified 状态：

```text
atomic RMW
≈ 本地 L1 原子操作 + pipeline/order 开销
```

虽然仍然比普通 `add [mem]` 贵，但不一定特别夸张。

真正昂贵的是 cache line ownership 在核心之间来回移动：

```text
Core 0: GetM → 修改
Core 1: GetM → Core 0 失效 → 修改
Core 0: GetM → Core 1 失效 → 修改
Core 1: GetM → ...
```

也就是 cache line ping-pong。

即使两个核心修改的是不同变量：

```c
struct {
    atomic_int a;
    atomic_int b;
};
```

只要 `a` 和 `b` 在同一个 cache line：

```text
Core 0 只修改 a
Core 1 只修改 b
```

仍然会争夺整个 cache line，因为 coherence 的粒度通常是 cache line，而不是单个 4-byte 变量。这就是 false sharing。

---

# 一句话模型

可以把单 cache line 内的原子 RMW 想象成：

```text
通过一致性协议取得 cache line 的独占权
        ↓
在 cache 子系统中开启一个不可插入的 RMW 事务
        ↓
读取旧值、计算新值、提交新值
        ↓
允许其他核心再次取得该 line
```

x86 `LOCK` 通常是**独占 cache line + coherence serialization**。

ARM LL/SC 通常是**读取时设置 reservation，写入时检查 reservation，失败则重试**。

ARM LSE 通常是**由硬件直接执行 coherence-aware 的单指令原子 RMW**。

它们电路实现不同，但最终都要完成同一件事：

> 在整个 coherence domain 中，为这个内存位置选出唯一的修改顺序。

[1]: https://www.intel.com/content/www/us/en/support/articles/000099741/processors/intel-xeon-processors.html?utm_source=chatgpt.com "Where is the Cache-Coherence Protocol Directory Placed ..."
[2]: https://perfmon-events.intel.com/platforms/graniterapids/core-events/core/?utm_source=chatgpt.com "Granite Rapids Server - Core Events - PerfMon Events - Intel"
[3]: https://developer.arm.com/documentation/ddi0602/2026-06/Base-Instructions/LDXR--Load-exclusive-register-?utm_source=chatgpt.com "LDXR - Arm A-profile A64 Instruction Set Architecture"
[4]: https://developer.arm.com/documentation/110478/0100/AArch64-Atomic-Instructions/The-different-groups-of-A64-atomic-instructions?utm_source=chatgpt.com "The different groups of A64 atomic instructions"
[5]: https://developer.arm.com/community/arm-community-blogs/b/tools-software-ides-blog/posts/making-the-most-of-the-arm-architecture-in-gcc-10?utm_source=chatgpt.com "Making the most of the Arm architecture with GCC 10"


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
