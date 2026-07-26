## 分析
1. 问题背景
   云上内存很贵，用户通常按峰值给 VM 分配内存，导致大量内存长期空闲。云厂商想做 memory overcommit：把 VM 里暂时不用的冷页换出
   到 SSD、远端内存或压缩存储里，把物理内存让给别的 VM。

3. 作者提出的方案
   他们做了一个 用户态 Memory Manager，每个 VM 可以有自己的内存管理策略。它基于：
    - userfaultfd：把 page fault 交给用户态处理；
    - QEMU/VM introspection：把 guest virtual address, GVA 映射到 host virtual address, HVA，用来做更聪明的 prefetch/reclaim；
    - userspace Storage Backend：把换出的页写到 NVMe、RDMA、LocalFS 等后端；
    - policy API：允许用户态写不同的 reclaimer、prefetcher、workload-specific policy。

4. 一个重要观点：2MB hugepage swap 在云 VM 场景很有价值
   文章比较了 4KB 和 2MB swap：
    - 如果 workload 很少访问被换出的冷页，2MB hugepage 保持 TLB 和页表性能，整体更好；
    - 如果 workload 随机访问大量被换出的页，4KB 可能更好，因为每次 fault 只读 4KB；
    - 但实际评测里很多云 workload 有足够 locality，2MB 方案通常性能更稳。

5. 另一个关键观察：虚拟化会破坏地址空间局部性
   Guest 进程的 GVA 访问可能是连续的，但到了 hypervisor 看到的 GPA/HVA 可能已经被 guest OS 分配打散。所以如果 prefetch 只按
   HVA 相邻页做，效果很差；必须通过 VM introspection 看到 GVA 层面的相邻关系。

6. 实验结论
   作者在 BERT、XSBench、Elasticsearch、Graph500、Kafka、matmul、nginx、Redis 等 workload 上评测。结论大致是：
    - 在 best-effort 冷内存回收下，用户态 2MB swap 能在接近 baseline 性能的情况下回收不少内存；
    - 相比 Linux kernel baseline，最多性能提升约 25%；
    - 在内存限制/forced reclamation 场景下，定制 policy 能比 Linux 提升约 30%；
    - workload-specific reclaimer/prefetcher 可以额外节省约 10% 内存或更快恢复性能。

一句话总结：这篇文章是在讲如何为云厂商的 KVM/QEMU 虚拟机场景设计一个灵活的用户态 swap 框架，让 VM 冷内存回收更可控、更适合
hugepage、更容易接入云管控面，并通过可插拔策略获得比 Linux 通用 swap 更好的性能和内存节省。

## kimi : access bit 是什么时候配置上的，是周期性的处理的吗?

Linux 内核对页表 access bit（young
bit）的清理不是由独立定时器周期性地"扫一遍所有页表"完成的，而是主要由内存回收路径按需触发。不过现代内核启用的 MGLRU（Multi-Gen
LRU）会让 kswapd 周期性地 walk page table，在 aging 过程中顺带清理，所以也可以说是"周期性 driven，按需清理"。

1. 传统 LRU / 直接回收路径（按需清理）

当 kswapd 或直接内存回收扫描到某个 LRU 上的 folio 时，如果它是 mapped folio，会调用：

```
  mm/vmscan.c
    folio_check_references()
      -> folio_referenced()           [mm/rmap.c]
         -> rmap walk 遍历所有映射该 folio 的 VMA
            -> clear_flush_young_ptes_notify() / pmdp_clear_flush_young_notify()
               -> ptep_clear_flush_young() / pmdp_clear_flush_young()
```

这里会：
• test-and-clear PTE/PMD 的 young/access bit
• flush TLB（clear_flush_young_ptes 带 flush）

如果清理后发现该页近期被访问过（referenced），就把它保留或激活；否则继续回收。

2. MGLRU 路径（周期性 aging + 清理）

如果启用了 CONFIG_LRU_GEN（现在很多发行版默认开启），kswapd 在 balance_pgdat() 里会调用：

```
  kswapd_age_node()
    -> lru_gen_age_node()      [mm/vmscan.c]
       -> 触发 page table walk
```

真正的 walk 在：

```
  walk_pte_range()   [mm/vmscan.c]
    -> test_and_clear_young_ptes_notify()
       -> ptep_test_and_clear_young()   // 清 young bit，但不逐 entry flush TLB
    -> walk_update_folio()              // 把被访问的页提升到新一代
```

对 PMD-mapped THP：

```
  walk_pmd_range() / walk_pmd_range_locked()
    -> pmdp_test_and_clear_young()      // 清 PMD 的 young bit
```

所以 MGLRU 下：
• 由 kswapd 周期性执行 aging
• aging 过程中会 walk page table，读取并清理 access bit
• 根据 young bit 的状态把 folio 划分到不同 generation

3. 其他清理路径

• page_idle：/sys/kernel/mm/page_idle/bitmap 接口会通过 ptep_test_and_clear_young() / pmdp_test_and_clear_young() 清理 access bit
来检测 idle page。
• unmap / zap_pte_range()：解除映射、fork COW、madvise 等路径也会清 bit。
• 架构特殊路径：比如 x86 的 ptep_test_and_clear_young()、arm64 的 __ptep_test_and_clear_young()（清 PTE_AF）。

4. 架构差异

┌───────┬────────────────────────────────────────────────────────────┐
│ 架构  │ Access bit                                                 │
├───────┼────────────────────────────────────────────────────────────┤
│ x86   │ PTE 的 PTE_ACCESSED 位，硬件自动置位，软件清理             │
├───────┼────────────────────────────────────────────────────────────┤
│ arm64 │ PTE 的 PTE_AF（Access Flag），缺页时由软件置位，回收时清理 │
├───────┼────────────────────────────────────────────────────────────┤
│ s390  │ 用 PGSTE / guest storage key 等更复杂机制                  │
└───────┴────────────────────────────────────────────────────────────┘

一句话总结

不是全局周期清 bit，而是回收/aging 走到哪清到哪。
传统 LRU 是回收时通过 rmap 按需清；MGLRU 是 kswapd 周期性 page table walk 时清，并据此更新 generation。

(其实，我基本上已经猜到了显然不是的，但是为什么可以不用周期性，然后又可以判断出来呢?)

### 如果是这样的话，daemon 如何和系统中的 page fault 机制共存?

## HVA 的局部性
4. 真的存在 HVA 的局部性会很差吗?
	- HVA 的局部性很差，那么 damon 为什么会存在基于 PA 的统计，需要给点原理吧

### kimi
有意义，但要把它理解成近似的冷热密度采样，不是“精确找到每个热页”。

  DAMON 的 region 机制确实依赖一个弱假设：相邻地址页的访问频率可能相近。在 pa 下这个假设比 va 弱，
  因为程序语义上的数组、heap object 连续性在 PA 上可能被打散。但它并不是完全没意义，原因有三个。

  第一，pa 采样估计的是一个物理 region 的“热页比例”。
  如果一个 region 里随机抽一个页，多轮采样后发现经常 accessed，那说明这个 region 里热页密度大概率较
  高。即使热页是散落的，只要散落不是完全随机到全系统均匀分布，统计上仍能筛出更热/更冷的物理范围。

  第二，物理地址并非完全无结构。
  buddy allocator、THP、page cache readahead、匿名页批量 fault、NUMA node、zone、memory tier、CXL/
  DRAM 范围，都会让 PA 上存在某种系统级聚集。它不是 VA 那种“程序对象连续”的局部性，但对 reclaim、
  migration、LRU sort 这类系统策略已经够用。

  第三，DAMOS 动作通常不是只靠 region 结果直接盲操作。
  比如 paddr 的 pageout 会遍历 region 内 folio，并默认加 young filter 做 page-level 再检查，避免把
  刚访问过的页直接回收。也就是说 region 采样常常是第一层筛选，真正执行动作时还能再过滤。

  但你的质疑是成立的：如果 PA 上冷热页真的完全随机、细粒度交错、没有任何物理聚集，那 DAMON paddr 的
  region 采样会退化：

  - 热/冷 region 区分不明显；
  - 需要更多 max_nr_regions 才能提高分辨率；
  - 采样噪声会变大；
  - 对单页级精准决策不适合；
  - 此时 vaddr/fvaddr 更适合分析某个进程的访问模式。

  所以结论是：pa 下 DAMON 的核心机制仍有意义，但它依赖的是弱物理聚集 + 多轮统计 + 动作前过滤。它适
  合做低开销、系统级、近似的内存治理；不适合在 PA 完全散乱的情况下精确还原程序访问局部性。

## 不如搞一个 shmem uffd ?
不如 mmap 一个 fuse ，然后 fuse 的后端是文件系统

userfaultfd 有一个致命问题，就是其监听的空间是虚拟地址空间，这个导致
如果空间是共享给其他的时候，很难处理

- 用户态共享:
	- spdk : 需要注册所有的设备
- kernel 共享 : 简直是灾难，直接导致 kernel thread 无法释放
	- vhsot
	- 那么，如果是 direct io 来 gup 似乎也可以吧

# cgroup 的链表关系

> 每个 memory cgroup 在每个 NUMA node 上都有一个独立的 lruvec，用于组织属于该 memcg、位于该 node 上的可回收 folio。它不是“一条总链表”。

当前代码是 v7.0.1。

## 数据结构

关系大致是：

mem_cgroup
  └── nodeinfo[nid]
        └── mem_cgroup_per_node
              └── lruvec
                    ├── inactive_anon
                    ├── active_anon
                    ├── inactive_file
                    ├── active_file
                    └── unevictable 统计

mem_cgroup_per_node 内嵌 lruvec：

struct mem_cgroup_per_node {
        struct mem_cgroup *memcg;
        ...
        struct lruvec lruvec;
        unsigned long lru_zone_size[MAX_NR_ZONES][NR_LRU_LISTS];
};

见 include/linux/memcontrol.h:85。

经典 LRU 的分类包括：

LRU_INACTIVE_ANON
LRU_ACTIVE_ANON
LRU_INACTIVE_FILE
LRU_ACTIVE_FILE
LRU_UNEVICTABLE

见 include/linux/mmzone.h:316。

所以索引实际是：

(memcg, NUMA node, LRU 类型)

不是简单的：

memcg -> 一条链表

## 一个 folio 如何知道自己属于哪个 cgroup

普通匿名页和文件页在 charge 时，把 mem_cgroup 指针记录到：

folio->memcg_data = (unsigned long)memcg;

见 mm/memcontrol.c:2559。

charge 的主要路径是：

分配 folio
  -> mem_cgroup_charge()
  -> get_mem_cgroup_from_mm()
  -> try_charge()
  -> 更新 memcg 及其祖先的 page_counter
  -> folio->memcg_data = memcg

相关实现见 mm/memcontrol.c:4739。

注意两个概念不同：

- page_counter：负责统计和限制内存用量。
- lruvec：负责组织可以扫描、老化和回收的 folio。

被 memcg 计费的内存不一定都在 LRU 上。例如页表、部分内核内存和 slab 对象有自己的管理机制。

## 如何找到 folio 对应的链表

内核同时根据：

- folio_memcg(folio)：所属 memcg；
- folio_pgdat(folio)：所在 NUMA node；

找到对应的 lruvec：

static inline struct lruvec *folio_lruvec(struct folio *folio)
{
        struct mem_cgroup *memcg = folio_memcg(folio);

        return mem_cgroup_lruvec(memcg, folio_pgdat(folio));
}

而 mem_cgroup_lruvec() 最终执行：

mz = memcg->nodeinfo[pgdat->node_id];
lruvec = &mz->lruvec;

见 include/linux/memcontrol.h:700。

## folio 如何加入链表

调用路径通常是：

folio_add_lru()
  -> 加入当前 CPU 的 folio_batch
  -> batch 满了或者主动 drain
  -> folio_lruvec()
  -> 锁定 lruvec->lru_lock
  -> lruvec_add_folio()
  -> list_add(&folio->lru, &lruvec->lists[lru])

真正加入经典 LRU 链表的位置是：

list_add(&folio->lru, &lruvec->lists[lru]);

见 include/linux/mm_inline.h:340。

为了降低锁竞争，folio_add_lru()不会总是立即修改链表，而是先放进 per-CPU batch，批量处理：

见 mm/swap.c:182 和 mm/swap.c:491。

链表和统计由每个 lruvec 自己的：

spinlock_t lru_lock;

保护，见 include/linux/mmzone.h:669。

## 父子 cgroup 是否各挂一份

不会。

假设：

A
└── B
    └── folio X

folio X 只挂在：

B -> nodeinfo[nid] -> lruvec

不会同时挂到 A 的 LRU 链表，因为一个 folio->lru 链表节点不能同时出现在多条链表中。

但是计费是层次化的：B 的用量会累计到 A。page_counter_try_charge()会从当前 cgroup 一路更新到祖先：

for (c = counter; c; c = c->parent)

见 mm/page_counter.c:110。

回收父 cgroup A 时，内核遍历 A 及其子 cgroup，分别获取每个 memcg 在当前 node 上的 lruvec：

memcg = mem_cgroup_iter(target_memcg, ...);
lruvec = mem_cgroup_lruvec(memcg, pgdat);
shrink_lruvec(lruvec, sc);

见 mm/vmscan.c:5960。

因此：

父 cgroup 的统计 = 层次聚合
父 cgroup 的回收 = 遍历后代各自的 lruvec

## MGLRU 的区别

如果启用了 Multi-Gen LRU，可回收 folio 不再主要使用四条 active/inactive 链表，而是放到：

folios[MAX_NR_GENS][ANON_AND_FILE][MAX_NR_ZONES]

即按以下维度组织：

(memcg, node, generation, anon/file, zone)

结构见 include/linux/mmzone.h:490。

但最外层原则没有变化：仍然是每个 (memcg, NUMA node) 一个 lruvec，只是 lruvec 内部从经典 active/inactive LRU 换成了多代 LRU。

一句话总结：

> memcg 通过 folio->memcg_data记录页的归属，通过每个 NUMA node 上独立的 lruvec维护可回收页；父子 cgroup 共享层次化计数，但页只挂在实际所属
> memcg 的一组 LRU 链表中。

## per cgroup
原来有人一直在做

你说的应该是 LGE 的 Youngjun Park 最近发在 linux-mm 上的 per-cgroup swap 系列 RFC。目前能看到的主要是两个版本：

1. 第一版：per-cgroup swap device priority
    • 标题：[RFC PATCH 0/2] mm/swap, memcg: Support per-cgroup swap device prioritization
    • 作者：Youngjun Park <youngjun.park@lge.com>
    • 时间：2025-06-12
    • 核心想法：每个 cgroup 可以有自己的 memory.swap.priority，格式是 unique_id:priority,...，这样不同 cgroup 对同一批 swap
设备可以使用不同的优先级顺序。
    • 入口链接：
          • LWN 摘要：https://lwn.net/Articles/1025137/
          • Lore 邮件归档：https://lore.kernel.org/linux-mm/20250612103743.3385842-1-youngjun.park@lge.com/
    • 两篇 patch：
          • mm/swap, memcg: basic structure and logic for per cgroup swap priority control
          • mm: swap: apply per cgroup swap priority mechanism on swap layer
2. 第二版（演进为 "Swap Tiers"）
    • 标题：mm/swap, memcg: Introduce swap tiers for cgroup based swap control
    • 时间：2026-01-26（RFC v2）
    • 核心变化：从“每个 cgroup 单独设优先级”改为“Swap Tier”抽象——把若干 swap 设备按速度/用途归到一个 tier，然后 cgroup 通过
swap.tiers 选择允许使用哪些 tier。这样避免了第一版里被指出的 LRU inversion 和过度设计问题，同时仍然实现“每个 cgroup 走不同 swap
场景”的目标。
    • 入口链接：
          • LWN 摘要：https://lwn.net/Articles/1055985/
          • v1 链接在 cover letter 里给出：https://lore.kernel.org/linux-mm/20251109124947.1101520-1-youngjun.park@lge.com/
3. 相关讨论与背景
    • LPC 2025 有专门议程讨论这个方向，题目里明确提到“Currently, the Linux kernel does not provide per-process or per-cgroup swap
selection”。议程页：https://lpc.events/event/19/timetable/?view=standard
    • Kairui Song（Red Hat）等人也有相关回复，提到可以把这套东西和 folio-aware swap allocator、vswap 等方向结合起来。

如果你要的是“每个 cgroup 独立选择 swap 设备/优先级”那套 RFC，最直接的入口就是第一版的 lore 链接；如果你想要的是最新设计，看第二版
Swap Tiers 的 LWN 摘要即可。需要我帮你把 patch 正文或者 memory.swap.priority / swap.tiers 的用法再展开一下吗？

## 各种 kobject 都是该归属谁来控制?

例如这个东西:
```txt
cd /sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service/app.slice
grep -E '^(kernel|kernel_stack|pagetables|percpu|sock|vmalloc|slab)' ./memory.stat

kernel 4802588672
kernel_stack 60817408
pagetables 222142464
percpu 14808904
sock 499712
vmalloc 921600
slab_reclaimable 4283794776
slab_unreclaimable 180386512
slab 4464181288
```

## 不如，uffd 的注册方式给 shmem
然后可以用来监听 swap out event 这个方向

## 想一想 ebpf

你大概率想的是这两篇之一：

**1. XRP: In-Kernel Storage Functions with eBPF**
OSDI 2022，Yuhong Zhong 等。这个最符合“eBPF + NVMe”的记忆点。它把用户注册的 eBPF storage function 放到 **NVMe driver hook / NVMe interrupt handler** 附近执行，用来做类似 B-tree pointer chasing、KV lookup 这种 dependent I/O，避免每一层都回到用户态再发下一次 I/O。论文里还提到为了保持文件系统语义，会把少量 kernel state 传播到 NVMe driver hook。([USENIX][1])

**2. BPF for Storage: An Exokernel-Inspired Approach**
HotOS 2021，基本可以看作 XRP 的前身/思想版。IETF 的 **“eBPF for NVMe”** slides 里提到“eBPF could be used to chase pointers in btrees… eventually the storage device”，并指向的 academic paper 就是这篇 DOI `10.1145/3458336.3465290`。([IETF Datatracker][2])

如果你记得的是“真的把 eBPF offload 到 computational storage / SSD 设备侧”，那可能是：

**3. Delilah: eBPF-offload on Computational Storage**
DaMoN 2023。它更偏 **computational storage device**，论文描述的是在实际 computational storage 平台上支持 eBPF offload；不是单纯在 Linux NVMe driver 里 hook。([ACM数字图书馆][3])

如果你记得的是 NVMe-oF / disaggregated storage：

**4. BPF-oF: Storage Function Pushdown Over the Network**
这是把 eBPF storage function pushdown 到远端 storage server，基于 **NVMe-oF** 场景，目标是减少网络往返和远端存储访问开销。([arXiv][4])

我感觉你说的“eBPF 和 NVMe 那篇”最可能是 **XRP: In-Kernel Storage Functions with eBPF**。搜索关键词可以用：

```text
XRP In-Kernel Storage Functions with eBPF NVMe
BPF for Storage Exokernel-Inspired Approach NVMe
eBPF for NVMe XRP
```

[1]: https://www.usenix.org/conference/osdi22/presentation/zhong?utm_source=chatgpt.com "XRP: In-Kernel Storage Functions with eBPF"
[2]: https://datatracker.ietf.org/meeting/116/materials/slides-116-bpf-ebpf-for-nvme-00.pdf?utm_source=chatgpt.com "eBPF for NVMe"
[3]: https://dl.acm.org/doi/10.1145/3592980.3595319?utm_source=chatgpt.com "Delilah: eBPF-offload on Computational Storage"
[4]: https://arxiv.org/abs/2312.06808?utm_source=chatgpt.com "BPF-oF: Storage Function Pushdown Over the Network"



## TODO
现在 swap 对于大页的支持是什么样子的?

1. KVM/EPT access bit 扫描：判断哪些 guest physical page 最近被访问；
	- 直接做这个扫描，和使用通用的机制，有区别吗?
		- 有，vhost 的区别?

3. daemon 说，用户态难以选择页面是站不住脚的

6. 简单考虑一下内核的 capabilities 的问题
	- 如果一个 user process 可以接受到 root process 页面，这显然不合理

7. 调研一下，其他的 userfault 的使用者都是如何处理的?
5. 调查一下 mthp 和 tireing memory ，不然似乎我丧失了全貌

6. cgroup 是如何实现单个 cgroup 内计算 cache 的

7. 这里已经提到了 per cgroup 的 swap
	- https://lpc.events/event/19/timetable/?view=standard

## 继续做这个观察吧
https://arxiv.org/html/2409.13327v1#S3.SS1

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
