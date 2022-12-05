# page-writeback.c 分析

## ref && doc
- [](http://www.wowotech.net/memory_management/327.html)

- [](https://lwn.net/Articles/648292/)
> @todo 理解 writeback 和 cgroup 的关系


## 此处不处理具体的写回操作，谢谢


## domain node global
对于 CPU 含有 domain，对于 memory 也是含有 domain 的概念。

## labtop mode

## policy

```c
/**
 * balance_dirty_pages_ratelimited - balance dirty memory state
 * @mapping: address_space which was dirtied
 *
 * Processes which are dirtying memory should call in here once for each page
 * which was newly dirtied.  The function will periodically check the system's
 * dirty state and will initiate writeback if needed.
 *
 * On really big machines, get_writeback_state is expensive, so try to avoid
 * calling it too often (ratelimiting).  But once we're over the dirty memory
 * limit we decrease the ratelimiting by a lot, to prevent individual processes
 * from overshooting the limit by (ratelimit_pages) each.
 */
void balance_dirty_pages_ratelimited(struct address_space *mapping)
// 1. balance_dirty_pages : 300 行的大函数
// 2.
```

## ratelimit_pages

两个 ref 位置 :
```c
/*
 * If ratelimit_pages is too high then we can get into dirty-data overload
 * if a large number of processes all perform writes at the same time.
 * If it is too low then SMP machines will call the (expensive)
 * get_writeback_state too often.
 *
 * Here we set ratelimit_pages to a level which ensures that when all CPUs are
 * dirtying in parallel, we cannot go more than 3% (1/32) over the dirty memory
 * thresholds.
 */

void writeback_set_ratelimit(void)
{
	struct wb_domain *dom = &global_wb_domain; // todo
	unsigned long background_thresh;
	unsigned long dirty_thresh;

	global_dirty_limits(&background_thresh, &dirty_thresh);
	dom->dirty_limit = dirty_thresh;
	ratelimit_pages = dirty_thresh / (num_online_cpus() * 32);
	if (ratelimit_pages < 16)
		ratelimit_pages = 16;
}

```



```c
/**
 * tag_pages_for_writeback - tag pages to be written by write_cache_pages
 * @mapping: address space structure to write
 * @start: starting page index
 * @end: ending page index (inclusive)
 *
 * This function scans the page range from @start to @end (inclusive) and tags
 * all pages that have DIRTY tag set with a special TOWRITE tag. The idea is
 * that write_cache_pages (or whoever calls this function) will then use
 * TOWRITE tag to identify pages eligible for writeback.  This mechanism is
 * used to avoid livelocking of writeback by a process steadily creating new
 * dirty pages in the file (thus it is important for this function to be quick
 * so that it can tag pages faster than a dirtying process can create them).
 */
/*
 * We tag pages in batches of WRITEBACK_TAG_BATCH to reduce the i_pages lock
 * latency.
 */
void tag_pages_for_writeback(struct address_space *mapping,
			     pgoff_t start, pgoff_t end)

// write_cache_pages ?
```






```c
/*
 * Dirty a page.
 *
 * For pages with a mapping this should be done under the page lock
 * for the benefit of asynchronous memory errors who prefer a consistent
 * dirty state. This rule can be broken in some special cases,
 * but should be better not to.
 *
 * If the mapping doesn't provide a set_page_dirty a_op, then
 * just fall through and assume that it wants buffer_heads.
 */
int set_page_dirty(struct page *page)
// 似乎这是全部的内容了
```






## 和外部交互的分析

```c
int do_writepages(struct address_space *mapping, struct writeback_control *wbc)
{
	int ret;

	if (wbc->nr_to_write <= 0)
		return 0;
	while (1) {
		if (mapping->a_ops->writepages)
			ret = mapping->a_ops->writepages(mapping, wbc);
		else
			ret = generic_writepages(mapping, wbc);
		if ((ret != -ENOMEM) || (wbc->sync_mode != WB_SYNC_ALL))
			break;
		cond_resched();
		congestion_wait(BLK_RW_ASYNC, HZ/50);
	}
	return ret;
}

int generic_writepages(struct address_space *mapping,
		       struct writeback_control *wbc)

int write_cache_pages(struct address_space *mapping,
		      struct writeback_control *wbc, writepage_t writepage,
		      void *data)
```



> 分析各种注册到 address_space 上的 writepage !

```c
// ext2  : 依赖于公共框架
static int ext2_readpage(struct file *file, struct page *page)
{
  // 原来居然只是 read 走 mpage，但是write 在buffer.c 中间
	return mpage_readpage(page, ext2_get_block);
}

static int ext2_writepage(struct page *page, struct writeback_control *wbc)
{
  // 其实这个就让人觉得特别奇怪
  // TODO 如果 page cache 连接到 ext2_writepage 中间，但是现在又依赖于 buffer 机制
	return block_write_full_page(page, ext2_get_block, wbc);
}


// ext4 : 直接进入 block 层，自己实现。
static int ext4_readpage(struct file *file, struct page *page)
{
	int ret = -EAGAIN;
	struct inode *inode = page->mapping->host;

	trace_ext4_readpage(page);

	if (ext4_has_inline_data(inode))
		ret = ext4_readpage_inline(inode, page);

	if (ret == -EAGAIN)
		return ext4_mpage_readpages(page->mapping, NULL, page, 1,
						false);

	return ret;
}

int ext4_mpage_readpages(struct address_space *mapping,
			 struct list_head *pages, struct page *page,
			 unsigned nr_pages, bool is_readahead)
// 一个非常长的函数，直接进入到 block layer 层次


static int ext4_writepage(struct page *page,
			  struct writeback_control *wbc)
// 也是 ext4 自己的负责的内容


// 进一步分析 ext4_writepages 和 ext4_readpages，前者其实依赖 generic_writepages 然后反复调用自己的 writepage 函数
// 但是后者没有 generic 这一部分，因为写会依赖于 page cache 中间的 radix tree 的功能，但是 read 不需要。

// swap : page_io.c 中间实现。


// 总体来说，ext2 很奇怪啊 !
```

> 分析一下各种

```c
// 所以，其实奇怪的地方不在于 address_space 而是在于为什么ext4 需要同时注册
// address_space(没有问题) 和 file_operations(有点问题, 似乎过于high level了,
// 越过了page cache)

// 如果 ext4 中间不持有 file_operations 我猜测 vfs 的设计如何进行 ?
// 显然不是为了 dax 机制，否则必定含有更简单的方法 ?

// ext2 : 依赖于filemap.c
static ssize_t ext2_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
#ifdef CONFIG_FS_DAX
	if (IS_DAX(iocb->ki_filp->f_mapping->host))
		return ext2_dax_read_iter(iocb, to);
#endif
	return generic_file_read_iter(iocb, to);
}

static ssize_t ext2_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
#ifdef CONFIG_FS_DAX
	if (IS_DAX(iocb->ki_filp->f_mapping->host))
		return ext2_dax_write_iter(iocb, from);
#endif
	return generic_file_write_iter(iocb, from);
}

// ext4 : 还是依赖于filemap.c
static ssize_t ext4_file_read_iter(struct kiocb *iocb, struct iov_iter *to)
{
	if (unlikely(ext4_forced_shutdown(EXT4_SB(file_inode(iocb->ki_filp)->i_sb))))
		return -EIO;

	if (!iov_iter_count(to))
		return 0; /* skip atime */

#ifdef CONFIG_FS_DAX
	if (IS_DAX(file_inode(iocb->ki_filp)))
		return ext4_dax_read_iter(iocb, to);
#endif
	return generic_file_read_iter(iocb, to);
}

static ssize_t
ext4_file_write_iter(struct kiocb *iocb, struct iov_iter *from)
// 提供了更多的检查之类的操作，然后调用 __generic_file_write_iter

// 就读写而言，一般就是将工作给page cache 或者 绕过 page cache，已经足够说明需要使用为file_operation 提供fs specific 的file io 了。
// 而且file 的操作不限于io 还有其他的和文件系统相关，所以fs 提供 file_operations 的接口是有必要的。
```

## page_writeback_init

```c
/*
 * Called early on to tune the page writeback dirty limits.
 *
 * We used to scale dirty pages according to how total memory
 * related to pages that could be allocated for buffers (by
 * comparing nr_free_buffer_pages() to vm_total_pages.
 *
 * However, that was when we used "dirty_ratio" to scale with
 * all memory, and we don't do that any more. "dirty_ratio"
 * is now applied to total non-HIGHPAGE memory (by subtracting
 * totalhigh_pages from vm_total_pages), and as such we can't
 * get into the old insane situation any more where we had
 * large amounts of dirty pages compared to a small amount of
 * non-HIGHMEM memory.
 *
 * But we might still want to scale the dirty_ratio by how
 * much memory the box has..
 */
void __init page_writeback_init(void)
{
	BUG_ON(wb_domain_init(&global_wb_domain, GFP_KERNEL));

	cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "mm/writeback:online",
			  page_writeback_cpu_online, NULL);
	cpuhp_setup_state(CPUHP_MM_WRITEBACK_DEAD, "mm/writeback:dead", NULL,
			  page_writeback_cpu_online);
}
```


## set_page_dirty : 不是设置一个标志位的事情，而是
1. 一般来说调用 `__set_page_dirty_buffers` 完成工作, 同时标记两个内容

> `set_page_dirty` allows an address space to provide a specific method of marking a page as
> dirty. However, this option is rarely used. In this case, the kernel automatically uses ccode `__set_page_dirty_buffers`
> to simultaneously mark the page as dirty on the buffer level and to add it to
> the `dirty_pages` list of the current mapping.

```c
/*
 * Dirty a page.
 *
 * For pages with a mapping this should be done under the page lock
 * for the benefit of asynchronous memory errors who prefer a consistent
 * dirty state. This rule can be broken in some special cases,
 * but should be better not to.
 *
 * If the mapping doesn't provide a set_page_dirty a_op, then
 * just fall through and assume that it wants buffer_heads.
 */
int set_page_dirty(struct page *page)
{
	struct address_space *mapping = page_mapping(page);

	page = compound_head(page);
	if (likely(mapping)) {
		int (*spd)(struct page *) = mapping->a_ops->set_page_dirty;
		/*
		 * readahead/lru_deactivate_page could remain
		 * PG_readahead/PG_reclaim due to race with end_page_writeback
		 * About readahead, if the page is written, the flags would be
		 * reset. So no problem.
		 * About lru_deactivate_page, if the page is redirty, the flag
		 * will be reset. So no problem. but if the page is used by readahead
		 * it will confuse readahead and make it restart the size rampup
		 * process. But it's a trivial problem.
		 */
		if (PageReclaim(page))
			ClearPageReclaim(page);
#ifdef CONFIG_BLOCK
		if (!spd)
			spd = __set_page_dirty_buffers;
#endif
		return (*spd)(page);
	}
	if (!PageDirty(page)) {
		if (!TestSetPageDirty(page))
			return 1;
	}
	return 0;
}
EXPORT_SYMBOL(set_page_dirty);
```

## test_clear_page_writeback && `__test_set_page_writeback`
1. 实际上的并没有办法区分这三个 flags 的含义是什么 ?
2. 实际上，这一个 TAG 是可以选择的，当没有取消掉这一个选项的时候，内容立刻变得非常简单了。


```diff
mm: don't use radix tree writeback tags for pages in swap cache

File pages use a set of radix tree tags (DIRTY, TOWRITE, WRITEBACK,
etc.) to accelerate finding the pages with a specific tag in the radix
tree during inode writeback.  But for anonymous pages in the swap cache,
there is no inode writeback.  So there is no need to find the pages with
some writeback tags in the radix tree.  It is not necessary to touch
radix tree writeback tags for pages in the swap cache.

Per Rik van Riel's suggestion, a new flag AS_NO_WRITEBACK_TAGS is
introduced for address spaces which don't need to update the writeback
tags.  The flag is set for swap caches.  It may be used for DAX file
systems, etc.

With this patch, the swap out bandwidth improved 22.3% (from ~1.2GB/s to
~1.48GBps) in the vm-scalability swap-w-seq test case with 8 processes.
The test is done on a Xeon E5 v3 system.  The swap device used is a RAM
simulated PMEM (persistent memory) device.  The improvement comes from
the reduced contention on the swap cache radix tree lock.  To test
sequential swapping out, the test case uses 8 processes, which
sequentially allocate and write to the anonymous pages until RAM and
part of the swap device is used up.

Link: http://lkml.kernel.org/r/1472578089-5560-1-git-send-email-ying.huang@intel.com
```
3. @todo 所以什么叫做 inode writeback ? radix tree 利用 tag 为什么可以加速查找的速度 ?

```c
static inline int mapping_use_writeback_tags(struct address_space *mapping)
{
	return !test_bit(AS_NO_WRITEBACK_TAGS, &mapping->flags);
}

/* XArray tags, for tagging dirty and writeback pages in the pagecache. */
#define PAGECACHE_TAG_DIRTY	XA_MARK_0
#define PAGECACHE_TAG_WRITEBACK	XA_MARK_1
#define PAGECACHE_TAG_TOWRITE	XA_MARK_2

/*
 * Bits in mapping->flags.
 */
enum mapping_flags {
	AS_EIO		= 0,	/* IO error on async write */
	AS_ENOSPC	= 1,	/* ENOSPC on async write */
	AS_MM_ALL_LOCKS	= 2,	/* under mm_take_all_locks() */
	AS_UNEVICTABLE	= 3,	/* e.g., ramdisk, SHM_LOCK */
	AS_EXITING	= 4, 	/* final truncate in progress */
	/* writeback related tags are not used */
	AS_NO_WRITEBACK_TAGS = 5,
};
```


```c
int test_clear_page_writeback(struct page *page)
{
	struct address_space *mapping = page_mapping(page);
	struct mem_cgroup *memcg;
	struct lruvec *lruvec;
	int ret;

	memcg = lock_page_memcg(page);
	lruvec = mem_cgroup_page_lruvec(page, page_pgdat(page));
	if (mapping && mapping_use_writeback_tags(mapping)) {
		struct inode *inode = mapping->host;
		struct backing_dev_info *bdi = inode_to_bdi(inode);
		unsigned long flags;

		xa_lock_irqsave(&mapping->i_pages, flags);
		ret = TestClearPageWriteback(page);
		if (ret) {
			__xa_clear_mark(&mapping->i_pages, page_index(page),
						PAGECACHE_TAG_WRITEBACK);
			if (bdi_cap_account_writeback(bdi)) {
				struct bdi_writeback *wb = inode_to_wb(inode);

				dec_wb_stat(wb, WB_WRITEBACK);
				__wb_writeout_inc(wb);
			}
		}

		if (mapping->host && !mapping_tagged(mapping,
						     PAGECACHE_TAG_WRITEBACK))
			sb_clear_inode_writeback(mapping->host);

		xa_unlock_irqrestore(&mapping->i_pages, flags);
	} else {
		ret = TestClearPageWriteback(page); // 这也太简化了吧!
	}
	/*
	 * NOTE: Page might be free now! Writeback doesn't hold a page
	 * reference on its own, it relies on truncation to wait for
	 * the clearing of PG_writeback. The below can only access
	 * page state that is static across allocation cycles.
	 */
	if (ret) { // todo 处理一下统计数据，虽然不知道为什么 >
		dec_lruvec_state(lruvec, NR_WRITEBACK);
		dec_zone_page_state(page, NR_ZONE_WRITE_PENDING);
		inc_node_page_state(page, NR_WRITTEN);
	}
	__unlock_page_memcg(memcg);
	return ret;
}

int __test_set_page_writeback(struct page *page, bool keep_write)
{
	struct address_space *mapping = page_mapping(page);
	int ret;

	lock_page_memcg(page);
	if (mapping && mapping_use_writeback_tags(mapping)) {
		XA_STATE(xas, &mapping->i_pages, page_index(page));
		struct inode *inode = mapping->host;
		struct backing_dev_info *bdi = inode_to_bdi(inode);
		unsigned long flags;

		xas_lock_irqsave(&xas, flags);
		xas_load(&xas);
		ret = TestSetPageWriteback(page);
		if (!ret) {
			bool on_wblist;

			on_wblist = mapping_tagged(mapping,
						   PAGECACHE_TAG_WRITEBACK);

			xas_set_mark(&xas, PAGECACHE_TAG_WRITEBACK);
			if (bdi_cap_account_writeback(bdi))
				inc_wb_stat(inode_to_wb(inode), WB_WRITEBACK);

			/*
			 * We can come through here when swapping anonymous
			 * pages, so we don't necessarily have an inode to track
			 * for sync.
			 */
			if (mapping->host && !on_wblist)
				sb_mark_inode_writeback(mapping->host);
		}
		if (!PageDirty(page))
			xas_clear_mark(&xas, PAGECACHE_TAG_DIRTY);
		if (!keep_write)
			xas_clear_mark(&xas, PAGECACHE_TAG_TOWRITE);
		xas_unlock_irqrestore(&xas, flags);
	} else {
		ret = TestSetPageWriteback(page); // 也是非常简化的
	}
	if (!ret) { // 也是统计数据
		inc_lruvec_page_state(page, NR_WRITEBACK);
		inc_zone_page_state(page, NR_ZONE_WRITE_PENDING);
	}
	unlock_page_memcg(page);
	return ret;

}
EXPORT_SYMBOL(__test_set_page_writeback);
```

## do_writepages  : 将整个 inode 中间的 dirty page 全部写会
```c
int do_writepages(struct address_space *mapping, struct writeback_control *wbc)
{
	int ret;

	if (wbc->nr_to_write <= 0)
		return 0;
	while (1) {
		if (mapping->a_ops->writepages)
			ret = mapping->a_ops->writepages(mapping, wbc);
		else
			ret = generic_writepages(mapping, wbc); //
		if ((ret != -ENOMEM) || (wbc->sync_mode != WB_SYNC_ALL))
			break;
		cond_resched();
		congestion_wait(BLK_RW_ASYNC, HZ/50);
	}
	return ret;
}


/**
 * generic_writepages - walk the list of dirty pages of the given address space and writepage() all of them.
 * @mapping: address space structure to write
 * @wbc: subtract the number of written pages from *@wbc->nr_to_write
 *
 * This is a library function, which implements the writepages()
 * address_space_operation.
 *
 * Return: %0 on success, negative error code otherwise
 */
int generic_writepages(struct address_space *mapping,
		       struct writeback_control *wbc)
{
	struct blk_plug plug;
	int ret;

	/* deal with chardevs and other special file */
	if (!mapping->a_ops->writepage)
		return 0;

	blk_start_plug(&plug); // todo blk plug 的作用是什么 ?
	ret = write_cache_pages(mapping, wbc, __writepage, mapping);
	blk_finish_plug(&plug);
	return ret;
}
```
