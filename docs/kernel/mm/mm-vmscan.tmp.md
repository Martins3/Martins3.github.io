## 构建一个基本调用流程

- try_to_free_pages
  - do_try_to_free_pages
    - shrink_node
      - shrink_node_memcgs : 对于 cgroupo tree 逐层向下
        - shrink_lruvec + shrink_slab
          - shrink_list
            - shrink_inactive_list
              - too_many_isolated
              - isolate_lru_folios
              - shrink_folio_list
                - folio_check_dirty_writeback : 似乎用于阻塞执行，具体检查下
                  - [ ] 内含一堆 flags 判断，理解下
                - folio_check_references
                  - [ ] 理解下其返回值 enum folio_references
                - pageout : 将 dirty page 写回
                  - [ ] 将 clean 的 page 释放的在什么位置
                        - shrink_active_list : 如果 inactive_is_low 的时候，收缩 active list

- mem_cgroup_soft_reclaim : 从 memcontrl 中触发的，但是并不是总是会从哪里触发
  - mem_cgroup_shrink_node
    - shrink_lruvec

## 细节 TO
```c
	/*
	 * Tag a node/memcg as congested if all the dirty pages were marked
	 * for writeback and immediate reclaim (counted in nr.congested).
	 *
	 * Legacy memcg will stall in page writeback so avoid forcibly
	 * stalling in reclaim_throttle().
	 */
	if (sc->nr.dirty && sc->nr.dirty == sc->nr.congested) {
		if (cgroup_reclaim(sc) && writeback_throttling_sane(sc))
			set_bit(LRUVEC_CGROUP_CONGESTED, &target_lruvec->flags);

		if (current_is_kswapd())
			set_bit(LRUVEC_NODE_CONGESTED, &target_lruvec->flags);
	}
```

## TODO 3 : shrink writeback 以及 compaction
我发现 compaction 以及 shrink ，page writeback 都是两条路径，
主动触发和被动触发。

1. `__alloc_pages_direct_reclaim` => `__perform_reclaim` => try_to_free_pages : 通过 reclaim page 进行分配
  - shrink_zones
    - shrink_node
2. `__alloc_pages_direct_compact` => try_to_compact_pages : 通过 compaction 分配
    - compact_zone_order
      - compact_zone

- kswapd
  - balance_pgdat
    - kswapd_shrink_node
      - shrink_node

kcompactd
  - kcompactd_do_work
    - compact_zone

其实 page writeback 机制和另外两个其实有点不同，
1. 似乎没有 direct 和 indirect 的两种方式
2. 使用的是 workqueue

- [ ] 似乎 page writeback 会写回，但是 shrink 的过程也会写回
  - 会出现这个 thread 同时在写回一个 page 吗？
  - 还是 page writeback 会首先 isolate 这些 page

- vmscan.c 中存在 compaction_ready 的

## TODO 4 : swappiness 的影响

- [ ] 当内存出现压力的时候，这三个的清理顺序是什么?
1. dirty file mapped
2. clean file mappped
3. unmapped 的文件

## TODO 5 : THP 是如何回收的
- [ ] 所以 shmem 的内存是如何被回收的
    - [ ] 将 shmem 的内存当做 swap cache ?
    - [ ] super_operations::nr_cached_objects 用于处理 transparent_hugepage

## TODO 6
https://lwn.net/Articles/550463/

## scan_control
```c
struct scan_control {
    /* How many pages shrink_list() should reclaim */
    unsigned long nr_to_reclaim;

    /* This context's GFP mask */
    gfp_t gfp_mask;

    /* Allocation order */
    int order;

    /*
     * Nodemask of nodes allowed by the caller. If NULL, all nodes
     * are scanned.
     */
    nodemask_t  *nodemask;

    /*
     * The memory cgroup that hit its limit and as a result is the
     * primary target of this reclaim invocation.
     */
    struct mem_cgroup *target_mem_cgroup;

    /* Scan (total_size >> priority) pages at once */
    int priority;

    /* The highest zone to isolate pages for reclaim from */
    enum zone_type reclaim_idx;

    /* Writepage batching in laptop mode; RECLAIM_WRITE */
    unsigned int may_writepage:1;

    /* Can mapped pages be reclaimed? */
    unsigned int may_unmap:1;

    /* Can pages be swapped as part of reclaim? */
    unsigned int may_swap:1;

    /*
     * Cgroups are not reclaimed below their configured memory.low,
     * unless we threaten to OOM. If any cgroups are skipped due to
     * memory.low and nothing was reclaimed, go back for memory.low.
     */
    unsigned int memcg_low_reclaim:1;
    unsigned int memcg_low_skipped:1;

    unsigned int hibernation_mode:1;

    /* One of the zones is ready for compaction */
    unsigned int compaction_ready:1;

    /* Incremented by the number of inactive pages that were scanned */
    unsigned long nr_scanned;

    /* Number of pages freed so far during a call to shrink_zones() */
    unsigned long nr_reclaimed;
};
```
- nr_to_reclaim：需要回收的页面数量；
- gfp_mask：申请分配的掩码，用户申请页面时可以通过设置标志来限制调用底层文件系统或不允许读写存储设备，最终传递给 LRU 处理；
- order：申请分配的阶数值，最终期望内存回收后能满足申请要求；
- nodemask：内存节点掩码，空指针则访问所有的节点；
- priority：扫描 LRU 链表的优先级，用于计算每次扫描页面的数量(total_size >> priority，初始值 12)，值越小，扫描的页面数越大，逐级增加扫描粒度；
- may_writepage：是否允许把修改过文件页写回存储设备；
- may_unmap : 是否支持回收将 unmapped 的 page 进行回收
- may_swap
- nr_scanned：统计扫描过的非活动页面总数；
- nr_reclaimed：统计回收了的页面总数


## 问题
- 那么 swap cache 中间的 page 也会被 reclaim 吗 ?
  - 当然会
- mark_folio_accessed && folio_check_references
- 如果理解 folio_test_lru
  - 如果理解了，重新阅读下 folio_mark_accessed
- folio_referenced 中会使用 folio_raw_mapping 检查一下，我不知道什么时候 folio_raw_mapping 在这里返回为 NULL ，尤其是前面还检查过 map count ，难道说是 private anon page 吗? 如果是，那么也不应该放到
	if (!pra.mapcount) 后面检查啊

## lru_list

```c
enum lru_list {
	LRU_INACTIVE_ANON = LRU_BASE,
	LRU_ACTIVE_ANON = LRU_BASE + LRU_ACTIVE,
	LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,
	LRU_ACTIVE_FILE = LRU_BASE + LRU_FILE + LRU_ACTIVE,
	LRU_UNEVICTABLE,
	NR_LRU_LISTS
};
```
- folio_putback_lru : 将 lru 重新加入到 lru list 中
- folio_isolate_lru : 将 page 从 lru 中取出来，准备作为其他的用途

```txt
#0  folio_isolate_lru (folio=0xffffea0004800000) at mm/vmscan.c:2422
#1  0xffffffff813567a8 in isolate_lru_page (page=page@entry=0xffffea0004800000) at mm/folio-compat.c:118
#2  0xffffffff81412b3d in __collapse_huge_page_isolate (compound_pagelist=0xffffc90000857d10, cc=0xffffffff82d94180 <khugepaged_collapse_control>, pte=0xffff888108b77000, address=140018752397312, vma=0xffff8881041ce000) at mm/khugepaged.c:644
#3  collapse_huge_page (mm=mm@entry=0xffff88810674f380, address=address@entry=140018750324736, referenced=referenced@entry=6, unmapped=unmapped@entry=0, cc=cc@entry=0xffffffff82d94180 <khugepaged_collapse_control>) at mm/khugepaged.c:1177
#4  0xffffffff81413e63 in hpage_collapse_scan_pmd (mm=mm@entry=0xffff88810674f380, vma=vma@entry=0xffff8881041ce000, address=140018750324736, mmap_locked=mmap_locked@entry=0xffffc90000857e67, cc=cc@entry=0xffffffff82d94180 <khugepaged_collapse_control>) at mm/khugepaged.c:1406
#5  0xffffffff81416ef8 in khugepaged_scan_mm_slot (cc=0xffffffff82d94180 <khugepaged_collapse_control>, result=<synthetic pointer>, pages=2881) at mm/khugepaged.c:2523
#6  khugepaged_do_scan (cc=0xffffffff82d94180 <khugepaged_collapse_control>) at mm/khugepaged.c:2629
#7  khugepaged (none=<optimized out>) at mm/khugepaged.c:2685
#8  0xffffffff81170674 in kthread (_create=0xffff888103da51c0) at kernel/kthread.c:389
#9  0xffffffff810e39f1 in ret_from_fork (prev=<optimized out>, regs=0xffffc90000857f58, fn=0xffffffff81170580 <kthread>, fn_arg=0xffff888103da51c0) at arch/x86/kernel/process.c:145
#10 0xffffffff8100259b in ret_from_fork_asm () at arch/x86/entry/entry_64.S:304
```

请问这里的 isolation 和 folio_isolate_lru 是一个概念吗?
此外，和 compaction 中的 isolation 是一个概念吗?
```c
/*
 * A direct reclaimer may isolate SWAP_CLUSTER_MAX pages from the LRU list and
 * then get rescheduled. When there are massive number of tasks doing page
 * allocation, such sleeping direct reclaimers may keep piling up on each CPU,
 * the LRU list will go small and be scanned faster than necessary, leading to
 * unnecessary swapping, thrashing and OOM.
 */
static int too_many_isolated(struct pglist_data *pgdat, int file,
		struct scan_control *sc)
```
还是指的是仅仅调用 lruvec_del_folio 而已，恐怕说明的是 isolate_lru_folios 吧

只有 pagecache 才会走到这里吗?
```c
static unsigned long isolate_lru_pages(unsigned long nr_to_scan,
		struct lruvec *lruvec, struct list_head *dst,
		unsigned long *nr_scanned, struct scan_control *sc,
		isolate_mode_t mode, enum lru_list lru)
    // 1. 实现 lruvec->lists[lru] 中间取出
    #define lru_to_page(head) (list_entry((head)->prev, struct page, lru))
    // 2. 循环 __isolate_lru_page 实现真正的移除
```

但是观察，这三个的引用位置，感觉还是
```c
/* Isolate unmapped pages */
#define ISOLATE_UNMAPPED	((__force isolate_mode_t)0x2)
/* Isolate for asynchronous migration */
#define ISOLATE_ASYNC_MIGRATE	((__force isolate_mode_t)0x4)
/* Isolate unevictable pages */
#define ISOLATE_UNEVICTABLE	((__force isolate_mode_t)0x8)
```

- [ ] 所以如何体现各种 list 之间移动的 ? 以及设计的策略的 ?
  - 一个 page 被设置为 page cache 就立刻加入的吗？
  - anon page
  - mapped page

### lruvec

- move_folios_to_lru 被 shrink_inactive_list 唯一调用
  - 将存放到 struct lruvec 中的 folio 的移动到 lru list 中

- mem_cgroup_lruvec 返回当前 memcg 中的 lruvec
  - 例如 shrink_node_memcgs

向 lruvec 加入和减去 page 的时候
- isolate_lru_folios : 将 page 从 lruvec 中取出来，然后分析是否应该放到
- lruvec_add_folio_tail / lruvec_del_folio

- [memcg lru lock 血泪史](https://mp.weixin.qq.com/s/7eDqHR06TIBh6hqUMTrZKg)

### cpu_fbatches

- [ ] 和 lru_list.c 的关系是什么?

- folio_batch_add_and_move 参数为 lru_add_fn
  - folio_batch_move_lru
    - 调用 hook : lru_add_fn
      - lruvec_add_folio

- [x] 通过 folio 找到 lruvec 的方法
  - folio 可以找到自己在哪一个 memcg 中的，进而找到 lruvec

- lru_cache_add_inactive_or_unevictable
- add_to_page_cache_lru

Usually a page is first regarded as inactive and has to earn its merits to be considered active. However, a
selected number of procedures have a high opinion of their pages and invoke `lru_cache_add_active` to
place pages directly on the zone’s active list:
1. `read_swap_cache_async` from mm/swap_state.c; this reads pages from the swap cache.
2. The page fault handlers `__do_fault`, `do_anonymous_page`, `do_wp_page`, and `do_no_page`; these are implemented in mm/memory.c.

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
