## mount 的实现
1. 系统的初始化的过程挂载根系统
2. 插入u盘的挂载

```c
// 除了那些标准操作之外，被mount_bdev唯一调用
struct block_device *blkdev_get_by_path(const char *path, fmode_t mode,
					void *holder)
```

## blkdev_get_by_path && blkdev_get_by_dev
1. blkdev_get_by_path
    1. rely on `lookup_bdev => kern_path`
    1. usage : get_tree_bdev, get_tree_bdev, both of them has parameter of `fill_super` type
    2. @todo so, why and how the kernel put the device on the fs system ?
        1. bd_type is mount by kern mount !!!!!! not exposed to user space !

2. blkdev_get
  1. bdget => iget5_locked
  2. iget5_locked : details of it reside in fs/inode.c, it seems that it search for the inode in the super_block !
      1. @todo OK, /dev/block/7:0 is associated with inode in the `blockdev_superblock`

```c
struct block_device *bdget(dev_t dev)
{
	inode = iget5_locked(blockdev_superblock, hash(dev), //
			bdev_test, bdev_set, &dev);
}
```


## bd_type
1. block_dev.c provide device for ext2 mount, but it has to kern_mount too !
1. there are so many kern fs, how to find them ? (maybe, the global variable `blockdev_superblock`)
3. who mount to /dev ?

```c

static int bd_init_fs_context(struct fs_context *fc) // XXX TODO in block_dev.c, we don't init the fc->ops->get_tree, and no fill_super
{
	struct pseudo_fs_context *ctx = init_pseudo(fc, BDEVFS_MAGIC);
	if (!ctx)
		return -ENOMEM;
	fc->s_iflags |= SB_I_CGROUPWB;
	ctx->ops = &bdev_sops;
	return 0;
}

struct super_block *blockdev_superblock __read_mostly;
EXPORT_SYMBOL_GPL(blockdev_superblock);

static const struct super_operations bdev_sops = { // the reason we need this super_operations is that we want to create the bdev_inode !
	.statfs = simple_statfs,
	.alloc_inode = bdev_alloc_inode,
	.free_inode = bdev_free_inode,
	.drop_inode = generic_delete_inode,
	.evict_inode = bdev_evict_inode,
};

static struct file_system_type bd_type = {
	.name		= "bdev",
	.init_fs_context = bd_init_fs_context,
	.kill_sb	= kill_anon_super,
};

struct bdev_inode {
	struct block_device bdev;
	struct inode vfs_inode;
};

/*
 * Common helper for pseudo-filesystems (sockfs, pipefs, bdev - stuff that
 * will never be mountable)
 */
struct pseudo_fs_context *init_pseudo(struct fs_context *fc,
					unsigned long magic)
{
	struct pseudo_fs_context *ctx;

	ctx = kzalloc(sizeof(struct pseudo_fs_context), GFP_KERNEL);
	if (likely(ctx)) {
		ctx->magic = magic;
		fc->fs_private = ctx;
		fc->ops = &pseudo_fs_context_ops;
		fc->sb_flags |= SB_NOUSER;
		fc->global = true;
	}
	return ctx;
}
```



Former version :
```c
static struct dentry *bd_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_pseudo(fs_type, "bdev:", &bdev_sops, NULL, 0x62646576);
}

static struct file_system_type bd_type = {
	.name		= "bdev",
	.mount		= bd_mount,
	.kill_sb	= kill_anon_super,
};
```

I believe that mount_pseudo has beed transform to this :
```c
/*
 * Common helper for pseudo-filesystems (sockfs, pipefs, bdev - stuff that
 * will never be mountable)
 */
struct pseudo_fs_context *init_pseudo(struct fs_context *fc,
					unsigned long magic)
{
	struct pseudo_fs_context *ctx;

	ctx = kzalloc(sizeof(struct pseudo_fs_context), GFP_KERNEL);
	if (likely(ctx)) {
		ctx->magic = magic;
		fc->fs_private = ctx;
		fc->ops = &pseudo_fs_context_ops;
		fc->sb_flags |= SB_NOUSER;
		fc->global = true;
	}
	return ctx;
}
```
