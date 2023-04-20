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

应该是这个来搞的吧
/home/martins3/core/linux/block/blk-timeout.c

## 一个 struct request 到底代表什么？
`struct request_queue` 和什么对应的?

```c
	struct request_queue *q = bdev_get_queue(bio->bi_bdev);
```

## 研究下什么是 zone device

```c
/*
 * blk_mq_plug() - Get caller context plug
 * @bio : the bio being submitted by the caller context
 *
 * Plugging, by design, may delay the insertion of BIOs into the elevator in
 * order to increase BIO merging opportunities. This however can cause BIO
 * insertion order to change from the order in which submit_bio() is being
 * executed in the case of multiple contexts concurrently issuing BIOs to a
 * device, even if these context are synchronized to tightly control BIO issuing
 * order. While this is not a problem with regular block devices, this ordering
 * change can cause write BIO failures with zoned block devices as these
 * require sequential write patterns to zones. Prevent this from happening by
 * ignoring the plug state of a BIO issuing context if it is for a zoned block
 * device and the BIO to plug is a write operation.
 *
 * Return current->plug if the bio can be plugged and NULL otherwise
 */
static inline struct blk_plug *blk_mq_plug( struct bio *bio)
{
	/* Zoned block device write operation case: do not plug the BIO */
	if (IS_ENABLED(CONFIG_BLK_DEV_ZONED) &&
	    bdev_op_is_zoned_write(bio->bi_bdev, bio_op(bio)))
		return NULL;

	/*
	 * For regular block devices or read operations, use the context plug
	 * which may be NULL if blk_start_plug() was not executed.
	 */
	return current->plug;
}
```

## blktrace
- https://developer.aliyun.com/article/698568

- call_bio_endio 中最后会调用到 `bio_end_io_acct`，是给 blktrace 来处理的吗?
