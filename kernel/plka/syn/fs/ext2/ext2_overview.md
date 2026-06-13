# ext2 analyze
> 不理解ext2 直接上ext4 是有点反人类的

| File             | blank | comment | code |
|------------------|-------|---------|------|
| super.c          | 180   | 150     | 1338 |
| inode.c          | 158   | 398     | 1113 |
| balloc.c         | 148   | 542     | 869  |
| xattr.c          | 90    | 214     | 730  |
| dir.c            | 64    | 77      | 586  |
| ext2.h           | 84    | 213     | 542  |
| ialloc.c         | 74    | 142     | 467  |
| namei.c          | 60    | 49      | 323  |
| acl.c            | 25    | 24      | 217  |
| file.c           | 23    | 43      | 140  |
| ioctl.c          | 25    | 25      | 138  |
| xattr.h          | 25    | 9       | 81   |
| acl.h            | 13    | 6       | 53   |
| xattr_security.c | 7     | 5       | 46   |
| xattr_user.c     | 6     | 7       | 36   |
| xattr_trusted.c  | 5     | 7       | 30   |
| symlink.c        | 3     | 19      | 16   |
| Makefile         | 3     | 4       | 6    |

> dir.c 主要做什么工作 ?

对应09 10 chapter 的内容。

## symbol link 的支持
```c
// symlink.c
const struct inode_operations ext2_symlink_inode_operations = {
	.get_link	= page_get_link,
	.setattr	= ext2_setattr,
#ifdef CONFIG_EXT2_FS_XATTR
	.listxattr	= ext2_listxattr,
#endif
};
 
const struct inode_operations ext2_fast_symlink_inode_operations = {
	.get_link	= simple_get_link,
	.setattr	= ext2_setattr,
#ifdef CONFIG_EXT2_FS_XATTR
	.listxattr	= ext2_listxattr,
#endif
};
```
page_get_link 和 simple_get_link 被移动到generic的位置，所以，
vfs 的含义: 各种文件系统的共同复用的部分。



```c
// ext2_symlink_inode_operations 的两个使用位置:

// 感觉这就是 : hardlink
struct inode *ext2_iget (struct super_block *sb, unsigned long ino)

// symbolic link
static int ext2_symlink (struct inode * dir, struct dentry * dentry,
	const char * symname)
```

```c
// libfs.c
const char *simple_get_link(struct dentry *dentry, struct inode *inode,
			    struct delayed_call *done)
{
	return inode->i_link;
}
EXPORT_SYMBOL(simple_get_link);

// namei.c
/* get the link contents into pagecache */
const char *page_get_link(struct dentry *dentry, struct inode *inode,
			  struct delayed_call *callback)
{
	char *kaddr;
	struct page *page;
	struct address_space *mapping = inode->i_mapping;

	if (!dentry) {
		page = find_get_page(mapping, 0);
		if (!page)
			return ERR_PTR(-ECHILD);
		if (!PageUptodate(page)) {
			put_page(page);
			return ERR_PTR(-ECHILD);
		}
	} else {
		page = read_mapping_page(mapping, 0, NULL);
		if (IS_ERR(page))
			return (char*)page;
	}
	set_delayed_call(callback, page_put_link, page);
	BUG_ON(mapping_gfp_mask(mapping) & __GFP_HIGHMEM);
	kaddr = page_address(page);
	nd_terminate_link(kaddr, inode->i_size, PAGE_SIZE - 1);
	return kaddr;
}
```

## ext2_dir_inode_operations 和 ext2_dir_operation
> 功能对比 ?

