## mq 的 queue 的数量就是 hctx 的数量
<!-- c44895fe-6562-4f63-93a5-7a974d0fc578 -->

qemu 会根据 CPU 数量设置队列数量:
```c
static void virtio_blk_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    VirtIOBlkPCI *dev = VIRTIO_BLK_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&dev->vdev);
    VirtIOBlkConf *conf = &dev->vdev.conf;

    if (conf->num_queues == VIRTIO_BLK_AUTO_NUM_QUEUES) {
        conf->num_queues = virtio_pci_optimal_num_queues(0);
    }

    if (vpci_dev->nvectors == DEV_NVECTORS_UNSPECIFIED) {
        vpci_dev->nvectors = conf->num_queues + 1;
    }

    qdev_realize(vdev, BUS(&vpci_dev->bus), errp);
}
```

在 virtio_blk 驱动中，从 qemu 那里获取到 num_queues
```c
static int init_vq(struct virtio_blk *vblk)
{
	int err;
	unsigned short i;
	vq_callback_t **callbacks;
	const char **names;
	struct virtqueue **vqs;
	unsigned short num_vqs;
	unsigned short num_poll_vqs;
	struct virtio_device *vdev = vblk->vdev;
	struct irq_affinity desc = { 0, };

	err = virtio_cread_feature(vdev, VIRTIO_BLK_F_MQ,
				   struct virtio_blk_config, num_queues,
				   &num_vqs);
	if (err)
		num_vqs = 1;
```


在 virtblk_probe 中，根据 num_queues 初始化硬件队列:
```c

	vblk->tag_set.nr_hw_queues = vblk->num_vqs;
```

## 如何获取 nvme 硬件队列数量
<!-- 9afa1826-2c74-44b5-889b-577da2562ec4 -->

参考: https://news.ycombinator.com/item?id=28706762

> Samsung's current flagship 980 PRO consumer drive supports 128 queues, but the previous generation (970 PRO/EVO/EVO Plus) only supported 32.
Their first two generations were limited to 8 and 7 queues. I wouldn't be surprised if these limits also applied to their entry-level enterprise SSDs that used the same controllers.

```txt
🤒  sudo nvme get-feature /dev/nvme0n1 -f 7
get-feature:0x07 (Number of Queues), Current value:0x00070007 # TiPro7100  # 等待确认

🧀  sudo nvme get-feature /dev/nvme1n1  -f 7
get-feature:0x07 (Number of Queues), Current value:0x00400040 # TiPro7000  # 32 个 hctx0

🧀  sudo nvme get-feature /dev/nvme2n1  -f 7
get-feature:0x07 (Number of Queues), Current value:0x000f000f # MAXIO 1602 # 16 个 hctx0
```
从软件上来说，就是 hctx = min(硬件队列的数量，CPU 数量)

```txt
🧀  sudo nvme get-feature /dev/nvme1n1 -f 7
get-feature:0x07 (Number of Queues), Current value:0x003e003e

🧀  lspci -nn | grep -i non
c8:00.0 Non-Volatile memory controller [0108]: PETAIO INC PETA8118 NVMe SSD Series [1ee4:1180] (rev 01)

🧀  cat /proc/interrupts | grep -i nvme | wc -l
60
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

Submission Queues 数量：
NSQ = 0x3e + 1 = 63
Completion Queues 数量：
NCQ = 0x3e + 1 = 63

所以，这都是完全符合预期的。

## /sys/block/*/inflight
<!-- 906c62f6-2c58-430e-a43d-2c854669778b -->

非常确认了，这个就是发送给设备的

每一个 block 设备都存在，但是为什么不是 queue 中的:
```sh
cat /sys/block/sda/inflight
```

```c
struct mq_inflight {
	struct block_device *part;
	unsigned int inflight[2];
};
```

对应的 backtrace 为:
```txt
#0  blk_mq_in_flight_rw (q=0xffff8881047a14a0, part=part@entry=0xffff888101a0ce00, inflight=inflight@entry=0xffffc900400a3db8) at block/blk-mq.c:156
#1  0xffffffff816d862a in part_inflight_show (dev=0xffff888101a0ce48, attr=<optimized out>, buf=0xffff888145862000 "") at block/genhd.c:1009
#2  0xffffffff81a6d9e8 in dev_attr_show (kobj=<optimized out>, attr=0xffffffff82df5740 <dev_attr_inflight>, buf=<optimized out>) at drivers/base/core.c:2196
#3  0xffffffff8146dc8b in sysfs_kf_seq_show (sf=0xffff8881460b1258, v=<optimized out>) at fs/sysfs/file.c:59
```

- inflight 连续出现了多次 bugfix ，从中是可以看到 mq 的大致工作内容

- commit a926c7afffcc ("block: Consider only dispatched requests for inflight statistic")
  - 之前的统计是错误，inflight 就是提交给硬件的，也就是 blk_mq_start_request 给 MQ_RQ_IN_FLIGHT 的位置才开始算起
  - 而之前统计是按照 hardware tag 被分配那么就是 inflight。是否存在 scheduler ，hardware 的 tag 创建时间不同
- commit b0d97557ebfc ("block: fix inflight statistics of part0")
  - 修复只能统计分区，无法统计整个盘的问题
- commit b81c14ca14b6 ("blk-mq: do not update io_ticks with passthrough requests")
  - 不要统计 flush or passthrough requests

## /sys/block/*/queue/nr_requests 的含义
<!-- 07c13417-2f22-4bae-900e-508c97852fbd -->

nr_requests 这个盘的一个硬件队列接受多少个请求，
所以，当使用了 scheduler ，那么 nr_requests 就去操控 scheduler
当没有使用 nr_requests ，那么就作用于硬件队列的大小

Documentation/ABI/stable/sysfs-block
```txt
nr_requests (RW)
----------------
This controls how many requests may be allocated in the block layer for
read or write requests. Note that the total allocated number may be twice
this amount, since it applies only to reads or writes (not the accumulated
sum).

To avoid priority inversion through request starvation, a request
queue maintains a separate request pool per each cgroup when
CONFIG_BLK_CGROUP is enabled, and this parameter applies to each such
per-block-cgroup request pool.  IOW, if there are N block cgroups,
each request queue may have up to N request pools, each independently
regulated by nr_requests.
```

更新这个字段的位置为 blk_mq_update_nr_requests

### cmd_per_lun

看上去这个关系很大呢?
```txt
  find /sys -name cmd_per_lun -exec cat {} \;
find: ‘/sys/kernel/debug’: Permission denied
0
0
0
0
0
0
0
0
0
0
0
0
256
```

### can_queue
<!-- 0d24fec1-0237-47f3-b543-97c59a866d51 -->

```txt
find /sys -name can_queue -exec cat {} \;
```

```txt
/sys/devices/pci0000:00/0000:00:17.0/ata1/host0/scsi_host/host0/can_queue
/sys/devices/pci0000:00/0000:00:17.0/ata8/host7/scsi_host/host7/can_queue
/sys/devices/pci0000:00/0000:00:17.0/ata6/host5/scsi_host/host5/can_queue
/sys/devices/pci0000:00/0000:00:17.0/ata4/host3/scsi_host/host3/can_queue
/sys/devices/pci0000:00/0000:00:17.0/ata2/host1/scsi_host/host1/can_queue
/sys/devices/pci0000:00/0000:00:17.0/ata7/host6/scsi_host/host6/can_queue
/sys/devices/pci0000:00/0000:00:17.0/ata5/host4/scsi_host/host4/can_queue
/sys/devices/pci0000:00/0000:00:17.0/ata3/host2/scsi_host/host2/can_queue
```

在服务器观察:
```txt
find /sys -name can_queue -exec cat {} \;
find: ‘/sys/kernel/debug’: Permission denied
32
32
32
32
32
32
32
32
32
32
32
32
5089
```
1. 获取

2. 传递给 block layer

scsi 中的 can_queue 就是 queue_depth ，在 `scsi_mq_setup_tags` 中将
```c
	tag_set->queue_depth = shost->can_queue;
```

### nr_requests 影响 sbitmap

```txt
- queue_requests_store
  - blk_mq_update_nr_requests
    - blk_mq_tag_update_depth
      - blk_mq_alloc_map_and_rqs
      - sbitmap_queue_resize
```

### scsi_device::queue_depth 和 scsi_device::max_queue_depth
继续分析一下，这种场景吧

```txt
echo 1 > /sys/block/sda/device/queue_depth
```

- sdev_store_queue_depth
  - 这里判断， 不能超过 virtio 的 can_queue
  - change_queue_depth
    - virtscsi_change_queue_depth
    - scsi_change_queue_depth
    - 修改 sdev->budget_map

queue_depth 起作用的地方是 scsi budget 机制
```txt
#0  scsi_dev_queue_ready (q=0xffff88810b428a98, sdev=0xffff88810c46d000) at drivers/scsi/scsi_lib.c:1253
#1  scsi_mq_get_budget (q=0xffff88810b428a98) at drivers/scsi/scsi_lib.c:1662
#2  0xffffffff8190f9ac in blk_mq_get_dispatch_budget (q=0xffff88810b428a98) at block/blk-mq.h:254
#3  blk_mq_get_budget_and_tag (rq=rq@entry=0xffff88810c0d6800) at block/blk-mq.c:2634
#4  0xffffffff81910863 in blk_mq_try_issue_directly (hctx=hctx@entry=0xffff88810bcfc200, rq=rq@entry=0xffff88810c0d6800) at block/blk-mq.c:2665
#5  0xffffffff81911754 in blk_mq_submit_bio (bio=<optimized out>) at block/blk-mq.c:3047
```

#### queue_depth 初始化的位置

virtscsi_probe 中初始化
```c
	shost->cmd_per_lun = min_t(u32, cmd_per_lun, shost->can_queue);
```

在 scsi_change_queue_depth 中
```c
	depth = sdev->host->cmd_per_lun ?: 1;
```

- do_one_initcall
  - virtio_scsi_init
    - register_virtio_driver
      - driver_register
        - bus_add_driver
          - driver_attach
            - bus_for_each_dev
              - __driver_attach
                - __driver_attach
                  - driver_probe_device
                    - __driver_probe_device
                      - really_probe
                        - call_driver_probe
                          - virtio_dev_probe
                            - virtscsi_probe
                              - scsi_scan_host
                                - scsi_scan_host
                                  - do_scsi_scan_host
                                    - scsi_scan_host_selected
                                      - scsi_scan_channel
                                        - scsi_scan_channel
                                          - __scsi_scan_target
                                            - scsi_probe_and_add_lun
                                              - scsi_alloc_sdev
                                                - scsi_change_queue_depth

#### max_queue_depth 的初始化和使用

无论初始化 (scsi_add_lun) 还是通过 sysfs 修改 (sdev_store_queue_depth) ，最后
都是设置 max_queue_depth = queue_depth

使用 max_queue_depth 地方在 : scsi_handle_queue_ramp_up

在 scsi_decide_disposition 和 scsi_eh_completed_normally 中如果检测到 queue 的大小在上升，那么就
使用 scsi_handle_queue_ramp_up 来提升 queue_depth ，其限制为 max_queue_depth

而 scsi_handle_queue_full 是用来限制 queue_depth 的

#### request_queue::queue_depth

修改 scsi_device::queue_depth 的时候会顺便修改 request_queue::queue_depth

实际上使用的地方并不多，修改 scsi_device::queue_depth 不会影响到 nr_requests

### blk_mq_tag_set
这个也整理也好:

一个 HBA 或者 nvme 控制器对应的一个 tag_set

`blk_mq_tag_set` 是放到 `nvme_dev` 中的

```c
struct nvme_dev {
	struct nvme_queue *queues;
	struct blk_mq_tag_set tagset;
```

类似的

```c
struct Scsi_Host {

	/* Area to keep a shared tag map */
	struct blk_mq_tag_set	tag_set;
```

以 nvme 为例子创建过程:
- nvme_probe -> nvme_alloc_io_tag_set
- nvme_pci_free_ctrl -> nvme_free_tagset -> nvme_remove_io_tag_set

### blk_mq_tag_set::nr_hw_queues

  nvme_alloc_io_tag_set 中 nvme_ctrl::queue_count 获取，后者
  nvme_alloc_queue 中每次加上 1 ，而 nvme_create_io_queues -> 会连续调用 nvme_alloc_queue (这里并没有完全分析清楚)

### blk_mq_tag_set::queue_depth : 硬件队列深度

nvme_alloc_io_tag_set 从 nvme_ctrl::sqsize 获取，后者在 nvme_setup_io_queues 初始化

### request_queue::queue_depth 和 request_queue::nr_reuqest 是什么关系

```c
/**
 * blk_set_queue_depth - tell the block layer about the device queue depth
 * @q:		the request queue for the device
 * @depth:		queue depth
 *
 */
void blk_set_queue_depth(struct request_queue *q, unsigned int depth)
{
	q->queue_depth = depth;
	rq_qos_queue_depth_changed(q);
}
```
https://patchwork.kernel.org/project/linux-scsi/patch/1597850436-116171-18-git-send-email-john.garry@huawei.com/

利用这个 backtrace ，可以轻松的追踪 tag 的大小是 tag_set::queue_depth 来初始化的
```txt
[    0.904009]  dump_stack_lvl+0x64/0x80
[    0.904009]  blk_mq_init_bitmaps+0x3b/0xc0
[    0.904009]  blk_mq_init_tags+0x7d/0xb0
[    0.904009]  blk_mq_alloc_map_and_rqs+0x4e/0x380
[    0.904009]  blk_mq_alloc_tag_set+0x1a4/0x3f0
[    0.904009]  scsi_add_host_with_dma+0xd0/0x370
[    0.904009]  virtscsi_probe+0x2ba/0x390
[    0.904009]  virtio_dev_probe+0x1b2/0x270
[    0.904009]  really_probe+0xbc/0x2c0
[    0.904009]  ? __pfx___driver_attach+0x10/0x10
[    0.904009]  __driver_probe_device+0x73/0x120
[    0.904009]  driver_probe_device+0x1f/0xe0
[    0.904009]  __driver_attach+0x88/0x180
[    0.904009]  bus_for_each_dev+0x85/0xd0
[    0.904009]  bus_add_driver+0xec/0x1f0
[    0.904009]  driver_register+0x59/0x100
[    0.904009]  ? __pfx_virtio_scsi_init+0x10/0x10
[    0.904009]  virtio_scsi_init+0x64/0xd0
[    0.904009]  ? __pfx_virtio_scsi_init+0x10/0x10
[    0.904009]  do_one_initcall+0x58/0x230
[    0.904009]  kernel_init_freeable+0x1c4/0x300
[    0.904009]  ? __pfx_kernel_init+0x10/0x10
[    0.904009]  kernel_init+0x1a/0x1c0
[    0.904009]  ret_from_fork+0x31/0x50
[    0.904009]  ? __pfx_kernel_init+0x10/0x10
[    0.904009]  ret_from_fork_asm+0x1b/0x30
[    0.904009]  </TASK>
[    0.904009] blk-mq: depth=256
[    0.904009] blk-mq: depth=256
```

nvme 中类似的操作：
- nvme_alloc_admin_tag_set : 这里初始化 tag_set->queue_depth 为 1000
  - blk_mq_alloc_tag_set


## mq 典型函数
<!-- 84578276-0ac4-4a76-9d7f-49cb7683fa6b -->

1.
```c
#define queue_for_each_hw_ctx(q, hctx, i)				\
	xa_for_each(&(q)->hctx_table, (i), (hctx))

#define hctx_for_each_ctx(hctx, ctx, i)					\
	for ((i) = 0; (i) < (hctx)->nr_ctx &&				\
	     ({ ctx = (hctx)->ctxs[(i)]; 1; }); (i)++)
```

2. blk_mq_queue_tag_busy_iter
遍历一个盘中提交到硬件队列及其之后阶段的 request

```c
int scsi_host_busy(struct Scsi_Host *shost)
{
	int cnt = 0;

	blk_mq_tagset_busy_iter(&shost->tag_set,
				scsi_host_check_in_flight, &cnt);
	return cnt;
}
```

## tag 是 hctx 级别的
<!-- 98a8dd7e-f9e7-4220-ab9c-b6f1e279c33c -->

( 2026-04-15 似乎并不是，其实可以让多个 hctx 共享 tag ，似乎和 shared tags 有关 ?)

获取 tags 实际上是依赖于 sbitmap 的，是 sbitmap 来实现

真正锁的粒度是在 `__sbitmap_get_word` 上面的。

每一个 blk_mq_tag_set 是 HBA 卡级别的，而 blk_mq_tags 是 hardware queue 级别的
```c
/*
 * @tags:	   Tag sets. One tag set per hardware queue. Has @nr_hw_queues
 *		   elements.
 */
struct blk_mq_tag_set {
  // ...
  unsigned int		nr_hw_queues;
  // ...
	struct blk_mq_tags	**tags;
```

一个盘上最多可以挂 nr_hw_queues * blk_mq_tags::nr_tags

会出现两个同时都是在 inflight 的 tag 的编号

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
