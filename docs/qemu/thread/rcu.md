# QEMU rcu
<!-- 09eae8f9-105a-4702-b839-a5c7fdfa39cf -->

- [QEMU RCU 文档](https://github.com/qemu/qemu/blob/master/docs/devel/rcu.txt)
	- https://qemu.readthedocs.io/en/v10.0.3/devel/rcu.html
- [terenceli 的 blog : QEMU RCU implementation](https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2021/03/14/qemu-rcu)

## 继续看看文档再说吧
- qatomic_rcu_set 什么作用

## 经典案例

### RAMList::dirty_memory

```c
typedef struct RAMList {
    // ...
    DirtyMemoryBlocks *dirty_memory[DIRTY_MEMORY_NUM];
    // ...
}
```
从 writer 的角度分析，做了两件事情
- 让 RAMList::dirty_memory 存储新的 DirtyMemoryBlocks 地址
- 释放老的 DirtyMemoryBlocks

```c
static void dirty_memory_extend(ram_addr_t old_ram_size,
                                ram_addr_t new_ram_size){
        new_blocks = g_malloc(sizeof(*new_blocks) +
                              sizeof(new_blocks->blocks[0]) * new_num_blocks);
        qatomic_rcu_set(&ram_list.dirty_memory[i], new_blocks);

        g_free_rcu(old_blocks, rcu);
}
```
- 如果一个 reader 从 RAMList::dirty_memory 中获取的就是新的 DirtyMemoryBlocks 地址，之后一切访问正常。
- 如果一个 reader 在更新 RAMList::dirty_memory 之前访问，获取的是旧的的 DirtyMemoryBlocks，现在是不能立刻将其释放掉的。需要等待 reader 都结束了才可以释放。
- 无论上面的哪一个情况，reader 通过 RAMList::dirty_memory 获取的 DirtyMemoryBlocks 总是 atomic 状态的，而不是一部分修改了，一部分没有修改，这是正确性的保证。
- g_free_rcu 是对于 call_rcu1 的简单包装，将需要调用释放函数推迟操作。

```c
void call_rcu1(struct rcu_head *node, void (*func)(struct rcu_head *node))
{
    node->func = func;
    enqueue(node);
    qatomic_inc(&rcu_call_count);
    qemu_event_set(&rcu_call_ready_event);
}
```
推迟的时间当然是等待所有的 reader 都结束才可以。

再看 reader 这一侧，使用 cpu_physical_memory_get_dirty 作为例子:

```c
static inline bool cpu_physical_memory_get_dirty(ram_addr_t start,
                                                 ram_addr_t length,
                                                 unsigned client)
{
    WITH_RCU_READ_LOCK_GUARD() {
      // 访问
    }
    return dirty;
}
```

这里还使用了 QLIST_NEXT_RCU ，也是经典位置了

### virtioqueue
```c
void virtqueue_push(VirtQueue *vq, const VirtQueueElement *elem,
                    unsigned int len)
{
    RCU_READ_LOCK_GUARD();
    virtqueue_fill(vq, elem, len, 0);
    virtqueue_flush(vq, 1);
}
```

应该配合使用的地方为:

virtio_init_region_cache
```c
    qatomic_rcu_set(&vq->vring.caches, new);
    if (old) {
        call_rcu(old, virtio_free_region_cache, rcu);
    }
```

## 问题

在 `call_rcu_thread` 中，需要持有 lock 才可以释放资源，这很奇怪。既然都是可以开始来执行 hook 函数了，
说明这些资源已经是没有人使用的，
那么为什么还需要使用 BQL 保护。
其原因在: https://lists.gnu.org/archive/html/qemu-devel/2015-02/msg03170.html

## 原理

WITH_RCU_READ_LOCK_GUARD 会展开为:

```txt
- rcu_read_auto_lock
  - rcu_read_lock
    - `rcu_reader->ctr = rcu_gp_ctr->ctr` : 在进入的时候同步 global 的 ctr 到本地，这样如果 global 的发生变动了，那么就可以检测出来

// 中间进行访问

- rcu_read_auto_unlock
  - rcu_read_unlock
    - 如果检测到 rcu_reader::waiting 的话，`qemu_event_set(&rcu_gp_event);`，通知 call_rcu thread 有 reader 结束了
```

先总结一下关联到几个主要结构体:

| 名称                 | 作用                                                                               |
|----------------------|------------------------------------------------------------------------------------|
| rcu_gp_ctr           | 全局变量，用于标记当前的 period                                                    |
| rcu_reader           | 每一个线程的局部变量，当 reader 进入 critical reagion 的时候，会和 rcu_gp_ctr 同步 |
| rcu_call_ready_event | 在 call_rcu1 中用于通知 `call_rcu` thread 有垃圾可以回收了                         |
| rcu_gp_event         | 在 rcu_read_unlock 中用于通知 `call_rcu` thread 有 reader 结束了                   |

reader 和 writer 都是和 call_rcu thread 来交互的:

- call_rcu_thread : 一个死循环，用户回收
  - 第一个 while 循环: 需要等待 writer 调用 call_rcu1 才可以, 然后等待一段时间
  - synchronize_rcu
    - 修改 rcu_gp_ctr, 表示进入回收的 period 了，如果一些 reader 正好在 critical region 中，那么因为 `rcu_reader->ctr` 和 `rcu_gp_ctr->ctr` 不相等而可以识别出来
    - wait_for_readers
      1. `static ThreadList registry = QLIST_HEAD_INITIALIZER(registry);` : 在 rcu_register_thread 的时候，将 thread local 的 rcu_reader 挂到上面去
      2. 对于 register 上挂载的 rcu_reader 调用 rcu_gp_ongoing 查询 local 的 `rcu_reader->ctr` 和 global 的 `rcu_gp_ctr->ctr` 是否存在差别，如果有，那么设置 rcu_reader_data::waiting 为 true, 如果版本相同，那么从 registry 中移除掉
      3. QLIST_EMPTY(&registry) : 这表示所有的 reader 都离开 critical region 了
  - try_dequeue && `node->func(node)` : 从队列中间取出需要执行的函数来, 这些执行函数就是进行垃圾回收



## 深入分析实现原理

源码:
- `include/qemu/rcu.h`
- `util/rcu.c` 源自 liburcu (urcu-mb 变体)，Paolo Bonzini 移植。

核心思想：**读者无锁、开销极小（只读写自己的 TLS 变量），写者负责等待所有读者离开临界区（宽限期）后才释放旧数据**。

### 经典例子
```txt
  假设当前代际为 101：

  读者 A                         写者                         读者 B
  ctr = 101
  读取 old
                                 发布 new
                                 gp: 101 -> 103
                                 发现 A.ctr=101，等待
                                                              ctr=103
                                                              只能取得新版本
  退出：ctr = 0
  唤醒写者
                                 释放 old
```

### 核心数据结构

```c
/* 全局宽限期计数器。bit0 恒为 1 (RCU_GP_LOCKED)，bit1 及以上是宽限期计数 (RCU_GP_CTR) */
unsigned long rcu_gp_ctr = RCU_GP_LOCKED;

struct rcu_reader_data {
    unsigned long ctr;      /* 进入临界区时记录的 rcu_gp_ctr 快照；0 = 不在临界区 */
    bool waiting;           /* 写者是否在请求我尽快退出临界区 */
    unsigned depth;         /* 读锁嵌套深度 */
    QLIST_ENTRY node;       /* 注册表链表节点 */
    NotifierList force_rcu; /* 强制退出通知器（协程专用） */
};
QEMU_DECLARE_CO_TLS(struct rcu_reader_data, rcu_reader)  /* 每个"协程"一份 */

static ThreadList registry;  /* 所有已注册读者的链表，由 rcu_registry_lock 保护 */
```

- `rcu_reader_data` 不是普通 `__thread`，而是 **CO_TLS (coroutine TLS)**
	- QEMU 的协程可以在不同线程间迁移，普通 TLS 会被编译器缓存导致读到旧线程的过期值。`QEMU_DEFINE_CO_TLS` 用 `noinline` + `asm volatile("" : "+rm"(ptr))` 阻止编译器缓存地址。
- 每个使用 RCU 的线程必须先 `rcu_register_thread()` 把本地 rcu_reader 挂到 registry，退出时 `rcu_unregister_thread()`。

### 读路径（无锁）

```c
static inline void rcu_read_lock(void) {
    if (p_rcu_reader->depth++ > 0) return;          /* 嵌套直接跳过 */
    ctr = qatomic_read(&rcu_gp_ctr);
    qatomic_set(&p_rcu_reader->ctr, ctr);           /* 记录进入时的宽限期 */
    smp_mb_placeholder();   /* 先写 ctr，再读受保护数据 */
}

static inline void rcu_read_unlock(void) {
    if (--p_rcu_reader->depth > 0) return;
    qatomic_store_release(&p_rcu_reader->ctr, 0);   /* 离开临界区 */
    smp_mb_placeholder();
    if (unlikely(qatomic_read(&p_rcu_reader->waiting))) {
        qatomic_set(&p_rcu_reader->waiting, false);
        qemu_event_set(&rcu_gp_event);              /* 唤醒等待的写者 */
    }
}
```

读者只做一次原子读 + 一次原子写（ctr 是私有数据，无争用），这是 RCU 高性能的根源。

### 写路径：synchronize_rcu()

```c
void synchronize_rcu(void) {
    QEMU_LOCK_GUARD(&rcu_sync_lock);                /* 写写互斥 */
    smp_mb_global();   /* 屏障 (1)：先发布新数据，再动宽限期计数 */
    QEMU_LOCK_GUARD(&rcu_registry_lock);
    if (!QLIST_EMPTY(&registry)) {
        if (sizeof(rcu_gp_ctr) < 8) {
            /* 32 位：奇偶翻转两阶段，避免计数器溢出/ABA */
            qatomic_set(&rcu_gp_ctr, rcu_gp_ctr ^ RCU_GP_CTR);
            wait_for_readers();
            qatomic_set(&rcu_gp_ctr, rcu_gp_ctr ^ RCU_GP_CTR);
        } else {
            qatomic_set(&rcu_gp_ctr, rcu_gp_ctr + RCU_GP_CTR);  /* 64 位直接 +2 */
        }
        wait_for_readers();
    }
}
```


### wait_for_readers()：等待 + 启发式强制退出

```c
static void wait_for_readers(void) {
    for (;;) {
        /* 启发式：回调积压>=30、轮询>=5次(50ms)、或在 drain_call_rcu 中 */
        if (!forced && (qatomic_read(&rcu_call_count) >= RCU_CALL_MIN_SIZE ||
                        sleeps >= 5 || qatomic_read(&in_drain_call_rcu))) {
            forced = true;
            QLIST_FOREACH(index, &registry, node) {
                notifier_list_notify(&index->force_rcu, NULL);
                qatomic_set(&index->waiting, true);   /* 请求读者尽快退出 */
            }
        }
        smp_mb_global();   /* 屏障 (2)：与 unlock 的 release 配对，保证 ctr 读写顺序一致 */
        /* 遍历 registry，把已静止的读者摘到 qsreaders 临时链表 */
	// 判断方法 rcu_gp_ongoing
        ...
        qemu_mutex_unlock(&rcu_registry_lock);   /* 等待期间放锁，允许注册/注销 */
        if (forced) {
            qemu_event_wait(&rcu_gp_event);      /* futex 睡，读者退出时会唤醒 */
        } else {
            g_usleep(10000);                     /* 否则 10ms 轮询 */
        }
        qemu_mutex_lock(&rcu_registry_lock);
    }
    QLIST_SWAP(&registry, &qsreaders, node);     /* 把临时链表还回 registry */
}
```

```c
static inline int rcu_gp_ongoing(unsigned long *ctr) {
    v = qatomic_read(ctr);
    return v && (v != rcu_gp_ctr);   /* 还在旧宽限期里 = 还没离开临界区 */
}
```

- 读者 `ctr == 0` → 不在临界区，已静止；
- 读者 `ctr == 当前 rcu_gp_ctr` → 它是在**新**宽限期开始后才进入的，不会读旧数据，可安全释放；
- 读者 `ctr` 是旧值（非零且 ≠ 当前计数）→ 它可能正在读旧数据，必须等它退出。

两个实用设计：
- **等待时释放 `rcu_registry_lock`**：新注册线程的 ctr 为 0，天然静止，下一轮会被摘走，不影响正确性；
- **force_rcu 通知器**：QEMU 的协程可能长时间停在临界区内（如等待 IO），纯等会死等。`call_rcu_thread`/`drain_call_rcu` 场景下触发 notifier（如协程事件循环的 kick）主动把读者"踢出"临界区，再靠 `waiting` + `rcu_gp_event` 精确等它退出。

### call_rcu：MPSC 延迟回收队列

```c
/* Multi-producer, single-consumer queue (liburcu wfqueue) */
static struct rcu_head dummy;
static struct rcu_head *head = &dummy, **tail = &dummy.next;

static void enqueue(struct rcu_head *node) {
    node->next = NULL;
    old_tail = qatomic_xchg(&tail, &node->next);  /* 拿旧 tail */
    qatomic_store_release(old_tail, node);        /* 再连上去 */
}

static struct rcu_head *try_dequeue(void) { ... /* 消费者唯一，head 无需原子 */ }
```

- 入队：`qatomic_xchg` 追加到链尾；消费者只有 `call_rcu_thread` 一个，所以 head 非原子。
- `call_rcu_thread`：等事件 → `synchronize_rcu()` 等一个宽限期 → **拿 BQL** 后逐个执行回调。
- `drain_call_rcu()`：投递哨兵回调（执行时置位 event），因回调按 FIFO 顺序执行，等它完成即保证此前所有回调都执行完；等待期间释放 BQL。

### 内存屏障的配对关系

| 位置 | 作用 |
|---|---|
| `synchronize_rcu` 开头 `smp_mb_global()`（屏障①） | 发布新指针**先于**递增宽限期 |
| `rcu_read_lock` 的 `smp_mb_placeholder()` | 写 ctr **先于**读受保护指针 |
| `rcu_read_unlock` 的 release + 屏障 | ctr=0 **先于**读 waiting |
| `wait_for_readers` 的 `smp_mb_global()`（屏障②） | 置 waiting **先于**读 ctr，保证 ctr 读写顺序一致 |

`smp_mb_global()` 在 Linux 上基于 **membarrier syscall**（不支持时退化为 `smp_mb()`），是进程级全局屏障。

### QEMU 特有细节

1. **32 位机器两阶段奇偶翻转**：64 位直接 +2 递增；32 位若也递增会溢出导致 ABA，所以先翻转奇偶等读者清空，再翻转回来再等一次。
2. **fork 支持**：`pthread_atfork` 钩子，子进程重建 registry 和 `call_rcu` 线程。
3. **与 BQL 的配合**：回调在 BQL 下执行（避免与其他 BQL 代码竞争）；`drain_call_rcu` 主动让出 BQL。
4. **自动锁守卫**：`WITH_RCU_READ_LOCK_GUARD()` / `RCU_READ_LOCK_GUARD()` 用 glib 的 `g_autoptr` 保证异常路径也解锁。

### 总结

```
读者:  rcu_read_lock()  → 记录 rcu_gp_ctr 快照到自己的 ctr
       ... 读受保护数据（无锁、无争用）...
       rcu_read_unlock() → ctr 置 0（必要时唤醒写者）

写者:  发布新数据
       synchronize_rcu(): 递增 rcu_gp_ctr → 等所有读者 ctr==0 或 ==新计数
       → 此时旧数据无人引用，可安全 free
```

QEMU RCU 最精巧的两点：
- 是**用单一全局计数器 + 每读者快照**就能判断宽限期，读者路径只有两次原子操作；
- 是针对协程调度做了适配（CO_TLS 防编译器缓存、force_rcu 踢长临界区）。


## 问题

### RCU 在用户态和内核态中实现的差异

### 为什么 kernel 的实现比 userspace 的复杂那么多

## 用户态 rcu
1. 同时 DPDK 中间也是有 RCU 的: https://doc.dpdk.org/guides/prog_guide/rcu_lib.html
2. https://liburcu.org/
3. 龟龟，cpp 2026 才支持 rcu

https://en.cppreference.com/w/cpp/header/rcu

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
