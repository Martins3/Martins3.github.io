# 双系统: blockdev

## 给电脑增加一个新的硬盘
![](./img/xx.jpg)
![](./img/yy.jpg)

## 问题
- [ ] gendisk 之类的到底是啥
- [ ] 直接对于 dev 进行读的时候，其 flamegraph 看看一下
- [ ] partion id 和 UUID 的区别是什么?
- [ ] UUID 存储在哪里了，为什么可以让 bootloader 可以分析
  - [ ] 我感觉分区信息中存在一些让 disk driver 知道在那个位置有数据的操作
- [ ] /etc/fstab 的作用是什么
- [ ] 我无法理解 BMBT 中，为什么一会是 UUID，一会是 PARTUUID 的
- [ ] 谁会去创建 /dev/nvme1n1p2 的，udev 的那个工具吗 ?
- [ ] 理解一下 IO scheduler 和 blkmq 的关系:
  - https://kernel.dk/blk-mq.pdf
  - [ ] 还是使用 flamegraph 分析一下吧，根本
- [ ] 从 ldd3 的 driver 的角度分析一下
- [ ] block/bfq-iosched.c 真的有人使用吗
- [ ] 将 Linux Block Driver 的实验写一下吧
- [ ] block 下存在一个 partitions 的文件夹，说实话，一直都不是非常理解啊
- [ ] 我的印象中，`def_blk_fops` 和下面的这种 ops 不是
  - [ ] Understanding Linux Kernel 的 14 章中的最后一部分分析过 `def_blk_fops` 的 open 操作的
  - [ ] gendisk 持有了 `block_device_operations` 又是做什么的
  - [x] block 设备提供给 multiqueue 的标准接口是啥 : 就是 `mq_ops`
```c
static const struct blk_mq_ops nbd_mq_ops = {
	.queue_rq	= nbd_queue_rq,
	.complete	= nbd_complete_rq,
	.init_request	= nbd_init_request,
	.timeout	= nbd_xmit_timeout,
};
```
- [ ] request queue 和 multiqueue 的关系是什么 ?
  - [ ] block/blk-core.c 中，几乎所有的函数的参数都是有 `request_queue` 的
- [ ] 可能更加有意思的问题是，为什么需要设计 /dev/ 和 major number 和 minor number 出来啊

## overview
<p align="center">
  <img src="https://nvmexpress.org/wp-content/uploads/Linux-storage-stack-diagram_v4.10-e1575939041721.png" alt="drawing" align="center"/>
</p>
<p align="center">
from https://nvmexpress.org/education/drivers/linux-driver-information/
</p>

## 关键的路径

```txt
#0  device_create_file (dev=dev@entry=0xffff88800380c648, attr=attr@entry=0xffffffff829d6c20 <dev_attr_uevent>) at drivers/base/core.c:2753
#1  0xffffffff8171ed09 in device_add (dev=dev@entry=0xffff88800380c648) at drivers/base/core.c:3349
#2  0xffffffff81431024 in device_add_disk (parent=parent@entry=0x0 <fixed_percpu_data>, disk=disk@entry=0xffff888004113800, groups=groups@entry=0x0 <fixed_percpu_data>)at block/genhd.c:464
#3  0xffffffff817426e6 in add_disk (disk=0xffff888004113800) at ./include/linux/blkdev.h:751
#4  loop_add (i=i@entry=0) at drivers/block/loop.c:2064
#5  0xffffffff83003c06 in loop_init () at drivers/block/loop.c:2268
```

## 关键的 ops
1. block/genhd.c 和 fs/block_dev.c 的关系是什么 ?
    1. genhd.c 处理都是 genhd 这个结构体 : 和具体的驱动处理
    2. block_dev 处理的是 block_device 这个内容 : 似乎是用来和 vfs

- [ ] 从 /dev/nvme0n1p1 到 genhd 的过程是什么样子的啊

## [x] io scheduler 和 multiqueue
- Kernel Documentaion : https://www.kernel.org/doc/html/latest/block/blk-mq.html

> blk-mq has two group of queues: software staging queues and hardware dispatch queues. When the request arrives at the block layer, it will try the shortest path possible: send it directly to the hardware queue. However, there are two cases that it might not do that: if there’s an IO scheduler attached at the layer or if we want to try to merge requests. In both cases, requests will be sent to the software queue.

> The block IO subsystem adds requests in the software staging queues (represented by struct blk_mq_ctx) in case that they weren’t sent directly to the driver. A request is one or more BIOs. They arrived at the block layer through the data structure struct bio. The block layer will then build a new structure from it, the struct request that will be used to communicate with the device driver. Each queue has its own lock and the number of queues is defined by a per-CPU or per-node basis.

```c
struct blk_mq_hw_ctx
```
原来还是可以修改 scheduler 的:
- https://linuxhint.com/change-i-o-scheduler-linux/
- https://askubuntu.com/questions/78682/how-do-i-change-to-the-noop-scheduler

检查了一下自己的机器的:
```c
➜ cat /sys/block/nvme0n1/queue/scheduler

[none] mq-deadline
```
应该只是支持 block/mq-deadline.c ，但是实际上并不会采用任何的 scheduler 的。

![](./img/fio.svg)

从函数 `blk_start_plug` 的注释说，plug 表示向 block layer 添加数据。

## ldd3 的驱动理解

- [ ] `register_blkdev` , `add_disk`, `get_gendisk`
  - [x] `register_blkdev` 注册了 major number 在 `major_names` 中间，但是 `major_names` 除了使用在 `blkdev_show` (cat /proc/devices) 之外没有看到其他的用处
    - https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html : 中说，`register_blkdev` 是会取消掉的
    - 从 virtio_blk.c 中间来看 : major 放到局部变量中间，所以实际功能是分配 major number

- [ ] `alloc_disk` 分配 `struct gendisk`，其中由于保存分区的

## bio layer
- [A block layer introduction part 1: the bio layer](https://lwn.net/Articles/736534/) https://yq.aliyun.com/articles/609907

- [Block layer introduction part 2: the request layer](https://lwn.net/Articles/738449/)
- [ ] 重新确定一下，request layer 的位置

http://byteliu.com/2019/05/10/Linux-The-block-I-O-layer/

http://byteliu.com/2019/05/21/What-is-the-major-difference-between-the-buffer-cache-and-the-page-cache-Why-were-they-separate-entities-in-older-kernels-Why-were-they-merged-later-on/

1. bio 给上下两个层次提供的接口是什么 ?
2. https://zhuanlan.zhihu.com/p/39199521
    1. bio 机制核心 : 合并请求

- [ ] 那么 bio 真的会合并请求吗 ?

## kernel
```plain
#0  __register_blkdev (major=major@entry=259, name=name@entry=0xffffffff825c1b9e "blkext", probe=probe@entry=0x0 <fixed_percpu_data>) at block/genhd.c:247
#1  0xffffffff82ff48bb in genhd_device_init () at block/genhd.c:876
#2  0xffffffff81000dbf in do_one_initcall (fn=0xffffffff82ff4879 <genhd_device_init>) at init/main.c:1298
#3  0xffffffff82fc5405 in do_initcall_level (command_line=0xffff888003546040 "root", level=4) at ./include/linux/compiler.h:234
#4  do_initcalls () at init/main.c:1387
#5  do_basic_setup () at init/main.c:1406
#6  kernel_init_freeable () at init/main.c:1613
#7  0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#8  0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#9  0x0000000000000000 in ?? ()

#0  __register_blkdev (major=major@entry=9, name=name@entry=0xffffffff8262944f "md", probe=probe@entry=0xffffffff818b5c60 <md_probe>) at block/genhd.c:247
#1  0xffffffff830071ad in md_init () at drivers/md/md.c:9619
#2  0xffffffff81000dbf in do_one_initcall (fn=0xffffffff8300712a <md_init>) at init/main.c:1298
#3  0xffffffff82fc5405 in do_initcall_level (command_line=0xffff888003546040 "root", level=4) at ./include/linux/compiler.h:234
#4  do_initcalls () at init/main.c:1387
#5  do_basic_setup () at init/main.c:1406
#6  kernel_init_freeable () at init/main.c:1613
#7  0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#8  0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#9  0x0000000000000000 in ?? ()

#0  __register_blkdev (major=major@entry=7, name=name@entry=0xffffffff826229bf "loop", probe=probe@entry=0xffffffff81742760 <loop_probe>) at block/genhd.c:247
#1  0xffffffff83003bdc in loop_init () at drivers/block/loop.c:2261
#2  0xffffffff81000dbf in do_one_initcall (fn=0xffffffff83003b33 <loop_init>) at init/main.c:1298
#3  0xffffffff82fc5405 in do_initcall_level (command_line=0xffff888003546040 "root", level=6) at ./include/linux/compiler.h:234
#4  do_initcalls () at init/main.c:1387
#5  do_basic_setup () at init/main.c:1406
#6  kernel_init_freeable () at init/main.c:1613
#7  0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#8  0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#9  0x0000000000000000 in ?? ()

#0  __register_blkdev (major=major@entry=0, name=name@entry=0xffffffff82622a3c "virtblk", probe=probe@entry=0x0 <fixed_percpu_data>) at block/genhd.c:247
#1  0xffffffff83003c50 in virtio_blk_init () at drivers/block/virtio_blk.c:1031
#2  0xffffffff81000dbf in do_one_initcall (fn=0xffffffff83003c1c <virtio_blk_init>) at init/main.c:1298
#3  0xffffffff82fc5405 in do_initcall_level (command_line=0xffff888003546040 "root", level=6) at ./include/linux/compiler.h:234
#4  do_initcalls () at init/main.c:1387
#5  do_basic_setup () at init/main.c:1406
#6  kernel_init_freeable () at init/main.c:1613
#7  0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#8  0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#9  0x0000000000000000 in ?? ()

#0  __register_blkdev (major=8, name=name@entry=0xffffffff8267346b "sd", probe=probe@entry=0xffffffff81760bf0 <sd_default_probe>) at block/genhd.c:247
#1  0xffffffff8300403e in init_sd () at drivers/scsi/sd.c:3729
#2  0xffffffff81000dbf in do_one_initcall (fn=0xffffffff8300401e <init_sd>) at init/main.c:1298
#3  0xffffffff82fc5405 in do_initcall_level (command_line=0xffff888003546040 "root", level=6) at ./include/linux/compiler.h:234
#4  do_initcalls () at init/main.c:1387
#5  do_basic_setup () at init/main.c:1406
#6  kernel_init_freeable () at init/main.c:1613
#7  0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#8  0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#9  0x0000000000000000 in ?? ()

#0  __register_blkdev (major=65, name=name@entry=0xffffffff8267346b "sd", probe=probe@entry=0xffffffff81760bf0 <sd_default_probe>) at block/genhd.c:247
#1  0xffffffff8300403e in init_sd () at drivers/scsi/sd.c:3729
#2  0xffffffff81000dbf in do_one_initcall (fn=0xffffffff8300401e <init_sd>) at init/main.c:1298
#3  0xffffffff82fc5405 in do_initcall_level (command_line=0xffff888003546040 "root", level=6) at ./include/linux/compiler.h:234
#4  do_initcalls () at init/main.c:1387
#5  do_basic_setup () at init/main.c:1406
#6  kernel_init_freeable () at init/main.c:1613
#7  0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#8  0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#9  0x0000000000000000 in ?? ()
```

## struct
- gendisk : 侧重和硬件交互
  - `add_disk` : 删除
  - `alloc_disk` : 创建
- `block_device` : 侧重和文件系统交互
- `struct hd_struct` : 描述分区信息

## md

```plain
#1  0xffffffff830071ad in md_init () at drivers/md/md.c:9619
```

## loop device

检查一下
```plain
#1  0xffffffff83003bdc in loop_init () at drivers/block/loop.c:2261
```


## partitions
构建多个分区显然是一个很划算的事情。

- https://www.baeldung.com/linux/partitioning-disks
> Instead of loading a boot loader from the MBR, UEFI uses efi images from the EFI System Partition. With UEFI and GPT, we can have large disk support.

- https://unix.stackexchange.com/questions/542223/how-to-create-a-logical-partition
> 各种 logical partition 和 extent partition 都是 MBR 之类的 legacy 时代的东西

```sh
bpftrace -e 'kprobe:blk_add_partitions { @[kstack] = count(); }'
```

```txt
@[
    blk_add_partitions+1
    __blkdev_get+228
    blkdev_get_by_dev+281
    blkdev_common_ioctl+2047
    blkdev_ioctl+228
    block_ioctl+61
    __x64_sys_ioctl+145
    do_syscall_64+97
    entry_SYSCALL_64_after_hwframe+68
]: 4
```

## nbd
将 `blk_mq_ops` 替换成为网络的接口就可以了

```c
static const struct blk_mq_ops nbd_mq_ops = {
	.queue_rq	= nbd_queue_rq,
	.complete	= nbd_complete_rq,
	.init_request	= nbd_init_request,
	.timeout	= nbd_xmit_timeout,
};
```

## 一些基本理论

### /proc/mounts
如何实现的

### /proc/partitions
这个东西是如何实现的，为什么

### /dev/nvme1n1p2
- [ ] 解释一下为什么不可以 mount /dev/nvme0n1，而是必须为

### /sys/dev/block/

## gui tools
### gpart
创建分区表和格式化分区

### gnome-Disks
https://askubuntu.com/questions/164926/how-to-make-partitions-mount-at-startup

## cli tools

### fdisk
- 处理分区表的

使用 `sudo fdisk -l` 可以得到如下结果

```txt
Disk /dev/nvme0n1: 476.96 GiB, 512110190592 bytes, 1000215216 sectors
Disk model: ZHITAI TiPlus5000 512GB
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: gpt
Disk identifier: 55CE3071-1646-4AB2-B052-511413221486

Device         Start        End    Sectors  Size Type
/dev/nvme0n1p1  2048 1000214527 1000212480  477G Linux filesystem


Disk /dev/nvme1n1: 238.49 GiB, 256060514304 bytes, 500118192 sectors
Disk model: SAMSUNG MZVLW256HEHP-00000
Units: sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disklabel type: gpt
Disk identifier: 3AD6EE50-0EA1-4F4D-9685-E3BABA41E442

Device           Start       End   Sectors  Size Type
/dev/nvme1n1p1    2048   1050623   1048576  512M EFI System
/dev/nvme1n1p2 1050624 500117503 499066880  238G Linux filesystem
```

### blkid

```txt
/dev/nvme1n1p2: UUID="7876593d-555f-4a0d-8c9f-9d90e1a94343" TYPE="ext4" PARTUUID="409f4ba4-bdf5-4d00-942e-9f7997056bba"
/dev/loop0: TYPE="squashfs"
/dev/nvme0n1p1: UUID="12855c04-ce76-47a0-840c-206253052ccf" TYPE="ext4" PARTUUID="ca0534aa-4c0b-4db1-b028-d73dc442b6e4"
/dev/nvme1n1p1: UUID="9EDC-2FD6" TYPE="vfat" PARTLABEL="EFI System Partition" PARTUUID="b04350e1-236a-4009-8855-9e49ee2d2456"
/dev/loop47: TYPE="squashfs"
```

### df
利用 [statfs(2)](https://man7.org/linux/man-pages/man2/statfs.2.html) 获取系统中文件系统的信息的。

使用 `df -h` 查看如下内容:
```txt
Filesystem      Size  Used Avail Use% Mounted on
udev            7.8G     0  7.8G   0% /dev
tmpfs           1.6G  2.5M  1.6G   1% /run
/dev/nvme1n1p2  234G  189G   33G  86% /
tmpfs           7.8G  291M  7.5G   4% /dev/shm
tmpfs           5.0M  4.0K  5.0M   1% /run/lock
tmpfs           7.8G     0  7.8G   0% /sys/fs/cgroup
/dev/loop0      128K  128K     0 100% /snap/bare/5
/dev/nvme0n1p1  469G  101G  345G  23% /home/maritns3/hack
/dev/nvme1n1p1  511M  5.3M  506M   2% /boot/efi
tmpfs           1.6G  320K  1.6G   1% /run/user/1000
/dev/loop47      84M   84M     0 100% /snap/veloren/580
```

### lsblk
```txt
NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINT
loop0         7:0    0     4K  1 loop /snap/bare/5
loop47        7:47   0  83.3M  1 loop /snap/veloren/580
nvme0n1     259:0    0   477G  0 disk
└─nvme0n1p1 259:1    0   477G  0 part /home/maritns3/hack
nvme1n1     259:2    0 238.5G  0 disk
├─nvme1n1p1 259:3    0   512M  0 part /boot/efi
└─nvme1n1p2 259:4    0   238G  0 part /
```

- /sys/dev/block/
- /dev/nvme1n1p1

- [ ] 找到对应的内核源代码，然后 ptrace 一下:

# block_dev.c 分析
> 主要的内容 : 实现 everything is file 的设计思路，不是放到 fs 下面实现 block io 的


## Doc
1. https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2018/06/14/linux-block-device-driver

When we add devices to system, a node in /dev/ will be created, this is done in ‘devtmpfs_create_node’.

2. https://unix.stackexchange.com/questions/176215/difference-between-dev-and-sys
3. https://sungju.github.io/kernel/internals/block_device_driver.html : 下面的插图有意思

![](https://sungju.github.io/kernel/internals/block_device_driver.jpg)

### Question
1. block_dev 是不是成为 block layer 的屏蔽层，这是文件系统看到的全部的内容
2. swap 的 partion 是如何自动被识别出来的 ?
3. ext4 文件系统注册在不同的 block 上，所以如何实现 submit_bio 进入到不同的 block 中间里面。
4. 现在，觉得，block_dev.c 只是 block layer 需要使用 fs 的内容而已，block 保证几个 page 的 io，但是全局的 io 需要 fs 提供。
5. 注意，block 上也可以挂载 inode 和 address_space，猜测，仅仅限于，当没有 fs 挂载到其上的时候 !

6. so, what is `struct block_device` ?
      1. relation with module ?
      2. I want to read the routin that the bios(or something else probe the device and create bdev_inode on the bdev file system )

8. blockdev_superblock :
    1. mount details ? f

9. how to add block device to the bdev file system ?

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

在 mount 的过程中，根据路径得到:
```txt
#0  blkdev_get_by_dev (dev=8388611, mode=129, holder=0xffffffff8296b2a0 <ext3_fs_type>) at block/bdev.c:787
#1  0xffffffff81415352 in blkdev_get_by_path (path=<optimized out>, mode=mode@entry=129, holder=0xffffffff8296b2a0 <ext3_fs_type>) at block/bdev.c:881
#2  0xffffffff81212297 in get_tree_bdev (fc=0xffff888004425300, fill_super=0xffffffff81310db0 <ext4_fill_super>) at fs/super.c:1242
#3  0xffffffff81211490 in vfs_get_tree (fc=fc@entry=0xffff888004425300) at fs/super.c:1497
#4  0xffffffff81238668 in do_new_mount (data=0x0 <fixed_percpu_data>, name=0xffffffff8257820b "/dev/root", mnt_flags=96, sb_flags=<optimized out>, fstype=0xffffc90000013de0 "`qQ\003\200\210\377\377", path=0xffffc90000013de0) at fs/namespace.c:3040
#5  path_mount (dev_name=dev_name@entry=0xffffffff8257820b "/dev/root", path=path@entry=0xffffc90000013de0, type_page=type_page@entry=0xffff8880044b6000 "ext3", flags=<optimized out>, flags@entry=32769, data_page=data_page@entry=0x0 <fixed_percpu_data>) at fs/namespace.c:3370
#6  0xffffffff82fef8fd in init_mount (dev_name=dev_name@entry=0xffffffff8257820b "/dev/root", dir_name=dir_name@entry=0xffffffff8257820f "/root", type_page=type_page@entry=0xffff8880044b6000 "ext3", flags=flags@entry=32769, data_page=0x0 <fixed_percpu_data>) at fs/init.c:25
#7  0xffffffff82fc5568 in do_mount_root (name=name@entry=0xffffffff8257820b "/dev/root", fs=fs@entry=0xffff8880044b6000 "ext3", flags=flags@entry=32769, data=<optimized out>) at init/do_mounts.c:375
#8  0xffffffff82fc5724 in mount_block_root (name=name@entry=0xffffffff8257820b "/dev/root", flags=32769) at init/do_mounts.c:414
#9  0xffffffff82fc5a0e in mount_root () at init/do_mounts.c:592
#10 0xffffffff82fc5b68 in prepare_namespace () at init/do_mounts.c:644
#11 0xffffffff82fc5442 in kernel_init_freeable () at init/main.c:1626
#12 0xffffffff81c9e851 in kernel_init (unused=<optimized out>) at init/main.c:1502
#13 0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#14 0x0000000000000000 in ?? ()
```
- 通过 `lookup_bdev` 可以将 path 装换为 `dev_t`

## interface

```c
static const struct super_operations bdev_sops = {
	.statfs = simple_statfs,
	.alloc_inode = bdev_alloc_inode,
	.destroy_inode = bdev_destroy_inode,
	.drop_inode = generic_delete_inode,
	.evict_inode = bdev_evict_inode,
};

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
	.open		= blkdev_open,
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
`address_space_operations` 是文件系统注册的，用于向下传导的

- `blkdev_readpage` 和 `ext4_mpage_readpages` 都会调用 `block_read_full_page` 的，但是 ext4 中会去调用 `ext4_get_block` 的实现。
  - `get_block_t`，给定一个 inode 以及偏移量，找到对应的 block number

简单分析一下 `super_operations` 的过程:
```plain
#0  bdev_alloc_inode (sb=0xffff888003455800) at block/bdev.c:388
#1  0xffffffff8122dab8 in alloc_inode (sb=0xffff888003455800) at fs/inode.c:260
#2  0xffffffff8122fc78 in new_inode_pseudo (sb=<optimized out>) at fs/inode.c:1018
#3  0xffffffff8122fcde in new_inode (sb=<optimized out>) at fs/inode.c:1047
#4  0xffffffff81414e38 in bdev_alloc (disk=disk@entry=0xffff8880043a3b00, partno=partno@entry=1 '\001') at block/bdev.c:479
#5  0xffffffff81433135 in add_partition (disk=disk@entry=0xffff8880043a3b00, partno=partno@entry=1, start=start@entry=2048, len=204800, flags=0, info=0xffffc900001e1095 at block/partitions/core.c:355
#6  0xffffffff814338ca in blk_add_partition (state=0xffff8880043d20c0, p=1, disk=0xffff8880043a3b00) at block/partitions/core.c:585
#7  blk_add_partitions (disk=0xffff8880043a3b00) at block/partitions/core.c:652
#8  bdev_disk_changed (invalidate=<optimized out>, disk=<optimized out>) at block/partitions/core.c:694
#9  bdev_disk_changed (disk=disk@entry=0xffff8880043a3b00, invalidate=invalidate@entry=false) at block/partitions/core.c:661
#10 0xffffffff8141448b in blkdev_get_whole (bdev=bdev@entry=0xffff888003814600, mode=mode@entry=1) at block/bdev.c:679
#11 0xffffffff814151a7 in blkdev_get_by_dev (holder=0x0 <fixed_percpu_data>, mode=1, dev=<optimized out>) at block/bdev.c:816
#12 blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>) at block/bdev.c:780
#13 0xffffffff81430f2f in disk_scan_partitions (disk=disk@entry=0xffff8880043a3b00, mode=mode@entry=1) at ./include/linux/blkdev.h:196
#14 0xffffffff8143127e in device_add_disk (parent=parent@entry=0xffff8880042f8198, disk=disk@entry=0xffff8880043a3b00, groups=groups@entry=0x0 <fixed_percpu_data>) at block/genhd.c:523
#15 0xffffffff81765cf7 in sd_probe (dev=0xffff8880042f8198) at drivers/scsi/sd.c:3475
#16 0xffffffff81722350 in call_driver_probe (drv=0xffffffff829dfce0 <sd_template>, drv=0xffffffff829dfce0 <sd_template>, dev=0xffff8880042f8198) at drivers/base/dd.c:542
#17 really_probe (drv=0xffffffff829dfce0 <sd_template>, dev=0xffff8880042f8198) at drivers/base/dd.c:621
#18 really_probe (dev=0xffff8880042f8198, drv=0xffffffff829dfce0 <sd_template>) at drivers/base/dd.c:566
#19 0xffffffff817224fd in __driver_probe_device (drv=drv@entry=0xffffffff829dfce0 <sd_template>, dev=dev@entry=0xffff8880042f8198) at drivers/base/dd.c:752
#20 0xffffffff81722579 in driver_probe_device (drv=drv@entry=0xffffffff829dfce0 <sd_template>, dev=dev@entry=0xffff8880042f8198) at drivers/base/dd.c:782
#21 0xffffffff81722807 in __device_attach_driver (drv=0xffffffff829dfce0 <sd_template>, _data=0xffffc9000004be48) at drivers/base/dd.c:899
#22 0xffffffff81720569 in bus_for_each_drv (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffc9000004be48, fn=fn@entry=0xffffffff817227a0 <__device_attach_driver>) at drivers/base/bus.c:427
#23 0xffffffff81721cba in __device_attach_async_helper (_dev=0xffff8880042f8198, cookie=<optimized out>) at drivers/base/dd.c:928
#24 0xffffffff8109250b in async_run_entry_fn (work=0xffff888004278bc0) at kernel/async.c:127
#25 0xffffffff81086133 in process_one_work (worker=0xffff88800344da80, work=0xffff888004278bc0) at kernel/workqueue.c:2289
#26 0xffffffff81086345 in worker_thread (__worker=0xffff88800344da80) at kernel/workqueue.c:2436
#27 0xffffffff8108d742 in kthread (_create=0xffff888003546040) at kernel/kthread.c:376
#28 0xffffffff810019a2 in ret_from_fork () at arch/x86/entry/entry_64.S:298
#29 0x0000000000000000 in ?? ()
```

## 结束语
好的，你现在对于 Linux 如何处理 Block 设备有了一个大概的认识，记得奖励自己一把英雄联盟哦。

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
