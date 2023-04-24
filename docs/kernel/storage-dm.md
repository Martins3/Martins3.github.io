# device mapper

## 基本使用
https://support.huawei.com/enterprise/zh/doc/EDOC1100154491/556f7079

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
vgcreate mm /dev/nvme1n1 /dev/nvme0n1

pvs
vgs
pvscan
lvscan
pvdisplay

# 增加新的盘
vgextend /dev/mm /dev/vdb

lvcreate -L +250G --name a mm

mkfs.ext4 /dev/mm/a
mount /dev/mm/a mnt

lvreduce -L -100M /dev/mm/a

vgremove mm
```

## 问题
最近安装 Ubunut server 作为 root，其启动参数如下，
```sh
root=/dev/mapper/ubuntu--vg-ubuntu--lv
```
这种模式，常规的切换内核是没有办法的，还是因为当时内核没有装上？

## 内核代码简单分析一下
### [](https://docs.kernel.org/admin-guide/device-mapper/index.html#)
