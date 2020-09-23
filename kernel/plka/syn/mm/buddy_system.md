# 摘要
However, the buddy systems of all zones and nodes are linked via the allocation fallback list.

Grouping pages by mobility order is one possible method to prevent fragmentation of physical
memory, but the kernel additionally provides another means to fight this problem: the virtual
zone ZONE_MOVABLE.

# 问题
0. per_cpu_pageset 到底是用于管理缓存的还是管理migration的?
1. 为什么定义如此之多的gfp_t 类型，都是用来做什么的 ? 都是如何被一一化解的 ?
1. imgration 的内容似乎就是简单的通过创建多个list 解决的
2. buddy system 如何确定那些page 的imagration 的类型 ?
3. watermark 不同数值会导致分配的策略变化是什么 ?
4. 找到cache hot-cold page 的内容位置 ?

1. alloc_pages 调用链条是什么？
2. fallback list ?
3. pageset watermark 如何融合的 ?
4. 如何通过zone movable机制实现anti-fragmentation

# alloc_pages 调用链条
```c
# 逐步进入核心
/*
 * This is the 'heart' of the zoned buddy allocator.
 */
struct page *
__alloc_pages_nodemask(gfp_t gfp_mask, unsigned int order, int preferred_nid,
							nodemask_t *nodemask)
{



static inline struct page *
__alloc_pages(gfp_t gfp_mask, unsigned int order, int preferred_nid)
{
	return __alloc_pages_nodemask(gfp_mask, order, preferred_nid, NULL);
}
```


mempolicy.c : 还需要考虑numa 的四种policy (现在默认分析default policy)
// 进一步获取node 和 nodemsk
```c
struct page *alloc_pages_current(gfp_t gfp, unsigned order)
{
	struct mempolicy *pol = &default_policy;
	struct page *page;

	if (!in_interrupt() && !(gfp & __GFP_THISNODE))
		pol = get_task_policy(current);

	/*
	 * No reference counting needed for current->mempolicy
	 * nor system default_policy
	 */
	if (pol->mode == MPOL_INTERLEAVE)
		page = alloc_page_interleave(gfp, order, interleave_nodes(pol));
	else
		page = __alloc_pages_nodemask(gfp, order,
				policy_node(gfp, pol, numa_node_id()),
				policy_nodemask(gfp, pol));

	return page;
}
EXPORT_SYMBOL(alloc_pages_current);


gfp.h
```c
static inline struct page *
alloc_pages(gfp_t gfp_mask, unsigned int order)
{
	return alloc_pages_current(gfp_mask, order);
}
```

# 核心内容
```c
/*
 * get_page_from_freelist goes through the zonelist trying to allocate
 * a page.
 */
 // 应该一般会进入的位置

/*
 * This is the 'heart' of the zoned buddy allocator.
 */
struct page *
__alloc_pages_nodemask(gfp_t gfp_mask, unsigned int order, int preferred_nid,
							nodemask_t *nodemask)
{
	/* First allocation attempt */
	page = get_page_from_freelist(alloc_mask, order, alloc_flags, &ac);
	if (likely(page))
		goto out;

/*
 * Structure for holding the mostly immutable allocation parameters passed
 * between functions involved in allocations, including the alloc_pages*
 * family of functions.
 *
 * nodemask, migratetype and high_zoneidx are initialized only once in
 * __alloc_pages_nodemask() and then never change.
 *
 * zonelist, preferred_zone and classzone_idx are set first in
 * __alloc_pages_nodemask() for the fast path, and might be later changed
 * in __alloc_pages_slowpath(). All other functions pass the whole strucure
 * by a const pointer.
 */
struct alloc_context {
	struct zonelist *zonelist;
	nodemask_t *nodemask;
	struct zoneref *preferred_zoneref;
	int migratetype;
	enum zone_type high_zoneidx;
	bool spread_dirty_pages;
};
// 为此特地定义的



static struct page *
get_page_from_freelist(gfp_t gfp_mask, unsigned int order, int alloc_flags,
						const struct alloc_context *ac)
{
...
try_this_zone:
		page = rmqueue(ac->preferred_zoneref->zone, zone, order,
				gfp_mask, alloc_flags, ac->migratetype);
...
```
// 经过一系列的检查最后找到的内容，其中就包括watermark相关的内容。

```c
/*
 * Allocate a page from the given zone. Use pcplists for order-0 allocations.
 */
static inline
struct page *rmqueue(struct zone *preferred_zone,
			struct zone *zone, unsigned int order,
			gfp_t gfp_flags, unsigned int alloc_flags,
			int migratetype)
{
	unsigned long flags;
	struct page *page;

  // 如果为
	if (likely(order == 0)) {
		page = rmqueue_pcplist(preferred_zone, zone, order,
				gfp_flags, migratetype);
		goto out;
	}




// 其实这才是最初认识的
/*
 * Go through the free lists for the given migratetype and remove
 * the smallest available page from the freelists
 */
static __always_inline
struct page *__rmqueue_smallest(struct zone *zone, unsigned int order,
						int migratetype)
{
	unsigned int current_order;
	struct free_area *area;
	struct page *page;

	/* Find a page of the appropriate size in the preferred list */
	for (current_order = order; current_order < MAX_ORDER; ++current_order) {
		area = &(zone->free_area[current_order]);
		page = list_first_entry_or_null(&area->free_list[migratetype],
							struct page, lru);
		if (!page)
			continue;
		list_del(&page->lru);
		rmv_page_order(page);
		area->nr_free--;
		expand(zone, page, order, current_order, area, migratetype);
		set_pcppage_migratetype(page, migratetype);
		return page;
	}

	return NULL;
}
```

> 处理所有的page 为1 的分配
```c
/* Lock and remove page from the per-cpu list */
static struct page *rmqueue_pcplist(struct zone *preferred_zone,
			struct zone *zone, unsigned int order,
			gfp_t gfp_flags, int migratetype)
{
	struct per_cpu_pages *pcp;
	struct list_head *list;
	struct page *page;
	unsigned long flags;

	local_irq_save(flags);
	pcp = &this_cpu_ptr(zone->pageset)->pcp;
	list = &pcp->lists[migratetype];
	page = __rmqueue_pcplist(zone,  migratetype, pcp, list);
	if (page) {
		__count_zid_vm_events(PGALLOC, page_zonenum(page), 1 << order);
		zone_statistics(preferred_zone, zone);
	}
	local_irq_restore(flags);
	return page;
}


/* Remove page from the per-cpu list, caller must protect the list */
static struct page *__rmqueue_pcplist(struct zone *zone, int migratetype,
			struct per_cpu_pages *pcp,
			struct list_head *list)
{
	struct page *page;

	do {
     // 从zone 中间获取，但是此时已经和pcp脱离关系了!
		if (list_empty(list)) {
      // 居然order = 0 难道不能order > 0 实现更加多，而且快速的分配吗 
			pcp->count += rmqueue_bulk(zone, 0,
					pcp->batch, list,
					migratetype);
			if (unlikely(list_empty(list)))
				return NULL;
		}

		page = list_first_entry(list, struct page, lru);
		list_del(&page->lru);
		pcp->count--;
	} while (check_new_pcp(page));

	return page;
}

/*
 * Obtain a specified number of elements from the buddy allocator, all under
 * a single hold of the lock, for efficiency.  Add them to the supplied list.
 * Returns the number of new pages which were placed at *list.
 */
static int rmqueue_bulk(struct zone *zone, unsigned int order,
			unsigned long count, struct list_head *list,
			int migratetype)
{



// 这一个函数几乎等价于调用，__rmqueue_smallest 而后者则是当order > 0 最后调用对象
// pcp 是一个作用到底是什么? 似乎像是给每一个page 给每一个cpu 的缓存 ?

/*
 * Do the hard work of removing an element from the buddy allocator.
 * Call me with the zone->lock already held.
 */
static __always_inline struct page *
__rmqueue(struct zone *zone, unsigned int order, int migratetype)
{
	struct page *page;

retry:
	page = __rmqueue_smallest(zone, order, migratetype);
	if (unlikely(!page)) {
		if (migratetype == MIGRATE_MOVABLE)
			page = __rmqueue_cma_fallback(zone, order);

		if (!page && __rmqueue_fallback(zone, order, migratetype))
			goto retry;
	}

	trace_mm_page_alloc_zone_locked(page, order, migratetype);
	return page;
}
```


# 释放过程

```c
void free_pages(unsigned long addr, unsigned int order)
{
	if (addr != 0) {
		VM_BUG_ON(!virt_addr_valid((void *)addr));
		__free_pages(virt_to_page((void *)addr), order);
	}
}

// 从kaddr => pa => pfn => page struct
/*
 * virt_to_page(kaddr) returns a valid pointer if and only if
 * virt_addr_valid(kaddr) returns true.
 */
#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)
#define pfn_to_kaddr(pfn)      __va((pfn) << PAGE_SHIFT)


void __free_pages(struct page *page, unsigned int order)
{
	if (put_page_testzero(page)) {
		if (order == 0)
			free_unref_page(page);
		else
			__free_pages_ok(page, order);
	}
}


static void __free_pages_ok(struct page *page, unsigned int order)
{
	unsigned long flags;
	int migratetype;
	unsigned long pfn = page_to_pfn(page);

	if (!free_pages_prepare(page, order, true))
		return;

	migratetype = get_pfnblock_migratetype(page, pfn);
	local_irq_save(flags);
	__count_vm_events(PGFREE, 1 << order);
	free_one_page(page_zone(page), page, pfn, order, migratetype);
	local_irq_restore(flags);
}

static void free_one_page(struct zone *zone,
				struct page *page, unsigned long pfn,
				unsigned int order,
				int migratetype)
{
	spin_lock(&zone->lock);
	if (unlikely(has_isolate_pageblock(zone) ||
		is_migrate_isolate(migratetype))) {
		migratetype = get_pfnblock_migratetype(page, pfn);
	}
	__free_one_page(page, pfn, zone, order, migratetype);
	spin_unlock(&zone->lock);
}

/*
 * Freeing function for a buddy system allocator.
 *
 * The concept of a buddy system is to maintain direct-mapped table
 * (containing bit values) for memory blocks of various "orders".
 * The bottom level table contains the map for the smallest allocatable
 * units of memory (here, pages), and each level above it describes
 * pairs of units from the levels below, hence, "buddies".
 * At a high level, all that happens here is marking the table entry
 * at the bottom level available, and propagating the changes upward
 * as necessary, plus some accounting needed to play nicely with other
 * parts of the VM system.
 * At each level, we keep a list of pages, which are heads of continuous
 * free pages of length of (1 << order) and marked with PageBuddy.
 * Page's order is recorded in page_private(page) field.
 * So when we are allocating or freeing one, we can derive the state of the
 * other.  That is, if we allocate a small block, and both were
 * free, the remainder of the region must be split into blocks.
 * If a block is freed, and its buddy is also free, then this
 * triggers coalescing into a block of larger size.
 *
 * -- nyc
 */

static inline void __free_one_page(struct page *page,
		unsigned long pfn,
		struct zone *zone, unsigned int order,
		int migratetype)
{
// 处理的内容比较简单，大概是free 之后的各种合并!
```


# 分析migration的函数
> 有必要在get_pages 的位置也加以分析啊!


```c
static __always_inline int get_pfnblock_migratetype(struct page *page, unsigned long pfn)
{
	return __get_pfnblock_flags_mask(page, pfn, PB_migrate_end, MIGRATETYPE_MASK);
}


/**
 * get_pfnblock_flags_mask - Return the requested group of flags for the pageblock_nr_pages block of pages
 * @page: The page within the block of interest
 * @pfn: The target page frame number
 * @end_bitidx: The last bit of interest to retrieve
 * @mask: mask of bits that the caller is interested in
 *
 * Return: pageblock_bits flags
 */
static __always_inline unsigned long __get_pfnblock_flags_mask(struct page *page,
					unsigned long pfn,
					unsigned long end_bitidx,
					unsigned long mask)
{
	unsigned long *bitmap;
	unsigned long bitidx, word_bitidx;
	unsigned long word;

	bitmap = get_pageblock_bitmap(page, pfn);
	bitidx = pfn_to_bitidx(page, pfn);
	word_bitidx = bitidx / BITS_PER_LONG;
	bitidx &= (BITS_PER_LONG-1);

	word = bitmap[word_bitidx];
	bitidx += end_bitidx;
	return (word >> (BITS_PER_LONG - bitidx - 1)) & mask;
}
```
> 居然，是使用bitmap 管理migrate type 的


```c
/* Return a pointer to the bitmap storing bits affecting a block of pages */
static inline unsigned long *get_pageblock_bitmap(struct page *page,
							unsigned long pfn)
{
#ifdef CONFIG_SPARSEMEM
	return __pfn_to_section(pfn)->pageblock_flags;
#else
	return page_zone(page)->pageblock_flags;
#endif /* CONFIG_SPARSEMEM */
}


static inline struct mem_section *__pfn_to_section(unsigned long pfn)
{
	return __nr_to_section(pfn_to_section_nr(pfn));
}


static inline struct mem_section *__nr_to_section(unsigned long nr)
{
#ifdef CONFIG_SPARSEMEM_EXTREME
	if (!mem_section)
		return NULL;
#endif
	if (!mem_section[SECTION_NR_TO_ROOT(nr)])
		return NULL;
	return &mem_section[SECTION_NR_TO_ROOT(nr)][nr & SECTION_ROOT_MASK];
}

static inline unsigned long pfn_to_section_nr(unsigned long pfn)
{
  // 似乎pfn 按照 27bit 形成一个区间，来管理全部section
	return pfn >> PFN_SECTION_SHIFT;
}
```
> 好吧，居然又看到了mem_section 了，而且似乎mem_setion 的功能变得诡异
> mem_section 的物理排布整个完全不清楚

```c
struct mem_section {
	/*
	 * This is, logically, a pointer to an array of struct
	 * pages.  However, it is stored with some other magic.
	 * (see sparse.c::sparse_init_one_section())
	 *
	 * Additionally during early boot we encode node id of
	 * the location of the section here to guide allocation.
	 * (see sparse.c::memory_present())
	 *
	 * Making it a UL at least makes someone do a cast
	 * before using it wrong.
	 */
	unsigned long section_mem_map;

	/* See declaration of similar field in struct zone */
	unsigned long *pageblock_flags;
	/*
	 * WARNING: mem_section must be a power-of-2 in size for the
	 * calculation and use of SECTION_ROOT_MASK to make sense.
	 */
};
```
> 实际上，mem_section 包含的内容很少 !
> pageblock_flags 初始化简单。


```c
static void __meminit sparse_init_one_section(struct mem_section *ms,
		unsigned long pnum, struct page *mem_map,
		unsigned long *pageblock_bitmap)
{
	ms->section_mem_map &= ~SECTION_MAP_MASK;
	ms->section_mem_map |= sparse_encode_mem_map(mem_map, pnum) |
							SECTION_HAS_MEM_MAP;
 	ms->pageblock_flags = pageblock_bitmap;
}
```


> 对于每一个node的初始化，同时会初始化page的flags相关的内容
```c
/*
 * Initialize sparse on a specific node. The node spans [pnum_begin, pnum_end)
 * And number of present sections in this node is map_count.
 */
static void __init sparse_init_nid(int nid, unsigned long pnum_begin,
				   unsigned long pnum_end,
				   unsigned long map_count)
{
	unsigned long pnum, usemap_longs, *usemap;
	struct page *map;

	usemap_longs = BITS_TO_LONGS(SECTION_BLOCKFLAGS_BITS);
	usemap = sparse_early_usemaps_alloc_pgdat_section(NODE_DATA(nid),
							  usemap_size() *
							  map_count);
	if (!usemap) {
		pr_err("%s: node[%d] usemap allocation failed", __func__, nid);
		goto failed;
	}
	sparse_buffer_init(map_count * section_map_size(), nid);
	for_each_present_section_nr(pnum_begin, pnum) {
		if (pnum >= pnum_end)
			break;

		map = sparse_mem_map_populate(pnum, nid, NULL);
		if (!map) {
			pr_err("%s: node[%d] memory map backing failed. Some memory will not be available.",
			       __func__, nid);
			pnum_begin = pnum;
			goto failed;
		}
		check_usemap_section_nr(nid, usemap);
		sparse_init_one_section(__nr_to_section(pnum), pnum, map, usemap);
		usemap += usemap_longs;
	}
	sparse_buffer_fini();
	return;
failed:
	/* We failed to allocate, mark all the following pnums as not present */
	for_each_present_section_nr(pnum_begin, pnum) {
		struct mem_section *ms;

		if (pnum >= pnum_end)
			break;
		ms = __nr_to_section(pnum);
		ms->section_mem_map = 0;
	}
}

/*
 * Allocate the accumulated non-linear sections, allocate a mem_map
 * for each and record the physical to section mapping.
 */
void __init sparse_init(void)

// 很好，又回来了
void __init paging_init(void)
```

> pageblock_flags 应该是给zone 使用，毕竟在memzone 中间定义的，但是TMD 初始化似乎是基于node 的!


```c
/* Bit indices that affect a whole block of pages */
enum pageblock_bits {
	PB_migrate,
	PB_migrate_end = PB_migrate + 3 - 1,
			/* 3 bits required for migrate types */
	PB_migrate_skip,/* If set the block is skipped by compaction */

	/*
	 * Assume the bits will always align on a word. If this assumption
	 * changes then get/set pageblock needs updating.
	 */
	NR_PAGEBLOCK_BITS
};
```
> 感觉pageblock_flags 就是用来处理migration 的 ?
> compaction 是做什么的?

mem_section => pageblock_flags 


> 似乎就是简单的分析gfp_t来分析migration，因为gfp其实在 alloc_pages 参数加以指定。
```c
static inline int gfpflags_to_migratetype(const gfp_t gfp_flags)
{
	VM_WARN_ON((gfp_flags & GFP_MOVABLE_MASK) == GFP_MOVABLE_MASK);
	BUILD_BUG_ON((1UL << GFP_MOVABLE_SHIFT) != ___GFP_MOVABLE);
	BUILD_BUG_ON((___GFP_MOVABLE >> GFP_MOVABLE_SHIFT) != MIGRATE_MOVABLE);

	if (unlikely(page_group_by_mobility_disabled))
		return MIGRATE_UNMOVABLE;

	/* Group based on mobility */
	return (gfp_flags & GFP_MOVABLE_MASK) >> GFP_MOVABLE_SHIFT;
}


// 当前环境信息以及gfp_t的信息在此处完成
static inline bool prepare_alloc_pages(gfp_t gfp_mask, unsigned int order,
		int preferred_nid, nodemask_t *nodemask,
		struct alloc_context *ac, gfp_t *alloc_mask,
		unsigned int *alloc_flags)
{
```


```c
/**
 * DOC: Useful GFP flag combinations
 *
 * Useful GFP flag combinations
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Useful GFP flag combinations that are commonly used. It is recommended
 * that subsystems start with one of these combinations and then set/clear
 * %__GFP_FOO flags as necessary.
 *
 * %GFP_ATOMIC users can not sleep and need the allocation to succeed. A lower
 * watermark is applied to allow access to "atomic reserves"
 *
 * %GFP_KERNEL is typical for kernel-internal allocations. The caller requires
 * %ZONE_NORMAL or a lower zone for direct access but can direct reclaim.
 *
 * %GFP_KERNEL_ACCOUNT is the same as GFP_KERNEL, except the allocation is
 * accounted to kmemcg.
 *
 * %GFP_NOWAIT is for kernel allocations that should not stall for direct
 * reclaim, start physical IO or use any filesystem callback.
 *
 * %GFP_NOIO will use direct reclaim to discard clean pages or slab pages
 * that do not require the starting of any physical IO.
 * Please try to avoid using this flag directly and instead use
 * memalloc_noio_{save,restore} to mark the whole scope which cannot
 * perform any IO with a short explanation why. All allocation requests
 * will inherit GFP_NOIO implicitly.
 *
 * %GFP_NOFS will use direct reclaim but will not use any filesystem interfaces.
 * Please try to avoid using this flag directly and instead use
 * memalloc_nofs_{save,restore} to mark the whole scope which cannot/shouldn't
 * recurse into the FS layer with a short explanation why. All allocation
 * requests will inherit GFP_NOFS implicitly.
 *
 * %GFP_USER is for userspace allocations that also need to be directly
 * accessibly by the kernel or hardware. It is typically used by hardware
 * for buffers that are mapped to userspace (e.g. graphics) that hardware
 * still must DMA to. cpuset limits are enforced for these allocations.
 *
 * %GFP_DMA exists for historical reasons and should be avoided where possible.
 * The flags indicates that the caller requires that the lowest zone be
 * used (%ZONE_DMA or 16M on x86-64). Ideally, this would be removed but
 * it would require careful auditing as some users really require it and
 * others use the flag to avoid lowmem reserves in %ZONE_DMA and treat the
 * lowest zone as a type of emergency reserve.
 *
 * %GFP_DMA32 is similar to %GFP_DMA except that the caller requires a 32-bit
 * address.
 *
 * %GFP_HIGHUSER is for userspace allocations that may be mapped to userspace,
 * do not need to be directly accessible by the kernel but that cannot
 * move once in use. An example may be a hardware allocation that maps
 * data directly into userspace but has no addressing limitations.
 *
 * %GFP_HIGHUSER_MOVABLE is for userspace allocations that the kernel does not
 * need direct access to but can use kmap() when access is required. They
 * are expected to be movable via page reclaim or page migration. Typically,
 * pages on the LRU would also be allocated with %GFP_HIGHUSER_MOVABLE.
 *
 * %GFP_TRANSHUGE and %GFP_TRANSHUGE_LIGHT are used for THP allocations. They
 * are compound allocations that will generally fail quickly if memory is not
 * available and will not wake kswapd/kcompactd on failure. The _LIGHT
 * version does not attempt reclaim/compaction at all and is by default used
 * in page fault path, while the non-light is used by khugepaged.
 */
```

```c
#define get_pageblock_migratetype(page)					\
	get_pfnblock_flags_mask(page, page_to_pfn(page),		\
			PB_migrate_end, MIGRATETYPE_MASK)
```

> 所以一个page 被放到 movable 和 unmovable 中间的区别是什么 ?

> movable_zone 居然是administer 指定的，我TM !


> cpu_set 想要指出的问题在于，为什么分配page 需要和CPU 挂钩，除非的确在假定使用物理节点
```c
/*
 * Drain pcplists of the indicated processor and zone.
 *
 * The processor must either be the current processor and the
 * thread pinned to the current processor or a processor that
 * is not online.
 */
static void drain_pages_zone(unsigned int cpu, struct zone *zone)

/*
 * Frees a number of pages from the PCP lists
 * Assumes all pages on list are in same zone, and of same order.
 * count is the number of pages to free.
 *
 * If the zone was previously in an "all pages pinned" state then look to
 * see if this freeing clears that state.
 *
 * And clear the zone's pages_scanned counter, to hold off the "all pages are
 * pinned" detection logic.
 */
static void free_pcppages_bulk(struct zone *zone, int count,
					struct per_cpu_pages *pcp)
```

## todo
> 回答一下问题可以推进代码的分析

1. 读一下文档，到底hot cold 中间的cache 是什么cache, 对于numa 的cache 还是就是 cpu 的cache ?

2. buddy system　其实很容易导致 fragmentation，fragmentation 的其实最显著的问题就是，即使拥有足够的内存，但是依旧没有内存用于分配order > 0 的内容。

https://linuxplumbersconf.org/event/2/contributions/65/attachments/15/171/slides-expanded.pdf
> 似乎解释了 compaction 的含义

- Isolates movable pages from their LRU lists
- Fallback to other type when matching pageblocks full (实际上就是划分为一个有一个区域，一个区域使用完成，然后向下一个区域，而不是说都是真的)
- Each marked as MOVABLE, UNMOVABLE or RECLAIMABLE migratetype (there are few more for other purposes
- Zones divided to pageblocks (order-9 = 2MB on x86) 我怀疑是page block中间含有两个内容
- Isolates free pages from buddy allocator (splits as needed)

> 1. migrate 从哪里迁移到哪里? 确定谁需要被迁移 !
> 2. 怀疑pageblock 的大小就是最大分配的连续的物理内存的大小 !
> 3. pcplist 是做什么的 ?
> 4. watermark 的作用 ?


> 解释一下isolate movable 以及 lru list

```c
int PageMovable(struct page *page)
{
	struct address_space *mapping;

	VM_BUG_ON_PAGE(!PageLocked(page), page);
  // 似乎movabel 不能给ano page 标记
	if (!__PageMovable(page))
		return 0;

	mapping = page_mapping(page);
  // ??? 只要isolate_page 就是movable，何时注册的?
	if (mapping && mapping->a_ops && mapping->a_ops->isolate_page)
		return 1;

	return 0;
}
EXPORT_SYMBOL(PageMovable);

static __always_inline int __PageMovable(struct page *page)
{
	return ((unsigned long)page->mapping & PAGE_MAPPING_FLAGS) ==
				PAGE_MAPPING_MOVABLE;
}


/*
 * On an anonymous page mapped into a user virtual memory area,
 * page->mapping points to its anon_vma, not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.  See rmap.h.
 *
 * On an anonymous page in a VM_MERGEABLE area, if CONFIG_KSM is enabled,
 * the PAGE_MAPPING_MOVABLE bit may be set along with the PAGE_MAPPING_ANON
 * bit; and then page->mapping points, not to an anon_vma, but to a private
 * structure which KSM associates with that merged page.  See ksm.h.
 *
 * PAGE_MAPPING_KSM without PAGE_MAPPING_ANON is used for non-lru movable
 * page and then page->mapping points a struct address_space.
 *
 * Please note that, confusingly, "page_mapping" refers to the inode
 * address_space which maps the page from disk; whereas "page_mapped"
 * refers to user virtual address space into which the page is mapped.
 */
#define PAGE_MAPPING_ANON	0x1
#define PAGE_MAPPING_MOVABLE	0x2
#define PAGE_MAPPING_KSM	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
#define PAGE_MAPPING_FLAGS	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
```



```c
/*
 * migrate_pages - migrate the pages specified in a list, to the free pages
 *		   supplied as the target for the page migration
 *
 * @from:		The list of pages to be migrated.
 * @get_new_page:	The function used to allocate free pages to be used
 *			as the target of the page migration.
 * @put_new_page:	The function used to free target pages if migration
 *			fails, or NULL if no special handling is necessary.
 * @private:		Private data to be passed on to get_new_page()
 * @mode:		The migration mode that specifies the constraints for
 *			page migration, if any.
 * @reason:		The reason for page migration.
 *
 * The function returns after 10 attempts or if no pages are movable any more
 * because the list has become empty or no retryable pages exist any more.
 * The caller should call putback_movable_pages() to return pages to the LRU
 * or free list only if ret != 0.
 *
 * Returns the number of pages that were not migrated, or an error code.
 */
int migrate_pages(struct list_head *from, new_page_t get_new_page,
		free_page_t put_new_page, unsigned long private,
		enum migrate_mode mode, int reason)
{


// compaction 唯一调用，功能似乎就是收集page 然后移动到特定的位置
static enum compact_result compact_zone(struct zone *zone, struct compact_control *cc)
// 后面还有几个函数来调用，都是简单的接口的封装，值的关注的内容是：存在一个daemon在不停调用这一个函数。


/*
 * Isolate all pages that can be migrated from the first suitable block,
 * starting at the block pointed to by the migrate scanner pfn within
 * compact_control.
 */
static isolate_migrate_t isolate_migratepages(struct zone *zone,
					struct compact_control *cc)
// 收集pages 似乎就是一个block逐渐收集的

/**
 * isolate_migratepages_block() - isolate all migrate-able pages within
 *				  a single pageblock
 * @cc:		Compaction control structure.
 * @low_pfn:	The first PFN to isolate
 * @end_pfn:	The one-past-the-last PFN to isolate, within same pageblock
 * @isolate_mode: Isolation mode to be used.
 *
 * Isolate all pages that can be migrated from the range specified by
 * [low_pfn, end_pfn). The range is expected to be within same pageblock.
 * Returns zero if there is a fatal signal pending, otherwise PFN of the
 * first page that was not scanned (which may be both less, equal to or more
 * than end_pfn).
 *
 * The pages are isolated on cc->migratepages list (not required to be empty),
 * and cc->nr_migratepages is updated accordingly. The cc->migrate_pfn field
 * is neither read nor updated.
 */
static unsigned long
isolate_migratepages_block(struct compact_control *cc, unsigned long low_pfn,
			unsigned long end_pfn, isolate_mode_t isolate_mode)
// shit 这是一个300行的函数!


/*
 * This is a migrate-callback that "allocates" freepages by taking pages
 * from the isolated freelists in the block we are migrating to.
 */
static struct page *compaction_alloc(struct page *migratepage,
					unsigned long data)
// migrate page 的关键一个环节，其他两个对称功能在 mempolicy 中间

// 似乎没有想象的那么复杂，就是之前分析的，找到page 然后调用 migrate 移动过去!
// 所以 isolate 到底是什么 、


// @todo a_ops 中间的几个函数到底是做什么的 ?
// 将会进一步 isolate_migratepages_block 调用
int isolate_movable_page(struct page *page, isolate_mode_t mode)
{
	struct address_space *mapping;

	/*
	 * Avoid burning cycles with pages that are yet under __free_pages(),
	 * or just got freed under us.
	 *
	 * In case we 'win' a race for a movable page being freed under us and
	 * raise its refcount preventing __free_pages() from doing its job
	 * the put_page() at the end of this block will take care of
	 * release this page, thus avoiding a nasty leakage.
	 */
	if (unlikely(!get_page_unless_zero(page)))
		goto out;

	/*
	 * Check PageMovable before holding a PG_lock because page's owner
	 * assumes anybody doesn't touch PG_lock of newly allocated page
	 * so unconditionally grapping the lock ruins page's owner side.
	 */
	if (unlikely(!__PageMovable(page)))
		goto out_putpage;
	/*
	 * As movable pages are not isolated from LRU lists, concurrent
	 * compaction threads can race against page migration functions
	 * as well as race against the releasing a page.
	 *
	 * In order to avoid having an already isolated movable page
	 * being (wrongly) re-isolated while it is under migration,
	 * or to avoid attempting to isolate pages being released,
	 * lets be sure we have the page lock
	 * before proceeding with the movable page isolation steps.
	 */
	if (unlikely(!trylock_page(page)))
		goto out_putpage;

	if (!PageMovable(page) || PageIsolated(page))
		goto out_no_isolated;

	mapping = page_mapping(page);
	VM_BUG_ON_PAGE(!mapping, page);

	if (!mapping->a_ops->isolate_page(page, mode))
		goto out_no_isolated;

	/* Driver shouldn't use PG_isolated bit of page->flags */
	WARN_ON_ONCE(PageIsolated(page));
	__SetPageIsolated(page);
	unlock_page(page);

	return 0;

out_no_isolated:
	unlock_page(page);
out_putpage:
	put_page(page);
out:
	return -EBUSY;
}
```


Documentation/vm/unevictable-lru.rst

Documentation/vm/page_migration.rst

> compaction 负责处理确定，migrate 负责迁移。

```c
// 在分配的时候确定 gfp_t 的时候确定! @todo 指令分配者如何知道自己如何使用
static inline bool prepare_alloc_pages(gfp_t gfp_mask, unsigned int order,
		int preferred_nid, nodemask_t *nodemask,
		struct alloc_context *ac, gfp_t *alloc_mask,
		unsigned int *alloc_flags)
{
```

fallback ? pageblock ?
```c
struct free_area {
	struct list_head	free_list[MIGRATE_TYPES];
	unsigned long		nr_free; //　@todo 作用是什么?
};

/* free areas of different sizes */
struct free_area	free_area[MAX_ORDER];

// free 操作如何实现 ? 链表持有顺序，merge
```

> page_block
```c
// migratetype 设置pageblock, 常用函数
void set_pageblock_migratetype(struct page *page, int migratetype)

/**
 * set_pfnblock_flags_mask - Set the requested group of flags for a pageblock_nr_pages block of pages
 * @page: The page within the block of interest
 * @flags: The flags to set
 * @pfn: The target page frame number
 * @end_bitidx: The last bit of interest
 * @mask: mask of bits that the caller is interested in
 */
void set_pfnblock_flags_mask(struct page *page, unsigned long flags,
					unsigned long pfn,
					unsigned long end_bitidx,
					unsigned long mask)
```

> steal 机制
```c
/*
 * This function implements actual steal behaviour. If order is large enough,
 * we can steal whole pageblock. If not, we first move freepages in this
 * pageblock to our migratetype and determine how many already-allocated pages
 * are there in the pageblock with a compatible migratetype. If at least half
 * of pages are free or compatible, we can change migratetype of the pageblock
 * itself, so pages freed in the future will be put on the correct free list.
 */
static void steal_suitable_fallback(struct zone *zone, struct page *page,
					int start_type, bool whole_block)
// 根据空闲数目不同而使用不同数值
// 会被下一个函数调用!


/*
 * Try finding a free buddy page on the fallback list and put it on the free
 * list of requested migratetype, possibly along with other pages from the same
 * block, depending on fragmentation avoidance heuristics. Returns true if
 * fallback was found so that __rmqueue_smallest() can grab it.
 *
 * The use of signed ints for order and current_order is a deliberate
 * deviation from the rest of this file, to make the for loop
 * condition simpler.
 */
static __always_inline bool
__rmqueue_fallback(struct zone *zone, int order, int start_migratetype)
// 进而被__rmqueue 使用
```

pcp pcplist : order = 0 使用pcplist
> 同意书上说的内容。但是具体能否模拟cache 不知道如何实现!
```c
/*
 * Allocate a page from the given zone. Use pcplists for order-0 allocations.
 */
static inline
struct page *rmqueue(struct zone *preferred_zone,
			struct zone *zone, unsigned int order,
			gfp_t gfp_flags, unsigned int alloc_flags,
			int migratetype)
{
	unsigned long flags;
	struct page *page;

	if (likely(order == 0)) {
		page = rmqueue_pcplist(preferred_zone, zone, order,
				gfp_flags, migratetype);
		goto out;
	}
```

The importance of these **watermarks will mainly show in Chapter 18**, but they also come into play in Section 3.5.5.
> 实际上，watermark 其实用作 swap 机制，vmscan 中间使用


# 总结一下
1. pcplist pageset -> cache
2. watermark  -> swap
3. migrate compaction fallback -> fragmentation
