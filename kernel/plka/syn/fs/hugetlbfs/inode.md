

1. 如何处理 mount 参数的 ? : 通用机制 fs_context_operations hugetlbfs_parse_param
2. 如何通往 mm/hugetlb.c 的


## aops
```c
// TODO 既然没有 read 和 write ，那么 set_page_dirty 的作用是什么 ?
static const struct address_space_operations hugetlbfs_aops = {
	.write_begin	= hugetlbfs_write_begin, // 只是装点一下牌面
	.write_end	= hugetlbfs_write_end,
	.set_page_dirty	= hugetlbfs_set_page_dirty,
	.migratepage    = hugetlbfs_migrate_page,
	.error_remove_page	= hugetlbfs_error_remove_page,
};
```

```c
/*
 * mark the head page dirty
 */
static int hugetlbfs_set_page_dirty(struct page *page)
{
	struct page *head = compound_head(page);

	SetPageDirty(head);
	return 0;
}

static inline struct page *compound_head(struct page *page)
{
	unsigned long head = READ_ONCE(page->compound_head); // 如果是普通 page ，那么直接指向 head 的地址

	if (unlikely(head & 1)) // 并且 set zero bit
		return (struct page *) (head - 1);
	return page; // head page 存储的是什么东西，并不知道，zero bit 没有 set
}
```


## file_operations
```c
const struct file_operations hugetlbfs_file_operations = {
	.read_iter		= hugetlbfs_read_iter,
	.mmap			= hugetlbfs_file_mmap,  // 
	.fsync			= noop_fsync,
	.get_unmapped_area	= hugetlb_get_unmapped_area,
	.llseek			= default_llseek,
	.fallocate		= hugetlbfs_fallocate,
};
```

1. 就是一个 fs instance 只能处理一种大小的
```c
static inline struct hstate *hstate_file(struct file *f)
{
	return hstate_inode(file_inode(f));
}

static inline struct hstate *hstate_inode(struct inode *i)
{
	return HUGETLBFS_SB(i->i_sb)->hstate;
}

struct hugetlbfs_sb_info {
	long	max_inodes;   /* inodes allowed */
	long	free_inodes;  /* inodes free */
	spinlock_t	stat_lock;
	struct hstate *hstate;
	struct hugepage_subpool *spool;
	kuid_t	uid;
	kgid_t	gid;
	umode_t mode;
};
```



## 文件上的操作
1. 按照道理来说 : 不需要任何特殊的处理啊! @todo 检查一下其中区别的

```c
static const struct inode_operations hugetlbfs_dir_inode_operations = {
	.create		= hugetlbfs_create,
	.lookup		= simple_lookup,
	.link		= simple_link,
	.unlink		= simple_unlink,
	.symlink	= hugetlbfs_symlink,
	.mkdir		= hugetlbfs_mkdir,
	.rmdir		= simple_rmdir,
	.mknod		= hugetlbfs_mknod,
	.rename		= simple_rename,
	.setattr	= hugetlbfs_setattr,
	.tmpfile	= hugetlbfs_tmpfile,
};

static const struct inode_operations hugetlbfs_inode_operations = {
	.setattr	= hugetlbfs_setattr,
};
```

## ops

猜测主要是处理 hugetlb inode 中间增加附加数据

```c
static const struct super_operations hugetlbfs_ops = {
	.alloc_inode    = hugetlbfs_alloc_inode,
	.free_inode     = hugetlbfs_free_inode,
	.destroy_inode  = hugetlbfs_destroy_inode,
	.evict_inode	= hugetlbfs_evict_inode,
	.statfs		= hugetlbfs_statfs,
	.put_super	= hugetlbfs_put_super,
	.show_options	= hugetlbfs_show_options,
};
```
