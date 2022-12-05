## list 从何时被注入 lru 中间

`lru_cache_add` is required only in `add_to_page_cache_lru` from `mm/filemap.c` and adds a page to
both the page cache and the LRU cache. **This is, however, the standard function to introduce a new
page both into the page cache and the LRU list.** Most importantly, it is used by `mpage_readpages` and
`do_generic_mapping_read`, the standard functions in which the block layer ends up when reading data
from a file or mapping.

## page 类型分类 以及 PageSwapBacked 的含义

```c
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

## 各种 page->flags 的含义
1. PG_reserved, PG_locked
2. PG_referenced, PG_reclaim

3. PG_swapcache : 表示该 page 被用于 swap cache , 在那个 radix tree 中间
4. PG_swapbacked : 该 page 被 swap backed ，在 disk 之类的含有备份。
    1. PageAnon : anon page
    2. @todo 所以，存在PG_swapcache 但是没有 PG_swapbacked 的吗 ?

5. PG_uptodate 和 PG_dirty
    1. PG_dirty 很容易理解
    2. PG_uptodate : filemap.c filemap_fault 中间，一个在 page cache 中间的 page 居然会不是 uptodate 的，既然所有的读写都是经过 page cache ，怎么可能内存中间的不是最新的
        1. 就算不是最新的，那么 cache 中间的内容如何保证 ?

```c
/*
 * Various page->flags bits:
 *
 * PG_reserved is set for special pages. The "struct page" of such a page
 * should in general not be touched (e.g. set dirty) except by its owner.
 * Pages marked as PG_reserved include:
 * - Pages part of the kernel image (including vDSO) and similar (e.g. BIOS,
 *   initrd, HW tables)
 * - Pages reserved or allocated early during boot (before the page allocator
 *   was initialized). This includes (depending on the architecture) the
 *   initial vmemmap, initial page tables, crashkernel, elfcorehdr, and much
 *   much more. Once (if ever) freed, PG_reserved is cleared and they will
 *   be given to the page allocator.
 * - Pages falling into physical memory gaps - not IORESOURCE_SYSRAM. Trying
 *   to read/write these pages might end badly. Don't touch!
 * - The zero page(s)
 * - Pages not added to the page allocator when onlining a section because
 *   they were excluded via the online_page_callback() or because they are
 *   PG_hwpoison.
 * - Pages allocated in the context of kexec/kdump (loaded kernel image,
 *   control pages, vmcoreinfo)
 * - MMIO/DMA pages. Some architectures don't allow to ioremap pages that are
 *   not marked PG_reserved (as they might be in use by somebody else who does
 *   not respect the caching strategy).
 * - Pages part of an offline section (struct pages of offline sections should
 *   not be trusted as they will be initialized when first onlined).
 * - MCA pages on ia64
 * - Pages holding CPU notes for POWER Firmware Assisted Dump
 * - Device memory (e.g. PMEM, DAX, HMM)
 * Some PG_reserved pages will be excluded from the hibernation image.
 * PG_reserved does in general not hinder anybody from dumping or swapping
 * and is no longer required for remap_pfn_range(). ioremap might require it.
 * Consequently, PG_reserved for a page mapped into user space can indicate
 * the zero page, the vDSO, MMIO pages or device memory.
 *
 * The PG_private bitflag is set on pagecache pages if they contain filesystem
 * specific data (which is normally at page->private). It can be used by
 * private allocations for its own usage.
 *
 * During initiation of disk I/O, PG_locked is set. This bit is set before I/O
 * and cleared when writeback _starts_ or when read _completes_. PG_writeback
 * is set before writeback starts and cleared when it finishes.
 *
 * PG_locked also pins a page in pagecache, and blocks truncation of the file
 * while it is held.
 *
 * page_waitqueue(page) is a wait queue of all tasks waiting for the page
 * to become unlocked.
 *
 * PG_uptodate tells whether the page's contents is valid.  When a read
 * completes, the page becomes uptodate, unless a disk I/O error happened.
 *
 * PG_referenced, PG_reclaim are used for page reclaim for anonymous and // @todo PG_reclaim 机制就像是从来没有使用过一样
 * file-backed pagecache (see mm/vmscan.c).
 *
 * PG_error is set to indicate that an I/O error occurred on this page.
 *
 * PG_arch_1 is an architecture specific page state bit.  The generic code
 * guarantees that this bit is cleared for a page when it first is entered into
 * the page cache.
 *
 * PG_hwpoison indicates that a page got corrupted in hardware and contains
 * data with incorrect ECC bits that triggered a machine check. Accessing is
 * not safe since it may cause another machine check. Don't touch!
 */

/*
 * Don't use the *_dontuse flags.  Use the macros.  Otherwise you'll break
 * locked- and dirty-page accounting.
 *
 * The page flags field is split into two parts, the main flags area
 * which extends from the low bits upwards, and the fields area which
 * extends from the high bits downwards.
 *
 *  | FIELD | ... | FLAGS |
 *  N-1           ^       0
 *               (NR_PAGEFLAGS)
 *
 * The fields area is reserved for fields mapping zone, node (for NUMA) and
 * SPARSEMEM section (for variants of SPARSEMEM that require section ids like
 * SPARSEMEM_EXTREME with !SPARSEMEM_VMEMMAP).
 */
enum pageflags {
	PG_locked,		/* Page is locked. Don't touch. */
	PG_referenced,
	PG_uptodate,
	PG_dirty,
	PG_lru,
	PG_active,
	PG_workingset,
	PG_waiters,		/* Page has waiters, check its waitqueue. Must be bit #7 and in the same byte as "PG_locked" */
	PG_error,
	PG_slab,
	PG_owner_priv_1,	/* Owner use. If pagecache, fs may use*/
	PG_arch_1,
	PG_reserved,
	PG_private,		/* If pagecache, has fs-private data */
	PG_private_2,		/* If pagecache, has fs aux data */
	PG_writeback,		/* Page is under writeback */
	PG_head,		/* A head page */
	PG_mappedtodisk,	/* Has blocks allocated on-disk */
	PG_reclaim,		/* To be reclaimed asap */
	PG_swapbacked,		/* Page is backed by RAM/swap */
	PG_unevictable,		/* Page is "unevictable"  */
#ifdef CONFIG_MMU
	PG_mlocked,		/* Page is vma mlocked */
#endif
#ifdef CONFIG_ARCH_USES_PG_UNCACHED
	PG_uncached,		/* Page has been mapped as uncached */
#endif
#ifdef CONFIG_MEMORY_FAILURE
	PG_hwpoison,		/* hardware poisoned page. Don't touch */
#endif
#if defined(CONFIG_IDLE_PAGE_TRACKING) && defined(CONFIG_64BIT)
	PG_young,
	PG_idle,
#endif
	__NR_PAGEFLAGS,

	/* Filesystems */
	PG_checked = PG_owner_priv_1,

	/* SwapBacked */
	PG_swapcache = PG_owner_priv_1,	/* Swap page: swp_entry_t in private */

	/* Two page bits are conscripted by FS-Cache to maintain local caching
	 * state.  These bits are set on pages belonging to the netfs's inodes
	 * when those inodes are being locally cached.
	 */
	PG_fscache = PG_private_2,	/* page backed by cache */

	/* XEN */
	/* Pinned in Xen as a read-only pagetable page. */
	PG_pinned = PG_owner_priv_1,
	/* Pinned as part of domain save (see xen_mm_pin_all()). */
	PG_savepinned = PG_dirty,
	/* Has a grant mapping of another (foreign) domain's page. */
	PG_foreign = PG_owner_priv_1,
	/* Remapped by swiotlb-xen. */
	PG_xen_remapped = PG_owner_priv_1,

	/* SLOB */
	PG_slob_free = PG_private,

	/* Compound pages. Stored in first tail page's flags */
	PG_double_map = PG_private_2,

	/* non-lru isolated movable page */
	PG_isolated = PG_reclaim,
};
```


## swap 相关的几个函数
1. delete_from_swap_cache


## vmscan.c 相关
1. try_to_unmap 唯一的正经的调用地方，vmscan.c : shrink_page_list


## 内核数据结构
https://lwn.net/Articles/336255/

https://kernelnewbies.org/FAQ/Hashtables

https://stackoverflow.com/questions/3058592/use-of-double-pointer-in-linux-kernel-hash-list-implementation


## Why we need PageUptodate ?
1. because someone else may access to the file simultaneously ?
    1. so how to inform the page ? (@todo should be easy!)
    2. file_operations::sync @todo
```c
static inline int PageUptodate(struct page *page)
{
	int ret;
	page = compound_head(page);
	ret = test_bit(PG_uptodate, &(page)->flags);
	/*
	 * Must ensure that the data we read out of the page is loaded
	 * _after_ we've loaded page->flags to check for PageUptodate.
	 * We can skip the barrier if the page is not uptodate, because
	 * we wouldn't be reading anything from it.
	 *
	 * See SetPageUptodate() for the other side of the story.
	 */
	if (ret)
		smp_rmb();

	return ret;
}
```

# lru

分析一下vmscan 的内容:
```c
f	unsigned long shrink_all_memory(unsigned long nr_to_reclaim)	[:3863 3]

f	unsigned long try_to_free_pages(struct zonelist *zonelist, int order,	[:3214 3]
f	unsigned long reclaim_clean_pages_from_list(struct zone *zone,	[:1509 3]　// 这一个函数唯一使用, 位置在page_alloc 中间，默认不启用

f	int remove_mapping(struct address_space *mapping, struct page *page)	[:972 3]
f	void putback_lru_page(struct page *page)	[:995 3]


f	void drop_slab_node(int nid)	[:717 3]
f	void drop_slab(void)	[:732 3]

f	unsigned long zone_reclaimable_pages(struct zone *zone)	[:319 3]
f	unsigned long lruvec_lru_size(struct lruvec *lruvec, enum lru_list lru, int zone_idx)	[:338 3]
f	int prealloc_shrinker(struct shrinker *shrinker)	[:370 3]
f	void free_prealloced_shrinker(struct shrinker *shrinker)	[:394 3]
f	void register_shrinker_prepared(struct shrinker *shrinker)	[:406 3]
```

## 线性地址的作用

```c
/**
 *	virt_to_phys	-	map virtual addresses to physical
 *	@address: address to remap
 *
 *	The returned physical address is the physical (CPU) mapping for
 *	the memory address given. It is only valid to use this function on
 *	addresses directly mapped or allocated via kmalloc.
 *
 *	This function does not give bus mappings for DMA transfers. In
 *	almost all conceivable cases a device driver should not be using
 *	this function
 */

static inline phys_addr_t virt_to_phys(volatile void *address)
{
	return __pa(address);
}
#define virt_to_phys virt_to_phys


#ifndef __pa
#define __pa(x)		__phys_addr((unsigned long)(x))
#endif

#define __phys_addr(x)		__phys_addr_nodebug(x)

static inline unsigned long __phys_addr_nodebug(unsigned long x)
{
	unsigned long y = x - __START_KERNEL_map;

	/* use the carry flag to determine if x was < __START_KERNEL_map */
	x = y + ((x > y) ? phys_base : (__START_KERNEL_map - PAGE_OFFSET));

	return x;
}


#define __START_KERNEL_map	_AC(0xffffffff80000000, UL)

#define PAGE_OFFSET		((unsigned long)__PAGE_OFFSET)

#ifdef CONFIG_DYNAMIC_MEMORY_LAYOUT
#define __PAGE_OFFSET           page_offset_base
#else
#define __PAGE_OFFSET           __PAGE_OFFSET_BASE_L4
#endif /* CONFIG_DYNAMIC_MEMORY_LAYOUT */


unsigned long page_offset_base __ro_after_init = __PAGE_OFFSET_BASE_L4;


/*
 * Set __PAGE_OFFSET to the most negative possible address +
 * PGDIR_SIZE*16 (pgd slot 272).  The gap is to allow a space for a
 * hypervisor to fit.  Choosing 16 slots here is arbitrary, but it's
 * what Xen requires.
 */
#define __PAGE_OFFSET_BASE_L5	_AC(0xff10000000000000, UL)
#define __PAGE_OFFSET_BASE_L4	_AC(0xffff880000000000, UL)
```

## `__SetPageSwapBacked` 和 `SetPageSwapBacked`
1. 和 SetPageSwapBacked 的关系 : 非上锁的版本
2. @todo 三个调用位置都非常的诡异啊!

```c
rmap.c : page_add_new_anon_rmap : @todo 这个函数的调用者简直就是一个神经病

memory.c : do_swap_page
```

3. `__read_swap_cache_async` 调用 `__SetPageSwapBacked`:

add_to_swap
1. 调用 add_to_swap_cache 用于实现 page 和 entry 关联(利用 radix tree)
    1. add_to_swap_cache 是 SetPageSwapCache 的唯三调用者，其余两个位置是 shmem.c 和 migrate.c 中间的，并没有什么实际的意义。
    2. 所以，可以说，当 page 被挂到 swap cache 中间的时候，那么这一个 flag 就会插上去。
