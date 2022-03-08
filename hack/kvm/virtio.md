## env
**如何打开 virtio 选项于 make menuconfig 中**
1. Device Driver => Virtio Drivers ==> PCI drivers for virtio devices
2. Device Driver => Block devices => Virtio block io
3. Device Driver => Network Device support => Virtio network driver

- [ ] 打开 console 的方法, 不然 mini-img 中的串口无法使用
  - 如果不打开 block device driver 的，直接无法启动


应该使用这种方法配置:
https://lore.kernel.org/patchwork/patch/267098/
```
CONFIG_BLK_MQ_VIRTIO=y
CONFIG_NET_9P_VIRTIO=y
CONFIG_VIRTIO_BLK=y
CONFIG_VIRTIO_NET=y
CONFIG_VIRTIO_CONSOLE=y
# CONFIG_HW_RANDOM_VIRTIO is not set
# CONFIG_DRM_VIRTIO_GPU is not set
CONFIG_VIRTIO=y
CONFIG_VIRTIO_PCI_LIB=y
CONFIG_VIRTIO_MENU=y
CONFIG_VIRTIO_PCI=y
CONFIG_VIRTIO_PCI_LEGACY=y
CONFIG_VIRTIO_BALLOON=y
CONFIG_VIRTIO_INPUT=y
CONFIG_VIRTIO_MMIO=y
CONFIG_VIRTIO_MMIO_CMDLINE_DEVICES=y
# CONFIG_RPMSG_VIRTIO is not set
# CONFIG_CRYPTO_DEV_VIRTIO is not set
```


为了共享 folder :
```
CONFIG_NET_9P=y
CONFIG_NET_9P_VIRTIO=y
CONFIG_NET_9P_DEBUG=y
CONFIG_9P_FS=y
CONFIG_9P_FS_POSIX_ACL=y
CONFIG_9P_FS_SECURITY=y
```
9P 的手动打开方法
1. Networking Support =>  Plan 9 Resource Sharing Support (9P2000)
2. File systems  =>  Network File Systems => Plan 9 Resource Sharing Support (9P2000)

VHOST 打开方法
1. Device Drivers  =>  VHOST drivers => Host kernel accelerator for virtio net
```
CONFIG_VHOST_IOTLB=y
CONFIG_VHOST=y
CONFIG_VHOST_MENU=y
CONFIG_VHOST_NET=y
# CONFIG_VHOST_CROSS_ENDIAN_LEGACY is not set
```

VIRTIO_SCSI 打开:
Device Driver ==> SCSI device support ==> SCSI low-level drivers ==> virtio-scsi support

**kvmtool** 的编译安装

- [ ] 如果 guest kernel 不支持 virtio, 但是 kvmtool / qemu 还是指定使用 virtio ，出现了错误，怎么办法

# TODO

- [ ] /home/maritns3/core/firecracker/src/devices/src/virtio/vsock/csm/connection.rs has a small typo

- [ ] virtiofs 的代码很少，资料很多，其实值得分析一下

- [ ] virtio and msi:

- [ ] We're over optimistic about the meaning of the complexity of virtio, try to read **/home/maritns3/core/linux/drivers/virtio**, please.

- [ ] what's scsi : https://www.linux-kvm.org/images/archive/f/f5/20110823142849!2011-forum-virtio-scsi.pdf

[ ](https://www.redhat.com/en/blog/deep-dive-virtio-networking-and-vhost-net)
[ ](https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net)

- [ ] 有的设备不支持 PCI 总线，需要使用 MMIO 的方式，但是kvmtool 怎么知道这个设备需要使用 MMIO

- [ ] 约定是第一个 bar 指向的 IO 空间在内核那一侧是怎么分配的 ?

- [ ] virtio_bus 是挂载到哪里的?

- [ ] virtio_console 的具体实现是怎么样子的 ?

- [ ] 现在对于 eventfd 都是从 virt-blk 角度理解的，其实如何利用 eventfd 实现 guest 到 kernel 的通知，比如 irqfd 来实现 Qemu 直接将 irq 注入到 guest 中

- [ ] LoyenWang 页分析过 virtio

## vhost
https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2020/04/18/vsock-internals

## virtiofs
- [ ] https://libvirt.org/kbase/virtiofs.html
  - [ ] ali 的人建议 : https://virtio-fs.gitlab.io/ : /home/maritns3/core/54-linux/fs/fuse/virtio_fs.c : 只有 1000 多行，9pfs 也很小，这些东西有什么特殊的地方吗 ?

首先将 virtiofs 用起来: https://www.tauceti.blog/post/qemu-kvm-share-host-directory-with-vm-with-virtio/
> virtio-fs on the other side is designed to offer local file system semantics and performance. virtio-fs takes advantage of the virtual machine’s co-location with the hypervisor to avoid overheads associated with network file systems. virtio-fs uses FUSE as the foundation. Unlike traditional FUSE where the file system daemon runs in userspace, the virtio-fs daemon runs on the host. A VIRTIO device carries FUSE messages and provides extensions for advanced features not available in traditional FUSE.

似乎需要 Qemu 5.0 才可以。


## virtio bus
- [ ] struct bus_type 的 match 和 probe 是什么关系?
```c
static inline int driver_match_device(struct device_driver *drv,
              struct device *dev)
{
  return drv->bus->match ? drv->bus->match(dev, drv) : 1;
}
```

device_attach 是提供给外部的一个常用的函数，会调用 `bus->probe`，在当前的上下文就是 pci_device_probe 了。

- [ ] pci_device_probe 的内容是很简单, 根据设备找驱动的地方在哪里？
  - 根据参数 struct device 获取 pc_driver 和 pci_device
  - 分配 irq number
  - 回调 `pci_driver->probe`, 使用 virtio_pci_probe 为例子
      - 初始化 pci 设备
      - 回调 `virtio_driver->probe`, 使用 virtnet_probe 作为例子

- [ ] virtio device 什么时候注册到 virtio bus 注册?
- [ ] virtio device 和设备什么时候匹配?


## 等待清理的部分

```c
static struct pci_driver virtio_pci_driver = {
  .name   = "virtio-pci",
  .id_table = virtio_pci_id_table,
  .probe    = virtio_pci_probe,
  .remove   = virtio_pci_remove,
#ifdef CONFIG_PM_SLEEP
  .driver.pm  = &virtio_pci_pm_ops,
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
  .feature_table      = features,
  .feature_table_size   = ARRAY_SIZE(features),
  .feature_table_legacy   = features_legacy,
  .feature_table_size_legacy  = ARRAY_SIZE(features_legacy),
  .driver.name      = KBUILD_MODNAME,
  .driver.owner     = THIS_MODULE,
  .id_table     = id_table,
  .probe        = virtblk_probe,
  .remove       = virtblk_remove,
  .config_changed     = virtblk_config_changed,
#ifdef CONFIG_PM_SLEEP
  .freeze       = virtblk_freeze,
  .restore      = virtblk_restore,
#endif
};
```

- [ ] 为什么会存在 virtio-pci 设备的存在，既然已经构建了一个 virtio_bus 的总线类型


- [ ] virtio_pci_probe
  - [ ] virtio_pci_modern_probe : 给 virtio_pci_device::vdev 注册


virtio_find_vqs

```c
/* Our device structure */
struct virtio_pci_device {
  struct virtio_device vdev;
  struct pci_dev *pci_dev;
```

```c
static const struct virtio_config_ops virtio_pci_config_ops = {
  .get    = vp_get,
  .set    = vp_set,
  .generation = vp_generation,
  .get_status = vp_get_status,
  .set_status = vp_set_status,
  .reset    = vp_reset,
  .find_vqs = vp_modern_find_vqs,
  .del_vqs  = vp_del_vqs,
  .get_features = vp_get_features,
  .finalize_features = vp_finalize_features,
  .bus_name = vp_bus_name,
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
  .queue_rq = virtio_queue_rq,
  .commit_rqs = virtio_commit_rqs,
  .complete = virtblk_request_done,
  .init_request = virtblk_init_request,
  .map_queues = virtblk_map_queues,
};
```

## 5.3 初始化 virtqueue
- virtio_pci_legacy_probe
  - `vp_dev->ioaddr = pci_iomap(pci_dev, 0, 0);` : the IO mapping for the PCI config space
    - [ ] 那么一个 virtio_dev 作为一个 pci 设备是有对应的配置空间的，是在哪里提供同意的访问方法的
    - 虽然 pci dev configuration space 访问起来是痛苦面具，但是 bar region 映射的空间还是很容易的
  - `vp_dev->setup_vq = setup_vq;` : 在这里初始化

接着上面分析 setup_vq 的调用位置:
- virtblk_probe
  - init_vq
    - 分配了 num_vqs 个 struct virtio_blk_vq, 就是对于 virtqueue 的封装吧
    - virtio_find_vqs
      - `vdev->config->find_vqs` : vdev 是 virtio_blk::vdev, virtio_device::virtio_config_ops 一共定义了三个 virtio_pci_config_ops , virtio_pci_config_nodev_ops, 在 virtio_pci_modern_probe/virtio_pci_legacy_probe 中间初始化, 这两个 probe 都是在 virtio_pci_probe 中间调用的
        - 从这个 probe 关系可以看到，virtio 下面是向下再次扩展一次类型的
        - 这里，从 parent 到 child 的初始化过程，到底是使用 modern pci 还是 legacy pci 是在 parent 已经初始化好了
      - vp_find_vqs : 使用这个作为例子
        - vp_find_vqs_msix
        - vp_find_vqs_intx
          - request_irq
          - vp_setup_vq
            - `vp_dev->setup_vq` : virtio_pci_device::setup_vq, 这个在 virtio_pci_legacy_probe 中间初始化
            - 使用 virtio_pci_legacy.c::setup_vq 作为例子
                - iowrite16(index, vp_dev->ioaddr + VIRTIO_PCI_QUEUE_SEL); // 告诉选择的数值是哪一个 queue
                - ioread16(vp_dev->ioaddr + VIRTIO_PCI_QUEUE_NUM); // 读 bar 0 约定的配置空间，得到 queue 的大小
                - vring_create_virtqueue
                   - 在这里，有一个参数 vp_nofify 作为 callback 函数
                   - vring_alloc_queue : 分配的页面是连续物理内存
                - iowrite32(q_pfn, vp_dev->ioaddr + VIRTIO_PCI_QUEUE_PFN); // 告诉 kvmtool，virtqueue 准备好了

```c
/* the notify function used when creating a virt queue */
bool vp_notify(struct virtqueue *vq)
{
  /* we write the queue's selector into the notification register to
   * signal the other end */
  iowrite16(vq->index, (void __iomem *)vq->priv);
  return true;
}
```

分析一下 kvmtool 的处理操作:
```c
static bool virtio_pci__data_out(struct kvm_cpu *vcpu, struct virtio_device *vdev,
         unsigned long offset, void *data, int size)
{
  case VIRTIO_PCI_QUEUE_PFN:
    val = ioport__read32(data);
    if (val) {
      virtio_pci__init_ioeventfd(kvm, vdev,
               vpci->queue_selector);
      vdev->ops->init_vq(kvm, vpci->dev, vpci->queue_selector,
             1 << VIRTIO_PCI_QUEUE_ADDR_SHIFT,
             VIRTIO_PCI_VRING_ALIGN, val);
    } else {
      virtio_pci_exit_vq(kvm, vdev, vpci->queue_selector);
    }
    break;
```
- [ ] ioeventfd 的使用

- `vdev->ops->init_vq`
  - kvmtool/virtio/blk.c:init_vq
    - virtio_get_vq : 转换获取 hva
    - vring_init : 将对应的数组初始化

### vring_size
参数 align 为 :
```c
/* The alignment to use between consumer and producer parts of vring.
 * x86 pagesize again. */
#define VIRTIO_PCI_VRING_ALIGN    4096
```
参数 num 是 VIRTIO_PCI_QUEUE_NUM, kvmtool 配置的是 16

```c
static inline unsigned vring_size(unsigned int num, unsigned long align)
{
  return ((sizeof(struct vring_desc) * num + sizeof(__virtio16) * (3 + num)
     + align - 1) & ~(align - 1))
    + sizeof(__virtio16) * 3 + sizeof(struct vring_used_elem) * num;
}
```
- [x] vring_size 含义在百度书上解释过，但是页面对齐，为什么不包含 vring_used 的
  - 从 vring_init 中间可以找到对称的数值

- [ ] vring_avail 和 vring_used 的成员都是只有两个(flags 和 idx), 为什么需要三个

### desc
如果从 multiqueue 到达了一个消息下来，那么 vring_avail 增加一个。
如果在 host 中间将任务完成了，那么 vring_used 增加一个, 在 host 通过中断的方式通知 guest 之后，guest 处理 vring_used ，并且释放
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
```

从 5.2 的描述看，vring_desc 会构成链表，描述一次 IO 的所有数据。

vring_avail 表示当前设备可以使用的 vring_desc, 这些数据从上层到达之后，vring_avail::idx++,
vring_avail::ring 描述的项目增加，如果被设备消费了，那么 last_avail_idx ++

设备每处理一个可用描述符数组 ring 的描述符链，都需要将其追加到 vring_used 数组中。

设备通过 vring_used 将告诉驱动那些数据被使用了。感觉这就是一个返回值列表罢了。

- [ ] idx 是谁维护的，两个 last_avail_idx 和 last_used_idx 是谁维护的

- [ ] 这些队列都是 guest 维护的吗?

## 5.4 驱动根据 I/O 请求组织描述符链

从 ldd3 的 sbull.c 的代码可以知道，queue_rq 就是上层数据到达的调用 block 设备的操作:
```c
static const struct blk_mq_ops virtio_mq_ops = {
  .queue_rq = virtio_queue_rq,
  .complete = virtblk_request_done,
  .init_request = virtblk_init_request,
  .map_queues = virtblk_map_queues,
};

/*
 * This comes first in the read scatter-gather list.
 * For legacy virtio, if VIRTIO_F_ANY_LAYOUT is not negotiated,
 * this is the first element of the read scatter-gather list.
 */
struct virtio_blk_outhdr {
  /* VIRTIO_BLK_T* */
  __virtio32 type;
  /* io priority. */
  __virtio32 ioprio;
  /* Sector (ie. 512 byte offset) */
  __virtio64 sector;
};
```
- virtio_queue_rq
  - 根据参数初始化 virtblk_req::virtio_blk_outhdr
  - blk_rq_map_sg : request 中间包含 bio 链表，每个 bio 中可能包含多个 bio_vec,  blk_rq_map_sg 将 bio 链表转换为 virtblk_req::scatterlist
  - virtblk_add_req
    - virtqueue_add_sgs
      - virtqueue_add : 根据模式选择下面中的一个
        - virtqueue_add_packed
        - virtqueue_add_split
          - 构造 vring_desc : desc[i].addr = cpu_to_virtio64(_vq->vdev, addr);
          - 设置 vring_avail : vq->split.vring.avail->idx = cpu_to_virtio16(_vq->vdev, vq->split.avail_idx_shadow);

- [ ] virtqueue

- [ ] virtqueue_add_outbuf

- [ ] vring_virtqueue :
  - 似乎划分为 split 和 pack 两种方法

- [ ] blk_mq_init_queue

- [ ] virt_queue 在内核端也是定义了的吗
  - [ ] 如何使用 ?
  - [ ] 驱动如何将 multiqueue 收到的消息整合成为 vring ?
  - virtblk_probe :初始化 `vblk->tag_set.ops = &virtio_mq_ops;`
     - [ ] virtio_queue_rq :
        - [ ] virtblk_add_req : 将 multiqueue 的 request 装换为

- [ ] guest 怎么知道使用真正的 设备驱动 还是 virt-blk 啊，跑一个 kvmtool 试试不就知道了

- [ ] 如果 virt 中间传输的地址是 HVA，但是到了 guest 的层次，其知道应该是 blk sector 啊!

## 5.6 驱动通知设备处理请求
virtio 标准在 virtio 设备的配置空间中，增加了一个 Queue Notify 寄存器，驱动准备好了 virtqueue 之后, 向 Queue Notify 寄存器发起写操作，
切换到 Host 状态中间。

- virtio_queue_rq
  - virtblk_add_req : 消息加入到队列中间
  - virtqueue_kick
    - virtqueue_kick_prepare
    - virtqueue_notify
      - `vq->notify`
        - vring_virtqueue::notify : 在 vring_alloc_queue 中间注册的

```c
/* the notify function used when creating a virt queue */
bool vp_notify(struct virtqueue *vq)
{
  /* we write the queue's selector into the notification register to
   * signal the other end */
  iowrite16(vq->index, (void __iomem *)vq->priv);
  return true;
}
```
现在从 kvmtool 的角度分析一下:

```c
static bool virtio_pci__data_out(struct kvm_cpu *vcpu, struct virtio_device *vdev,
         unsigned long offset, void *data, int size)
  case VIRTIO_PCI_QUEUE_NOTIFY:
    val = ioport__read16(data);
    vdev->ops->notify_vq(kvm, vpci->dev, val);
    break;
```
在 notify_vq 注册的内容是:
```c
static int notify_vq(struct kvm *kvm, void *dev, u32 vq)
{
  struct blk_dev *bdev = dev;
  u64 data = 1;
  int r;

  r = write(bdev->io_efd, &data, sizeof(data));
  if (r < 0)
    return r;

  return 0;
}
```

- [ ] 如果存在 eventfd 机制，因为通知方式是 MMIO，所以，其实内核可以在另一侧就通知出来这个事情，不需要在下上通知退出原因是什么位置的 MMIO

- virtio_blk_thread
  - `r = read(bdev->io_efd, &data, sizeof(u64));` ： 阻塞到 eventfd 上
  - virtio_blk_do_io
    - virt_queue__available
    - virtio_blk_do_io_request
      - disk_image__read
      - virtio_blk_complete

## 5.7 驱动侧回收IO请求
发送中断的位置在:
- virtio_blk_complet
  - `bdev->vdev.ops->signal_vq(req->kvm, &bdev->vdev, queueid);` : 其中的 vdev.ops 表示一个 virtio_device 的通用技能，因为为了区分 MMIO 和 PCI, 感觉可以完全忽略这个东西
    - `kvm__irq_trigger(kvm, vpci->legacy_irq_line);` : 好几种触发方式的一种, 还有 msix 之类的，但是 百度书 上使用 legacy_irq_line 的方式

- [x] 这个中断号感觉就是非常随意的分配了一下而已，之后，guest kernel 怎么知道给谁注册, 什么时候

- 这些消息都是放到 : kvmtool/x86/mptable.c 中间的，之后 guest kernel 启动的时候，这些数据就可以静态的确定了


在 vp_find_vqs_intx 中间注册的函数:
```c
  rr = request_irq(vp_dev->pci_dev->irq, vp_interrupt, IRQF_SHARED,
      dev_name(&vdev->dev), vp_dev);
```
handler 是 vp_interrupt
```c
irqreturn_t vring_interrupt(int irq, void *_vq)
{
  struct vring_virtqueue *vq = to_vvq(_vq);

  if (!more_used(vq)) {
    pr_debug("virtqueue interrupt with no work for %p\n", vq);
    return IRQ_NONE;
  }

  if (unlikely(vq->broken))
    return IRQ_HANDLED;

  pr_debug("virtqueue callback for %p (%p)\n", vq, vq->vq.callback);
  if (vq->vq.callback)
    vq->vq.callback(&vq->vq);

  return IRQ_HANDLED;
}
```
这里的 callback 函数的注册路径和 notify_vq 相同，在调用链中间漫长的流转，在 blk 中间，是: virtblk_done

- virtblk_done :将 IO 返回到上层, last_used_idx ++, 释放 vring_used 之类的事情
  - virtqueue_get_buf
  - blk_mq_complete_request

## 5.8 设备异步处理 IO
同步进行的方式是 : vp_notify 导致 guest vmexit, 然后在 host 中间处理，处理完成，发出中断，同时进入 guest, 在 guest 中间接受中断.
异步进行的方式 : 将 vp_nofify 通知出去之后，立刻重新进入到 host 中间去。

## 5.9 轻量虚拟机退出

这就是 eventfd 机制了
