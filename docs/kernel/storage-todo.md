- [IO 子系统全流程介绍](https://zhuanlan.zhihu.com/p/545906763)

## 这两个 timeout 有什么关系 : 居然就是一个东西
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

- 似乎是最后会调用到这里:
  - blk_mq_timeout_work
