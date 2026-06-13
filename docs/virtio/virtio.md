## virtio split ring 结构
<!-- ce14ccf6-c6a1-4d6c-b404-4b096cc8a2d2 -->

- https://blogs.oracle.com/linux/introduction-to-virtio-part-2-vhost

它分成 3 块：
+----------------------+
|   Descriptor Table   |
+----------------------+
| desc[0] -> buf A     |
| desc[1] -> buf B     |
| desc[2] -> buf C     |
| ...                  |
+----------------------+
+----------------------+
|      Avail Ring      |   guest 写
+----------------------+
| idx = 3              |
| ring[0] = 0          |
| ring[1] = 1          |
| ring[2] = 2          |
+----------------------+
+----------------------+
|      Used Ring       |   device 写
+----------------------+
| idx = 2              |
| ring[0] = 0          |
| ring[1] = 1          |
+----------------------+


过程图

guest:
  1. 填 desc[0], desc[1], desc[2]
  2. 把 0,1,2 放进 avail ring
  3. 通知 device
device:
  4. 从 avail ring 看到 0,1,2 可用
  5. 按 desc 去访问 buffer
  6. 做完后把 0,1,2 放进 used ring
  7. 通知 guest

所以 split ring 的感觉就是：
Descriptor Table   = 货物详情
Avail Ring         = 待处理列表
Used Ring          = 已处理列表


virtio queue 中的 desc queue 被 avail 和 used queue 索引
- desc queue 只有 driver 才可以写。
- avail queue driver 写
- used queue 用于 device 告诉 driver ，哪些 desc ready 了

virtio，甚至说，设备的工作基本模型是，driver (虚拟机) 提供来提供内存，这些内存填充到
desc table 中。也就是无论 guest 给 device 发消息还是 device 给 guest 发消息，内存都是 guest
来提前准备的。

这三个 queue 都是只有一个 writer ，所以很容易同步。
也就是 guest 来写 desc table 和 avail ring ，而 device 来写 used ring 。

队列大小 4，假设：
- guest 已提交数 = avail.idx
- device 已消费数 = last_avail_idx (device 维护，但是为什么不是 last used 来维护)

- guest 这边也是靠自己的 last_used_idx 去判断哪些完成项是新的。


真的是这样吗? 关于 last_avail_idx 和 used.idx 的关系为:
```txt
假设 guest 一次提交了 4 个请求：

avail.idx = 4
used.idx = 0

device 很快把这 4 个请求都从 avail ring 里读出来了，于是它内部变成：

last_avail_idx = 4

但这 4 个请求还在处理中，只完成了 1 个，于是共享内存里是：

used.idx = 1

这时就出现了：

last_avail_idx = 4
used.idx = 1
```

补充一下，那么如果 device 想要给 guest 发送消息，那么是什么样子的?
qemu 中 virtio balloon 有一个经典的例子，也就是说，有一段内存本来就是共享的，
然后发送中断就可以了:
```c
static void virtio_balloon_free_page_stop(VirtIOBalloon *s)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(s);

    if (s->free_page_hint_status != FREE_PAGE_HINT_S_STOP) {
        /*
         * The lock also guarantees us that the
         * virtio_ballloon_get_free_page_hints exits after the
         * free_page_hint_status is set to S_STOP.
         */
        qemu_mutex_lock(&s->free_page_lock);
        /*
         * The guest isn't done hinting, so send a notification
         * to the guest to actively stop the hinting.
         */
        s->free_page_hint_status = FREE_PAGE_HINT_S_STOP;
        qemu_mutex_unlock(&s->free_page_lock);
        virtio_notify_config(vdev);
    }
}
```
内核中的响应函数:
```c
void virtio_config_changed(struct virtio_device *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->config_lock, flags);
	__virtio_config_changed(dev);
	spin_unlock_irqrestore(&dev->config_lock, flags);
}
```

### 内核数据结构:
- struct vring_virtqueue
  - struct virtqueue
  - struct vring_virtqueue_split
    - struct vring vring 🌕 核心
    	- unsigned int num
      - vring_desc_t *desc : vring_desc 会构成链表，描述一次 IO 的所有数据
        	- __virtio64 addr
          - __virtio32 len
          - __virtio16 flags
          - __virtio16 next
      - vring_avail_t *avail
          -	__virtio16 flags
          - __virtio16 idx
          - __virtio16 ring[]
      - vring_used_t *used
          - __virtio16 flags
          - __virtio16 idx
          - vring_used_elem_t ring[]
            - __virtio32 id
            - __virtio32 len
  - struct vring_virtqueue_packed

以 virtio-blk 为例，如果从 multiqueue 到达了一个消息下来，那么 vring_avail 增加一个。
如果在 host 中间将任务完成了，那么 vring_used 增加一个,
在 host 通过中断的方式通知 guest 之后，guest 处理 vring_used ，并且释放

进一步的细节:
- vring_virtqueue::notify 一般赋值为 vp_notify
- vring_virtqueue::last_used_idx : 不理解
- virtqueue::callback，对于 virtio-balloon 而言，是 balloon_ack

- [ ] virtqueue 的定位是什么?

- 注意 一个 vring_desc 不是对应一个 page 的

- 从 virtblk_add_req 分析来看 driver 如何发送请求
  - sg_init_one 将 virtblk_req::out_hdr virtblk_req::in_hdr 组装
  - virtqueue_add_sgs
    - virtqueue_add
      - virtqueue_add_split
        - vring_map_one_sg : 将 sg 转换为
        - virtqueue_add_desc_split : 将 vq 中的东西写入到 vring_desc_t 中
        - 调整 vring_avail_t

- virtqueue_get_buf
  - virtqueue_get_buf_ctx : 参数 virtqueue len
    - to_vvq : 从 virtqueue 获取 vring_virtqueue
    - virtqueue_get_buf_ctx_packed
    - virtqueue_get_buf_ctx_split : 访问 vring::used 和 vring_virtqueue::last_used_idx，获取灌注的内容


### QEMU 数据结构 VirtQueue
<!-- fdeb5532-0c54-436f-9abe-36fa07d8dec0 -->

- struct VirtQueue
	- EventNotifier guest_notifier;
	- EventNotifier host_notifier;
	- VRing 🌕 核心
		- VRingAvail
		- VRingDesc
		- VRingUsed

这里的，VRing 包含 VRingAvail VRingDesc VRingUsed 三个结构体是有一些技巧的
需要考虑地址空间的问题。

### VirtQueueElement
<!-- f8615206-a09e-4768-b4ec-a1727c38a618 -->

```c
typedef struct VirtQueueElement
{
    unsigned int index;
    unsigned int len;
    unsigned int ndescs;
    unsigned int out_num;
    unsigned int in_num;
    /* Element has been processed (VIRTIO_F_IN_ORDER) */
    bool in_order_filled;
    hwaddr *in_addr;
    hwaddr *out_addr;
    struct iovec *in_sg;
    struct iovec *out_sg;
} VirtQueueElement;
```
VirtQueueElement 是对于 VRingDesc 中元素的封装。细节如下:
1. virtqueue_split_pop 中 通过 vring_split_desc_read 和 virtqueue_split_read_next_desc 将链表构建起来的 desc 转换到 `VirtQueueElement` 的数组中。
2. virtio_scsi_handle_cmd_vq 中 先调用 virtio_scsi_pop_req -> virtqueue_split_pop 组装获取到 VirtIOSCSIReq (其中第一个元素是 VirtQueueElement)
然后通过 virtio_scsi_handle_cmd_req_submit 真的去处理

VirtQueueElement::in_num 和 VirtQueueElement::out_num 是什么东西
- out_sg / out_num：guest 提供给 device 读取的数据
- in_sg / in_num：guest 提供给 device 写回的数据缓冲区

hw/virtio/virtio-balloon.c 中的 get_free_page_hints 为例子:

```txt
if (elem->out_num) {
    uint32_t id;
    ...
}

意思是：

- 这个 vring 元素里带了一个 guest -> device 的输出 buffer
- QEMU 从 out_sg 里读一个 uint32_t id
- 这个 id 是 free-page-hint 的 command/session id
- 如果这个 id 和当前 host 在 config 里发布的 free_page_hint_cmd_id 一致，就说明 guest 正在响应这一次 hint 请求，于是把状态从 REQUESTED 切到 START

if (elem->in_num && dev->free_page_hint_status == FREE_PAGE_HINT_S_START) {
    for (i = 0; i < elem->in_num; i++) {
        qemu_guest_free_page_hint(elem->in_sg[i].iov_base,
                                  elem->in_sg[i].iov_len);
    }
}

意思是：

- 这个 vring 元素里带了若干个 device 可访问的 in buffer
- 对 free-page-hint 这个队列来说，这些 in buffer 不是“device 要回写结果”，而是 guest 把“这些页是 free page”的内存段挂出来，让 host 直接看
- QEMU 对每个 in_sg[i] 调 qemu_guest_free_page_hint()，把这段 guest 内存对应的页从 migration dirty bitmap 里清掉
```

### qemu 提交元素
- virtqueue_push
  - virtqueue_split_fill

## virtio packed ring 结构
<!-- 0a8c1860-549b-4666-9d5d-b2f76d3a9714 -->

先找文档，解决为什么问题，相比较而言，差别是什么?

## 核心的结构体

### virtio_config_ops
- virtio_config_ops : **操作配置空间的** ， 是 virtio_device 和 virtio 连接的桥梁。
  - get/set : 读写 config
  - [ ] find_vqs :
  - [ ] set_vq_affinity : 一般初始化为 vp_set_vq_affinity
    - [ ] 软中断是如何设置 affinity 的

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


## 细节
- [ ] 有的设备不支持 PCI 总线，需要使用 MMIO 的方式，但是 kvmtool 怎么知道这个设备需要使用 MMIO
- [ ] 约定是第一个 bar 指向的 IO 空间在内核那一侧是怎么分配的 ?
- [ ] virtio_bus 是挂载到哪里的?
- [ ] virtio_console 的具体实现是怎么样子的 ?
- [ ] QEMU 是如何初始化 virtio 设备的
- [ ] 热插拔，参考 virtio-mem 吧
- [ ] 如何协商 vq 的数量，有时候 vq 的数量取决于是否打开一些 feature 的?

- [ ] 的确是记得没有错，但是 `virtio_pci_common_cfg` 是处理 pci 的，但是不知道为什么是错的

- [ ] vp_find_vqs_intx : 为什么 balloon 总是调用 int 而不是 msi ，而且只有 balloon 会调用这个

- ret_from_fork
  - kernel_init
    - kernel_init_freeable
      - do_basic_setup
        - do_initcalls
          - do_initcall_level
            - do_one_initcall
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
                                    - virtballoon_probe
                                      - init_vqs
                                        - virtio_find_vqs
                                          - vp_modern_find_vqs
                                            - vp_find_vqs
                                              - vp_find_vqs_intx

1. 勉强看看
https://github.com/rust-vmm/vm-virtio/blob/main/virtio-queue/README.md

2. 原来文档就没这么点啊
  - https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html

## vring_size
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

- [ ] vp_setup_vq 是需要和 vp_find_vqs_intx 下调用的，和中断是关系的吗?

- 通过 vp_active_vq 将 vring_desc , vring_avail 和 vring_used 的地址发送给 QEMU 的。

## desc

vring_avail 表示当前设备可以使用的 vring_desc, 这些数据从上层到达之后，vring_avail::idx++,
vring_avail::ring 描述的项目增加，如果被设备消费了，那么 last_avail_idx ++

设备每处理一个可用描述符数组 ring 的描述符链，都需要将其追加到 vring_used 数组中。

设备通过 vring_used 将告诉驱动那些数据被使用了。感觉这就是一个返回值列表罢了。

- [ ] idx 是谁维护的，两个 last_avail_idx 和 last_used_idx 是谁维护的

- 在设备侧定义 last_avail_idx ，在驱动侧定义 last_used_idx
  - vring_virtqueue::last_used_idx 例如， virtblk_done -> virtqueue_get_buf 的时候会更新的，因为设备处理数据完成。

- [ ] last_avail_idx 和 last_used_idx 都是队列的一部分，总是需要和另一个共享吧，不然队列直接被毁掉了，如何处理?

## vring_used_elem::len 是什么作用的

```txt
#0  virtqueue_get_buf_ctx_split (ctx=0x0 <fixed_percpu_data>, len=0xffffc90000003f1c, _vq=0xffff888140418e00) at drivers/virtio/virtio_ring.c:790
#1  virtqueue_get_buf_ctx (_vq=0xffff888140418e00, len=len@entry=0xffffc90000003f1c, ctx=ctx@entry=0x0 <fixed_percpu_data>) at drivers/virtio/virtio_ring.c:2282
#2  0xffffffff81722f27 in virtqueue_get_buf (_vq=<optimized out>, len=len@entry=0xffffc90000003f1c) at drivers/virtio/virtio_ring.c:2288
#3  0xffffffff819766ba in virtblk_done (vq=0xffff888140418e00) at drivers/block/virtio_blk.c:283
#4  0xffffffff81722f85 in vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2462
#5  vring_interrupt (irq=<optimized out>, _vq=<optimized out>) at drivers/virtio/virtio_ring.c:2437
#6  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888140458400) at kernel/irq/handle.c:158
#7  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888140458400) at kernel/irq/handle.c:193
#8  handle_irq_event (desc=desc@entry=0xffff888140458400) at kernel/irq/handle.c:210
#9  0xffffffff81169e0a in handle_edge_irq (desc=0xffff888140458400) at kernel/irq/chip.c:819
#10 0xffffffff810b9a14 in generic_handle_irq_desc (desc=0xffff888140458400) at include/linux/irqdesc.h:158
#11 handle_irq (regs=<optimized out>, desc=0xffff888140458400) at arch/x86/kernel/irq.c:231
#12 __common_interrupt (regs=<optimized out>, vector=36) at arch/x86/kernel/irq.c:250
#13 0xffffffff81efa443 in common_interrupt (regs=0xffffc90001027998, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
```
- 修改 vring_used_elem::len 的位置只有在 `vhost_net` 和 `virtqueue_get_buf_ctx_split` 中，`virtqueue_get_buf_ctx_split` 的调用是非常频繁的，看来 vring_used_elem::len 总是在被 host 更新
  - 而且 vhost_net 本来就是 host 的代码，所以更加说明这是 host 告诉 guest 的。
  - 的确是设备写回到 guest 驱动的数据。


- [ ] 为什么 vring_avail 没有这个 length
- [ ] 难道不能从 vring_desc 中获取吗?

[^4]: [Standardizing virtio](https://lwn.net/Articles/580186/)

## 细节
1. 如何理解 virtio config
- 之前一直以为是类似 PCI 配置空间，但是似乎不是
  - qemu 中 virtio_balloon_get_config 是做什么的

3. 为什么感觉两个 config 是不一样的
  - 一个 balloon 的 config
  - 一个是 PCI 的 config

1. 如何理解 virtio_rmb

2. 整理一下
https://www.cnblogs.com/LoyenWang/p/14589296.html


1. 显然，用这个去理解 virtio 就是很好的，这种测试工具
tools/virtio/

3. 这个东西很有想象力
drivers/net/caif/caif_virtio.c

1. 使用 bcc 的工具 virtiostat

2. qemu : 三个设备之间的关系
virtio-blk-pci vs virtio-blk-device vs virtio-blk
显示的是 vda，所以 virtio-blk-pci 应该和 -drive 中 if=virtio 等价吧

3. 有办法把提供的 virtio-blk 是基于 mmio 的吗?
  - 可以的，需要看看如何配置

## 如何理解这里的 Device-independent features
https://github.com/rcore-os/virtio-drivers

xen 也是需要搞 iommu 么?
drivers/xen/grant-dma-iommu.c

然后找到了这个，看来是需要 nvme 启动的?
https://lists.xen.org/archives/html/xen-devel/2022-06/msg00820.html


## 如何理解 `vq[i].vring.num`

一个 vq 一个 vring ，还是一个 vq 有多个 vring ?
也就是 multiqueue 的实现是通过构建多个 VirtQueue 实现的？
```c
static void virtio_memory_listener_commit(MemoryListener *listener)
{
    VirtIODevice *vdev = container_of(listener, VirtIODevice, listener);
    int i;

    for (i = 0; i < VIRTIO_QUEUE_MAX; i++) {
        if (vdev->vq[i].vring.num == 0) {
            break;
        }
        virtio_init_region_cache(vdev, i);
    }
}
```

## 这个结构体真的让人陌生啊
```c
typedef struct VirtIOBlockReq {
    VirtQueueElement elem;
    int64_t sector_num;
    VirtIOBlock *dev;
    VirtQueue *vq;
    IOVDiscardUndo inhdr_undo;
    IOVDiscardUndo outhdr_undo;
    struct virtio_blk_inhdr *in;
    struct virtio_blk_outhdr out;
    QEMUIOVector qiov;
    size_t in_len;
    struct VirtIOBlockReq *next;
    struct VirtIOBlockReq *mr_next;
    BlockAcctCookie acct;
} VirtIOBlockReq;
```

有趣 https://blog.stephenmarz.com/2020/11/11/risc-v-os-using-rust-graphics/
在自己的 os 中实现一个 vring 的结构

## 注意，hmp 中存在如下命令

```txt
info virtio  -- List all available virtio devices
info virtio-queue-element path queue [index] -- Display element of a given virtio queue
info virtio-queue-status path queue -- Display status of a given virtio queue
info virtio-status path -- Display status of a given virtio device
info virtio-vhost-queue-status path queue -- Display status of a given vhost queue
```

## VirtQueueElement

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
