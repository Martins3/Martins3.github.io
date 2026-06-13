## bio-based 和 request-based 的区分
<!-- cae10a29-6410-4aae-81de-232a8a8576d1 -->

(这个没写清楚，不知道当时想要表达什么)

两个非 mq 的创建:
- kernfs_fop_write_iter
  - module_attr_store
    - param_attr_store
      - add_named_array
        - md_alloc_and_put
          - md_alloc
            - __blk_alloc_disk
              - blk_alloc_queue

```txt
@[
        blk_alloc_queue+0
        dm_create+520
        dev_create+92
        ctl_ioctl+632
        dm_ctl_ioctl+24
        __arm64_sys_ioctl+1396
        invoke_syscall.constprop.0+88
        do_el0_svc+72
        el0_svc+92
        el0t_64_sync_handler+268
        el0t_64_sync+408
]: 1
```

而 virtio-blk
```txt
blk_alloc_queue+0
__blk_mq_alloc_disk+40
virtblk_probe+1164
virtio_dev_probe+400
really_probe+184
```

- queue_is_mq 的判断 request_queue::mq_ops ，如果是 multiqueue 的时候，
从 blk_mq_init_allocated_queue 中，会来初始化：

blk_mq_init_allocated_queue -> __blk_mq_alloc_disk


提交是从 `__submit_bio` 开始的，观察 raid1 的实现，发现根本不会制作 request ，直接处理 bio 了

https://lwn.net/Articles/736534/ 中绘制这个图已经过时了:
![](https://static.lwn.net/images/2017/neil-blocklayer.png)

不过这里有一个问题可以继续看看，就是 request_queue 中的结构体基本上都是给 mq 用的
```c
struct request_queue {
	// ...
	const struct blk_mq_ops	*mq_ops;

	/* sw queues */
	struct blk_mq_ctx __percpu	*queue_ctx;

	/* hw dispatch queues */
	unsigned int		nr_hw_queues;
	struct xarray		hctx_table;
```

哪些内容是给 bio based 使用的

## 一个 request_queue 是对应一个盘的
<!-- 2ecb63da-efaf-4778-a8ba-fec1adac7718 -->

而不是一个分区的，不是一个 hba ，就是一个盘。

那么，这意味着什么?
1. 对于 HBA 卡这种，多个 request_queue 需要只想


从代码的跟踪来说:
`__blk_mq_alloc_requests` 中，因为 request_queue 可以知道软队列在那里，
根据软队列可以找到硬件队列:
```c
	data->ctx = blk_mq_get_ctx(q); // request_queue 中存储了 percpu 的 struct blk_mq_ctx
  data->hctx = blk_mq_map_queue(data->cmd_flags, data->ctx); // 通过 ctx 可以找到 hwctx
```

在 blk_mq_submit_bio 中调用 bdev_get_queue() 获取到 request_queue
```c
static inline struct request_queue *bdev_get_queue(struct block_device *bdev)
{
	return bdev->bd_queue;	/* this is never NULL */
}
```

这个 request_queue 的分配位置为 : blk_alloc_queue

但是从 blk_mq_init_allocated_queue 看，就是会构建两个
```txt
	if (blk_mq_alloc_ctxs(q))
		goto err_exit;
```

使用 yyds-fs 虚拟机继续分析下吧，感觉怎么现在共享一个 scsi HBA 的两个盘的只想的 hctx 都是不一样的。

cd /home/martins3/data/vn/docs/kernel/tutorial/drgn/drgn_analysis
 ./drgn-wrapper.sh ./blk_mq_relationship.py sdb sdc

## 虚拟盘和 block_device_operations

[A block layer introduction part 1: the bio layer](https://lwn.net/Articles/736534/)
More often the next layer is an intermediate layer such as for the virtual devices provided by md and dm.

```c
struct block_device_operations {
	void (*submit_bio)(struct bio *bio);
	int (*poll_bio)(struct bio *bio, struct io_comp_batch *iob,
			unsigned int flags);
      // ...
  }
```

在 `__submit_bio` 这里，request based 和 bio based 的盘
通过 BD_HAS_SUBMIT_BIO 来标注虚拟盘

```c
	if (!bdev_test_flag(bio->bi_bdev, BD_HAS_SUBMIT_BIO)) {
		blk_mq_submit_bio(bio); // 基于 mq 的
	} else if (likely(bio_queue_enter(bio) == 0)) {
    // ...
    disk->fops->submit_bio(bio); // 基于
  }
```

### `request_queue::mq_ops` 等价于  bdev_test_flag(bio->bi_bdev, BD_HAS_SUBMIT_BIO)

基本可以确定，request based 就是 mq ，而 bio based 就是另外一个。
```c
static int __add_disk(struct device *parent, struct gendisk *disk,
		      const struct attribute_group **groups,
		      struct fwnode_handle *fwnode)

{
	struct device *ddev = disk_to_dev(disk);
	int ret;

	if (WARN_ON_ONCE(bdev_nr_sectors(disk->part0) > BLK_DEV_MAX_SECTORS))
		return -EINVAL;

	if (queue_is_mq(disk->queue)) {
		/*
		 * ->submit_bio and ->poll_bio are bypassed for blk-mq drivers.
		 */
		if (disk->fops->submit_bio || disk->fops->poll_bio)
			return -EINVAL;
	} else {
		if (!disk->fops->submit_bio)
			return -EINVAL;
		bdev_set_flag(disk->part0, BD_HAS_SUBMIT_BIO);
	}
```

看看 queue_is_mq() 调用位置

- `bd_has_submit_bio` 是在 device_add_disk() 中初始化的
- fops 的赋值，例如在 `md_alloc` 中

```txt
#0  device_add_disk (parent=parent@entry=0x0 <fixed_percpu_data>, disk=disk@entry=0xffff88810bbf9400, groups=groups@entry=0x0 <fixed_percpu_data>) at block/genhd.c:399
#1  0xffffffff81d9b4bd in add_disk (disk=0xffff88810bbf9400) at ./include/linux/blkdev.h:735
#2  md_alloc (dev=<optimized out>, name=name@entry=0x0 <fixed_percpu_data>) at drivers/md/md.c:5694
#3  0xffffffff81d9b8cd in md_alloc_and_put (name=0x0 <fixed_percpu_data>, dev=<optimized out>) at drivers/md/md.c:5729
#4  add_named_array (val=0xffff888105115678 "md10", kp=<optimized out>) at drivers/md/md.c:5769
#5  0xffffffff8116e1e5 in param_attr_store (mattr=0xffff888108d59e70, mk=0xffff888105c5b660, buf=0xffff888105115678 "md10", len=4) at kernel/params.c:586
#6  0xffffffff8116d5ce in module_attr_store (kobj=0x60000, attr=0x0 <fixed_percpu_data>, buf=0x0 <fixed_percpu_data>, len=48) at kernel/params.c:920
#7  0xffffffff8150d73c in kernfs_fop_write_iter (iocb=0xffffc901f6533e98, iter=<optimized out>) at fs/kernfs/file.c:334
#8  0xffffffff81457d2a in call_write_iter (iter=0xffff88810bbf9400, kio=0xffffc901f6533e98, file=0xffff8881052dd900) at ./include/linux/fs.h:1956
```


```c
	struct request_queue *q = bdev_get_queue(bio->bi_bdev);
```
bdev_alloc 中进行的赋值

```txt
#0  bdev_alloc (disk=disk@entry=0xffff8881097e7800, partno=partno@entry=1 '\001') at block/bdev.c:386
#1  0xffffffff817f794f in add_partition (disk=disk@entry=0xffff8881097e7800, partno=partno@entry=1, start=2048, len=1024000, flags=0, info=0xffffc900009d9095) at block/partitions/core.c:340
#2  0xffffffff817f80bf in blk_add_partition (p=1, state=0xffff88810a907000, disk=0xffff8881097e7800) at block/partitions/core.c:570
#3  blk_add_partitions (disk=0xffff8881097e7800) at block/partitions/core.c:640
#4  bdev_disk_changed (disk=disk@entry=0xffff8881097e7800, invalidate=invalidate@entry=false) at block/partitions/core.c:685
#5  0xffffffff817d1f6a in blkdev_get_whole (bdev=bdev@entry=0xffff888100613200, mode=mode@entry=1) at block/bdev.c:653
#6  0xffffffff817d2ab4 in blkdev_get_by_dev (dev=<optimized out>, mode=<optimized out>, mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>, hops=hops@entry=0x0 <fixed_percpu_data>) at block/bdev.c:795
#7  0xffffffff817d2ccc in blkdev_get_by_dev (dev=<optimized out>, mode=mode@entry=1, holder=holder@entry=0x0 <fixed_percpu_data>, hops=hops@entry=0x0 <fixed_percpu_data>) at block/bdev.c:829
#8  0xffffffff817f41d9 in disk_scan_partitions (disk=disk@entry=0xffff8881097e7800, mode=mode@entry=1) at block/genhd.c:369
#9  0xffffffff817f5bab in device_add_disk (parent=parent@entry=0xffff8881097bdc10, disk=0xffff8881097e7800, groups=groups@entry=0xffffffff83057bf0 <virtblk_attr_groups>)
    at block/genhd.c:510
```
block_device 中的 queue 来自于 gendisk

- `__blk_alloc_disk` : 调用者和那些注册 submmit_bio 的用户正好在一起
- `__blk_mq_alloc_disk`  / blk_mq_alloc_disk_for_queue : 在 mq 中创建，自动携带了，从 tag_set 中获取到 mq_ops


对于 rq-based 的，的确是如此:
https://lore.kernel.org/all/bawtcsifeew7jtinckwxfrg7bach366uoccecfc5v56xmdhqsn@kj72oenu5j2w/

但是对于 __submit_bio 中可以看到，对于 md 这种非 rq-based ，这是不可以的


## loop 设备居然是一个 mq 设备 !
/sys/block/loop0/mq:
drwxr-xr-x - root  1 Aug 20:19 0

/sys/devices/virtual/block/loop0/queue/nr_requests

```c
static const struct blk_mq_ops loop_mq_ops = {
	.queue_rq       = loop_queue_rq,
	.complete	= lo_complete_rq,
};
```

### 那岂不是 loop 还支持 scheduler 吗?


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
