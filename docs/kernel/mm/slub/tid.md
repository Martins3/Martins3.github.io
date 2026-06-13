# Slub TID

> @todo 万万没有想到，和 preemption 有关的

```c
#ifdef CONFIG_PREEMPT
/*
 * Calculate the next globally unique transaction for disambiguiation
 * during cmpxchg. The transactions start with the cpu number and are then
 * incremented by CONFIG_NR_CPUS.
 */
#define TID_STEP  roundup_pow_of_two(CONFIG_NR_CPUS)
#else
/*
 * No preemption supported therefore also no need to check for
 * different cpus.
 */
#define TID_STEP 1
#endif

static inline unsigned long next_tid(unsigned long tid)
{
	return tid + TID_STEP;
}

static inline unsigned int tid_to_cpu(unsigned long tid)
{
	return tid % TID_STEP;
}

static inline unsigned long tid_to_event(unsigned long tid)
{
	return tid / TID_STEP;
}

static inline unsigned int init_tid(int cpu)
{
	return cpu;
}
```

## 是为了解决 aba 问题吗?


## 为什么只有 slab 才有 aba 问题?

## 如何理解 slub::system_has_freelist_aba 问题

```c
#define SLAB_TRACE		__SLAB_FLAG_BIT(_SLAB_TRACE)
```

## [ ] mm/slub.c 开头的注释解释了锁的实现

#### transaction id
从 《用芯探核:基于龙芯的 Linux 内核探索解析》
的描述，这是专门给 本地无锁 (kmem_cache_cpu::freelist) 使用的

看上去就是只是给 get_freelist 使用的

## 回答我

如需结合该代码回答关于 SLUB 内部机制
、对象分配/释放流程、锁策略、引用安全、RCU 释放、调试特性等问题，
请随时提出。


## slub 的 ABA 问题是什么
<!-- f5185bac-7baf-4732-8348-e5cef78df70e -->

理解 slub 的 ABA 问题之前，先需要知道，管理一个 object 是单向链表。

可以仅仅通过 cmpxchg_failure 来实现 freelist 的无锁的 add / delete 的。

1. add : 先让 object 指向 freelist head ，然后 cmpxchg ( cmp 是 object 还是指向 freelist head 的，如果是，就配置 freelist 指向 object)
2. delete : 找到 freelist 的下一个，然后 cmpxchg (cmp 是当前的 freelist 没有变化，如果是，freelist 更新为下一个)

(这个应该写一个 demo 测试一下)

假如你感觉这就是全部的工作后，你就掉到了 ABA 问题的陷阱之中。

解决办法也很容易，就是同时比较指针和 tid ，只有两个都相等，才可以修改。

SLUB 中的 **ABA 问题**（ABA Problem）主要出现在其**无锁（lockless）快速路径**（fastpath）的实现中，
尤其是在使用单指针加计数器（freelist + transaction ID）作为原子更新单元的场景下。这个问题的本质是：**在无锁并发操作中，仅靠比较并交换（CAS）无法区分“值未变”和“值变回原样”这两种情况**，从而可能导致使用已释放或无效对象。


### 一、SLUB 的 fastpath 无锁机制简述

SLUB 为了高性能，在分配/释放对象的 fastpath 中尽量避免锁：

- 每个 CPU 有本地缓存：`struct kmem_cache_cpu`，包含：
  - `freelist`：指向下一个可用对象
  - `tid`（transaction ID）：用于检测是否被其他上下文干扰
- 分配时通过 `this_cpu_cmpxchg_double()` 原子地更新 `freelist` 和 `tid`
- 释放时同样尝试无锁地将对象插回 `freelist`

这个双字 CAS（double-word CAS）操作的结构如下（简化）：

```c
struct {
  void *freelist;
  unsigned long tid;
} kmem_cache_cpu;
```

### 二、ABA 问题如何产生？

1. **CPU0** 准备从 `freelist` 分配对象 A：
   - 当前 `freelist = A`
   - `tid = 100`

2. **CPU0 被抢占**（或发生中断、调度），暂停执行。

3. **CPU1**：
   - 分配 A（`freelist` 变成 B）
   - 使用完后 **释放 A**，A 被插回 `freelist`
   - 此时 `freelist` 又变回 `A`
   - `tid` 被递增多次，比如变成 `102`

4. **CPU0 恢复执行**，执行 CAS：
   - 它仍认为 `freelist == A && tid == 100`
   - 但实际上 `tid` 已是 `102` → CAS **会失败**，ABA 被检测到！

**这说明 SLUB 通过引入 `tid` 已经** ***缓解了经典 ABA 问题***。


### 三、那为什么还会说“SLUB 有 ABA 问题”？

关键点在于：**ABA 风险依然存在，但仅限于某些特定平台或关闭了 `tid` 保护的情况**。

#### 1. **不支持 `cmpxchg_double` 的架构**

SLUB 源码中提到：

```c
/*
 * The slab_lock is only used on arches that do not have the ability
 * to do a cmpxchg_double.
 */
```

在这些平台上（如某些 ARM32、MIPS）：
- 无法原子地更新 `(freelist, tid)` 两个字段
- SLUB **回退到使用 `slab_lock()`（即 page lock）**
- 此时虽无 ABA，但性能下降

#### 2. **`tid` 回绕（wrap-around）**

`tid` 是 `unsigned long`，虽然空间大，但理论上仍可能回绕：

- 如果一个 CPU 在 `tid` 回绕周期内被长期抢占（极罕见）
- `tid` 从 `ULONG_MAX` 回到 `100`，恰好与当初的值相同
- 此时 CAS 会**错误地成功**，导致使用一个可能已被释放的对象

> 不过 `tid` 的步长是 `TID_STEP = roundup_pow_of_two(NR_CPUS)`，且每次分配/释放都递增，回绕周期极长（数百年量级），实践中几乎不可能发生。

#### 3. **错误的内存重用（UAF + ABA）**

如果内核存在 **Use-After-Free（UAF）** 漏洞：
- 一个对象被释放后，被另一个分配器重用（比如分配给另一个 `kmem_cache`）
- 但其地址仍与原 `freelist` 中某对象相同
- 此时即使 `tid` 正确，对象内容已完全改变

但这是**内存安全问题**，不属于 ABA 本身，而是 ABA 可能加剧的危害。

### 四、SLUB 如何应对 ABA？

1. **引入 `tid`（Transaction ID）**
   - 每次 CPU 修改 `freelist` 时递增 `tid`
   - CAS 同时校验 `(freelist, tid)`，防止“看似未变实则中间被篡改”

2. **检测到 CAS 失败时重试**
   - `note_cmpxchg_failure()` 打印调试信息
   - 重新读取 `freelist` 和 `tid`，进入 slowpath 或重试 fastpath

3. **架构不支持双字 CAS 时回退到加锁**
   - 使用 `slab_lock()` 保证安全，牺牲性能

4. **KASAN、SLAB_DEBUG 等调试机制辅助检测异常使用**


如需进一步结合代码（如 `__update_cpu_freelist_fast`、`note_cmpxchg_failure`）分析，也可继续深入。

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
