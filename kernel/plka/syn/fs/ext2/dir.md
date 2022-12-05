# ext2/dir.c 分析

> 从287行开始分析，前边各种辅助小函数，@todo 部分小函数也是莫名奇妙啊!


```c
// 唯一使用位置，居然是:
const struct file_operations ext2_dir_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.iterate_shared	= ext2_readdir,

static int ext2_readdir(struct file *file, struct dir_context *ctx)
// 参数居然是file ?
// TODO 没有分析 2016年新增内容
```
所以什么是 iterate_shared : https://lwn.net/Articles/687184/


```c
struct ext2_dir_entry_2 *ext2_find_entry (struct inode *dir,
			const struct qstr *child, struct page **res_page)
  // 从一开始，问题就非常的奇怪，为什么返回值是res_page ?

	start = ei->i_dir_start_lookup; // 似乎每一次查询都是紧接着上一次查询的，而且查询是一个循环, nb啊!

static inline unsigned long dir_pages(struct inode *inode)
{
	return (unsigned long)(inode->i_size + PAGE_SIZE - 1) >>
			       PAGE_SHIFT;
}


// 实现的内容
static struct page * ext2_get_page(struct inode *dir, unsigned long n,
				   int quiet)
{
	struct address_space *mapping = dir->i_mapping;
	struct page *page = read_mapping_page(mapping, n, NULL);
	if (!IS_ERR(page)) {
    // holy shit ! kmap到底是什么 ?
		kmap(page);
		if (unlikely(!PageChecked(page))) {
			if (PageError(page) || !ext2_check_page(page, quiet))
				goto fail;
		}
	}
	return page;

fail:
	ext2_put_page(page);
	return ERR_PTR(-EIO);
}

// get_page 和 put_page 总是成对出现 !
// 不浪费吗 ?　 TODO 显然哪里理解出现问题了
// 为什么在这个地方就是不使用 buffer.c 中间的内容了 !
// TODO kmap 和 kunmap 的作用是什么，似乎跳转有问题啊!
// https://www.kernel.org/doc/gorman/html/understand/understand012.html
// highmem 中间的内容，从原理上分析，kmap 和 kunmap 都是空函数才对啊!
page = ext2_get_page(dir, n, dir_has_error);
    -- read_mapping_page 进入到filemap之后，无穷的恐惧啊!

ext2_put_page(page);
```

```c
// ext2_rename 的辅助函数
// 使用和ext2_find_entry 相同的技术
struct ext2_dir_entry_2 * ext2_dotdot (struct inode *dir, struct page **p)
```

```c
// ext2_get_page 的封装函数
ino_t ext2_inode_by_name(struct inode *dir, const struct qstr *child)
```

```c
// TODO 又是进入到buffer.c 中间了 !
static int ext2_prepare_chunk(struct page *page, loff_t pos, unsigned len)
{
	return __block_write_begin(page, pos, len, ext2_get_block);
}
```

```c
// 逐渐看不懂了啊!
void ext2_set_link(struct inode *dir, struct ext2_dir_entry_2 *de,
		   struct page *page, struct inode *inode, int update_times)

  // 主要是
	de->inode = cpu_to_le32(inode->i_ino);

  // ext2_prepare_chunk
  // ext2_commit_chunk
```

```c
// 被ext2_link等函数使用
int ext2_add_link (struct dentry *dentry, struct inode *inode)
// 主要的内容就是循环，找到dentry 在parent 中间的对应的 TODO 没有细看，应该不麻烦
```
> 后面的几个函数应该都是简单的内容。
