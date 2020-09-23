# mm/sparse.md

## keynote
1. 需要 sparse 的原因 : 内存中间出现大量空洞，如果使用 mem_map array 的构成 page frame (pf) 和 page descriptor (pd) 之间的关系
  那么浪费很多 pd 
2. 建立 pf 和 pd 之间的映射，所以从此之后，那些 pf 的通过 pd 被管理起来了，**根本不用在于这些物理地址是否被内核空间page table 映射的问题。**
    1. 内核会事先映射 896M 的空间，但是其余的物理空间是没有被线性映射的，但是被内核管理起来了。
    2. 因为内核被加载物理内存最开始的位置上，所以，内核管理的物理空间就是用于动态分配的。

巧妙的映射 : 以前依赖于，对于 mem_map 的偏移量和管理的 page 的编号是一一对应，进而形成互相一一对应的。

如何实现双向的映射。
1. pd 在物理空间还是连续的。
2. 非常的浪费虚拟地址空间。
3. 64bit 的空间中间，内核的虚拟地址空间和物理空间没有任何重合的可能性。
3. pd 到 pf (获取其管理的page frame 的物理地址)，只是需要对于某一个数值做差即可。
4. pf 到 pd 的方法类似


2. # define SUBSECTION_SHIFT 21
3. # define SECTION_SIZE_BITS	27 /* matt - 128 is convenient right now */
1. 
```c
#define PMD_SHIFT	21
#define HPAGE_SHIFT		PMD_SHIFT
#define HUGETLB_PAGE_ORDER	(HPAGE_SHIFT - PAGE_SHIFT)
/* Huge pages are a constant size */
#define pageblock_order		HUGETLB_PAGE_ORDER
```
> 居然，这些 pageblock 的大小和 subsection 的大小相同，所以其实两者没有什么关系 ?

3. 注意使用 vmemmap 的时候，分配物理空间还是考虑过numa 系统的

## todo && questions
1. 所以 memblock 之类是干什么的 ?
    1. 这么简单的内容为什么 需要 sparse 来 ?
    2. 

2. 由于 section 用于替代 mem_map ，其实不再是为了实现 pf pd 之间映射，而是为了别的管理
    1. 实现的


## Documentation
1. 找到 sparse 和 sparse_extreme的内容
2. 初始化的过程
3. memory hotplug 和 numa ?
4. include/asm-generic/memory_model.h 中间说明具体的内容

```
choice
	prompt "Memory model"
	depends on SELECT_MEMORY_MODEL
	default DISCONTIGMEM_MANUAL if ARCH_DISCONTIGMEM_DEFAULT
	default SPARSEMEM_MANUAL if ARCH_SPARSEMEM_DEFAULT
	default FLATMEM_MANUAL
	help
	  This option allows you to change some of the ways that
	  Linux manages its memory internally. Most users will
	  only have one option here selected by the architecture
	  configuration. This is normal.

config FLATMEM_MANUAL
	bool "Flat Memory"
	depends on !(ARCH_DISCONTIGMEM_ENABLE || ARCH_SPARSEMEM_ENABLE) || ARCH_FLATMEM_ENABLE
	help
	  This option is best suited for non-NUMA systems with
	  flat address space. The FLATMEM is the most efficient
	  system in terms of performance and resource consumption
	  and it is the best option for smaller systems.

	  For systems that have holes in their physical address
	  spaces and for features like NUMA and memory hotplug,
	  choose "Sparse Memory".

	  If unsure, choose this option (Flat Memory) over any other.

config DISCONTIGMEM_MANUAL
	bool "Discontiguous Memory"
	depends on ARCH_DISCONTIGMEM_ENABLE
	help
	  This option provides enhanced support for discontiguous
	  memory systems, over FLATMEM.  These systems have holes
	  in their physical address spaces, and this option provides
	  more efficient handling of these holes.

	  Although "Discontiguous Memory" is still used by several
	  architectures, it is considered deprecated in favor of
	  "Sparse Memory".

	  If unsure, choose "Sparse Memory" over this option.

config SPARSEMEM_MANUAL
	bool "Sparse Memory"
	depends on ARCH_SPARSEMEM_ENABLE
	help
	  This will be the only option for some systems, including
	  memory hot-plug systems.  This is normal.

	  This option provides efficient support for systems with
	  holes is their physical address space and allows memory
	  hot-plug and hot-remove.

	  If unsure, choose "Flat Memory" over this option.

endchoice
```

## memsection

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

	struct mem_section_usage *usage;
	/*
	 * WARNING: mem_section must be a power-of-2 in size for the
	 * calculation and use of SECTION_ROOT_MASK to make sense.
	 */
};
```

1. section_mem_map 两种使用方法
2. usage ?


## API : pf 和 pd 的装换

```c
/* memmap is virtually contiguous.  */
#define __pfn_to_page(pfn)	(vmemmap + (pfn))
#define __page_to_pfn(page)	(unsigned long)((page) - vmemmap)

/*
 * Convert a physical address to a Page Frame Number and back
 */
#define	__phys_to_pfn(paddr)	PHYS_PFN(paddr)
#define	__pfn_to_phys(pfn)	PFN_PHYS(pfn)
```

1. vmemmap 就是内核空间中间，放置 pd 的开始的位置。
2. vmemmap 类型是 page strut 类型的

> 下面分析一下对应的实现



arch/x86/include/asm/pgtable_64.h
```c
#define vmemmap ((struct page *)VMEMMAP_START)
```

arch/x86/include/asm/pgtable_64_types.h
```c
#ifdef CONFIG_DYNAMIC_MEMORY_LAYOUT // 这是 x86 自定义的
# define VMALLOC_START		vmalloc_base
# define VMALLOC_SIZE_TB	(pgtable_l5_enabled() ? VMALLOC_SIZE_TB_L5 : VMALLOC_SIZE_TB_L4)
# define VMEMMAP_START		vmemmap_base
#else
# define VMALLOC_START		__VMALLOC_BASE_L4
# define VMALLOC_SIZE_TB	VMALLOC_SIZE_TB_L4
# define VMEMMAP_START		__VMEMMAP_BASE_L4
#endif /* CONFIG_DYNAMIC_MEMORY_LAYOUT */
```

arch/x86/kernel/head64.c
```c

#ifdef CONFIG_DYNAMIC_MEMORY_LAYOUT
unsigned long page_offset_base __ro_after_init = __PAGE_OFFSET_BASE_L4;
EXPORT_SYMBOL(page_offset_base);
unsigned long vmalloc_base __ro_after_init = __VMALLOC_BASE_L4;
EXPORT_SYMBOL(vmalloc_base);
unsigned long vmemmap_base __ro_after_init = __VMEMMAP_BASE_L4;
EXPORT_SYMBOL(vmemmap_base);
#endif
```

arch/x86/include/asm/pgtable_64_types.h
```c
#define __VMEMMAP_BASE_L4	0xffffea0000000000UL
```

> 看来就是一个固定的数值


## init

```c
// todo paging_init 的含义是什么，memblock_alloc 返回的就是虚拟地址，那时候 paging 就应该已经建立好了才对的
void __init paging_init(void) 
{
	sparse_memory_present_with_active_regions(MAX_NUMNODES); // MAX_NUMNODES 的含义到底是什么 ?
	sparse_init();

	/*
	 * clear the default setting with node 0
	 * note: don't use nodes_clear here, that is really clearing when
	 *	 numa support is not compiled in, and later node_set_state
	 *	 will not set it back.
	 */
	node_clear_state(0, N_MEMORY);
	if (N_MEMORY != N_NORMAL_MEMORY)
		node_clear_state(0, N_NORMAL_MEMORY);

	zone_sizes_init(); // todo
}

/**
 * sparse_memory_present_with_active_regions - Call memory_present for each active range
 * @nid: The node to call memory_present for. If MAX_NUMNODES, all nodes will be used.
 *
 * If an architecture guarantees that all ranges registered contain no holes and may
 * be freed, this function may be used instead of calling memory_present() manually.
 */
void __init sparse_memory_present_with_active_regions(int nid)
{
	unsigned long start_pfn, end_pfn;
	int i, this_nid;

	for_each_mem_pfn_range(i, nid, &start_pfn, &end_pfn, &this_nid)
		memory_present(this_nid, start_pfn, end_pfn);
}

#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
int memblock_search_pfn_nid(unsigned long pfn, unsigned long *start_pfn,
			    unsigned long  *end_pfn);
void __next_mem_pfn_range(int *idx, int nid, unsigned long *out_start_pfn,
			  unsigned long *out_end_pfn, int *out_nid);

/**
 * for_each_mem_pfn_range - early memory pfn range iterator
 * @i: an integer used as loop variable
 * @nid: node selector, %MAX_NUMNODES for all nodes
 * @p_start: ptr to ulong for start pfn of the range, can be %NULL
 * @p_end: ptr to ulong for end pfn of the range, can be %NULL
 * @p_nid: ptr to int for nid of the range, can be %NULL
 *
 * Walks over configured memory ranges.
 */
#define for_each_mem_pfn_range(i, nid, p_start, p_end, p_nid)		\
	for (i = -1, __next_mem_pfn_range(&i, nid, p_start, p_end, p_nid); \
	     i >= 0; __next_mem_pfn_range(&i, nid, p_start, p_end, p_nid))
#endif /* CONFIG_HAVE_MEMBLOCK_NODE_MAP */
// 从 struct memblock_type *type = &memblock.memory; 中间收集 pfn
```

> 分析两个 memblock 和 sparse_init 的调用关系 : 

```c
/*
 * The platform setup functions are preset with the default functions
 * for standard PC hardware.
 */
struct x86_init_ops x86_init __initdata = {

	.paging = {
		.pagetable_init		= native_pagetable_init, // amd64 下就是 paging_init 
	},

start_kernel
    setup_arch : // 此处使用 paging_init 函数
        e820__memblock_setup();

	      x86_init.paging.pagetable_init();

    mm_init : // 在 setup_arch 后面的位置
      mem_init
        memblock_free_all
```


```c
/* Record a memory area against a node. */
void __init memory_present(int nid, unsigned long start, unsigned long end)
{
	unsigned long pfn;

#ifdef CONFIG_SPARSEMEM_EXTREME
	if (unlikely(!mem_section)) {
		unsigned long size, align;

		size = sizeof(struct mem_section*) * NR_SECTION_ROOTS; // 还是二维数组，但是需要初始化一下。
		align = 1 << (INTERNODE_CACHE_SHIFT);
		mem_section = memblock_alloc(size, align); // 此时 buddy system 没有建立起来
		if (!mem_section)
			panic("%s: Failed to allocate %lu bytes align=0x%lx\n",
			      __func__, size, align);
	}
#endif

	start &= PAGE_SECTION_MASK;
	mminit_validate_memmodel_limits(&start, &end);
	for (pfn = start; pfn < end; pfn += PAGES_PER_SECTION) {  // 将 memblock 管理的内存划分成为一个个的 mem_section
		unsigned long section = pfn_to_section_nr(pfn);
		struct mem_section *ms;

		sparse_index_init(section, nid);
		set_section_nid(section, nid); // 因为 !NODE_NOT_IN_PAGE_FLAGS  也就是 page descriptor 中间含有 node number，所以，这是一个空操作。

		ms = __nr_to_section(section); // 获取到该 mem_section 的数值
		if (!ms->section_mem_map) {
			ms->section_mem_map = sparse_encode_early_nid(nid) |
							SECTION_IS_ONLINE;
			section_mark_present(ms);
		}
	}
}

// 分析各种常量一下 : 
#define PAGES_PER_SECTION       (1UL << PFN_SECTION_SHIFT)

#define PFN_SECTION_SHIFT	(SECTION_SIZE_BITS - PAGE_SHIFT)

#define PAGE_SHIFT		12

# define SECTION_SIZE_BITS	27 /* matt - 128 is convenient right now */

// 简单的辅助函数 : mem_section 的也是顺序排列的
static inline unsigned long pfn_to_section_nr(unsigned long pfn)
{
	return pfn >> PFN_SECTION_SHIFT;
}
static inline unsigned long section_nr_to_pfn(unsigned long sec)
{
	return sec << PFN_SECTION_SHIFT;
}
```


```c
// 在非常早之前
static int __meminit sparse_index_init(unsigned long section_nr, int nid)
{
	unsigned long root = SECTION_NR_TO_ROOT(section_nr);
	struct mem_section *section;

	/*
	 * An existing section is possible in the sub-section hotplug
	 * case. First hot-add instantiates, follow-on hot-add reuses
	 * the existing section.
	 *
	 * The mem_hotplug_lock resolves the apparent race below.
	 */
	if (mem_section[root]) // todo 垃圾 hotplug 如果之前初始化过，那么表示已经初始化过 !
		return 0;

	section = sparse_index_alloc(nid); // 将 section_nr 所在 mem_section 全部进行初始化
	if (!section)
		return -ENOMEM;

	mem_section[root] = section;

	return 0;
}

static noinline struct mem_section __ref *sparse_index_alloc(int nid)
{
	struct mem_section *section = NULL;
	unsigned long array_size = SECTIONS_PER_ROOT *
				   sizeof(struct mem_section);

	if (slab_is_available()) {
		section = kzalloc_node(array_size, GFP_KERNEL, nid);
	} else {
		section = memblock_alloc_node(array_size, SMP_CACHE_BYTES,
					      nid);
		if (!section)
			panic("%s: Failed to allocate %lu bytes nid=%d\n",
			      __func__, array_size, nid);
	}

	return section;
}
```

> 分析一波 sparse_extreme 的 root : 
```c
// 简单的二级结构
#ifdef CONFIG_SPARSEMEM_EXTREME
#define SECTIONS_PER_ROOT       (PAGE_SIZE / sizeof (struct mem_section)) 
#else
#define SECTIONS_PER_ROOT	1
#endif

#define SECTION_NR_TO_ROOT(sec)	((sec) / SECTIONS_PER_ROOT)
```

> memory_present 最后的操作 : 处理 
```c
/*
 * During early boot, before section_mem_map is used for an actual
 * mem_map, we use section_mem_map to store the section's NUMA
 * node.  This keeps us from having to use another data structure.  The
 * node information is cleared just before we store the real mem_map.
 */
static inline unsigned long sparse_encode_early_nid(int nid)
{
	return (nid << SECTION_NID_SHIFT);
}

/*
 * We use the lower bits of the mem_map pointer to store
 * a little bit of information.  The pointer is calculated
 * as mem_map - section_nr_to_pfn(pnum).  The result is
 * aligned to the minimum alignment of the two values:
 *   1. All mem_map arrays are page-aligned.
 *   2. section_nr_to_pfn() always clears PFN_SECTION_SHIFT
 *      lowest bits.  PFN_SECTION_SHIFT is arch-specific
 *      (equal SECTION_SIZE_BITS - PAGE_SHIFT), and the
 *      worst combination is powerpc with 256k pages,
 *      which results in PFN_SECTION_SHIFT equal 6.
 * To sum it up, at least 6 bits are available.
 */
#define	SECTION_MARKED_PRESENT	(1UL<<0)
#define SECTION_HAS_MEM_MAP	(1UL<<1)
#define SECTION_IS_ONLINE	(1UL<<2)
#define SECTION_IS_EARLY	(1UL<<3)
#define SECTION_MAP_LAST_BIT	(1UL<<4)
#define SECTION_MAP_MASK	(~(SECTION_MAP_LAST_BIT-1))
#define SECTION_NID_SHIFT	3

// mem_section::section_mem_map 就会该区域映射的 pd array 的开始位置。

// 获取该 section 对应的 pfn 开始
static inline unsigned long section_nr_to_pfn(unsigned long sec)
{
	return sec << PFN_SECTION_SHIFT;
}
```

1. 为什么 mem_section 可以实现 pf pd 之间的映射 ?
    1. 当该区域没有内存的时候，那么该 section 是没有任何内容的
    2. mem_section的数量决定了最多可以使用的内存
    3. pf -> pfn -> memsection -> section_mem_map -> 得到pd
        1. pd 需要靠什么才可以知道自己的对于 section_mem_map 的偏移量，pd 中间并没有 ?
    4. 为什么这样节省了虚拟地址空间 ?  (显然的)
    5. 所以如何和 SPARSEMEM_VMEMMAP 共存的 ?

2. 
## encode && decode

1. unsigned long coded_mem_map = (unsigned long)(mem_map - (section_nr_to_pfn(pnum))); // 变得和 VMEMMAP 的靠近了，这是在求解在 mem_map 在 section 的偏移量了。


```c
/*
 * Subtle, we encode the real pfn into the mem_map such that
 * the identity pfn - section_mem_map will return the actual
 * physical page frame number.
 */
static unsigned long sparse_encode_mem_map(struct page *mem_map, unsigned long pnum)
{
	unsigned long coded_mem_map =
		(unsigned long)(mem_map - (section_nr_to_pfn(pnum)));
	BUILD_BUG_ON(SECTION_MAP_LAST_BIT > (1UL<<PFN_SECTION_SHIFT));
	BUG_ON(coded_mem_map & ~SECTION_MAP_MASK);
	return coded_mem_map;
}

/*
 * Decode mem_map from the coded memmap
 */
struct page *sparse_decode_mem_map(unsigned long coded_mem_map, unsigned long pnum)
{
	/* mask off the extra low bits of information */
	coded_mem_map &= SECTION_MAP_MASK;
	return ((struct page *)coded_mem_map) + section_nr_to_pfn(pnum);
}

static void __meminit sparse_init_one_section(struct mem_section *ms,
		unsigned long pnum, struct page *mem_map,
		struct mem_section_usage *usage, unsigned long flags)
{
	ms->section_mem_map &= ~SECTION_MAP_MASK;
	ms->section_mem_map |= sparse_encode_mem_map(mem_map, pnum)
		| SECTION_HAS_MEM_MAP | flags;
	ms->usage = usage;
}
```

## sparse_init 调用路线
1. sparse_init 被 paging_init，之后就是 zone 的初始化
2. 循环调用 sparse_early_nid，其中的小策略就是将 nid 相同的一起初始化。
    1. 是不是暗示说 : 同一个 nid 中间的物理地址都是相同的

```c
/*
 * Allocate the accumulated non-linear sections, allocate a mem_map
 * for each and record the physical to section mapping.
 */
void __init sparse_init(void)
{
	unsigned long pnum_begin = first_present_section_nr();
	int nid_begin = sparse_early_nid(__nr_to_section(pnum_begin));
	unsigned long pnum_end, map_count = 1;

	/* Setup pageblock_order for HUGETLB_PAGE_SIZE_VARIABLE */
	set_pageblock_order(); // 空的函数

	for_each_present_section_nr(pnum_begin + 1, pnum_end) {
		int nid = sparse_early_nid(__nr_to_section(pnum_end));

		if (nid == nid_begin) {
			map_count++;
			continue;
		}
		/* Init node with sections in range [pnum_begin, pnum_end) */
		sparse_init_nid(nid_begin, pnum_begin, pnum_end, map_count);
		nid_begin = nid;
		pnum_begin = pnum_end;
		map_count = 1;
	}
	/* cover the last node */
	sparse_init_nid(nid_begin, pnum_begin, pnum_end, map_count);
	vmemmap_populate_print_last();
}
```






## sparse_init_nid : sparse_init 的关键实现
```c
/*
 * Initialize sparse on a specific node. The node spans [pnum_begin, pnum_end)
 * And number of present sections in this node is map_count.
 */
static void __init sparse_init_nid(int nid, unsigned long pnum_begin,
				   unsigned long pnum_end,
				   unsigned long map_count) // map_count 只有一个可能, 需要被 map 的 section 的数量
{
	struct mem_section_usage *usage;
	unsigned long pnum;
	struct page *map;

	usage = sparse_early_usemaps_alloc_pgdat_section(NODE_DATA(nid),
			mem_section_usage_size() * map_count);

	if (!usage) {
		pr_err("%s: node[%d] usemap allocation failed", __func__, nid);
		goto failed;
	}
	sparse_buffer_init(map_count * section_map_size(), nid); // 创建出来需要的物理地址空间，这些空间会被 sparse-vmemmap.c 中间的 populate 使用
	for_each_present_section_nr(pnum_begin, pnum) {
		unsigned long pfn = section_nr_to_pfn(pnum); //  获取该 section 对应的 pfn

		if (pnum >= pnum_end)
			break;

		map = __populate_section_memmap(pfn, PAGES_PER_SECTION,
				nid, NULL); // map 是 现在是可以直接访问的虚拟地址了，指向struct page

    // 将 section 的各种初始化一下
		sparse_init_one_section(__nr_to_section(pnum), pnum, map, usage,
				SECTION_IS_EARLY);
    // usage 刚开始分配的，这里对于其进行滑动。
		usage = (void *) usage + mem_section_usage_size();
	}
	sparse_buffer_fini(); // 将没有使用完的空间释放
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
```

mem_section_usage_size : 表示一个 section 中间 mem_section::usage 的大小
```c
/*
 * PMD_SHIFT determines the size of the area a middle-level
 * page table can map
 */
#define PMD_SHIFT	21
#define HPAGE_SHIFT		PMD_SHIFT
#define HUGETLB_PAGE_ORDER	(HPAGE_SHIFT - PAGE_SHIFT)
/* Huge pages are a constant size */
#define pageblock_order		HUGETLB_PAGE_ORDER

#define PB_migratetype_bits 3
/* Bit indices that affect a whole block of pages */
enum pageblock_bits {  // XXX 千真万确，pageblock 就是用于实现 page migration 的
	PB_migrate,
	PB_migrate_end = PB_migrate + PB_migratetype_bits - 1,
			/* 3 bits required for migrate types */
	PB_migrate_skip,/* If set the block is skipped by compaction */

	/*
	 * Assume the bits will always align on a word. If this assumption
	 * changes then get/set pageblock needs updating.
	 */
	NR_PAGEBLOCK_BITS
};

// 1UL << (PFN_SECTION_SHIFT - pageblock_order ) 表示一个 memsection 中间包含的 memblock 的数量
// 表示一个 section 需要为 pageblock 提供的 flag bit 数量
#define SECTION_BLOCKFLAGS_BITS \
	((1UL << (PFN_SECTION_SHIFT - pageblock_order)) * NR_PAGEBLOCK_BITS)

static unsigned long usemap_size(void)
{
	return BITS_TO_LONGS(SECTION_BLOCKFLAGS_BITS) * sizeof(unsigned long);
}

size_t mem_section_usage_size(void)
{
	return sizeof(struct mem_section_usage) + usemap_size(); // mem_section_usage 的后面可以对应长度的bits 
}

struct mem_section_usage {
	DECLARE_BITMAP(subsection_map, SUBSECTIONS_PER_SECTION); // 第一个元素描述 subsection, subsection 是和 vmemmap 分配 pd 相关的
	/* See declaration of similar field in struct zone */
	unsigned long pageblock_flags[0];
};
```

section_map_size : 表示一个 section 持有的 pd 占用的空间
```c
static unsigned long __init section_map_size(void) // 为什么需要按照 PMD 对其啊 ?
{
	return ALIGN(sizeof(struct page) * PAGES_PER_SECTION, PMD_SIZE);
}
```

> 就是按照从某一个 section_nr 开始，逐个打扫出来 present 的 section
> section 标注 present 的工作之前完成过了
```c
#define for_each_present_section_nr(start, section_nr)		\
	for (section_nr = next_present_section_nr(start-1);	\
	     ((section_nr != -1) &&				\
	      (section_nr <= __highest_present_section_nr));	\
	     section_nr = next_present_section_nr(section_nr))

static inline unsigned long next_present_section_nr(unsigned long section_nr)
{
	while (++section_nr <= __highest_present_section_nr) {
		if (present_section_nr(section_nr))
			return section_nr;
	}

	return -1;
}

static inline int present_section_nr(unsigned long nr)
{
	return present_section(__nr_to_section(nr));
}

static inline int present_section(struct mem_section *section)
{
	return (section && (section->section_mem_map & SECTION_MARKED_PRESENT));
}
```

## subsection_map_init : todo
1. 为什么需要 subsection ?
2. 其实连为什么需要

## mem_section::section_mem_map 使用位置
1. mmzone.h : 是提交访问的各种接口函数
2. sparse.c : 初始化
3. section_mem_map 由于指向 struct page 的对其，较低位置是各种 flag
4. 当 vmemmap 存在的时候，***这些内容其实没有作用的 !***

```c
static inline struct page *__section_mem_map_addr(struct mem_section *section)
{
	unsigned long map = section->section_mem_map;
	map &= SECTION_MAP_MASK;
	return (struct page *)map;
}

#elif defined(CONFIG_SPARSEMEM)
/*
 * Note: section's mem_map is encoded to reflect its start_pfn.
 * section[i].section_mem_map == mem_map's address - start_pfn;
 */
#define __page_to_pfn(pg)					\
({	const struct page *__pg = (pg);				\
	int __sec = page_to_section(__pg);			\
	(unsigned long)(__pg - __section_mem_map_addr(__nr_to_section(__sec)));	\
})

#define __pfn_to_page(pfn)				\
({	unsigned long __pfn = (pfn);			\
	struct mem_section *__sec = __pfn_to_section(__pfn);	\
	__section_mem_map_addr(__sec) + __pfn;		\
})
#endif /* CONFIG_FLATMEM/DISCONTIGMEM/SPARSEMEM */

// 这种情况就是依赖于 page->flags 实现的，和猜想的相同
#ifdef SECTION_IN_PAGE_FLAGS
static inline void set_page_section(struct page *page, unsigned long section)
{
	page->flags &= ~(SECTIONS_MASK << SECTIONS_PGSHIFT);
	page->flags |= (section & SECTIONS_MASK) << SECTIONS_PGSHIFT;
}

static inline unsigned long page_to_section(const struct page *page)
{
	return (page->flags >> SECTIONS_PGSHIFT) & SECTIONS_MASK;
}
#endif
```
## mem_section::usage

1. `__populate_section_memmap` 中间的注释 : 对于一个 section 进行填充是按照 subsection 为粒度的
2. 
```c
#ifdef CONFIG_SPARSEMEM_VMEMMAP
static inline int pfn_section_valid(struct mem_section *ms, unsigned long pfn) // 提供给 fpn_valid 使用
{
	int idx = subsection_map_index(pfn);

	return test_bit(idx, ms->usage->subsection_map);
}
#else
static inline int pfn_section_valid(struct mem_section *ms, unsigned long pfn)
{
	return 1;
}
#endif

void __init subsection_map_init(unsigned long pfn, unsigned long nr_pages) // todo 除非进一步理解其调用者是什么 ?
{
	int end_sec = pfn_to_section_nr(pfn + nr_pages - 1);
	unsigned long nr, start_sec = pfn_to_section_nr(pfn);

	if (!nr_pages)
		return;

	for (nr = start_sec; nr <= end_sec; nr++) {
		struct mem_section *ms;
		unsigned long pfns;

		pfns = min(nr_pages, PAGES_PER_SECTION
				- (pfn & ~PAGE_SECTION_MASK));
		ms = __nr_to_section(nr);
		subsection_mask_set(ms->usage->subsection_map, pfn, pfns);

		pr_debug("%s: sec: %lu pfns: %lu set(%d, %d)\n", __func__, nr,
				pfns, subsection_map_index(pfn),
				subsection_map_index(pfn + pfns - 1));

		pfn += pfns;
		nr_pages -= pfns;
	}
}
```

2. pageblock_flags : 将 pageblock_flags 放在此处是非常合理的，不然放到哪里 ? 只是对于有效内存进行管理。
```c
/* Return a pointer to the bitmap storing bits affecting a block of pages */
static inline unsigned long *get_pageblock_bitmap(struct page *page,
							unsigned long pfn)
{
#ifdef CONFIG_SPARSEMEM
	return section_to_usemap(__pfn_to_section(pfn));
#else
	return page_zone(page)->pageblock_flags;
#endif /* CONFIG_SPARSEMEM */
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

/**
 * set_pfnblock_flags_mask - Set the requested group of flags for a pageblock_nr_pages block of pages
 * @page: The page within the block of interest
 * @flags: The flags to set
 * @pfn: The target page frame number
 * @end_bitidx: The last bit of interest
 * @mask: mask of bits that the caller is interested in
 */
void set_pfnblock_flags_mask(struct page *page, unsigned long flags,
```

