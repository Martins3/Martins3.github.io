# Linux 内核 Slab 数据结构字段深度解析

<!-- 本文档详细解析 struct kmem_cache, struct slab 等核心数据结构的字段含义 -->

## 一、高屋建瓴：为什么需要理解这些字段？

### 1.1 设计哲学：元数据复用与内存效率

SLUB 分配器的一个核心设计思想是**复用现有数据结构**（struct page/slab）来存储元数据，而不是维护独立的描述符数组：

```
┌─────────────────────────────────────────────────────────────────┐
│                   SLUB 设计哲学                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  传统 SLAB 的问题：                                             │
│  ┌─────────────┐    ┌──────────────────────────────────────┐   │
│  │ 对象数组    │    │ 独立 slab 描述符数组 (额外内存开销)  │   │
│  │ [obj][obj]  │ ←→ │ [desc][desc][desc]...                │   │
│  └─────────────┘    └──────────────────────────────────────┘   │
│                                                                 │
│  SLUB 的解决方案：                                              │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  struct slab (复用 struct page)                          │   │
│  │  ┌──────────┬──────────────────────────────────────┐   │   │
│  │  │ 页框元数据│  slab 专用字段 (freelist, inuse等)   │   │   │
│  │  └──────────┴──────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  优势：                                                         │
│  • 减少内存碎片                                                │
│  • 简化数据结构关系                                            │
│  • 更好的缓存局部性                                            │
│  • 兼容 NUMA 架构                                              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 字段分类与关注点

理解 slab 字段需要关注三个层面：

| 层面 | 数据结构 | 关注重点 |
|------|---------|---------|
| **Cache 级别** | `struct kmem_cache` | 全局参数、策略配置、跨 CPU/Node 组织 |
| **Slab 级别** | `struct slab` | 页面状态、对象管理、归属关系 |
| **CPU 级别** | `struct kmem_cache_cpu` | 本地缓存、快速分配、无锁操作 |

---

## 二、核心概念：数据结构详解

### 2.1 struct kmem_cache —— Cache 描述符

```c
/*
 * Slab cache management.
 * 管理一个特定大小对象的缓存池
 */
struct kmem_cache {
    /* ========== Per-CPU 快速分配路径 ========== */
#ifndef CONFIG_SLUB_TINY
    struct kmem_cache_cpu __percpu *cpu_slab;  // [关键] 每个 CPU 的本地缓存
#endif

    /* ========== 基本配置参数 ========== */
    slab_flags_t flags;           // SLAB 标志位 (DEBUG, ACCOUNT等)
    unsigned long min_partial;    // Node 上保留的最小 partial slab 数
    unsigned int size;            // 对象实际大小（包含元数据/对齐）
    unsigned int object_size;     // 对象原始大小（用户请求大小）
    unsigned int offset;          // Free pointer 在对象内的偏移

#ifdef CONFIG_SLUB_CPU_PARTIAL
    /* ========== CPU Partial 配置 ========== */
    unsigned int cpu_partial;       // CPU partial 列表最大对象数
    unsigned int cpu_partial_slabs; // CPU partial 列表最大 slab 数
#endif

    /* ========== 内存分配参数 ========== */
    struct kmem_cache_order_objects oo;  // 高16位:order  低16位:objects
    struct kmem_cache_order_objects min; // 最小分配配置
    gfp_t allocflags;                    // 分配页面时使用的 GFP 标志

    /* ========== 生命周期管理 ========== */
    int refcount;                 // 引用计数（用于 cache 销毁）
    void (*ctor)(void *object);   // 对象构造函数
    unsigned int inuse;           // 对象内部使用的偏移（redzone等）
    unsigned int align;           // 对齐要求
    unsigned int red_left_pad;    // 左侧 redzone 大小

    /* ========== 链表与标识 ========== */
    const char *name;             // Cache 名称（/proc/slabinfo 显示）
    struct list_head list;        // 全局 slab cache 链表

#ifdef CONFIG_SYSFS
    struct kobject kobj;          // Sysfs 接口对象
#endif

    /* ========== NUMA 相关 ========== */
    unsigned int remote_node_defrag_ratio;  // 跨节点分配时的碎片整理比例
    struct kmem_cache_node *node[MAX_NUMNODES];  // 每个 NUMA 节点的管理结构
};
```

#### 关键字段详解

##### 1. cpu_slab —— 性能核心

```
┌─────────────────────────────────────────────────────────────────┐
│ cpu_slab: Per-CPU 本地缓存                                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  作用：                                                         │
│  • 每个 CPU 独立的快速分配路径                                  │
│  • 避免多 CPU 间的缓存行竞争                                    │
│  • 实现无锁快速分配（fastpath）                                 │
│                                                                 │
│  内存布局：                                                     │
│  CPU0 → kmem_cache_cpu[0]                                       │
│  CPU1 → kmem_cache_cpu[1]                                       │
│  CPU2 → kmem_cache_cpu[2]                                       │
│  ...                                                            │
│                                                                 │
│  关键：__percpu 属性确保每个 CPU 访问自己的副本                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

##### 2. size vs object_size —— 空间分配细节

```c
// size: 实际分配给每个对象的空间（包含各种开销）
// object_size: 用户请求的对象大小

// 示例: kmalloc-64
object_size = 64                    // 用户请求大小
size = 64                           // 实际分配大小（无调试选项时相同）

// 示例: 启用 SLUB_DEBUG 后
object_size = 64
size = 128                          // 增加了 redzone, poison 等
inuse = 8                           // 对象前 8 字节用于 SLUB 元数据

对象内存布局:
┌─────────────────────────────────────────────────────────────┐
│ inuse (元数据) │ red_left_pad │ 用户对象 │ redzone │ 对齐填充 │
└─────────────────────────────────────────────────────────────┘
```

##### 3. oo (order_objects) —— 紧凑的参数存储

```c
// kmem_cache_order_objects 是一个巧妙的位域设计：
// 高 16 位: 分配页面的 order (0-15, 即 1-32768 页)
// 低 16 位: 每个 slab 可容纳的对象数

struct kmem_cache_order_objects {
    unsigned int x;  // order << 16 | objects
};

// 示例: kmalloc-256
// order = 0  (1 页)
// objects = 16  (4KB / 256B = 16)
// oo.x = (0 << 16) | 16 = 16

// 示例: ext4_inode_cache
// order = 3  (8 页)
// objects = 29
// oo.x = (3 << 16) | 29 = 0x3001D
```

### 2.2 struct slab —— 页面描述符

```c
/*
 * struct slab - 描述被 slab 分配器管理的内存页面
 * 设计精妙：与 struct page 完全 overlay，不破坏其他用途的字段
 */
struct slab {
    /* ========== 继承自 struct page 的字段 ========== */
    unsigned long __page_flags;     // 页面标志位 (PG_slab, PG_frozen等)

    /* ========== 所属关系 ========== */
    struct kmem_cache *slab_cache;  // [关键] 指向所属的 kmem_cache

    union {
        struct {
            /* ===== 列表管理（Node/CPU partial） ===== */
            union {
                struct list_head slab_list;  // 在 node partial 列表中时使用

#ifdef CONFIG_SLUB_CPU_PARTIAL
                struct {                     // 在 cpu partial 列表中时使用
                    struct slab *next;       // 单链表指针
                    int slabs;               // 该链表中 slab 数量
                };
#endif
            };

            /* ===== 对象管理核心字段 ===== */
            union {
                struct {
                    void *freelist;          // [关键] 第一个空闲对象指针

                    union {
                        unsigned long counters;  // 组合计数器（原子操作）

                        struct {
                            unsigned inuse:16;     // 已使用对象数
                            unsigned objects:15;   // 总对象数
                            unsigned frozen:1;     // [关键] 冻结标志
                        };
                    };
                };

#ifdef system_has_freelist_aba
                freelist_aba_t freelist_counter;  // ABA 防护版本
#endif
            };
        };

        /* ===== RCU 延迟释放 ===== */
        struct rcu_head rcu_head;    // SLAB_TYPESAFE_BY_RCU 时使用
    };

    /* ========== 其他元数据 ========== */
    unsigned int __page_type;        // 页面类型
    atomic_t __page_refcount;        // 页面引用计数

#ifdef CONFIG_SLAB_OBJ_EXT
    unsigned long obj_exts;          // 对象扩展信息（如 KASAN）
#endif
};
```

#### 关键字段详解

##### 1. freelist —— 空闲对象链表

```
┌─────────────────────────────────────────────────────────────────┐
│ freelist: 空闲对象链表头                                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  组织方式：单向链表，使用对象内部的空间存储 next 指针           │
│                                                                 │
│  内存布局：                                                     │
│  ┌─────────┐    ┌─────────┐    ┌─────────┐                     │
│  │ 空闲对象0 │ → │ 空闲对象1 │ → │ 空闲对象2 │ → NULL            │
│  │ next指针 │    │ next指针 │    │ next指针 │                    │
│  └─────────┘    └─────────┘    └─────────┘                     │
│       ↑                                                         │
│   freelist (slab->freelist)                                     │
│                                                                 │
│  分配对象：返回 freelist 指向的对象，freelist = obj->next       │
│  释放对象：obj->next = freelist, freelist = obj               │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

##### 2. counters / inuse / objects / frozen —— 状态压缩

```c
// counters 是一个 32 位组合字段：
// ┌─────────────────┬─────────────────┬─────────┐
// │     inuse       │    objects      │ frozen  │
// │    (16 bits)    │    (15 bits)    │ (1 bit) │
// └─────────────────┴─────────────────┴─────────┘

// 使用场景：
// 1. 判断 slab 状态：
//    - inuse == 0: 完全空闲，可回收
//    - inuse == objects: 已满
//    - 0 < inuse < objects: partial

// 2. frozen 标志：
//    - 1: 被某个 CPU 冻结，该 CPU 独占分配权
//    - 0: 在 node 列表中，多 CPU 可竞争

// 原子操作示例：
// 分配对象时，原子地：
//   - 从 freelist 取出对象
//   - inuse++
// 使用 cmpxchg_double 保证原子性
```

##### 3. slab_cache —— 反向引用

```
┌─────────────────────────────────────────────────────────────────┐
│ slab_cache: 从 slab 找到其所属的 kmem_cache                      │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  用途：                                                         │
│  1. 释放对象时确定对象大小、对齐方式等                          │
│  2. 回收 slab 时找到正确的 kmem_cache_node                      │
│  3. 调试和统计信息收集                                          │
│                                                                 │
│  代码示例 (kfree)：                                             │
│  void kfree(const void *x) {                                    │
│      struct page *page = virt_to_head_page(x);                  │
│      struct kmem_cache *s = page->slab_cache;                   │
│      // 使用 s 来确定如何释放对象                               │
│  }                                                              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.3 struct kmem_cache_cpu —— CPU 本地缓存

```c
/*
 * 每个 CPU 的本地 slab 缓存
 * 设计目标：无锁快速分配
 * 注意：字段布局与 cmpxchg_double 对齐要求兼容
 */
struct kmem_cache_cpu {
    union {
        struct {
            void **freelist;           // [关键] 本地空闲对象链表
            unsigned long tid;         // [关键] Transaction ID（ABA防护）
        };
        freelist_aba_t freelist_tid;   // 组合用于双字 CAS
    };

    struct slab *slab;                 // [关键] 当前正在使用的 slab

#ifdef CONFIG_SLUB_CPU_PARTIAL
    struct slab *partial;              // [关键] CPU partial 列表头
#endif

    local_lock_t lock;                 // 保护慢路径操作
};
```

#### 关键字段详解

##### 1. freelist 与 tid —— 无锁分配的核心

```
┌─────────────────────────────────────────────────────────────────┐
│ freelist + tid: 实现无锁快速分配                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  为什么需要 tid？                                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │ ABA 问题场景：                                            │   │
│  │                                                           │   │
│  │ CPU0: 准备分配对象 A                                       │   │
│  │       freelist = A, tid = 100                             │   │
│  │       [被抢占]                                             │   │
│  │                                                           │   │
│  │ CPU1: 分配 A → 释放 A                                      │   │
│  │       freelist = A (没变!), tid = 102                     │   │
│  │                                                           │   │
│  │ CPU0: [恢复执行]                                           │   │
│  │       CAS(freelist=A, tid=100 → new_freelist, tid=101)    │   │
│  │       如果没有 tid 检查，会认为 freelist 没变，成功分配     │   │
│  │       但此时 A 可能已被释放并重新初始化！                   │   │
│  │                                                           │   │
│  │ 解决方案：同时检查 freelist 和 tid                         │   │
│  │       CAS(freelist, tid → new_freelist, new_tid)          │   │
│  │       tid 变了 → CAS 失败 → 重试或进入慢路径               │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

##### 2. slab —— 当前活动 slab

```
┌─────────────────────────────────────────────────────────────────┐
│ slab: 当前 CPU 主要分配的 slab                                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  特点：                                                         │
│  • 总是被 frozen（由当前 CPU 冻结）                             │
│  • 不在任何 node 列表中                                         │
│  • freelist 指向该 slab 中的空闲对象                            │
│                                                                 │
│  分配流程：                                                     │
│  1. 尝试从 cpu_slab->freelist 分配（无锁）                      │
│  2. 如果 freelist 为空，从 cpu_slab->slab->freelist 迁移        │
│  3. 如果 slab 为空，从 cpu_partial 获取                         │
│  4. 如果 cpu_partial 为空，从 node partial 获取                 │
│  5. 如果都为空，从 buddy system 分配新 slab                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

##### 3. partial —— 备用 slab 列表

```
┌─────────────────────────────────────────────────────────────────┐
│ partial: CPU 私有的 partial slab 列表                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  作用：                                                         │
│  • 当当前 slab 用完时，快速切换到备用 slab                      │
│  • 避免频繁访问 node 列表（需要 list_lock）                     │
│                                                                 │
│  组织方式：单链表，通过 slab->next 链接                         │
│                                                                 │
│  限制：                                                         │
│  • cpu_partial 控制列表中对象总数上限                           │
│  • cpu_partial_slabs 控制列表中 slab 数量上限                   │
│  • 超出限制时，部分 slab 会被 flush 回 node                     │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 2.4 struct kmem_cache_node —— NUMA 节点管理

```c
/*
 * NUMA 节点级别的 slab 管理
 * 保护跨 CPU 共享的 partial slab 列表
 */
struct kmem_cache_node {
    spinlock_t list_lock;              // [关键] 保护以下列表

    unsigned long nr_partial;          // partial 列表中的 slab 数量
    struct list_head partial;          // partial slab 链表

#ifdef CONFIG_SLUB_DEBUG
    atomic_long_t nr_slabs;            // 总 slab 数（调试用）
    atomic_long_t total_objects;       // 总对象数（调试用）
    struct list_head full;             // full slab 列表（调试用）
#endif
};
```

---

## 三、字段间的关系与交互

### 3.1 分配流程中的字段变化

```
┌─────────────────────────────────────────────────────────────────┐
│                    对象分配流程                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  场景1: 从 CPU freelist 分配（最快路径）                        │
│  ─────────────────────────────────────                          │
│  kmem_cache_cpu.freelist ──→ 分配对象                          │
│         ↓                                                       │
│  kmem_cache_cpu.freelist = obj->next                           │
│  kmem_cache_cpu.tid++                                          │
│                                                                 │
│  ─────────────────────────────────────────────────────────────  │
│                                                                 │
│  场景2: CPU freelist 为空，从 slab 迁移                         │
│  ─────────────────────────────────────                          │
│  slab.freelist ──→ kmem_cache_cpu.freelist                     │
│         ↓                                                       │
│  slab.inuse 增加                                                │
│                                                                 │
│  ─────────────────────────────────────────────────────────────  │
│                                                                 │
│  场景3: slab 用完，从 node partial 获取                         │
│  ─────────────────────────────────────                          │
│  获取 node.list_lock                                            │
│         ↓                                                       │
│  list_del(partial_slab)  // 从 node 列表移除                   │
│  node.nr_partial--                                             │
│  释放 node.list_lock                                            │
│         ↓                                                       │
│  slab.frozen = 1                                               │
│  kmem_cache_cpu.slab = slab                                    │
│  kmem_cache_cpu.freelist = slab.freelist                       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 释放流程中的字段变化

```
┌─────────────────────────────────────────────────────────────────┐
│                    对象释放流程                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  场景1: 释放到 frozen slab（本地释放）                          │
│  ─────────────────────────────────────                          │
│  obj->next = slab.freelist                                     │
│  slab.freelist = obj                                           │
│  slab.inuse--                                                  │
│  (原子操作：cmpxchg_double)                                     │
│                                                                 │
│  ─────────────────────────────────────────────────────────────  │
│                                                                 │
│  场景2: 释放到 node partial slab（远程释放）                    │
│  ─────────────────────────────────────                          │
│  获取 node.list_lock                                            │
│         ↓                                                       │
│  obj->next = slab.freelist                                     │
│  slab.freelist = obj                                           │
│  slab.inuse--                                                  │
│         ↓                                                       │
│  如果 slab 从 full 变为 partial：                              │
│      list_add(slab, &node.partial)                             │
│      node.nr_partial++                                         │
│  释放 node.list_lock                                            │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## 四、实际场景：字段调试与分析

### 4.1 查看 slab 字段值

```bash
# 1. 通过 /proc/slabinfo 查看基本统计
cat /proc/slabinfo | head -20
# 输出格式：
# name            <active_objs> <num_objs> <objsize> <objperslab> <pagesperslab>
# kmalloc-256      2048   2048    256   16    1

# 2. 通过 sysfs 查看详细字段
ls /sys/kernel/slab/kmalloc-256/
cat /sys/kernel/slab/kmalloc-256/objs_per_slab    # objects 字段
cat /sys/kernel/slab/kmalloc-256/order            # oo 的高16位

# 3. 使用 crash 工具分析运行中的内核
crash /usr/lib/debug/lib/modules/$(uname -r)/vmlinux

# 查看 kmem_cache 结构
crash> kmem_cache -S kmalloc-256
# 查看特定 slab
crash> slab -s kmalloc-256

# 4. 使用 bpftrace 跟踪字段变化
bpftrace -e 'kprobe:slab_alloc_node {
    printf("cache: %s, cpu_slab freelist: %p\n",
           ((struct kmem_cache *)arg0)->name,
           ((struct kmem_cache *)arg0)->cpu_slab->freelist);
}'
```

### 4.2 常见问题分析

#### 问题1: objects 和 inuse 不一致

```
现象: slabinfo 显示 active_objs 远小于 num_objs，但内存压力高

分析步骤:
1. 检查 cpu_partial 列表:
   cat /sys/kernel/slab/<name>/slabs_cpu_partial

2. 检查 node partial:
   cat /sys/kernel/slab/<name>/total_objects

3. 可能原因:
   - 对象被分配但未使用（内存碎片）
   - slab 在 CPU partial 中滞留
   - min_partial 设置过高导致 slab 无法释放

解决方向:
- 调整 cpu_partial 和 min_partial
- 检查是否有内存泄漏
```

#### 问题2: frozen slab 过多

```
现象: 系统内存紧张，但很多 slab 处于 frozen 状态无法回收

分析:
1. frozen slab 属于特定 CPU，其他 CPU 无法直接回收
2. 如果进程频繁在 CPU 间迁移，会导致 slab 在各 CPU 间分布不均

诊断:
  # 查看各 CPU 的 slab 分布
  cat /sys/kernel/slab/<name>/cpu_slabs

  # 使用 perf 检查 CPU 迁移
  perf stat -e migrations -a sleep 10

解决方案:
- 使用 taskset 绑定进程到特定 CPU
- 降低 cpu_partial 限制
- 触发手动 flush: echo 2 > /proc/sys/vm/drop_caches
```

#### 问题3: 对象大小 (size) 与预期不符

```
现象: kmalloc(100) 但实际占用内存远大于 100 字节

分析:
1. size 包含了各种开销：
   size = object_size + inuse + red_left_pad + redzone + alignment

2. 调试选项影响：
   - SLAB_RED_ZONE: 增加前后 redzone
   - SLAB_POISON: 可能增加填充
   - KASAN: 显著增加元数据开销

诊断:
  # 查看实际 size
  cat /sys/kernel/slab/kmalloc-256/object_size
  cat /sys/kernel/slab/kmalloc-256/size

  # 查看对齐要求
  cat /sys/kernel/slab/kmalloc-256/align

优化:
- 生产环境禁用调试选项
- 合理选择 kmalloc 大小（对齐到标准 size）
```

## 六、字段速查表

### kmem_cache 关键字段

| 字段 | 类型 | 含义 | 查看方式 |
|------|------|------|---------|
| cpu_slab | percpu指针 | Per-CPU 本地缓存 | crash/调试器 |
| flags | slab_flags_t | 调试/行为标志 | /sys/kernel/slab/<name>/ |
| size | unsigned int | 对象实际大小 | /sys/kernel/slab/<name>/size |
| object_size | unsigned int | 用户请求大小 | /sys/kernel/slab/<name>/object_size |
| oo | kmem_cache_order_objects | order+objects | /sys/kernel/slab/<name>/order |
| cpu_partial | unsigned int | CPU partial 限制 | /sys/kernel/slab/<name>/cpu_partial |
| min_partial | unsigned long | Node partial 最小值 | /sys/kernel/slab/<name>/min_partial |

### slab 关键字段

| 字段 | 类型 | 含义 | 查看方式 |
|------|------|------|---------|
| slab_cache | 指针 | 所属 kmem_cache | 内部使用 |
| freelist | void* | 空闲对象链表 | 内部使用 |
| inuse | bit-field | 已使用对象数 | /proc/slabinfo |
| objects | bit-field | 总对象数 | /proc/slabinfo |
| frozen | bit-field | 冻结标志 | 内部使用 |
| slab_list | list_head | Node 列表链接 | 内部使用 |

### kmem_cache_cpu 关键字段

| 字段 | 类型 | 含义 | 查看方式 |
|------|------|------|---------|
| freelist | void** | 本地空闲链表 | crash/调试器 |
| tid | unsigned long | Transaction ID | crash/调试器 |
| slab | struct slab* | 当前 slab | crash/调试器 |
| partial | struct slab* | Partial 列表头 | /sys/kernel/slab/<name>/slabs_cpu_partial |


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
