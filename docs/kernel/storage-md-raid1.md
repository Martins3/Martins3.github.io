# raid1 使用尝试

sudo mdadm --create --verbose /dev/md0 --level=1 --raid-devices=2 /dev/sdb /dev/sdc

```txt
➜  ~ mdadm --detail /dev/md0
/dev/md0:
           Version : 1.2
     Creation Time : Thu Apr 13 14:53:57 2023
        Raid Level : raid1
     Used Dev Size : 1022976 (999.00 MiB 1047.53 MB)
      Raid Devices : 2
     Total Devices : 2
       Persistence : Superblock is persistent

       Update Time : Thu Apr 13 14:53:57 2023
             State : active, FAILED, Not Started
    Active Devices : 2
   Working Devices : 2
    Failed Devices : 0
     Spare Devices : 0

Consistency Policy : unknown

              Name : localhost.localdomain:0  (local to host localhost.localdomain)
              UUID : 9455c367:d88c7a01:3a034e4e:011dfe96
            Events : 0

    Number   Major   Minor   RaidDevice State
       -       0        0        0      removed
       -       0        0        1      removed

       -       8       32        1      sync   /dev/sdc
       -       8        0        0      sync   /dev/sda
```

- [ ] 似乎是可以设置 Consistency Policy 的

似乎建立的 md0 是存在问题的:
```txt
➜  ~ mkfs.ext4 /dev/md0
mke2fs 1.46.4 (18-Aug-2021)
mkfs.ext4: Device size reported to be zero.  Invalid partition specified, or
        partition table wasn't reread after running fdisk, due to
        a modified partition being busy and in use.  You may need to reboot
        to re-read your partition table.
```

https://unix.stackexchange.com/questions/332061/remove-drive-from-soft-raid

## 删除一个设备
mdadm /dev/md0 --fail /dev/sdc1
mdadm /dev/md0 --remove /dev/sdc1

## 解散
mdadm -S /dev/md0

## 创建
mdadm --create /dev/md0 --name=martins3 --level=mirror --force --raid-devices="/dev/sda1 /dev/sdb1"
mdadm --create --verbose /dev/md0 --level=1 --raid-devices=2 /dev/sda1 /dev/sdc1

- [ ] 一个 partition 已经组成了 raid 想不到还是可以直接写，这样是处于什么考虑

## 增加一个设备
mdadm --add /dev/md0 /dev/sdd1
或者
mdadm --manage /dev/md0 --add /dev/vda
mdadm --grow --raid-devices=3 /dev/md0
mdadm --wait /dev/md0

```txt
➜  ~ cat /proc/mdstat
Personalities : [raid1]
md0 : active raid1 sdd1[3](S) sdc1[2] sda1[0]
      1020928 blocks super 1.2 [2/2] [UU]

unused devices: <none>
```
## 一些 backtrace

## 删除

mdadm /dev/md0 --fail /dev/sdc1

这一个命令会船舰这两个:
```txt
#0  raid1_error (mddev=0xffff888107dcc000, rdev=0xffff888105c3d400) at drivers/md/raid1.c:1660
#1  0xffffffff81e1cee9 in md_error (mddev=mddev@entry=0xffff888107dcc000, rdev=0xffff888105c3d400) at drivers/md/md.c:7975
#2  0xffffffff81e26889 in md_error (rdev=<optimized out>, mddev=0xffff888107dcc000) at drivers/md/md.c:7970
#3  set_disk_faulty (dev=8388609, mddev=0xffff888107dcc000) at drivers/md/md.c:7427
#4  md_ioctl (bdev=0xffff888100490600, mode=<optimized out>, cmd=2345, arg=<optimized out>) at drivers/md/md.c:7569
#5  0xffffffff81754ed7 in blkdev_ioctl (file=<optimized out>, cmd=2345, arg=2049) at block/ioctl.c:615
#6  0xffffffff8143d13b in vfs_ioctl (arg=2049, cmd=<optimized out>, filp=0xffff888106c4f000) at fs/ioctl.c:51
#7  __do_sys_ioctl (arg=2049, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:870
#8  __se_sys_ioctl (arg=2049, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:856
#9  __x64_sys_ioctl (regs=<optimized out>) at fs/ioctl.c:856
#10 0xffffffff82286f9c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900015abf58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc900015abf58, nr=<optimized out>) at arch/x86/entry/common.c:80
```

```txt
#0  raid1_remove_disk (mddev=0xffff888107dcc000, rdev=0xffff888105c3d400) at drivers/md/raid1.c:1844
#1  0xffffffff81e1d8c7 in remove_and_add_spares (mddev=mddev@entry=0xffff888107dcc000, this=this@entry=0x0 <fixed_percpu_data>) at drivers/md/md.c:9163
#2  0xffffffff81e25839 in md_check_recovery (mddev=mddev@entry=0xffff888107dcc000) at drivers/md/md.c:9408
#3  0xffffffff81e11dde in raid1d (thread=<optimized out>) at drivers/md/raid1.c:2564
#4  0xffffffff81e1a062 in md_thread (arg=0xffff888105d98c60) at drivers/md/md.c:7903
#5  0xffffffff8116ea09 in kthread (_create=0xffff888105b40e00) at kernel/kthread.c:376
#6  0xffffffff810029b9 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

mdadm /dev/md0 --remove /dev/sdc1

## 文档的记录

1. 活动 : resync, recovery, or reshape activity
  - resync ?

```txt
       --backup-file=
              This  is  needed  when  --grow  is  used  to increase the number of raid-devices in a RAID5
              or RAID6 if there are no spare devices available, or to shrink, change RAID level or layout.
              See the GROW MODE section below on RAID-DEVICES CHANGES.  The file must be stored on a separate device, not on the RAID array being reshaped.
```

## 分析

```c
static struct md_personality raid1_personality =
{
	.name		= "raid1",
	.level		= 1,
	.owner		= THIS_MODULE,
	.make_request	= raid1_make_request,
	.run		= raid1_run,
	.free		= raid1_free,
	.status		= raid1_status,
	.error_handler	= raid1_error,
	.hot_add_disk	= raid1_add_disk,
	.hot_remove_disk= raid1_remove_disk,
	.spare_active	= raid1_spare_active,
	.sync_request	= raid1_sync_request,
	.resize		= raid1_resize,
	.size		= raid1_size,
	.check_reshape	= raid1_reshape,
	.quiesce	= raid1_quiesce,
	.takeover	= raid1_takeover,
};
```
好像也没什么可以看的。

`raid1_make_request` 中注册了 `raid1_end_write_request`

## 分析下

## 如何理解其中的 barrier
- raid1_reshape
  - freeze_array(conf, 1 ) :
  - 更新 conf->raid_disks = mddev->raid_disks = raid_disks;
  - unfreeze_array(conf);

- wait_read_barrier
- wait_barrier

- 机制加入的时间 : fd76863e37fef26fe05547fddfa6e3d05e1682e6

思考一个很牛逼的场景

## 第一个 freeze_array 还没执行完成，结果第二个开始执行了？
- [ ] freeze_array 到底是阻碍别人，还是别别人阻碍？

## 提交一个 bio，还没返回的时候，首先将一个 devices 删除，然后新增一个新的 devices



## 基本的数据结构
```c
	struct r1conf *conf = mddev->private;

	conf->mirrors = newmirrors;
	kfree(conf->poolinfo);
	conf->poolinfo = newpoolinfo;
```
mddev 是 generic 的，而 r1conf 并不是


## io 返回的场景
```txt
18660 [  998.657409]  [<ffffffffc042abee>] free_r1bio+0x5e/0x80 [raid1]
18661 [  998.657826]  [<ffffffffc042ad68>] close_write+0xb8/0xc0 [raid1]
18662 [  998.658228]  [<ffffffffc042b3e5>] r1_bio_write_done+0x25/0x50 [raid1]
18663 [  998.658664]  [<ffffffffc042bc78>] raid1_end_write_request+0x118/0x2f0 [raid1] # 注册的 hook
18664 [  998.659150]  [<ffffffffc0137b57>] ? ata_scsi_qc_complete+0x67/0x450 [libata]
18665 [  998.659622]  [<ffffffff8128d83c>] bio_endio+0x8c/0x130
18666 [  998.659969]  [<ffffffff81355a40>] blk_update_request+0x90/0x370
18667 [  998.660374]  [<ffffffff814ed944>] scsi_end_request+0x34/0x1e0
18668 [  998.660763]  [<ffffffff814edcb8>] scsi_io_completion+0x168/0x720
18669 [  998.661174]  [<ffffffffc0145053>] ? __ata_sff_port_intr+0xa3/0x130 [libata]
18670 [  998.661757]  [<ffffffff814e2fac>] scsi_finish_command+0xdc/0x140
18671 [  998.662164]  [<ffffffff814ed200>] scsi_softirq_done+0x130/0x160
18672 [  998.662569]  [<ffffffff8135d3c6>] blk_done_softirq+0x96/0xc0
18673 [  998.662955]  [<ffffffff810a4c15>] __do_softirq+0xf5/0x280
18674 [  998.663326]  [<ffffffff817984ec>] call_softirq+0x1c/0x30
18675 [  998.663690]  [<ffffffff8102f715>] do_softirq+0x65/0xa0
18676 [  998.664041]  [<ffffffff810a4f95>] irq_exit+0x105/0x110
18677 [  998.664384]  [<ffffffff81799936>] do_IRQ+0x56/0xf0
18678 [  998.664711]  [<ffffffff8178b36a>] common_interrupt+0x16a/0x16a
```

最后的位置:
```c
static void put_all_bios(struct r1conf *conf, struct r1bio *r1_bio)
{
	int i;

	for (i = 0; i < conf->raid_disks * 2; i++) {
		struct bio **bio = r1_bio->bios + i;
		if (!BIO_SPECIAL(*bio))
			bio_put(*bio);
		*bio = NULL;
	}
}

static void free_r1bio(struct r1bio *r1_bio)
{
	struct r1conf *conf = r1_bio->mddev->private;

	put_all_bios(conf, r1_bio);
	mempool_free(r1_bio, &conf->r1bio_pool);
}
```

其实并不会，因为 `raid1_end_write_request`

## 在 raid1 中，每一个 r1bio 在每一个 disk 中都会持有一个对应的 io
- [ ] 需要等到所有人返回的再返回吧?
