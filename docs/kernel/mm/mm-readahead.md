# Readahead

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

```txt
1. blk_start_plug
2. list_del(&page->lru); // todo 到底其中 page->lru 的作用是什么 : 从slub的位置就看到了
```

## 才知道 readahead 会如此之激进啊

mmap 4G 的文件，但是所在的 cgroup 只有 2G，然后对于其进行顺序读，pgmafault 几乎没有什么增加的。

## 原来 readahead 机制一般都是自动计算的

fio mmap + randread
```txt
                        - 81.43% do_mem_abort
                           - 81.35% do_translation_fault
                              - 81.31% do_page_fault
                                 - 76.22% handle_mm_fault
                                    - 74.55% __handle_mm_fault
                                       - 72.89% do_fault
                                          - 69.29% __do_fault
                                             - 69.10% filemap_fault
                                                - 40.17% filemap_read_folio
                                                   - 33.92% ext4_read_folio
                                                      - 33.66% ext4_mpage_readpages
                                                         - 24.83% submit_bio
                                                            - submit_bio_noacct
```


有 readahead 的时候:
```txt
@[
    page_cache_ra_unbounded+0
    filemap_fault+1664
    __do_fault+68
    do_fault+936
    __handle_mm_fault+648
    handle_mm_fault+228
    do_page_fault+352
    do_translation_fault+180
    do_mem_abort+72
    el0_ia+132
    el0t_64_sync_handler+220
    el0t_64_sync+408
]: 1
```

没有 readahead 的时候
```txt
@[
    ext4_read_folio+0
    filemap_fault+1764
    __do_fault+68
    do_fault+936
    __handle_mm_fault+648
    handle_mm_fault+228
    do_page_fault+352
    do_translation_fault+180
    do_mem_abort+72
    el0_da+92
    el0t_64_sync_handler+196
    el0t_64_sync+408
]: 15783
```

可以通过这个方法来
```diff
diff --git a/mm/filemap.c b/mm/filemap.c
index a6459874bb2a..1e21b7d3f896 100644
--- a/mm/filemap.c
+++ b/mm/filemap.c
@@ -3258,7 +3258,7 @@ static struct file *do_sync_mmap_readahead(struct vm_fault *vmf)
         * Do we miss much more than hit in this file? If so,
         * stop bothering with read-ahead. It will only hurt.
         */
-       if (mmap_miss > MMAP_LOTSAMISS)
+       if (mmap_miss > MMAP_LOTSAMISS || true)
                return fpin;

        /*
```

## 原来文件系统还会注册不同的 api
```c
static int ext4_read_folio(struct file *file, struct folio *folio)
{
	int ret = -EAGAIN;
	struct inode *inode = folio->mapping->host;

	trace_ext4_read_folio(inode, folio);

	if (ext4_has_inline_data(inode))
		ret = ext4_readpage_inline(inode, folio);

	if (ret == -EAGAIN)
		return ext4_mpage_readpages(inode, NULL, folio);

	return ret;
}

static void ext4_readahead(struct readahead_control *rac)
{
	struct inode *inode = rac->mapping->host;

	/* If the file has inline data, no need to do readahead. */
	if (ext4_has_inline_data(inode))
		return;

	ext4_mpage_readpages(inode, rac, NULL);
}
```

## readahead 只有 mmap 的时候才会用么?

如果使用 read 或者 write 读一个文件，也会有 readahead 机制么?

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
