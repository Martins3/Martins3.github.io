- [IO 子系统全流程介绍](https://zhuanlan.zhihu.com/p/545906763)

## 一些收集

https://www.techtarget.com/searchstorage/definition/host-bus-adapter
https://en.wikipedia.org/wiki/Host_adapter


## scheduler 队列都是放到什么位置的?

## blk cgroup 之类的是放到什么位置的?

## 才意识到 blk_update_request 是一个关键节点

- blk_update_request
  - req_bio_endio



## queue_is_mq() 的那些盘，是不是创建出来的时候就是如此设置的，还是说后期可以调整

## md 的元数据是存放到哪里的

## 如果 guest 参差和 host 层次都是使用 aio，那么是不是将内容写入到操作系统中，从头到尾都是无需任何复制的



## 盘的 htcx 是否共用是看是不是共享的 host

13900k 上的 sda 和 sdb 两个 sata 盘不是一样的:
```txt
lrwxrwxrwx  1 root root 0 Aug 16 13:38 sda -> ../devices/pci0000:00/0000:00:17.0/ata7/host6/target6:0:0/6:0:0:0/block/sda
lrwxrwxrwx  1 root root 0 Aug 16 11:33 sdb -> ../devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/block/sdb
lrwxrwxrwx  1 root root 0 Aug 16 13:38 sdc -> ../devices/pseudo_0/adapter0/host8/target8:0:0/8:0:0:0/block/sdc
```

## 发现了一个奇怪的驱动，在 drivers/block 下唯一的硬件驱动
drivers/block/mtip32xx/mtip32xx.c

http://www.storagereview.com/micron_p420m_enterprise_pcie_ssd_review

https://www.storagereview.com/review/micron-p420m-enterprise-pcie-ssd-review

那么这个驱动，在 /dev/ 下的盘的名称是什么?

找到他在 windows 中驱动在哪里?

## 一个有趣的商业模式
drivers/block/drbd/

这个模块是这个公司控制的:
https://linbit.com/drbd/

https://ubuntu.com/server/docs/distributed-replicated-block-device-drbd


## 这三个工具是开源的吗？
- hdparm
- storcli64
- Arcconf : https://support.huawei.com/enterprise/en/doc/EDOC1100048773/919d6284/downloading-and-installing-arcconf

## 这两个东西是组合使用的吧，似乎已经不维护了
drivers/scsi/libiscsi.c
drivers/scsi/libiscsi_tcp.c

这个还是维护的:
https://github.com/open-iscsi/open-iscsi

但是为什么 open-iscsi 的代码量是 libiscsi 的 10 倍啊?

## htop 的 IO tab 可以检查每一个 process 的 IO 情况
这是是从哪一个接口读到的

## 可以看看这个公众号的其他东西
https://mp.weixin.qq.com/s?__biz=MzkyMDU5NDI3OQ%3D%3D&chksm=c1913022f6e6b934058e97433c1ae2034b46d5b2f54cb1ea42d264b317ec5287aa63ee143b60&idx=1&mid=2247483946&scene=21&sn=835a1def63acde1e2486e5a9c5f7a212
https://zhuanlan.zhihu.com/p/669385346

## blk-cgroup.c 中内容测试下

是所有的 blk 相关的 cgroup 的内容都是实现在这里吗?

https://mp.weixin.qq.com/s/fmCXKpZ46k1X4xLRNYI9dw

## qcow2 文件，如果不用 direct io ，用一会就会有全部 io 都是在搞 page cache
和这个有关系吗?
https://zhuanlan.zhihu.com/p/7485748615

https://lore.kernel.org/linux-fsdevel/20241114152743.2381672-2-axboe@kernel.dk/T/#mafc1fbe4fcbbd0af41e48388bb14d966b39def71

## 想不到 qemu 的 trim 的确有用的

展示文件的大小，可以看到为:
```txt
.rw-r--r--  369G martins3 23 Dec 12:16   1.qcow2
```

使用 du -h 可以看到:
233G    .

有趣的文件系统功能。

## qemu 似乎不太可以模拟出来多个盘的场景来

如果这个盘下面真的有数据，使用 10 个盘的总性能和一个盘的总性能其实差不多:
```txt
[global]
time_based
runtime=1000
ioengine=io_uring
# ioengine=mmap
ioengine=aio
iodepth=64
direct=1
numjobs=8
bs=4k

[trash]
rw=randread
filename=/dev/vda:/dev/vdb:/dev/vdc:/dev/vdd:/dev/vde:/dev/vdf:/dev/vdg:/dev/vdh:/dev/vdi:/dev/vdj:/dev/vdk:/dev/vdl
filename=/dev/vda
```
都是只有 200k 左右，还是看看 spdk 之后的性能是多少吧

应该是没有使用 iothread 吧发

## 想不到 iostat -xz 1 直接内容都可以没有，如果把 /sys/block/nvme1n1/queue/iostats 配置为 0 ，那么 iostsat -xz 1 中的内容直接为 0

## 开发一个将 block lazyer 各个 queue 的数值展示出来

## 这个是做什么的
drivers/nvme/target/pci-epf.c

## loop device 也相当于一个 block 设备

例如:
```c
static blk_status_t loop_queue_rq(struct blk_mq_hw_ctx *hctx,
		const struct blk_mq_queue_data *bd)
```
所以，也可以给 loop device 配置队列等等之类的东西了

## 看看
https://www.zhihu.com/question/43687427/answer/123682560

## docs/kernel/sched/iowait.md


## https://lwn.net/Articles/1009298/

## fio 中的这些引擎

```txt
 fio-engine-dev-dax     x86_64  3.37-4.fc42             fedora         19.6 KiB
 fio-engine-http        x86_64  3.37-4.fc42             fedora         44.4 KiB
 fio-engine-libaio      x86_64  3.37-4.fc42             fedora         35.5 KiB
 fio-engine-libpmem     x86_64  3.37-4.fc42             fedora         19.6 KiB
 fio-engine-nbd         x86_64  3.37-4.fc42             fedora         22.6 KiB
 fio-engine-rados       x86_64  3.37-4.fc42             fedora         34.0 KiB
 fio-engine-rbd         x86_64  3.37-4.fc42             fedora         32.6 KiB
 fio-engine-rdma        x86_64  3.37-4.fc42             fedora         39.2
```
fio-engine-rados 和 fio-engine-rbd 有什么区别?

## 有趣的东西
[Supporting untorn buffered writes](https://mp.weixin.qq.com/s/6vxnpAxQR-jmeGR96tdGLA)


## 看看
- https://github.com/Alluxio/alluxio
  - https://www2.eecs.berkeley.edu/Pubs/TechRpts/2018/EECS-2018-29.html

- bluestore : 每天都在听说 bluestore
  - https://paper-notes.zhjwpku.com/storage/bluestore.html

## 资料

- https://hexancer.github.io/StorageX/others/howresearch.html
- https://haslab.org/ : 右侧有一些论文阅读笔记，还是不错的
- https://haslab.org/docs/research/howtoread.html

## 这个做什么的?
```txt
BFQ I/O SCHEDULER
M:	Yu Kuai <yukuai3@huawei.com>
L:	linux-block@vger.kernel.org
S:	Odd Fixes
F:	Documentation/block/bfq-iosched.rst
F:	block/bfq-*
```

## 这里的警告是什么意思

```c
struct block_device *blkdev_get_no_open(dev_t dev, bool autoload)
{
	struct block_device *bdev;
	struct inode *inode;

	inode = ilookup(blockdev_superblock, dev);
	if (!inode && autoload && IS_ENABLED(CONFIG_BLOCK_LEGACY_AUTOLOAD)) {
		blk_request_module(dev);
		inode = ilookup(blockdev_superblock, dev);
		if (inode)
			pr_warn_ratelimited(
"block device autoloading is deprecated and will be removed.\n");
	}
	if (!inode)
		return NULL;

	/* switch from the inode reference to a device mode one: */
	bdev = &BDEV_I(inode)->bdev;
	if (!kobject_get_unless_zero(&bdev->bd_device.kobj))
		bdev = NULL;
	iput(inode);
	return bdev;
}
```

## 看看真正的深入分析
https://mp.weixin.qq.com/s/zV0xii0xd56xNdtVRlut2g?click_id=16

## 在 hyper-v 上，一个盘一个 host bridge
```txt
🧀  ls -la /sys/block
lrwxrwxrwx - root 27 Sep 00:17 dm-0 -> ../devices/virtual/block/dm-0
lrwxrwxrwx - root 27 Sep 00:17 sda -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/565b810e-cd29-44f1-8135-81d820149f29/host0/target0:0:0/0:0:0:0/block/sda
lrwxrwxrwx - root 27 Sep 00:17 sdb -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/906436cf-9de6-4cbd-a7ff-49e04666f668/host1/target1:0:0/1:0:0:1/block/sdb
lrwxrwxrwx - root 27 Sep 00:17 sr0 -> ../devices/LNXSYSTM:00/LNXSYBUS:00/ACPI0004:00/VMBUS:00/565b810e-cd29-44f1-8135-81d820149f29/host0/target0:0:0/0:0:0:1/block/sr0
vn on  master 🐉
🧀  lsscsi
[0:0:0:0]    disk    Msft     Virtual Disk     1.0   /dev/sda
[0:0:0:1]    cd/dvd  Msft     Virtual DVD-ROM  1.0   /dev/sr0
[1:0:0:1]    disk    Msft     Virtual Disk     1.0   /dev/sdb
```

## 有趣的
https://news.ycombinator.com/item?id=45595724

## zpool 是做什么的
https://github.com/morbidrsa/zloopctl

## 这里的设备都认识吗?
https://www.broadcom.cn/products/storage

## 原来盘可以容忍一些扇区的

<p align="center">
  <img src="https://nvmexpress.org/wp-content/uploads/Linux-storage-stack-diagram_v4.10-e1575939041721.png" alt="drawing" align="center"/>
</p>
<p align="center">
from https://nvmexpress.org/education/drivers/linux-driver-information/
</p>


```txt
unused devices: <none>
[root@localhost 12:47:33 ~]# [ 3033.717755] C51 blk_update_request: protection error, dev nvme10n1, sector 2064 op 0x1:(WRIT E) flags 0x10800 phys_seg 1 prio class 0
[ 3033.732764] C51 md: super_written gets error=-84
[ 3033.739255] C51 md/raid1:md127: Disk failure on nvme10n1p1, disabling device.
[ 3033.739255] C51 md/raid1:md127: Operation continuing on 1 devices.
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
