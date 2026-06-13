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
