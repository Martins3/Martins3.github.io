# Page Flags
1. PG_reserved, PG_locked
2. PG_referenced, PG_reclaim

3. PG_swapcache : 表示该 page 被用于 swap cache , 在那个 radix tree 中间
4. PG_swapbacked : 该 page 被 swap backed ，在 disk 之类的含有备份。
    1. PageAnon : anon page
    2. @todo 所以，存在 PG_swapcache 但是没有 PG_swapbacked 的吗 ?

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
## PG_writeback
参考 docs/kernel/fs-writeback.md

## [ ] PG_reclaim

使用位置：
```txt
mm/swap.c
594:            folio_set_reclaim(folio);

mm/zswap.c
1073:   folio_set_reclaim(folio);

mm/vmscan.c
679:            folio_set_reclaim(folio);
1173:                           folio_set_reclaim(folio);
1342:                           folio_set_reclaim(folio);
```

- pageout 中

在这里调用:
```c
		res = mapping->a_ops->writepage(&folio->page, &wbc);
```
那么说明，而且当 writeback 之后，就清理掉，所以
```c
		if (!folio_test_writeback(folio)) {
			/* synchronous write or broken a_ops? */
			folio_clear_reclaim(folio);
		}
```
当含有 reclaim 的 flag 的时候，这个 page 正处于 cache 的 write back 中。



## PG_reserved

```txt
#0  reserve_bootmem_region (start=0, end=1048576, nid=0) at mm/mm_init.c:742
#1  0xffffffff835d0428 in memmap_init_reserved_pages () at mm/memblock.c:2117
#2  free_low_memory_core_early () at mm/memblock.c:2129
#3  memblock_free_all () at mm/memblock.c:2176
#4  0xffffffff835b35cd in mem_init () at arch/x86/mm/init_64.c:1338
#5  0xffffffff835cca23 in mm_core_init () at mm/mm_init.c:2790
#6  0xffffffff8358cef9 in start_kernel () at init/main.c:928
#7  0xffffffff83598898 in x86_64_start_reservations (
    real_mode_data=real_mode_data@entry=0x13b10 <exception_stacks+31504> <error: Cannot access mem
ory at address 0x13b10>) at arch/x86/kernel/head64.c:556
#8  0xffffffff835989d5 in x86_64_start_kernel (
    real_mode_data=0x13b10 <exception_stacks+31504> <error: Cannot access memory at address 0x13b1
0>) at arch/x86/kernel/head64.c:537
#9  0xffffffff810001ce in secondary_startup_64 () at arch/x86/kernel/head_64.S:441
#10 0x0000000000000000 in ?? ()
```

## PG_private
在 folio_check_dirty_writeback 中有一段小注释说明:
```c
	/* Verify dirty/writeback state if the filesystem supports it */
	if (!folio_test_private(folio))
		return;
```
所以，如果 folio 是关联文件系统的，就会设置上这个
如果是这种考虑，为什么通过 struct address_space *folio::(anon union)::(anon struct)::mapping 来判断

```c
/*
 * The PG_private bitflag is set on pagecache pages if they contain filesystem
 * specific data (which is normally at page->private). It can be used by
 * private allocations for its own usage.
 */
```

### Question 1
- [ ] folio_test_private 的修改需要持有什么锁吗?
```c
		if (unlikely(buffer_heads_over_limit)) {
			if (folio_test_private(folio) && folio_trylock(folio)) {
				if (folio_test_private(folio))
					filemap_release_folio(folio, 0);
				folio_unlock(folio);
			}
		}
```
不然，为什么判断两次啊，但是感觉 folio_test_private 的地方都是很随意的
这里判断两次，主要是因为 folio_trylock 的开销太高了吧

### Question 2 : 什么时候设置上 private 的

1. 通过调用 folio_attach_private 的时候

设置上 buffer head 的时候:
- link_dev_buffers
- folio_create_empty_buffers

设置 iomap 的时候:
- iomap_page_create

migration 的时候，因为 page 切换了，所以需要让 private 指向新的位置:
- filemap_migrate_folio
- __buffer_migrate_folio

总是 nfs 的似乎有点特殊的, 在 nfs_inode_add_request 中，存在如下内容:
```c
	if (likely(!folio_test_swapcache(folio))) {
		set_bit(PG_MAPPED, &req->wb_flags);
		folio_set_private(folio);
		folio->private = req;
	}
```

2. 通过 set_page_private 的调用的时候
- add_to_swap_cache : swap cache 的 page 存放 swp_entry_t
- kvm_mmu_alloc_shadow_page : 存储 kvm_mmu_page

## PG_workingset


folio_set_workingset
folio_test_workingset

1. 看上去和内存回收有关，具体需要看看代码
2. slub 中使用 PG_workingset 来

```c
/*
 * SLUB reuses PG_workingset bit to keep track of whether it's on
 * the per-node partial list.
 */
static inline bool slab_test_node_partial(const struct slab *slab)
{
	return folio_test_workingset(slab_folio(slab));
}

static inline void slab_set_node_partial(struct slab *slab)
{
	set_bit(PG_workingset, folio_flags(slab_folio(slab), 0));
}

static inline void slab_clear_node_partial(struct slab *slab)
{
	clear_bit(PG_workingset, folio_flags(slab_folio(slab), 0));
}
```

## PG_locked

收集各种场景:
1. writeback drity page 的过程中，应该是 lock page, submmit , set PG_writeback，然后 unlock
  - 这样，shrinker 就会看到一个 unlocked page 的 PG_writeback 的 page

TODO : 这个代码中确认一下。

## PG_swapbacked
表示这个 page 如果换出，那么需要写入到 swap 中。

## PG_swapcache

folio_test_swapcache

```c
static __always_inline bool folio_test_swapcache(struct folio *folio)
{
	return folio_test_swapbacked(folio) &&
			test_bit(PG_swapcache, folio_flags(folio, 0));
}
```
mm/memory-failure.c 中也是存在类似的定义:
```c
#define sc		((1UL << PG_swapcache) | (1UL << PG_swapbacked))
```

显然，PG_swapcache 和 PG_swapbacked 有区别的:

PG_swapbacked 表示该内存可以写入到 swap 中，PG_swapcache 表示就是 swapcache 。

## PG_isolated

PG_isolated 其实就是给 movable page 的移动用的

docs/kernel/mm-mapping.md 中的 commit msg 中有解释:
```c
PAGEFLAG(Isolated, isolated, PF_ANY);
```
简单看看 Isolated 的含义吧

和 mm/vmscan.c 中 isolate_folio 似乎重叠了

mf_isolate_folio 正好描述了两个函数:
```c
		if (lru)
			isolated = folio_isolate_lru(folio);
		else
			isolated = isolate_movable_page(&folio->page,
							ISOLATE_UNEVICTABLE);
```

对于 movable page ，也就是类似 balloon 之类的，通过 PG_isolated 可以
防止多个 CPU 在 isolate 同一个。

folio_isolate_lru 的做法是将 page 从 lru 上取下来
其他的 CPU 只能操作 lru 的，就不会同时操作一个 page 了。


### folio_test_swapbacked : 指的是该 page 被写入到 swap 中

只要是，anon 和 shmem 就会有
```c
/*
 * PG_swapbacked is set when a page uses swap as a backing storage.  This are
 * usually PageAnon or shmem pages but please note that even anonymous pages
 * might lose their PG_swapbacked flag when they simply can be dropped (e.g. as
 * a result of MADV_FREE).
 */
```

这么说，任何时候，只要发生 pgfault 的时候，只要是 anon，就应该设置上。
是的，的确如此:
- page_add_new_anon_rmap : anon page fault 代码总是会调用到此处的

这个 flag 存在什么时候起作用?
- 调用的位置太多了


### 对比 folio_test_anon 和 folio_test_swapbacked
- https://www.cnblogs.com/liuhailong0112/p/14426096.html

- PageAnon : 看是否有 page->mapping 为空，只有
- PageSwapBacked: 和 file backed 向对

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
- PageAnon(page) && PageSwapBacked(page) : private anon
- !PageAnon(page) && PageSwapBacked(page) : shared anon
- PageAnon(page) && !PageSwapBacked(page) : madvice 导致的特殊情况


PageAnon 是一种区分，同时，当在 folio_is_file_lru 中，是另一种区分:

```c
/**
 * folio_is_file_lru - Should the folio be on a file LRU or anon LRU?
 * @folio: The folio to test.
 *
 * We would like to get this info without a page flag, but the state
 * needs to survive until the folio is last deleted from the LRU, which
 * could be as far down as __page_cache_release.
 *
 * Return: An integer (not a boolean!) used to sort a folio onto the
 * right LRU list and to account folios correctly.
 * 1 if @folio is a regular filesystem backed page cache folio
 * or a lazily freed anonymous folio (e.g. via MADV_FREE).
 * 0 if @folio is a normal anonymous folio, a tmpfs folio or otherwise
 * ram or swap backed folio.
 */
static inline int folio_is_file_lru(struct folio *folio)
{
	return !folio_test_swapbacked(folio);
}
```

## PG_lru
参考 vmscan.tmp.md 中内容


## 所有的 flags 展开的基本规则
要是实在看不懂，就用这个来解决吧: docs/kernel/code/mm/pageflags-expand.c

例如
```c
PAGEFLAG(Referenced, referenced, PF_HEAD)
	TESTCLEARFLAG(Referenced, referenced, PF_HEAD)
	__SETPAGEFLAG(Referenced, referenced, PF_HEAD)
```
1. PAGEFLAG 提供 test set clear 的功能，例如 test 会分解为:
  - folio_test_lru
  - PageLRU
2. TESTCLEARFLAG 和 __TESTCLEARFLAG 的差别体现于

- `set_bit` 还是 `__set_bit` 的区别在于原子性

```c
/**
 * set_bit - Atomically set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * This is a relaxed atomic operation (no implied memory barriers).
 *
 * Note that @nr may be almost arbitrarily large; this function is not
 * restricted to acting on a single-word quantity.
 */
static __always_inline void set_bit(long nr, volatile unsigned long *addr)
{
	instrument_atomic_write(addr + BIT_WORD(nr), sizeof(long)); // KASAN 相关的内容
	arch_set_bit(nr, addr);
}

#define __set_bit(nr, addr)		bitop(___set_bit, nr, addr)

/**
 * ___set_bit - Set a bit in memory
 * @nr: the bit to set
 * @addr: the address to start counting from
 *
 * Unlike set_bit(), this function is non-atomic. If it is called on the same
 * region of memory concurrently, the effect may be that only one operation
 * succeeds.
 */
static __always_inline void
___set_bit(unsigned long nr, volatile unsigned long *addr)
{
	instrument_write(addr + BIT_WORD(nr), sizeof(long));
	arch___set_bit(nr, addr);
}
```

## PG_uptodate

```c
static __always_inline void __folio_mark_uptodate(struct folio *folio)
{
	smp_wmb();
	__set_bit(PG_uptodate, folio_flags(folio, 0));
}

static __always_inline void folio_mark_uptodate(struct folio *folio)
{
	/*
	 * Memory barrier must be issued before setting the PG_uptodate bit,
	 * so that all previous stores issued in order to bring the folio
	 * uptodate are actually visible before folio_test_uptodate becomes true.
	 */
	smp_wmb();
	set_bit(PG_uptodate, folio_flags(folio, 0));
}

CLEARPAGEFLAG(Uptodate, uptodate, PF_NO_TAIL)
```
很容易可以注意到，PG_uptodate 没有使用那些常规的 macro

```c
static __always_inline bool folio_test_idle(struct folio *folio) {
  return test_bit(PG_idle, folio_flags(folio, FOLIO_PF_ANY));
}

static __always_inline void folio_set_idle(struct folio *folio) {
  set_bit(PG_idle, folio_flags(folio, FOLIO_PF_ANY));
}

static __always_inline void folio_clear_idle(struct folio *folio) {
  clear_bit(PG_idle, folio_flags(folio, FOLIO_PF_ANY));
}
```

## PG_young
在 docs/kernel/mm-idle.md 解释


## PG_dirty
参考分析一下： docs/kernel/mm-page-writeback.md

## 仔细看看
- https://mp.weixin.qq.com/s/BUCldFsgPCSaH46e8n7ooA
  - https://lwn.net/Articles/974515/
  - https://lwn.net/ml/linux-mm/ZkOu4yXP-sGGtwc4@casper.infradead.org/
    - 最后的总结

#### (mm) PageUptodate
想知道 page 上的各种 flag 的作用是什么 ?

## 内核中一定有一个函数，来 dump 一个 page 上有的各种 flags 的

## PG_anon_exclusive
这个做啥的，为啥这么长的注释啊?

- 6c287605fd56466e645693eff3ae7c08fba56e0
- 78fbe906cc900b33ce078102e13e0e99b5b8c406

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
