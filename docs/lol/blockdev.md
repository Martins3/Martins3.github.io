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

## block layer
<p align="center">
  <img src="https://nvmexpress.org/wp-content/uploads/Linux-storage-stack-diagram_v4.10-e1575939041721.png" alt="drawing" align="center"/>
</p>
<p align="center">
from https://nvmexpress.org/education/drivers/linux-driver-information/
</p>


## ldd3 的驱动理解

- [ ] register_blkdev , add_disk, get_gendisk
  - [x] register_blkdev 注册了 major number 在 `major_names` 中间，但是 `major_names` 除了使用在 blkdev_show (cat /proc/devices) 之外没有看到其他的用处
    - https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html : 中说，register_blkdev 是会取消掉的
    - 从 virtio_blk.c 中间来看 : major 放到局部变量中间，所以实际功能是分配 major number

- [ ] 似乎很多的初始化工作是放到了 init 中间了

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
