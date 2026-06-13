# Multi-Gen LRU

## doc
### https://docs.kernel.org/mm/multigen_lru.html

- The youngest generation number is stored in `lrugen->max_seq` for both anon and file types as they are aged on an equal footing.
- The oldest generation numbers are stored in `lrugen->min_seq[]`

Each generation is divided into multiple tiers.
A page accessed `N` times through file descriptors is in tier `order_base_2(N)`.

> The aging produces young generations.
Given an lruvec, it increments `max_seq` when `max_seq-min_seq+1` approaches `MIN_NR_GENS`.

### https://docs.kernel.org/admin-guide/mm/multigen_lru.html

为什么要 clear bit ?

- /sys/kernel/mm/lru_gen/enabled : 中控制了这两个判断:
```c
static bool should_walk_mmu(void)
{
	return arch_has_hw_pte_young() && get_cap(LRU_GEN_MM_WALK);
}

static bool should_clear_pmd_young(void)
{
	return arch_has_hw_nonleaf_pmd_young() && get_cap(LRU_GEN_NONLEAF_YOUNG);
}
```
不过，想不到还有 clear pmd 的操作

- min_ttl_ms : 如何才可以实现让一个 memory 待这么多时间，给每一个 page 一个来时间上的计时，怎么可能?


似乎是，使用 Bloom filter 来记录哪些 pmd 指向的 page 应该是持有 young pages 的:


使用 core/vn/code/src/m/mm/swap.sh 观测到的:
```txt
memcg    22 /swap_test
 node     0
    1127118         24       6947           0
    1127119          8      10961           0
    1127120          6       4001           0
 node     1
      37107         96          0           1
      37108         95          2           0
      37109         90          0           0
```

- [ ] 没有理解 tier 的含义
  - 将 tier 放到 `folio->flags` 中去

A feedback loop modeled after the PID controller monitors refaults over all the tiers from anon and file types
and decides which tiers from which types to evict or protect.

To select a type and a tier to evict from, it first compares `min_seq[]` to select the older type.

### https://lwn.net/Articles/851184/

The use of independent lists in control groups makes it hard for the kernel to compare the relative age of pages across groups.

Add more LRU lists to cover a range of page ages between the current active and inactive lists; these lists are called "generations".

This mechanism allows for ready comparison of the ages of anonymous and file-backed pages,
and, by **tracking the creation time of each generation**, of the ages of pages in different control groups;
this information is lost in current kernels. That, in turn, makes it easier to identify and reclaim idle anonymous pages.

- 通过比较 generation 创建的时间就可以吗?

### https://lwn.net/Articles/894859/


## 两个路径的区分
只有不打开 gen_lru 的时候，这个路径才会走到。
  - ret_from_fork_asm
    - ret_from_fork
      - kthread
        - khugepaged
          - khugepaged_do_scan
            - khugepaged_scan_mm_slot
              - hpage_collapse_scan_pmd
                - collapse_huge_page
                  - __collapse_huge_page_isolate
                    - isolate_lru_page
                      - folio_isolate_lru

此外，folio_isolate_lru 也不容易走到。

## 参考

1. folio_mark_accessed : 使用的是 folio_inc_refs

## 仔细看看这个文档吧


## lru_gen 会让让 cgroup 中的 memory.stat 完全失真 nr_inactive_anon

现在我也不知道到底怎么回事?
```txt
nr_inactive_anon 75468
nr_active_anon 1676837
nr_inactive_file 1377505
nr_active_file 733412
```

mmap 文件，都是开始是 inactive 的，但是之后是 active 的
当进行 shrink 的时候，这个数值会发生修改，但是这个数值并不是类似

- [ ] 可以对比下 lru gen 和 非 lru gen 的初始值的差别，也就是没有 shrink 的时候的过

https://lwn.net/Articles/856931/


## 先看看如何使用
- https://docs.kernel.org/admin-guide/mm/multigen_lru.html
- https://docs.kernel.org/mm/multigen_lru.html

看不懂

## 看看代码吧

如果用 lru gen ，那么会直接将其中的 lru 跳过:

shrink_lruvec -> lru_gen_shrink_lruvec
shrink_node -> lru_gen_shrink_node
snapshot_refaults
kswapd_age_node
prepare_scan_control

```c
	if (lru_gen_enabled() && !root_reclaim(sc)) {
		lru_gen_shrink_lruvec(lruvec, sc);
		return;
	}
```




```txt
History:        #0
Commit:         a579086c99ed70cc4bfc104348dbe3dd8f2787e6
Author:         Yu Zhao <yuzhao@google.com>
Committer:      Andrew Morton <akpm@linux-foundation.org>
Author Date:    Thu 22 Dec 2022 12:19:01 PM CST
Committer Date: Thu 19 Jan 2023 09:12:48 AM CST

mm: multi-gen LRU: remove eviction fairness safeguard

Recall that the eviction consumes the oldest generation: first it
bucket-sorts folios whose gen counters were updated by the aging and
reclaims the rest; then it increments lrugen->min_seq.

The current eviction fairness safeguard for global reclaim has a
dilemma: when there are multiple eligible memcgs, should it continue
or stop upon meeting the reclaim goal? If it continues, it overshoots
and increases direct reclaim latency; if it stops, it loses fairness
between memcgs it has taken memory away from and those it has yet to.

With memcg LRU, the eviction, while ensuring eventual fairness, will
stop upon meeting its goal. Therefore the current eviction fairness
safeguard for global reclaim will not be needed.

Note that memcg LRU only applies to global reclaim. For memcg reclaim,
the eviction will continue, even if it is overshooting. This becomes
unconditional due to code simplification.

Link: https://lkml.kernel.org/r/20221222041905.2431096-4-yuzhao@google.com
Signed-off-by: Yu Zhao <yuzhao@google.com>
Cc: Johannes Weiner <hannes@cmpxchg.org>
Cc: Jonathan Corbet <corbet@lwn.net>
Cc: Michael Larabel <Michael@MichaelLarabel.com>
Cc: Michal Hocko <mhocko@kernel.org>
Cc: Mike Rapoport <rppt@kernel.org>
Cc: Roman Gushchin <roman.gushchin@linux.dev>
Cc: Suren Baghdasaryan <surenb@google.com>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>

diff --git a/mm/vmscan.c b/mm/vmscan.c
index d8a53b7443d4..bfbfc98c856c 100644
--- a/mm/vmscan.c
+++ b/mm/vmscan.c
@@ -449,6 +449,11 @@ static bool cgroup_reclaim(struct scan_control *sc)
 	return sc->target_mem_cgroup;
 }
```

也需要看看:
```c
/*
 * Evictable pages are divided into multiple generations. The youngest and the
 * oldest generation numbers, max_seq and min_seq, are monotonically increasing.
 * They form a sliding window of a variable size [MIN_NR_GENS, MAX_NR_GENS]. An
 * offset within MAX_NR_GENS, i.e., gen, indexes the LRU list of the
 * corresponding generation. The gen counter in folio->flags stores gen+1 while
 * a page is on one of lrugen->folios[]. Otherwise it stores 0.
 *
 * A page is added to the youngest generation on faulting. The aging needs to
 * check the accessed bit at least twice before handing this page over to the
 * eviction. The first check takes care of the accessed bit set on the initial
 * fault; the second check makes sure this page hasn't been used since then.
 * This process, AKA second chance, requires a minimum of two generations,
 * hence MIN_NR_GENS. And to maintain ABI compatibility with the active/inactive
 * LRU, e.g., /proc/vmstat, these two generations are considered active; the
 * rest of generations, if they exist, are considered inactive. See
 * lru_gen_is_active().
 *
 * PG_active is always cleared while a page is on one of lrugen->folios[] so
 * that the aging needs not to worry about it. And it's set again when a page
 * considered active is isolated for non-reclaiming purposes, e.g., migration.
 * See lru_gen_add_folio() and lru_gen_del_folio().
 *
 * MAX_NR_GENS is set to 4 so that the multi-gen LRU can support twice the
 * number of categories of the active/inactive LRU when keeping track of
 * accesses through page tables. This requires order_base_2(MAX_NR_GENS+1) bits
 * in folio->flags.
 */
#define MIN_NR_GENS		2U
#define MAX_NR_GENS		4U

/*
 * Each generation is divided into multiple tiers. A page accessed N times
 * through file descriptors is in tier order_base_2(N). A page in the first tier
 * (N=0,1) is marked by PG_referenced unless it was faulted in through page
 * tables or read ahead. A page in any other tier (N>1) is marked by
 * PG_referenced and PG_workingset. This implies a minimum of two tiers is
 * supported without using additional bits in folio->flags.
 *
 * In contrast to moving across generations which requires the LRU lock, moving
 * across tiers only involves atomic operations on folio->flags and therefore
 * has a negligible cost in the buffered access path. In the eviction path,
 * comparisons of refaulted/(evicted+protected) from the first tier and the
 * rest infer whether pages accessed multiple times through file descriptors
 * are statistically hot and thus worth protecting.
 *
 * MAX_NR_TIERS is set to 4 so that the multi-gen LRU can support twice the
 * number of categories of the active/inactive LRU when keeping track of
 * accesses through file descriptors. This uses MAX_NR_TIERS-2 spare bits in
 * folio->flags.
 */
#define MAX_NR_TIERS		4U
```

```txt
/*
 * For each node, memcgs are divided into two generations: the old and the
 * young. For each generation, memcgs are randomly sharded into multiple bins
 * to improve scalability. For each bin, the hlist_nulls is virtually divided
 * into three segments: the head, the tail and the default.
 *
 * An onlining memcg is added to the tail of a random bin in the old generation.
 * The eviction starts at the head of a random bin in the old generation. The
 * per-node memcg generation counter, whose reminder (mod MEMCG_NR_GENS) indexes
 * the old generation, is incremented when all its bins become empty.
 *
 * There are four operations:
 * 1. MEMCG_LRU_HEAD, which moves a memcg to the head of a random bin in its
 *    current generation (old or young) and updates its "seg" to "head";
 * 2. MEMCG_LRU_TAIL, which moves a memcg to the tail of a random bin in its
 *    current generation (old or young) and updates its "seg" to "tail";
 * 3. MEMCG_LRU_OLD, which moves a memcg to the head of a random bin in the old
 *    generation, updates its "gen" to "old" and resets its "seg" to "default";
 * 4. MEMCG_LRU_YOUNG, which moves a memcg to the tail of a random bin in the
 *    young generation, updates its "gen" to "young" and resets its "seg" to
 *    "default".
 *
 * The events that trigger the above operations are:
 * 1. Exceeding the soft limit, which triggers MEMCG_LRU_HEAD;
 * 2. The first attempt to reclaim a memcg below low, which triggers
 *    MEMCG_LRU_TAIL;
 * 3. The first attempt to reclaim a memcg offlined or below reclaimable size
 *    threshold, which triggers MEMCG_LRU_TAIL;
 * 4. The second attempt to reclaim a memcg offlined or below reclaimable size
 *    threshold, which triggers MEMCG_LRU_YOUNG;
 * 5. Attempting to reclaim a memcg below min, which triggers MEMCG_LRU_YOUNG;
 * 6. Finishing the aging on the eviction path, which triggers MEMCG_LRU_YOUNG;
 * 7. Offlining a memcg, which triggers MEMCG_LRU_OLD.
 *
 * Notes:
 * 1. Memcg LRU only applies to global reclaim, and the round-robin incrementing
 *    of their max_seq counters ensures the eventual fairness to all eligible
 *    memcgs. For memcg reclaim, it still relies on mem_cgroup_iter().
 * 2. There are only two valid generations: old (seq) and young (seq+1).
 *    MEMCG_NR_GENS is set to three so that when reading the generation counter
 *    locklessly, a stale value (seq-1) does not wraparound to young.
 */
#define MEMCG_NR_GENS	3
#define MEMCG_NR_BINS	8
```


- 到底是 lruvec::lists ，还是在 lru_gen_memcg::fifo 中

## walk_mm 是做什么的?

两个问题:
```c
#ifdef CONFIG_LRU_GEN_WALKS_MMU
		struct {
			/* this mm_struct is on lru_gen_mm_list */
			struct list_head list;
			/*
			 * Set when switching to this mm_struct, as a hint of
			 * whether it has been used since the last time per-node
			 * page table walkers cleared the corresponding bits.
			 */
			unsigned long bitmap;
#ifdef CONFIG_MEMCG
			/* points to the memcg of "owner" above */
			struct mem_cgroup *memcg;
#endif
		} lru_gen;
#endif /* CONFIG_LRU_GEN_WALKS_MMU */
	} __randomize_layout;
```

想一想这个问题:
```txt
sudo bpftrace -e "kprobe:isolate_lru_folios { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    isolate_lru_folios+1
    shrink_lruvec+2247
    shrink_node+534
    do_try_to_free_pages+305
    try_to_free_mem_cgroup_pages+430
    try_charge_memcg+735
    mem_cgroup_swapin_charge_folio+193
    __read_swap_cache_async+417
    swap_cluster_readahead+551
    swapin_readahead+112
    do_swap_page+422
    handle_mm_fault+2086
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 9383
```

## 把 lru gen 关闭，应该就没有这个了吧

lru_gen_shrink_node

- `try_to_inc_max_seq`

## what
结构是什么样子的

- shorthand helpers
- Bloom filters
- mm_struct list
- PID controller
- the aging
- working set protection
- rmapPT walk feedback
- memcg LRU
- the eviction
- state change : 当使用模式变化
- sysfs interface
- debugfs interface

有趣的 pid 机制: tools/thermal/tmon/pid.c

## 关键问题，一次会 isolate_folios 多少个


## 原来还有好几个 config 啊！
- CONFIG_LRU_GEN_WALKS_MMU

## 到底是按照什么单位来做 shrink 的

每一个 zone 都有两个 min low high 的

- lru_gen_shrink_node

mem_cgroup_iter 迭代所有的

## vmscan 将一个扫描一次需要多长时间

```txt
@[
    shrink_node+1
    do_try_to_free_pages+305
    try_to_free_mem_cgroup_pages+430 <- 在这里初始化的 struct scan_control ，其中 .nr_to_reclaim = max(nr_pages, SWAP_CLUSTER_MAX),
    try_charge_memcg+735
    mem_cgroup_swapin_charge_folio+193
    __read_swap_cache_async+417
    swap_cluster_readahead+551
    swapin_readahead+112
    do_swap_page+422
    handle_mm_fault+2086
    do_user_addr_fault+503
    exc_page_fault+137
    asm_exc_page_fault+38
]: 12144
```

扫描的数量就是经过这个 page table 吗?

## 看看宋老师的 blog 了
https://mp.weixin.qq.com/s/f-DBVzDN_dzpADGlvPFfjQ

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
