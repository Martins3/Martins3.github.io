# cgroup

| cgroup interface | source     code |
|------------------|-----------------|
| io.latency       | blk-iolatency.c |
| io.max           | blk-throttle.c  |
| io.prio.class    | blk-ioprio.c    |
| io.stat          | blk-stat.c      |
| io.weight        | blk-iocost.c    |

## 基本使用
为什么这样执行之后:
- duck echo "8:16  wiops=100000" > io.max
- cgexec -g io:duck dd if=/dev/zero of=/dev/sdb bs=1M count=1000

结果 kernel 卡到了这里啊:
```txt
➜  cat /proc/1624/stack
[<0>] blkdev_get_by_dev.part.0+0xf7/0x2f0
[<0>] blkdev_open+0x4b/0x90
[<0>] do_dentry_open+0x157/0x430
[<0>] path_openat+0xb6b/0x1030
[<0>] do_filp_open+0xb1/0x160
[<0>] do_sys_openat2+0x95/0x160
[<0>] __x64_sys_openat+0x52/0xa0
[<0>] do_syscall_64+0x3c/0x90
[<0>] entry_SYSCALL_64_after_hwframe+0x72/0xdc
```

具体的分析应该是很有趣的，但是没时间了。

## 参考
- http://127.0.0.1:3434/admin-guide/cgroup-v2.html
- https://facebookmicrosites.github.io/cgroup2/docs/io-controller.html

## iolatency

- iolatency_set_min_lat_nsec

## throttle

```txt
#0  blk_cgroup_bio_start (bio=bio@entry=0xffff8881374a7800) at block/blk-cgroup.c:1992
#1  0xffffffff816c0aad in submit_bio_noacct (bio=0xffff8881374a7800) at block/blk-core.c:794
#2  0xffffffff81412289 in submit_bh_wbc (opf=<optimized out>, bh=<optimized out>, wbc=<optimized out>) at fs/buffer.c:2702
#3  0xffffffff81412b8a in submit_bh (bh=0xffff88809e886000, opf=0) at fs/buffer.c:2707
#4  __bh_read (bh=bh@entry=0xffff88809e886000, op_flags=op_flags@entry=0, wait=wait@entry=false) at fs/buffer.c:2972
#5  0xffffffff81415c68 in bh_read_nowait (op_flags=0, bh=0xffff88809e886000) at ./include/linux/buffer_head.h:427
#6  __block_write_begin_int (folio=<optimized out>, pos=pos@entry=624168960, len=len@entry=3, get_block=get_block@entry=0xffffffff816b7f10 <blkdev_get_block>, iomap=iomap@entry=0x0 <fixed_percpu_data>) at fs/buffer.c:2021
#7  0xffffffff81415d41 in __block_write_begin (get_block=0xffffffff816b7f10 <blkdev_get_block>, len=3, pos=624168960, page=0xffffea0002715f80) at fs/buffer.c:2041
#8  block_write_begin (mapping=<optimized out>, pos=624168960, len=3, pagep=0xffffc900402efd88, get_block=0xffffffff816b7f10 <blkdev_get_block>) at fs/buffer.c:2102
#9  0xffffffff812de041 in generic_perform_write (iocb=<optimized out>, i=0xffffc900402efe78) at mm/filemap.c:3772
#10 0xffffffff812e317f in __generic_file_write_iter (iocb=iocb@entry=0xffffc900402efea0, from=from@entry=0xffffc900402efe78) at mm/filemap.c:3900
#11 0xffffffff816b8483 in blkdev_write_iter (iocb=0xffffc900402efea0, from=0xffffc900402efe78) at block/fops.c:534
#12 0xffffffff813c60bd in call_write_iter (iter=0x1000 <cpu_debug_store>, kio=0xffff8881374a7800, file=0xffff888023f85f00) at ./include/linux/fs.h:2186
#13 new_sync_write (ppos=0xffffc900402eff08, len=3, buf=0x40200d "str", filp=0xffff888023f85f00) at fs/read_write.c:491
#14 vfs_write (file=file@entry=0xffff888023f85f00, buf=buf@entry=0x40200d "str", count=count@entry=3, pos=pos@entry=0xffffc900402eff08) at fs/read_write.c:584
#15 0xffffffff813c650e in ksys_write (fd=<optimized out>, buf=0x40200d "str", count=3) at fs/read_write.c:637
#16 0xffffffff8217cbbc in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900402eff58) at arch/x86/entry/common.c:50
#17 do_syscall_64 (regs=0xffffc900402eff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#18 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## io.stat

```txt
#0  blkcg_iostat_update (blkg=blkg@entry=0xffff8880201d3400, cur=cur@entry=0xffffc900402efaa0, last=0xffffe8ff01c60cb8) at block/blk-cgroup.c:847
#1  0xffffffff816dff61 in blkcg_rstat_flush (css=<optimized out>, cpu=<optimized out>) at block/blk-cgroup.c:894
#2  0xffffffff81205f2f in cgroup_rstat_flush_locked (cgrp=cgrp@entry=0xffffffff82d536f0 <cgrp_dfl_root+16>, may_sleep=may_sleep@entry=false) at kernel/cgroup/rstat.c:205
#3  0xffffffff81206073 in cgroup_rstat_flush_irqsafe (cgrp=0xffffffff82d536f0 <cgrp_dfl_root+16>) at kernel/cgroup/rstat.c:254
#4  0xffffffff81399696 in __mem_cgroup_flush_stats () at mm/memcontrol.c:636
#5  0xffffffff813a2da8 in mem_cgroup_flush_stats () at mm/memcontrol.c:644
#6  mem_cgroup_wb_stats (wb=wb@entry=0xffff888138329000, pfilepages=pfilepages@entry=0xffffc900402efc10, pheadroom=pheadroom@entry=0xffffc900402efc18, pdirty=pdirty@entry=0xffffc900402efcb0, pwriteback=pwriteback@entry=0xffffc900402efc20) at mm/memcontrol.c:4583
#7  0xffffffff812ea500 in balance_dirty_pages (wb=wb@entry=0xffff888138329000, pages_dirtied=32, flags=flags@entry=0) at mm/page-writeback.c:1724
#8  0xffffffff812eb2ed in balance_dirty_pages_ratelimited_flags (mapping=mapping@entry=0xffff888140806508, flags=flags@entry=0) at ./arch/x86/include/asm/current.h:41
#9  0xffffffff812eb40b in balance_dirty_pages_ratelimited (mapping=mapping@entry=0xffff888140806508) at mm/page-writeback.c:2064
#10 0xffffffff812de0c9 in generic_perform_write (iocb=<optimized out>, i=0xffffc900402efe78) at mm/filemap.c:3806
#11 0xffffffff812e317f in __generic_file_write_iter (iocb=iocb@entry=0xffffc900402efea0, from=from@entry=0xffffc900402efe78) at mm/filemap.c:3900
#12 0xffffffff816b8483 in blkdev_write_iter (iocb=0xffffc900402efea0, from=0xffffc900402efe78) at block/fops.c:534
#13 0xffffffff813c60bd in call_write_iter (iter=0xffffc900402efaa0, kio=0xffff8880201d3400, file=0xffff888023f85f00) at ./include/linux/fs.h:2186
#14 new_sync_write (ppos=0xffffc900402eff08, len=3, buf=0x40200d "str", filp=0xffff888023f85f00) at fs/read_write.c:491
#15 vfs_write (file=file@entry=0xffff888023f85f00, buf=buf@entry=0x40200d "str", count=count@entry=3, pos=pos@entry=0xffffc900402eff08) at fs/read_write.c:584
#16 0xffffffff813c650e in ksys_write (fd=<optimized out>, buf=0x40200d "str", count=3) at fs/read_write.c:637
#17 0xffffffff8217cbbc in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900402eff58) at arch/x86/entry/common.c:50
#18 do_syscall_64 (regs=0xffffc900402eff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#19 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

## 似乎存在一个通用框架

```c
enum rq_qos_id {
	RQ_QOS_WBT,
	RQ_QOS_LATENCY,
	RQ_QOS_COST,
};

static struct rq_qos_ops wbt_rqos_ops = {
	.throttle = wbt_wait,
	.issue = wbt_issue,
	.track = wbt_track,
	.requeue = wbt_requeue,
	.done = wbt_done,
	.cleanup = wbt_cleanup,
	.queue_depth_changed = wbt_queue_depth_changed,
	.exit = wbt_exit,
#ifdef CONFIG_BLK_DEBUG_FS
	.debugfs_attrs = wbt_debugfs_attrs,
#endif
};

static struct rq_qos_ops blkcg_iolatency_ops = {
	.throttle = blkcg_iolatency_throttle,
	.done_bio = blkcg_iolatency_done_bio,
	.exit = blkcg_iolatency_exit,
};

static struct rq_qos_ops ioc_rqos_ops = {
	.throttle = ioc_rqos_throttle,
	.merge = ioc_rqos_merge,
	.done_bio = ioc_rqos_done_bio,
	.done = ioc_rqos_done,
	.queue_depth_changed = ioc_rqos_queue_depth_changed,
	.exit = ioc_rqos_exit,
};
```

当 disk 出现错误，为什么最后是在 wbt_wait 上:

## 选项 CONFIG_BLK_DEV_THROTTLING 是做什么的
- 2000 行?

## qos 的注册内容

两个的注册位置是不同的:
```txt
#0  rq_qos_add (rqos=<optimized out>, q=<optimized out>) at block/blk-rq-qos.h:99
#1  wbt_init (q=q@entry=0xffff8881008f9ef0) at block/blk-wbt.c:871
#2  0xffffffff816f8eef in wbt_enable_default (q=q@entry=0xffff8881008f9ef0) at block/blk-wbt.c:673
#3  0xffffffff816c3e03 in blk_register_queue (disk=disk@entry=0xffff888103981400) at block/blk-sysfs.c:820
#4  0xffffffff816d9692 in device_add_disk (parent=parent@entry=0xffff8881008e3010, disk=0xffff888103981400, groups=groups@entry=0xffffffff82e1b8d0 <virtblk_attr_groups>) at block/genhd.c:485
#5  0xffffffff81aa3ba1 in virtblk_probe (vdev=0xffff8881008e3000) at drivers/block/virtio_blk.c:1150
#6  0xffffffff8180ae6e in virtio_dev_probe (_d=0xffff8881008e3010) at drivers/virtio/virtio.c:305
#7  0xffffffff81a76484 in call_driver_probe (drv=0xffffffff82e1b760 <virtio_blk>, dev=0xffff8881008e3010) at drivers/base/dd.c:560
#8  really_probe (dev=dev@entry=0xffff8881008e3010, drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:639
#9  0xffffffff81a766bd in __driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff8881008e3010) at drivers/base/dd.c:778
#10 0xffffffff81a76749 in driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff8881008e3010) at drivers/base/dd.c:808
#11 0xffffffff81a769c5 in __driver_attach (data=0xffffffff82e1b760 <virtio_blk>, dev=0xffff8881008e3010) at drivers/base/dd.c:1194
#12 __driver_attach (dev=0xffff8881008e3010, data=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1134
#13 0xffffffff81a73fc7 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82e1b760 <virtio_blk>, fn=fn@entry=0xffffffff81a76940 <__driver_attach>) at drivers/base/bus.c:301
#14 0xffffffff81a75e79 in driver_attach (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1211
#15 0xffffffff81a75810 in bus_add_driver (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/bus.c:618
#16 0xffffffff81a77c2e in driver_register (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/driver.c:246
#17 0xffffffff8180a58b in register_virtio_driver (driver=driver@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/virtio/virtio.c:357
#18 0xffffffff835eb0fb in virtio_blk_init () at drivers/block/virtio_blk.c:1284
#19 0xffffffff81001943 in do_one_initcall (fn=0xffffffff835eb0aa <virtio_blk_init>) at init/main.c:1306
#20 0xffffffff8359b818 in do_initcall_level (command_line=0xffff888100129b40 "root", level=6) at init/main.c:1379
#21 do_initcalls () at init/main.c:1395
#22 do_basic_setup () at init/main.c:1414
#23 kernel_init_freeable () at init/main.c:1634
#24 0xffffffff82186485 in kernel_init (unused=<optimized out>) at init/main.c:1522
#25 0xffffffff8100265c in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

- `__alloc_disk_node`
  - blkcg_init_disk
    - blk_ioprio_init
    - blk_throtl_init
    - blk_iolatency_init


## block layer queue depth 和 nr_requests
- https://www.ibm.com/docs/en/linux-on-systems?topic=wsd-setting-queue-depth

cat /sys/block/sd<X>/queue/nr_requests

```c
struct request_queue {

	unsigned int		queue_depth;

	unsigned long		nr_requests;	/* Max # of requests */
```

```txt
#0  wbt_queue_depth_changed (rqos=0xffff888102204858) at block/blk-wbt.c:703
#1  0xffffffff816dd69c in __rq_qos_queue_depth_changed (rqos=0xffff888102204858) at block/blk-rq-qos.c:102
#2  0xffffffff816c5095 in rq_qos_queue_depth_changed (q=<optimized out>) at block/blk-rq-qos.h:230
#3  0xffffffff81ac4ff9 in scsi_change_queue_depth (sdev=0xffff888100e93000, depth=<optimized out>) at drivers/scsi/scsi.c:227
#4  0xffffffff81ad4b51 in sdev_store_queue_depth (dev=0xffff888100e931b8, attr=<optimized out>, buf=<optimized out>, count=4) at drivers/scsi/scsi_sysfs.c:1036
#5  0xffffffff8146c957 in kernfs_fop_write_iter (iocb=0xffffc9003f72bea0, iter=<optimized out>) at fs/kernfs/file.c:334
#6  0xffffffff813c70c0 in call_write_iter (iter=0xffffc9003f72be78, kio=0xffffc9003f72bea0, file=0xffff8881485de400) at ./include/linux/fs.h:2186
#7  new_sync_write (ppos=0xffffc9003f72bf08, len=4, buf=0x55de5904c8c0 "100\nho\033\\s/0:0:0:0\033\\.15\nm0.32\033[0m\non Dec 19 02:52:33 PM CST 2022\n\n", filp=0xffff8881485de400) at fs/read_write.c:491
#8  vfs_write (file=file@entry=0xffff8881485de400, buf=buf@entry=0x55de5904c8c0 "100\nho\033\\s/0:0:0:0\033\\.15\nm0.32\033[0m\non Dec 19 02:52:33 PM CST 2022\n\n", count=count@entry=4, pos=pos@entry=0xffffc9003f72bf08) at fs/read_write.c:584
#9  0xffffffff813c750e in ksys_write (fd=<optimized out>, buf=0x55de5904c8c0 "100\nho\033\\s/0:0:0:0\033\\.15\nm0.32\033[0m\non Dec 19 02:52:33 PM CST 2022\n\n", count=4) at fs/read_write.c:637
#10 0xffffffff82180c3f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9003f72bf58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc9003f72bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#12 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```


两者的关系非常的微妙啊！
```c
static inline unsigned int blk_queue_depth(struct request_queue *q)
{
	if (q->queue_depth)
		return q->queue_depth;

	return q->nr_requests;
}
```

对于一些老设备，queue_depth 比 nr_requests 更加科学的。

```diff
History:        #0
Commit:         d278d4a8892f13b6a9eb6102b356402f0e062324
Author:         Jens Axboe <axboe@fb.com>
Author Date:    Thu 31 Mar 2016 12:21:08 AM CST
Committer Date: Sun 06 Nov 2016 07:09:53 AM CST

block: add code to track actual device queue depth

For blk-mq, ->nr_requests does track queue depth, at least at init
time. But for the older queue paths, it's simply a soft setting.
On top of that, it's generally larger than the hardware setting
on purpose, to allow backup of requests for merging.

Fill a hole in struct request with a 'queue_depth' member, that
drivers can call to more closely inform the block layer of the
real queue depth.

Signed-off-by: Jens Axboe <axboe@fb.com>
Reviewed-by: Jan Kara <jack@suse.cz>
```

想不到，nvme 的 nr_requests 更加大:
```txt
➜  ~ cat /sys/block/nvme0n1/queue/nr_requests
1024
➜  ~ cat /sys/block/sda/queue/nr_requests
256
```

## nr_requests 是描述 queue 的深度

- blk_mq_update_nr_requests 让用户态来控制的 nr_requests

? 什么是描述 queue 中实际上包含了多少个 request

## 如果 queue 满了，会如何？
