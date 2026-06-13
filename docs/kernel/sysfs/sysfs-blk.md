# sysfs block

## /sys/block/*

首先，在 genhd.c 中:
```c
static DEVICE_ATTR(range, 0444, disk_range_show, NULL);
static DEVICE_ATTR(ext_range, 0444, disk_ext_range_show, NULL);
static DEVICE_ATTR(removable, 0444, disk_removable_show, NULL);
static DEVICE_ATTR(hidden, 0444, disk_hidden_show, NULL);
static DEVICE_ATTR(ro, 0444, disk_ro_show, NULL);
static DEVICE_ATTR(size, 0444, part_size_show, NULL);
static DEVICE_ATTR(alignment_offset, 0444, disk_alignment_offset_show, NULL);
static DEVICE_ATTR(discard_alignment, 0444, disk_discard_alignment_show, NULL);
static DEVICE_ATTR(capability, 0444, disk_capability_show, NULL);
static DEVICE_ATTR(stat, 0444, part_stat_show, NULL);
static DEVICE_ATTR(inflight, 0444, part_inflight_show, NULL);
static DEVICE_ATTR(badblocks, 0644, disk_badblocks_show, disk_badblocks_store);
static DEVICE_ATTR(diskseq, 0444, diskseq_show, NULL);
```

### stat

实现 genhd.c 中的 part_stat_show

Documentation/block/stat.rst

```txt
in_flight
=========

This value counts the number of I/O requests that have been issued to
the device driver but have not yet completed.  It does not include I/O
requests that are in the queue but not yet issued to the device driver.
```

inflight = part_in_flight(bdev);

### inflight

和 /sys/block/*/stat 中的 in_flight 的关系

1. part_inflight_show 会分别展示 read 和 write ，而
/sys/block/*/stat 中的 in_flight 中两个结果是合并的
2. part_inflight_show 是考虑 mq 的，也是对于 nvme ，/sys/block/*/stat 中的 in_flight 的输出是 0

在看看啥意思，当时写的，忘记了
> part_inflight_show 中分别统计了 mq 和非 mq 的情况， part_in_flight_rw 说明，
> 是将统计数据放到 block_device::bd_stats::in_flight 中的

而 mq 的设备:
- bio_start_io_acct
- `bio_end_io_acct`


### /sys/block/*/device

#### scsi
scsi 的: drivers/scsi/scsi_sysfs.c

scsi_sdev_attrs

```txt
➜  device ls /sys/block/sda/device
blacklist      delete          evt_capacity_change_reported        evt_soft_threshold_reached  ioerr_cnt      power        scsi_device   subsystem  vpd_pg0   wwid
block          device_blocked  evt_inquiry_change_reported         generic                     iorequest_cnt  queue_depth  scsi_disk     timeout    vpd_pg83
bsg            device_busy     evt_lun_change_reported             inquiry                     iotmo_cnt      queue_type   scsi_generic  type       vpd_pgb0
cdl_enable     driver          evt_media_change                    iocounterbits               modalias       rescan       scsi_level    uevent     vpd_pgb1
cdl_supported  eh_timeout      evt_mode_parameter_change_reported  iodone_cnt                  model          rev          state         vendor     vpd_pgb2
```

echo offline > /sys/block/sdx/device/state

#### nvme

drivers/nvme/host/sysfs.c
```txt
➜  device ls /sys/block/nvme0n1/device
address  cntrltype  dev     firmware_rev  kato   ng0n1      nvme0n1  queue_count        reset_controller  sqsize  subsysnqn  transport
cntlid   dctype     device  hwmon0        model  numa_node  power    rescan_controller  serial            state   subsystem  uevent
```

device -> ../../../0000:83:00.0

#### virtio-blk

```txt
➜  ~ ls /sys/block/vda/device
block  device  driver  features  modalias  power  status  subsystem  uevent  vendor
```

device -> ../../../virtio1


### /sys/block/*/trace/
用于 blktrace ，只有打开 CONFIG_BLK_DEV_IO_TRACE 这个目录才存在。

### /sys/block/*/queue/
```txt
add_random                   discard_max_hw_bytes  logical_block_size      nr_requests          virt_boundary_mask
atomic_write_boundary_bytes  discard_zeroes_data   max_discard_segments    nr_zones             write_cache
atomic_write_max_bytes       dma_alignment         max_hw_sectors_kb       optimal_io_size      write_same_max_bytes
atomic_write_unit_max_bytes  fua                   max_integrity_segments  physical_block_size  write_zeroes_max_bytes
atomic_write_unit_min_bytes  hw_sector_size        max_sectors_kb          read_ahead_kb        zone_append_max_bytes
chunk_sectors                io_poll               max_segment_size        rotational           zone_write_granularity
dax                          io_poll_delay         max_segments            rq_affinity          zoned
discard_granularity          iostats               minimum_io_size         scheduler
discard_max_bytes            iostats_passthrough   nomerges                stable_writes
```
block/blk-mq-sysfs.c

注意，以上内容分为两个部分，其中 request-based 多出来一些内容:
```c
/* Common attributes for bio-based and request-based queues. */
static struct attribute *queue_attrs[] = { }

/* Request-based queue attributes that are not relevant for bio-based queues. */
static struct attribute *blk_mq_queue_attrs[] = {
	/*
	 * Attributes which require some form of locking other than
	 * q->sysfs_lock.
	 */
	&elv_iosched_entry.attr,
	&queue_requests_entry.attr,
#ifdef CONFIG_BLK_WBT
	&queue_wb_lat_entry.attr,
#endif
	/*
	 * Attributes which don't require locking.
	 */
	&queue_rq_affinity_entry.attr,
	&queue_io_timeout_entry.attr,

	NULL,
};
```

### /sys/block/*/mq/
block/blk-sysfs.c

virtio-blk 在一个 4 core 的虚拟机上
```txt
├── 0
│   ├── cpu0
│   ├── cpu_list
│   ├── nr_reserved_tags
│   └── nr_tags (256)
├── 1
│   ├── cpu1
│   ├── cpu_list
│   ├── nr_reserved_tags
│   └── nr_tags
├── 2
│   ├── cpu2
│   ├── cpu_list
│   ├── nr_reserved_tags
│   └── nr_tags
└── 3
    ├── cpu3
    ├── cpu_list
    ├── nr_reserved_tags
    └── nr_tags
```

实现的代码在 : block/blk-mq-sysfs.c

具体的路径 : /sys/block/nvme0n1/mq

因为创建的是 kobject_add 是自动添加的目录:
```c
	hctx_for_each_ctx(hctx, ctx, i) {
		ret = kobject_add(&ctx->kobj, &hctx->kobj, "cpu%u", ctx->cpu);
		if (ret)
			goto out;
	}
```

这里的内容相对于 debugfs ，太弱了

# /proc/partitions

这个接口本身很简单，但是我

# /proc/diskstats

```txt
🧀  cat /proc/diskstats
 259       0 nvme1n1 1141423 165624 55597064 204574 5461706 6557411 242187142 967652 0 1014962 1345517 0 0 0 0 872323 173291
 259       1 nvme1n1p1 1140939 164658 55574251 204415 5461483 6541994 242062036 967375 0 1014812 1171790 0 0 0 0 0 0
 259       2 nvme1n1p2 114 15 7096 48 221 15417 125104 276 0 200 325 0 0 0 0 0 0
 259       3 nvme1n1p3 277 951 12045 102 2 0 2 0 0 92 102 0 0 0 0 0 0
 259       4 nvme0n1 154 0 8144 10 0 0 0 0 0 15 10 0 0 0 0 0 0
 259       5 nvme0n1p1 59 0 4432 4 0 0 0 0 0 6 4 0 0 0 0 0 0
   8      16 sdb 94 0 4504 94 0 0 0 0 0 94 94 0 0 0 0 0 0
   8       0 sda 94 0 4504 32 0 0 0 0 0 29 32 0 0 0 0 0 0
   7       0 loop0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       1 loop1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       2 loop2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       3 loop3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       4 loop4 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       5 loop5 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       6 loop6 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
   7       7 loop7 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
```

- Documentation/ABI/testing/procfs-diskstats
- Documentation/admin-guide/iostats.rst

- https://bean-li.github.io/dive-into-iostat/
- https://ykrocku.github.io/blog/2014/04/11/diskstats/

实现的位置参考:
- blk_account_io_done

在 iostat -xz 5 中 w_await 是所有 io 的平均值。

这里 ifutil 的项目中，

/proc/diskstats 的具体每一项的实现在这里
- /sys/devices/pci0000:00/0000:00:08.0/nvme/nvme1/nvme1n1/stat

# 其他
## 源码
genhd.c

```c
static int __init proc_genhd_init(void)
{
  proc_create_seq("diskstats", 0, NULL, &diskstats_op);
  proc_create_seq("partitions", 0, NULL, &partitions_op);
  return 0;
}

```

对应的接口为:
3. /sys/block/sda/ 下的各种

register_blkdev 的调用，只要内存存在这个模块，就会进行对应的注册。

## 两个 timeout 接口
/sys/block/sda/device/timeout
/sys/block/sda/queue/io_timeout

```c
static ssize_t queue_io_timeout_show(struct request_queue *q, char *page)
{
	return sprintf(page, "%u\n", jiffies_to_msecs(q->rq_timeout));
}
```

```c
/*
 * TODO: can we make these symlinks to the block layer ones?
 */
static ssize_t
sdev_show_timeout (struct device *dev, struct device_attribute *attr, char *buf)
{
	struct scsi_device *sdev;
	sdev = to_scsi_device(dev);
	return snprintf(buf, 20, "%d\n", sdev->request_queue->rq_timeout / HZ);
}

static ssize_t
sdev_store_timeout (struct device *dev, struct device_attribute *attr,
		    const char *buf, size_t count)
{
	struct scsi_device *sdev;
	int timeout;
	sdev = to_scsi_device(dev);
	sscanf (buf, "%d\n", &timeout);
	blk_queue_rq_timeout(sdev->request_queue, timeout * HZ);
	return count;
}
static DEVICE_ATTR(timeout, S_IRUGO | S_IWUSR, sdev_show_timeout, sdev_store_timeout);
```

block/blk-timeout.c 中

blk_mq_timeout_work

## device 的路径解析

/sys/devices/pci0000:00/0000:00:0a.0/virtio5/host1/target1:0:0/1:0:0:0/timeout

1. 0000:00:0a.0

```txt
➜  ~ lspci -s 0000:00:0a.0
00:0a.0 SCSI storage controller: Virtio: Virtio SCSI
```

2. virtio5 : 0000:00:0a.0 是第五个 virtio 设备
3. host1 : 指的是 host 为 1
4. target1:0:0 : ?
5. 1:0:0:0 : 就是 Host Channel Id Lun 编号

其下存在三个目录:

/sys/devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0

- scsi_device : 从这里触发，最后指回来
- scsi_disk : 存储 drivers/scsi/sd.c 中暴露的内容，因为当前设备是 scsi sd ，所以存储到此处
- block : /sys/block 软链接直接指向这里
- bsg

```txt
🧀  pwd
/sys/devices/pci0000:00/0000:00:17.0/ata8/host7/target7:0:0/7:0:0:0/scsi_device/7:0:0:0
🧀  ls
Permissions Size User Date Modified Name
lrwxrwxrwx     0 root 18 Oct 11:06   device -> ../../../7:0:0:0 # scsi_device 最后会指回来
drwxr-xr-x     - root 18 Oct 11:06   power
lrwxrwxrwx     0 root 16 Oct 21:42   subsystem -> ../../../../../../../../../class/scsi_device
.rw-r--r--  4.1k root 18 Oct 11:06   uevent
```

通过软链接，将各种代码整体放到一起。

不过，代码实现上，这些都是如何构成的。

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
