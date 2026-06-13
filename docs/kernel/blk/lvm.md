# device mapper

## 修复 lvm

万万没有想到，居然可以使用 fsck 来修复一个重启之后的 lvm .

![](./img/lvm-fix.png)

## 基本使用
https://support.huawei.com/enterprise/zh/doc/EDOC1100154491/556f7079

## 如何理解 LV
参考 https://www.cyberciti.biz/faq/howto-add-disk-to-lvm-volume-on-linux-to-increase-size-of-pool/
- Physical Volumes (PV) – Actual disks (e.g. /dev/sda, /dev,sdb, /dev/vdb and so on)
- Volume Groups (VG) – Physical volumes are combined into volume groups. (e.g. my_vg = /dev/sda + /dev/sdb.)
- Logical Volumes (LV) – A volume group is divided up into logical volumes (e.g. my_vg divided into my_vg/data, my_vg/backups, my_vg/home, my_vg/mysqldb and so on)

LV 和 partion 是一个概念吗?

## lvm 会导致性能下降多少?

## lvm2 是做啥的

## device mapper 还可以提供什么功能

## dm
- [ ] device-mapper 和 lvm2 是什么关系 ?


- https://en.wikipedia.org/wiki/Logical_Volume_Manager_(Linux)
- https://wiki.archlinux.org/index.php/LVM : 卷 逻辑卷 到底是啥 ?

## 基本教程

```txt
total 0
drwxr-xr-x.  2 root root     100 Apr 23 21:56 .
drwxr-xr-x. 18 root root    3.5K Apr 23 21:56 ..
crw-------.  1 root root 10, 236 Apr 23 21:51 control
lrwxrwxrwx.  1 root root       7 Apr 23 21:56 LVMvgTEST-bbbb -> ../dm-1
lrwxrwxrwx.  1 root root       7 Apr 23 21:51 LVMvgTEST-Stuff -> ../dm-0
```

## 可以 sdd 和 hdd 混合在一起吗?

## 常用命令

- lvdisplay
- pvs

```sh
vgcreate hi /dev/nvme1n1 /dev/nvme0n1

pvs
lvs
vgs

pvscan
lvscan
vgscan

pvdisplay
lvdisplay
vgdisplay

# 增加新的盘
vgextend /dev/mm /dev/vdb

lvcreate -L +250G --name A hi # 创建逻辑卷，这个就是直接使用的了
lvreduce -L -100M /dev/hi/A
lvextend -l +100%FREE /dev/hi/A

vgremove mm
```

## 将一个盘被 lvm 纳管
```txt
disk=/dev/vda
sudo pvcreate $disk
sudo vgcreate martins3 $disk
sudo lvcreate -L 1G -n data martins3
```

## 如何添加盘到根文件系统中
```txt
NAME               MAJ:MIN RM   SIZE RO TYPE MOUNTPOINTS
sr0                 11:0    1  1024M  0 rom
vda                252:0    0   400G  0 disk
├─vda1             252:1    0     1M  0 part
├─vda2             252:2    0     1G  0 part /boot
└─vda3             252:3    0   399G  0 part
  ├─openeuler-root 253:0    0    70G  0 lvm  /
  ├─openeuler-swap 253:1    0   7.8G  0 lvm  [SWAP]
  └─openeuler-home 253:2    0 321.2G  0 lvm  /home
vdb                252:16   0   7.8T  0 disk
```

```txt
# 1. 创建物理卷
sudo pvcreate /dev/vdb

# 2. 扩展卷组
sudo vgextend openeuler /dev/vdb

# 3. 扩展逻辑卷（使用所有空闲空间）
sudo lvextend -l +100%FREE /dev/openeuler/home

# 4. 扩展文件系统（根据类型选择）
# ext4:
sudo resize2fs /dev/openeuler/home
# 或 xfs:
# sudo xfs_growfs /home
```
### 增大一个 lv
```sh
sudo lvextend -l +100%FREE /dev/mapper/ubuntu--vg-ubuntu--lv
# 如果是 ext4
sudo resize2fs /dev/mapper/ubuntu--vg-ubuntu--lv
# 如果是 xfs
sudo xfs_growfs -d /dev/vg-group-name/lv-name

sudo lvextend -l +100%FREE /dev/mapper/fedora-root
sudo xfs_growfs -d /dev/mapper/fedora-root

```

https://serverfault.com/questions/1063890/how-to-extend-the-size-of-a-xfs

### [ ] 缩小一个 lv 的大小
发现 / 依旧用满了，现在需要移动一些 /home 下的空间到 / 中:
```txt
NAME               MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
sda                  8:0    0  1.8T  0 disk
├─sda1               8:1    0    1M  0 part
├─sda2               8:2    0    1G  0 part /boot
└─sda3               8:3    0  1.8T  0 part
  ├─openeuler-root 252:0    0   70G  0 lvm  /
  ├─openeuler-swap 252:1    0  7.9G  0 lvm  [SWAP]
  └─openeuler-home 252:2    0  1.7T  0 lvm  /home
```
计划这么操作，但是发现 resize2fs 的文件系统的时候，
文件系统无法动态 reduce 的。
```txt
sudo resize2fs /dev/mapper/openeuler-home 1000G
lvresize --size +10G /dev/mapper/system-root
```


### 如何动态增加盘到 lv 中

ubuntu 默认设置是 100G，但是 partion 却
- https://packetpushers.net/ubuntu-extend-your-default-lvm-space/

- 步骤 1 : 扩展一个盘
  - sudo pvcreate /dev/vdb
  - sudo vgextend ubuntu-vg /dev/vdb

- 步骤 2 :  扩展文件系统
  - sudo lvextend -l +100%FREE /dev/ubuntu-vg/ubuntu-lv
  - sudo resize2fs /dev/mapper/ubuntu--vg-ubuntu--lv


```txt
$ sudo lvs
  LV        VG        Attr       LSize    Pool Origin Data%  Meta%  Move Log Cpy%Sync Convert
  ubuntu-lv ubuntu-vg -wi-ao---- <798.00g
```

## 内核代码简单分析一下
### [kernel doc : Device Mapper](https://docs.kernel.org/admin-guide/device-mapper/index.html#)


## 这个写的好一点 ?
https://wiki.gentoo.org/wiki/Device-mapper

## dm-thin 是做啥的?

## dm raid

```sh
sudo mdadm --create /dev/md0 --level=1 --raid-devices=2 /dev/sda1 /dev/sdb1 # 创建一个 raid
sudo dmsetup create myraid --table "0 $(blockdev --getsize /dev/md0) raid1 /dev/md0 0" # 这个错误无法找到啊
```

```c
static struct target_type mirror_target = {
	.name	 = "mirror",
	.version = {1, 14, 0},
	.module	 = THIS_MODULE,
	.ctr	 = mirror_ctr,
	.dtr	 = mirror_dtr,
	.map	 = mirror_map,
	.end_io	 = mirror_end_io,
	.presuspend = mirror_presuspend,
	.postsuspend = mirror_postsuspend,
	.resume	 = mirror_resume,
	.status	 = mirror_status,
	.iterate_devices = mirror_iterate_devices,
};
```

参考:
- https://superuser.com/questions/721795/how-fake-raid-communicates-with-operating-systemlinux : 关键总结
- https://www.jinbuguo.com/storage/raid_types.html
- https://skrypuch.com/raid/ : 三种 raid 的简单总结
- https://datahunter.org/fakeraid : 更加细节的使用

> Fake RAID communicates with Linux through device-mapper. There are several drivers involved in this (dm-stripe, dm-raid) and useful utility dmraid.


看上去，实际上这个东西没有什么人用。

- https://en.wikipedia.org/wiki/Standard_RAID_levels

## raid0

超级简单，但是除了一个稍微复杂点的 `raid0_takeover`

## raid10

准备四个 disk，首先两两组建为 raid1，然后将两个 raid1 组件为 raid0

[is-there-a-difference-between-raid-10-10-and-raid-01-01](https://serverfault.com/questions/145319/is-there-a-difference-between-raid-10-10-and-raid-01-01)

之所以不推荐 raid 0 + 1 ，是因为
1. 加入 4 个盘
2. 如果单盘坏掉，都无所谓。
3. 如果 3 个或者 4 坏掉，都挂掉。
4. 如果只是坏掉两个，一共六种情况，raid10 在两种情况中会坏掉，而 raid01 是 4 种。

当然，这里的前提是最多 raid0 认为其下的盘坏一个，整个 raid0 就报废了。

## 测试下 growpart
growpart + xfs_growfs


## 实在是没想到，居然可以通过 lvm 实现 disk cache
https://quantum5.ca/2025/05/11/fast-cheap-bulk-storage-using-lvm-to-cache-hdds-on-ssds/

## dm-vdo 是这个作用吗?
drivers/md/dm-vdo/ refers to the Linux kernel source code directory for dm-vdo (Device Mapper Virtual Data Optimizer), a block device target providing data deduplication and compression to save storage space, originally from Permabit and now part of the mainline kernel, used for creating thin-provisioned, efficient storage pools with userspace tools like vdo

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
