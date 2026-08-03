#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/module.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/interrupt.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include "dummy.h"

int testcase;
module_param_named(testcase, testcase, int, 0644);

static int queue_depth = DUMMY_QUEUE_DEPTH;
module_param_named(queue_depth, queue_depth, int, 0644);

static int queue_num = 1;
module_param_named(queue_num, queue_num, int, 0644);

struct virtio_dummy_req {
	struct virtio_dummy_req_hdr hdr;
	u8 status;
	u16 data_sg_count;
	struct scatterlist sg[DUMMY_MAX_SEGS + 2];
	struct scatterlist *sgs[DUMMY_MAX_SEGS + 2];
};

/* device private data (one per device) */
struct virtio_dummy_dev {
	struct virtio_device *vdev;
	struct virtqueue *vq;
	struct blk_mq_tag_set *tag_set;
	struct gendisk *disk;

	// 保护 virtioqueue
	spinlock_t lock;
	sector_t capacity_sectors;
	u32 blk_size;
	int major;
};

static blk_status_t dummy_status_to_blk(u8 status)
{
	switch (status) {
	case VIRTIO_DUMMY_S_OK:
		return BLK_STS_OK;
	case VIRTIO_DUMMY_S_UNSUPP:
		return BLK_STS_NOTSUPP;
	default:
		return BLK_STS_IOERR;
	}
}

static ssize_t internal_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	return count;
}

static ssize_t internal_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	// TODO 这个宏的确是没有想到的，这个是什么原理
	struct gendisk *disk = dev_to_disk(dev);

	debug_dump_gendisk(disk);
	return sysfs_emit(buf, "%s\n", "dummy");
}

static DEVICE_ATTR_RW(internal);

static struct attribute *virtblk_attrs[] = {
	&dev_attr_internal.attr,
	NULL,
};

static umode_t virtblk_attrs_are_visible(struct kobject *kobj,
					 struct attribute *a, int n)
{
	return a->mode;
}

static const struct attribute_group virtblk_attr_group = {
	.attrs = virtblk_attrs,
	.is_visible = virtblk_attrs_are_visible,
};

static const struct attribute_group *virtblk_attr_groups[] = {
	&virtblk_attr_group,
	NULL,
};

static void dummy_complete_rq(struct request *req)
{
	struct virtio_dummy_req *dummy_req = blk_mq_rq_to_pdu(req);

	blk_mq_end_request(req, dummy_status_to_blk(dummy_req->status));
}

// 参考 virtblk_add_req 和 __virtscsi_add_cmd
static blk_status_t dummy_queue_rq(struct blk_mq_hw_ctx *hctx,
				   const struct blk_mq_queue_data *bd)
{
	struct request *req = bd->rq;
	struct virtio_dummy_dev *dummy_dev = hctx->queue->queuedata;
	struct virtio_dummy_req *dummy_req = blk_mq_rq_to_pdu(req);
	struct bio_vec bvec;
	struct req_iterator iter;
	unsigned long flags;
	unsigned int data_len;
	int err;
	int num_out;
	int num_in;
	int data_sg_count = 0;
	int i = 0;

	// TODO 如果想要是多队列，这个似乎不可以使用了
	if (queue_num != 1)
		pr_info_once("queue_num=%d is ignored, current implementation uses a single virtqueue\n",
			     queue_num);

	data_len = blk_rq_bytes(req);
	memset(dummy_req, 0, sizeof(*dummy_req));
	dummy_req->status = VIRTIO_DUMMY_S_IOERR;

	if (blk_rq_is_passthrough(req))
		return BLK_STS_NOTSUPP;

	if (blk_rq_nr_phys_segments(req) > DUMMY_MAX_SEGS)
		return BLK_STS_IOERR;

	if (req_op(req) != REQ_OP_READ && req_op(req) != REQ_OP_WRITE &&
	    req_op(req) != REQ_OP_FLUSH &&
	    req_op(req) != REQ_OP_WRITE_ZEROES)
		return BLK_STS_NOTSUPP;

	if (req_op(req) != REQ_OP_FLUSH &&
	    blk_rq_pos(req) + blk_rq_sectors(req) > dummy_dev->capacity_sectors)
		return BLK_STS_IOERR;

	dummy_req->hdr.type = cpu_to_le32(req_op(req) == REQ_OP_WRITE ?
					  VIRTIO_DUMMY_T_OUT :
					  req_op(req) == REQ_OP_READ ?
					  VIRTIO_DUMMY_T_IN :
					  req_op(req) == REQ_OP_WRITE_ZEROES ?
					  VIRTIO_DUMMY_T_WRITE_ZEROES :
					  VIRTIO_DUMMY_T_FLUSH);
	dummy_req->hdr.sector = cpu_to_le64((u64)blk_rq_pos(req));
	dummy_req->hdr.data_len = cpu_to_le32(data_len);

	sg_init_one(&dummy_req->sg[i], &dummy_req->hdr, sizeof(dummy_req->hdr));
	dummy_req->sgs[i] = &dummy_req->sg[i];
	i++;

	rq_for_each_segment(bvec, req, iter) {
		sg_set_page(&dummy_req->sg[i], bvec.bv_page, bvec.bv_len,
			    bvec.bv_offset);
		sg_mark_end(&dummy_req->sg[i]);
		dummy_req->sgs[i] = &dummy_req->sg[i];
		i++;
		data_sg_count++;
	}
	dummy_req->data_sg_count = data_sg_count;

	sg_init_one(&dummy_req->sg[i], &dummy_req->status,
		    sizeof(dummy_req->status));
	dummy_req->sgs[i] = &dummy_req->sg[i];

	if (req_op(req) == REQ_OP_READ) {
		num_out = 1;
		num_in = data_sg_count + 1;
	} else if (req_op(req) == REQ_OP_WRITE) {
		num_out = 1 + data_sg_count;
		num_in = 1;
	} else {
		num_out = 1;
		num_in = 1;
	}

	debug_dump_request(req);
	blk_mq_start_request(req);

	// TODO lock ?
	// 当前在 rcu_read_lock 中，不能使用 GFP_KERNEL
	spin_lock_irqsave(&dummy_dev->lock, flags);
	err = virtqueue_add_sgs(dummy_dev->vq, dummy_req->sgs, num_out, num_in,
				req, GFP_ATOMIC);
	if (err) {
		virtqueue_kick(dummy_dev->vq);
		/* Don't stop the queue if -ENOMEM: we may have failed to
		 * bounce the buffer due to global resource outage.
		 */
		if (err == -ENOSPC)
			blk_mq_stop_hw_queue(hctx);
		spin_unlock_irqrestore(&dummy_dev->lock, flags);
		return err == -ENOMEM ? BLK_STS_RESOURCE : BLK_STS_DEV_RESOURCE;
	}
	spin_unlock_irqrestore(&dummy_dev->lock, flags);

	virtqueue_notify(dummy_dev->vq);

	return BLK_STS_OK;
}

static const struct blk_mq_ops dummy_mq_ops = {
	.queue_rq = dummy_queue_rq,
	.complete = dummy_complete_rq,
};

// 参考 dm_mq_init_request_queue 和 nullblk 的实现
static int dummy_init_request_queue(struct virtio_dummy_dev *dummy)
{
	struct blk_mq_tag_set *tag_set;
	int err;

	BUG_ON(dummy == NULL);
	tag_set = kzalloc_node(sizeof(*tag_set), GFP_KERNEL, NUMA_NO_NODE);
	if (!tag_set)
		return -ENOMEM;

	tag_set->ops = &dummy_mq_ops;
	tag_set->queue_depth = queue_depth;
	tag_set->numa_node = NUMA_NO_NODE;
	tag_set->flags = 0;
	tag_set->nr_hw_queues = 1;
	tag_set->nr_maps = 1;
	// TODO 这个 cmd_size 是如何利用的
	tag_set->cmd_size = sizeof(struct virtio_dummy_req);
	tag_set->driver_data = dummy;

	err = blk_mq_alloc_tag_set(tag_set);
	if (err)
		goto out_kfree_tag_set;

	dummy->tag_set = tag_set;
	spin_lock_init(&dummy->lock);
	return 0;

out_kfree_tag_set:
	kfree(tag_set);
	return err;
}

// TODO 这个东西为什么只有注册一个空的就可以了?
static const struct block_device_operations dummy_rq_ops = {
	.owner = THIS_MODULE,
};

static int dummy_read_config(struct virtio_dummy_dev *dummy)
{
	virtio_cread_le(dummy->vdev, struct virtio_dummy_config,
			capacity_sectors, &dummy->capacity_sectors);
	virtio_cread_le(dummy->vdev, struct virtio_dummy_config,
			blk_size, &dummy->blk_size);

	if (!dummy->blk_size)
		dummy->blk_size = DUMMY_BLOCK_SIZE;
	if (!dummy->capacity_sectors)
		return -ENODEV;

	return 0;
}

static int dummy_init_disk(struct virtio_dummy_dev *dummy)
{
	struct queue_limits lim = {
		.logical_block_size = dummy->blk_size,
		.physical_block_size = dummy->blk_size,
		.io_min = dummy->blk_size,
		.max_segments = DUMMY_MAX_SEGS,
		.max_hw_sectors = 1024,
		.max_segment_size = PAGE_SIZE,
		.features = BLK_FEAT_SYNCHRONOUS,
	};
	int rv;

	// register_blkdev 的功能:
	// - allocating a dynamic major number if requested, and
	// - creating an entry in /proc/devices
	dummy->major = register_blkdev(0, "dummy-null");
	if (dummy->major < 0)
		return dummy->major;

	dummy->disk = blk_mq_alloc_disk(dummy->tag_set, &lim, dummy);
	if (IS_ERR(dummy->disk)) {
		rv = PTR_ERR(dummy->disk);
		goto fail_unregister_blkdev;
	}

	set_capacity(dummy->disk, dummy->capacity_sectors);
	dummy->disk->major = dummy->major;
	dummy->disk->first_minor = 0;
	dummy->disk->minors = 1;
	dummy->disk->fops = &dummy_rq_ops;
	dummy->disk->private_data = dummy;
	snprintf(dummy->disk->disk_name, DISK_NAME_LEN, "dummy");
	return 0;

fail_unregister_blkdev:
	unregister_blkdev(dummy->major, "dummy-null");
	dummy->major = 0;
	return rv;
}

/**
 * 调用路径为:
 *
 * virtio_dummy_recv_cb+0x34/0x90 [virtio_dummy]
 * vring_interrupt+0x5b/0x90
 * vp_vring_interrupt+0x57/0x90
 * __handle_irq_event_percpu+0x6d/0x1d0
 * handle_irq_event+0x38/0x80
 * handle_fasteoi_irq+0x7c/0x210
 * __common_interrupt+0x3c/0xa0
 * common_interrupt+0x83/0xa0
 *
 * 参考实现: virtblk_done
 */
static void virtio_dummy_recv_cb(struct virtqueue *vq)
{
	struct virtio_dummy_dev *dev = vq->vdev->priv;
	struct request *req;
	unsigned int len;
	unsigned long flags;
	bool req_done = false;

	BUG_ON(!in_interrupt());
	debug_hardirq();
	// msleep(100);
	// 无论是在中断中，还是在软中断中睡眠，
	// 最终在 __schedule -> __schedule_bug 中触发 crash
	spin_lock_irqsave(&dev->lock, flags);
	do {
		virtqueue_disable_cb(dev->vq);
		while ((req = virtqueue_get_buf(dev->vq, &len)) != NULL) {
			blk_mq_complete_request(req);
			req_done = true;
		}
	} while (!virtqueue_enable_cb(dev->vq));
	if (req_done)
		blk_mq_start_stopped_hw_queues(dev->disk->queue, true);
	spin_unlock_irqrestore(&dev->lock, flags);
}

static int virtio_dummy_probe(struct virtio_device *vdev)
{
	struct virtio_dummy_dev *dummy;
	int rv;

	/* the device has a single virtqueue */
	dummy = kzalloc(sizeof(*dummy), GFP_KERNEL);
	if (!dummy)
		return -ENOMEM;

	dummy->vdev = vdev;
	// TODO 如果想要是多队列，这个还可以用吗?
	dummy->vq = virtio_find_single_vq(vdev, virtio_dummy_recv_cb, "request");
	if (IS_ERR(dummy->vq)) {
		rv = PTR_ERR(dummy->vq);
		goto out_free_dummy;
	}
	vdev->priv = dummy;

	rv = dummy_read_config(dummy);
	if (rv)
		goto out_del_vqs;

	rv = dummy_init_request_queue(dummy);
	if (rv)
		goto out_del_vqs;

	rv = dummy_init_disk(dummy);
	if (rv)
		goto out_free_tag_set;

	/* from this point on, the device can notify and get callbacks */
	virtio_device_ready(vdev);
	return device_add_disk(&vdev->dev, dummy->disk, virtblk_attr_groups);

out_free_tag_set:
	blk_mq_free_tag_set(dummy->tag_set);
	kfree(dummy->tag_set);
out_del_vqs:
	vdev->config->del_vqs(vdev);
out_free_dummy:
	kfree(dummy);
	return rv;
}

static void virtio_dummy_remove(struct virtio_device *vdev)
{
	struct virtio_dummy_dev *dev = vdev->priv;

	/* disable vq interrupts: equivalent to vdev->config->reset(vdev) */
	virtio_reset_device(vdev);

	// TODO codex 说这个没必要，这是真的吗?
	// 还是说，下面的 del_vqs 自动完成了工作
	// /* detach unused buffers */
	// while ((buf = virtqueue_detach_unused_buf(dev->vq)) != NULL) {
	// 	kfree(buf);
	// }
	
	/* remove virtqueues */
	vdev->config->del_vqs(vdev);

	del_gendisk(dev->disk);
	put_disk(dev->disk);
	// TODO 这个是必须的麻烦?
	if (dev->major > 0)
		unregister_blkdev(dev->major, "dummy-null");
	blk_mq_free_tag_set(dev->tag_set);
	kfree(dev->tag_set);
	kfree(dev);
}

static const struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_DUMMY, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_dummy_driver = {
	.driver.name = KBUILD_MODNAME,
	.driver.owner = THIS_MODULE,
	.id_table = id_table,
	.probe = virtio_dummy_probe,
	.remove = virtio_dummy_remove,
};

// TODO 这有什么区别吗?
/* module_virtio_driver(virtio_dummy_driver); */
//
// TODO 到底 virtio_dummy_probe 什么时候调用的 ？
// 为什么需要先 remove 一次，第二次 insmod 才会调用
//
// register_virtio_device 什么时候被调用?

static int __init virtio_dummy_init(void)
{
	return register_virtio_driver(&virtio_dummy_driver);
}

static void __exit virtio_dummy_exit(void)
{
	unregister_virtio_driver(&virtio_dummy_driver);
}

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Dummy virtio driver");
MODULE_LICENSE("GPL");
module_init(virtio_dummy_init);
module_exit(virtio_dummy_exit);
