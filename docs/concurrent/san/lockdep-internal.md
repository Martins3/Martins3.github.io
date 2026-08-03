# lockdep 实现

```c
struct held_lock {
	/*
	 * One-way hash of the dependency chain up to this point. We
	 * hash the hashes step by step as the dependency chain grows.
	 *
	 * We use it for dependency-caching and we skip detection
	 * passes and dependency-updates if there is a cache-hit, so
	 * it is absolutely critical for 100% coverage of the validator
	 * to have a unique key value for every unique dependency path
	 * that can occur in the system, to make a unique hash value
	 * as likely as possible - hence the 64-bit width.
	 *
	 * The task struct holds the current hash value (initialized
	 * with zero), here we store the previous hash value:
	 */
	u64				prev_chain_key;
	unsigned long			acquire_ip;
	struct lockdep_map		*instance;
	struct lockdep_map		*nest_lock;
#ifdef CONFIG_LOCK_STAT
	u64 				waittime_stamp;
	u64				holdtime_stamp;
#endif
	/*
	 * class_idx is zero-indexed; it points to the element in
	 * lock_classes this held lock instance belongs to. class_idx is in
	 * the range from 0 to (MAX_LOCKDEP_KEYS-1) inclusive.
	 */
	unsigned int			class_idx:MAX_LOCKDEP_KEYS_BITS;
	/*
	 * The lock-stack is unified in that the lock chains of interrupt
	 * contexts nest ontop of process context chains, but we 'separate'
	 * the hashes by starting with 0 if we cross into an interrupt
	 * context, and we also keep do not add cross-context lock
	 * dependencies - the lock usage graph walking covers that area
	 * anyway, and we'd just unnecessarily increase the number of
	 * dependencies otherwise. [Note: hardirq and softirq contexts
	 * are separated from each other too.]
	 *
	 * The following field is used to detect when we cross into an
	 * interrupt context:
	 */
	unsigned int irq_context:2; /* bit 0 - soft, bit 1 - hard */
	unsigned int trylock:1;						/* 16 bits */

	unsigned int read:2;        /* see lock_acquire() comment */
	unsigned int check:1;       /* see lock_acquire() comment */
	unsigned int hardirqs_off:1;
	unsigned int references:12;					/* 32 bits */
	unsigned int pin_count;
};
```

核心: 根据每个任务实际执行过的锁嵌套关系，动态建立一张“锁类别有向图”；每次准备增加一条新边前，检查它是否会让图形成可导致阻塞的环。

它不是观察“哪个线程正在等待哪个线程”，而是在验证整个内核已经暴露出来的锁顺序规则。

## 三个核心对象

### 锁实例：`struct lockdep_map`

每个受 lockdep 跟踪的锁都会嵌入一个 `dep_map`。`lockdep_map` 保存锁的 key、名字和 class 缓存：

```c
struct lockdep_map {
	struct lock_class_key *key;
	struct lock_class *class_cache[];
	const char *name;
	/* ... */
};
```

### 锁类别：`struct lock_class`

lockdep 图中的节点不是具体的锁地址，而是 lock class：

```c
struct lock_class {
	struct list_head locks_after;
	struct list_head locks_before;

	const struct lockdep_subclass_key *key;
	unsigned long usage_mask;
	/* ... */
};
```

例如：

```c
struct inode {
	spinlock_t i_lock;
};
```

系统中可能有几十万个 inode 和几十万个 `i_lock` 实例，但这些锁通常属于同一个 class：

```text
inode->i_lock class
```

这样 lockdep 能从不同锁实例的执行历史中归纳出同一类锁的通用嵌套规则。这也是 lockdep 能“举一反三”的根本原因。

### 当前任务持锁栈：`struct held_lock`

每个任务都有：

```c
current->held_locks[];
current->lockdep_depth;
current->curr_chain_key;
```

栈中每一项是一个 `struct held_lock`，记录具体锁实例、所属 class、获取位置、读写模式、trylock、IRQ 上下文等信息。它表达的是：

```text
当前任务已经获取：A -> B -> C
```

## 一次加锁发生了什么

以：

```c
spin_lock(&lock);
```

为例，核心调用关系是：

```text
spin_lock()
  -> spin_acquire(&lock->dep_map, ...)
     -> lock_acquire()
        -> __lock_acquire()
```

对于普通 spinlock，lockdep 检查发生在真正尝试获取底层锁之前：

```c
spin_acquire(&lock->dep_map, ...);
LOCK_CONTENDED(lock, do_raw_spin_trylock, do_raw_spin_lock);
```

所以它有机会在 CPU 真正自旋卡住之前报警。

`__lock_acquire()` 的主要步骤如下。

### 查找或注册 lock class

```c
class = lock->class_cache[subclass];

if (!class)
	class = register_lock_class(lock, subclass, 0);
```

`register_lock_class()` 根据 `lock key + subclass` 查找或者建立全局 `lock_class`。

### 构造 `held_lock`

它先在当前任务持锁栈的尾部构造一个临时记录：

```c
hlock->class_idx    = class_idx;
hlock->instance     = lock;
hlock->irq_context  = task_irq_context(curr);
hlock->trylock      = trylock;
hlock->read         = read;
hlock->hardirqs_off = hardirqs_off;
```

这里不仅记录“是什么锁”，还记录：

- 独占锁、普通读锁还是递归读锁；
- 是否为 trylock；
- 当前是进程、softirq 还是 hardirq 上下文；
- 获取时 IRQ 是否关闭；
- 获取位置的指令地址。

### 记录 IRQ 使用属性

`mark_usage()` 会为 class 设置 usage bits，例如：

```text
曾在 hardirq 中获取
曾在 softirq 中获取
曾在 IRQ enabled 时获取
曾以 read 模式获取
```

例如，同一个锁曾经在进程上下文、IRQ enabled 时获取，后来又观察到它在 hardirq 中被获取，那么就存在：

```text
进程持有 A
    ↓ 被硬中断打断
硬中断再次获取 A
```

lockdep 因此报告 inconsistent lock state。

## 最关键的算法：加边前进行可达性搜索

假设当前任务已经持有 `A`，现在准备获取 `B`：

```c
spin_lock(&A);
spin_lock(&B);
```

lockdep 将这个事实表示成一条有向边：

```text
A -> B
```

它的含义是：曾经观察到持有 A 时获取 B。

边由 `struct lock_list` 表示，同时存入：

```text
A.locks_after  : B
B.locks_before : A
```

但是在增加 `A -> B` 前，lockdep 会先反向提问：

> 从 B 出发，通过已有依赖图，是否已经能走到 A？

也就是：

```text
path_exists(B, A) ?
```

如果存在，就意味着加入新边后会形成：

```text
A -> B -> ... -> A
```

核心代码是：

```c
/* Adding prev -> next: search from next and see whether prev is reachable. */
ret = check_noncircular(next, prev, trace);
```

搜索使用 BFS：

```text
候选新边：prev -> next
搜索方向：next -> ... -> prev
找到：形成环，报警
没找到：加入新边
```

使用 BFS 而不是 DFS，主要是为了找到最短的循环路径，方便输出容易理解的报告。

## ABBA 是怎么被发现的

假设路径一执行过：

```c
spin_lock(&A);
spin_lock(&B);
```

图中产生：

```text
A -> B
```

之后路径二执行：

```c
spin_lock(&B);
spin_lock(&A);
```

在第二个 `spin_lock(&A)` 之前，lockdep 准备增加：

```text
B -> A
```

它先从新锁 `A` 开始搜索，发现已有：

```text
A -> B
```

因此：

```text
已有：A -> B
待加：B -> A
结果：A -> B -> A
```

于是立即输出：

```text
WARNING: possible circular locking dependency detected
```

此时不要求两条路径同时执行、两个 CPU 真的互相等待或者系统真的已经卡住。它只需要分别观察到构成死锁的局部锁顺序。

多级环也一样：

```text
曾观察：A -> B
曾观察：B -> C
现在要加：C -> A
```

从 A 出发能够经过 B 到达 C，所以 lockdep 判断加入 `C -> A` 会形成环。

## 为什么不是简单地“有环就报警”

读写锁带来了更复杂的问题。lockdep 将锁获取模式区分为：

```text
E：exclusive writer
S：shared reader
R：recursive reader
N：non-recursive locker
```

某些读锁之间不会互相阻塞，所以依赖图里出现形式上的环，并不一定真的能够构造出等待环。因此每条依赖边还带有 `ER`、`EN`、`SR`、`SN` 等类型。

BFS 只沿着可能产生实际阻塞的 strong dependency path 搜索。所以现代 lockdep 的逻辑更准确地说是：

> 检测由冲突锁模式组成的强依赖环，而不是检测任意有向环。

## 为什么开销还能接受：chain cache

如果每次加锁都扫描持锁栈并进行图搜索，开销会非常大。lockdep 为当前锁序列计算一个增量 hash：

```text
初始值
  hash(A)
  hash(hash(A), B)
  hash(hash(hash(A), B), C)
```

核心函数是：

```c
iterate_chain_key(chain_key, hlock_id);
```

锁链中的 class 和读写模式都会参与 `chain_key` 的构造：

```text
A(write) -> B(read) -> C(write)
```

然后：

```text
chain cache 命中
    -> 这个完整锁链以前已经验证过
    -> 跳过昂贵的依赖更新和图搜索

chain cache 未命中
    -> 执行完整验证
    -> 验证通过后加入 cache
```

因此 lockdep 的性能模型大致是：

```text
第一次看到某种锁链：昂贵
之后重复同一锁链：hash 查询，便宜很多
```

## 解锁如何处理

解锁的核心调用路径是：

```text
lock_release()
  -> __lock_release()
```

lockdep 在 `current->held_locks[]` 中找到对应的锁实例，然后恢复：

```c
curr->lockdep_depth = i;
curr->curr_chain_key = hlock->prev_chain_key;
```

如果释放的是栈顶锁，直接完成。如果是非 LIFO 解锁，例如：

```text
持有栈：A -> B -> C
释放：B
剩余：A -> C
```

lockdep 会先截断到 A，再把 C 重新“获取”一次，从而重建正确的 held-lock 栈和 chain hash。

## 整体流程

```text
             当前任务 held_locks[]
                     │
                     │ 正在持有 A，现在获取 B
                     ▼
              产生候选依赖 A -> B
                     │
        ┌────────────┴────────────┐
        │                         │
        ▼                         ▼
  检查 class usage          从 B 开始 BFS
  IRQ/softirq/wait          能否到达 A？
        │                         │
        │                   ┌─────┴─────┐
        │                   │           │
        │                  能          不能
        │                   │           │
        │                   ▼           ▼
        │                报警      加入 A -> B
        │                               │
        └───────────────────────────────┘
                                        │
                                        ▼
                              更新 chain cache
                                        │
                                        ▼
                              压入 held_locks[]
```

最终可以把 lockdep 看成三个层次：

```text
局部事实：某任务持有 A 时获取了 B
                  ↓
全局模型：锁 class 依赖图中加入 A -> B
                  ↓
全局推理：新边是否闭合出可阻塞的强依赖环
```

它最巧妙的地方不是 BFS 本身，而是：

> 把不同 CPU、不同时刻、不同任务观察到的局部锁顺序，合并成一张全局 class 依赖图，从而在真正需要同时满足复杂时序之前，就推导出潜在死锁。

## 相关源码入口

- `include/linux/lockdep_types.h`：`lockdep_map`、`lock_class`、`held_lock`。
- `include/linux/lockdep.h`：不同锁原语到 `lock_acquire()`/`lock_release()` 的映射。
- `kernel/locking/lockdep.c:1285`：`register_lock_class()`。
- `kernel/locking/lockdep.c:2149`：`check_noncircular()`。
- `kernel/locking/lockdep.c:3122`：`check_prev_add()`。
- `kernel/locking/lockdep.c:3816`：chain cache 查询与添加。
- `kernel/locking/lockdep.c:3861`：`validate_chain()`。
- `kernel/locking/lockdep.c:5077`：`__lock_acquire()` 主流程。
- `kernel/locking/lockdep.c:5511`：`__lock_release()` 主流程。

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
