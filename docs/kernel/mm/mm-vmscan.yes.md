## mm vmscan
## cgroup 将 lruvec 按照 node 切分的

cgroup 将每一个 node 一个 ，两个函数基本相同:

```c
static struct lruvec *get_lruvec(struct mem_cgroup *memcg, int nid)
{
	struct pglist_data *pgdat = NODE_DATA(nid);

#ifdef CONFIG_MEMCG
	if (memcg) {
		struct lruvec *lruvec = &memcg->nodeinfo[nid]->lruvec;

		/* see the comment in mem_cgroup_lruvec() */
		if (!lruvec->pgdat)
			lruvec->pgdat = pgdat;

		return lruvec;
	}
#endif
	VM_WARN_ON_ONCE(!mem_cgroup_disabled());

	return &pgdat->__lruvec;
}
```

```c
/**
 * mem_cgroup_lruvec - get the lru list vector for a memcg & node
 * @memcg: memcg of the wanted lruvec
 * @pgdat: pglist_data
 *
 * Returns the lru list vector holding pages for a given @memcg &
 * @pgdat combination. This can be the node lruvec, if the memory
 * controller is disabled.
 */
static inline struct lruvec *mem_cgroup_lruvec(struct mem_cgroup *memcg,
					       struct pglist_data *pgdat)
{
	struct mem_cgroup_per_node *mz;
	struct lruvec *lruvec;

	if (mem_cgroup_disabled()) {
		lruvec = &pgdat->__lruvec;
		goto out;
	}

	if (!memcg)
		memcg = root_mem_cgroup;

	mz = memcg->nodeinfo[pgdat->node_id];
	lruvec = &mz->lruvec;
out:
	/*
	 * Since a node can be onlined after the mem_cgroup was created,
	 * we have to be prepared to initialize lruvec->pgdat here;
	 * and if offlined then reonlined, we need to reinitialize it.
	 */
	if (unlikely(lruvec->pgdat != pgdat))
		lruvec->pgdat = pgdat;
	return lruvec;
}
```

- [ ] 但是 shrink_zones 的参数是 struct zonelist ，这其实不太科学啊


## cgroup 通过 lruvec 来分析链表

```c
struct lruvec {
	struct list_head		lists[NR_LRU_LISTS];
```

## zone 的控制和 cgroup 是分开的
- mem_cgroup_lruvec

如果是 cgroup 的 swap :
```txt
@[
    pageout+1
    shrink_folio_list+2137
    shrink_inactive_list+371
    shrink_lruvec+693
    shrink_node_memcgs+270
    shrink_node+174
    shrink_zones.constprop.0+133
    do_try_to_free_pages+90
    try_to_free_mem_cgroup_pages+283 // [^1]
    try_charge_memcg+423
    mem_cgroup_swapin_charge_folio+102
    __read_swap_cache_async+501
    swap_cluster_readahead+340
    do_swap_page+1243
    __handle_mm_fault+1426
    handle_mm_fault+430
    exc_page_fault+492
    asm_exc_page_fault+34
]: 216556
```
[^1]: 如果是 cgroup charge 导致的，那么在 try_to_free_mem_cgroup_pages 中会 scan_control::target_mem_cgroup


如果是 global 的 swap :
```txt
@[
    pageout+1
    shrink_folio_list+2137
    shrink_inactive_list+371
    shrink_lruvec+693
    shrink_node_memcgs+270
    shrink_node+174
    balance_pgdat+645
    kswapd+231
    kthread+201
    ret_from_fork+45
    ret_from_fork_asm+27
]: 911523
```

- ret_from_fork_asm
  - ret_from_fork
    - kthread
      - kswapd
        - balance_pgdat
          - kswapd_shrink_node
            - shrink_node
              - shrink_node_memcgs
                - shrink_lruvec


- asm_exc_page_fault
  - exc_page_fault
    - handle_page_fault
      - do_user_addr_fault
        - handle_mm_fault
          - __handle_mm_fault
              - do_pte_missing
                - do_anonymous_page
                  - mem_cgroup_charge
                    - __mem_cgroup_charge
                      - charge_memcg
                        - try_charge
                          - try_charge_memcg
                            - try_to_free_mem_cgroup_pages
                              - do_try_to_free_pages
                                - shrink_zones
                                  - shrink_node
                                    - shrink_node_memcgs
                                      - shrink_lruvec


## [x] 多个 lruvec 排序的

显然，我们没有办法对于各个 cgroup 中的 lru 排序，甚至，我们没有办法对于
同一个 cgroup 的 page 做完整的排序，因为不同的属性的 page 在不同的链表中。

在 shrink_lruvec -> get_scan_count 中计算了一次遍历的时候，如何从每一个 cgroup 中取多少 page 出来。

是的，就是没有更好的办法。

## zone_reclaim_mode

这个参数控制，如果内存不足的时候， 是先回收本地的 zone 的，还是直接从其他的 zone 中分配

/proc/sys/vm/zone_reclaim_mode

- https://docs.kernel.org/admin-guide/sysctl/vm.html

```c
/*
 * Node reclaim mode
 *
 * If non-zero call node_reclaim when the number of free pages falls below
 * the watermarks.
 */
int node_reclaim_mode __read_mostly;
```

控制了在其他的 node 中分配 page 是否需要开始在本 node 中

```c
/*
 * These bit locations are exposed in the vm.zone_reclaim_mode sysctl
 * ABI.  New bits are OK, but existing bits can never change.
 */
#define RECLAIM_ZONE	(1<<0)	/* Run shrink_inactive_list on the zone */
#define RECLAIM_WRITE	(1<<1)	/* Writeout pages during reclaim */
#define RECLAIM_UNMAP	(1<<2)	/* Unmap pages during reclaim */
```

## 即使是 mapcount 是 1 ，也是需要 rmap 的

在 shrink_folio_list 中，显然，是可以的:

```c
		if (folio_needs_release(folio)) {
			if (!filemap_release_folio(folio, sc->gfp_mask))
				goto activate_locked;
			if (!mapping && folio_ref_count(folio) == 1) {
				folio_unlock(folio);
				if (folio_put_testzero(folio))
					goto free_it;
				else {
					/*
					 * rare race with speculative reference.
					 * the speculative reference will free
					 * this folio shortly, so we may
					 * increment nr_reclaimed here (and
					 * leave it off the LRU).
					 */
					nr_reclaimed += nr_pages;
					continue;
				}
			}
		}
```

但是无论如何，在 `folio_referenced` 中都是需要使用 rmap_walk 的，
这是为了通过 folio 找到 pte 的。这么看，开销还是很大的。

## 参考
- [wowotech : 简单介绍](http://www.wowotech.net/memory_management/page_reclaim_basic.html)
- [wowotech : 原理简介](http://www.wowotech.net/memory_management/233.html)

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
