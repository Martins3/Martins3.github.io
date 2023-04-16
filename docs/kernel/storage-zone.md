# Zoned block devices

## https://zonedstorage.io/
看上去是西部数据在推动这个事情

## 关联的文件
- drivers/scsi/sd_zbc.c
- block/blk-zoned.c
- drivers/nvme/host/zns.c
- drivers/md/dm-zoned 之类的好几个文件

- https://www.kernel.org/doc/html/latest/filesystems/zonefs.html
- zoned-block-devices.rst

## 可以看看的文章: Advanced Zoned Namespace Interface for Supporting In-Storage Zone Compaction

## https://www.qemu.org/2022/11/17/zoned-emulation/ : 只是一个尝试
