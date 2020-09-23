阅读plka的时候，对于突然出现的SPARSE 模型很疑惑，最近看到一个文章https://lwn.net/Articles/789304/ ，其总结的很好，以此为基础讲解一下linux内存模型的架构是什么。

问题:

1. 在amd64 defconfig 中间CONFIG_NUMA = 1 吗 ? 是的，虽然该配置打开，但是并不代表该机器上持有多个物理memory node，因为NUMA退化版本就是UMA.
3. 每一个node 主要管理什么信息，每一个zone 中间放置什么内容?
4. 哪里涉及到了nodemask 的，主要作用是什么 ?

下面是对于原文的总结性的翻译：

> FLAT：ucore中间使用的模型。Back in the beginning of Linux, memory was flat: it was a simple linear sequence with physical addresses starting at zero and ending at several megabytes.实现从一个page frame 的PFN转化为其对应的page struct 很容易且高效。

> DISCONTIG：为了应对物理内存中间出现空洞，以及NUMA而产生的。其提出一个重要的概念，memory node。每一个memory node 中间含有独立buddy system，各种统计信息等。在每一个memory node 中间，其中是管理连续的物理地址空间，并且含有一个page struct 数组和一个物理页面一一对应。虽然解决问题，但是也带来了PFN 难以查询page struct 的问题。

> SPARSE：在64位系统上，At the cost of additional page table entries, page_to_pfn(), and pfn_to_page() became as simple as with the flat model. (其他我就真的没有看懂

## SPARSE 
下面说明一下sparse memory model的架构代码体现。
#### A
arch/x86/include/asm/mmzone_32.h 和 arch/x86/include/asm/mmzone_64.h 中间定义pg_data_t 的数组。

#### B
include/linux/mmzone.h 中间定义了pg_data_t，其描述的内容一个node 
0. zone类型的数组。
1. 每一个节点都有自己的ID：node_id；
3. node_start_pfn是这个节点的起始页号；
4.  swap 和统计信息。

#### C
include/linux/mmzone.h 其中同样定义了zone，查看一下中间的内容，其中包括:
1. unsigned long watermark[NR_WMARK]; watermark 用于设置各种阈值，比如(
2. struct free_area	free_area[MAX_ORDER]; 定义伙伴系统
3. atomic_long_t	vm_stat[NR_VM_ZONE_STAT_ITEMS]; 统计信息
4. per_cpu_pageset用于区分冷热页

总结一下，首先讲解了kernel的memory model 的演变，然后分析一下sparse的树状结构。下面是具体问题的分析。


## 1. pfn_to_page 如何实现的 ?


## 2. zone 和 node 如何初始化 ?
初始化前提首先是探测物理内存，其提供的信息为:
> TODO

```
void __init paging_init(void)
{
	sparse_memory_present_with_active_regions(MAX_NUMNODES);
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

	zone_sizes_init();
}
void __init zone_sizes_init(void)
{
	unsigned long max_zone_pfns[MAX_NR_ZONES]; // 大小为4, noramle moveable dma 和 dma32

	memset(max_zone_pfns, 0, sizeof(max_zone_pfns));

#ifdef CONFIG_ZONE_DMA
	max_zone_pfns[ZONE_DMA]		= min(MAX_DMA_PFN, max_low_pfn);
#endif
#ifdef CONFIG_ZONE_DMA32
	max_zone_pfns[ZONE_DMA32]	= min(MAX_DMA32_PFN, max_low_pfn);
#endif
	max_zone_pfns[ZONE_NORMAL]	= max_low_pfn;
// 在amd64 配置中，下面的不用考虑
#ifdef CONFIG_HIGHMEM
	max_zone_pfns[ZONE_HIGHMEM]	= max_pfn;
#endif

	free_area_init_nodes(max_zone_pfns);
}
```
然后调用到node 和 zone 的初始化位置:

free_area_init_nodes 的工作:
1. 填充arch 初始化提供的信息:
```
static unsigned long arch_zone_lowest_possible_pfn[MAX_NR_ZONES] __meminitdata;
static unsigned long arch_zone_highest_possible_pfn[MAX_NR_ZONES] __meminitdata;
```
其中movable zone 中间的内容需要单独计算。
2. 输出各种调试信息
3. 调用free_area_init_node 逐个初始化node

free_area_init_node 的工作:
1. 各种pg_data_t 成员初始化
2. 调用free_area_init_core 
    1. 初始化pg_data_t `pgdat_init_internals(pgdat)`
    2. 对于每一个zone 为buddy system 启用做准备，顺便初始化各个zone `		zone_init_internals(zone, j, nid, freesize)`

## 3. zone 和 node 中间都含有统计信息，分别统计什么?
1. 这些统计信息是通过什么接口提供给用户程序的，或者内核如何使用它们?
2. zone 和 node 统计内容有什么侧重?

node 统计信息定义
```
	/* Per-node vmstats */
	struct per_cpu_nodestat __percpu *per_cpu_nodestats;
	atomic_long_t		vm_stat[NR_VM_NODE_STAT_ITEMS];
} pg_data_t;

struct zone {
...
	/* Zone statistics */
	atomic_long_t		vm_stat[NR_VM_ZONE_STAT_ITEMS];
	atomic_long_t		vm_numa_stat[NR_VM_NUMA_STAT_ITEMS];
} ____cacheline_internodealigned_in_smp;

struct per_cpu_nodestat {
	s8 stat_threshold;
	s8 vm_node_stat_diff[NR_VM_NODE_STAT_ITEMS];
};

enum zone_stat_item {
	/* First 128 byte cacheline (assuming 64 bit words) */
	NR_FREE_PAGES,
	NR_ZONE_LRU_BASE, /* Used only for compaction and reclaim retry */
	NR_ZONE_INACTIVE_ANON = NR_ZONE_LRU_BASE,
	NR_ZONE_ACTIVE_ANON,
	NR_ZONE_INACTIVE_FILE,
	NR_ZONE_ACTIVE_FILE,
	NR_ZONE_UNEVICTABLE,
	NR_ZONE_WRITE_PENDING,	/* Count of dirty, writeback and unstable pages */
	NR_MLOCK,		/* mlock()ed pages found and moved off LRU */
	NR_PAGETABLE,		/* used for pagetables */
	NR_KERNEL_STACK_KB,	/* measured in KiB */
	/* Second 128 byte cacheline */
	NR_BOUNCE,
#if IS_ENABLED(CONFIG_ZSMALLOC)
	NR_ZSPAGES,		/* allocated in zsmalloc */
#endif
	NR_FREE_CMA_PAGES,
	NR_VM_ZONE_STAT_ITEMS };

enum node_stat_item {
	NR_LRU_BASE,
	NR_INACTIVE_ANON = NR_LRU_BASE, /* must match order of LRU_[IN]ACTIVE */
	NR_ACTIVE_ANON,		/*  "     "     "   "       "         */
	NR_INACTIVE_FILE,	/*  "     "     "   "       "         */
	NR_ACTIVE_FILE,		/*  "     "     "   "       "         */
	NR_UNEVICTABLE,		/*  "     "     "   "       "         */
	NR_SLAB_RECLAIMABLE,
	NR_SLAB_UNRECLAIMABLE,
	NR_ISOLATED_ANON,	/* Temporary isolated pages from anon lru */
	NR_ISOLATED_FILE,	/* Temporary isolated pages from file lru */
	WORKINGSET_REFAULT,
	WORKINGSET_ACTIVATE,
	WORKINGSET_NODERECLAIM,
	NR_ANON_MAPPED,	/* Mapped anonymous pages */
	NR_FILE_MAPPED,	/* pagecache pages mapped into pagetables.
			   only modified from process context */
	NR_FILE_PAGES,
	NR_FILE_DIRTY,
	NR_WRITEBACK,
	NR_WRITEBACK_TEMP,	/* Writeback using temporary buffers */
	NR_SHMEM,		/* shmem pages (included tmpfs/GEM pages) */
	NR_SHMEM_THPS,
	NR_SHMEM_PMDMAPPED,
	NR_ANON_THPS,
	NR_UNSTABLE_NFS,	/* NFS unstable pages */
	NR_VMSCAN_WRITE,
	NR_VMSCAN_IMMEDIATE,	/* Prioritise for reclaim when writeback ends */
	NR_DIRTIED,		/* page dirtyings since bootup */
	NR_WRITTEN,		/* page writings since bootup */
	NR_INDIRECTLY_RECLAIMABLE_BYTES, /* measured in bytes */
	NR_VM_NODE_STAT_ITEMS
};
```


## 4. pageset swap migration watermark 和 buddy system 分别依赖于zone 还是 node，为什么?
首先大致解释一下，这些东西都是什么东西和作用!
| module       | desc                                              | based on |
|--------------|---------------------------------------------------|----------|
| lruvec       | 配合swap 使用                                     | zone     |
| swap         |                                                   | node     |
| pageset      | migration 相关，用于实现page frame的fragmentation | zone     |
| watermark    | 配合pageset 使用                                  | zone     |
| buddy system |                                                   | zone     |

emmmm 貌似 migration 就是管理pageset 的?

1. 所以，migration 如何和buddy system 打交道? alloc_page.c 应该就是整个buddy system 的位置 ? 然后pageset
2. 这大概就是为什么讲解buddy system 首先会分析 pageset 的原因了吧!
pcp 接口是什么 ?
```c
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




```c
enum migratetype {
	MIGRATE_UNMOVABLE,
	MIGRATE_MOVABLE,
	MIGRATE_RECLAIMABLE,
	MIGRATE_PCPTYPES,	/* the number of types on the pcp lists */
	MIGRATE_HIGHATOMIC = MIGRATE_PCPTYPES,
#ifdef CONFIG_CMA
	/*
	 * MIGRATE_CMA migration type is designed to mimic the way
	 * ZONE_MOVABLE works.  Only movable pages can be allocated
	 * from MIGRATE_CMA pageblocks and page allocator never
	 * implicitly change migration type of MIGRATE_CMA pageblock.
	 *
	 * The way to use it is to change migratetype of a range of
	 * pageblocks to MIGRATE_CMA which can be done by
	 * __free_pageblock_cma() function.  What is important though
	 * is that a range of pageblocks must be aligned to
	 * MAX_ORDER_NR_PAGES should biggest page be bigger then
	 * a single pageblock.
	 */
	MIGRATE_CMA,
#endif
#ifdef CONFIG_MEMORY_ISOLATION
	MIGRATE_ISOLATE,	/* can't allocate from here */
#endif
	MIGRATE_TYPES
};

struct per_cpu_pages {
	int count;		/* number of pages in the list */
	int high;		/* high watermark, emptying needed */
	int batch;		/* chunk size for buddy add/remove */

	/* Lists of pages, one per migrate type stored on the pcp-lists */
	struct list_head lists[MIGRATE_PCPTYPES];
};
// pageset 居然就是和watermrk 合作使用的


struct per_cpu_pageset {
	struct per_cpu_pages pcp;
#ifdef CONFIG_NUMA
	s8 expire;
	u16 vm_numa_stat_diff[NR_VM_NUMA_STAT_ITEMS];
#endif
#ifdef CONFIG_SMP
	s8 stat_threshold;
	s8 vm_stat_diff[NR_VM_ZONE_STAT_ITEMS];
#endif
};
```

swap 在node 中间的内容, vmscan 中间的内容。
```c
typedef struct pglist_data {
	wait_queue_head_t kswapd_wait;
	wait_queue_head_t pfmemalloc_wait;
	struct task_struct *kswapd;	/* Protected by
					   mem_hotplug_begin/end() */
	int kswapd_order;
	enum zone_type kswapd_classzone_idx;

	int kswapd_failures;		/* Number of 'reclaimed == 0' runs */
  ...
}
```

```c
enum zone_watermarks {
	WMARK_MIN,
	WMARK_LOW,
	WMARK_HIGH,
	NR_WMARK
};

#define min_wmark_pages(z) (z->watermark[WMARK_MIN])
#define low_wmark_pages(z) (z->watermark[WMARK_LOW])
#define high_wmark_pages(z) (z->watermark[WMARK_HIGH])


struct zone {
	/* Read-mostly fields */

	/* zone watermarks, access with *_wmark_pages(zone) macros */
	unsigned long watermark[NR_WMARK];
```
> 主要的作用出现在 :vmscan 和 page_alloc 中间

https://www.halolinux.us/kernel-architecture/calculation-of-zone-watermarks.html
> 似乎就是抄书啊!
> watermark 似乎用于swap 的，但是又是在node 中间 ? 不科学啊!
> 忽然意识到： 居然和冷热cache 有关的 冷热cache 内容很有问题!


```c
struct free_area {
	struct list_head	free_list[MIGRATE_TYPES];
	unsigned long		nr_free;
};

struct zone {
	/* free areas of different sizes */
	struct free_area	free_area[MAX_ORDER];
```
> 从此处就已经含有MIGRATE_TYPES 的处理了


## 5. 各种类型的zone都是用来做什么的，这些zone的划分到底是物理设备决定的，还是软件配置的？

## 6. DMA 到底的原理是什么，为什么需要单独建立一个node 来处理它?

## 7. movable zone 的作用是什么 ?
如果说去掉movable zone 和 dma，似乎sparse为什么

## 8. 为什么物理内存不是连续的?
https://stackoverflow.com/questions/23626165/what-is-meant-by-holes-in-the-memory-linux

## 9.  分析函数 page_zone实现!  
```c
static inline struct zone *page_zone(const struct page *page)
{
	return &NODE_DATA(page_to_nid(page))->node_zones[page_zonenum(page)];
}
```
