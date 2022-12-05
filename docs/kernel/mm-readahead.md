# mm/readahead.c

总体:
1. 提供两个终极外部接口 : page_cache_sync_readahead 和 page_cache_async_readahead
2. read_pages 调用 : `mapping->a_ops->readpages` 或者 `mapping->a_ops->readpage` 实现真正读取工作


## file_ra_state_init
// todo 实现简单，但是可以找到 ra 状态一下。

## page_cache_sync_readahead 和 page_cache_async_readahead

```c
/**
 * page_cache_sync_readahead - generic file readahead
 * @mapping: address_space which holds the pagecache and I/O vectors
 * @ra: file_ra_state which holds the readahead state
 * @filp: passed on to ->readpage() and ->readpages()
 * @offset: start offset into @mapping, in pagecache page-sized units
 * @req_size: hint: total size of the read which the caller is performing in
 *            pagecache pages
 *
 * page_cache_sync_readahead() should be called when a cache miss happened:
 * it will submit the read.  The readahead logic may decide to piggyback more
 * pages onto the read request if access patterns suggest it will improve
 * performance.
 */
void page_cache_sync_readahead(struct address_space *mapping,
			       struct file_ra_state *ra, struct file *filp,
			       pgoff_t offset, unsigned long req_size)

/**
 * page_cache_async_readahead - file readahead for marked pages
 * @mapping: address_space which holds the pagecache and I/O vectors
 * @ra: file_ra_state which holds the readahead state
 * @filp: passed on to ->readpage() and ->readpages()
 * @page: the page at @offset which has the PG_readahead flag set
 * @offset: start offset into @mapping, in pagecache page-sized units
 * @req_size: hint: total size of the read which the caller is performing in
 *            pagecache pages
 *
 * page_cache_async_readahead() should be called when a page is used which
 * has the PG_readahead flag; this is a marker to suggest that the application
 * has used up enough of the readahead window that we should start pulling in
 * more pages.
 */
void
page_cache_async_readahead(struct address_space *mapping,
			   struct file_ra_state *ra, struct file *filp,
			   struct page *page, pgoff_t offset,
			   unsigned long req_size)
```
1. 实现的区别
2. 使用上的区别


## ondemand_readahead

page_cache_sync_readahead 和 page_cache_async_readahead 都会调用的


## `__do_page_cache_readahead`

真正的读取工作

```c
/*
 * __do_page_cache_readahead() actually reads a chunk of disk.  It allocates
 * the pages first, then submits them for I/O. This avoids the very bad
 * behaviour which would occur if page allocations are causing VM writeback.
 * We really don't want to intermingle reads and writes like that.
 *
 * Returns the number of pages requested, or the maximum amount of I/O allowed.
 */
unsigned int __do_page_cache_readahead(struct address_space *mapping,
		struct file *filp, pgoff_t offset, unsigned long nr_to_read,
		unsigned long lookahead_size)
```

```c
// 三个 ref
/*
 * Submit IO for the read-ahead request in file_ra_state.
 */
static inline unsigned long ra_submit(struct file_ra_state *ra,
		struct address_space *mapping, struct file *filp)
{
	return __do_page_cache_readahead(mapping, filp,
					ra->start, ra->size, ra->async_size);
}

/*
 * A minimal readahead algorithm for trivial sequential/random reads.
 */
static unsigned long
ondemand_readahead(struct address_space *mapping,
		   struct file_ra_state *ra, struct file *filp,
		   bool hit_readahead_marker, pgoff_t offset,
		   unsigned long req_size)

/*
 * Chunk the readahead into 2 megabyte units, so that we don't pin too much
 * memory at once.
 */
int force_page_cache_readahead(struct address_space *mapping, struct file *filp,
			       pgoff_t offset, unsigned long nr_to_read)
```



## ksys_readahead
Man readahead(2)
Man fadvise(2)


本以为 readahead 需要利用其中的函数，但是:

```c
int vfs_fadvise(struct file *file, loff_t offset, loff_t len, int advice)
{
	if (file->f_op->fadvise) // 至今，无人为此注册，所以等价于访问 generic_fadvise
		return file->f_op->fadvise(file, offset, len, advice);

	return generic_fadvise(file, offset, len, advice); // 由于 advice == POSIX_FADV_WILLNEED, 选择 generic_fadvise
}

int force_page_cache_readahead(struct address_space *mapping, struct file *filp,
			       pgoff_t offset, unsigned long nr_to_read)
```


## read_pages
> 你真的完全理解了吗 ?

参数 file * 从 generic_file_buffered_read 的位置发生了变化了吗 ?

```c
1. blk_start_plug
2. list_del(&page->lru); // todo 到底其中 page->lru 的作用是什么 : 从slub的位置就看到了
```
