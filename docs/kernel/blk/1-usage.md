# Linux block layer 用户态工具

## Block Layer 设备注册关键路径

```txt
#0  device_create_file (dev=dev@entry=0xffff88800380c648, attr=attr@entry=0xffffffff829d6c20 <dev_attr_uevent>) at drivers/base/core.c:2753
#1  0xffffffff8171ed09 in device_add (dev=dev@entry=0xffff88800380c648) at drivers/base/core.c:3349
#2  0xffffffff81431024 in device_add_disk (parent=parent@entry=0x0 <fixed_percpu_data>, disk=disk@entry=0xffff888004113800, groups=groups@entry=0x0 <fixed_percpu_data>)at block/genhd.c:464
#3  0xffffffff817426e6 in add_disk (disk=0xffff888004113800) at ./include/linux/blkdev.h:751
#4  loop_add (i=i@entry=0) at drivers/block/loop.c:2064
#5  0xffffffff83003c06 in loop_init () at drivers/block/loop.c:2268
```

## /dev/sda1 这个路径是谁创建的
本来以为是 udev 在系统启动的时候创建的，但是曾经使用 initramfs 的，/dev 一旦 mount 上，里面啥都有了。

drivers/base/devtmpfs.c:devtmpfs_work_loop 中打点，可以看到如下设备都是内核直接添加的:
```txt
rfkill vga_arbiter mem null port zero full random urandom kmsg
tty console tty0 vcs vcsu vcsa vcs1 vcsu1 vcsa1 tty1
tty2 tty3 tty4 tty5 tty6 tty7 tty8 tty9 tty10 tty11
tty12 tty13 tty14 tty15 tty16 tty17 tty18 tty19 tty20 tty21
tty22 tty23 tty24 tty25 tty26 tty27 tty28 tty29 tty30 tty31
tty32 tty33 tty34 tty35 tty36 tty37 tty38 tty39 tty40 tty41
tty42 tty43 tty44 tty45 tty46 tty47 tty48 tty49 tty50 tty51
tty52 tty53 tty54 tty55 tty56 tty57 tty58 tty59 tty60 tty61
tty62 tty63 hwrng kvm cpu/0/msr cpu/1/msr cpu/2/msr cpu/3/msr cpu/4/msr cpu/5/msr
cpu/0/cpuid cpu/1/cpuid cpu/2/cpuid cpu/3/cpuid cpu/4/cpuid cpu/5/cpuid snapshot userfaultfd autofs fuse
ptmx ttyS0 ttyS1 ttyS2 ttyS3 hpet nvram loop-control loop0 loop1
loop2 loop3 loop4 loop5 loop6 loop7 vda vdb vdb1 vdb2
vdb3 ublk-control bsg/0:0:0:0 sda sg0 sg1 bsg/1:0:0:0 nvme-fabrics nvme0 net/tun
usbmon0 sdb input/event0 rtc0 ptp0 mapper/control snd/timer input/event1 snd/seq nvme0n1
ng0n1 cpu_dma_latency sg2 bsg/2:0:0:0 sg3 bsg/2:0:1:0 sdc sdd input/event2 vcs6
vcsu6 vcsa6 vcs2 vcsu2 vcsa2 vcs3 vcsu3 vcsa3 vcs4 vcsu4
vcsa4 vcs5 vcsu5 vcsa5
```

## partitions

对应的源码，在 block/partion 的位置

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

## 一些基本理论

### /etc/fstab

指挥 systemd 如何加载 disk 的:
```txt
➜  vn git:(master) ✗ cat /etc/fstab
# /etc/fstab: static file system information.
#
# Use 'blkid' to print the universally unique identifier for a
# device; this may be used with UUID= as a more robust way to name devices
# that works even if disks are added and removed. See fstab(5).
#
# <file system> <mount point>   <type>  <options>       <dump>  <pass>
# / was on /dev/nvme0n1p2 during installation
UUID=7876593d-555f-4a0d-8c9f-9d90e1a94343 /               ext4    errors=remount-ro 0       1
# /boot/efi was on /dev/nvme0n1p1 during installation
UUID=9EDC-2FD6  /boot/efi       vfat    umask=0077      0       1
/swapfile                                 none            swap    sw              0       0
/dev/disk/by-uuid/12855c04-ce76-47a0-840c-206253052ccf /home/maritns3/hack auto nosuid,nodev,nofail,x-gvfs-show 0 0
```

## cli tools

### parted : 格式化设备

```sh
parted /dev/nvme1n1 -- mklabel gpt # 制作分区表，所有的分区都会被清理掉
parted /dev/nvme1n1 -- mkpart primary 1MiB 100%
```

```sh
parted /dev/sdc -- mklabel gpt
parted /dev/sdc -- mkpart primary 1MiB -1MiB
```

扩展分区，原来的分区是这个样子的:
```txt
nvme0n1     259:0    0   1.5T  0 disk
├─nvme0n1p1 259:1    0   512M  0 part /boot/efi
├─nvme0n1p2 259:2    0     5G  0 part /boot
├─nvme0n1p3 259:3 0 185G 0 part /
└─nvme0n1p4 259:4 0 100G 0 part
```

```txt
sudo parted /dev/nvme0n1
(parted) rm 4
(parted) resizepart 3 100%
# 可能需要自动输入一下 end ，例如 1500GB
```

然后增大文件系统:
```sh
resize2fs /dev/nvme0n1p3
xfs_growfs /
```

然后就可以观察到这些:
```txt
nvme0n1     259:0    0   1.5T  0 disk
├─nvme0n1p1 259:1    0   512M  0 part /boot/efi
├─nvme0n1p2 259:2    0     5G  0 part /boot
└─nvme0n1p3 259:3    0   1.4T  0 part /
```

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

主要读取如下: /sys/dev/block/

### 制作系统盘

ventoy

大多数时候制作启动盘可以通过这个方法:
sudo dd if=openEuler-22.09-x86_64-dvd.iso of=/dev/sda

但是有几次，windows 的启动盘 dd 的方法没用，可以采用这个工具:

https://www.ventoy.net/cn/index.html

1. 首先: sudo ventoy -i /dev/sda ，这会构建两个 partion 出来
2. 然后 mount 其中较大的那个，将 iso 拷贝进去即可。

### 常用操作
- fdisk 可以非常方便的创建新的分区，如果当前磁盘还有空间的话
  - fdisk  输入 m 查看 help ，输入 n ，一路 enter ，最后 w


## ldd3 的驱动理解

- gendisk : 对应的一个设备
- block_device : 对应一个 partion


- [ ] `register_blkdev` , `add_disk`, `get_gendisk`
  - [x] `register_blkdev` 注册了 major number 在 `major_names` 中间，但是 `major_names` 除了使用在 `blkdev_show` (cat /proc/devices) 之外没有看到其他的用处
    - https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html : 中说，`register_blkdev` 是会取消掉的
    - 从 virtio_blk.c 中间来看 : major 放到局部变量中间，所以实际功能是分配 major number

- [ ] `alloc_disk` 分配 `struct gendisk`，其中由于保存分区的

block_device_operations

Char devices make their operations available to the system by way of the `file_operations` structure. A similar structure is used with block devices; it is `struct
block_device_operations`, which is declared in `<linux/blkdev.h>`.

```c
struct block_device_operations {
  int (*open) (struct block_device *, fmode_t);
  void (*release) (struct gendisk *, fmode_t);
  int (*rw_page)(struct block_device *, sector_t, struct page *, int rw);
  int (*ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
  int (*compat_ioctl) (struct block_device *, fmode_t, unsigned, unsigned long);
  long (*direct_access)(struct block_device *, sector_t, void __pmem **,
      unsigned long *pfn);
  unsigned int (*check_events) (struct gendisk *disk,
              unsigned int clearing);
  /* ->media_changed() is DEPRECATED, use ->check_events() instead */
  int (*media_changed) (struct gendisk *);
  void (*unlock_native_capacity) (struct gendisk *);
  int (*revalidate_disk) (struct gendisk *);
  int (*getgeo)(struct block_device *, struct hd_geometry *);
  /* this callback is with swap_lock and sometimes page table lock held */
  void (*swap_slot_free_notify) (struct block_device *, unsigned long);
  struct module *owner;
  const struct pr_ops *pr_ops;
};
```
1. 这些函数的注册都是让驱动完成的
2. 这些函数的使用位置在哪里啊 ?
    1. open : blkdev_get : @todo 当设备出现在 /dev/nvme0n1p1 上的时候，此时 blkdev_get 被调用过没有，blkdev_get 会被调用吗 ? 如果不调用，那么似乎驱动就没有被初始化，那么连 aops 的实现基础 bio request 之类的实现似乎无从谈起了。


## 使用 drivers/scsi/sd.c 作为例子分析一下

## add_disk
```plain
#0  device_add_disk (parent=parent@entry=0xffff888100c22010, disk=0xffff8880055f3400, groups=groups@entry=0xffffffff82e1b8d0 <virtblk_attr_groups>) at block/genhd.c:393
#1  0xffffffff81aa2ba1 in virtblk_probe (vdev=0xffff888100c22000) at drivers/block/virtio_blk.c:1150
#2  0xffffffff81809e6b in virtio_dev_probe (_d=0xffff888100c22010) at drivers/virtio/virtio.c:305
#3  0xffffffff81a75481 in call_driver_probe (drv=0xffffffff82e1b760 <virtio_blk>, dev=0xffff888100c22010) at drivers/base/dd.c:560
#4  really_probe (dev=dev@entry=0xffff888100c22010, drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:639
#5  0xffffffff81a756bd in __driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff888100c22010) at drivers/base/dd.c:778
#6  0xffffffff81a75749 in driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff888100c22010) at drivers/base/dd.c:808
#7  0xffffffff81a759c5 in __driver_attach (data=0xffffffff82e1b760 <virtio_blk>, dev=0xffff888100c22010) at drivers/base/dd.c:1194
#8  __driver_attach (dev=0xffff888100c22010, data=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1134
#9  0xffffffff81a72fc4 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82e1b760 <virtio_blk>, fn=fn@entry=0xffffffff81a75940 <__driver_attach>) at drivers/base/bus.c:301
#10 0xffffffff81a74e79 in driver_attach (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1211
#11 0xffffffff81a74810 in bus_add_driver (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/bus.c:618
#12 0xffffffff81a76c2e in driver_register (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/driver.c:246
#13 0xffffffff8180958b in register_virtio_driver (driver=driver@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/virtio/virtio.c:357
#14 0xffffffff835eb0fb in virtio_blk_init () at drivers/block/virtio_blk.c:1284
#15 0xffffffff81001940 in do_one_initcall (fn=0xffffffff835eb0aa <virtio_blk_init>) at init/main.c:1306
#16 0xffffffff8359b818 in do_initcall_level (command_line=0xffff888003c5cc00 "root", level=6) at init/main.c:1379
#17 do_initcalls () at init/main.c:1395
#18 do_basic_setup () at init/main.c:1414
#19 kernel_init_freeable () at init/main.c:1634
#20 0xffffffff82183405 in kernel_init (unused=<optimized out>) at init/main.c:1522
#21 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

virtio 的 probe 没有经过 pci ?


## partion 的生成
在读取系统的 partion table 的时候
```txt
#0  add_partition (disk=disk@entry=0xffff8880044ccc00, partno=partno@entry=1, start=start@entry=2048, len=1024000, flags=0, info=0xffffc90001562095) at block/partitions/core.c:318
#1  0xffffffff816dae18 in blk_add_partition (p=1, state=0xffff8880054ba840, disk=0xffff8880044ccc00) at block/partitions/core.c:576
#2  blk_add_partitions (disk=0xffff8880044ccc00) at block/partitions/core.c:646
#3  bdev_disk_changed (invalidate=<optimized out>, disk=<optimized out>) at block/partitions/core.c:688
#4  bdev_disk_changed (disk=disk@entry=0xffff8880044ccc00, invalidate=invalidate@entry=false) at block/partitions/core.c:655
#5  0xffffffff816b6ab5 in blkdev_get_whole (bdev=bdev@entry=0xffff888140803c00, mode=mode@entry=1) at block/bdev.c:685
#6  0xffffffff816b779d in blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>) at block/bdev.c:822
#7  0xffffffff816b79c0 in blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>) at block/bdev.c:856
#8  0xffffffff816d845b in disk_scan_partitions (disk=disk@entry=0xffff8880044ccc00, mode=mode@entry=1, owner=owner@entry=0x0 <fixed_percpu_data>) at block/genhd.c:374
#9  0xffffffff816d8822 in device_add_disk (parent=parent@entry=0xffff8880045f2810, disk=0xffff8880044ccc00, groups=groups@entry=0xffffffff82e1b8d0 <virtblk_attr_groups>) at block/genhd.c:502
#10 0xffffffff81aa2bc1 in virtblk_probe (vdev=0xffff8880045f2800) at drivers/block/virtio_blk.c:1150
#11 0xffffffff81809e8b in virtio_dev_probe (_d=0xffff8880045f2810) at drivers/virtio/virtio.c:305
#12 0xffffffff81a754a1 in call_driver_probe (drv=0xffffffff82e1b760 <virtio_blk>, dev=0xffff8880045f2810) at drivers/base/dd.c:560
#13 really_probe (dev=dev@entry=0xffff8880045f2810, drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:639
#14 0xffffffff81a756dd in __driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff8880045f2810) at drivers/base/dd.c:778
#15 0xffffffff81a75769 in driver_probe_device (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>, dev=dev@entry=0xffff8880045f2810) at drivers/base/dd.c:808
#16 0xffffffff81a759e5 in __driver_attach (data=0xffffffff82e1b760 <virtio_blk>, dev=0xffff8880045f2810) at drivers/base/dd.c:1194
#17 __driver_attach (dev=0xffff8880045f2810, data=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1134
#18 0xffffffff81a72fe4 in bus_for_each_dev (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffffff82e1b760 <virtio_blk>, fn=fn@entry=0xffffffff81a75960 <__driver_attach>) at drivers/base/bus.c:301
#19 0xffffffff81a74e99 in driver_attach (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/dd.c:1211
#20 0xffffffff81a74830 in bus_add_driver (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/bus.c:618
#21 0xffffffff81a76c4e in driver_register (drv=drv@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/base/driver.c:246
#22 0xffffffff818095ab in register_virtio_driver (driver=driver@entry=0xffffffff82e1b760 <virtio_blk>) at drivers/virtio/virtio.c:357
#23 0xffffffff835eb0fb in virtio_blk_init () at drivers/block/virtio_blk.c:1284
#24 0xffffffff81001940 in do_one_initcall (fn=0xffffffff835eb0aa <virtio_blk_init>) at init/main.c:1306
#25 0xffffffff8359b818 in do_initcall_level (command_line=0xffff888003c5cc00 "root", level=6) at init/main.c:1379
#26 do_initcalls () at init/main.c:1395
#27 do_basic_setup () at init/main.c:1414
#28 kernel_init_freeable () at init/main.c:1634
#29 0xffffffff821854a5 in kernel_init (unused=<optimized out>) at init/main.c:1522
#30 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

## Block Device Operations

1. block/genhd.c 和 block/fops.c
    1. genhd.c 处理都是 genhd 这个结构体 : 和具体的驱动处理
    2. block_dev 处理的是 block_device 这个内容 : 似乎是用来和 vfs

```c
const struct file_operations def_blk_fops = {
  .open   = blkdev_open,
  .release  = blkdev_close,
  .llseek   = blkdev_llseek,
  .read_iter  = blkdev_read_iter,
  .write_iter = blkdev_write_iter,
  .iopoll   = iocb_bio_iopoll,
  .mmap   = generic_file_mmap,
  .fsync    = blkdev_fsync,
  .unlocked_ioctl = blkdev_ioctl,
#ifdef CONFIG_COMPAT
  .compat_ioctl = compat_blkdev_ioctl,
#endif
  .splice_read  = generic_file_splice_read,
  .splice_write = iter_file_splice_write,
  .fallocate  = blkdev_fallocate,
};
```

```c
static const struct block_device_operations sd_fops = {
  .owner      = THIS_MODULE,
  .open     = sd_open,
  .release    = sd_release,
  .ioctl      = sd_ioctl,
  .getgeo     = sd_getgeo,
  .compat_ioctl   = blkdev_compat_ptr_ioctl,
  .check_events   = sd_check_events,
  .unlock_native_capacity = sd_unlock_native_capacity,
  .report_zones   = sd_zbc_report_zones,
  .get_unique_id    = sd_get_unique_id,
  .free_disk    = scsi_disk_free_disk,
  .pr_ops     = &sd_pr_ops,
};
```

从通用的 vfs 到达具体的 block device :
```txt
#0  sd_open (bdev=0xffff8880053b0000, mode=1207959582) at drivers/scsi/sd.c:1316
#1  0xffffffff816b6a5d in blkdev_get_whole (bdev=bdev@entry=0xffff8880053b0000, mode=mode@entry=1207959582) at block/bdev.c:672
#2  0xffffffff816b779d in blkdev_get_by_dev (dev=<optimized out>, mode=1207959582, holder=holder@entry=0xffff8881161a7700) at block/bdev.c:822
#3  0xffffffff816b79c0 in blkdev_get_by_dev (dev=<optimized out>, mode=<optimized out>, holder=holder@entry=0xffff8881161a7700) at block/bdev.c:856
#4  0xffffffff816b826b in blkdev_open (inode=<optimized out>, filp=0xffff8881161a7700) at block/fops.c:478
#5  0xffffffff813bfd97 in do_dentry_open (f=f@entry=0xffff8881161a7700, inode=0xffff888100c35258, open=0xffffffff816b8220 <blkdev_open>, open@entry=0x0 <fixed_percpu_data>) at fs/open.c:882
#6  0xffffffff813c1cdd in vfs_open (path=path@entry=0xffffc90040513dc0, file=file@entry=0xffff8881161a7700) at fs/open.c:1013
#7  0xffffffff813d8e4b in do_open (op=0xffffc90040513edc, file=0xffff8881161a7700, nd=0xffffc90040513dc0) at fs/namei.c:3557
#8  path_openat (nd=nd@entry=0xffffc90040513dc0, op=op@entry=0xffffc90040513edc, flags=flags@entry=65) at fs/namei.c:3714
#9  0xffffffff813da411 in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff888100785000, op=op@entry=0xffffc90040513edc) at fs/namei.c:3741
#10 0xffffffff813c1fd5 in do_sys_openat2 (dfd=-100, filename=<optimized out>, how=how@entry=0xffffc90040513f18) at fs/open.c:1310
#11 0xffffffff813c24d2 in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1326
#12 __do_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1342
#13 __se_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1337
#14 __x64_sys_openat (regs=<optimized out>) at fs/open.c:1337
#15 0xffffffff8217fc5c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90040513f58) at arch/x86/entry/common.c:50
#16 do_syscall_64 (regs=0xffffc90040513f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#17 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

初始化的时候，系统也会使用:
```txt
#0  sd_open (bdev=0xffff8880053b0000, mode=1) at drivers/scsi/sd.c:1316
#1  0xffffffff816b6a5d in blkdev_get_whole (bdev=bdev@entry=0xffff8880053b0000, mode=mode@entry=1) at block/bdev.c:672
#2  0xffffffff816b779d in blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>) at block/bdev.c:822
#3  0xffffffff816b79c0 in blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>) at block/bdev.c:856
#4  0xffffffff816d845b in disk_scan_partitions (disk=disk@entry=0xffff8880054c6000, mode=mode@entry=1, owner=owner@entry=0x0 <fixed_percpu_data>) at block/genhd.c:374
#5  0xffffffff816d8822 in device_add_disk (parent=parent@entry=0xffff88800567a1b8, disk=disk@entry=0xffff8880054c6000, groups=groups@entry=0x0 <fixed_percpu_data>) at block/genhd.c:502
#6  0xffffffff81af740d in sd_probe (dev=0xffff88800567a1b8) at drivers/scsi/sd.c:3536
#7  0xffffffff81a75620 in call_driver_probe (drv=0xffffffff82e25f00 <sd_template>, dev=0xffff88800567a1b8) at drivers/base/dd.c:560
#8  really_probe (dev=dev@entry=0xffff88800567a1b8, drv=drv@entry=0xffffffff82e25f00 <sd_template>) at drivers/base/dd.c:639
#9  0xffffffff81a756dd in __driver_probe_device (drv=0xffffffff82e25f00 <sd_template>, dev=dev@entry=0xffff88800567a1b8) at drivers/base/dd.c:778
#10 0xffffffff81a75769 in driver_probe_device (drv=<optimized out>, dev=dev@entry=0xffff88800567a1b8) at drivers/base/dd.c:808
#11 0xffffffff81a75b3e in __driver_attach_async_helper (_dev=0xffff88800567a1b8, cookie=<optimized out>) at drivers/base/dd.c:1126
#12 0xffffffff8115c6bc in async_run_entry_fn (work=0xffff8880054baec0) at kernel/async.c:127
#13 0xffffffff8114bd54 in process_one_work (worker=worker@entry=0xffff888003c5ba80, work=0xffff8880054baec0) at kernel/workqueue.c:2289
#14 0xffffffff8114bf7c in worker_thread (__worker=0xffff888003c5ba80) at kernel/workqueue.c:2436
#15 0xffffffff811546c4 in kthread (_create=0xffff8880046f0040) at kernel/kthread.c:376
#16 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

https://askubuntu.com/questions/780715/how-to-check-serial-number-of-nvme-disk

## 结束语
好的，你现在对于 Linux 如何处理 Block 设备有了一个大概的认识，记得奖励自己一把英雄联盟哦。

## TODO

- [【硬件科普】硬盘的 SATA M.2 NGFF NVME 是什么意思，详解硬盘的总线协议与接口](https://www.bilibili.com/video/BV1Qv411t7ZL)
- [【硬件科普】固态硬盘的缓存是干什么的？有缓存和无缓存有什么区别？](https://www.bilibili.com/video/BV1aF411u7Ct)
- [【装机教程】全网最好的装机教程，没有之一](https://www.bilibili.com/video/BV1BG4y137mG)


- 数据传输协议: IDE AHCI NVMe SCSI
- 总线: SATA PCIe SAS
- 硬盘接口: SATA mSATA(只是缩小体积) SATA express(提升一倍的速度) M.2 PCIe U.2 SAS

看 ahci_qc_issue 的存储协议也是使用的 scsi
```txt
@[
    ahci_qc_issue+1
    ata_qc_issue+265
    __ata_scsi_queuecmd+521
    ata_scsi_queuecmd+87
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
    read_pages+444
    page_cache_ra_unbounded+293
    force_page_cache_ra+197
    filemap_get_pages+233
    filemap_read+185
    blkdev_read_iter+176
    vfs_read+513
    ksys_read+95
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 41
```

## TODO
- https://news.ycombinator.com/item?id=33455052
	- https://0pointer.net/blog/linux-boot-partitions.html
- https://opensource.com/article/18/7/how-use-dd-linux

## 记录一个宝藏脚本
```sh
	sudo parted $disk -- mklabel gpt
	sudo parted $disk -- mkpart primary xfs 1MiB 50%
	sudo parted $disk -- mkpart primary xfs 50% -1MiB
```

## 先安装一个 linux 到物理机上吧
https://github.com/pbatard/rufus

也许和 ventoy 对比一下

## 扩展一个分区和文件系统

如果恰好是 vda4 恰好是最后一个分区，那么可以:
```sh
sudo yum install cloud-utils-growpart -y
sudo growpart /dev/vda 4
sudo resize2fs /dev/vda4 # ext4
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
