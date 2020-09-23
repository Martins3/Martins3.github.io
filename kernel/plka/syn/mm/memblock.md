# mm/memblock.c

## TODO
1. 其中的设计思想是什么 ?
    1. 总体来说 : 将 free 和 allocated 的保存在两个数组中间
2. 我想知道探测内存遇到 numa 是如何处理的 ?
3. 三种 memblock_type 的含义是什么 : E820_TYPE_SOFT_RESERVED 会被调用 memblock_reserve 中间 ?

1. 那么开始的时候，内核的代码段，stack 的空间是被标注为 reserve 还是 memory 的 ?
    1. 应该是 memory 吧 ? 毕竟似乎连 `_etext` 都没有出现过 !
    2. 那么 reserve 的含义是什么 ? 为什么 e820 的内容会探测出来这个 ? 找一下手册
## Doc
- [A quick history of early-boot memory allocators](https://lwn.net/Articles/761215/)
- `memblock.c`

```c
/**
 * DOC: memblock overview
 *
 * Memblock is a method of managing memory regions during the early
 * boot period when the usual kernel memory allocators are not up and
 * running.
 *
 * Memblock views the system memory as collections of contiguous
 * regions. There are several types of these collections:
 *
 * * ``memory`` - describes the physical memory available to the
 *   kernel; this may differ from the actual physical memory installed
 *   in the system, for instance when the memory is restricted with
 *   ``mem=`` command line parameter
 * * ``reserved`` - describes the regions that were allocated
 * * ``physmap`` - describes the actual physical memory regardless of
 *   the possible restrictions; the ``physmap`` type is only available
 *   on some architectures.
 *
 * Each region is represented by :c:type:`struct memblock_region` that
 * defines the region extents, its attributes and NUMA node id on NUMA
 * systems. Every memory type is described by the :c:type:`struct
 * memblock_type` which contains an array of memory regions along with
 * the allocator metadata. The memory types are nicely wrapped with
 * :c:type:`struct memblock`. This structure is statically initialzed
 * at build time. The region arrays for the "memory" and "reserved"
 * types are initially sized to %INIT_MEMBLOCK_REGIONS and for the
 * "physmap" type to %INIT_PHYSMEM_REGIONS.
 * The :c:func:`memblock_allow_resize` enables automatic resizing of
 * the region arrays during addition of new regions. This feature
 * should be used with care so that memory allocated for the region
 * array will not overlap with areas that should be reserved, for
 * example initrd.
 *
 * The early architecture setup should tell memblock what the physical
 * memory layout is by using :c:func:`memblock_add` or
 * :c:func:`memblock_add_node` functions. The first function does not
 * assign the region to a NUMA node and it is appropriate for UMA
 * systems. Yet, it is possible to use it on NUMA systems as well and
 * assign the region to a NUMA node later in the setup process using
 * :c:func:`memblock_set_node`. The :c:func:`memblock_add_node`
 * performs such an assignment directly.
 *
 * Once memblock is setup the memory can be allocated using one of the
 * API variants:
 *
 * * :c:func:`memblock_phys_alloc*` - these functions return the
 *   **physical** address of the allocated memory
 * * :c:func:`memblock_alloc*` - these functions return the **virtual**
 *   address of the allocated memory.
 *
 * Note, that both API variants use implict assumptions about allowed
 * memory ranges and the fallback methods. Consult the documentation
 * of :c:func:`memblock_alloc_internal` and
 * :c:func:`memblock_alloc_range_nid` functions for more elaboarte
 * description.
 *
 * As the system boot progresses, the architecture specific
 * :c:func:`mem_init` function frees all the memory to the buddy page
 * allocator.
 *
 * If an architecure enables %CONFIG_ARCH_DISCARD_MEMBLOCK, the
 * memblock data structures will be discarded after the system
 * initialization compltes.
 */
```



> 当添加新 region 的时候 : 
```c
/**
 * memblock_double_array - double the size of the memblock regions array
 * @type: memblock type of the regions array being doubled
 * @new_area_start: starting address of memory range to avoid overlap with
 * @new_area_size: size of memory range to avoid overlap with
 *
 * Double the size of the @type regions array. If memblock is being used to
 * allocate memory for a new reserved regions array and there is a previously
 * allocated memory range [@new_area_start, @new_area_start + @new_area_size]
 * waiting to be reserved, ensure the memory used by the new array does
 * not overlap.
 *
 * Return:
 * 0 on success, -1 on failure.
 */
static int __init_memblock memblock_double_array(struct memblock_type *type,
						phys_addr_t new_area_start,
						phys_addr_t new_area_size)
```

        mem=nn[KMG]     [KNL,BOOT] Force usage of a specific amount of memory
                        Amount of memory to be used when the kernel is not able
                        to see the whole system memory or for test.
                        [X86] Work as limiting max address. Use together
                        with memmap= to avoid physical address space collisions.
                        Without memmap= PCI devices could be placed at addresses
                        belonging to unused RAM.

        mem=nopentium   [BUGS=X86-32] Disable usage of 4MB pages for kernel
                        memory.


## memblock_add_range && memblock_remove_range

```c
/**
 * memblock_add_node - add new memblock region within a NUMA node
 * @base: base address of the new region
 * @size: size of the new region
 * @nid: nid of the new region
 *
 * Add new memblock region [@base, @base + @size) to the "memory"
 * type. See memblock_add_range() description for mode details
 *
 * Return:
 * 0 on success, -errno on failure.
 */
int __init_memblock memblock_add_node(phys_addr_t base, phys_addr_t size,
				       int nid)
{
	return memblock_add_range(&memblock.memory, base, size, nid, 0);
}

/**
 * memblock_add - add new memblock region
 * @base: base address of the new region
 * @size: size of the new region
 *
 * Add new memblock region [@base, @base + @size) to the "memory"
 * type. See memblock_add_range() description for mode details
 *
 * Return:
 * 0 on success, -errno on failure.
 */
int __init_memblock memblock_add(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("%s: [%pa-%pa] %pS\n", __func__,
		     &base, &end, (void *)_RET_IP_);

	return memblock_add_range(&memblock.memory, base, size, MAX_NUMNODES, 0);
}
```

> 1. MAX_NUMNODES 的含义是 ?
> 2. flag  ?
```c
/**
 * memblock_merge_regions - merge neighboring compatible regions
 * @type: memblock type to scan
 *
 * Scan @type and merge neighboring compatible regions.
 */
static void __init_memblock memblock_merge_regions(struct memblock_type *type)
{
	int i = 0;

	/* cnt never goes below 1 */
	while (i < type->cnt - 1) {
		struct memblock_region *this = &type->regions[i];
		struct memblock_region *next = &type->regions[i + 1];

    // 需要保证的在同一个node , 物理地址连续，flag 相同
		if (this->base + this->size != next->base ||
		    memblock_get_region_node(this) !=
		    memblock_get_region_node(next) ||
		    this->flags != next->flags) {
			BUG_ON(this->base + this->size > next->base);
			i++;
			continue;
		}

		this->size += next->size;
		/* move forward from next + 1, index of which is i + 2 */
    // 当没有链表的时候，原来删除一个元素可以这么酷炫 !
		memmove(next, next + 1, (type->cnt - (i + 2)) * sizeof(*next));
		type->cnt--;
	}
}

/**
 * memblock_insert_region - insert new memblock region
 * @type:	memblock type to insert into
 * @idx:	index for the insertion point
 * @base:	base address of the new region
 * @size:	size of the new region
 * @nid:	node id of the new region
 * @flags:	flags of the new region
 *
 * Insert new memblock region [@base, @base + @size) into @type at @idx.
 * @type must already have extra room to accommodate the new region.
 */
static void __init_memblock memblock_insert_region(struct memblock_type *type,
						   int idx, phys_addr_t base,
						   phys_addr_t size,
						   int nid,
						   enum memblock_flags flags)
{
	struct memblock_region *rgn = &type->regions[idx];

	BUG_ON(type->cnt >= type->max);
	memmove(rgn + 1, rgn, (type->cnt - idx) * sizeof(*rgn));
	rgn->base = base;
	rgn->size = size;
	rgn->flags = flags;
	memblock_set_region_node(rgn, nid);
	type->cnt++;
	type->total_size += size;
}

/**
 * memblock_add_range - add new memblock region
 * @type: memblock type to add new region into
 * @base: base address of the new region
 * @size: size of the new region
 * @nid: nid of the new region
 * @flags: flags of the new region
 *
 * Add new memblock region [@base, @base + @size) into @type.  The new region
 * is allowed to overlap with existing ones - overlaps don't affect already
 * existing regions.  @type is guaranteed to be minimal (all neighbouring
 * compatible regions are merged) after the addition.
 *
 * Return:
 * 0 on success, -errno on failure.
 */
static int __init_memblock memblock_add_range(struct memblock_type *type,
				phys_addr_t base, phys_addr_t size,
				int nid, enum memblock_flags flags)
{
	bool insert = false;
	phys_addr_t obase = base;
	phys_addr_t end = base + memblock_cap_size(base, &size);
	int idx, nr_new;
	struct memblock_region *rgn;

	if (!size)
		return 0;

	/* special case for empty array */
	if (type->regions[0].size == 0) {
		WARN_ON(type->cnt != 1 || type->total_size);
		type->regions[0].base = base;
		type->regions[0].size = size;
		type->regions[0].flags = flags;
		memblock_set_region_node(&type->regions[0], nid);
		type->total_size = size;
		return 0;
	}
repeat:
	/*
	 * The following is executed twice.  Once with %false @insert and
	 * then with %true.  The first counts the number of regions needed
	 * to accommodate the new area.  The second actually inserts them.
	 */
	base = obase;
	nr_new = 0;

	for_each_memblock_type(idx, type, rgn) {
		phys_addr_t rbase = rgn->base;
		phys_addr_t rend = rbase + rgn->size;

		if (rbase >= end)
			break;
		if (rend <= base)
			continue;

    // 如果到达此处，两者一定相交
    // 但是无法理解为什么 : rbase > base 才算 ?
		/*
		 * @rgn overlaps.  If it separates the lower part of new
		 * area, insert that portion.
		 */
		if (rbase > base) {
    // 认为当他们相交的时候，那么所在nid 和 flag 需要相同的
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
			WARN_ON(nid != memblock_get_region_node(rgn));
#endif
			WARN_ON(flags != rgn->flags);
			nr_new++;
			if (insert)
				memblock_insert_region(type, idx++, base,
						       rbase - base, nid,
						       flags);
		}
		/* area below @rend is dealt with, forget about it */
		base = min(rend, end);
	}

  // 当和任何部分没有任何相交的位置的时候，
	/* insert the remaining portion */
	if (base < end) {
		nr_new++;
		if (insert)
			memblock_insert_region(type, idx, base, end - base,
					       nid, flags);
	}

	if (!nr_new)
		return 0;

	/*
	 * If this was the first round, resize array and repeat for actual
	 * insertions; otherwise, merge and return.
	 */
	if (!insert) {
		while (type->cnt + nr_new > type->max)
			if (memblock_double_array(type, obase, size) < 0)
				return -ENOMEM;
		insert = true;
		goto repeat;
	} else {
    // 将之前拆分的
		memblock_merge_regions(type);
		return 0;
	}
}
```
> @todo 这个分析很有意思的，一种 leetcode 的感觉，但是不想在此处浪费时间。
> 其效果就是如此 : 形成链表，顺序从小到大，一个 region 描述一个连续物理地址空间。


> 第二部分 : 下面分析 memblock_remove_range :

```c
static int __init_memblock memblock_remove_range(struct memblock_type *type,
					  phys_addr_t base, phys_addr_t size)
{
	int start_rgn, end_rgn;
	int i, ret;

	ret = memblock_isolate_range(type, base, size, &start_rgn, &end_rgn); // 和 memblock_add_node 对称，找到与我相交的区域。
	if (ret)
		return ret;

	for (i = end_rgn - 1; i >= start_rgn; i--)
		memblock_remove_region(type, i);
	return 0;
}
```


## API : alloc && free

```c
void *memblock_alloc_exact_nid_raw(phys_addr_t size, phys_addr_t align,
				 phys_addr_t min_addr, phys_addr_t max_addr,
				 int nid);
void *memblock_alloc_try_nid_raw(phys_addr_t size, phys_addr_t align,
				 phys_addr_t min_addr, phys_addr_t max_addr,
				 int nid);
void *memblock_alloc_try_nid(phys_addr_t size, phys_addr_t align,
			     phys_addr_t min_addr, phys_addr_t max_addr,
			     int nid);

static inline void * __init memblock_alloc(phys_addr_t size,  phys_addr_t align)
{
	return memblock_alloc_try_nid(size, align, MEMBLOCK_LOW_LIMIT,
				      MEMBLOCK_ALLOC_ACCESSIBLE, NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_raw(phys_addr_t size,
					       phys_addr_t align)
{
	return memblock_alloc_try_nid_raw(size, align, MEMBLOCK_LOW_LIMIT,
					  MEMBLOCK_ALLOC_ACCESSIBLE,
					  NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_from(phys_addr_t size,
						phys_addr_t align,
						phys_addr_t min_addr)
{
	return memblock_alloc_try_nid(size, align, min_addr,
				      MEMBLOCK_ALLOC_ACCESSIBLE, NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_low(phys_addr_t size,
					       phys_addr_t align)
{
	return memblock_alloc_try_nid(size, align, MEMBLOCK_LOW_LIMIT,
				      ARCH_LOW_ADDRESS_LIMIT, NUMA_NO_NODE);
}

static inline void * __init memblock_alloc_node(phys_addr_t size,
						phys_addr_t align, int nid)
{
	return memblock_alloc_try_nid(size, align, MEMBLOCK_LOW_LIMIT,
				      MEMBLOCK_ALLOC_ACCESSIBLE, nid);
}

static inline void __init memblock_free_early(phys_addr_t base,
					      phys_addr_t size)
{
	memblock_free(base, size);
}

static inline void __init memblock_free_early_nid(phys_addr_t base,
						  phys_addr_t size, int nid)
{
	memblock_free(base, size);
}

static inline void __init memblock_free_late(phys_addr_t base, phys_addr_t size)
{
	__memblock_free_late(base, size);
}
```


> 来分析具体的实现 :

```c
/**
 * memblock_free - free boot memory block
 * @base: phys starting address of the  boot memory block
 * @size: size of the boot memory block in bytes
 *
 * Free boot memory block previously allocated by memblock_alloc_xx() API.
 * The freeing memory will not be released to the buddy allocator.
 */
int __init_memblock memblock_free(phys_addr_t base, phys_addr_t size)
{
	phys_addr_t end = base + size - 1;

	memblock_dbg("%s: [%pa-%pa] %pS\n", __func__,
		     &base, &end, (void *)_RET_IP_);

	kmemleak_free_part_phys(base, size);
	return memblock_remove_range(&memblock.reserved, base, size);
}


/**
 * memblock_alloc_try_nid - allocate boot memory block
 * @size: size of memory block to be allocated in bytes
 * @align: alignment of the region and block's size
 * @min_addr: the lower bound of the memory region from where the allocation
 *	  is preferred (phys address)
 * @max_addr: the upper bound of the memory region from where the allocation
 *	      is preferred (phys address), or %MEMBLOCK_ALLOC_ACCESSIBLE to
 *	      allocate only from memory limited by memblock.current_limit value
 * @nid: nid of the free area to find, %NUMA_NO_NODE for any node
 *
 * Public function, provides additional debug information (including caller
 * info), if enabled. This function zeroes the allocated memory.
 *
 * Return:
 * Virtual address of allocated memory block on success, NULL on failure.
 */
void * __init memblock_alloc_try_nid(
			phys_addr_t size, phys_addr_t align,
			phys_addr_t min_addr, phys_addr_t max_addr,
			int nid)
{
	void *ptr;

	memblock_dbg("%s: %llu bytes align=0x%llx nid=%d from=%pa max_addr=%pa %pS\n",
		     __func__, (u64)size, (u64)align, nid, &min_addr,
		     &max_addr, (void *)_RET_IP_);
	ptr = memblock_alloc_internal(size, align,
					   min_addr, max_addr, nid, false);
	if (ptr)
		memset(ptr, 0, size);

	return ptr;
}

// todo 返回值是虚拟地址意味着什么 ?
// 1. 那么物理地址和虚拟地址的线性映射已经建立好了。
// 2. 还有的是 :
```

剩下的部分调用:
1. memblock_find_in_range_node
2. memblock_reserve

当然中间出现很多的检查，但是就是利用两个链表维持生活的。

## memblock_free_all : 将 page 给 buddy allocator
1. 其实非常奇怪，难道不是尽早建立好 buddy system 吗 ?
2. 总体来说 : 将 memblock 中间 reserve_type 的那些区间标记上 reserve，将 memory_type 的区间通过 `__free_pages` 的方法释放到 buddy system 中间

```c
/**
 * memblock_free_all - release free pages to the buddy allocator
 *
 * Return: the number of pages actually released.
 */
unsigned long __init memblock_free_all(void)
{
	unsigned long pages;

	reset_all_zones_managed_pages(); // 将所有的node上的所有zone上的 managed_pages 设置为0 todo 但是并不知道 managed_pages 的作用是什么 ?

	pages = free_low_memory_core_early();
	totalram_pages_add(pages);

	return pages;
}

void __init reset_all_zones_managed_pages(void)
{
	struct pglist_data *pgdat;

	if (reset_managed_pages_done)
		return;

	for_each_online_pgdat(pgdat)
		reset_node_managed_pages(pgdat);

	reset_managed_pages_done = 1;
}
```

1. reset_all_zones_managed_pages
```c
/**
 * for_each_online_pgdat - helper macro to iterate over all online nodes
 * @pgdat - pointer to a pg_data_t variable
 */
#define for_each_online_pgdat(pgdat)			\
	for (pgdat = first_online_pgdat();		\
	     pgdat;					\
	     pgdat = next_online_pgdat(pgdat))

// todo 这些 oneline node 的到底是如何分析出来的 ?

void reset_node_managed_pages(pg_data_t *pgdat)
{
	struct zone *z;

	for (z = pgdat->node_zones; z < pgdat->node_zones + MAX_NR_ZONES; z++)
		atomic_long_set(&z->managed_pages, 0);
}
```

2. free_low_memory_core_early
```c
static unsigned long __init free_low_memory_core_early(void)
{
	unsigned long count = 0;
	phys_addr_t start, end;
	u64 i;

	memblock_clear_hotplug(0, -1);

	for_each_reserved_mem_region(i, &start, &end)
      reserve_bootmem_region(start, end); // 将从 start 到 end 对应的 struct page 标记上为 reserve 的，todo 所以 reserve 的含义就是表示被使用，并且拒绝被 buddy allocator 管理 ?

	/*
	 * We need to use NUMA_NO_NODE instead of NODE_DATA(0)->node_id
	 *  because in some case like Node0 doesn't have RAM installed
	 *  low ram will be on Node1
	 */
	for_each_free_mem_range(i, NUMA_NO_NODE, MEMBLOCK_NONE, &start, &end,
				NULL)
		count += __free_memory_core(start, end);

	return count;
}

static unsigned long __init __free_memory_core(phys_addr_t start,
				 phys_addr_t end)
{
	unsigned long start_pfn = PFN_UP(start);
	unsigned long end_pfn = min_t(unsigned long,
				      PFN_DOWN(end), max_low_pfn);

	if (start_pfn >= end_pfn)
		return 0;

	__free_pages_memory(start_pfn, end_pfn);

	return end_pfn - start_pfn;
}


static void __init __free_pages_memory(unsigned long start, unsigned long end)
{
	int order;

	while (start < end) {
		order = min(MAX_ORDER - 1UL, __ffs(start)); // 正如我们的猜测，对于连续的区间，就是尽可能的从最可能的大小键入链表中间

		while (start + (1UL << order) > end)
			order--;

		memblock_free_pages(pfn_to_page(start), start, order);

		start += (1UL << order);
	}
}


void __init memblock_free_pages(struct page *page, unsigned long pfn,
							unsigned int order)
{
	if (early_page_uninitialised(pfn)) //　未配置 CONFIG_DEFERRED_STRUCT_PAGE_INIT ，此内容为空
		return;
	__free_pages_core(page, order);
}

void __free_pages_core(struct page *page, unsigned int order) // 设置 refcount 以及清理 Reserve flag
{
	unsigned int nr_pages = 1 << order;
	struct page *p = page;
	unsigned int loop;

	prefetchw(p);
	for (loop = 0; loop < (nr_pages - 1); loop++, p++) {
		prefetchw(p + 1);
		__ClearPageReserved(p);
		set_page_count(p, 0);
	}
	__ClearPageReserved(p); // 最后的一个位置不用 prefetchw
	set_page_count(p, 0);

	atomic_long_add(nr_pages, &page_zone(page)->managed_pages); 
	set_page_refcounted(page); // 第一个page 的 refcoutn 需要特殊处理
	__free_pages(page, order);
}

void __free_pages(struct page *page, unsigned int order) // 这是一个通用的 free 函数，定义在 page_alloc 中间
{
	if (put_page_testzero(page))
		free_the_page(page, order); // struct page 的 flags 中间是保存了 zone id 和 node id 的，所以加入到哪里并不是问题!
}
EXPORT_SYMBOL(__free_pages);
```

## memblock_add_node : 来自 e820 的问候

```c
void __init e820__memblock_setup(void)
{
	int i;
	u64 end;

	/*
	 * The bootstrap memblock region count maximum is 128 entries
	 * (INIT_MEMBLOCK_REGIONS), but EFI might pass us more E820 entries
	 * than that - so allow memblock resizing.
	 *
	 * This is safe, because this call happens pretty late during x86 setup,
	 * so we know about reserved memory regions already. (This is important
	 * so that memblock resizing does no stomp over reserved areas.)
	 */
	memblock_allow_resize();

	for (i = 0; i < e820_table->nr_entries; i++) {  // 真的异常简单
		struct e820_entry *entry = &e820_table->entries[i];

		end = entry->addr + entry->size;
		if (end != (resource_size_t)end)
			continue;

		if (entry->type == E820_TYPE_SOFT_RESERVED)
			memblock_reserve(entry->addr, entry->size);

		if (entry->type != E820_TYPE_RAM && entry->type != E820_TYPE_RESERVED_KERN)
			continue;

		memblock_add(entry->addr, entry->size);
	}

	/* Throw away partial pages: */
	memblock_trim_memory(PAGE_SIZE);

	memblock_dump_all();  // debug 的辅助函数而已 !
}
```
