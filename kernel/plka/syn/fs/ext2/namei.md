# namei

```c
static int ext2_mkdir(struct inode * dir, struct dentry * dentry, umode_t mode)


// 调用我
const struct inode_operations ext2_dir_inode_operations = {
	.mkdir		= ext2_mkdir,

// 分析我
// 1. 分配一个inode
// 2. 初始化三个operation, 才发现一共含有5个内容ops
// 然后分别调用如下三个函数

// 创建的新的dir 需要立刻向其中添加 . 和 .. 两个entry
int ext2_make_empty(struct inode *inode, struct inode *parent)

// 将当前的dentry 放到parent inode 的列表中间
int ext2_add_link (struct dentry *dentry, struct inode *inode)

// 处理dcache相关的内容
/*
 * This should be equivalent to d_instantiate() + unlock_new_inode(),
 * with lockdep-related part of unlock_new_inode() done before
 * anything else.  Use that instead of open-coding d_instantiate()/
 * unlock_new_inode() combinations.
 */
void d_instantiate_new(struct dentry *entry, struct inode *inode)
```

```c
// 无聊的函数，封装+错误处理
static inline int ext2_add_nondir(struct dentry *dentry, struct inode *inode)
{
	int err = ext2_add_link(dentry, inode);
	if (!err) {
		d_instantiate_new(dentry, inode);
		return 0;
	}
	inode_dec_link_count(inode);
	discard_new_inode(inode);
	return err;
}
```

```c
ext2_lookup
ext2_get_parent
ext2_create // TODO 其实很奇怪，为什么 ext2_dir_inode_operations 的 create 初始化创建的是
ext2_tmpfile  // 神奇的系统调用
ext2_mknod // 没有什么神奇的地方相对于ext2_create, 调用一个神奇的 init_special_inode

ext2_symlink // 创建新的符号链接
// 似乎i_data 中间的内容
// TODO 参考一下书，这个有点复杂!

ext2_link // 直观的实现

ext2_unlink
ext2_rmdir
ext2_rename
// rename 使用了几个关键的dir.c 的函数
// 1. ext2_add_link
// 2. ext2_empty_dir
// 3. ext2_find_entry
// 4. ext2_set_link
// 5. ext2_delete_entry
```



> 外部问题
```c
/**
 * d_splice_alias - splice a disconnected dentry into the tree if one exists
 * @inode:  the inode which may have a disconnected dentry
 * @dentry: a negative dentry which we want to point to the inode.
 *
 * If inode is a directory and has an IS_ROOT alias, then d_move that in
 * place of the given dentry and return it, else simply d_add the inode
 * to the dentry and return NULL.
 *
 * If a non-IS_ROOT directory is found, the filesystem is corrupt, and
 * we should error out: directories can't have multiple aliases.
 *
 * This is needed in the lookup routine of any filesystem that is exportable
 * (via knfsd) so that we can build dcache paths to directories effectively.
 *
 * If a dentry was found and moved, then it is returned.  Otherwise NULL
 * is returned.  This matches the expected return value of ->lookup.
 *
 * Cluster filesystems may call this function with a negative, hashed dentry.
 * In that case, we know that the inode will be a regular file, and also this
 * will only occur during atomic_open. So we need to check for the dentry
 * being already hashed only in the final case.
 */
struct dentry *d_splice_alias(struct inode *inode, struct dentry *dentry)

// 1. 这个函数每一个开始一个 dquot_initialize 这个东西是 2015 年添加进去的
// 2. 若隐若现的kmap 让人厌烦
// 3. 
```







> 最后添加的内容
```c
const struct inode_operations ext2_dir_inode_operations = {
	.create		= ext2_create,
	.lookup		= ext2_lookup,
	.link		= ext2_link,
	.unlink		= ext2_unlink,
	.symlink	= ext2_symlink,
	.mkdir		= ext2_mkdir,
	.rmdir		= ext2_rmdir,
	.mknod		= ext2_mknod,
	.rename		= ext2_rename,
#ifdef CONFIG_EXT2_FS_XATTR
	.listxattr	= ext2_listxattr,
#endif
	.setattr	= ext2_setattr,
	.get_acl	= ext2_get_acl,
	.set_acl	= ext2_set_acl,
	.tmpfile	= ext2_tmpfile,
};

const struct inode_operations ext2_special_inode_operations = {
#ifdef CONFIG_EXT2_FS_XATTR
	.listxattr	= ext2_listxattr,
#endif
	.setattr	= ext2_setattr,
	.get_acl	= ext2_get_acl,
	.set_acl	= ext2_set_acl,
};
```

