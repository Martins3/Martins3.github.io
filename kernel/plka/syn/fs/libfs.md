# fs/libfs.c

1. libfs seems to be a fundation of : seq_flie.c kprobe.c and debugfs/
    1. or is the function of all the virtual (not based on block device) fs 
    2. so maybe the proc and sysfs rely on libfs too.

## KeyNote

| Name                                                                                                               | desc                                |
|--------------------------------------------------------------------------------------------------------------------|-------------------------------------|
| `ssize_t simple_read_from_buffer(void __user *to, size_t count, loff_t *ppos, const void *from, size_t available)` | make some check and copy_to_user    |
| `ssize_t simple_write_to_buffer(void *to, size_t available, loff_t *ppos, const void __user *from, size_t count)`  | similar  : copy_from_user           |
| `ssize_t memory_read_from_buffer(void *to, size_t count, loff_t *ppos, const void *from, size_t available)`        | similar  : memcpy. used by derivers |
| simple_write_begin simple_write_end | @todo  |


## simple_dir_operations
1. usage : debugfs and many others

```c
const struct file_operations simple_dir_operations = {
	.open		= dcache_dir_open,
	.release	= dcache_dir_close,
	.llseek		= dcache_dir_lseek,
	.read		= generic_read_dir,
	.iterate_shared	= dcache_readdir, // XXX this is used by the syscall Man getdents64(2), todo by far, I haven't dig into the details
	.fsync		= noop_fsync,
};
EXPORT_SYMBOL(simple_dir_operations);
```

```c
int dcache_dir_open(struct inode *inode, struct file *file)
{
	file->private_data = d_alloc_cursor(file->f_path.dentry); // todo maybe related to dcache

	return file->private_data ? 0 : -ENOMEM;
}

ssize_t generic_read_dir(struct file *filp, char __user *buf, size_t siz, loff_t *ppos) // todo strongly, guess that, this is because syscall readdir(2) is obsolete
{
	return -EISDIR;
}
```



## init_pseudo

```c
static int pseudo_fs_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, pseudo_fs_fill_super);
}

static void pseudo_fs_free(struct fs_context *fc)
{
	kfree(fc->fs_private); // 释放 pseudo_fs_context
}


static const struct fs_context_operations pseudo_fs_context_ops = {
	.free		= pseudo_fs_free,
	.get_tree	= pseudo_fs_get_tree, // ext2 / ext4 利用这一个函数实现调用 bdev_mount
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
		fc->fs_private = ctx; // 原来如此，利用 fs_private 存储 pseudo_fs_context 
		fc->ops = &pseudo_fs_context_ops; 
		fc->sb_flags |= SB_NOUSER;
		fc->global = true;
	}
	return ctx;
}
EXPORT_SYMBOL(init_pseudo);
```

## pseudo_fs_get_tree


```c
static int pseudo_fs_get_tree(struct fs_context *fc)
{
	return get_tree_nodev(fc, pseudo_fs_fill_super); // 对于获取到的 super_block 进行初始化
  // get_tree_nodev 进入到 super.c 中间
}
```

## pseudo_fs_fill_super && simple_fill_super
1. simple_super_operations : several usage, [trace fs](https://lwn.net/Articles/630526/), binfmt_misc and debugfs
2. pseudo_fs_fill_super : init_pseudo => pseudo_fs_context_ops::pseudo_fs_get_tree

3. pseudo_fs_fill_super is used for kern_mount, and simple_super_operations is used for real fs, debugfs for example:

```c
static struct dentry *debug_mount(struct file_system_type *fs_type,
			int flags, const char *dev_name,
			void *data)
{
	return mount_single(fs_type, flags, data, debug_fill_super);
}

static struct file_system_type debug_fs_type = {
	.owner =	THIS_MODULE,
	.name =		"debugfs",
	.mount =	debug_mount,
	.kill_sb =	kill_litter_super,
};
MODULE_ALIAS_FS("debugfs");
```

