# workingset

参考内容:
- [ ] 奔跑吧 P315 5.3.11

## PG_workingset

```c
PAGEFLAG(Workingset, workingset, PF_HEAD)
	TESTCLEARFLAG(Workingset, workingset, PF_HEAD)
```

可以展开为:

```c
static __always_inline bool folio_test_workingset(struct folio *folio) {
  return test_bit(PG_workingset, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline int PageWorkingset(struct page *page) {
  return test_bit(PG_workingset, &PF_HEAD(page, 0)->flags);
}
static __always_inline void folio_set_workingset(struct folio *folio) {
  set_bit(PG_workingset, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline void SetPageWorkingset(struct page *page) {
  set_bit(PG_workingset, &PF_HEAD(page, 1)->flags);
}
static __always_inline void folio_clear_workingset(struct folio *folio) {
  clear_bit(PG_workingset, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline void ClearPageWorkingset(struct page *page) {
  clear_bit(PG_workingset, &PF_HEAD(page, 1)->flags);
}

static __always_inline bool folio_test_clear_workingset(struct folio *folio) {
  return test_and_clear_bit(PG_workingset, folio_flags(folio, FOLIO_PF_HEAD));
}
static __always_inline int TestClearPageWorkingset(struct page *page) {
  return test_and_clear_bit(PG_workingset, &PF_HEAD(page, 1)->flags);
}
```

## workingset_refault

## working set
- folio_mark_accessed 唯一调用 workingset_activation


分析一下:
```c
struct lruvec {
	struct list_head		lists[NR_LRU_LISTS];
	/* per lruvec lru_lock for memcg */
	spinlock_t			lru_lock;
	/*
	 * These track the cost of reclaiming one LRU - file or anon -
	 * over the other. As the observed cost of reclaiming one LRU
	 * increases, the reclaim scan balance tips toward the other.
	 */
	unsigned long			anon_cost;
	unsigned long			file_cost;
	/* Non-resident age, driven by LRU movement */
	atomic_long_t			nonresident_age;
	/* Refaults at the time of last reclaim cycle */
	unsigned long			refaults[ANON_AND_FILE];
	/* Various lruvec state flags (enum lruvec_flags) */
	unsigned long			flags;
#ifdef CONFIG_LRU_GEN
	/* evictable pages divided into generations */
	struct lru_gen_struct		lrugen;
	/* to concurrently iterate lru_gen_mm_list */
	struct lru_gen_mm_state		mm_state;
#endif
#ifdef CONFIG_MEMCG
	struct pglist_data *pgdat;
#endif
};
```
- [ ] cost

- [ ] nonresident_age
- [ ] refaults

- pgdat 为什么只有在 working 的时候才需要，因为每一个函数中本来就存在:
```c
static inline struct pglist_data *lruvec_pgdat(struct lruvec *lruvec)
{
#ifdef CONFIG_MEMCG
	return lruvec->pgdat;
#else
	return container_of(lruvec, struct pglist_data, __lruvec);
#endif
}
```

## snapshot_refaults

- do_try_to_free_pages
  - shrink_zones
    - shrink_node
      - prepare_scan_count 检查数值是否相等
  - snapshot_refaults ：将 `lruvec_page_state` 中的保存下来

- balance_pgdat
  - kswapd_shrink_node : 最后调用到 prepare_scan_count 中
  - snapshot_refaults


如果在 prepare_scan_count 中，检测，然后设置 scan_control::may_deactivate，如果
是设置为 true，那么将会在 shrink_list 中调用 shrink_active_list 的，否则只是调用 shrink_inactive_list

```c
	/*
	 * Target desirable inactive:active list ratios for the anon
	 * and file LRU lists.
	 */
	if (!sc->force_deactivate) {
		unsigned long refaults;

		/*
		 * When refaults are being observed, it means a new
		 * workingset is being established. Deactivate to get
		 * rid of any stale active pages quickly.
		 */
		refaults = lruvec_page_state(target_lruvec,
				WORKINGSET_ACTIVATE_ANON);
		if (refaults != target_lruvec->refaults[WORKINGSET_ANON] ||
			inactive_is_low(target_lruvec, LRU_INACTIVE_ANON))
			sc->may_deactivate |= DEACTIVATE_ANON;
		else
			sc->may_deactivate &= ~DEACTIVATE_ANON;

		refaults = lruvec_page_state(target_lruvec,
				WORKINGSET_ACTIVATE_FILE);
		if (refaults != target_lruvec->refaults[WORKINGSET_FILE] ||
		    inactive_is_low(target_lruvec, LRU_INACTIVE_FILE))
			sc->may_deactivate |= DEACTIVATE_FILE;
		else
			sc->may_deactivate &= ~DEACTIVATE_FILE;
	} else
		sc->may_deactivate = DEACTIVATE_ANON | DEACTIVATE_FILE;
```

-----------> 理解一下 lruvec_page_state 的含义是什么

- lruvec_page_state 获取此处的数据
```c
struct lruvec_stats {
	/* Aggregated (CPU and subtree) state */
	long state[NR_VM_NODE_STAT_ITEMS];

	/* Pending child counts during tree propagation */
	long state_pending[NR_VM_NODE_STAT_ITEMS];
};
```

flush 的过程是因为统计数据需要向上汇总，从而实现 memcg_vmstats 和 memcg_vmstats_percpu 将数据调用进来的:
```txt
#0  mem_cgroup_css_rstat_flush (css=0xffff88816a1ab000, cpu=0) at include/linux/memcontrol.h:852
#1  0xffffffff811d1d92 in cgroup_rstat_flush_locked (cgrp=cgrp@entry=0xffffffff82b54d90 <cgrp_dfl_root+16>, may_sleep=may_sleep@entry=false) at kernel/cgroup/rstat.c:205
#2  0xffffffff811d1e9f in cgroup_rstat_flush_irqsafe (cgrp=0xffffffff82b54d90 <cgrp_dfl_root+16>) at kernel/cgroup/rstat.c:254
#3  0xffffffff813441e6 in __mem_cgroup_flush_stats () at mm/memcontrol.c:635
#4  0xffffffff81347d9a in mem_cgroup_flush_stats () at mm/memcontrol.c:643
#5  0xffffffff812afa64 in prepare_scan_count (sc=0xffffc900017efc68, pgdat=0xffff88823fff9000) at mm/vmscan.c:2787
#6  shrink_node (pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc900017efc68) at mm/vmscan.c:6103
#7  0xffffffff812b0e10 in shrink_zones (sc=0xffffc900017efc68, zonelist=<optimized out>) at mm/vmscan.c:6343
#8  do_try_to_free_pages (zonelist=zonelist@entry=0xffff88823fffaa00, sc=sc@entry=0xffffc900017efc68) at mm/vmscan.c:6405
```

通过如下窗口函数，来调用 memcg_vmstats 和 memcg_vmstats_percpu
- `__mod_memcg_lruvec_state`
- `__mod_lruvec_state`
- `__mod_lruvec_page_state`
- `__mod_lruvec_kmem_state`

## workingset_refault

```txt
#0  workingset_refault (folio=folio@entry=0xffffea00060c3b40, shadow=0x245001101) at arch/x86/include/asm/bitops.h:206
#1  0xffffffff8130886e in __read_swap_cache_async (entry=entry@entry=..., gfp_mask=gfp_mask@entry=1051850, vma=vma@entry=0xffff88816fd49000, addr=addr@entry=139841821999104, new_page_allocated=new_page_allocated@entry=0xffffc900017efd06) at mm/swap_state.c:496
#2  0xffffffff81308a86 in swap_cluster_readahead (entry=..., gfp_mask=gfp_mask@entry=1051850, vmf=vmf@entry=0xffffc900017efdf8) at mm/swap_state.c:641
#3  0xffffffff81308fcf in swapin_readahead (entry=..., entry@entry=..., gfp_mask=gfp_mask@entry=1051850, vmf=vmf@entry=0xffffc900017efdf8) at mm/swap_state.c:855
#4  0xffffffff812d834c in do_swap_page (vmf=vmf@entry=0xffffc900017efdf8) at mm/memory.c:3822
#5  0xffffffff812dcb2c in handle_pte_fault (vmf=0xffffc900017efdf8) at mm/memory.c:4959
#6  __handle_mm_fault (vma=vma@entry=0xffff88816fd49000, address=address@entry=139841821999104, flags=flags@entry=596) at mm/memory.c:5097
#7  0xffffffff812dd600 in handle_mm_fault (vma=0xffff88816fd49000, address=address@entry=139841821999104, flags=flags@entry=596, regs=regs@entry=0xffffc900017eff58) at mm/memory.c:5218
#8  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc900017eff58, error_code=error_code@entry=4, address=address@entry=139841821999104) at arch/x86/mm/fault.c:1428
#9  0xffffffff81fa7e22 in handle_page_fault (address=139841821999104, error_code=4, regs=0xffffc900017eff58) at arch/x86/mm/fault.c:1519
#10 exc_page_fault (regs=0xffffc900017eff58, error_code=4) at arch/x86/mm/fault.c:1575
#11 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#12 0x0000000000000008 in fixed_percpu_data ()
```

其中会调用的

```c
mod_lruvec_state(lruvec, WORKINGSET_ACTIVATE_BASE + file, nr);
```

## workingset_eviction
```txt
#0  workingset_eviction (folio=folio@entry=0xffffea00060c7fc0, target_memcg=target_memcg@entry=0xffff88816a1ab000) at mm/workingset.c:353
#1  0xffffffff812ab66d in __remove_mapping (mapping=0xffff8881229ac000, folio=folio@entry=0xffffea00060c7fc0, reclaimed=reclaimed@entry=true, target_memcg=0xffff88816a1ab000) at mm/vmscan.c:1351
#2  0xffffffff812adb6a in shrink_folio_list (folio_list=folio_list@entry=0xffffc900017ef928, pgdat=pgdat@entry=0xffff88823fff9000, sc=sc@entry=0xffffc900017efb00, stat=stat@entry=0xffffc900017ef9b0, ignore_references=ignore_references@entry=false) at mm/vmscan.c:2016
#3  0xffffffff812af3b8 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc900017efb00, lruvec=0xffff88816a18d800, nr_to_scan=<optimized out>) at mm/vmscan.c:2489
#4  shrink_list (sc=0xffffc900017efb00, lruvec=0xffff88816a18d800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2716
```

- `__read_swap_cache_async`
  - add_to_swap_cache
  - workingset_refault
