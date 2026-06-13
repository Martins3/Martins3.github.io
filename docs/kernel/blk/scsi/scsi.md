# scsi

参考 [SCSI Addressing](https://tldp.org/HOWTO/SCSI-2.4-HOWTO/scsiaddr.html) 分析了为什么 scsi 这这样寻址的

> <host, bus, target, lun>

> Each HBA may control one or more SCSI buses. The various types of SCSI buses are listed in Appendix A.↳


## 如何理解 ATA 和 scsi 的关系是什么?

- scsi_queue_rq
  - scsi_dispatch_cmd
    - host->hostt->queuecommand(host, cmd) :

scsi_host_template::queuecommand 的注册者，如果是 sata 总线，那么就是 ata_scsi_queuecmd

```c
static const struct scsi_host_template ahci_sht = {
	AHCI_SHT("ahci"),
};
```

从这里看，ata 类似 hba 卡的存在

## 一些参考

- [Linux SCSI 子系统剖析](https://zhuanlan.zhihu.com/p/62004722)
- [How to emulate block devices with QEMU](https://blogs.oracle.com/linux/post/how-to-emulate-block-devices-with-qemu)

Documentation/scsi/ 看上去存在很多文档，但是都是关于 hba 驱动的

Documentation/scsi/libsas.rst : 不明觉厉

Documentation/scsi/scsi.rst : 三个层次
1. upper : sd sr tape sg
  - config BLK_DEV_SD
  - config CHR_DEV_ST
  - config BLK_DEV_SR
  - config CHR_DEV_SG
2. middle : 核心
3. lower : Those individual cards are often called Host Bus Adapters (HBAs).

Documentation/scsi/scsi_mid_low_api.rst : 写给 hba 卡驱动开发者的

## scsi 的 middle layer 包含的那些内容

感觉主要是这几个文件吧:
drivers/scsi/hosts.c
drivers/scsi/scsi.c
drivers/scsi/scsi_lib.c
drivers/scsi/scsi_error.c

并不是物理意义上需要这个层次，
只是将公共的代码抽象到此处

### mlqueue
好滴，我知道是 middle level queue ，但是具体来说是做什么的?

似乎和错误处理的关系很大

### scsi.c
主要的几个函数:
1. scsi_change_queue_depth : 修改硬件队列的深度
2. scsi_device_get
3. scsi_device_lookup_by_target

### hosts.c
似乎是一些抽象的函数，用来处理 scsi_host 的
1. scsi_queue_work
2. scsi_host_busy

### scsi_lib.c

各种小的辅助函数

### scsi_error.c
这个问题单独在

## 如何理解 sg

- sg_add_device : 注册 sg_fops，这是为了将 scsi device 直接暴露为文件吗? 那么为什么不使用 block layer 的哇

```txt
#0  sg_add_device (cl_dev=0xffff88801af49490, cl_intf=0xffffffff82e250e0 <sg_interface>) at drivers/scsi/sg.c:1492
#1  0xffffffff81a68eaf in device_add (dev=0xffff88801af49490) at drivers/base/core.c:3541
#2  0xffffffff81acce4d in scsi_sysfs_add_sdev (sdev=sdev@entry=0xffff88801af49000) at drivers/scsi/scsi_sysfs.c:1393
#3  0xffffffff81ac9641 in scsi_add_lun (async=0, bflags=<synthetic pointer>, inq_result=0xffff888100e0ba00 "", sdev=0xffff88801af49000) at drivers/scsi/scsi_scan.c:1094
#4  scsi_probe_and_add_lun (starget=starget@entry=0xffff8880054f5c00, lun=lun@entry=0, bflagsp=bflagsp@entry=0xffffc9000005fb98, sdevp=<optimized out>, rescan=<optimized out>, hostdata=<optimized out>) at drivers/scsi/scsi_scan.c:1260
#5  0xffffffff81ac9f0b in __scsi_scan_target (parent=parent@entry=0xffff888100730258, channel=channel@entry=0, id=0, lun=lun@entry=18446744073709551615, rescan=rescan@entry=SCSI_SCAN_INITIAL) at drivers/scsi/scsi_scan.c:1665
#6  0xffffffff81aca513 in scsi_scan_channel (id=0, rescan=<optimized out>, lun=<optimized out>, channel=<optimized out>, shost=<optimized out>) at drivers/scsi/scsi_scan.c:1753
#7  scsi_scan_channel (shost=0xffff888100730000, channel=0, id=<optimized out>, lun=18446744073709551615, rescan=SCSI_SCAN_INITIAL) at drivers/scsi/scsi_scan.c:1729
#8  0xffffffff81aca63e in scsi_scan_host_selected (shost=0xffff888100730000, channel=0, channel@entry=4294967295, id=id@entry=4294967295, lun=lun@entry=18446744073709551615, rescan=rescan@entry=SCSI_SCAN_INITIAL) at drivers/scsi/scsi_scan.c:1782
#9  0xffffffff81aca710 in do_scsi_scan_host (shost=shost@entry=0xffff888100730000) at drivers/scsi/scsi_scan.c:1921
#10 0xffffffff81aca8a5 in scsi_scan_host (shost=0xffff888100730000) at drivers/scsi/scsi_scan.c:1951
#11 scsi_scan_host (shost=shost@entry=0xffff888100730000) at drivers/scsi/scsi_scan.c:1939
#12 0xffffffff81afa267 in sdebug_driver_probe (dev=0xffff88810017e820) at drivers/scsi/scsi_debug.c:7930
#13 0xffffffff81a6d9a1 in call_driver_probe (drv=0xffffffff82e25940 <sdebug_driverfs_driver>, dev=0xffff88810017e820) at drivers/base/dd.c:560
#14 really_probe (dev=dev@entry=0xffff88810017e820, drv=drv@entry=0xffffffff82e25940 <sdebug_driverfs_driver>) at drivers/base/dd.c:639
#15 0xffffffff81a6dbdd in __driver_probe_device (drv=drv@entry=0xffffffff82e25940 <sdebug_driverfs_driver>, dev=dev@entry=0xffff88810017e820) at drivers/base/dd.c:778
#16 0xffffffff81a6dc69 in driver_probe_device (drv=drv@entry=0xffffffff82e25940 <sdebug_driverfs_driver>, dev=dev@entry=0xffff88810017e820) at drivers/base/dd.c:808
#17 0xffffffff81a6e36e in __device_attach_driver (drv=0xffffffff82e25940 <sdebug_driverfs_driver>, _data=0xffffc9000005fd78) at drivers/base/dd.c:936
#18 0xffffffff81a6b62a in bus_for_each_drv (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffc9000005fd78, fn=fn@entry=0xffffffff81a6e2f0 <__device_attach_driver>) at drivers/base/bus.c:427
#19 0xffffffff81a6df67 in __device_attach (dev=dev@entry=0xffff88810017e820, allow_async=allow_async@entry=true) at drivers/base/dd.c:1008
#20 0xffffffff81a6e5be in device_initial_probe (dev=dev@entry=0xffff88810017e820) at drivers/base/dd.c:1057
#21 0xffffffff81a6cadd in bus_probe_device (dev=dev@entry=0xffff88810017e820) at drivers/base/bus.c:487
#22 0xffffffff81a68e10 in device_add (dev=dev@entry=0xffff88810017e820) at drivers/base/core.c:3517
#23 0xffffffff81a69386 in device_register (dev=dev@entry=0xffff88810017e820) at drivers/base/core.c:3598
#24 0xffffffff81afe7dd in sdebug_add_host_helper (per_host_idx=per_host_idx@entry=0) at drivers/scsi/scsi_debug.c:7325
#25 0xffffffff835e7669 in scsi_debug_init () at drivers/scsi/scsi_debug.c:7115
#26 0xffffffff81001940 in do_one_initcall (fn=0xffffffff835e7023 <scsi_debug_init>) at init/main.c:1306
#27 0xffffffff83596818 in do_initcall_level (command_line=0xffff888003c5cc00 "root", level=6) at init/main.c:1379
#28 do_initcalls () at init/main.c:1395
#29 do_basic_setup () at init/main.c:1414
#30 kernel_init_freeable () at init/main.c:1634
#31 0xffffffff8217c3d5 in kernel_init (unused=<optimized out>) at init/main.c:1522
#32 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

```c
#define SCSI_GENERIC_MAJOR	21
```

ls -la /dev | grep 21

```txt
crw-rw----.  1 root disk     21,   0 Dec 16 20:42 sg0
crw-rw----.  1 root disk     21,   1 Dec 16 20:42 sg1
crw-rw----.  1 root disk     21,   2 Dec 16 20:42 sg2
```

这个设备是和创建的 disk 对应的，每多个 /dev/sdx 就会增加一个这个。

具体的不知道如何使用，不过猜测就是给具体的 scsi 设备提供更加底层的接口，从而去操作特殊的设备:

- [kernel doc : SCSI Generic (sg) driver](https://www.kernel.org/doc/html/latest/scsi/scsi-generic.html)
- [SCSI-2.4-HOWTO : 9.4. Generic driver (sg)](https://tldp.org/HOWTO/SCSI-2.4-HOWTO/sg.html)

应该是可以通过 sg 直接操作 sd 设备的，只是更加底层。

当 echo a > /dev/sda 的时候，居然走这个路径:
```txt
#0  scsi_initialize_rq (rq=0xffff88810c338200) at drivers/scsi/scsi_lib.c:1125
#1  scsi_alloc_request (q=<optimized out>, opf=<optimized out>, flags=flags@entry=0) at drivers/scsi/scsi_lib.c:1140
#2  0xffffffff81a5165a in sg_io (sdev=sdev@entry=0xffff88810c2e8000, hdr=hdr@entry=0xffffc901f6dd3e28, open_for_write=open_for_write@entry=false)
    at drivers/scsi/scsi_ioctl.c:441
#3  0xffffffff81a52460 in scsi_ioctl_sg_io (argp=0x7ffcf4b74390, open_for_write=false, sdev=0xffff88810c2e8000) at drivers/scsi/scsi_ioctl.c:844
#4  scsi_ioctl (sdev=0xffff88810c2e8000, open_for_write=<optimized out>, cmd=8837, arg=0x7ffcf4b74390) at drivers/scsi/scsi_ioctl.c:899
#5  0xffffffff817f3623 in blkdev_ioctl (file=<optimized out>, cmd=8837, arg=140724414137232) at block/ioctl.c:630
#6  0xffffffff81472924 in vfs_ioctl (arg=140724414137232, cmd=<optimized out>, filp=0xffff888109e47f00) at fs/ioctl.c:51
#7  __do_sys_ioctl (arg=140724414137232, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:871
#8  __se_sys_ioctl (arg=140724414137232, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:857
#9  __x64_sys_ioctl (regs=<optimized out>) at fs/ioctl.c:857
#10 0xffffffff8221fa8b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc901f6dd3f58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc901f6dd3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#12 0xffffffff824000ea in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

打了一个点，最后发现是在: https://linux.die.net/man/8/scsi_id 中的。

因为是 echo a > /dev/sda 的时候，导致 udev 自动刷新一下之类的，如此使用 sg 也是很酷的。

## 基本概念

```txt
@[
    ata_scsi_queuecmd+1
    scsi_queue_rq+902
    blk_mq_dispatch_rq_list+539
    blk_mq_do_dispatch_sched+830
    __blk_mq_sched_dispatch_requests+240
    blk_mq_sched_dispatch_requests+53
    __blk_mq_run_hw_queue+53
    blk_mq_sched_insert_requests+106
    blk_mq_flush_plug_list+284
    __blk_flush_plug+258
    blk_finish_plug+37
    __iomap_dio_rw+1413
    iomap_dio_rw+14
    ext4_file_read_iter+145
    aio_read+236
    io_submit_one+546
    __x64_sys_io_submit+128
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 32
```

## hba 卡是 sas 协议的实现吗?

sas 和 scsi 的关系: https://zh.wikipedia.org/wiki/%E4%B8%B2%E5%88%97SCSI

SAS 是 Serial Attached SCSI 的缩写。

所以，简单来说，SAS 和 SCSI 关机紧密，而 sata 单独是一个。

## [ ] sata 如何理解

https://www.diffen.com/difference/SATA_vs_Serial_Attached_SCSI

ata 是存在一个单独的目录的 : drivers/ata/

同时存在这个文件
drivers/ata/libata-scsi.c

ata_scsi_queuecmd

```txt
@[
    ata_scsi_queuecmd+5
    scsi_queue_rq+908
    blk_mq_dispatch_rq_list+755
    __blk_mq_sched_dispatch_requests+1065
    blk_mq_sched_dispatch_requests+55
    blk_mq_run_work_fn+100
    process_one_work+479
    worker_thread+81
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 90278
```

```txt
#0  ata_scsi_queuecmd (shost=0xffff888105068000, cmd=0xffff88810e9170f8) at drivers/ata/libata-scsi.c:4214
#1  0xffffffff81a5ac20 in scsi_dispatch_cmd (cmd=0xffff88810e9170f8) at drivers/scsi/scsi_lib.c:1517
#2  scsi_queue_rq (hctx=<optimized out>, bd=<optimized out>) at drivers/scsi/scsi_lib.c:1759
#3  0xffffffff817ebf90 in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff88810cd4da00, list=list@entry=0xffffc900014e3df0, nr_budgets=0, nr_budgets@entry=1) at block/blk-mq.c:2049
#4  0xffffffff817f1ee7 in __blk_mq_do_dispatch_sched (hctx=0xffff88810cd4da00) at block/blk-mq-sched.c:170
#5  blk_mq_do_dispatch_sched (hctx=0xffff88810cd4da00) at block/blk-mq-sched.c:184
#6  __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88810cd4da00) at block/blk-mq-sched.c:309
#7  0xffffffff817f2127 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88810cd4da00) at block/blk-mq-sched.c:333
#8  0xffffffff817e5c11 in blk_mq_run_work_fn (work=0xffff88810cd4da40) at block/blk-mq.c:2434
#9  0xffffffff81165d76 in process_one_work (worker=worker@entry=0xffff8881062080c0, work=0xffff88810cd4da40) at kernel/workqueue.c:2630
#10 0xffffffff81166df5 in process_scheduled_works (worker=<optimized out>) at kernel/workqueue.c:2703
#11 worker_thread (__worker=0xffff8881062080c0) at kernel/workqueue.c:2784
#12 0xffffffff81170984 in kthread (_create=0xffff8881061cc000) at kernel/kthread.c:388
#13 0xffffffff810e3da1 in ret_from_fork (prev=<optimized out>, regs=0xffffc900014e3f58, fn=0xffffffff81170890 <kthread>, fn_arg=0xffff8881061cc000) at arch/x86/kernel/process.c:147
#14 0xffffffff8100259b in ret_from_fork_asm () at arch/x86/entry/entry_64.S:304
```

从 [How to emulate a SATA disk drive in QEMU](https://stackoverflow.com/questions/48351096/how-to-emulate-a-sata-disk-drive-in-qemu) 中看，
这个代码是 hw/ide/ahci.c 应该是实现 libata 的地方。

## sda 的名称是从哪里的来的?

- https://superuser.com/questions/558156/what-does-dev-sda-in-linux-mean

以前是 scsi, 但是后来扩展开来了。

virtio scsi, megaraid 以及 sata, usb 设备都会调用 sd_probe

```c
static struct scsi_driver sd_template = {
	.gendrv = {
		.name		= "sd",
		.owner		= THIS_MODULE,
		.probe		= sd_probe,
		.probe_type	= PROBE_PREFER_ASYNCHRONOUS,
		.remove		= sd_remove,
		.shutdown	= sd_shutdown,
		.pm		= &sd_pm_ops,
	},
	.rescan			= sd_rescan,
	.init_command		= sd_init_command,
	.uninit_command		= sd_uninit_command,
	.done			= sd_done,
	.eh_action		= sd_eh_action,
	.eh_reset		= sd_eh_reset,
};
```

## scsi 重要的数据结构

### scsi_device
```c
struct scsi_device {

	struct device		sdev_gendev,
				sdev_dev;
```

```c
static inline struct scsi_disk *scsi_disk(struct gendisk *disk)
{
	return disk->private_data;
}
```

那么 scsi_disk 和 scsi_device 的区别是什么?

scsi_disk : 是 higher level 的
scsi_device : 是 middle level 的


```c
struct scsi_disk {
	struct scsi_device *device;

	/*
	 * disk_dev is used to show attributes in /sys/class/scsi_disk/,
	 * but otherwise not really needed.  Do not use for refcounting.
	 */
	struct device	disk_dev;
	struct gendisk	*disk;
```

执行命令 : `ag "struct scsi_device \*device;"`
```txt
drivers/scsi/sg.c
163:    struct scsi_device *device;

drivers/scsi/st.h
120:    struct scsi_device *device;

drivers/scsi/sd.h
85:     struct scsi_device *device;

drivers/scsi/sr.h
35:     struct scsi_device *device;
```

## scsi transport 如何理解?

srp : scsi rdma
- https://docs.nvidia.com/networking/display/mlnxofedv590590/srp+-+scsi+rdma+protocol
- https://en.wikipedia.org/wiki/SCSI_RDMA_Protocol

```txt
CONFIG_SCSI_SPI_ATTRS=y --> Parallel SCSI (SPI) Transport Attributes
# CONFIG_SCSI_FC_ATTRS is not set --> FiberChannel Transport Attributes
CONFIG_SCSI_ISCSI_ATTRS=y
CONFIG_SCSI_SAS_ATTRS=y  --> SAS Transport Attributes : smartpqi 和 mpt3sas 都是需要依赖
# CONFIG_SCSI_SAS_LIBSAS is not set
# CONFIG_SCSI_SRP_ATTRS is not set --> scsi RDMA
```

## drivers/scsi/scsi_debugfs.c

scsi_show_rq

对应 /sys/kernel/debug/block/sda/hctx0/dispatch 中的内容

## scsi-hd 的对应的驱动的作用

drivers/scsi/sd.c 中，将会将 block device 的操作最后转化为 virtio-scsi 的命令，
命令中已经指定了操作的哪一个盘。


## 有趣的 backtrace

将 mq-deadline 的输出修改为:
```txt
@[
    sd_init_command+5
    scsi_queue_rq+2193
    blk_mq_dispatch_rq_list+755
    __blk_mq_sched_dispatch_requests+1065
    blk_mq_sched_dispatch_requests+55
    blk_mq_run_hw_queue+360
    blk_mq_flush_plug_list.part.0+435
    blk_add_rq_to_plug+167
    blk_mq_submit_bio+1230
    submit_bio_noacct_nocheck+653
    ext4_bio_write_folio+338
    mpage_submit_folio+100
    mpage_map_and_submit_buffers+343
    ext4_do_writepages+1949
    ext4_writepages+173
    do_writepages+207
    __writeback_single_inode+61
    writeback_sb_inodes+501
    __writeback_inodes_wb+76
    wb_writeback+664
    wb_workfn+686
    process_one_work+479
    worker_thread+81
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 4744
```

非常有趣的，命令是来自于 blk-mq 的，但是如果将 queue 修改为 none ，那还是有办法知道是谁创建的
因为 backtrace 是这个样子的:
```txt
@[
    sd_init_command+5
    scsi_queue_rq+2193
    __blk_mq_issue_directly+72
    blk_mq_plug_issue_direct+107
    blk_mq_flush_plug_list.part.0+1431
    __blk_flush_plug+245
    blk_finish_plug+41
    __iomap_dio_rw+1206
    iomap_dio_rw+18
    ext4_file_read_iter+210
    io_read+233
    io_issue_sqe+96
    io_submit_sqes+490
    __do_sys_io_uring_enter+1489
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 2700
```

## 这个文件是做什么的 ？
drivers/scsi/libsas/sas_expander.c

## 基本命令
- TUR
- STU


## scsi 的基本的中断的过程

```c
static __latent_entropy void blk_done_softirq(struct softirq_action *h)
{
	blk_complete_reqs(this_cpu_ptr(&blk_cpu_done));
}
```

硬中断的位置:
- virtscsi_req_done
  - virtscsi_complete_cmd
    - scsi_done
      - scsi_done_internal

```c
	if (complete_directly)
		blk_mq_complete_request_direct(req, scsi_complete); // 直接在硬中断的上下文中操作
	else
		blk_mq_complete_request(req); // 发起为软中断
```

scsi 的软中断开始:
- scsi_complete : 正经的开始处理事情

## 初始化 scsi_cmnd 的时机
```txt
#0  scsi_initialize_rq (rq=<optimized out>) at drivers/scsi/scsi_lib.c:1163
#1  scsi_init_command (dev=dev@entry=0xffff88810c2e8000, cmd=cmd@entry=0xffff88810bd6f7f8) at drivers/scsi/scsi_lib.c:1164
#2  0xffffffff81a5af42 in scsi_prepare_cmd (req=0xffff88810bd6f700) at drivers/scsi/scsi_lib.c:1554
#3  scsi_queue_rq (hctx=<optimized out>, bd=0xffffc90000067c38) at drivers/scsi/scsi_lib.c:1745
#4  0xffffffff817e7cc8 in __blk_mq_issue_directly (hctx=0xffff88810c2e8000, rq=rq@entry=0xffff88810bd6f700, last=224) at block/blk-mq.c:2591
#5  0xffffffff817eae38 in blk_mq_request_issue_directly (rq=rq@entry=0xffff88810bd6f700, last=last@entry=true) at block/blk-mq.c:2677
#6  0xffffffff817eaed5 in blk_mq_plug_issue_direct (plug=0xffffc90000067d70) at block/blk-mq.c:2698
#7  0xffffffff817eb5e4 in blk_mq_flush_plug_list (plug=plug@entry=0xffffc90000067d70, from_schedule=from_schedule@entry=false) at block/blk-mq.c:2812
#8  0xffffffff817ec489 in blk_mq_flush_plug_list (plug=plug@entry=0xffffc90000067d70, from_schedule=from_schedule@entry=false) at block/blk-mq.c:2784
#9  0xffffffff817dd1c6 in __blk_flush_plug (plug=0xffffc90000067d70, plug@entry=0xffffc90000067d00, from_schedule=from_schedule@entry=false) at block/blk-core.c:1142
#10 0xffffffff817dd419 in blk_finish_plug (plug=0xffffc90000067d00) at block/blk-core.c:1166
#11 blk_finish_plug (plug=plug@entry=0xffffc90000067d70) at block/blk-core.c:1163
#12 0xffffffff8149a677 in wb_writeback (wb=wb@entry=0xffff88830b777400, work=work@entry=0xffffc90000067e08) at fs/fs-writeback.c:2113
#13 0xffffffff8149ba98 in wb_check_background_flush (wb=0xffff88830b777400) at fs/fs-writeback.c:2147
#14 wb_do_writeback (wb=0xffff88830b777400) at fs/fs-writeback.c:2235
#15 wb_workfn (work=0xffff88830b777588) at fs/fs-writeback.c:2262
```

## megaraid
- megasas_reset_bus_host
  - megasas_generic_reset
    - megasas_wait_for_outstanding

https://www.yuanguohuo.com/2019/09/06/megaraid/

## hba 卡驱动
- https://techtellectual.com/sas-expander-vs-hba/

A SAS HBA has multiple SAS ports that connect to hard disks, SSDs, SAS Expanders, JBODs, backplanes, etc., via the SAS protocol.

关于 JBOD : https://serverfault.com/questions/137380/is-raid-0-or-jbod-better-for-home-media-server
- JBOD 就是把多个 disk 放到一起的组成一个连续的，而 raid0 是将 disk sector 交错放置的

## 这些 tracepoint 就是 s

```txt
./tracing/events/scsi/scsi_dispatch_cmd_start
./tracing/events/scsi/scsi_dispatch_cmd_error
./tracing/events/scsi/scsi_dispatch_cmd_done
./tracing/events/scsi/scsi_dispatch_cmd_timeout
./tracing/events/scsi/scsi_eh_wakeup
```

## 中断返回
- virtscsi_req_done
  - virtscsi_vq_done
    - scsi_complete
      - scsi_decide_disposition && scsi_cmd_runtime_exceeced : 中断返回，命令可以不成功，可以尝试

## LBP
- https://www.jianshu.com/p/5ddb7e2af714
- https://gist.github.com/cathay4t/e80e02a737242a5f3824606543631bfe

可以知道一个 scsi 设备的 logical block 和物理 block 是否是映射的

## scsi sg

```txt
🧀  fio --enghelp
Available IO engines:
        sg
```
sg engine 是 fio 提供的一个 I/O 引擎，它允许 fio 绕过标准块设备接口，直接通过 /dev/sgX 向 SCSI 设备发送原始 SCSI 命令。

那么 scsi 按道理也该可以实现 iouring cmd 吧

## qemu 模拟 megaraid

qemu 中参考
https://blogs.oracle.com/linux/post/how-to-emulate-block-devices-with-qemu
```txt
arg_megasas="  -device megasas,id=scsi1 "
arg_megasas+=" -device scsi-hd,drive=drive0,bus=scsi1.0,channel=0,scsi-id=0,lun=0 "
arg_megasas+=" -drive file=${workstation}/img9,if=none,id=drive0 "
arg_megasas+=" -device scsi-hd,drive=drive1,bus=scsi1.0,channel=0,scsi-id=1,lun=0 "
arg_megasas+=" -drive file=${workstation}/img10,if=none,id=drive1 "
```
之前 qemu 启动会有这个问题，不过现在依旧修复了:
```txt
[   56.611215] critical target error, dev sda, sector 0 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[   56.612442] sd 0:2:0:0: [sda] Unit Not Ready
[   56.612658] sd 0:2:0:0: [sda] Sense Key : Hardware Error [current]
[   56.612963] sd 0:2:0:0: [sda] Add. Sense: Internal target failure
[   56.613371] sd 0:2:0:0: [sda] tag#786 FAILED Result: hostbyte=DID_OK driverbyte=DRIVER_OK cmd_age=0s
[   56.613800] sd 0:2:0:0: [sda] tag#786 Sense Key : Illegal Request [current]
[   56.614135] sd 0:2:0:0: [sda] tag#786 Add. Sense: Invalid command operation code
[   56.614481] sd 0:2:0:0: [sda] tag#786 CDB: Read(6) 08 00 00 00 08 00
[   56.614778] critical target error, dev sda, sector 0 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[   56.615202] buffer_io_error: 5 callbacks suppressed
[   56.615431] Buffer I/O error on dev sda, logical block 0, async page read
[   56.615766] sd 0:2:0:0: [sda] tag#787 FAILED Result: hostbyte=DID_OK driverbyte=DRIVER_OK cmd_age=0s
[   56.616191] sd 0:2:0:0: [sda] tag#787 Sense Key : Illegal Request [current]
[   56.616519] sd 0:2:0:0: [sda] tag#787 Add. Sense: Invalid command operation code
[   56.616860] sd 0:2:0:0: [sda] tag#787 CDB: Read(6) 08 00 00 00 08 00
[   56.617155] critical target error, dev sda, sector 0 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[   56.617574] Buffer I/O error on dev sda, logical block 0, async page read
[   56.617893]  sda: unable to read partition table
[   56.625891] sd 0:2:0:0: [sda] tag#199 FAILED Result: hostbyte=DID_OK driverbyte=DRIVER_OK cmd_age=0s
[   56.626318] sd 0:2:0:0: [sda] tag#199 Sense Key : Illegal Request [current]
[   56.626643] sd 0:2:0:0: [sda] tag#199 Add. Sense: Invalid command operation code
[   56.626984] sd 0:2:0:0: [sda] tag#199 CDB: Read(10) 28 00 0c 7f ff 80 00 00 08 00
[   56.627333] critical target error, dev sda, sector 209715072 op 0x0:(READ) flags 0x80700 phys_seg 1 prio class 2
[   56.627837] sd 0:2:0:0: [sda] tag#200 FAILED Result: hostbyte=DID_OK driverbyte=DRIVER_OK cmd_age=0s
[   56.628261] sd 0:2:0:0: [sda] tag#200 Sense Key : Illegal Request [current]
[   56.628588] sd 0:2:0:0: [sda] tag#200 Add. Sense: Invalid command operation code
[   56.628933] sd 0:2:0:0: [sda] tag#200 CDB: Read(10) 28 00 0c 7f ff 80 00 00 08 00
[   56.629285] critical target error, dev sda, sector 209715072 op 0x0:(READ) flags 0x0 phys_seg 1 prio class 2
[   56.629742] Buffer I/O error on dev sda, logical block 26214384, async page read
```

## 盘异常导致 iso 无法安装
iso 无法安装，bmc 错误为:
```txt
[    0.275190] Spectre V2 : WARNING: Unprivileged eBPF is enabled with eIBRS on,
                 data leaks possible via Spectre V2 BHB attacks!
[   10.752788] dracut-pre-udev[925]: modprobe: FATAL: Module floppy not found.
[   10.791564] dracut-pre-udev[925]: modprobe: ERROR: could not insert 'edac': No
                 such device
[   11.395200] dracut-pre-udev[925]: modprobe: FATAL: Module i2c_piix4 not found.
[   11.934411] ACPI Error: No handler for Region [USR1] (00000000c3faf618) [IPMI
                 class]
[   11.934441] ACPI Error: Region IPMI (ID=7) has no handler (20221020/exfldio-2
                 814)
[   11.934693] ACPI Error: Method parse/execution failed \_SB.PMI0._GHL, AE_NOT_
                 EXIST (20221020/psparse-529)
[   11.934891] ACPI Error: Method parse/execution failed \_SB.PMI0._PMC, AE_NOT_
                 EXIST (20221020/psparse-529)
[   11.935144] ACPI Error: AE_NOT_EXIST, Evaluating _PMC (20221020/power_meter-7
                 010)
[  OK  ] Started Show Plymouth Boot Screen.
[  OK  ] Started Device-Mapper Multipath Device Controller.
          Starting Open-iSCSI...
[  OK  ] Started Forward Password Requests to Plymouth Directory Watch.
[  OK  ] Reached target Paths.
[  OK  ] Reached target Basic System.
[  OK  ] Started Open-iSCSI.
          Starting dracut initqueue hook...
[   12.518438] mei 0000:00:16.0-55213584-9a29-4916-badf-0fb7ed68a2eb: Could not
                 read FW version
[   12.518444] mei 0000:00:16.0-55213584-9a29-4916-badf-0fb7ed68a2eb: FW version
                 command failed -5
[  114.239921] sd 0:0:5:0: [sdg] Asking for cache data failed
[  114.239982] sd 0:0:5:0: [sdg] Assuming drive cache: write through
```

使用 fedora 42 ，可以正常安装，看来 fedora 可以容忍这种错误，
进入到系统之后:

可以观察到的:
```txt
root@bogon:~# dmesg | grep sdg
[    5.675683] sd 17:0:5:0: [sdg] Spinning up disk...
[  105.750674] sd 17:0:5:0: [sdg] Read Capacity(16) failed: Result: hostbyte=DID_OK driverbyte=DRIVER_OK
[  105.750688] sd 17:0:5:0: [sdg] Sense Key : Not Ready [current]
[  105.750697] sd 17:0:5:0: [sdg] Add. Sense: Logical unit is in process of becoming ready
[  105.791109] sd 17:0:5:0: [sdg] Read Capacity(10) failed: Result: hostbyte=DID_OK driverbyte=DRIVER_OK
[  105.791123] sd 17:0:5:0: [sdg] Sense Key : Not Ready [current]
[  105.791130] sd 17:0:5:0: [sdg] Add. Sense: Logical unit is in process of becoming ready
[  105.791141] sd 17:0:5:0: [sdg] 0 512-byte logical blocks: (0 B/0 B)
[  105.791147] sd 17:0:5:0: [sdg] 0-byte physical blocks
[  105.821452] sd 17:0:5:0: [sdg] Test WP failed, assume Write Enabled
[  105.831534] sd 17:0:5:0: [sdg] Asking for cache data failed
[  105.831546] sd 17:0:5:0: [sdg] Assuming drive cache: write through
[  105.873740] sd 17:0:5:0: [sdg] Attached SCSI disk
```

而正常的盘为:
```txt
root@bogon:~# dmesg | grep sdf
[    5.639453] sd 17:0:4:0: [sdf] 4688430768 512-byte logical blocks: (2.40 TB/2.18 TiB)
[    5.639466] sd 17:0:4:0: [sdf] 4096-byte physical blocks
[    5.641292] sd 17:0:4:0: [sdf] Write Protect is off
[    5.641304] sd 17:0:4:0: [sdf] Mode Sense: d3 00 10 08
[    5.644660] sd 17:0:4:0: [sdf] Write cache: enabled, read cache: enabled, supports DPO and FUA
[    5.653197] sd 17:0:4:0: [sdf] Enabling DIF Type 2 protection
[    5.713439]  sdf: sdf1
[    5.713717] sd 17:0:4:0: [sdf] Attached SCSI disk
```

## scsi 的基本结构

可以从 make menuconfig 中窥见一二:

```txt
-> Device Drivers
  -> SCSI device support
    -> SCSI Transports (这里有多个传输协议)
      -> FiberChannel Transport Attributes (SCSI_FC_ATTRS [=m])
```

- [ ] CONFIG_RAID_ATTRS 是什么?

FC SAN 的整体结构
```txt
+-----------+        +-----------+        +-------------+
| Server A  |        | FC Switch |        | Storage     |
| (HBA)     |--------|  Fabric   |--------| Array       |
+-----------+        +-----------+        +-------------+
      |                    |                    |
      | Initiator          | Fabric             | Target
```

```txt
FC-0  物理层（光纤 / 电信号）
FC-1  编码（64b/66b）
FC-2  帧、流控、交换（核心）
FC-3  公共服务（很少用）
FC-4  上层协议映射（FCP / NVMe-FC）
```

可以类比成：
FC HBA ≈ “只会跑存储协议的专用 NIC”
FC Fabric ≈ “只服务于存储流量的专用交换网络

那么 sata 到底是在哪里的?

## hyper-v 虚拟机中观察到

是不是有这个日志
```txt
[ 3299.492206] hv_storvsc 565b810e-cd29-44f1-8135-81d820149f29: tag#1228 cmd 0xa1 status: scsi 0x2 srb 0x86 host 0xc0000001
```

不过 smartd 是没有运行的，不知道是什么导致的:
```txt
🧀  systemctl status smartd            
○ smartd.service - Self Monitoring and Reporting Technology (SMART) Daemon
     Loaded: loaded (/usr/lib/systemd/system/smartd.service; enabled; preset: enabled)
    Drop-In: /usr/lib/systemd/system/service.d
             └─10-timeout-abort.conf
     Active: inactive (dead)
  Condition: start condition unmet at Thu 2026-02-12 07:47:33 CST; 4h 31min ago
       Docs: man:smartd(8)
             man:smartd.conf(5)

```

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
