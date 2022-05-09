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
- [ ] 谁会去创建 /dev/nvme1n1p2 的，为什么
- [ ] mknod 的原理是什么
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

## block layer
<p align="center">
  <img src="https://nvmexpress.org/wp-content/uploads/Linux-storage-stack-diagram_v4.10-e1575939041721.png" alt="drawing" align="center"/>
</p>
<p align="center">
from https://nvmexpress.org/education/drivers/linux-driver-information/
</p>

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
- `block_device` : 侧重和文件系统交互
- `struct hd_struct` : 描述分区信息

# genhd.c

1. 700 :  alloc_disk add_disk
2. 1300 : ref counter
3. end : events


## TODO
1. block/genhd.c 和 fs/block_dev.c 的关系是什么 ?
    1. genhd.c 处理都是 genhd 这个结构体 : 和具体的驱动处理
    2. block_dev 处理的是 block_device 这个内容 : 似乎是用来和 vfs
    3. 所以，两者如何是如何关联在一起的 ?

2. partion 和 minor number 的关系 ?
3. add_disk 调用的内容非常的多 !
4. ldd ch16 在 disk 初始化的过程中间，完成 make_request_fn 的注册，所以，整个 mq ，io scheduler 都是 lib 吗 ? driver 如何和他们协作的 ?

## add_disk 和 alloc_disk

```c
static inline void add_disk(struct gendisk *disk)
{
	device_add_disk(NULL, disk, NULL);
}

void device_add_disk(struct device *parent, struct gendisk *disk,
		     const struct attribute_group **groups)

{
	__device_add_disk(parent, disk, groups, true);
}
EXPORT_SYMBOL(device_add_disk);


#define alloc_disk(minors) alloc_disk_node(minors, NUMA_NO_NODE)

#define alloc_disk_node(minors, node_id)				\
({									\
	static struct lock_class_key __key;				\
	const char *__name;						\
	struct gendisk *__disk;						\
									\
	__name = "(gendisk_completion)"#minors"("#node_id")";		\
									\
	__disk = __alloc_disk_node(minors, node_id);			\
									\
	if (__disk)							\
		lockdep_init_map(&__disk->lockdep_map, __name, &__key, 0); \
									\
	__disk;								\
})
```


1. 到底如何管理设备 ? 至少让 fs mount 的时候可以找到它

```c
/**
 * register_blkdev - register a new block device
 *
 * @major: the requested major device number [1..BLKDEV_MAJOR_MAX-1]. If
 *         @major = 0, try to allocate any unused major number.
 * @name: the name of the new block device as a zero terminated string
 *
 * The @name must be unique within the system.
 *
 * The return value depends on the @major input parameter:
 *
 *  - if a major device number was requested in range [1..BLKDEV_MAJOR_MAX-1]
 *    then the function returns zero on success, or a negative error code
 *  - if any unused major number was requested with @major = 0 parameter
 *    then the return value is the allocated major number in range
 *    [1..BLKDEV_MAJOR_MAX-1] or a negative error code otherwise
 *
 * See Documentation/admin-guide/devices.txt for the list of allocated
 * major numbers.
 */
int register_blkdev(unsigned int major, const char *name) // 很简单，向major_names 中间注册即可发
```

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
