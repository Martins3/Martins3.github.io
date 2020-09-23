# block_dev.c 分析
> 主要的内容 : 实现 everything is file 的设计思路，不是放到fs下面实现block io 的


## Doc
1. https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2018/06/14/linux-block-device-driver

When we add devices to system, a node in /dev/ will be created, this is done in ‘devtmpfs_create_node’.

2. https://unix.stackexchange.com/questions/176215/difference-between-dev-and-sys
3. https://sungju.github.io/kernel/internals/block_device_driver.html : 下面的插图有意思

![](https://sungju.github.io/kernel/internals/block_device_driver.jpg)


## KeyNote
1. device_add => device_create_file


### Question
1. block_dev 是不是成为block layer 的屏蔽层，这是文件系统看到的全部的内容
2. swap 的 partion 是如何自动被识别出来的 ?
3. ext4文件系统注册在不同的block 上，所以如何实现submit_bio 进入到不同的block中间里面。
4. 现在，觉得，block_dev.c 只是 block layer 需要使用fs 的内容而已，block 保证几个page 的io，但是全局的io 需要fs 提供。
5. 注意，block 上也可以挂载inode 和 address_space，猜测，仅仅限于，当没有fs 挂载到其上的时候 !

6. so, what is `struct block_device` ?
      1. relation with module ?
      2. I want to read the routin that the bios(or something else probe the device and create bdev_inode on the bdev file system )

8. blockdev_superblock :
    1. mount details ? f

9. how to add block device to the bdev file system ?

10. why we don't need char file system ?

11. how dev_t involved ?
      1. dev_t => inode
      2. dev_t => pathname
      3. dev_t alloc
      4. /sys

```c
struct block_device {
// ...
	struct gendisk *	bd_disk;
	struct request_queue *  bd_queue;
	struct backing_dev_info *bd_bdi;
// ...
```
block size 和 sector size 的区别是什么 ?

## link
1. 主要是一些驱动调用的

1. 为什么需要 symbol link
2. 从哪里创建到哪里的 symbol link 的　: bd_find_holder_disk 的注释说过，但是看不懂


```c
#ifdef CONFIG_SYSFS // 完全在这一个 config 的包围下
struct bd_holder_disk {
	struct list_head	list;
	struct gendisk		*disk;
	int			refcnt;
};

static struct bd_holder_disk *bd_find_holder_disk(struct block_device *bdev,
						  struct gendisk *disk)
{
	struct bd_holder_disk *holder;

	list_for_each_entry(holder, &bdev->bd_holder_disks, list)
		if (holder->disk == disk)
			return holder;
	return NULL;
}

static int add_symlink(struct kobject *from, struct kobject *to)
{
	return sysfs_create_link(from, to, kobject_name(to));
}

static void del_symlink(struct kobject *from, struct kobject *to)
{
	sysfs_remove_link(from, kobject_name(to));
}

/**
 * bd_link_disk_holder - create symlinks between holding disk and slave bdev
 * @bdev: the claimed slave bdev
 * @disk: the holding disk
 *
 * DON'T USE THIS UNLESS YOU'RE ALREADY USING IT.
 *
 * This functions creates the following sysfs symlinks.
 *
 * - from "slaves" directory of the holder @disk to the claimed @bdev
 * - from "holders" directory of the @bdev to the holder @disk
 *
 * For example, if /dev/dm-0 maps to /dev/sda and disk for dm-0 is
 * passed to bd_link_disk_holder(), then:
 *
 *   /sys/block/dm-0/slaves/sda --> /sys/block/sda
 *   /sys/block/sda/holders/dm-0 --> /sys/block/dm-0
 *
 * The caller must have claimed @bdev before calling this function and
 * ensure that both @bdev and @disk are valid during the creation and
 * lifetime of these symlinks.
 *
 * CONTEXT:
 * Might sleep.
 *
 * RETURNS:
 * 0 on success, -errno on failure.
 */
int bd_link_disk_holder(struct block_device *bdev, struct gendisk *disk)
{
	struct bd_holder_disk *holder;
	int ret = 0;

	mutex_lock(&bdev->bd_mutex);

	WARN_ON_ONCE(!bdev->bd_holder);

	/* FIXME: remove the following once add_disk() handles errors */
	if (WARN_ON(!disk->slave_dir || !bdev->bd_part->holder_dir))
		goto out_unlock;

	holder = bd_find_holder_disk(bdev, disk);
	if (holder) {
		holder->refcnt++;
		goto out_unlock;
	}

	holder = kzalloc(sizeof(*holder), GFP_KERNEL);
	if (!holder) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	INIT_LIST_HEAD(&holder->list);
	holder->disk = disk;
	holder->refcnt = 1;

	ret = add_symlink(disk->slave_dir, &part_to_dev(bdev->bd_part)->kobj);
	if (ret)
		goto out_free;

	ret = add_symlink(bdev->bd_part->holder_dir, &disk_to_dev(disk)->kobj);
	if (ret)
		goto out_del;
	/*
	 * bdev could be deleted beneath us which would implicitly destroy
	 * the holder directory.  Hold on to it.
	 */
	kobject_get(bdev->bd_part->holder_dir);

	list_add(&holder->list, &bdev->bd_holder_disks);
	goto out_unlock;

out_del:
	del_symlink(disk->slave_dir, &part_to_dev(bdev->bd_part)->kobj);
out_free:
	kfree(holder);
out_unlock:
	mutex_unlock(&bdev->bd_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(bd_link_disk_holder);

/**
 * bd_unlink_disk_holder - destroy symlinks created by bd_link_disk_holder()
 * @bdev: the calimed slave bdev
 * @disk: the holding disk
 *
 * DON'T USE THIS UNLESS YOU'RE ALREADY USING IT.
 *
 * CONTEXT:
 * Might sleep.
 */
void bd_unlink_disk_holder(struct block_device *bdev, struct gendisk *disk)
{
	struct bd_holder_disk *holder;

	mutex_lock(&bdev->bd_mutex);

	holder = bd_find_holder_disk(bdev, disk);

	if (!WARN_ON_ONCE(holder == NULL) && !--holder->refcnt) {
		del_symlink(disk->slave_dir, &part_to_dev(bdev->bd_part)->kobj);
		del_symlink(bdev->bd_part->holder_dir,
			    &disk_to_dev(disk)->kobj);
		kobject_put(bdev->bd_part->holder_dir);
		list_del_init(&holder->list);
		kfree(holder);
	}

	mutex_unlock(&bdev->bd_mutex);
}
EXPORT_SYMBOL_GPL(bd_unlink_disk_holder);
#endif
```


## freeze_bdev 和 thaw_bdev



## blkdev_get 和 blkdev_put
1. 好像，这就是给正儿八经的文件系统 mount 到特定的设备上需要的, @todo 所以，过程是下面这样的吗 ?
      1. 系统启动，创建 vfs，/dev 之类的东西
      2. 然后 ext4 等文件系统 mount ， mount 的时候需要 /dev

2. blkdev_get 调用两个有意思的函数: blkdev_get_by_path 和 blkdev_get_by_dev

```c
void blkdev_put(struct block_device *bdev, fmode_t mode)

/**
 * blkdev_get - open a block device
 * @bdev: block_device to open
 * @mode: FMODE_* mask
 * @holder: exclusive holder identifier
 *
 * Open @bdev with @mode.  If @mode includes %FMODE_EXCL, @bdev is
 * open with exclusive access.  Specifying %FMODE_EXCL with %NULL
 * @holder is invalid.  Exclusive opens may nest for the same @holder.
 *
 * On success, the reference count of @bdev is unchanged.  On failure,
 * @bdev is put.
 *
 * CONTEXT:
 * Might sleep.
 *
 * RETURNS:
 * 0 on success, -errno on failure.
 */
int blkdev_get(struct block_device *bdev, fmode_t mode, void *holder)
```

## fs
1. bdev 这个 fs 怎么使用，谁使用 ?
2. fs_context 都是搞什么的 ?

```c
static const struct super_operations bdev_sops = {
	.statfs = simple_statfs,
	.alloc_inode = bdev_alloc_inode,
	.free_inode = bdev_free_inode,
	.drop_inode = generic_delete_inode,
	.evict_inode = bdev_evict_inode,
};

static int bd_init_fs_context(struct fs_context *fc)
{
	struct pseudo_fs_context *ctx = init_pseudo(fc, BDEVFS_MAGIC);
	if (!ctx)
		return -ENOMEM;
	fc->s_iflags |= SB_I_CGROUPWB;
	ctx->ops = &bdev_sops;
	return 0;
}

static struct file_system_type bd_type = {
	.name		= "bdev",
	.init_fs_context = bd_init_fs_context,
	.kill_sb	= kill_anon_super,
};
```



## sync

```c
// 感觉block_dev 应该是最底层的东西，但是，实际上，居然调用 filemap 的实现
// 这两个函数的最终调用 :  do_writepages 也就是实际的写会的操作还是 ext2 ext4 文件系统的writeback
// TODO 所以block_dev.c 的核心目的是什么 ?
// 反而 fs/sync.c 中间的 int sync_filesystem(struct super_block *sb) 反而调用比较正常
int __sync_blockdev(struct block_device *bdev, int wait)
{
	if (!bdev)
		return 0;
	if (!wait)
		return filemap_flush(bdev->bd_inode->i_mapping);
	return filemap_write_and_wait(bdev->bd_inode->i_mapping);
}

// 被block_layer 调用，比如invalidate dev 的时候，首先将数据写会
/*
 * Write out and wait upon all dirty data associated with this
 * device.   Filesystem data as well as the underlying block
 * device.  Takes the superblock lock.
 */
int fsync_bdev(struct block_device *bdev)
{

  // 居然block 上需要划分是否挂载了文件系统的情况
  // 是不是，当block_device 上用于swap 的时候，没有fs 然后 。。。
	struct super_block *sb = get_super(bdev);
	if (sb) {
		int res = sync_filesystem(sb);
		drop_super(sb);
		return res;
	}
	return sync_blockdev(bdev);
}
```

## interface

```c
// 曾经的设想，这些操作是将设备当做文件处理的基础，但是 ?
// todo 疯狂打脸，block_dev 为什么会需要这个 fs

// TODO 普通文件是如何处理 super_operations 的 ?
static const struct super_operations bdev_sops = {
	.statfs = simple_statfs,
	.alloc_inode = bdev_alloc_inode,
	.destroy_inode = bdev_destroy_inode,
	.drop_inode = generic_delete_inode,
	.evict_inode = bdev_evict_inode,
};


// 这就已经很难解释了 ! (
// 其实不难解释 :
// 1. 将设备当做文件处理，现在提供一些通用的操作而已
// 2. 既可以将设备当做文件处理，同时，设备又是fs的基础
// 3. 注意 : 既然 block dev 要利用 vfs，利用 inode 表示，必然就具有所有的 inode 都有 fops 和 aops
//      2. 利用 vfs 将 block 设备暴露出来其实是一个非常酷炫的事情: 可以直接 sudo less -f  /dev/nvme0n1p5 访问磁盘中间的内容啊
//      1. TODO block dev 对应的 inode 是怎么创建的 ?
//      3. 一个文件系统的功能，实现效果无非就是向上提供 fops ，向下提供 aops
//          1. 当一个文件对应的 inode 属于某一个文件系统的时候，只需要只有这些 fops 和 aops 就可以实现正常的访问了。
//          2. 所以，block dev 对应的 inode 一旦持有了对应 fops 和 aops ，其效果就像是 backed by some fs 的效果。
//          3. TODO 那么 block_dev.c 想要提供的功能就是这有这吗 ?
//
// XXX : 需要理解文件系统的作用到底是什么: 如果只有一个文件，那么我需要文件系统吗 ? (显然不需要) (其实 fs 的关键在于实现 get_block_t，给定一个 inode 以及 偏移量，找到对应的 block number ?
//
// 3. TODO 所以将/dev/nvme0n1p5 暴露出来的作用是什么 ? 显然是因为存在部分操作需要越过fs
// 4. TODO 需要借助ucore 理解一下根文件系统挂载 /dev/nvme0n1p5 同时，这是根的所在的地方，这种鸡生蛋的问题 !

static const struct address_space_operations def_blk_aops = {
	.readpage	= blkdev_readpage,
	.readpages	= blkdev_readpages,
	.writepage	= blkdev_writepage,
	.write_begin	= blkdev_write_begin,
	.write_end	= blkdev_write_end,
	.writepages	= blkdev_writepages,
	.releasepage	= blkdev_releasepage,
	.direct_IO	= blkdev_direct_IO,
	.is_dirty_writeback = buffer_check_dirty_writeback,
};

const struct file_operations def_blk_fops = {
	.open		= blkdev_open, // todo 很有意思的，似乎是驱动首先注册之后就可以使用了
	.release	= blkdev_close,
	.llseek		= block_llseek,
	.read_iter	= blkdev_read_iter,
	.write_iter	= blkdev_write_iter,
	.mmap		= generic_file_mmap,
	.fsync		= blkdev_fsync,
	.unlocked_ioctl	= block_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= compat_blkdev_ioctl,
#endif
	.splice_read	= generic_file_splice_read,
	.splice_write	= iter_file_splice_write,
	.fallocate	= blkdev_fallocate,
};
```

## mount的实现 :
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

