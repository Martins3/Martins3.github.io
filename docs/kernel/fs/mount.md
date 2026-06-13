# mount

## fs/fs_context.md

## Documentation/filesystems/mount_api.txt

> The mount context is created by calling vfs_new_fs_context() or
> vfs_dup_fs_context() and is destroyed with put_fs_context().  Note that the
> structure is not refcounted.
>
> VFS, security and filesystem mount options are set individually with
> vfs_parse_mount_option().  Options provided by the old mount(2) system call as
> a page of data can be parsed with generic_parse_monolithic().

什么 option

> When mounting, the filesystem is allowed to take data from any of the pointers
> and attach it to the superblock (or whatever), provided it clears the pointer
> in the mount context.
>
> The filesystem is also allowed to allocate resources and pin them with the
> mount context.  For instance, NFS might pin the appropriate protocol version
> module.

## chroot
了解下 chroot 的实现

- https://github.com/google/nsjail
- https://github.com/proot-me/proot

## bind mount : /nix/store/

https://nixos.wiki/wiki/NFS

在系统中增加上
```nix
  fileSystems."/mnt" = {
    device = "/home/martins3/bind/";
    options = [ "bind" ];
  };
```

```txt
/dev/nvme1n1p1 on /nix/store type ext4 (ro,relatime)
/dev/nvme1n1p1 on /mnt type ext4 (rw,relatime)
```

- [ ] https://unix.stackexchange.com/questions/198590/what-is-a-bind-mount
  - 更多细节阅读下这个

## mount 的实现
1. 系统的初始化的过程挂载根系统
2. 插入 u 盘的挂载

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

## 看看 umount 是如何被阻碍的，如果存在有进程在使用一个该 mnt 上的文件，到底怎么处理的

## 分析一下 mount 中各种 options

### noexec

如果有这个 option ，这个目录中文件没有执行权限，有趣。

## 为什么 docker 中继续 mount 需要 --previlege 参数

虚拟机中继续 mount ，在外部可以观察到吗? 或者说

1. /dev/sda 如果被挂载到系统中，最后看到的是什么结果?
2. 如果是挂载 loop 的，在外部可以看到吗?


https://proot-me.github.io/

## 看看
[zhihu : mount syscall](https://zhuanlan.zhihu.com/p/36268333)
[zhihu : mount](https://zhuanlan.zhihu.com/p/76419740)

## mount

- [ ] https://www.kernel.org/doc/html/latest/filesystems/mount_api.html read the documenation

问题:
1. mount 在路径查询到时候的作用是什么 ?
2. master 和 slave 在这里是个什么概念
3. kern_mount 的时候 : 那些根本没有路径的，应该不调用 graft_tree 吧
4. 根文件系统的 mount 是怎么回事

6. 忽然想到，在本来的文件系统上，一个文件夹，比如 /home/shen/core 下面本来就是存在文件的，
在这种情况下，该文件系统被 mount ， 描述一下这种情况

TODO
1. https://zhuanlan.zhihu.com/p/93592262 新版的 mount 看这里就可以了吧，现在大致理解了 mount 的作用，先看下一个吧!


| function    | explaination                                                                                                                                                                                             |
|-------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| lock_mount  | 后挂载的文件系统会在挂载到前一次挂载的文件系统的根 dentry 上。lock_mount 这个函数的一部分逻辑就保证了在多文件系统挂载同路径的时候，让每个新挂载的文件系统都顺序的挂载（覆盖）上一次挂载实例的根 dentry。[^5] |
| lokup_mount | 通过 parent fs 的 vfsmount 和 dentry 找到在该位置上的 mount 实例，lock_mount 需要调用从而找到多次在同一个位置 mount 的最终 mount 点                                                                      |



两种特殊的 mount 情况:
1. 同一个设备可以 mount 到不同的位置
    1. 可以理解为多个进入到该文件系统可以多个入口
2. 多个设备可以 mount 到同一个目录中间
    1. 后面的隐藏前面的
    2. 多次 mount 需要多次 unmount 对应


loop device: The loop device driver transforms operations on the associated block device into file(system)operations, that's how the data/partitions end up in a file. [^3]
> 1. 问题是，loop device 的实现方法在哪里，不会是 deriver/block/loop 吧
> 2. dev/loop 的文件是做什么的 ?  `mkfs.xfs -f /dev/loop0` 的作用是啥 ?

理解一下 mount 系统调用:
实际上 flags + data 对应 mount 命令的所有-o 选项。那怎么区分哪些属于 flags 哪些属于 data 呢？
在深入内核代码一探究竟之前（本篇不深入了）我们可以通俗的认为 flags 就是所有文件系统通用的挂载选项，由 VFS 层解析。data 是每个文件系统特有的挂载选项，由文件系统自己解析。[^4]
> 其中还总结了，/mnt/mi/linux/tools/include/uapi/linux/mount.h MS 之类的宏和 mount -o 的对应关系


file_system_type + super block + mount 的关系 : file_system_type 一个文件系统中间只有一个，每一个 partitions 被 mount 之后，都可以创建一个 superblock，当其中的
1. sda1 同时挂在了/mnt/a 和/mnt/x 上，所以它有两个挂载实例对应同一个 super_block. : 当一个设备被 mount 的了，就会存在一个 superblock，但是之后无论 mount 多少次，都是只有一个
2. 每 mount 一次就会存在一个 mount 实例



相关的数据结构:
```c
// 这个结构体强调的是自己的信息
struct vfsmount {
  struct dentry *mnt_root;  /* root of the mounted tree */
  struct super_block *mnt_sb; /* pointer to superblock */
  int mnt_flags;
} __randomize_layout;

// 和其他人关系
// 1. namespace
// 2. mount
struct mount {
  struct hlist_node mnt_hash; // 全局表格
  struct mount *mnt_parent; // 爸爸是谁
  struct dentry *mnt_mountpoint; // 放在爸爸的什么位置
  struct vfsmount mnt;
  union {
    struct rcu_head mnt_rcu;
    struct llist_node mnt_llist;
  };
#ifdef CONFIG_SMP
  struct mnt_pcp __percpu *mnt_pcp;
#else
  int mnt_count;
  int mnt_writers;
#endif
  struct list_head mnt_mounts;  /* list of children, anchored here */
  struct list_head mnt_child; /* and going through their mnt_child */
  struct list_head mnt_instance;  /* mount instance on sb->s_mounts */
  const char *mnt_devname;  /* Name of device e.g. /dev/dsk/hda1 */
  struct list_head mnt_list; /* 链接到进程namespace中已挂载文件系统中，表头为mnt_namespace的list域 */
  struct list_head mnt_expire;  /* link in fs-specific expiry list */
  struct list_head mnt_share; /* circular list of shared mounts */
  struct list_head mnt_slave_list;/* list of slave mounts */
  struct list_head mnt_slave; /* slave list entry */
  struct mount *mnt_master; /* slave is on master->mnt_slave_list */
  struct mnt_namespace *mnt_ns; /* containing namespace */
  struct mountpoint *mnt_mp;  /* where is it mounted */
  union {
    struct hlist_node mnt_mp_list;  /* list mounts with the same mountpoint */
    struct hlist_node mnt_umount;
  };
  struct list_head mnt_umounting; /* list entry for umount propagation */
#ifdef CONFIG_FSNOTIFY
  struct fsnotify_mark_connector __rcu *mnt_fsnotify_marks;
  __u32 mnt_fsnotify_mask;
#endif
  int mnt_id;     /* mount identifier */
  int mnt_group_id;   /* peer group identifier */
  int mnt_expiry_mark;    /* true if marked for expiry */
  struct hlist_head mnt_pins;
  struct hlist_head mnt_stuck_children;
} __randomize_layout;
```


老版本的记录:
1. register_filesystem => file_system_type::mount => 调用文件系统自定义 mount 函数，该函数只是 mount_nodev 的封装，作用是提供自己的 fill_super
2. fill_super 完成的基本任务 :


按照全新的版本 : fs_context_operations 即可
1. 参数解析
2. 上线

```c
static const struct fs_context_operations hugetlbfs_fs_context_ops = {
  .free   = hugetlbfs_fs_context_free,
  .parse_param  = hugetlbfs_parse_param,
  .get_tree = hugetlbfs_get_tree,
};

static int hugetlbfs_get_tree(struct fs_context *fc)
{
  int err = hugetlbfs_validate(fc);
  if (err)
    return err;
  return get_tree_nodev(fc, hugetlbfs_fill_super);
  // @todo 总结一下 get_tree_nodev 对于含有 dev 对称函数
}
```

新版本:
1. some important calling graph of mount :
```c
kern_mount
  vfs_kern_mount : 调用特定文件系统的回调函数，构建一个vfsmount
      fs_context_for_mount
        alloc_fs_context : fc->fs_type->init_fs_context or legacy_init_fs_context
  fc_mount
      vfs_get_tree
          fc->ops->get_tree : get_tree is init in the alloc_fs_context, legacy_init_fs_context is init get_tree with legacy_get_tree
              A : legacy_get_tree : fc->fs_type->mount
              B : get_tree_nodev : it seems that kernel always call get_tree_nodev eg. get_tree_nodev(fc, shmem_fill_super) :
                    vfs_get_super
                        sget_fc
                        fill_super
      vfs_create_mount : Note that this does not attach the mount to anything.
```

```c
do_mount
  do_new_mount
    fs_context_for_mount : init the context
    vfs_get_tree
    do_new_mount_fc
      vfs_create_mount
      do_add_mount : 将 vfsmount 结构放到全局目录树中间
        graft_tree
```


2. some important function :
    1. fs_context_for_mount : init context
    2. vfs_get_tree :  sget and fill_super
    3. vfs_create_mount : init vfsmount

从 do_mount 的代码可以它主要就是：
- 将 dir_name 解析为 path 格式到内核
- 一路解析 flags 位表，将 flags 拆分位 mnt_flags 和 sb_flags
- 根据 flags 中的标记，决定下面做哪一个 mount 操作。

do_add_mount 函数主要做两件事：
1. lock_mount 确定本次挂载要挂载到哪个父挂载实例 parent 的哪个挂载点 mp 上。
2. 把 newmnt 挂载到 parent 的 mp 下，完成 newmnt 到全局的安装。安装后的样子就像我们前文讲述的那样。

#### pivot_root
1. 为什么更改 root 的 mount 位置存在这种诡异的需求啊
    1. 找到使用这个的软件




## superblock
super 作为一个文件系统实例的信息管理中心，很多动态信息依赖于 superlock.
    1. sync，fs-writeback
    2. remove/create inode / file

mount 的时候，首先需要加载 superblock

问题 1 : kill super 到底是在做什么 ?
```c
void kill_anon_super(struct super_block *sb)
{
  dev_t dev = sb->s_dev;
  generic_shutdown_super(sb);
  free_anon_bdev(dev);
}

void kill_litter_super(struct super_block *sb)
{
  if (sb->s_root)
    d_genocide(sb->s_root);
  kill_anon_super(sb);
}

void kill_block_super(struct super_block *sb) // 提供基于block 的 fs, ext2 fat 之类的
{
  struct block_device *bdev = sb->s_bdev;
  fmode_t mode = sb->s_mode;

  bdev->bd_super = NULL;
  generic_shutdown_super(sb); // TODO
  sync_blockdev(bdev);
  WARN_ON_ONCE(!(mode & FMODE_EXCL));
  blkdev_put(bdev, mode | FMODE_EXCL);
}
```
- kill_block_super(), which unmounts a file system on a block device
- kill_anon_super(), which unmounts a virtual file system (information is generated when requested)
- kill_litter_super(), which unmounts a file system that is not on a physical device (the information is kept in memory)
An example for a file system without disk support is the ramfs_mount() function in the ramfs file system:
> kill_anon_super 和 kill_litter_super 使用标准是什么，基于内存的和 generated when needed 的区别还是很清晰的，
> 但是检查了一下 reference ，感觉存在疑惑 !
> 其次 TODO 这些 kill_super 函数 和 super_operation::put_super 的关系是什么 ?

问题 2 : 各种 mount 辅助函数，搞不清楚到底谁在使用新版本的接口
- mount_bdev(), which mounts a file system stored on a block device
- mount_single(), which mounts a file system that shares an instance between all mount operations
- mount_nodev(), which mounts a file system that is not on a physical device
- mount_pseudo(), a helper function for pseudo-file systems (sockfs, pipefs, generally file systems that can not be mounted)

#### super operations
> TODO 这些都是需要检查一下，总结一下的。
- alloc_inode: allocates an inode. Usually, this funcion allocates a struct <fsname>_inode_info structure and performs basic VFS inode initialization (using inode_init_once()); minix uses for allocation the kmem_cache_alloc() function that interacts with the SLAB subsystem. For each allocation, the cache construction is called, which in the case of minix is the init_once() function. Alternatively, kmalloc() can be used, in which case the inode_init_once() function should be called. The alloc_inode() function will be called by the new_inode() and iget_locked() functions.
- destroy_inode releases the memory occupied by inode
- write_inode : 对于一个 inode 的信息进行更新
- - evict_inode: removes any information about the inode with the number received in the i_ino field **from the disk and memory** (both the inode on the disk and the associated data blocks). This involves performing the following operations:

```c
static const struct super_operations myfs_sops = {
  .statfs = simple_statfs,
  .drop_inode = generic_delete_inode,
};
```

## mnt
在 fs/namespace.c 下面:

```txt
__do_sys_mount
__do_sys_umount
__do_sys_fsmount
__do_sys_oldumount
__do_sys_open_tree
__do_sys_move_mount
__do_sys_pivot_root
__do_sys_mount_setattr
```
open_tree 和 move_mount 连 man 都没有

[这里](https://unix.stackexchange.com/questions/464033/understanding-how-mount-namespaces-work-in-linux)
提供了一个很好的例子，首先 unshare mount namespace，创建一个新的 mount point, 使用 findmnt 来检查，可以看到新的 mnt 点, 但是在另一个 shell 中间就是没有该 mnt 的。
在 container 的情况下，为了修改一个程序内部观测到 root 是从我们设置好的位置，需要 pivot_root 那嫁接一下。

猜测 : 由于 mount 的空间不同，实际上，在同一个路径，一个可能进入到 mount，一个可能不会，这导致路径查找变得更加复杂了。


3. https://zhuanlan.zhihu.com/zorrolang 关于 mount 写了一系列的文章，水平应该不错。

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
