# Data Structures

## 一、章节定位与核心思想

本章以 Linus Torvalds 的名言开篇："Bad programmers worry about the code. Good programmers worry about data structures and their relationships." 在并发编程中，这一论断被进一步放大——数据结构的设计不仅要考虑时间复杂度，更要考虑并发效应（concurrency effects）。因为在多核、多 socket 系统上，并发带来的开销往往会淹没算法本身的复杂度优势。

本章通过"Schrodinger's Zoo"这一动机应用，揭示了并发数据结构设计中的四大复杂问题：

1. **NUMA 导致的扩展性失败**：即使完全按照分区原则设计的数据结构，在跨 NUMA 节点时仍可能性能崩塌。
2. **Cache 容量导致的扩展性失败**：即使使用延迟处理（deferred processing）技术，Cache 容量不足仍会导致扩展性问题。
3. **无同步只读遍历的扩展性失败**：即使完全避免同步的只读遍历，在数据量超过 Cache 容量时仍无法线性扩展。
4. **并发更新导致的扩展性失败**：即使避开了上述所有问题，并发更新仍可能阻碍遍历性能。

本章以**哈希表**为主线，依次探讨了可分区、读多写少、不可分区三类数据结构的实现与优化。

---

## 二、动机应用：Schrodinger's Zoo

这是一个内存数据库应用，每个动物对应一个数据项，以名字为 key。应用特点包括：

- **高更新率**：出生、捕获、购买对应插入；死亡、释放、销售对应删除。由于动物园中有大量短寿命动物（如老鼠、昆虫），数据库必须处理极高的更新频率。
- **高查询率**：对特定动物（尤其是 Schrodinger 的猫）存在极高的查询频率，即存在**热点（hot spot）**。

这个简单应用对传统并发数据结构提出了严峻挑战：不仅要支持高并发读写，还要处理热点问题。

---

## 三、可分区数据结构：哈希表与 per-bucket 锁

### 3.1 设计原理

哈希表是并发编程中最重要的"功臣"之一，核心原因在于它是**完全可分区（completely partitionable）**的。哈希表由一组哈希桶（hash bucket）组成，每个桶管理一条哈希链（hash chain）。通过哈希函数将 key 映射到桶，只要桶的数量足够多且哈希函数分布均匀，每条链都会很短，从而实现 O(1) 的访问效率。

关键设计决策：

- **每个桶一把锁**：不同桶的操作完全独立，实现了数据层面的分区。
- **不返回元素计数**：`hashtab_add()` 和 `hashtab_del()` 返回 void，避免了全局计数的竞争。
- **允许重复元素**：简化接口，消除错误返回值。
- **删除时传入元素指针**：避免了在删除路径上做复杂查找。

### 3.2 代码实现（perfbook 原版）

核心数据结构：

```c
struct ht_elem {
    struct cds_list_head hte_next;
    unsigned long hte_hash;
};

struct ht_bucket {
    struct cds_list_head htb_head;
    spinlock_t htb_lock;
};

struct hashtab {
    unsigned long ht_nbuckets;
    int (*ht_cmp)(struct ht_elem *htep, void *key);
    struct ht_bucket ht_bkt[0];
};
```

查找函数先比较缓存的 hash 值，再调用回调比较实际 key：

```c
struct ht_elem *hashtab_lookup(struct hashtab *htp, unsigned long hash, void *key)
{
    struct ht_bucket *htb = HASH2BKT(htp, hash);
    struct ht_elem *htep;
    cds_list_for_each_entry(htep, &htb->htb_head, hte_next) {
        if (htep->hte_hash != hash)
            continue;
        if (htp->ht_cmp(htep, key))
            return htep;
    }
    return NULL;
}
```

修改操作要求调用者先通过 `hashtab_lock_mod()` 获取对应桶的锁。

### 3.3 性能陷阱：NUMA 效应

在单 socket（28 核）上，只读哈希表性能几乎线性扩展。但当扩展到 448 核（8 socket）时，性能在 29 核（即第二个 socket）处断崖式下跌。原因在于：

- **锁导致的写操作**：即使只读查询，`pthread_mutex_lock/unlock` 或 spinlock 也会修改锁变量所在的 Cache line，引发跨 socket 的 Cache coherence 流量。
- **数据局部性缺失**：测试系统的 CPU 0--27 和 224--251 映射到 socket 0，CPU 28 映射到 socket 1。一旦涉及跨 socket，延迟剧增。

增加桶数量（从 262144 到更大）几乎没有任何效果，证明问题不是锁竞争，而是** NUMA 远程访问开销**。

**启示**：在大型多 socket 系统上，仅靠分区不够，还必须保证**访问局部性（locality of reference）**。

---

## 四、读多写少数据结构：RCU 与 Hazard Pointers

### 4.1 核心思路

对于读多写少的工作负载，RCU（Read-Copy Update）可以将读端的同步开销降到极低。基本策略是：

- **读端**：完全无锁，仅通过 `rcu_read_lock()` / `rcu_read_unlock()` 标记临界区。
- **写端**：继续使用 per-bucket 锁，但使用 RCU-aware 的链表操作。

### 4.2 RCU 哈希表的关键变化

相比 per-bucket lock 版本，RCU 保护版本有以下变化：

**读端并发控制**：

```c
static void hashtab_lock_lookup(struct hashtab *htp, unsigned long hash)
{
    rcu_read_lock();
}

static void hashtab_unlock_lookup(struct hashtab *htp, unsigned long hash)
{
    rcu_read_unlock();
}
```

**查找遍历**：使用 `cds_list_for_each_entry_rcu()` 代替普通版本，确保在并发插入时内存顺序正确。

```c
cds_list_for_each_entry_rcu(htep, &htb->htb_head, hte_next) {
    ...
}
```

**修改操作**：

```c
void hashtab_add(struct hashtab *htp, unsigned long hash, struct ht_elem *htep)
{
    htep->hte_hash = hash;
    cds_list_add_rcu(&htep->hte_next, &HASH2BKT(htp, hash)->htb_head);
}

void hashtab_del(struct ht_elem *htep)
{
    cds_list_del_rcu(&htep->hte_next);
}
```

关键点：

- `cds_list_add_rcu()` 确保插入操作对读者可见的顺序性。
- `cds_list_del_rcu()` 不修改被删元素的前向指针，这样正在遍历的读者仍能到达下一个元素。
- 调用 `hashtab_del()` 后，必须等待一个 grace period（如 `synchronize_rcu()`）才能释放元素内存。

### 4.3 性能分析

**只读性能**：RCU 和 hazard pointers 都远优于 per-bucket locking。原因是读端完全避免了写共享数据，数据可以被复制到所有 CPU 的 Cache 中，消除了 NUMA 效应。

**Cache 容量瓶颈**：但即便使用完全无同步（unsynchronized）的读，性能仍只有理想的 1/5。问题在于数据量（262144 个桶 + 262144 个元素，约 33MB）远超 L2 Cache（1MB），甚至接近 L3 Cache（40MB）。当数据量超过 Cache 容量时，内存系统成为瓶颈。log-log 图显示：在 8000 个元素以下（约 1MB）性能接近理想；超过 30 万元素时（超出 L3 Cache）性能断崖式下跌。

**更新影响**：随着更新 CPU 增加，查询吞吐量下降。RCU 表现优于 hazard pointers，因为后者读端需要 memory barrier，在更新存在时开销更大。

**热点问题**：当所有 CPU 只查询"猫"时，per-bucket lock 退化为全局锁，性能与全局锁相当。RCU 和 hazard pointers 仍能很好地处理热点，因为读端无竞争。

### 4.4 一致性讨论

RCU 允许并发读者看到不同的状态：一个读者可能在删除前获取指针，另一个在删除后。作者以"兽医判断猫是否死亡"为例，说明在许多真实场景中，**绝对内部一致性**既不可能也不必要。RCU 和 hazard pointers 放弃部分内部一致性，换取更好的外部一致性和性能。

---

## 五、不可分区数据结构：可调整大小哈希表

### 5.1 问题所在

固定大小哈希表完全可分区，但**可调整大小**的哈希表在 grow/shrink 时面临分区挑战：resize 操作本身涉及所有桶，无法简单分区。

### 5.2 Herbert Xu 的双指针集方案

核心洞察：每个数据元素维护**两组链表指针**。一组被当前读者使用，另一组用于构建新尺寸的哈希表。

数据结构变化：

```c
struct ht_elem {
    struct rcu_head rh;
    struct cds_list_head hte_next[2];  // 两组指针
};

struct ht {
    long ht_nbuckets;
    long ht_resize_cur;   // -1 表示未在 resize，否则表示已迁移到的桶索引
    struct ht *ht_new;    // 指向新表
    int ht_idx;           // 当前使用哪组指针（0 或 1）
    struct ht_bucket ht_bkt[0];
};

struct hashtab {
    struct ht *ht_cur;
    spinlock_t ht_lock;   // 串行化并发 resize 尝试
};
```

Resize 过程：

1. 分配新 `ht`，使用另一组指针索引（`!idx`）。
2. 将新表通过 `rcu_assign_pointer()` 挂到旧表的 `ht_new`。
3. `synchronize_rcu()` 等待所有旧读者完成。
4. 遍历旧表每个桶，将元素通过新指针集挂到新表对应桶。
5. `rcu_assign_pointer()` 将新表设为当前表。
6. 再次 `synchronize_rcu()` 等待引用旧表的读者完成。
7. 释放旧表。

### 5.3 更新端并发控制

`hashtab_lock_mod()` 需要处理并发 resize：

```c
void hashtab_lock_mod(struct hashtab *htp_master, void *key,
                      struct ht_lock_state *lsp)
{
    rcu_read_lock();
    struct ht *htp = rcu_dereference(htp_master->ht_cur);
    struct ht_bucket *htbp = ht_get_bucket(htp, key, &b, &h);
    spin_lock(&htbp->htb_lock);
    lsp->hbp[0] = htbp;
    lsp->hls_idx[0] = htp->ht_idx;
    if (b > READ_ONCE(htp->ht_resize_cur)) {
        lsp->hbp[1] = NULL;
        return;
    }
    // 该桶已被迁移，还需锁新桶
    htp = rcu_dereference(htp->ht_new);
    htbp = ht_get_bucket(htp, key, &b, &h);
    spin_lock(&htbp->htb_lock);
    lsp->hbp[1] = htbp;
    lsp->hls_idx[1] = htp->ht_idx;
}
```

关键点：

- 如果桶尚未被迁移，只锁旧桶。
- 如果桶已被迁移，需要同时锁旧桶和新桶，防止并发 resize 与更新冲突。
- 总是先锁旧桶再锁新桶，避免死锁。

`hashtab_add()` 和 `hashtab_del()` 根据 `lsp->hbp[1]` 是否非空，决定是否需要同时修改两张表：

```c
void hashtab_add(struct ht_elem *htep, struct ht_lock_state *lsp)
{
    int i = lsp->hls_idx[0];
    cds_list_add_rcu(&htep->hte_next[i], &lsp->hbp[0]->htb_head);
    if (lsp->hbp[1])
        cds_list_add_rcu(&htep->hte_next[!i], &lsp->hbp[1]->htb_head);
}
```

### 5.4 性能

可调整大小哈希表的性能和扩展性几乎与固定大小 RCU 哈希表相当。resize 期间的额外开销主要来自指针更新的 Cache miss，在内存系统成为瓶颈时尤为明显。因此建议：

- resize 幅度要大，避免频繁 resize。
- 使用**滞回（hysteresis）**防止在边界上来回 resize。
- 在内存充足环境下，增大比减小更激进。

### 5.5 其他可调整大小哈希表

**Relativistic hash table**（Josh Triplett）：每个元素只需一组指针。resize 时通过增量"拉链"（unzip）方式分裂/合并链表。 shrinking 时新桶直接指向旧链，读者可能多遍历几个不匹配的节点，但这无害（会被 key 比较过滤）。growing 时需要多次 grace period 逐步将链表拆分到新桶。

**Split-order list + RCU**：将元素组织成一条有序链表，每个桶指向该桶的第一个元素。删除时标记指针低位，后续遍历遇到时移除。提供 lock-free 的插入、删除、查找保证，但读端需要位反转 key，有一定开销。

---

## 六、其他并发数据结构

本章最后简要回顾了其他重要数据结构：

- **Radix tree / trie**：天然可分区，按 key 的位逐层划分。Linux 内核中大量使用（如页缓存）。稀疏 key 空间可能导致内存浪费，可通过哈希 key 到更小的空间来缓解。
- **数组 / 矩阵**：完全可分区，在数值计算中广泛应用。
- **红黑树**：顺序代码中广泛使用，但重新平衡（rebalance）激进，不利于并行。现代方案使用 RCU 保护读端，用 hashed array of locks 保护写端。另有"bonsai trees"通过减少重平衡频率来优化并发更新。
- **Skip lists**：天然适合 RCU 读者，是早期类似 RCU 技术的学术应用。
- **栈和队列**：Treiber stack 是经典无锁栈；现代研究倾向于放宽严格 FIFO/LIFO 语义以获得更好性能。

---

## 七、结合 Linux 内核源码分析

### 7.1 内核基础哈希表：`include/linux/hashtable.h`

内核提供了静态大小的哈希表宏和 RCU 支持：

```c
#define DEFINE_HASHTABLE(name, bits) \
    struct hlist_head name[1 << (bits)] = \
        { [0 ... ((1 << (bits)) - 1)] = HLIST_HEAD_INIT }

#define hash_add(hashtable, node, key) \
    hlist_add_head(node, &hashtable[hash_min(key, HASH_BITS(hashtable))])

#define hash_add_rcu(hashtable, node, key) \
    hlist_add_head_rcu(node, &hashtable[hash_min(key, HASH_BITS(hashtable))])

#define hash_del_rcu(node) hlist_del_init_rcu(node)
```

内核使用 `hlist_head` / `hlist_node`（单指针头、双指针节点的链表）实现哈希链。RCU 版本在 `include/linux/rculist.h` 中定义：

```c
static inline void hlist_add_head_rcu(struct hlist_node *n,
                                      struct hlist_head *h)
{
    struct hlist_node *first = h->first;
    n->next = first;
    n->pprev = &h->first;
    rcu_assign_pointer(hlist_first_rcu(h), n);
    if (first)
        first->pprev = &n->next;
}
```

这与 perfbook 中的 `cds_list_add_rcu()` 原理一致：通过 `rcu_assign_pointer()` 确保读者能安全看到新节点。

### 7.2 内核可调整大小哈希表：`rhashtable`

内核的 `rhashtable`（`include/linux/rhashtable.h`）是 Herbert Xu 等人开发的工业级可调整大小哈希表，被网络子系统、BPF、IPC 等大量使用。

关键设计：

```c
struct rhashtable {
    struct bucket_table __rcu *tbl;
    struct rhashtable_params p;
    struct work_struct run_work;   // 异步 resize worker
    struct mutex mutex;
    atomic_t nelems;
};

struct bucket_table {
    unsigned int size;
    struct bucket_table __rcu *future_tbl;
    struct rhash_lock_head __rcu *buckets[] ____cacheline_aligned_in_smp;
};
```

与 perfbook 的双指针集方案不同，`rhashtable` 每个元素只有**一个 `rhash_head` 指针**。resize 时利用 **bit 0 作为 bucket lock**，通过 `bit_spinlock` 保护链表的并发修改。链尾使用 **NULLS marker**（最低位为 1 的特殊指针）来标识正确的桶边界，防止读者穿越到错误的桶。

这对应了 perfbook 中提到的 Josh Triplett 的 relativistic hash table 改进方案——内核实现通过维护单独指针跟踪迁移进度，大大减少了 resize 所需的 grace period 数量。

### 7.3 内核中的实际使用案例

**`kernel/workqueue.c`**：使用 RCU 哈希表 `wci_hash` 管理工作项回调信息：

```c
static DEFINE_HASHTABLE(wci_hash, ilog2(WCI_MAX_ENTS));
// ...
hash_add_rcu(wci_hash, &ent->hash_node, (unsigned long)func);
```

**`kernel/cgroup/cgroup.c`**：`css_set_table` 使用 `DEFINE_HASHTABLE` 管理 cgroup 子系统状态集。

**`kernel/livepatch/shadow.c`**：使用 RCU 哈希表 `klp_shadow_hash` 管理 livepatch 的 shadow 变量，使用 `hash_add_rcu` / `hash_del_rcu`。

**`kernel/pid.c`**：PID 哈希表传统上使用 `tasklist_lock` 保护，但 PID 分配本身是 lockless 的（基于 bitmap）。这展示了即使是内核核心数据结构，也会根据访问模式混合使用不同的同步策略。

---

## 八、本机代码实现与测试

为了验证本章概念，编写了三个简化版 C 程序，配有 Makefile，二进制后缀为 `.out`。

### 8.1 固定大小哈希表（`hash_fixed.c`）

使用 `pthread_mutex_t` 实现 per-bucket 锁，模拟本章第一版实现。

```c
struct ht_bucket {
    struct ht_elem *head;
    pthread_mutex_t lock;
};

static struct ht_elem *hashtab_lookup(struct hashtab *htp, int key)
{
    unsigned long h = hashfn(key);
    struct ht_bucket *b = &htp->bkt[h % htp->nbuckets];
    pthread_mutex_lock(&b->lock);
    // 遍历链表...
    pthread_mutex_unlock(&b->lock);
}
```

测试：4 个 reader，2 个 writer，10 万元素，1024 个桶。

### 8.2 RCU-like 读优化哈希表（`hash_rcu.c`）

使用 C11 `_Atomic` 指针和 memory order 实现无锁读端，写端仍使用 per-bucket mutex。

```c
// 读端完全无锁
struct ht_elem *e = atomic_load_explicit(&b->head, memory_order_acquire);
while (e) {
    if (e->key == key) return e;
    e = atomic_load_explicit(&e->next, memory_order_acquire);
}

// 写端使用 release 语义发布新节点
atomic_thread_fence(memory_order_release);
atomic_store_explicit(&b->head, e, memory_order_release);
```

这是 RCU 的核心思想在用户空间的简化模拟：读者通过 acquire-load 看到写者的 release-store，无需显式锁。

### 8.3 可调整大小哈希表（`hash_resize.c`）

使用 indirection（`hashtab -> ht`）实现 resize。`hashtab` 持有 `_Atomic(struct ht *) cur`，resize 时分配新 `ht`、迁移数据、原子替换指针。

```c
struct hashtab {
    _Atomic(struct ht *) cur;
    pthread_mutex_t resize_lock;
};
```

resize 过程锁化整个旧表逐桶迁移，然后原子切换指针。这是教学简化版，实际生产环境应使用更细粒度的并发控制（如内核 `rhashtable` 的 per-bucket bit-lock）。

### 8.4 编译与测试结果

```bash
$ make test
gcc -O2 -Wall -Wextra -pthread -o hash_fixed.out hash_fixed.c
gcc -O2 -Wall -Wextra -pthread -o hash_rcu.out hash_rcu.c
gcc -O2 -Wall -Wextra -pthread -o hash_resize.out hash_resize.c
=== Testing hash_fixed.out ===
fixed-hash: 0.159 sec (readers=4 writers=2 items=100000 buckets=1024)
=== Testing hash_rcu.out ===
rcu-like-hash: 0.109 sec (readers=4 writers=2 items=100000 buckets=1024)
=== Testing hash_resize.out ===
resize-hash: 0.003 sec (readers=4 items=50000 resized 256->2048)
```

结果分析：

- **RCU-like 版本比 fixed 版本快约 31%**（0.109s vs 0.159s），在 4 reader + 2 writer 场景下，无锁读端的优势已经体现。在更高并发或 NUMA 环境下，优势会更显著。
- **resize 版本**在 4 个 reader 并发执行时完成 resize，耗时仅 0.003 秒，说明 indirection 方案的 resize 开销在可控范围内。

---

## 九、与并发笔记的关联：是否解答了疑问

### 9.1 `data-structure.md` 中的核心疑问

> "哪些数据结构是可以并行的，那些是数据结构没办法？"  
> "内核中那些数据结构是并行的，那些不是？"  
> "红黑树可以并行访问吗？"

本章给出了明确答案：

1. **完全可分区**：哈希表（固定大小）、数组、矩阵、radix tree。这类结构在单 socket 上几乎线性扩展。
2. **读多写少优化**：通过 RCU / hazard pointers，哈希表、树、skip list 都可以实现高效的并发读。写端仍需锁或其他同步。
3. **不可分区但可优化**：可调整大小哈希表通过双指针集或 relativistic 方法，实现了 resize 与读写并发。
4. **红黑树**：本章明确指出红黑树可以并行读（RCU），但写端需要 hashed array of locks。内核中的 bonsai tree 进一步通过降低重平衡频率来优化并发写。因此"红黑树可以并行访问吗？"的答案是：**读可以无锁并行，写可以锁分区并行**。

内核中的对应：

- `hashtable.h` + `rculist.h`：基础可分区 + RCU 读。
- `rhashtable.h`：不可分区但高度优化的可调整大小哈希表。
- `radix-tree.h` / `xarray.h`：可分区树结构，广泛用于页缓存。

### 9.2 `todo.md` 中的 RCU 相关疑问

> "如何正确的使用 RCU，以及 Linux 内核中的 ART"

本章提供了 RCU 在哈希表中的完整使用范式：

- 读端：`rcu_read_lock()` -> `rcu_dereference()` / RCU-aware 遍历 -> `rcu_read_unlock()`
- 写端：获取 bucket lock -> `cds_list_add_rcu()` / `cds_list_del_rcu()` -> `synchronize_rcu()` / `call_rcu()` 后释放内存
- resize：`rcu_assign_pointer()` 发布新表 -> `synchronize_rcu()` 等待旧读者 -> 释放旧结构

内核中 `rhashtable` 正是这一范式的工业级实现，使用了 `rcu_assign_pointer`、`synchronize_rcu`、`call_rcu` 等核心 API。

### 9.3 尚未完全解答的疑问

- **Memory model 的形成原因**：本章提到了 memory barrier 和 acquire/release 语义，但没有从 CPU 微架构（store buffer、invalidate queue）角度深入解释为什么需要这些语义。这属于本书其他章节（如 Hardware and its Habits、Memory Ordering）的范畴。
- **wait-free / lock-free 转化方法**：本章主要讨论的是 RCU + locking，仅在 Other Data Structures 一节提到 split-order list 提供 lock-free 保证。对于 todo.md 中提到的"wait-free 转化的基本方法（compareAndSet 失败重试、help）"，本章没有涉及。
- **Hazard pointers 的详细实现**：本章将其作为 RCU 的替代品提及，但没有展开其 read-side memory barrier 和 retire 机制。

---

## 十、总结

本章通过哈希表这一核心数据结构，层层深入地展示了并发数据结构设计中的关键权衡：

1. **分区是第一步**：per-bucket lock 的哈希表在单 socket 上表现优异，是并发编程的基础工具。
2. **NUMA 和 Cache 是隐形杀手**：跨 socket 访问和 Cache 容量不足可以轻易摧毁扩展性，即使算法上完美无缺。
3. **RCU 是读多写少的利器**：通过消除读端同步，RCU 不仅避免了锁竞争，还消除了 NUMA 效应和 Cache line bouncing。
4. **Indirection 解决不可分区问题**：可调整大小哈希表通过引入间接层（双指针集或单独进度指针），在保持 RCU 读端优势的同时，实现了动态的容量调整。
5. **一致性是有代价的**：绝对内部一致性并非所有问题域的自然需求。RCU 的弱一致性模型在真实世界（如心跳检测、硬件故障检测）中往往是合理且必要的。

对于工程实践，本章的启示是：

- 优先选择天然可分区的数据结构（哈希表、数组、radix tree）。
- 在大型 NUMA 系统上，必须为读端消除写操作（RCU / hazard pointers）。
- 关注数据结构的内存 footprint，确保热数据能留在 L3 Cache 内。
- 对于必须 resize 的场景，使用 indirection + RCU，或直接使用内核的 `rhashtable`。

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
