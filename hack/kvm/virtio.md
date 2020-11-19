# TODO
- [ ] virtiofs 的内容很少

## eventfd

**如何打开 virtio 选项于 make menuconfig 中**
1. Device Driver => Virtio Drivers
2. Device Driver => Block devices => Virtio block io

- [ ] http://blog.allenx.org/2015/07/05/kvm-irqfd-and-ioeventfd

## virtiofs
- [ ] https://libvirt.org/kbase/virtiofs.html
  - [ ] ali 的人建议 : https://virtio-fs.gitlab.io/ : /home/maritns3/core/54-linux/fs/fuse/virtio_fs.c : 只有 1000 多行，9pfs 也很小，这些东西有什么特殊的地方吗 ?

## virtio-blk

### eventfd
```c
static struct pci_driver virtio_pci_driver = {
	.name		= "virtio-pci",
	.id_table	= virtio_pci_id_table,
	.probe		= virtio_pci_probe,
	.remove		= virtio_pci_remove,
#ifdef CONFIG_PM_SLEEP
	.driver.pm	= &virtio_pci_pm_ops,
#endif
	.sriov_configure = virtio_pci_sriov_configure,
};
```

virtio_dev_probe 调用 virtio_blk::probe
```c
static struct bus_type virtio_bus = {
	.name  = "virtio",
	.match = virtio_dev_match,
	.dev_groups = virtio_dev_groups,
	.uevent = virtio_uevent,
	.probe = virtio_dev_probe,
	.remove = virtio_dev_remove,
};
```

```c
static struct virtio_driver virtio_blk = {
	.feature_table			= features,
	.feature_table_size		= ARRAY_SIZE(features),
	.feature_table_legacy		= features_legacy,
	.feature_table_size_legacy	= ARRAY_SIZE(features_legacy),
	.driver.name			= KBUILD_MODNAME,
	.driver.owner			= THIS_MODULE,
	.id_table			= id_table,
	.probe				= virtblk_probe,
	.remove				= virtblk_remove,
	.config_changed			= virtblk_config_changed,
#ifdef CONFIG_PM_SLEEP
	.freeze				= virtblk_freeze,
	.restore			= virtblk_restore,
#endif
};
```

- [ ] 为什么会存在 virtio-pci 设备的存在，既然已经构建了一个 virtio_bus 的总线类型


- [ ] virtio_pci_probe
  - [ ] virtio_pci_modern_probe : 给 virtio_pci_device::vdev 注册

## 等待清理的部分

virtio_find_vqs

```c
/* Our device structure */
struct virtio_pci_device {
	struct virtio_device vdev;
	struct pci_dev *pci_dev;
```

```c
static const struct virtio_config_ops virtio_pci_config_ops = {
	.get		= vp_get,
	.set		= vp_set,
	.generation	= vp_generation,
	.get_status	= vp_get_status,
	.set_status	= vp_set_status,
	.reset		= vp_reset,
	.find_vqs	= vp_modern_find_vqs,
	.del_vqs	= vp_del_vqs,
	.get_features	= vp_get_features,
	.finalize_features = vp_finalize_features,
	.bus_name	= vp_bus_name,
	.set_vq_affinity = vp_set_vq_affinity,
	.get_vq_affinity = vp_get_vq_affinity,
	.get_shm_region  = vp_get_shm_region,
};
```

```c
static int vp_modern_find_vqs(struct virtio_device *vdev, unsigned nvqs,
			      struct virtqueue *vqs[],
			      vq_callback_t *callbacks[],
			      const char * const names[], const bool *ctx,
			      struct irq_affinity *desc)
{
	struct virtio_pci_device *vp_dev = to_vp_device(vdev);
	struct virtqueue *vq;
	int rc = vp_find_vqs(vdev, nvqs, vqs, callbacks, names, ctx, desc);

	if (rc)
		return rc;

	/* Select and activate all queues. Has to be done last: once we do
	 * this, there's no way to go back except reset.
	 */
	list_for_each_entry(vq, &vdev->vqs, list) {
		vp_iowrite16(vq->index, &vp_dev->common->queue_select);
		vp_iowrite16(1, &vp_dev->common->queue_enable);
	}

	return 0;
}
```



```c
/**
 * virtio_device - representation of a device using virtio
 * @index: unique position on the virtio bus
 * @failed: saved value for VIRTIO_CONFIG_S_FAILED bit (for restore)
 * @config_enabled: configuration change reporting enabled
 * @config_change_pending: configuration change reported while disabled
 * @config_lock: protects configuration change reporting
 * @dev: underlying device.
 * @id: the device type identification (used to match it with a driver).
 * @config: the configuration ops for this device.
 * @vringh_config: configuration ops for host vrings.
 * @vqs: the list of virtqueues for this device.
 * @features: the features supported by both driver and device.
 * @priv: private pointer for the driver's use.
 */
struct virtio_device {
	int index;
	bool failed;
	bool config_enabled;
	bool config_change_pending;
	spinlock_t config_lock;
	struct device dev;
	struct virtio_device_id id;
	const struct virtio_config_ops *config;
	const struct vringh_config_ops *vringh_config;
	struct list_head vqs;
	u64 features;
	void *priv;
};
```

```c
/* Virtio ring descriptors: 16 bytes.  These can chain together via "next". */
struct vring_desc {
	/* Address (guest-physical). */
	__virtio64 addr;
	/* Length. */
	__virtio32 len;
	/* The flags as indicated above. */
	__virtio16 flags;
	/* We chain unused descriptors via this, too */
	__virtio16 next;
};

struct vring_avail {
	__virtio16 flags;
	__virtio16 idx;
	__virtio16 ring[];
};

/* u32 is used here for ids for padding reasons. */
struct vring_used_elem {
	/* Index of start of used descriptor chain. */
	__virtio32 id;
	/* Total length of the descriptor chain which was used (written to) */
	__virtio32 len;
};

struct vring_used {
	__virtio16 flags;
	__virtio16 idx;
	struct vring_used_elem ring[];
};

struct vring {
	unsigned int num;

	struct vring_desc *desc;

	struct vring_avail *avail;

	struct vring_used *used;
};

/**
 * virtqueue - a queue to register buffers for sending or receiving.
 * @list: the chain of virtqueues for this device
 * @callback: the function to call when buffers are consumed (can be NULL).
 * @name: the name of this virtqueue (mainly for debugging)
 * @vdev: the virtio device this queue was created for.
 * @priv: a pointer for the virtqueue implementation to use.
 * @index: the zero-based ordinal number for this queue.
 * @num_free: number of elements we expect to be able to fit.
 *
 * A note on @num_free: with indirect buffers, each buffer needs one
 * element in the queue, otherwise a buffer will need one element per
 * sg element.
 */
struct virtqueue {
	struct list_head list;
	void (*callback)(struct virtqueue *vq);
	const char *name;
	struct virtio_device *vdev;
	unsigned int index;
	unsigned int num_free;
	void *priv;
};

static const struct blk_mq_ops virtio_mq_ops = {
	.queue_rq	= virtio_queue_rq,
	.commit_rqs	= virtio_commit_rqs,
	.complete	= virtblk_request_done,
	.init_request	= virtblk_init_request,
	.map_queues	= virtblk_map_queues,
};
```

- [ ] virtqueue

- [ ] virtqueue_add_outbuf

- [ ] vring_virtqueue :
  - 似乎划分为 split 和 pack 两种方法

 blk_mq_init_queue


- [ ] virt_queue 在内核端也是定义了的吗
  - [ ] 如何使用 ?
  - [ ] 驱动如何将 multiqueue 收到的消息整合成为 vring ?
  - virtblk_probe :初始化 `vblk->tag_set.ops = &virtio_mq_ops;`
     - [ ] virtio_queue_rq : 
        - [ ] virtblk_add_req : 将 multiqueue 的 request 装换为

- [ ] guest 怎么知道使用真正的 设备驱动 还是 virt-blk 啊 ?

- [ ] 如果 virt 中间传输的地址是 HVA，但是到了 guest 的层次，其知道应该是 blk sector 啊!
