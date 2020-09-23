# swap.c 分析

1. lurvec 和 pagevec 各自的作用: lrulist 封装和 batch 操作封装
2. 本文件处理的内容和 swap 没有什么蛇皮关系，虽然主要的内容是 pagevec 的各种操作，但是实际上是向各种 lrulist 中间添加。

## core sruct 
```c
// pg_data_t 中间的实现每个 node 独立分配的
static inline struct lruvec *mem_cgroup_page_lruvec(struct page *page,
						    struct pglist_data *pgdat)
{
	return &pgdat->lruvec;
}
```

```c
struct pagevec {
	unsigned char nr;
	bool percpu_pvec_drained;
	struct page *pages[PAGEVEC_SIZE];
};
```

一个标准的例子 :
```c
/*
 * deactivate_page - deactivate a page
 * @page: page to deactivate
 *
 * deactivate_page() moves @page to the inactive list if @page was on the active
 * list and was not an unevictable page.  This is done to accelerate the reclaim
 * of @page.
 */
void deactivate_page(struct page *page)
{
	if (PageLRU(page) && PageActive(page) && !PageUnevictable(page)) {
		struct pagevec *pvec = &get_cpu_var(lru_deactivate_pvecs);

		get_page(page);
		if (!pagevec_add(pvec, page) || PageCompound(page)) // 首先试着添加到 percpu 上
			pagevec_lru_move_fn(pvec, lru_deactivate_fn, NULL); // 如果满了，就利用 pagevec_lru_move_fn + 特定的移动函数，将 pagevec 清空
		put_cpu_var(lru_deactivate_pvecs);
	}
}
```


# 调用环节 : 莫名奇妙的
> vmscan.c 整个维持swap页面的替换回去，但是page cache 的刷新回去的操作谁来控制 ? 
> page cache 和 swap cache 是不是采用相同的模型进行的 ? 如果说，其中，将anon memory 当做 swap 形成的file based 那么岂不是很好。

```c
// 继续分析其调用者
unsigned long reclaim_clean_pages_from_list(struct zone *zone,
					    struct list_head *page_list)

static unsigned long shrink_page_list(struct list_head *page_list, // vmscan.c
				      struct pglist_data *pgdat,
				      struct scan_control *sc,
				      enum ttu_flags ttu_flags,
				      struct reclaim_stat *stat,
				      bool force_reclaim)

  // 唯一调用
  -> int add_to_swap(struct page *page) // swap_state.c
```


## lru_cache_add
> @todo 本函数的实现的分析

lru_cache_add 调用者, 一共三个位置，三个不同的文件
```c
// 分析 lru_cache_add 的作用 : 根据page 的 flags 将page 放到 lru list 中间。

// vmscan.c
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

// filemap.c
// TODO 这是我一直想要的page swap 的回收吗 ?
int add_to_page_cache_lru(struct page *page, struct address_space *mapping,
				pgoff_t offset, gfp_t gfp_mask)

// swap.c 中间，应该用于进一步封装 lru_cache_add 的，TODO 了解其调用者
/**
 * lru_cache_add_active_or_unevictable
 * @page:  the page to be added to LRU
 * @vma:   vma in which page is mapped for determining reclaimability
 *
 * Place @page on the active or unevictable LRU list, depending on its
 * evictability.  Note that if the page is not evictable, it goes
 * directly back onto it's zone's unevictable list, it does NOT use a
 * per cpu pagevec.
 */
void lru_cache_add_active_or_unevictable(struct page *page,
					 struct vm_area_struct *vma)
```



```c
// TODO 这是和 lru_cache_add 的反向的内容吗 ?
void lru_add_drain(void)
{
	lru_add_drain_cpu(get_cpu());
	put_cpu();
}
```


## wired

```c
// TODO 完成相同的函数为什么写两次，
// 而且不知道和 lru_cache_add 的区别啊 !
// TODO 它被谁调用 ?

/**
 * lru_cache_add_anon - add a page to the page lists
 * @page: the page to add
 */
void lru_cache_add_anon(struct page *page)
{
	if (PageActive(page))
		ClearPageActive(page);
	__lru_cache_add(page);
}

void lru_cache_add_file(struct page *page)
{
	if (PageActive(page))
		ClearPageActive(page);
	__lru_cache_add(page);
}
EXPORT_SYMBOL(lru_cache_add_file);
```


## pagevec_lru_move_fn
1. 对于 pagevec 中间的所有 page 调用 move_fn
2. 然后 release_pages

```c
static void pagevec_lru_move_fn(struct pagevec *pvec,
	void (*move_fn)(struct page *page, struct lruvec *lruvec, void *arg),
	void *arg)
{
	int i;
	struct pglist_data *pgdat = NULL;
	struct lruvec *lruvec;
	unsigned long flags = 0;

	for (i = 0; i < pagevec_count(pvec); i++) { // 对于 pagevec 中间所有的数值处理掉
		struct page *page = pvec->pages[i];
		struct pglist_data *pagepgdat = page_pgdat(page);

		if (pagepgdat != pgdat) {
			if (pgdat)
				spin_unlock_irqrestore(&pgdat->lru_lock, flags); // 当切换到不同的 pgdat 的时候，需要将 pgdat 对于 lru lock 去掉锁
			pgdat = pagepgdat;
			spin_lock_irqsave(&pgdat->lru_lock, flags);
		}

		lruvec = mem_cgroup_page_lruvec(page, pgdat);
		(*move_fn)(page, lruvec, arg);
	}
	if (pgdat)
		spin_unlock_irqrestore(&pgdat->lru_lock, flags);
	release_pages(pvec->pages, pvec->nr); // todo 为什么要 release
	pagevec_reinit(pvec);
}
```

## pagevec_lru_move_fn 的调用者 : 其实是外部的接口
> 依赖于某一个模式，move_fn 定义的 paradiam : 将当前的page 移除所在的lru list 然后添加到制定的lru list 中间。
```c
/*
 * pagevec_move_tail() must be called with IRQ disabled.
 * Otherwise this may cause nasty races.
 */
static void pagevec_move_tail(struct pagevec *pvec)
{
	int pgmoved = 0;

	pagevec_lru_move_fn(pvec, pagevec_move_tail_fn, &pgmoved);
	__count_vm_events(PGROTATED, pgmoved);
}

static void activate_page_drain(int cpu)
{
	struct pagevec *pvec = &per_cpu(activate_page_pvecs, cpu);

	if (pagevec_count(pvec))
		pagevec_lru_move_fn(pvec, __activate_page, NULL);
}


/*
 * Drain pages out of the cpu's pagevecs.
 * Either "cpu" is the current CPU, and preemption has already been
 * disabled; or "cpu" is being hot-unplugged, and is already dead.
 */
void lru_add_drain_cpu(int cpu)
	if (pagevec_count(pvec))
		pagevec_lru_move_fn(pvec, lru_deactivate_file_fn, NULL);

	pvec = &per_cpu(lru_lazyfree_pvecs, cpu);
	if (pagevec_count(pvec))
		pagevec_lru_move_fn(pvec, lru_lazyfree_fn, NULL);


/**
 * deactivate_file_page - forcefully deactivate a file page
 * @page: page to deactivate
 *
 * This function hints the VM that @page is a good reclaim candidate,
 * for example if its invalidation fails due to the page being dirty
 * or under writeback.
 */
void deactivate_file_page(struct page *page)
{
	/*
	 * In a workload with many unevictable page such as mprotect,
	 * unevictable page deactivation for accelerating reclaim is pointless.
	 */
	if (PageUnevictable(page))
		return;

	if (likely(get_page_unless_zero(page))) {
		struct pagevec *pvec = &get_cpu_var(lru_deactivate_file_pvecs);

		if (!pagevec_add(pvec, page) || PageCompound(page))
			pagevec_lru_move_fn(pvec, lru_deactivate_file_fn, NULL);
		put_cpu_var(lru_deactivate_file_pvecs);
	}
}

/**
 * mark_page_lazyfree - make an anon page lazyfree
 * @page: page to deactivate
 *
 * mark_page_lazyfree() moves @page to the inactive file list.
 * This is done to accelerate the reclaim of @page.
 */
void mark_page_lazyfree(struct page *page)
{
	if (PageLRU(page) && PageAnon(page) && PageSwapBacked(page) &&
	    !PageSwapCache(page) && !PageUnevictable(page)) {
		struct pagevec *pvec = &get_cpu_var(lru_lazyfree_pvecs);

		get_page(page);
		if (!pagevec_add(pvec, page) || PageCompound(page))
			pagevec_lru_move_fn(pvec, lru_lazyfree_fn, NULL);
		put_cpu_var(lru_lazyfree_pvecs);
	}
}

/*
 * Add the passed pages to the LRU, then drop the caller's refcount
 * on them.  Reinitialises the caller's pagevec.
 */
void __pagevec_lru_add(struct pagevec *pvec)
{
	pagevec_lru_move_fn(pvec, __pagevec_lru_add_fn, NULL);
}
EXPORT_SYMBOL(__pagevec_lru_add);
```

## pagevec_lru_move_fn 的参数
似乎是除了第一个，其余的都是用于实现在 lrulsit 上进行移动的，其实也不是移动，只是从一个list 上删除，然后加入到另一个 list 上，删除也只是为了保持不要同时出现在多个 list 上的防护，
所以，下面的各种移动函数，只是特殊版本的添加函数。只要这些函数是作为 pagevec_lru_move_fn 的参数，那么，其作用就是添加，而不是移动。

#### (1) `__pagevec_lru_add_fn`

1. 确定正确的 lrulist 然后添加到其中
```c
static void __pagevec_lru_add_fn(struct page *page, struct lruvec *lruvec,
				 void *arg)
{
	enum lru_list lru;
	int was_unevictable = TestClearPageUnevictable(page);

	VM_BUG_ON_PAGE(PageLRU(page), page);

	SetPageLRU(page);
	/*
	 * Page becomes evictable in two ways:
	 * 1) Within LRU lock [munlock_vma_page() and __munlock_pagevec()].
	 * 2) Before acquiring LRU lock to put the page to correct LRU and then
	 *   a) do PageLRU check with lock [check_move_unevictable_pages]
	 *   b) do PageLRU check before lock [clear_page_mlock]
	 *
	 * (1) & (2a) are ok as LRU lock will serialize them. For (2b), we need
	 * following strict ordering:
	 *
	 * #0: __pagevec_lru_add_fn		#1: clear_page_mlock
	 *
	 * SetPageLRU()				TestClearPageMlocked()
	 * smp_mb() // explicit ordering	// above provides strict
	 *					// ordering
	 * PageMlocked()			PageLRU()
	 *
	 *
	 * if '#1' does not observe setting of PG_lru by '#0' and fails
	 * isolation, the explicit barrier will make sure that page_evictable
	 * check will put the page in correct LRU. Without smp_mb(), SetPageLRU
	 * can be reordered after PageMlocked check and can make '#1' to fail
	 * the isolation of the page whose Mlocked bit is cleared (#0 is also
	 * looking at the same page) and the evictable page will be stranded
	 * in an unevictable LRU.
	 */
	smp_mb(); // todo 777777 这应该是理解 smp_mb() 最简单的位置了

	if (page_evictable(page)) {
		lru = page_lru(page);
		update_page_reclaim_stat(lruvec, page_is_file_cache(page),
					 PageActive(page));
		if (was_unevictable)
			count_vm_event(UNEVICTABLE_PGRESCUED);
	} else {
		lru = LRU_UNEVICTABLE;
		ClearPageActive(page); // clearPageActive ，因为都设置为 unevictable 了
		SetPageUnevictable(page);
		if (!was_unevictable)
			count_vm_event(UNEVICTABLE_PGCULLED);
	}

	add_page_to_lru_list(page, lruvec, lru);
	trace_mm_lru_insertion(page, lru);
}
```

#### (2) lru_lazyfree_fn 搬运

```c
static void lru_lazyfree_fn(struct page *page, struct lruvec *lruvec,
			    void *arg)
{
	if (PageLRU(page) && PageAnon(page) && PageSwapBacked(page) &&
	    !PageSwapCache(page) && !PageUnevictable(page)) { // 各种限制条件
		bool active = PageActive(page);

		del_page_from_lru_list(page, lruvec,
				       LRU_INACTIVE_ANON + active); // todo 取出来的位置是 ANON
		ClearPageActive(page);
		ClearPageReferenced(page);
		/*
		 * lazyfree pages are clean anonymous pages. They have
		 * SwapBacked flag cleared to distinguish normal anonymous
		 * pages
		 */
		ClearPageSwapBacked(page);
		add_page_to_lru_list(page, lruvec, LRU_INACTIVE_FILE); // todo 放入的位置却是 FILE

		__count_vm_events(PGLAZYFREE, hpage_nr_pages(page));
		count_memcg_page_event(page, PGLAZYFREE);
		update_page_reclaim_stat(lruvec, 1, 0);
	}
}
```


#### (3) lru_deactivate_file_fn 搬运
> @todo 这注释看不懂啊 !

```c
/*
 * If the page can not be invalidated, it is moved to the
 * inactive list to speed up its reclaim.  It is moved to the
 * head of the list, rather than the tail, to give the flusher
 * threads some time to write it out, as this is much more
 * effective than the single-page writeout from reclaim.
 *
 * If the page isn't page_mapped and dirty/writeback, the page
 * could reclaim asap using PG_reclaim.
 *
 * 1. active, mapped page -> none
 * 2. active, dirty/writeback page -> inactive, head, PG_reclaim
 * 3. inactive, mapped page -> none
 * 4. inactive, dirty/writeback page -> inactive, head, PG_reclaim
 * 5. inactive, clean -> inactive, tail
 * 6. Others -> none
 *
 * In 4, why it moves inactive's head, the VM expects the page would
 * be write it out by flusher threads as this is much more effective
 * than the single-page writeout from reclaim.
 */
static void lru_deactivate_file_fn(struct page *page, struct lruvec *lruvec,
			      void *arg)
{
	int lru, file;
	bool active;

	if (!PageLRU(page))
		return;

	if (PageUnevictable(page))
		return;

	/* Some processes are using the page */
	if (page_mapped(page))
		return;

	active = PageActive(page);
	file = page_is_file_cache(page);
	lru = page_lru_base_type(page);

	del_page_from_lru_list(page, lruvec, lru + active);
	ClearPageActive(page);
	ClearPageReferenced(page);

	if (PageWriteback(page) || PageDirty(page)) {
		/*
		 * PG_reclaim could be raced with end_page_writeback
		 * It can make readahead confusing.  But race window
		 * is _really_ small and  it's non-critical problem.
		 */
		add_page_to_lru_list(page, lruvec, lru);
		SetPageReclaim(page);
	} else {
		/*
		 * The page's writeback ends up during pagevec
		 * We moves tha page into tail of inactive.
		 */
		add_page_to_lru_list_tail(page, lruvec, lru);
		__count_vm_event(PGROTATED);
	}

	if (active)
		__count_vm_event(PGDEACTIVATE);
	update_page_reclaim_stat(lruvec, file, 0);
}
```

#### (4) lru_deactivate_fn 搬运
```c
static void lru_deactivate_fn(struct page *page, struct lruvec *lruvec,
			    void *arg)
{
	if (PageLRU(page) && PageActive(page) && !PageUnevictable(page)) {
		int file = page_is_file_cache(page);
		int lru = page_lru_base_type(page);

		del_page_from_lru_list(page, lruvec, lru + LRU_ACTIVE);
		ClearPageActive(page);
		ClearPageReferenced(page);
		add_page_to_lru_list(page, lruvec, lru);

		__count_vm_events(PGDEACTIVATE, hpage_nr_pages(page));
		update_page_reclaim_stat(lruvec, file, 0);
	}
}
```

#### (5) `__activate_page` 搬运

```c
static void __activate_page(struct page *page, struct lruvec *lruvec,
			    void *arg)
{
	if (PageLRU(page) && !PageActive(page) && !PageUnevictable(page)) {
		int file = page_is_file_cache(page);
		int lru = page_lru_base_type(page);

		del_page_from_lru_list(page, lruvec, lru);
		SetPageActive(page);
		lru += LRU_ACTIVE;
		add_page_to_lru_list(page, lruvec, lru);
		trace_mm_lru_activate(page);

		__count_vm_event(PGACTIVATE);
		update_page_reclaim_stat(lruvec, file, 1);
	}
}
```



## lru_add_drain_cpu
1. 想不到还有一堆 percpu 的 pagevec
    1. 这不是废话吗 ? 不然将 page 积累在什么地方上 ?
```c
/*
 * Drain pages out of the cpu's pagevecs.
 * Either "cpu" is the current CPU, and preemption has already been
 * disabled; or "cpu" is being hot-unplugged, and is already dead.
 */
void lru_add_drain_cpu(int cpu)
{
	struct pagevec *pvec = &per_cpu(lru_add_pvec, cpu);

	if (pagevec_count(pvec))
		__pagevec_lru_add(pvec);                                 // 1

	pvec = &per_cpu(lru_rotate_pvecs, cpu);
	if (pagevec_count(pvec)) {
		unsigned long flags;

		/* No harm done if a racing interrupt already did this */
		local_irq_save(flags);
		pagevec_move_tail(pvec);
		local_irq_restore(flags);
	}

	pvec = &per_cpu(lru_deactivate_file_pvecs, cpu);
	if (pagevec_count(pvec))
		pagevec_lru_move_fn(pvec, lru_deactivate_file_fn, NULL); // 2

	pvec = &per_cpu(lru_deactivate_pvecs, cpu);
	if (pagevec_count(pvec))
		pagevec_lru_move_fn(pvec, lru_deactivate_fn, NULL);      // 3 

	pvec = &per_cpu(lru_lazyfree_pvecs, cpu);
	if (pagevec_count(pvec))
		pagevec_lru_move_fn(pvec, lru_lazyfree_fn, NULL);        // 4

	activate_page_drain(cpu);
}
```

## mark_page_accessed
> @todo 也许参考一下其他的位置

```c
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

	if (!PageReferenced(page)) {
		SetPageReferenced(page);
	} else if (PageUnevictable(page)) {
		/*
		 * Unevictable pages are on the "LRU_UNEVICTABLE" list. But,
		 * this list is never rotated or maintained, so marking an
		 * evictable page accessed has no effect.
		 */
	} else if (!PageActive(page)) {
		/*
		 * If the page is on the LRU, queue it for activation via
		 * activate_page_pvecs. Otherwise, assume the page is on a
		 * pagevec, mark it active and it'll be moved to the active
		 * LRU on the next drain.
		 */
		if (PageLRU(page))
			activate_page(page);
		else
			__lru_cache_activate_page(page);
		ClearPageReferenced(page);
		if (page_is_file_cache(page))
			workingset_activation(page);
	}
}
EXPORT_SYMBOL(mark_page_accessed);
```
1. filemap.c : do_read_cache_page  pagecache_get_page generic_file_buffered_read
2. buffered-io.c : 看不懂
3. fs/buffer.c : `__find_get_block`
4. shmem.c : 没有分析

> @todo 此处仅仅处理了文件io 使用的 mark_page_accessed 的情况
> @todo 当一个 page 用于实现加速 读写文件的时候，还有 rmap 的需求吗 ?
    1. rmap 用于实现page frame 和 vma 之间的关系
        1. 三个入口了解一下

> @todo 同样的，此处分析的内容缺少 anon 的分析，除非 shmem.c 中间的是用于 /tmp 的，所以似乎没有用于 anon 的
> 除非，mark_page_accessed 表示这次，这个东西是和 fs 打过交道的
> 所以，对于 anon 处理的地方在于 swap cache 中间吗 ?


> @todo 非常的怀疑，page_referenced 的调用，只有当发现其实在上一次调用 page_referenced 到此次，根本没有任何
> 对于该 page frame 的映射发生过访问，所以，决定下调。
> 而 mark_page_accessed 出现的位置表示 : 该 page 刚刚读入进来，如果此时换出，非常的不应该。


mark_page_accessed : 慢速访问，重量级访问
    1. anon page 加入和 mark_page_accessed 的操作都是不知道在哪里的!
    2. 如果mark_page_accessed 真的是 慢速访问，重量级访问，那么，应该swap cache 就需要含有对应的调用
        1. swap cache 也是需要加入到 lrulist 中间吧 !

page_referenced : 硬件记录，轻量级访问
    1. 但是 page cache 
    2. 当一个 page 被映射到 vma，为什么还需要 page cache ?
        1. page cache 是用于读写文件的加速的
        2. 证据容易验证 : file mapped 的发生 pgfault 发生的时候，会利用 page cache 吗 ? 会拷贝，还是直接修改 page table 就可以了 ?

lruvec 中间的类型只有 : anon 以及 file 和 unevictable，都是映射而已。
    1. 如果是靠这个怀疑 io page cache 不在 reclaim 的机制之下，那么 generic_file_buffered_read 中间也是调用过 mark_page_accessed 的
    2. page-writeback 的功能只是表示 dirty 数量不要超过某一个阈值，和到底存在多少个 page 在内存中间没有关系

**filemap_fault 调用过 find_get_page，所以 mapped 的区间不可能不被 page cache 管理过**



## `put_page` : 基于 `_refcount` 的释放 page

```c
static inline void put_page(struct page *page)
{
	page = compound_head(page);

	/*
	 * For devmap managed pages we need to catch refcount transition from
	 * 2 to 1, when refcount reach one it means the page is free and we
	 * need to inform the device driver through callback. See
	 * include/linux/memremap.h and HMM for details.
	 */
	if (page_is_devmap_managed(page)) { // 依赖于 CONFIG_DEV_PAGEMAP_OPS
		put_devmap_managed_page(page);
		return;
	}

	if (put_page_testzero(page)) // 减少其 _refcount，当 _refcount 为 0 的时候，删除 page 
		__put_page(page);
}

void __put_page(struct page *page)
{
	if (is_zone_device_page(page)) {
		put_dev_pagemap(page->pgmap);

		/*
		 * The page belongs to the device that created pgmap. Do
		 * not return it to page allocator.
		 */
		return;
	}

	if (unlikely(PageCompound(page)))
		__put_compound_page(page);
	else
		__put_single_page(page);
}


static void __put_single_page(struct page *page)
{
	__page_cache_release(page);
	mem_cgroup_uncharge(page);
	free_unref_page(page); // 释放工作
}

/*
 * This path almost never happens for VM activity - pages are normally
 * freed via pagevecs.  But it gets used by networking.
 */
static void __page_cache_release(struct page *page)
{
	if (PageLRU(page)) { // todo 不知道为什么此处突然出现了 LRU list 的删除工作的含义是什么 ?
		pg_data_t *pgdat = page_pgdat(page);
		struct lruvec *lruvec;
		unsigned long flags;

		spin_lock_irqsave(&pgdat->lru_lock, flags);
		lruvec = mem_cgroup_page_lruvec(page, pgdat);
		VM_BUG_ON_PAGE(!PageLRU(page), page);
		__ClearPageLRU(page);
		del_page_from_lru_list(page, lruvec, page_off_lru(page));
		spin_unlock_irqrestore(&pgdat->lru_lock, flags);
	}
	__ClearPageWaiters(page); // todo 什么意思 ?
}
```

