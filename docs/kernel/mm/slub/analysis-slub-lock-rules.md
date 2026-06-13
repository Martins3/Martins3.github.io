## 基本是正确的 slub 分析
<!-- b3427187-c71e-40d4-88c5-07f6f2731f4e -->

锁规则要解决的根本问题

```
┌─────────────────────────────────────────────────────────────────┐
│ 问题1: 如何在多 CPU 并发分配时避免锁竞争？                       │
│ ─────────────────────────────────────────                       │
│ 解决: 每个 CPU 有本地缓存 (kmem_cache_cpu)，无锁访问             │
├─────────────────────────────────────────────────────────────────┤
│ 问题2: 如何安全地将对象从一个 CPU 释放到另一个 CPU 分配的 slab？   │
│ ─────────────────────────────────────────────────────────────   │
│ 解决: Frozen 机制 + 远程释放队列                                 │
├─────────────────────────────────────────────────────────────────┤
│ 问题3: 如何在内存紧张时安全地回收 slab？                         │
│ ─────────────────────────────────────────                       │
│ 解决: 分层锁 + 严格的状态机                                      │
├─────────────────────────────────────────────────────────────────┤
│ 问题4: 如何避免死锁？                                            │
│ ─────────────────────────────────────────                       │
│ 解决: 严格的锁层次结构 (Lock Hierarchy)                          │
└─────────────────────────────────────────────────────────────────┘
```


```
┌─────────────────────────────────────────────────────────────────┐
│                  SLUB 锁分层体系                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Layer 1: slab_mutex (全局互斥锁)                               │
│  ────────────────────────────────                               │
│  • 保护全局 slab cache 列表                                      │
│  • 用于 kmem_cache_create/destroy                               │
│  • 内存热插拔回调同步                                            │
│  • 使用场景: 元数据修改（极少发生）                              │
│                                                                 │
│  Layer 2: node->list_lock (节点自旋锁)                          │
│  ───────────────────────────────────                            │
│  • 保护 NUMA 节点的 partial 列表                                 │
│  • 用于跨 CPU 分配/释放时的 slab 迁移                            │
│  • 使用场景: 本地 slab 耗尽，从节点获取 partial                  │
│                                                                 │
│  Layer 3: kmem_cache_cpu->lock (本地锁)                         │
│  ─────────────────────────────────────                          │
│  • 保护 per-CPU slab 状态                                        │
│  • 用于慢路径操作                                                │
│  • 在 PREEMPT_RT 上特殊处理                                      │
│                                                                 │
│  Layer 4: slab_lock (页面锁，可选)                              │
│  ─────────────────────────────────                              │
│  • 仅在无 cmpxchg_double 架构使用                                │
│  • 保护 freelist/inuse/frozen 等字段                             │
│  • 现代 x86_64 不使用此锁（使用原子操作替代）                    │
│                                                                 │
│  Layer 5: object_map_lock (调试专用)                            │
│  ───────────────────────────────────                            │
│  • 仅在 SLUB_DEBUG 时使用                                        │
│  • 用于对象跟踪和调试                                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Frozen Slab 机制（核心创新）

"Frozen"（冻结）是 SLUB 实现高性能的关键机制：

```
┌─────────────────────────────────────────────────────────────────┐
│                     Frozen Slab 概念                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  定义: 被某个 CPU 标记为 frozen 的 slab 成为该 CPU 私有          │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  Frozen Slab 特权                                       │   │
│  │  ─────────────────                                      │   │
│  │  1. 只有冻结它的 CPU 才能从中分配对象                    │   │
│  │  2. 从节点公共列表中移除（豁免列表管理）                 │   │
│  │  3. 其他 CPU 只能向其中释放对象，不能分配                 │   │
│  │  4. 冻结/解冻操作只能由拥有者 CPU 执行                   │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  优势: 实现无锁快路径（Lockless Fastpath）                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 Slab 的四种状态

根据 `PG_Workingset` 标志位和 `frozen` 状态的组合：

| Slab 类型 | Workingset | Frozen | 位置 | 说明 |
|-----------|------------|--------|------|------|
| **Node partial** | ✓ | ✗ | Node 公共列表 | 可供所有 CPU 分配 |
| **CPU partial** | ✗ | ✗ | CPU 私有链表 | 预留备用（无锁访问） |
| **CPU slab** | ✗ | ✓ | 当前分配源 | 性能最高，完全私有 |
| **Full slab** | ✗ | ✗ | 不挂链表 | 已满，通常不跟踪 |

```
状态转换图:

┌──────────────┐    freeze     ┌─────────────┐
│ Node partial │ ─────────────→│  CPU slab   │
│  (共享)      │               │  (私有)     │
└──────────────┘               └──────┬──────┘
       ▲                              │
       │ unfreeze                     │ deactivate
       │ (to partial)                 │
┌──────┴───────┐               ┌──────▼──────┐
│  CPU partial │ ←─────────────┤   Full      │
│  (备用)      │   free obj    │   (满)      │
└──────────────┘               └─────────────┘
```

---

## 三、源码实现：锁规则的具体应用

### 3.1 锁的获取顺序（必须严格遵守）

```c
// mm/slub.c 中的锁顺序注释
/*
 * Lock order:
 *   1. slab_mutex (Global Mutex)
 *   2. node->list_lock (Spinlock)
 *   3. kmem_cache->cpu_slab->lock (Local lock)
 *   4. slab_lock(slab) (Only on some arches)
 *   5. object_map_lock (Only for debugging)
 *
 * 严格遵守此顺序可防止死锁！
 */
```

### 3.2 快速路径（Fastpath）：完全无锁

```c
// mm/slub.c
static __always_inline void *slab_alloc_node(struct kmem_cache *s,
        gfp_t gfpflags, int node, unsigned long addr)
{
    struct kmem_cache_cpu *c;
    void *object;
    unsigned long tid;

redo:
    // 1. 读取当前 CPU 的 kmem_cache_cpu
    c = raw_cpu_ptr(s->cpu_slab);

    // 2. 读取 tid（用于检测抢占/迁移）
    tid = READ_ONCE(c->tid);

    // 3. 检查是否可以使用 fastpath
    //    - 没有开启调试选项
    //    - 本地 slab 可用
    //    - freelist 不为空

    // 4. 原子地获取对象（cmpxchg_double 操作）
    //    同时更新 freelist 和 tid
    object = c->freelist;
    if (unlikely(!object || !slab_try_update_freelist(s, c, tid)))
        goto slowpath;

    // 5. 成功获取对象，无锁完成分配！
    return object;

slowpath:
    // 进入慢路径，需要获取各种锁
    return __slab_alloc(s, gfpflags, node, addr, c);
}
```

### 3.3 慢路径（Slowpath）：锁的获取

```c
// mm/slub.c
static void *__slab_alloc(struct kmem_cache *s, gfp_t gfpflags, int node,
                          unsigned long addr, struct kmem_cache_cpu *c)
{
    void *p;
    unsigned long flags;

    // 1. 关闭中断，获取本地锁
    local_irq_save(flags);

#ifdef CONFIG_PREEMPT
    // 2. 检查是否被抢占并迁移到其他 CPU
    //    如果是，重新获取当前 CPU 的 kmem_cache_cpu
    c = this_cpu_ptr(s->cpu_slab);
#endif

    // 3. 调用核心慢路径逻辑
    p = ___slab_alloc(s, gfpflags, node, addr, c);

    // 4. 恢复中断
    local_irq_restore(flags);
    return p;
}
```

### 3.4 Frozen 机制的实现

#### 冻结 Slab

```c
// 将 slab 从 node partial 列表中取出并冻结
static inline void *freeze_slab(struct kmem_cache *s, struct slab *slab)
{
    void *freelist;

    // 设置 frozen 标志
    slab->frozen = 1;

    // 获取 freelist（原子操作）
    freelist = slab->freelist;

    // 清除 slab 在 node 列表中的状态
    // 此时其他 CPU 不能再从这个 slab 分配

    return freelist;
}
```

#### 解冻 Slab

```c
// 将 slab 从 frozen 状态释放回 node
static void unfreeze_partials(struct kmem_cache *s, struct kmem_cache_cpu *c)
{
    struct slab *slab;
    struct slab *next;
    struct kmem_cache_node *n = get_node(s, smp_processor_id());

    // 遍历 CPU partial 列表中的所有 slab
    for (slab = c->partial; slab; slab = next) {
        next = slab->next;

        // 获取 node 的 list_lock
        spin_lock(&n->list_lock);

        // 清除 frozen 标志
        slab->frozen = 0;

        // 根据使用情况决定去向：
        // - 还有空闲对象 → 加入 node partial 列表
        // - 完全空闲 → 释放回 buddy system
        // - 已满 → 加入 full 列表（调试模式）

        spin_unlock(&n->list_lock);
    }
}
```

### 3.5 跨 CPU 释放的处理

```c
// 处理对象释放时的跨 CPU 情况
static void __slab_free(struct kmem_cache *s, struct slab *slab,
                        void *head, void *tail, int cnt,
                        unsigned long addr)
{
    // 情况1: slab 是 frozen 状态（属于某个 CPU）
    if (slab->frozen) {
        // 简单地将对象加入 freelist（原子操作）
        // 只有拥有者 CPU 可以修改 slab 状态
        // 其他 CPU 只能添加对象到 freelist
    }

    // 情况2: slab 在 node partial 列表中
    else if (slab->pfmemalloc) {
        // 需要获取 node->list_lock
        spin_lock(&n->list_lock);

        // 将对象加入 slab->freelist
        // 检查是否需要移动 slab（如从 full 到 partial）

        spin_unlock(&n->list_lock);
    }

    // 情况3: 其他状态（错误情况）
    else {
        // 调用 slowpath 处理
    }
}
```

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
