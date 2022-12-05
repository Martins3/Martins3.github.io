# scsi

- [ ] 看看内核日志
- [ ] 如何没有理解错误，那么 scsi 应该是在 nvme 差不多的层次，也就是在 multiqueue 的下面
- [ ] scsi disk / scsi cd-rom / scsi - 磁带 / scsi - generic
- [ ] QEMU 中的 -cdrom 是如何实现的
- [ ] scsi 是不是连接在 PCIe 上的
- [ ] 理解一下 virtio_scsi 是如何在内核和 QEMU 两侧沟通的。
  - https://github.com/spdk/spdk/issues/162 :  vhost-user-scsi-pci 是做什么的?
  - [ ] what's scsi : https://www.linux-kvm.org/images/archive/f/f5/20110823142849!2011-forum-virtio-scsi.pdf
  - https://mpolednik.github.io/2017/01/23/virtio-blk-vs-virtio-scsi/
  - https://stackoverflow.com/questions/39031456/why-is-virtio-scsi-much-slower-than-virtio-blk-in-my-experiment-over-and-ceph-r
- [ ] 如何理解 ATA 和 scsi 的关系是什么?
- 使用 QEMU 模拟 scsi 的: https://blogs.oracle.com/linux/post/how-to-emulate-block-devices-with-qemu
- megasas 是啥?

-device vhost-user-scsi-pci vs -device virtio-scsi-pci

启动的时候必然遇到这个:
```txt
#0  scsi_block_when_processing_errors (sdev=sdev@entry=0xffff888100aac000) at drivers/scsi/scsi_error.c:380
#1  0xffffffff819dbfa8 in sd_open (bdev=0xffff888222460000, mode=1) at drivers/scsi/sd.c:1326
#2  0xffffffff81618f5d in blkdev_get_whole (bdev=0xffff888100aac000, bdev@entry=0xffff888222460000, mode=mode@entry=1) at block/bdev.c:672
#3  0xffffffff81619cf6 in blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>) at block/bdev.c:822
#4  0xffffffff81619f0c in blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>) at block/bdev.c:856
#5  0xffffffff81637050 in disk_scan_partitions (disk=disk@entry=0xffff888221e68200, mode=mode@entry=1) at block/genhd.c:371
#6  0xffffffff816373d2 in device_add_disk (parent=parent@entry=0xffff888100aac1b8, disk=disk@entry=0xffff888221e68200, groups=groups@entry=0x0 <fixed_percpu_data>) at block/genhd.c:502
#7  0xffffffff819dc3c5 in sd_probe (dev=0xffff888100aac1b8) at drivers/scsi/sd.c:3528
#8  0xffffffff8197d3f0 in call_driver_probe (drv=0xffffffff82c29100 <sd_template>, dev=0xffff888100aac1b8) at drivers/base/dd.c:560
#9  really_probe (dev=dev@entry=0xffff888100aac1b8, drv=drv@entry=0xffffffff82c29100 <sd_template>) at drivers/base/dd.c:639
#10 0xffffffff8197d49d in __driver_probe_device (drv=0xffffffff82c29100 <sd_template>, dev=dev@entry=0xffff888100aac1b8) at drivers/base/dd.c:778
#11 0xffffffff8197d519 in driver_probe_device (drv=<optimized out>, dev=dev@entry=0xffff888100aac1b8) at drivers/base/dd.c:808
#12 0xffffffff8197d62a in __driver_attach_async_helper (_dev=0xffff888100aac1b8, cookie=<optimized out>) at drivers/base/dd.c:1126
#13 0xffffffff811390e8 in async_run_entry_fn (work=0xffff888221f232c0) at kernel/async.c:127
#14 0xffffffff8112bd34 in process_one_work (worker=worker@entry=0xffff8882001249c0, work=0xffff888221f232c0) at kernel/workqueue.c:2289
#15 0xffffffff8112bf48 in worker_thread (__worker=0xffff8882001249c0) at kernel/workqueue.c:2436
#16 0xffffffff81133850 in kthread (_create=0xffff888200165040) at kernel/kthread.c:376
#17 0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
#18 0x0000000000000000 in ?? ()


> [  OK  ] Listening on Device-mapper event daemon FIFOs.
>         Starting Monitoring of LVM2 mirrors…ng dmeventd or progress polling...

#0  scsi_block_when_processing_errors (sdev=sdev@entry=0xffff888100aac000) at drivers/scsi/scsi_error.c:380
#1  0xffffffff819dbfa8 in sd_open (bdev=0xffff888222460000, mode=1207959581) at drivers/scsi/sd.c:1326
#2  0xffffffff81618f5d in blkdev_get_whole (bdev=0xffff888100aac000, bdev@entry=0xffff888222460000, mode=mode@entry=1207959581) at block/bdev.c:672
#3  0xffffffff81619cf6 in blkdev_get_by_dev (dev=<optimized out>, mode=1207959581, holder=holder@entry=0xffff888122f45200) at block/bdev.c:822
#4  0xffffffff81619f0c in blkdev_get_by_dev (dev=<optimized out>, mode=<optimized out>, holder=holder@entry=0xffff888122f45200) at block/bdev.c:856
#5  0xffffffff8161a687 in blkdev_open (inode=<optimized out>, filp=0xffff888122f45200) at block/fops.c:485
#6  0xffffffff813609d7 in do_dentry_open (f=f@entry=0xffff888122f45200, inode=0xffff888100b1b090, open=0xffffffff8161a640 <blkdev_open>, open@entry=0x0 <fixed_percpu_data>) at fs/open.c:882
#7  0xffffffff81362529 in vfs_open (path=path@entry=0xffffc900016cfdc0, file=file@entry=0xffff888122f45200) at fs/open.c:1013
#8  0xffffffff81377978 in do_open (op=0xffffc900016cfedc, file=0xffff888122f45200, nd=0xffffc900016cfdc0) at fs/namei.c:3557
#9  path_openat (nd=nd@entry=0xffffc900016cfdc0, op=op@entry=0xffffc900016cfedc, flags=flags@entry=65) at fs/namei.c:3713
#10 0xffffffff81378d2d in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff888100bc7000, op=op@entry=0xffffc900016cfedc) at fs/namei.c:3740
#11 0xffffffff813627e5 in do_sys_openat2 (dfd=-100, filename=<optimized out>, how=how@entry=0xffffc900016cff18) at fs/open.c:1310
#12 0xffffffff81362c4e in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1326
#13 __do_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1342
#14 __se_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1337
#15 __x64_sys_openat (regs=<optimized out>) at fs/open.c:1337
#16 0xffffffff81fa1888 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900016cff58) at arch/x86/entry/common.c:50
#17 do_syscall_64 (regs=0xffffc900016cff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#18 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120

#0  scsi_block_when_processing_errors (sdev=sdev@entry=0xffff888100aac000) at drivers/scsi/scsi_error.c:380
#1  0xffffffff819c2c54 in scsi_ioctl_block_when_processing_errors (cmd=21297, ndelay=false, sdev=0xffff888100aac000) at drivers/scsi/scsi_ioctl.c:951
#2  scsi_ioctl_block_when_processing_errors (sdev=sdev@entry=0xffff888100aac000, cmd=cmd@entry=21297, ndelay=ndelay@entry=false) at drivers/scsi/scsi_ioctl.c:944
#3  0xffffffff819d8167 in sd_ioctl (bdev=<optimized out>, mode=1212813341, cmd=21297, arg=0) at drivers/scsi/sd.c:1457
#4  0xffffffff8163576e in blkdev_ioctl (file=<optimized out>, cmd=21297, arg=0) at block/ioctl.c:614
#5  0xffffffff8137d172 in vfs_ioctl (arg=0, cmd=<optimized out>, filp=0xffff888122f45200) at fs/ioctl.c:51
#6  __do_sys_ioctl (arg=0, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:870
#7  __se_sys_ioctl (arg=0, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:856
#8  __x64_sys_ioctl (regs=<optimized out>) at fs/ioctl.c:856
#9  0xffffffff81fa1888 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900016cff58) at arch/x86/entry/common.c:50
#10 do_syscall_64 (regs=0xffffc900016cff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#11 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#12 0x00007f0d0de3d308 in ?? ()
#13 0x0000000000000000 in ?? ()
```

## scsi_block_when_processing_errors
- scsi_error_handler
  - scsi_restart_operations :star:

- sr_block_ioctl
- sd_ioctl
- sg_ioctl_common
- sg_ioctl
  - scsi_ioctl
    - scsi_ioctl_reset :star:


## scsi 的经典 backtrace

### 中断

### 读写

## 主要的文件

| file| content|
| sg.c|

### sg.c
- sg_add_device : 注册 sg_fops，这是为了将 scsi device 直接暴露为文件吗? 那么为什么不使用 block layer 的哇


## 参考资料

### [Linux SCSI 子系统剖析](https://zhuanlan.zhihu.com/p/62004722)

## 代码分析

### scsi 命令

### sg.c 和 scsi_mq_ops 沟通在一起的
为什么在
```c
static const struct blk_mq_ops scsi_mq_ops = {
	.get_budget	= scsi_mq_get_budget,
	.put_budget	= scsi_mq_put_budget,
	.queue_rq	= scsi_queue_rq,
	.commit_rqs	= scsi_commit_rqs,
	.complete	= scsi_complete,
	.timeout	= scsi_timeout,
#ifdef CONFIG_BLK_DEBUG_FS
	.show_rq	= scsi_show_rq,
#endif
	.init_request	= scsi_mq_init_request,
	.exit_request	= scsi_mq_exit_request,
	.cleanup_rq	= scsi_cleanup_rq,
	.busy		= scsi_mq_lld_busy,
	.map_queues	= scsi_map_queues,
	.init_hctx	= scsi_init_hctx,
	.poll		= scsi_mq_poll,
	.set_rq_budget_token = scsi_mq_set_rq_budget_token,
	.get_rq_budget_token = scsi_mq_get_rq_budget_token,
};
```

## scsi
```txt
/sys/bus/scsi🔒
😀  tree
.
├── devices
│   ├── 0:0:0:0 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host0/target0:0:0/0:0:0:0
│   ├── host0 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host0
│   ├── host1 -> ../../../devices/pci0000:00/0000:00:01.1/ata2/host1
│   └── target0:0:0 -> ../../../devices/pci0000:00/0000:00:01.1/ata1/host0/target0:0:0
├── drivers
│   └── sr
│       ├── 0:0:0:0 -> ../../../../devices/pci0000:00/0000:00:01.1/ata1/host0/target0:0:0/0:0:0:0
│       ├── bind
│       ├── module -> ../../../../module/sr_mod
│       ├── uevent
│       └── unbind
├── drivers_autoprobe
├── drivers_probe
└── uevent

9 directories, 6 files
/sys/bus/scsi🔒
```

```txt
[root@localhost scsi]# tree
.
├── devices
│   ├── 0:0:0:0 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0/target0:0:0/0:0:0:0
│   ├── 24:0:0:0 -> ../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24/target24:0:0/24:0:0:0
│   ├── 49:0:0:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:1/end_device-49:1/target49:0:0/49:0:0:0
│   ├── 49:0:1:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:2/end_device-49:2/target49:0:1/49:0:1:0
│   ├── 49:0:2:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:3/end_device-49:3/target49:0:2/49:0:2:0
│   ├── 49:0:3:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:4/end_device-49:4/target49:0:3/49:0:3:0
│   ├── 49:0:4:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:5/end_device-49:5/target49:0:4/49:0:4:0
│   ├── 49:2:0:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/target49:2:0/49:2:0:0
│   ├── host0 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0
│   ├── host1 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata2/host1
│   ├── host10 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata11/host10
│   ├── host11 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata12/host11
│   ├── host12 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata13/host12
│   ├── host13 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata14/host13
│   ├── host14 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata15/host14
│   ├── host15 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata16/host15
│   ├── host16 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata17/host16
│   ├── host17 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata18/host17
│   ├── host18 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata19/host18
│   ├── host19 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata20/host19
│   ├── host2 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata3/host2
│   ├── host20 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata21/host20
│   ├── host21 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata22/host21
│   ├── host22 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata23/host22
│   ├── host23 -> ../../../devices/pci0000:00/0000:00:01.7/0000:05:00.0/ata24/host23
│   ├── host24 -> ../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24
│   ├── host25 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata25/host25
│   ├── host26 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata26/host26
│   ├── host27 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata27/host27
│   ├── host28 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata28/host28
│   ├── host29 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata29/host29
│   ├── host3 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata4/host3
│   ├── host30 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata30/host30
│   ├── host31 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata31/host31
│   ├── host32 -> ../../../devices/pci0000:10/0000:10:08.1/0000:14:00.2/ata32/host32
│   ├── host33 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata33/host33
│   ├── host34 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata34/host34
│   ├── host35 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata35/host35
│   ├── host36 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata36/host36
│   ├── host37 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata37/host37
│   ├── host38 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata38/host38
│   ├── host39 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata39/host39
│   ├── host4 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata5/host4
│   ├── host40 -> ../../../devices/pci0000:40/0000:40:08.1/0000:42:00.2/ata40/host40
│   ├── host41 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata41/host41
│   ├── host42 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata42/host42
│   ├── host43 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata43/host43
│   ├── host44 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata44/host44
│   ├── host45 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata45/host45
│   ├── host46 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata46/host46
│   ├── host47 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata47/host47
│   ├── host48 -> ../../../devices/pci0000:50/0000:50:08.1/0000:52:00.2/ata48/host48
│   ├── host49 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49
│   ├── host5 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata6/host5
│   ├── host6 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata7/host6
│   ├── host7 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata8/host7
│   ├── host8 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata9/host8
│   ├── host9 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata10/host9
│   ├── target0:0:0 -> ../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0/target0:0:0
│   ├── target24:0:0 -> ../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24/target24:0:0
│   ├── target49:0:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:1/end_device-49:1/target49:0:0
│   ├── target49:0:1 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:2/end_device-49:2/target49:0:1
│   ├── target49:0:2 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:3/end_device-49:3/target49:0:2
│   ├── target49:0:3 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:4/end_device-49:4/target49:0:3
│   ├── target49:0:4 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:5/end_device-49:5/target49:0:4
│   └── target49:2:0 -> ../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/target49:2:0
├── drivers
│   ├── sd
│   │   ├── 0:0:0:0 -> ../../../../devices/pci0000:00/0000:00:01.6/0000:04:00.0/ata1/host0/target0:0:0/0:0:0:0
│   │   ├── 49:0:0:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:1/end_device-49:1/target49:0:0/49:0:0:0
│   │   ├── 49:0:1:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:2/end_device-49:2/target49:0:1/49:0:1:0
│   │   ├── 49:0:2:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:3/end_device-49:3/target49:0:2/49:0:2:0
│   │   ├── 49:0:3:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:4/end_device-49:4/target49:0:3/49:0:3:0
│   │   ├── bind
│   │   ├── module -> ../../../../module/sd_mod
│   │   ├── uevent
│   │   └── unbind
│   ├── ses
│   │   ├── 49:0:4:0 -> ../../../../devices/pci0000:20/0000:20:03.2/0000:22:00.0/host49/scsi_host/host49/port-49:5/end_device-49:5/target49:0:4/49:0:4:0
│   │   ├── bind
│   │   ├── module -> ../../../../module/ses
│   │   ├── uevent
│   │   └── unbind
│   └── sr
│       ├── 24:0:0:0 -> ../../../../devices/pci0000:10/0000:10:07.1/0000:13:00.3/usb3/3-2/3-2.2/3-2.2.3/3-2.2.3:1.0/host24/target24:0:0/24:0:0:0
│       ├── bind
│       ├── module -> ../../../../module/sr_mod
│       ├── uevent
│       └── unbind
├── drivers_autoprobe
├── drivers_probe
└── uevent
```

## cat /proc/scsi/scsi
1. QEMu
```c
Attached devices:
Host: scsi0 Channel: 00 Id: 00 Lun: 00
  Vendor: QEMU     Model: QEMU DVD-ROM     Rev: 2.5+
  Type:   CD-ROM                           ANSI  SCSI revision: 05
```


2. mi
nothing

3. surplus
```txt
[root@localhost ~]# cat /proc/scsi/scsi
Attached devices:
Host: scsi0 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: INTEL SSDSC2KB24 Rev: 0132
  Type:   Direct-Access                    ANSI  SCSI revision: 05
Host: scsi49 Channel: 00 Id: 00 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 01 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 02 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 03 Lun: 00
  Vendor: ATA      Model: HGST HUH721212AL Rev: T6B0
  Type:   Direct-Access                    ANSI  SCSI revision: 06
Host: scsi49 Channel: 00 Id: 04 Lun: 00
  Vendor: INSPUR   Model: Smart Adapter    Rev: 3.53
  Type:   Enclosure                        ANSI  SCSI revision: 05
Host: scsi49 Channel: 02 Id: 00 Lun: 00
  Vendor: INSPUR   Model: 8222-SHBA        Rev: 3.53
  Type:   RAID                             ANSI  SCSI revision: 05
Host: scsi24 Channel: 00 Id: 00 Lun: 00
  Vendor: Byosoft  Model: Virtual CDROM    Rev: 0502
  Type:   CD-ROM                           ANSI  SCSI revision: 02
```

## lsscsi

1. QEMU

[nix-shell:/sys/bus/scsi]$ lsscsi
[0:0:0:0]    cd/dvd  QEMU     QEMU DVD-ROM     2.5+  /dev/sr0

2. mi

[N:0:0:1] disk ZHITAI TiPlus5000 512GB__1 /dev/nvme0n1
[N:1:2:1] disk SAMSUNG MZVLW256HEHP-00000__1 /dev/nvme1n1

3. surplus

```txt
[root@localhost ~]# lsscsi
[0:0:0:0]    disk    ATA      INTEL SSDSC2KB24 0132  /dev/sda
[24:0:0:0]   cd/dvd  Byosoft  Virtual CDROM    0502  /dev/sr0
[49:0:0:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sdb
[49:0:1:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sdc
[49:0:2:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sdd
[49:0:3:0]   disk    ATA      HGST HUH721212AL T6B0  /dev/sde
[49:0:4:0]   enclosu INSPUR   Smart Adapter    3.53  -
[49:2:0:0]   storage INSPUR   8222-SHBA        3.53  -
[N:0:0:1]    disk    INTEL SSDPE2KE016T8__1                     /dev/nvme0n1
[N:1:0:1]    disk    INTEL SSDPE2KE016T8__1                     /dev/nvme1n1
```
