# vmscan.c

## Key Question

## 记住
1. 用户的所有的物理页面都会被 vma 管理的，被vma 管理，那么就一定会有 pte
  1. fread 读到的目的地也是 brk 分配出来的 heap 空间
  2. fread 和 fwrite 的操作就像是 : 直接在磁盘上一样，根本没有任何 explicit page frame ，这些形成 page cache 不是用户的页面

2. 所有file 读取的结果都会经过 page cache

3. 当page 在 lrulist 中间的时候，page 的会持有 LRU 的 flag

1. page_mapped 访问获取 `page->_refcount` 和 page_referenced 的区别 ?
    1. page_mapped 表示具体被映射的次数，但是
    2.

4. 被 shrink_page_list 唯一调用的三个函数
```c
/*
 * pageout is called by shrink_page_list() for each dirty page.
 * Calls ->writepage().
 */
static pageout_t pageout(struct page *page, struct address_space *mapping,
			 struct scan_control *sc)

static enum page_references page_check_references(struct page *page,
						  struct scan_control *sc)

/* Check if a page is dirty or under writeback */
static void page_check_dirty_writeback(struct page *page,
				       bool *dirty, bool *writeback)
```

## 问题
2. page 分类:
    1. anon file :
    2. page cache 的内容是内核页面: file map 管理，page cache 也是其中的

那么 swap cache 中间的page 也会被 reclaim 吗 ?

**到底如何回收 page cache 中间的 page**

1. 开始猜测，vmscan 是不是只能仅仅 scan 其中 map 的内容 ? 证据如下:
    1. page_references 参数含有 vm_flags
    2. lruvec 处理的类型 : anon + file 似乎是
    3. page cache 中间保存的page 是对于上层隐藏的吗 ?
        1. page cache 中间的页面 和 file vma 的物理页面 是必须拷贝的吗 ? (应该不会拷贝)
        2. 同样的，文件IO 也不会对其进行 从 page cache 到 brk 的拷贝

2. 开始怀疑: page cache 中间的物理页面是内核管理的 和　用户 file vma 映射的物理页面　是完全分开的.
    1. memory.c : do_fault 似乎并不同意(@todo 阅读一下plka)

> 1. scan_control 的内容 ?
> 2. 所以如何体现各种list 之间移动的 ? 以及设计的策略的 ?

4. shrinker 和 slab 机制的关系是什么 ?

5. mark_page_accessed 和 page_references 的操作诡异之极？

6. 各种各样的蛇皮 page flags (reserved 等等) 的作用是什么 ?

7. page_referenced 和 active 来实现切换的原理好玄学啊 !

8. page 应该是在创建的时候就会被加入到 lru 中间 ?
    1. file : add_to_page_cache_lru
    2. anon : 不知道在什么地方 ?

9. kswapd 的可以分析一下。

10. 此处调用了一生之敌 compaction 的内容。

## ref && doc
[函数描述](http://www.wowotech.net/memory_management/233.html)

2. [基本概念](http://www.wowotech.net/memory_management/page_reclaim_basic.html)

## TODO

```c
/*
 * swapper_space is a fiction, retained to simplify the path through
 * vmscan's shrink_page_list.
 */

 // todo 如果不定义， shrink_page_list 的写法修改如果!
 // 这个 address_space_operations 所挂载的 address 不会在inode 之类的上，想要访问必须持有 swp_entry_t
 // 既然如此，那么就不需要这一个封装了
static const struct address_space_operations swap_aops = {
	.writepage	= swap_writepage,
	.set_page_dirty	= swap_set_page_dirty,
#ifdef CONFIG_MIGRATION
	.migratepage	= migrate_page,
#endif
};
```

```c
// shrinker 的 register alloc 等工作
// TODO 所以哪里用过shrinker ? 扫描和释放不是固定的算法吗?
```

```c
// 莫名奇妙的出现
static enum page_references page_check_references(struct page *page,
						  struct scan_control *sc)
```

## memcg 为什么需要参与进来



## page reference
1. 为什么使用 PG_referenced 和 PG_active 两个标志位 ?
    1. 感觉像是 : 2bit 的分支预测器
2. mark_page_accessed :
    1. 就像是命中之后数值加1 的操作 : 其中 active 和 reference 分别为较高和较低两位
    2. 其对称的，反向的操作是什么 ? 是不是就是 inactive list 中间的被删除光了，然后就从 active list 中间去掉部分 ?
    3. 从 inactive 到 active 的转移 : 全部转移 ? 部分转移 ?
        1. 既然是不够使用就开始转移，那么构建两个 list 的作用是什么 ? 这样就有了两个队列头
```c
enum page_references {
	PAGEREF_RECLAIM,
	PAGEREF_RECLAIM_CLEAN,
	PAGEREF_KEEP,
	PAGEREF_ACTIVATE,
};

static enum page_references page_check_references(struct page *page, // 被 shrink_page_list 唯一访问 // todo 其作用如何 ?
						  struct scan_control *sc)
{
	int referenced_ptes, referenced_page;
	unsigned long vm_flags;

	referenced_ptes = page_referenced(page, 1, sc->target_mem_cgroup,
					  &vm_flags); // 似乎是用于计算 page referenced 数量，需要利用 rmap 机制
	referenced_page = TestClearPageReferenced(page); // 标准 bit 操作
  // todo 下面存在一堆各种，关于 page flags 的检查，得到 enum page_references 的结果
  // 但是不知道其中的逻辑是什么 ?
```

```c
/* swap.c */
/*
 * Mark a page as having seen activity.
 *
 * inactive,unreferenced	->	inactive,referenced
 * inactive,referenced		->	active,unreferenced
 * active,unreferenced		->	active,referenced
 *
 * When a newly allocated page is not yet visible, so safe for non-atomic ops,
 * __SetPageReferenced(page) may be substituted for mark_page_accessed(page).
 */
void mark_page_accessed(struct page *page)
{
	page = compound_head(page);
	if (!PageActive(page) && !PageUnevictable(page) &&
			PageReferenced(page)) {

		/*
		 * If the page is on the LRU, queue it for activation via
		 * activate_page_pvecs. Otherwise, assume the page is on a
		 * pagevec, mark it active and it'll be moved to the active
		 * LRU on the next drain.
		 */
		if (PageLRU(page))  // 分类处理在 LRU 上和在 pagevec 上两种情况
			activate_page(page);
		else
			__lru_cache_activate_page(page);
		ClearPageReferenced(page);

		if (page_is_file_cache(page)) // todo ???? 这是啥 ?
			workingset_activation(page);
	} else if (!PageReferenced(page)) {
    // 将 active unreferenced 转化为 active referenced
    // todo 为什么 SetPageReferenced 没有将 page 从队列开始的位置放到结束的位置
    // 是不是 : lru list 其实先后并不会反映访问时间的先后
		SetPageReferenced(page);
	}
	if (page_is_idle(page))
		clear_page_idle(page);
}
```

> PG_referenced以及由逆向映射提供的信息用来确定页面活动程度，每次清除该位时，都会检测页面活动程度，page_referenced函数实现了该行为；


```c
/**
 * page_referenced - test if the page was referenced
 * @page: the page to test
 * @is_locked: caller holds lock on the page
 * @memcg: target memory cgroup
 * @vm_flags: collect encountered vma->vm_flags who actually referenced the page
 *
 * Quick test_and_clear_referenced for all mappings to a page,
 * returns the number of ptes which referenced the page.
 */
int page_referenced(struct page *page,
		    int is_locked,
		    struct mem_cgroup *memcg,
		    unsigned long *vm_flags)
{
	int we_locked = 0;
	struct page_referenced_arg pra = {
		.mapcount = total_mapcount(page),
		.memcg = memcg,
	};
	struct rmap_walk_control rwc = {
		.rmap_one = page_referenced_one, // todo 重新分析一下
		.arg = (void *)&pra,
		.anon_lock = page_lock_anon_vma_read,
	};

	*vm_flags = 0;
	if (!page_mapped(page)) // 利用 page->_mapcount 确定
		return 0;

	if (!page_rmapping(page)) // 测试 page->mapping 是不是为 NULL
		return 0;
  // todo 也就是说只要page 被映射，那么上面两个变量都必须标记上，
  // 那么对于page cache 中间不被 vma 映射的也会用到这两个变量吗(比如读一个文件形成的 page cache)
  // 除非，调用此函数分析的 page 只有被 map 的page


	if (!is_locked && (!PageAnon(page) || PageKsm(page))) {
		we_locked = trylock_page(page);
		if (!we_locked)
			return 1;
	}

	/*
	 * If we are reclaiming on behalf of a cgroup, skip
	 * counting on behalf of references from different
	 * cgroups
	 */
	if (memcg) {
		rwc.invalid_vma = invalid_page_referenced_vma;
	}

	rmap_walk(page, &rwc); // 利用 rmap 分析一下结果
	*vm_flags = pra.vm_flags;

	if (we_locked)
		unlock_page(page);

	return pra.referenced;
}
```

## shrink_page_list : vmscan 的终极深渊
1. 将函数分段处理一下
2. 了解各种判断的 goto 的含义是什么 ?

整个函数，try_to_free_pages 的位置，都是想要确定到底数量是多少
```c
/*
 * shrink_page_list() returns the number of reclaimed pages
 */
static unsigned long shrink_page_list(struct list_head *page_list,
				      struct pglist_data *pgdat,
				      struct scan_control *sc,
				      enum ttu_flags ttu_flags,
				      struct reclaim_stat *stat,
				      bool force_reclaim)
{
	LIST_HEAD(ret_pages); // ret_pages 保存无法被 reclaim 的 page // todo 那些情况下，vma 无法被删除 ?
	LIST_HEAD(free_pages);
	int pgactivate = 0;
	unsigned nr_unqueued_dirty = 0;
	unsigned nr_dirty = 0;
	unsigned nr_congested = 0; // todo congested 和 writeback 的关系是什么 ?
	unsigned nr_reclaimed = 0;
	unsigned nr_writeback = 0;
	unsigned nr_immediate = 0;
	unsigned nr_ref_keep = 0;
	unsigned nr_unmap_fail = 0;

	cond_resched();

	while (!list_empty(page_list)) {
		struct address_space *mapping;
		struct page *page;
		int may_enter_fs;
		enum page_references references = PAGEREF_RECLAIM_CLEAN;
		bool dirty, writeback;

		cond_resched();

		page = lru_to_page(page_list);
		list_del(&page->lru);

		if (!trylock_page(page)) // 利用 PG_locked 将 page 锁住
			goto keep;

		VM_BUG_ON_PAGE(PageActive(page), page);

		sc->nr_scanned++;

		if (unlikely(!page_evictable(page)))
			goto activate_locked;

		if (!sc->may_unmap && page_mapped(page)) // todo sc->may_unmap 的含义是什么 ? page_mapped(page) 只是访问一下 _mapcount 而已
			goto keep_locked;

		/* Double the slab pressure for mapped and swapcache pages */
		if ((page_mapped(page) || PageSwapCache(page)) &&
		    !(PageAnon(page) && !PageSwapBacked(page)))　// PageSwapBacked 和 PageSwapCache 的含义是什么 ?
			sc->nr_scanned++;

		may_enter_fs = (sc->gfp_mask & __GFP_FS) ||
			(PageSwapCache(page) && (sc->gfp_mask & __GFP_IO));

		/*
		 * The number of dirty pages determines if a node is marked
		 * reclaim_congested which affects wait_iff_congested. kswapd
		 * will stall and start writing pages if the tail of the LRU
		 * is all dirty unqueued pages.
		 */
		page_check_dirty_writeback(page, &dirty, &writeback); // Check if a page is dirty or under writeback
		if (dirty || writeback)
			nr_dirty++;

		if (dirty && !writeback)
			nr_unqueued_dirty++;

		/*
		 * Treat this page as congested if the underlying BDI is or if
		 * pages are cycling through the LRU so quickly that the
		 * pages marked for immediate reclaim are making it to the
		 * end of the LRU a second time.
		 */
		mapping = page_mapping(page);
		if (((dirty || writeback) && mapping &&
		     inode_write_congested(mapping->host)) ||
		    (writeback && PageReclaim(page)))
			nr_congested++;

		/*
		 * If a page at the tail of the LRU is under writeback, there
		 * are three cases to consider.
		 *
		 * 1) If reclaim is encountering an excessive number of pages
		 *    under writeback and this page is both under writeback and
		 *    PageReclaim then it indicates that pages are being queued
		 *    for IO but are being recycled through the LRU before the
		 *    IO can complete. Waiting on the page itself risks an
		 *    indefinite stall if it is impossible to writeback the
		 *    page due to IO error or disconnected storage so instead
		 *    note that the LRU is being scanned too quickly and the
		 *    caller can stall after page list has been processed.
		 *
		 * 2) Global or new memcg reclaim encounters a page that is
		 *    not marked for immediate reclaim, or the caller does not
		 *    have __GFP_FS (or __GFP_IO if it's simply going to swap,
		 *    not to fs). In this case mark the page for immediate
		 *    reclaim and continue scanning.
		 *
		 *    Require may_enter_fs because we would wait on fs, which
		 *    may not have submitted IO yet. And the loop driver might
		 *    enter reclaim, and deadlock if it waits on a page for
		 *    which it is needed to do the write (loop masks off
		 *    __GFP_IO|__GFP_FS for this reason); but more thought
		 *    would probably show more reasons.
		 *
		 * 3) Legacy memcg encounters a page that is already marked
		 *    PageReclaim. memcg does not have any dirty pages
		 *    throttling so we could easily OOM just because too many
		 *    pages are in writeback and there is nothing else to
		 *    reclaim. Wait for the writeback to complete.
		 *
		 * In cases 1) and 2) we activate the pages to get them out of
		 * the way while we continue scanning for clean pages on the
		 * inactive list and refilling from the active list. The
		 * observation here is that waiting for disk writes is more
		 * expensive than potentially causing reloads down the line.
		 * Since they're marked for immediate reclaim, they won't put
		 * memory pressure on the cache working set any longer than it
		 * takes to write them to disk.
		 */
		if (PageWriteback(page)) {
			/* Case 1 above */
			if (current_is_kswapd() &&
			    PageReclaim(page) &&
			    test_bit(PGDAT_WRITEBACK, &pgdat->flags)) {
				nr_immediate++;
				goto activate_locked;

			/* Case 2 above */
			} else if (sane_reclaim(sc) ||
			    !PageReclaim(page) || !may_enter_fs) {
				/*
				 * This is slightly racy - end_page_writeback()
				 * might have just cleared PageReclaim, then
				 * setting PageReclaim here end up interpreted
				 * as PageReadahead - but that does not matter
				 * enough to care.  What we do want is for this
				 * page to have PageReclaim set next time memcg
				 * reclaim reaches the tests above, so it will
				 * then wait_on_page_writeback() to avoid OOM;
				 * and it's also appropriate in global reclaim.
				 */
				SetPageReclaim(page);
				nr_writeback++;
				goto activate_locked;

			/* Case 3 above */
			} else {
				unlock_page(page);
				wait_on_page_writeback(page);
				/* then go back and try same page again */
				list_add_tail(&page->lru, page_list);
				continue;
			}
		}

		if (!force_reclaim)
			references = page_check_references(page, sc);

		switch (references) {
		case PAGEREF_ACTIVATE:
			goto activate_locked;
		case PAGEREF_KEEP:
			nr_ref_keep++;
			goto keep_locked;
		case PAGEREF_RECLAIM:
		case PAGEREF_RECLAIM_CLEAN:
			; /* try to reclaim the page below */

      // 使用下面的分析是什么 ?
		}


    // 1. 处理 anon 并且有 backing store
		/*
		 * Anonymous process memory has backing store?
		 * Try to allocate it some swap space here.
		 * Lazyfree page could be freed directly
		 */
		if (PageAnon(page) && PageSwapBacked(page)) { // XXX
       // 如果都是 swap backed，
			if (!PageSwapCache(page)) {
				if (!(sc->gfp_mask & __GFP_IO))
					goto keep_locked;

				if (!add_to_swap(page)) {
					if (!add_to_swap(page))
						goto activate_locked; // 如果 add_to_swap 失败了
				}

				may_enter_fs = 1;

				/* Adding to swap updated mapping */
				mapping = page_mapping(page);
			}
		} else if (unlikely(PageTransHuge(page))) {
			/* Split file THP */
			if (split_huge_page_to_list(page, page_list))
				goto keep_locked;
		}

		/*
		 * The page is mapped into the page tables of one or more
		 * processes. Try to unmap it here.
		 */
		if (page_mapped(page)) {
			enum ttu_flags flags = ttu_flags | TTU_BATCH_FLUSH;

			if (unlikely(PageTransHuge(page)))
				flags |= TTU_SPLIT_HUGE_PMD;
			if (!try_to_unmap(page, flags)) {
				nr_unmap_fail++;
				goto activate_locked;
			}
		}

		if (PageDirty(page)) {
			/*
			 * Only kswapd can writeback filesystem pages
			 * to avoid risk of stack overflow. But avoid
			 * injecting inefficient single-page IO into
			 * flusher writeback as much as possible: only
			 * write pages when we've encountered many
			 * dirty pages, and when we've already scanned
			 * the rest of the LRU for clean pages and see
			 * the same dirty pages again (PageReclaim).
			 */
			if (page_is_file_cache(page) &&
			    (!current_is_kswapd() || !PageReclaim(page) ||
			     !test_bit(PGDAT_DIRTY, &pgdat->flags))) {
				/*
				 * Immediately reclaim when written back.
				 * Similar in principal to deactivate_page()
				 * except we already have the page isolated
				 * and know it's dirty
				 */
				inc_node_page_state(page, NR_VMSCAN_IMMEDIATE);
				SetPageReclaim(page);

				goto activate_locked;
			}

			if (references == PAGEREF_RECLAIM_CLEAN)
				goto keep_locked;
			if (!may_enter_fs)
				goto keep_locked;
			if (!sc->may_writepage)
				goto keep_locked;

			/*
			 * Page is dirty. Flush the TLB if a writable entry
			 * potentially exists to avoid CPU writes after IO
			 * starts and then write it out here.
			 */
			try_to_unmap_flush_dirty();
			switch (pageout(page, mapping, sc)) {
			case PAGE_KEEP:
				goto keep_locked;
			case PAGE_ACTIVATE:
				goto activate_locked;
			case PAGE_SUCCESS:
				if (PageWriteback(page))
					goto keep;
				if (PageDirty(page))
					goto keep;

				/*
				 * A synchronous write - probably a ramdisk.  Go
				 * ahead and try to reclaim the page.
				 */
				if (!trylock_page(page))
					goto keep;
				if (PageDirty(page) || PageWriteback(page))
					goto keep_locked;
				mapping = page_mapping(page);
			case PAGE_CLEAN:
				; /* try to free the page below */
			}
		}

		/*
		 * If the page has buffers, try to free the buffer mappings
		 * associated with this page. If we succeed we try to free
		 * the page as well.
		 *
		 * We do this even if the page is PageDirty().
		 * try_to_release_page() does not perform I/O, but it is
		 * possible for a page to have PageDirty set, but it is actually
		 * clean (all its buffers are clean).  This happens if the
		 * buffers were written out directly, with submit_bh(). ext3
		 * will do this, as well as the blockdev mapping.
		 * try_to_release_page() will discover that cleanness and will
		 * drop the buffers and mark the page clean - it can be freed.
		 *
		 * Rarely, pages can have buffers and no ->mapping.  These are
		 * the pages which were not successfully invalidated in
		 * truncate_complete_page().  We try to drop those buffers here
		 * and if that worked, and the page is no longer mapped into
		 * process address space (page_count == 1) it can be freed.
		 * Otherwise, leave the page on the LRU so it is swappable.
		 */
		if (page_has_private(page)) {
			if (!try_to_release_page(page, sc->gfp_mask))
				goto activate_locked;
			if (!mapping && page_count(page) == 1) {
				unlock_page(page);
				if (put_page_testzero(page))
					goto free_it;
				else {
					/*
					 * rare race with speculative reference.
					 * the speculative reference will free
					 * this page shortly, so we may
					 * increment nr_reclaimed here (and
					 * leave it off the LRU).
					 */
					nr_reclaimed++;
					continue;
				}
			}
		}

		if (PageAnon(page) && !PageSwapBacked(page)) {
			/* follow __remove_mapping for reference */
			if (!page_ref_freeze(page, 1))
				goto keep_locked;
			if (PageDirty(page)) {
				page_ref_unfreeze(page, 1);
				goto keep_locked;
			}

			count_vm_event(PGLAZYFREED);
			count_memcg_page_event(page, PGLAZYFREED);
		} else if (!mapping || !__remove_mapping(mapping, page, true))
			goto keep_locked;
		/*
		 * At this point, we have no other references and there is
		 * no way to pick any more up (removed from LRU, removed
		 * from pagecache). Can use non-atomic bitops now (and
		 * we obviously don't have to worry about waking up a process
		 * waiting on the page lock, because there are no references.
		 */
		__ClearPageLocked(page);
free_it:
		nr_reclaimed++;

		/*
		 * Is there need to periodically free_page_list? It would
		 * appear not as the counts should be low
		 */
		if (unlikely(PageTransHuge(page))) {
			mem_cgroup_uncharge(page);
			(*get_compound_page_dtor(page))(page);
		} else
			list_add(&page->lru, &free_pages);
		continue;

activate_locked:
		/* Not a candidate for swapping, so reclaim swap space. */
		if (PageSwapCache(page) && (mem_cgroup_swap_full(page) ||
						PageMlocked(page)))
			try_to_free_swap(page);
		VM_BUG_ON_PAGE(PageActive(page), page);
		if (!PageMlocked(page)) {
			SetPageActive(page);
			pgactivate++;
			count_memcg_page_event(page, PGACTIVATE);
		}
keep_locked:
		unlock_page(page);
keep:
		list_add(&page->lru, &ret_pages);
		VM_BUG_ON_PAGE(PageLRU(page) || PageUnevictable(page), page);
	}

	mem_cgroup_uncharge_list(&free_pages);
	try_to_unmap_flush();
	free_unref_page_list(&free_pages);

	list_splice(&ret_pages, page_list);
	count_vm_events(PGACTIVATE, pgactivate);

	if (stat) {
		stat->nr_dirty = nr_dirty;
		stat->nr_congested = nr_congested;
		stat->nr_unqueued_dirty = nr_unqueued_dirty;
		stat->nr_writeback = nr_writeback;
		stat->nr_immediate = nr_immediate;
		stat->nr_activate = pgactivate;
		stat->nr_ref_keep = nr_ref_keep;
		stat->nr_unmap_fail = nr_unmap_fail;
	}
	return nr_reclaimed;
}

```




## lruvec->lists

```c
enum lru_list {
	LRU_INACTIVE_ANON = LRU_BASE,
	LRU_ACTIVE_ANON = LRU_BASE + LRU_ACTIVE,
	LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,
	LRU_ACTIVE_FILE = LRU_BASE + LRU_FILE + LRU_ACTIVE,
	LRU_UNEVICTABLE,
	NR_LRU_LISTS
};

// todo 此处的 active 就是表示 active 而不是 dirty (写了，但是没有刷新到磁盘)
#define for_each_lru(lru) for (lru = 0; lru < NR_LRU_LISTS; lru++)

#define for_each_evictable_lru(lru) for (lru = 0; lru <= LRU_ACTIVE_FILE; lru++)

static inline int is_file_lru(enum lru_list lru)
{
	return (lru == LRU_INACTIVE_FILE || lru == LRU_ACTIVE_FILE);
}

static inline int is_active_lru(enum lru_list lru)
{
	return (lru == LRU_ACTIVE_ANON || lru == LRU_ACTIVE_FILE);
}
```

## slab

```c
// TODO 那么fs中的dcache.c的内容，而且buffer.c 中间的内容还是看不懂!

// 调用者，这只是一个调用路径而已
// https://sysctl-explorer.net/vm/drop_caches/
int drop_caches_sysctl_handler(struct ctl_table *table, int write, void __user *buffer, size_t *length, loff_t *ppos)
  -> void drop_slab(void)
    -> void drop_slab_node(int nid)
      -> static unsigned long shrink_slab(gfp_t gfp_mask, int nid, struct mem_cgroup *memcg, int priority) ```
        -> static unsigned long do_shrink_slab(struct shrink_control *shrinkctl, struct shrinker *shrinker, int priority)
```

## isolate
1. 此处的 isolate : 和 compaction 没有任何关系，正如 isolate_lru_pages 上的注释所说，因为 heavily contended lock
    1. 所以，其调用者就是 : 真正进行 lrulist 移动的函数 : shrink_inactive_list + shrink_active_list
2. isolate_lru_pages 调用 `__isolate_lru_page` 其中 `__isolate_lru_page` 的实现是 lockless 的
3. isolate_lru_page 是 `__isolate_lru_page` 的上锁的版本。
4. too_many_isolated :
    1. @todo 应该不麻烦


```c
/* Isolate unmapped file */
#define ISOLATE_UNMAPPED	((__force isolate_mode_t)0x2)
/* Isolate for asynchronous migration */
#define ISOLATE_ASYNC_MIGRATE	((__force isolate_mode_t)0x4)
/* Isolate unevictable pages */
#define ISOLATE_UNEVICTABLE	((__force isolate_mode_t)0x8)
```

```c
/*
 * zone_lru_lock is heavily contended.  Some of the functions that
 * shrink the lists perform better by taking out a batch of pages
 * and working on them outside the LRU lock.
 *
 * For pagecache intensive workloads, this function is the hottest
 * spot in the kernel (apart from copy_*_user functions).
 *
 * Appropriate locks must be held before calling this function.
 *
 * @nr_to_scan:	The number of eligible pages to look through on the list.
 * @lruvec:	The LRU vector to pull pages from.
 * @dst:	The temp list to put pages on to.
 * @nr_scanned:	The number of pages that were scanned.
 * @sc:		The scan_control struct for this reclaim session
 * @mode:	One of the LRU isolation modes
 * @lru:	LRU list id for isolating
 *
 * returns how many pages were moved onto *@dst.
 */
static unsigned long isolate_lru_pages(unsigned long nr_to_scan,
		struct lruvec *lruvec, struct list_head *dst,
		unsigned long *nr_scanned, struct scan_control *sc,
		isolate_mode_t mode, enum lru_list lru)
    // 1. 实现 lruvec->lists[lru] 中间取出
    #define lru_to_page(head) (list_entry((head)->prev, struct page, lru))
    // 2. 循环 __isolate_lru_page 实现真正的移除

/*
 * Attempt to remove the specified page from its LRU.  Only take this page
 * if it is of the appropriate PageActive status.  Pages which are being
 * freed elsewhere are also ignored.
 *
 * page:	page to consider
 * mode:	one of the LRU isolation modes defined above
 *
 * returns 0 on success, -ve errno on failure.
 */
int __isolate_lru_page(struct page *page, isolate_mode_t mode)
    // 使用逻辑删除的概念，将page 中间的 LRU flag 清掉
    // 之所以可以如此，flags 的访问是原子的，不可能出现重复删除(因为首先会判断，最多只有一个成功)


/**
 * isolate_lru_page - tries to isolate a page from its LRU list
 * @page: page to isolate from its LRU list
 *
 * Isolates a @page from an LRU list, clears PageLRU and adjusts the
 * vmstat statistic corresponding to whatever LRU list the page was on.
 *
 * Returns 0 if the page was removed from an LRU list.
 * Returns -EBUSY if the page was not on an LRU list.
 *
 * The returned page will have PageLRU() cleared.  If it was found on
 * the active list, it will have PageActive set.  If it was found on
 * the unevictable list, it will have the PageUnevictable bit set. That flag
 * may need to be cleared by the caller before letting the page go.
 *
 * The vmstat statistic corresponding to the list on which the page was
 * found will be decremented.
 *
 * Restrictions:
 *
 * (1) Must be called with an elevated refcount on the page. This is a
 *     fundamentnal difference from isolate_lru_pages (which is called
 *     without a stable reference).
 * (2) the lru_lock must not be held.
 * (3) interrupts must be enabled.
 */

 // elevated refcount ? without stable reference ?
 // lru_lock must not be held ?
 // XXX 似乎和 __isolate_lru_page 实现的区别在于: 在修改 list 的时候，使用了 spin_lock_irq(zone_lru_lock(zone));
int isolate_lru_page(struct page *page)
{
	int ret = -EBUSY;

	VM_BUG_ON_PAGE(!page_count(page), page);
	WARN_RATELIMIT(PageTail(page), "trying to isolate tail page");

	if (PageLRU(page)) {
		struct zone *zone = page_zone(page);
		struct lruvec *lruvec;

		spin_lock_irq(zone_lru_lock(zone));
		lruvec = mem_cgroup_page_lruvec(page, zone->zone_pgdat);
		if (PageLRU(page)) {
			int lru = page_lru(page);
			get_page(page);
			ClearPageLRU(page);
			del_page_from_lru_list(page, lruvec, lru);
			ret = 0;
		}
		spin_unlock_irq(zone_lru_lock(zone));
	}
	return ret;
}

/*
 * A direct reclaimer may isolate SWAP_CLUSTER_MAX pages from the LRU list and
 * then get resheduled. When there are massive number of tasks doing page
 * allocation, such sleeping direct reclaimers may keep piling up on each CPU,
 * the LRU list will go small and be scanned faster than necessary, leading to
 * unnecessary swapping, thrashing and OOM.
 */
 // todo 注释没有看懂呀 !
static int too_many_isolated(struct pglist_data *pgdat, int file, struct scan_control *sc)
 // 被 shrink_inactive_list 唯一调用

/**
 * putback_lru_page - put previously isolated page onto appropriate LRU list
 * @page: page to be put back to appropriate lru list
 *
 * Add previously isolated @page to appropriate LRU list.
 * Page may still be unevictable for other reasons.
 *
 * lru_lock must not be held, interrupts must be enabled.
 */
void putback_lru_page(struct page *page)
{
	lru_cache_add(page);
	put_page(page);		/* drop ref from isolate */
}
```


## struct scan_control

## kswapd

## page_check_dirty_writeback : 辅助理解的小函数
> 虽然也理解不了

```c
/* Check if a page is dirty or under writeback */
static void page_check_dirty_writeback(struct page *page,
				       bool *dirty, bool *writeback)
{
	struct address_space *mapping;

	/*
	 * Anonymous pages are not handled by flushers and must be written
	 * from reclaim context. Do not stall reclaim based on them
	 */
	if (!page_is_file_cache(page) ||
	    (PageAnon(page) && !PageSwapBacked(page))) { // todo 显然，我是看不懂这一个判断句的含义的
		*dirty = false;
		*writeback = false;
		return;
	}

	/* By default assume that the page flags are accurate */
	*dirty = PageDirty(page);
	*writeback = PageWriteback(page);

	/* Verify dirty/writeback state if the filesystem supports it */
	if (!page_has_private(page)) // todo 也就是说，dirty/writeback 总是需要 private 字段的支持的
		return;

	mapping = page_mapping(page);
	if (mapping && mapping->a_ops->is_dirty_writeback) // todo 在什么情况下，is_dirty_writeback 是被注册过的，什么时候没有 ?
		mapping->a_ops->is_dirty_writeback(page, dirty, writeback);
}

/**
 * page_is_file_cache - should the page be on a file LRU or anon LRU?
 * @page: the page to test
 *
 * Returns 1 if @page is page cache page backed by a regular filesystem,
 * or 0 if @page is anonymous, tmpfs or otherwise ram or swap backed.
 * Used by functions that manipulate the LRU lists, to sort a page
 * onto the right LRU list.
 *
 * We would like to get this info without a page flag, but the state
 * needs to survive until the page is last deleted from the LRU, which
 * could be as far down as __page_cache_release.
 */
static inline int page_is_file_cache(struct page *page)
{
	return !PageSwapBacked(page);
}
```

## page_check_references : shrink_page_list 唯一调用
1. 但是根本不知道各种 flags 下的 page 的含义是什么 ?

```c
static enum page_references page_check_references(struct page *page,
						  struct scan_control *sc)
{
	int referenced_ptes, referenced_page;
	unsigned long vm_flags;

	referenced_ptes = page_referenced(page, 1, sc->target_mem_cgroup,
					  &vm_flags);
	referenced_page = TestClearPageReferenced(page);

	/*
	 * Mlock lost the isolation race with us.  Let try_to_unmap()
	 * move the page to the unevictable list.
	 */
	if (vm_flags & VM_LOCKED)
		return PAGEREF_RECLAIM;

	if (referenced_ptes) { // 当被映射的时候
		if (PageSwapBacked(page))
			return PAGEREF_ACTIVATE; // 为什么和 PAGEREF_ACTIVATE 的 todo
		/*
		 * All mapped pages start out with page table
		 * references from the instantiating fault, so we need
		 * to look twice if a mapped file page is used more
		 * than once.
		 *
		 * Mark it and spare it for another trip around the
		 * inactive list.  Another page table reference will
		 * lead to its activation.
		 *
		 * Note: the mark is set for activated pages as well
		 * so that recently deactivated but used pages are
		 * quickly recovered.
		 */
		SetPageReferenced(page);

		if (referenced_page || referenced_ptes > 1) // 所以为什么需要 referenced_ptes > 1 才可以啊 !
			return PAGEREF_ACTIVATE;

		/*
		 * Activate file-backed executable pages after first usage.
		 */
		if (vm_flags & VM_EXEC)
			return PAGEREF_ACTIVATE;

		return PAGEREF_KEEP;
	}

	/* Reclaim if clean, defer dirty pages to writeback */
	if (referenced_page && !PageSwapBacked(page)) //
		return PAGEREF_RECLAIM_CLEAN;

	return PAGEREF_RECLAIM;
}
```

## pageout : shrink_page_list 用于处理
1. 是将 dirty page 中间的 page write back 还是 write back 加上清理 ?
2. 这调用的内容太恐怖了
    1. 大致的内容是 : 将 dirty page 写会

```c
/*
 * pageout is called by shrink_page_list() for each dirty page.
 * Calls ->writepage().
 */
static pageout_t pageout(struct page *page, struct address_space *mapping)
{
	/*
	 * If the page is dirty, only perform writeback if that write
	 * will be non-blocking.  To prevent this allocation from being
	 * stalled by pagecache activity.  But note that there may be
	 * stalls if we need to run get_block().  We could test
	 * PagePrivate for that.
	 *
	 * If this process is currently in __generic_file_write_iter() against
	 * this page's queue, we can perform writeback even if that
	 * will block.
	 *
	 * If the page is swapcache, write it back even if that would
	 * block, for some throttling. This happens by accident, because
	 * swap_backing_dev_info is bust: it doesn't reflect the
	 * congestion state of the swapdevs.  Easy to fix, if needed.
	 */
	if (!is_page_cache_freeable(page))
		return PAGE_KEEP;
	if (!mapping) {
		/*
		 * Some data journaling orphaned pages can have
		 * page->mapping == NULL while being dirty with clean buffers.
		 */
		if (page_has_private(page)) {
			if (try_to_free_buffers(page)) {
				ClearPageDirty(page);
				pr_info("%s: orphaned page\n", __func__);
				return PAGE_CLEAN;
			}
		}
		return PAGE_KEEP;
	}
	if (mapping->a_ops->writepage == NULL)
		return PAGE_ACTIVATE;
	if (!may_write_to_inode(mapping->host))
		return PAGE_KEEP;

	if (clear_page_dirty_for_io(page)) {
		int res;
		struct writeback_control wbc = {
			.sync_mode = WB_SYNC_NONE,
			.nr_to_write = SWAP_CLUSTER_MAX,
			.range_start = 0,
			.range_end = LLONG_MAX,
			.for_reclaim = 1,
		};

		SetPageReclaim(page);
		res = mapping->a_ops->writepage(page, &wbc);
		if (res < 0)
			handle_write_error(mapping, page, res);
		if (res == AOP_WRITEPAGE_ACTIVATE) {
			ClearPageReclaim(page);
			return PAGE_ACTIVATE;
		}

		if (!PageWriteback(page)) {
			/* synchronous write or broken a_ops? */
			ClearPageReclaim(page);
		}
		trace_mm_vmscan_writepage(page);
		inc_node_page_state(page, NR_VMSCAN_WRITE);
		return PAGE_SUCCESS;
	}

	return PAGE_CLEAN;
}


static inline int is_page_cache_freeable(struct page *page)
{
	/*
	 * A freeable page cache page is referenced only by the caller
	 * that isolated the page, the page cache and optional buffer
	 * heads at page->private.
	 */
	int page_cache_pins = PageTransHuge(page) && PageSwapCache(page) ?
		HPAGE_PMD_NR : 1;
	return page_count(page) - page_has_private(page) == 1 + page_cache_pins;
}
```
