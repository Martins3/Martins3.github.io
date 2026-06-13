## 分析下 disk events

- disk_flush_events
- __disk_unblock_events

```txt
#0  disk_flush_events (disk=disk@entry=0xffff888103703000, mask=mask@entry=1) at block/disk-events.c:152
#1  0xffffffff818d8ee6 in blkdev_put (bdev=0xffff8881088c8640, holder=<optimized out>) at block/bdev.c:937
#2  0xffffffff818d9029 in bdev_release (handle=0xffff8881036fe140) at block/bdev.c:952
#3  0xffffffff818da145 in blkdev_release (inode=<optimized out>, filp=<optimized out>) at block/fops.c:616
#4  0xffffffff81460eff in __fput (file=0xffff888101e8f300) at fs/file_table.c:394
#5  0xffffffff814611ba in __fput_sync (file=<optimized out>) at fs/file_table.c:475
#6  0xffffffff8145878d in __do_sys_close (fd=<optimized out>) at fs/open.c:1590
#7  __se_sys_close (fd=<optimized out>) at fs/open.c:1575
#8  __x64_sys_close (regs=<optimized out>) at fs/open.c:1575
#9  0xffffffff82399be3 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001adbf58) at arch/x86/entry/common.c:51
#10 do_syscall_64 (regs=0xffffc90001adbf58, nr=<optimized out>) at arch/x86/entry/common.c:82
#11 0xffffffff824000eb in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

谁可以注册上: disk events
```c
int disk_alloc_events(struct gendisk *disk)
{
	struct disk_events *ev;

	if (!disk->fops->check_events || !disk->events){
		return 0;
	}

	ev = kzalloc(sizeof(*ev), GFP_KERNEL);
	if (!ev) {
		pr_warn("%s: failed to initialize events\n", disk->disk_name);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&ev->node);
	ev->disk = disk;
	spin_lock_init(&ev->lock);
	mutex_init(&ev->block_mutex);
	ev->block = 1;
	ev->poll_msecs = -1;
	INIT_DELAYED_WORK(&ev->dwork, disk_events_workfn);

	disk->ev = ev;
	return 0;
}
```

```txt
[    0.460105] [martins3:disk_alloc_events:440] fd0
[    1.543530] [martins3:disk_alloc_events:440] md127
```

大致的流程为:
```txt
[    1.536932]  dump_stack_lvl+0x64/0x80
[    1.537885]  disk_alloc_events+0x36/0x120
[    1.538118]  device_add_disk+0xfb/0x3d0
[    1.538259]  md_alloc+0x191/0x580
[    1.538458]  md_probe+0x29/0x70
[    1.538574]  blk_request_module+0x63/0xc0
[    1.538756]  blkdev_get_no_open+0x60/0xa0
[    1.538928]  blkdev_get_by_dev.part.0+0x21/0x2e0
[    1.539179]  bdev_open_by_dev+0x9c/0xc0
[    1.539318]  ? __pfx_blkdev_open+0x10/0x10
[    1.539464]  blkdev_open+0x3b/0xa0
[    1.539588]  do_dentry_open+0x202/0x550
[    1.539738]  path_openat+0xcfa/0x11f0
[    1.539872]  ? __kmem_cache_alloc_node+0x1a8/0x2b0
[    1.540030]  ? __xa_alloc+0xc5/0x150
[    1.540162]  do_filp_open+0xb3/0x160
[    1.540293]  do_sys_openat2+0xab/0xe0
[    1.540428]  __x64_sys_openat+0x6e/0xa0
[    1.540623]  do_syscall_64+0x43/0xf0
[    1.540822]  entry_SYSCALL_64_after_hwframe+0x6f/0x77
```

```c
	if (sdp->removable) {
		gd->flags |= GENHD_FL_REMOVABLE;
		gd->events |= DISK_EVENT_MEDIA_CHANGE;
		gd->event_flags = DISK_EVENT_FLAG_POLL | DISK_EVENT_FLAG_UEVENT;
	}
```

```c
/**
 * DOC: genhd capability flags
 *
 * ``GENHD_FL_REMOVABLE``: indicates that the block device gives access to
 * removable media.  When set, the device remains present even when media is not
 * inserted.  Shall not be set for devices which are removed entirely when the
 * media is removed.
 *
 * ``GENHD_FL_HIDDEN``: the block device is hidden; it doesn't produce events,
 * doesn't appear in sysfs, and can't be opened from userspace or using
 * blkdev_get*. Used for the underlying components of multipath devices.
 *
 * ``GENHD_FL_NO_PART``: partition support is disabled.  The kernel will not
 * scan for partitions from add_disk, and users can't add partitions manually.
 *
 */
enum {
	GENHD_FL_REMOVABLE			= 1 << 0,
	GENHD_FL_HIDDEN				= 1 << 1,
	GENHD_FL_NO_PART			= 1 << 2,
};

enum {
	DISK_EVENT_MEDIA_CHANGE			= 1 << 0, /* media changed */
	DISK_EVENT_EJECT_REQUEST		= 1 << 1, /* eject requested */
};

enum {
	/* Poll even if events_poll_msecs is unset */
	DISK_EVENT_FLAG_POLL			= 1 << 0,
	/* Forward events to udev */
	DISK_EVENT_FLAG_UEVENT			= 1 << 1,
	/* Block event polling when open for exclusive write */
	DISK_EVENT_FLAG_BLOCK_ON_EXCL_WRITE	= 1 << 2,
};
```

是我理解错了，GENHD_FL_REMOVABLE 其实是给 CD-ROM 这种工具用的，这种还有一个托盘的。

[ ] 但是 raid 为什么需要 disk-events ，有趣的问题


## exclusive open 的含义


```txt
[    1.588872] CPU: 0 PID: 373 Comm: lvm Not tainted 6.10.0 #116
[    1.589245] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[    1.589679] Call Trace:
[    1.589870]  <TASK>
[    1.590036]  dump_stack_lvl+0x3f/0xb0
[    1.590295]  bdev_open+0x3b4/0x3d0
[    1.590541]  bdev_file_open_by_dev+0x12f/0x160
[    1.590836]  dm_get_table_device+0xc0/0x1c0
[    1.591124]  dm_get_device+0x1a9/0x290
[    1.591383]  linear_ctr+0x9a/0x120
[    1.591621]  dm_table_add_target+0x200/0x3c0
[    1.591907]  table_load+0x13e/0x3e0
[    1.592151]  dm_ctl_ioctl+0x3af/0x4d0
[    1.592405]  ? __pfx_table_load+0x10/0x10
[    1.592676]  __se_sys_ioctl+0x6e/0xc0
[    1.592928]  do_syscall_64+0xe3/0x200
[    1.593184]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
```

问题背景:
```txt
config BLK_DEV_WRITE_MOUNTED
	bool "Allow writing to mounted block devices"
	default y
	help
	When a block device is mounted, writing to its buffer cache is very
	likely going to cause filesystem corruption. It is also rather easy to
	crash the kernel in this way since the filesystem has no practical way
	of detecting these writes to buffer cache and verifying its metadata
	integrity. However there are some setups that need this capability
	like running fsck on read-only mounted root device, modifying some
	features on mounted ext4 filesystem, and similar. If you say N, the
	kernel will prevent processes from writing to block devices that are
	mounted by filesystems which provides some more protection from runaway
	privileged processes and generally makes it much harder to crash
	filesystem drivers. Note however that this does not prevent
	underlying device(s) from being modified by other means, e.g. by
	directly submitting SCSI commands or through access to lower layers of
	storage stack. If in doubt, say Y. The configuration can be overridden
	with the bdev_allow_write_mounted boot option.
```

## raid 的 sync 也有这个问题
为什么做 sync 的时候，下面的盘的 inflight 这么小, 几乎总是 0

即使是错误注入之后，也就是最大的才可以观察到
```txt
Every 2.0s: cat /sys/block/vda/inflight /sys/block/vdb/inflight                                 bogon: Fri May 10 17:15:31 2024
       4        0
       0        0
```
这说明，提交 io 的间隙很大。

## 我们发现关闭 iostat 基本不影响性能

如果把这些关闭，那么 iops 可以提升多少性能
```c
static inline bool blk_do_io_stat(struct request *rq)
{
	return (rq->rq_flags & RQF_IO_STAT) && !blk_rq_is_passthrough(rq);
}
```
看看这个会影响那些内容

iostat 到底提供了什么东西?

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
