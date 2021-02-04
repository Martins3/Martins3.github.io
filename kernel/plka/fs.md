# address_space

1. dirty page 除了 address_space 的跟踪，还有什么位置？
2. 每一个process 对应一个address_space 还是，一类对应一个?
3. 控制page 是不是dirty 和 page belong to which process 的实现是不是含有重叠的位置？
4. address_space 同时在支持swap file , page cache 和　swap 如此相似，难道代码实现没有公用的部分吗 ?
5. radix_tree 进行管理，整个内核中间其实只有一个
6. 应该并不是每一个进程管理一个 address_space ,但是实际上，每一个inode 对应一个 address_space
7. 通过mmap 的实现，所有的vm_struct 都是和　inod 对应的吗，当为匿名映射的时候，虽然没有inode, 但是含有swap 的管理工作

todo:
1. radix_tree 的工作原理: radix_tree 的键和值是什么 ?


> *mapping* specifies the address space in which a page frame is located. *index is the offset within
> the mapping.* Address spaces are a very general concept used, for example, when reading a file
> into memory. An address space is used to associate the file contents (data) with the areas in
> memory into which the contents are read.
>
> By means of a small trick, mapping is able to hold not
> only a pointer, but also information on whether a page belongs to an anonymous memory area
> that is not associated with an address space. **If the bit with numeric value 1 is set in mapping, the
> pointer does not point to an instance of address_space** but to another data structure (anon_vma)
> that is important in the implementation of reverse mapping for anonymous pages; 
> this structure is discussed in Section 4.11.2. Double use of the pointer is possible because address_space
> instances are always aligned with sizeof(long); the least significant bit of a pointer to this
> instance is therefore 0 on all machines supported by Linux.
> The pointer can be used directly if it points normally to an instance of address_space. If the trick
> involving setting the least significant bit to 1 is used, the kernel can restore the pointer by means
> of the following operation:
>     anon_vma = (struct anon_vma *) (mapping - PAGE_MAPPING_ANON)

这样说: page cache 使用address_space 而匿名映射实际上被替换为 anao_vma

mm/rmap.c 中间实现了反向映射，而且它的头注释介绍了锁的层级结构

> page_get_anon_vma 获取上含有锁的tricky 的机制，感觉其实没有什么特殊，那岂不是访问page 的所有变量都是需要锁机制的吗 ?
```c
struct anon_vma *page_get_anon_vma(struct page *page){
...
  anon_vma = (struct anon_vma *) (mapping - PAGE_MAPPING_ANON)
...
}
```
> rmap.c 有待进一步学习:



address_space 仅仅在page　cache 含有作用，实际上反向映射没有什么关系(不对吧，fs.txt 中间已经说明了含有连个，而且rmap 中间也是大量的使用了该文件)

> Of course, I have shown the situation in simplified form because file data are not generally stored contiguously on hard disk but are distributed over several smaller areas (this is discussed in Chapter 9). The
> kernel makes use of the address_space data structure4 to provide a set of methods to read data from
> a backing store — from a filesystem, for example. address_spaces therefore form an auxiliary layer to
> represent the mapped data as a contiguous linear area to memory management.
是不是说: 如何将一个disk 文件mapped 内存中间，由于disk 本身不是连续的，而且
而且此时address_space 的功能变化成为了，读写磁盘中间商（是virtual process　和 ***?***)

其中定义了 readpage 和 writepage 的功能，aops 的赋值在
```c
void ext4_set_aops(struct inode *inode)
```
但是其中的函数进一步被谁调用目前不清楚啊!

@todo 据说ext2_readpage 的内容非常简单。也说明了address_space的内容穿透到了vfs 层次。

> **Filesystem and block layers are linked by the address_space_operations discussed in Chapter 4.** In the
> Ext2 filesystem, these operations are filled with the following entries
```c
static int ext2_readpage(struct file *file, struct page *page) {
  return mpage_readpage(page, ext2_get_block);
}

static int ext2_writepage(struct page *page, struct writeback_control *wbc) {
  return block_write_full_page(page, ext2_get_block, wbc);
}
```
> 这就是一直想知道的fs 通向底层的道路啊，再一次ext_readpage 很麻烦，被单独放到ext4/readpage 中间了。


```c
	/*
	 * For areas with an address space and backing store,
	 * linkage into the address_space->i_mmap interval tree.
	 */
	struct {
		struct rb_node rb;
		unsigned long rb_subtree_last;
	} shared;

	/*
	 * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
	 * list, after a COW of one of the file pages.	A MAP_SHARED vma
	 * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
	 * or brk vma (with NULL file) can only be in an anon_vma list.
   * @todo 这J8 英语说的是什么东西 ? 到底含有多少种类FLAGS, 分别表示什么含义 ?
	 */
	struct list_head anon_vma_chain; /* Serialized by mmap_sem &
					  * page_table_lock */
	struct anon_vma *anon_vma;	/* Serialized by page_table_lock */
```
> mm_area_struct 中间的内容，注释说: 对于含有backing store 的 area, 那么放到i_mmap

i_mmap 在 address_space 中间定义
```c
	struct rb_root_cached	i_mmap;		/* tree of private and shared mappings */
```
@todo i_mmap 的具体使用规则也是不清楚的

doc:
1. 观察page 的定义，page 中间的定义支持各种类型，
其中，包括:  *page cache and anonymous pages*
> 1. 从buddy 系统中间分配的page 含有确定的类型吗, 除了上面的两个类型还有什么类型
> 2. ucore 如何管理page cache 
> 3. 其他类型的page 都没有含有address_space, 是不是意味着这些page 永远不会被刷新出去，只是被内核使用的

在16章才是对于address_space的终极描述:
1. host  page_tree 的作用: 
The link with the areas managed by an address space is established by means of a pointer to
an inode instance (of type struct inode) to specify the backing store and a root radix tree
(page_tree) with a list of all physical memory pages in the address space.(@question 这是说明一个backstore (disk ssd partition)对应一个文档，还是说仅仅对应一个file)
@question 所以什么时候创建的address_space

#### mmap and sys_read

一个进程为读写文件:
1. 使用mmap 的机制和read 的机制是不是有区别
2. 其中mmap 是不是特指将二进制镜像读入,一般的文件采用其他read vfs ext4 之类的
3. 或者说mmap 和 read 的关系到底是什么

* **filep**
1. filep 出现的位置在什么层次 ?
2. filep 包含的内容是什么 ?
3. filep 的出现就是为了支持process可以访问同一个文件，谁持有文件，也就是持有inode, 在inode 中间包含进程需要的信息，比如引用计数不就可以了吗, 
为什么需要单独独立出来信息 ?
4. 其中的andress_space 的作用是什么 ？
5. file_operations 什么时候赋值 ?
6. 问题是inode 中间也包含file_operations 结构体，所以和struct file　的 file_operations 有什么区别 ?


* **`anon_vma`**

```c
/*
 * The anon_vma heads a list of private "related" vmas, to scan if
 * an anonymous page pointing to this anon_vma needs to be unmapped:
 * the vmas on the list will be related by forking, or by splitting.
 *
 * Since vmas come and go as they are split and merged (particularly
 * in mprotect), the mapping field of an anonymous page cannot point
 * directly to a vma: instead it points to an anon_vma, on whose list
 * the related vmas can be easily linked or unlinked.
 *
 * After unlinking the last vma on the list, we must garbage collect
 * the anon_vma object itself: we're guaranteed no page can be
 * pointing to this anon_vma once its vma list is empty.
 */
struct anon_vma {
```
> 为了管理 anonymous page 单独建立的数据结构，具体原因没有太看懂.


#### swap fs 到底是什么fs
https://askubuntu.com/questions/846163/does-swap-space-have-a-filesystem?newreg=e3aeff4154e3447891d7a686d502ea20


`swap.h`
```c
/*
 * Magic header for a swap area. The first part of the union is
 * what the swap magic looks like for the old (limited to 128MB)
 * swap area format, the second part of the union adds - in the
 * old reserved area - some extra information. Note that the first
 * kilobyte is reserved for boot loader or disk label stuff...
 *
 * Having the magic at the end of the PAGE_SIZE makes detecting swap
 * areas somewhat tricky on machines that support multiple page sizes.
 * For 2.5 we'll probably want to move the magic to just beyond the
 * bootbits...
 */
union swap_header {
	struct {
		char reserved[PAGE_SIZE - 10];
		char magic[10];			/* SWAP-SPACE or SWAPSPACE2 */
	} magic;
	struct {
		char		bootbits[1024];	/* Space for disklabel etc. */
		__u32		version;
		__u32		last_page;
		__u32		nr_badpages;
		unsigned char	sws_uuid[16];
		unsigned char	sws_volume[16];
		__u32		padding[117];
		__u32		badpages[1];
	} info;
};
```

分析 swapon 的片段 :

```c
	page = read_mapping_page(mapping, 0, swap_file); // 龟龟，address_space的内容无处不在，再一次，通过 address_space 读取磁盘中间的内容，最后读取工作进入到filemap 中间
	if (IS_ERR(page)) {
		error = PTR_ERR(page);
		goto bad_swap;
	}
	swap_header = kmap(page);  // 不是重点

	maxpages = read_swap_header(p, swap_header, inode);  // 分析读取到的 swap_header，返回持有的最大的page
```

@todo 所以关于描述每一个页的信息保存在什么地方 ? bitmap ?

#### 定义的几个类型
swp_entry_t
sector_t

装换类型是什么:

```c
/*
 * Extract the `type` field from a swp_entry_t.  The swp_entry_t is in
 * arch-independent format
 */
static inline unsigned swp_type(swp_entry_t entry)
{
	return (entry.val >> SWP_TYPE_SHIFT(entry));
}

/*
 * Extract the `offset` field from a swp_entry_t.  The swp_entry_t is in
 * arch-independent format
 */
static inline pgoff_t swp_offset(swp_entry_t entry)
{
	return entry.val & SWP_OFFSET_MASK(entry);
}
```
> wow
> 居然可以从 swp_entry_t 中间访问获取:
> 1. swap_info_struct 的偏移
> 2. 在 swap 空间中间的偏移量


@todo pte 和 swp_entry_t 之间装换关系是什么 ?


#### page 的private 都可以搞什么事情

```c
/*
 * Returns the page offset into bdev for the specified page's swap entry.
 */
sector_t map_swap_page(struct page *page, struct block_device **bdev)
{
	swp_entry_t entry;
	entry.val = page_private(page);
	return map_swap_entry(entry, bdev);
}
```
1. 可以在物理页面中间存储该页面在swap中间的偏移量，显然是不可能放到 pte 中间的，中间最多放一个flag位表示被换到swap中间了，理解错误的地方


#### swap 中如何添加 page 到各种list 中间

```c
/**
 * page_lru - which LRU list should a page be on?
 * @page: the page to test
 *
 * Returns the LRU list a page should be on, as an index
 * into the array of LRU lists.
 */
static __always_inline enum lru_list page_lru(struct page *page)
{
	enum lru_list lru;

	if (PageUnevictable(page))
		lru = LRU_UNEVICTABLE;
	else {
		lru = page_lru_base_type(page);
		if (PageActive(page))
			lru += LRU_ACTIVE; // 正好的设计
	}
	return lru;
}


/*
 * We do arithmetic on the LRU lists in various places in the code,
 * so it is important to keep the active lists LRU_ACTIVE higher in
 * the array than the corresponding inactive lists, and to keep
 * the *_FILE lists LRU_FILE higher than the corresponding _ANON lists.
 *
 * This has to be kept in sync with the statistics in zone_stat_item
 * above and the descriptions in vmstat_text in mm/vmstat.c
 */
#define LRU_BASE 0
#define LRU_ACTIVE 1
#define LRU_FILE 2

enum lru_list {
	LRU_INACTIVE_ANON = LRU_BASE,
	LRU_ACTIVE_ANON = LRU_BASE + LRU_ACTIVE,　 // @todo anon 什么时候添加进来的
	LRU_INACTIVE_FILE = LRU_BASE + LRU_FILE,
	LRU_ACTIVE_FILE = LRU_BASE + LRU_FILE + LRU_ACTIVE,
	LRU_UNEVICTABLE,
	NR_LRU_LISTS
};


struct lruvec {
	struct list_head		lists[NR_LRU_LISTS];
	struct zone_reclaim_stat	reclaim_stat;
	/* Evictions & activations on the inactive file list */
	atomic_long_t			inactive_age;
	/* Refaults at the time of last reclaim cycle */
	unsigned long			refaults;
#ifdef CONFIG_MEMCG
	struct pglist_data *pgdat;
#endif
};


// page active 如何确定 : mpage_readpages 中间加以跟踪

// file or not 如何确定

// unevictable 如何确定
```


#### swap 中间的小问题




```c
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
{
	VM_BUG_ON_PAGE(PageLRU(page), page); // @todo 无法找到证据说明，调用此函数该 page 一定具有 Page

	if (likely((vma->vm_flags & (VM_LOCKED | VM_SPECIAL)) != VM_LOCKED))
		SetPageActive(page);
	else if (!TestSetPageMlocked(page)) { // 只要是VM_LOCKED 的，那么就一定是mlock的，对于新添加的page, 进行stat
		/*
		 * We use the irq-unsafe __mod_zone_page_stat because this
		 * counter is not modified from interrupt context, and the pte
		 * lock is held(spinlock), which implies preemption disabled.
		 */
		__mod_zone_page_state(page_zone(page), NR_MLOCK,  // 总是同时出现
				    hpage_nr_pages(page)); 
		count_vm_event(UNEVICTABLE_PGMLOCKED);
	}
	lru_cache_add(page);
}

#define VM_BUG_ON_PAGE(cond, page)					\
	do {								\
		if (unlikely(cond)) {					\
			dump_page(page, "VM_BUG_ON_PAGE(" __stringify(cond)")");\
			BUG();						\
		}							\
	} while (0)

  
// Mlocked 
PAGEFLAG(Mlocked, mlocked, PF_NO_TAIL)
	__CLEARPAGEFLAG(Mlocked, mlocked, PF_NO_TAIL)
	TESTSCFLAG(Mlocked, mlocked, PF_NO_TAIL)

/*
 * Various page->flags bits:
 *
 * PG_reserved is set for special pages, which can never be swapped out. Some
 * of them might not even exist...
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
 * PG_referenced, PG_reclaim are used for page reclaim for anonymous and
 * file-backed pagecache (see mm/vmscan.c). // @todo 为什么多出来一个PG_reclaim
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
```

vm_event_item.h

```c
enum vm_event_item  // 神奇的操作


static inline void count_vm_event(enum vm_event_item item)
{
	this_cpu_inc(vm_event_states.event[item]);
}


DEFINE_PER_CPU(struct vm_event_state, vm_event_states) = {{0}};
EXPORT_PER_CPU_SYMBOL(vm_event_states);

/*
 * Light weight per cpu counter implementation.
 *
 * Counters should only be incremented and no critical kernel component
 * should rely on the counter values.
 *
 * Counters are handled completely inline. On many platforms the code
 * generated will simply be the increment of a global address.
 */

struct vm_event_state {
	unsigned long event[NR_VM_EVENT_ITEMS];
};

// @todo 谁使用，这些消息，暂时不在乎


/*
 * For use when we know that interrupts are disabled,
 * or when we know that preemption is disabled and that
 * particular counter cannot be updated from interrupt context.
 */
void __mod_zone_page_state(struct zone *zone, enum zone_stat_item item,
			   long delta)
{
	struct per_cpu_pageset __percpu *pcp = zone->pageset;
	s8 __percpu *p = pcp->vm_stat_diff + item;
	long x;
	long t;

	x = delta + __this_cpu_read(*p);

	t = __this_cpu_read(pcp->stat_threshold);

	if (unlikely(x > t || x < -t)) {
		zone_page_state_add(x, zone, item);
		x = 0;
	}
	__this_cpu_write(*p, x);
}
EXPORT_SYMBOL(__mod_zone_page_state);


enum zone_stat_item {　 // 这些统计的内容完全为了swap 机制设置的
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
	NR_VM_ZONE_STAT_ITEMS
};


// physical memory
// 居然一个page 可以检查出来其对应的zone 所在的位置:
static inline struct zone *page_zone(const struct page *page)
{
	return &NODE_DATA(page_to_nid(page))->node_zones[page_zonenum(page)];
}


#define NODE_DATA(nid)		(node_data[nid])
struct pglist_data *node_data[MAX_NUMNODES] __read_mostly; 


static inline int page_to_nid(const struct page *page)
{
	struct page *p = (struct page *)page;

	return (PF_POISONED_CHECK(p)->flags >> NODES_PGSHIFT) & NODES_MASK;
}

// @todo poison flag 是搞什么的 ?
#define NODES_PGSHIFT		(NODES_PGOFF * (NODES_WIDTH != 0))



/*
 * page->flags layout:
 *
 * There are five possibilities for how page->flags get laid out.  The first
 * pair is for the normal case without sparsemem. The second pair is for
 * sparsemem when there is plenty of space for node and section information.
 * The last is when there is insufficient space in page->flags and a separate
 * lookup is necessary.
 *
 * No sparsemem or sparsemem vmemmap: |       NODE     | ZONE |             ... | FLAGS |
 *      " plus space for last_cpupid: |       NODE     | ZONE | LAST_CPUPID ... | FLAGS |
 * classic sparse with space for node:| SECTION | NODE | ZONE |             ... | FLAGS |
 *      " plus space for last_cpupid: | SECTION | NODE | ZONE | LAST_CPUPID ... | FLAGS |
 * classic sparse no space for node:  | SECTION |     ZONE    | ... | FLAGS |
 */

// @todo 找到flags 的初始化的方法，以及当page 被释放的时候，这些flags 的擦除时间(也许不需要擦除
```

```c
/*
 * For use when we know that interrupts are disabled,
 * or when we know that preemption is disabled and that
 * particular counter cannot be updated from interrupt context.
 */
void __mod_zone_page_state(struct zone *zone, enum zone_stat_item item,
			   long delta)
{
	struct per_cpu_pageset __percpu *pcp = zone->pageset;
	s8 __percpu *p = pcp->vm_stat_diff + item;
	long x;
	long t;

	x = delta + __this_cpu_read(*p);

	t = __this_cpu_read(pcp->stat_threshold);

	if (unlikely(x > t || x < -t)) {
		zone_page_state_add(x, zone, item);
		x = 0;
	}
	__this_cpu_write(*p, x);
}
EXPORT_SYMBOL(__mod_zone_page_state);
// pageset 中间管理的是短期数值，而 zone->vm_stat 中间管理的当超过 threshold 的数值
// 全局变量和 per zone 的共同管理:

static inline void zone_page_state_add(long x, struct zone *zone,
				 enum zone_stat_item item)
{
	atomic_long_add(x, &zone->vm_stat[item]);
	atomic_long_add(x, &vm_zone_stat[item]);
}
```

pageset的作用在于管理冷热page的，通过统计数据，从而进行设置冷热cache
https://www.halolinux.us/kernel-architecture/hotncold-pages.html

# swap cache
`swap_state.c` 中间:

// 初始化
```c
struct address_space *swapper_spaces[MAX_SWAPFILES] __read_mostly; // 每一个file 对应一个 address_space 
static unsigned int nr_swapper_spaces[MAX_SWAPFILES] __read_mostly; // address_space 的存储含有上限，每128M一个 address_space 对象

int init_swap_address_space(unsigned int type, unsigned long nr_pages) // 参数 nr_pages


SYSCALL_DEFINE2(swapon, const char __user *, specialfile, int, swap_flags)
	maxpages = read_swap_header(p, swap_header, inode); // 通过read header 中间的 last_page 确定(其中arch 中字段长度是理论限制)

```

// 查询

// 插入 删除

```c
/**
 * add_to_swap - allocate swap space for a page
 * @page: page we want to move to swap
 *
 * Allocate swap space for the page and add the page to the
 * swap cache.  Caller needs to hold the page lock. 
 */
int add_to_swap(struct page *page)
{
	swp_entry_t entry;
	int err;

	VM_BUG_ON_PAGE(!PageLocked(page), page);
	VM_BUG_ON_PAGE(!PageUptodate(page), page);

	entry = get_swap_page(page);
	if (!entry.val)
		return 0;

	/*
	 * Radix-tree node allocations from PF_MEMALLOC contexts could
	 * completely exhaust the page allocator. __GFP_NOMEMALLOC
	 * stops emergency reserves from being allocated.
	 *
	 * TODO: this could cause a theoretical memory reclaim
	 * deadlock in the swap out path.
	 */
	/*
	 * Add it to the swap cache.
	 */
	err = add_to_swap_cache(page, entry,
			__GFP_HIGH|__GFP_NOMEMALLOC|__GFP_NOWARN);
	/* -ENOMEM radix-tree allocation failure */
	if (err)
		/*
		 * add_to_swap_cache() doesn't return -EEXIST, so we can safely
		 * clear SWAP_HAS_CACHE flag.
		 */
		goto fail;
	/*
	 * Normally the page will be dirtied in unmap because its pte should be
	 * dirty. A special case is MADV_FREE page. The page'e pte could have
	 * dirty bit cleared but the page's SwapBacked bit is still set because
	 * clearing the dirty bit and SwapBacked bit has no lock protected. For
	 * such page, unmap will not set dirty bit for it, so page reclaim will
	 * not write the page out. This can cause data corruption when the page
	 * is swap in later. Always setting the dirty bit for the page solves
	 * the problem.
	 */
	set_page_dirty(page);

	return 1;

fail:
	put_swap_page(page, entry);
	return 0;
}

/*
 * This must be called only on pages that have
 * been verified to be in the swap cache and locked.
 * It will never put the page into the free list,
 * the caller has a reference on the page.
 */
void delete_from_swap_cache(struct page *page)
{
	swp_entry_t entry;
	struct address_space *address_space;

	entry.val = page_private(page);

	address_space = swap_address_space(entry); // 获取 address_space 和 entry
	xa_lock_irq(&address_space->i_pages);
	__delete_from_swap_cache(page);
	xa_unlock_irq(&address_space->i_pages);

	put_swap_page(page, entry);
	page_ref_sub(page, hpage_nr_pages(page));
}

/*
 * This must be called only on pages that have
 * been verified to be in the swap cache.
 */
void __delete_from_swap_cache(struct page *page)
{
	struct address_space *address_space;
	int i, nr = hpage_nr_pages(page);
	swp_entry_t entry;
	pgoff_t idx;

	VM_BUG_ON_PAGE(!PageLocked(page), page);
	VM_BUG_ON_PAGE(!PageSwapCache(page), page);
	VM_BUG_ON_PAGE(PageWriteback(page), page);

	entry.val = page_private(page);
	address_space = swap_address_space(entry);
	idx = swp_offset(entry);
	for (i = 0; i < nr; i++) {
		radix_tree_delete(&address_space->i_pages, idx + i); // 将radix 的索引的删除
		set_page_private(page + i, 0);
	}
	ClearPageSwapCache(page);
	address_space->nrpages -= nr;
	__mod_node_page_state(page_pgdat(page), NR_FILE_PAGES, -nr);
	ADD_CACHE_INFO(del_total, nr);
}


// 获取offset 数值，然后采用 
#define swap_address_space(entry)			    \
	(&swapper_spaces[swp_type(entry)][swp_offset(entry) \
		>> SWAP_ADDRESS_SPACE_SHIFT])
/*
 * Extract the `type` field from a swp_entry_t.  The swp_entry_t is in
 * arch-independent format
 */
static inline unsigned swp_type(swp_entry_t entry)
{
	return (entry.val >> SWP_TYPE_SHIFT(entry)); // swap 的本身包含选中哪一个 swap file/dev 的字段
}
```

# swap 的 readahead cluster 



# swap 和 反向映射
如果没有swap 机制，rmap 马上就是一个几乎没有任何作用的东西了，
1. 暂时无法想象没有swap　机制

为什么我们需要swap cache 的内容, 从 swap_entry_t　获取 page ？

When swapping pages out to the swap files, 
Linux avoids writing pages if it does not have to. (什么意思)
There are times when a page is both in a swap file and in physical memory.(什么情况)
This happens when a page that was swapped out of memory was then brought back into memory when it was again accessed by a process. 
So long as the page in memory is not written to, the copy in the swap file remains valid.


Linux uses the swap cache to track these pages. The swap cache is a list of page table entries,
one per physical page in the system. 
This is a page table entry for a swapped out page 
and describes which swap file the page is being held in together with its location in the swap file.
If a swap cache entry is non-zero, it represents a page.


# swp_entry_t 为什么会出现在 page->private 中间的
```c
			/**
			 * @private: Mapping-private opaque data.
			 * Usually used for buffer_heads if PagePrivate.
			 * Used for swp_entry_t if PageSwapCache.
			 * Indicates order in the buddy system if PageBuddy.
			 */
			unsigned long private;
```
1. PageSwapCache 是什么东西 ? 只有当在page swap cache 中间的才有意义
2. add_to_swap 被调用的条件 是什么 ? 分析shrink_page_list
3. swap cache 不是加快操作，而是必须存在的


# fread 和 mmap 有什么区别
反正最后数据都是交给 page cache 中间了

1. fread 会一开始就将 整个文件缓存到内存中间吗 ? 应该不会，fopen 只是打开文件，而fread 的参数会指定大小，用户程序负责这些细节。
2. fread 打开的文件显然不在地址空间中间
3. 当fread 打开的文件会被共享吗 ? 应该不会 ! 
4. fread file 还会继续使用 radix tree 实现反向映射吗 ?

