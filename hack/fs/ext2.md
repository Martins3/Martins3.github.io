# ext2
> 按照 minfs 的风格走一遍


<!-- vim-markdown-toc GitLab -->

- [src tree layout](#src-tree-layout)
- [disk layout](#disk-layout)
- [important function](#important-function)
  - [inode.c](#inodec)
    - [ext2_get_block](#ext2_get_block)
    - [ext2_iget](#ext2_iget)
    - [ext2_fill_super](#ext2_fill_super)
  - [dir.c](#dirc)
    - [ext2_readdir](#ext2_readdir)
  - [namei.c](#nameic)
    - [ext2_rmdir](#ext2_rmdir)
    - [ext2_symlink](#ext2_symlink)
    - [ext2_create](#ext2_create)
    - [ext2_add_link](#ext2_add_link)
    - [ext2_mknod && ext2_create](#ext2_mknod-ext2_create)
    - [ext2_lookup](#ext2_lookup)

<!-- vim-markdown-toc -->


## src tree layout
1. iname.c : 处理各种 inode.c 的东西，
2. file.c : 处理各种 file operations 的内容



## disk layout
通过 ext2_inode_info 不能知道 ext2 在文件系统中间的布局到底是如何，
但是可以靠 ext2_inode。
ext2_inode 似乎就是磁盘的原始表示，然后会加载到 ext2_inode_info 中间。
 ext2_inode 和 ext2_inode_info 的关系 : disk 和 内存
```c
/*
 * second extended file system inode data in memory
 */
struct ext2_inode_info {
```
```c
/*
 * Structure of an inode on the disk
 */
struct ext2_inode {
```

## important function

### inode.c
#### ext2_get_block
// TODO
1. 据说可以用于分析 block 布局
translate a block of a file into a block on the device.

#### ext2_iget

1. 初步分析 ino 是什么 ?
ext2 被反复挂载，这些 root inode 的 ino 都是 EXT2_ROOT_INO
```c
root = ext2_iget(sb, EXT2_ROOT_INO);
```
关键问题在于: find_inode_fast，由于也使用 sb 作为 hash，
让即使两个 inode 的 ino 相同，被定位在一起的概率也很小。
所以 ino 是文件系统和磁盘之间的通信，但是 VFS 之间沟通应该更多的是直接使用
vfs inode 的指针吧。

调用路线:
1. iget_locked : ino 作为真正的索引，用于快速定位某一个 inode 是否存在于操作系统中间。
    1. 使用 find_inode_fast 来在 inode cache 中间查找
    2. alloc_inode 实现加载和基本初始化，并且使用 ino 初始化 inode。注意，此时获取的 inode 并没有磁盘信息，还是一个空的。
2. ext2_get_inode : 使用参数 ino 从磁盘中间加载需要的 raw_inode.
3. 利用 ext2_get_inode 的返回值初始化 inode 磁盘中间的信息，除此之外，需要进行各种运行时信息的初始化:
    1. ext2_set_file_ops
    2. init_special_inode
    3. ..

#### ext2_fill_super


### dir.c
> TODO
> 不得不说，file_operations 到底提供给多少了类型的 inode
> 总结一下，
> 1. 有的函数不能提供给特定类型的 : read 对于 ext2_dir_operations 没有意义


```c
const struct file_operations ext2_dir_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.iterate_shared	= ext2_readdir,
	.unlocked_ioctl = ext2_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= ext2_compat_ioctl,
#endif
	.fsync		= ext2_fsync,
};
```

#### ext2_readdir
easy，遍历所有的 dir，然后 dir_emit 将 dir 传送到用户层

### namei.c

#### ext2_rmdir
有文件就直接报错

```c
static int ext2_rmdir (struct inode * dir, struct dentry *dentry)
{
	struct inode * inode = d_inode(dentry);
	int err = -ENOTEMPTY;

	if (ext2_empty_dir(inode)) { // 难道这就是为什么，每次删除还有内容的文件夹就会报错
		err = ext2_unlink(dir, dentry);
		if (!err) {
			inode->i_size = 0;
			inode_dec_link_count(inode);
			inode_dec_link_count(dir);
		}
	}
	return err;
}
```

#### ext2_symlink
hardlink 中间是，两个 dentry 指向同一个 inode

symlink 是创建一个自己的 inode ，并且存储指向的位置的地址，如果指向的文件被删除了，当访问 symlink 就直接消失了

1. fast symlink 和 slow symlink 的区别:
如果指向的路径的名称可以存储在 : `ext2_inode_info->i_data`，那么就是 fast，否则就需要访问数据盘获取路径。

2. 所以，slow 版本创建的时候，使用
```c
int page_symlink(struct inode *inode, const char *symname, int len)
{
	return __page_symlink(inode, symname, len,
			!mapping_gfp_constraint(inode->i_mapping, __GFP_FS)); // __page_symlink 这个函数非常简单的
}
```
3. file_operations::get_link 的通用注册对象 page_get_link 的实现也算是非常

4. symlink 可以用于文件夹，但是 hardlink 不可以。
> 为什么 hardlink 不可以，应该是防止递归吧!

#### ext2_create
dentry 已经创建，现在创建与其关键的 inode
```c
/*
 * By the time this is called, we already have created
 * the directory cache entry for the new file, but it
 * is so far negative - it has no inode.
 *
 * If the create succeeds, we fill in the inode information
 * with d_instantiate().
 */
static int ext2_create (struct inode * dir, struct dentry * dentry, umode_t mode, bool excl)
{
	struct inode *inode;
	int err;

	err = dquot_initialize(dir); // quata 相关的
	if (err)
		return err;

	inode = ext2_new_inode(dir, mode, &dentry->d_name);
	if (IS_ERR(inode))
		return PTR_ERR(inode);

	ext2_set_file_ops(inode);
	mark_inode_dirty(inode);
	return ext2_add_nondir(dentry, inode);
}
```

中 ext2_new_inode 调用 new_inode 在内存中间分配一个该文件系统的 inode，然后查询磁盘找到空闲的 inode，并且进行初始化。

主要的四个步骤:
- Introduces a new entry into the physical structure on the disk; the update of the bit maps on the disk must not be forgotten.
- Configures access rights to those received as a parameter.
- Marks the inode as dirty with the mark_inode_dirty() function.
- Instantiates the directory entry (dentry) with the d_instantiate function.

#### ext2_add_link

代码分析: 将 inode 和 dentry 关联起来
```c
int ext2_add_link (struct dentry *dentry, struct inode *inode) //
```


1. ext2_link:
```c

static int ext2_link (struct dentry * old_dentry, struct inode * dir,
	struct dentry *dentry)
{
	struct inode *inode = d_inode(old_dentry); // 从 old_dentry 获取共享的 inode
	int err;

	err = dquot_initialize(dir); // quota 相关
	if (err)
		return err;

	inode->i_ctime = current_time(inode);
	inode_inc_link_count(inode); // 增加 i_nlink
	ihold(inode); // 增加 i_count，TODO i_count 的作用 ?

	err = ext2_add_link(dentry, inode); // 构建联系
	if (!err) {
		d_instantiate(dentry, inode); // 使用 inode 的信息进行填充，从而实现 detail
		return 0;
	}
	inode_dec_link_count(inode);
	iput(inode);
	return err;
}
```

2. ext2_add_nondir : 这不就是 ext2_link 的简化版，直接提供了 inode 而已，应该就是创建新的 inode 的时候调用的吧，调用位置:
    1. ext2_create
    2. ext2_mknod
    3. ext2_symlink

```c
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

/*
 * This should be equivalent to d_instantiate() + unlock_new_inode(),
 * with lockdep-related part of unlock_new_inode() done before
 * anything else.  Use that instead of open-coding d_instantiate()/
 * unlock_new_inode() combinations.
 */
void d_instantiate_new(struct dentry *entry, struct inode *inode)
```

3. ext2_mkdir : 非常简单了，和创建文件没有特别大的区别，除了使用 ext2_make_empty 在该文件夹中间创建 . 和 ..
4. ext2_rename : 也是很简单的内容

#### ext2_mknod && ext2_create
几乎是相同的套路:
1. dquot_initialize
2. ext2_new_inode : 创建一个 inode，应该是由于 inode 号码不可以随意分配，所以，即使是 mknod 也是需要让　访问磁盘寻找合适的 inode，如此销毁也可以复用通用的处理，考虑到 mknod 调用也不是很频繁。
3. mark_inode_dirty
4. ext2_add_nondir
不同的位置 : init_special_inode 和 ext2_set_file_ops，只是设置一下 file operation 不同而已。

之所以还是被拆分为不同的函数，是因为 rdev 这个参数。
```c
/*
 * By the time this is called, we already have created
 * the directory cache entry for the new file, but it
 * is so far negative - it has no inode.
 *
 * If the create succeeds, we fill in the inode information
 * with d_instantiate().
 */
static int ext2_create (struct inode * dir, struct dentry * dentry, umode_t mode, bool excl)
{
	struct inode *inode;
	int err;

	err = dquot_initialize(dir);
	if (err)
		return err;

	inode = ext2_new_inode(dir, mode, &dentry->d_name);
	if (IS_ERR(inode))
		return PTR_ERR(inode);

	ext2_set_file_ops(inode);
	mark_inode_dirty(inode);
	return ext2_add_nondir(dentry, inode);
}

static int ext2_mknod (struct inode * dir, struct dentry *dentry, umode_t mode, dev_t rdev)
{
	struct inode * inode;
	int err;

	err = dquot_initialize(dir);
	if (err)
		return err;

	inode = ext2_new_inode (dir, mode, &dentry->d_name);
	err = PTR_ERR(inode);
	if (!IS_ERR(inode)) {
		init_special_inode(inode, inode->i_mode, rdev);
#ifdef CONFIG_EXT2_FS_XATTR
		inode->i_op = &ext2_special_inode_operations;
#endif
		mark_inode_dirty(inode);
		err = ext2_add_nondir(dentry, inode);
	}
	return err;
}
```

#### ext2_lookup
This function is called indirectly when information about the inode associated with an entry in a directory is needed.
Searces in the directory indicated by dir the entry having the name `dentry->d_name.name`.
If the entry is found, it will return NULL and associate the inode with the name using the d_add() function.

1. ext2_inode_by_name : 对于 dir 中间搜索，获取 ino
2. ext2_iget : 利用 ino 获取 inode
3. d_splice_alias : TODO 处理文件的很简单，将 dentry 和 inode 关联起来就可以了，由于 link 的原因，inode 可以对应多个 dentry，所以需要使用链表管理起来，很正常的。

`d_alias` links the dentry objects of identical files. This situation arises when links are used to
make the file available under two different names. This list is linked from the corresponding
inode by using its `i_dentry` element as a list head. The individual dentry objects are linked by
`d_alias`.

> 其中的证据：
```c
static struct dentry * __d_find_any_alias(struct inode *inode)
{
	struct dentry *alias;

	if (hlist_empty(&inode->i_dentry))
		return NULL;
  // 1. inode->i_dentry 用于挂载 dentry
  // 2. dentry 使用 d_alias 吧自己挂上去
	alias = hlist_entry(inode->i_dentry.first, struct dentry, d_u.d_alias);
	__dget(alias);
	return alias;
}
```

分析 file_operations::lookup 的调用位置:
1. `__lookup_hash`
2. `__lookup_slow`
3. lookup_open
