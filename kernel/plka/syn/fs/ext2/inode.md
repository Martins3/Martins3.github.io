> chain

 
这个文件到底处理什么事情

读写函数到底如何搞得

branch 感觉是indirect 导致的东西


在1000 line 作用的位置，定义一些ext2对于vfs的接口，特别的，为 address_space_operations.
inode.c 实际内容处理 data block 和 物理页面的IO, 这一个工作的具体完成其实被交给VFS的通用辅助函数了。

```c
// wirte_inode 和 address_space_operations.write_page　分别写入的内容为 
static int __ext2_write_inode(struct inode *inode, int do_sync)

struct inode *ext2_iget (struct super_block *sb, unsigned long ino)

// 获取disk inode, 被 ext2_iget 使用
static struct ext2_inode *ext2_get_inode(struct super_block *sb, ino_t ino,
					struct buffer_head **p)
```

CONFIG_FS_DAX


重点分析 ext2_iget 的内容；
```c
// ext2_iget 的含义 : 清晰明了，持有sb 就是持有该分区，ino 就是inode 编号
struct inode *ext2_iget (struct super_block *sb, unsigned long ino);

// 某一个调用者，内容简单，通过扫描dir 这一个inode获取 ino ，然后 ext2_iget 获取到inode
static struct dentry *ext2_lookup(struct inode * dir, struct dentry *dentry, unsigned int flags)

// ext2_lookup 的唯一使用位置
const struct inode_operations ext2_dir_inode_operations = {
	.create		= ext2_create,
	.lookup		= ext2_lookup,
  // ...

// ext2_dir_inode_operations 使用位置:
  // 1. ext2_iget 是获取初始化刚刚从disk 中间加载的inode 节点
  // 2. ext2_mkdir



// 返回ino 
// 心态崩了，才意识到，原来在文件系统中间的inode 号码就是写入到磁盘中间的
// 总是，读disk 获取inode number 然后通过inode number 读取disk 中间的inode
// TODO 但是，各种设备的inode num怎么办 ?
ino_t ext2_inode_by_name(struct inode *dir, const struct qstr *child)
{
	ino_t res = 0;
	struct ext2_dir_entry_2 *de;
	struct page *page;
	
	de = ext2_find_entry (dir, child, &page);
	if (de) {
		res = le32_to_cpu(de->inode); // 存储big endian 和 little endian 的问题
		ext2_put_page(page);
	}
	return res;
}

// 握草，这一个操作就是读取disk 到一个page，将 disk_dir_entry 读出来啊!
/*
 *	ext2_find_entry()
 *
 * finds an entry in the specified directory with the wanted name. It
 * returns the page in which the entry was found (as a parameter - res_page),
 * and the entry itself. Page is returned mapped and unlocked.
 * Entry is guaranteed to be valid.
 */
struct ext2_dir_entry_2 *ext2_find_entry (struct inode *dir,
			const struct qstr *child, struct page **res_page)

// 其中用来访问disk的部分
// dir.c
static struct page * ext2_get_page(struct inode *dir, unsigned long n,
				   int quiet)
{
	struct address_space *mapping = dir->i_mapping;
	struct page *page = read_mapping_page(mapping, n, NULL);
	if (!IS_ERR(page)) {
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

// 解释参数n的作用
static inline unsigned long dir_pages(struct inode *inode)
{
  // 除非我们知道 i_size 是做什么的
  // 似乎是有点像 所在的下标 ?
	return (unsigned long)(inode->i_size + PAGE_SIZE - 1) >>
			       PAGE_SHIFT;
}


// 反向映射，一生之敌
static inline struct page *read_mapping_page(struct address_space *mapping,
				pgoff_t index, void *data)
{
  // 此处的类型感觉有点奇怪啊!
	filler_t *filler = (filler_t *)mapping->a_ops->readpage;
	return read_cache_page(mapping, index, filler, data);
}

// 进入到page cache 的领域中间
/**
 * read_cache_page - read into page cache, fill it if needed
 * @mapping:	the page's address_space
 * @index:	the page index
 * @filler:	function to perform the read
 * @data:	first arg to filler(data, page) function, often left as NULL
 *
 * Read into the page cache. If a page already exists, and PageUptodate() is
 * not set, try to fill the page and wait for it to become unlocked.
 *
 * If the page does not get brought uptodate, return -EIO.
 */
struct page *read_cache_page(struct address_space *mapping,
				pgoff_t index,
				int (*filler)(void *, struct page *),
				void *data)
{
	return do_read_cache_page(mapping, index, filler, data, mapping_gfp_mask(mapping));
}
```
