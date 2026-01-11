## KVM 中断注入
<!-- 7216915c-74d6-4a92-af37-25502a5aa006 -->

一共存在这些方法，我们关注的重点当然是通过 eventfd 来注入

| ioctl               | 解释                                                                     |
|---------------------|--------------------------------------------------------------------------|
| KVM_IRQFD           | 基于 eventfd : host 给 guest 注入中断，之后使用 ioeventfd 来通知就可以了 |
| KVM_IOEVENTFD       | 基于 eventfd : guest 事件通知 host                                       |
| KVM_SIGNAL_MSI      | host 直接给注入 msi (no gsi)                                                      |
| KVM_IRQ_LINE        | host 直接给注入普通中断 (gsi)                                                  |
| KVM_SET_GSI_ROUTING | 中断路由的配置                                                           |
| KVM_INTERRUPT       | 直接注入中断到 CPU 中，当在用户态的中断模拟的时候的时候使用              |


QEMU 和 kvm : ioeventfd 和 irqfd 来实现设备的事件，这是最高效的，例如 virtio 的封装:
```c
struct VirtQueue {
    // ...
    EventNotifier guest_notifier; // 告知 guest
    EventNotifier host_notifier; // 告知 qemu
    // ...
}
```

关联的内核文件为:
- fs/eventfd.c
- virt/kvm/eventfd.c

- ioeventfd 对于 fd 进行 epoll
- irqfd 在 qemu 中调用 event_notifier_set

## 1: qemu 通过 ioeventfd 来接受 Guest OS 的通知
<!-- 78670593-1ba7-4f4d-817a-b33af7b7b5cd -->

### 普通的用户使用

KVM_EXIT_IO 和 KVM_EXIT_MMIO ，从 kvm 退出之后，同时提供
io 的地址和需要执行的内容

- 所以，我们很容易理解为什么 ioeventfd 更加快，内核除了这个方法，没有
更好的方法来通知用户态，触发 vCPU thread 退出来执行任务，那就开销太高了。

### 基本执行流程
内核的执行流程:
```txt
@[
    ioeventfd_write+5
    __kvm_io_bus_write+137
    kvm_io_bus_write+90
    handle_ept_misconfig.part.0+52
    vmx_handle_exit+1988
    vcpu_enter_guest.constprop.0+1613
    kvm_arch_vcpu_ioctl_run+855
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 6848
```

qemu 中处理的流程:
```txt
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_dispatch_handlers
                      - aio_dispatch_handler
                        - virtio_queue_notify_vq
                          - virtio_blk_handle_output
```

这里封装的相当好，在 virtio_queue_aio_attach_host_notifier 中，
给 aio_set_event_notifier 注册上 virtio_queue_host_notifier_aio_poll_ready 就可以了:
```c
static void virtio_queue_host_notifier_aio_poll_ready(EventNotifier *n)
{
    VirtQueue *vq = container_of(n, VirtQueue, host_notifier);

    virtio_queue_notify_vq(vq);
}
```

### ioeventfd 需要感知 memory region 的变化，从而让内核知道通知哪一个 eventfd

- memory_region_write_accessor
  - virtio_pci_common_write
    - virtio_pci_start_ioeventfd
      - virtio_bus_start_ioeventfd
        - virtio_blk_start_ioeventfd ( virtio blk 注册的 start)
          - memory_region_transaction_commit
            - memory_region_transaction_commit
              - address_space_update_ioeventfds
                - address_space_update_ioeventfds
                  - address_space_add_del_ioeventfds
                    - kvm_mem_ioeventfd_add
                      - kvm_set_ioeventfd_mmio

ioeventfd 的作用是，当 guest os 的 virtio queue kick ，vmexit 之后，会通过写 ioeventfd 来告知 qemu
很显然，当 memory region 变化之后，注册的 ioeventfd 的检测范围会变化，所以需要重新注册。

在内核中的 __kvm_io_bus_write 中，可以看到内核根据数组选择注册的范围。

### 不同的 virtio 设备开启 ioeventfd 的方式不同

所以，他们注册不同的 hook 来:
```c
static void virtio_blk_class_init(ObjectClass *klass, const void *data)
{
    // ...
    vdc->start_ioeventfd = virtio_blk_start_ioeventfd;
    vdc->stop_ioeventfd = virtio_blk_stop_ioeventfd;
}
```

memory_region_add_eventfd

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - flatview_write_continue_step
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - virtio_pci_common_write
                            - virtio_pci_start_ioeventfd
                              - virtio_bus_start_ioeventfd
                                - virtio_blk_start_ioeventfd


## 2:  qemu 通过来向 Guest OS irqfd 注入中断
<!-- 88680604-56f8-480d-b3d1-de23cfa2522a -->

虽然 qemu 可以注入中断一路到到 apic 的驱动实现中，但是这样其实降低了 qemu 的速度。
假如， QEMU 一个核来做 virtio ，一个核来 vCPU ，使用了 irqfd 之后，
其实中断注入的后半部分就给到第三个物理 CPU 来执行了。

### 为什么有时候观察不到 irqfd 的使用

不是太理解，为什么 qemu_in_iothread 来决定如何使用他们的差别
```c
static void virtio_blk_req_complete(VirtIOBlockReq *req, unsigned char status)
{
    VirtIOBlock *s = req->dev;
    VirtIODevice *vdev = VIRTIO_DEVICE(s);

    trace_virtio_blk_req_complete(vdev, req, status);

    stb_p(&req->in->status, status);
    iov_discard_undo(&req->inhdr_undo);
    iov_discard_undo(&req->outhdr_undo);
    virtqueue_push(req->vq, &req->elem, req->in_len);
    if (qemu_in_iothread()) {
        virtio_notify_irqfd(vdev, req->vq);
    } else {
        virtio_notify(vdev, req->vq);
    }
}
```

哦，原来这个是 virtio-blk 的原因，似乎也不容易理解:
```txt
History:        #0
Commit:         bfa36802d1704fc413c590ebdcc4e5ae0eacf439
Author:         Stefan Hajnoczi <stefanha@redhat.com>
Committer:      Kevin Wolf <kwolf@redhat.com>
Author Date:    Tue 23 Jan 2024 01:26:25 AM CST
Committer Date: Thu 08 Feb 2024 04:37:33 PM CST

virtio-blk: avoid using ioeventfd state in irqfd conditional

Requests that complete in an IOThread use irqfd to notify the guest
while requests that complete in the main loop thread use the traditional
qdev irq code path. The reason for this conditional is that the irq code
path requires the BQL:

  if (s->ioeventfd_started && !s->ioeventfd_disabled) {
      virtio_notify_irqfd(vdev, req->vq);
  } else {
      virtio_notify(vdev, req->vq);
  }

There is a corner case where the conditional invokes the irq code path
instead of the irqfd code path:

  static void virtio_blk_stop_ioeventfd(VirtIODevice *vdev)
  {
      ...
      /*
       * Set ->ioeventfd_started to false before draining so that host notifiers
       * are not detached/attached anymore.
       */
      s->ioeventfd_started = false;

      /* Wait for virtio_blk_dma_restart_bh() and in flight I/O to complete */
      blk_drain(s->conf.conf.blk);

During blk_drain() the conditional produces the wrong result because
ioeventfd_started is false.

Use qemu_in_iothread() instead of checking the ioeventfd state.

Cc: qemu-stable@nongnu.org
Buglink: https://issues.redhat.com/browse/RHEL-15394
Signed-off-by: Stefan Hajnoczi <stefanha@redhat.com>
Message-ID: <20240122172625.415386-1-stefanha@redhat.com>
Reviewed-by: Kevin Wolf <kwolf@redhat.com>
Signed-off-by: Kevin Wolf <kwolf@redhat.com>
```

在 virito-scsi 中就不是这个东西:

virtio_scsi_complete_req 中:
```c
    if (s->dataplane_started && !s->dataplane_fenced) {
        virtio_notify_irqfd(vdev, vq);
    } else {
        virtio_notify(vdev, vq);
    }

```


### 不使用 irqfd 的中断注入流程
<!-- 475c6769-ea8f-4001-bd87-5c4339209a0e -->

QEMU 中:

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_bh_poll
                      - blk_aio_complete_bh
                        - blk_aio_complete
                          - blk_aio_complete
                            - virtio_blk_rw_complete
                              - address_space_stl_le
                                - address_space_stl_internal
                                  - memory_region_dispatch_write
                                    - access_with_adjusted_size
                                      - memory_region_write_accessor
                                        - kvm_apic_mem_write
                                          - kvm_send_msi
                                            - kvm_irqchip_send_msi

```txt
@[
    vmx_deliver_interrupt+5
    __apic_accept_irq+248
    kvm_irq_delivery_to_apic_fast+332
    kvm_irq_delivery_to_apic+103
    kvm_set_msi+146
    kvm_send_userspace_msi+120
    kvm_vm_ioctl+2280
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 42699
```
这个流程图看上去就很容易了，而且这个就是模拟硬件的行为，硬件想要发送中断，那么就写入 lapic mmio 的空间
写入的地址和数据来控制到底到那个 CPU 和 vector 中:

```c
static void kvm_apic_mem_write(void *opaque, hwaddr addr,
                               uint64_t data, unsigned size)
{
    MSIMessage msg = { .address = addr, .data = data };

    kvm_send_msi(&msg);
}
```
设备想要触发那个中断，就从 msix-table 中取出 address / data 来执行
所以，想要控制中断亲和性，那么虚拟机中修改 msix-table ，然后 qemu 记录，
之后查询 msix-table ，就可以自动处理了

### 使用 irqfd 基本流程
```c
void virtio_blk_req_complete(VirtIOBlockReq *req, unsigned char status)
{
    VirtIOBlock *s = req->dev;
    VirtIODevice *vdev = VIRTIO_DEVICE(s);

    trace_virtio_blk_req_complete(vdev, req, status);

    stb_p(&req->in->status, status);
    iov_discard_undo(&req->inhdr_undo);
    iov_discard_undo(&req->outhdr_undo);
    virtqueue_push(req->vq, &req->elem, req->in_len);
    if (qemu_in_iothread()) {
        virtio_notify_irqfd(vdev, req->vq);
    } else {
        virtio_notify(vdev, req->vq);
    }
}
```
qemu 注册的基本流程:

- __clone3
  - start_thread
    - qemu_thread_start
      - iothread_run
        - aio_poll
          - aio_bh_poll
            - blk_aio_complete_bh
              - blk_aio_complete
                - blk_aio_complete
                  - virtio_blk_rw_complete
                    - virtio_blk_req_complete

内核的接受流程:
```txt
@[
    irqfd_wakeup+5
    __wake_up_common+117
    eventfd_write+201
    vfs_write+267
    ksys_write+110
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 8227
```

## gsi 简述
<!-- 62c576a1-7859-4e9a-b126-57f49f291c3f -->

对于我们关心的高速路径:
1. KVM_SET_GSI_ROUTING (配置 gsi 实际上会指向那些内容)
2. KVM_IRQFD (配置 eventfd 和 gsi 的关系)

```c
struct kvm_irqfd {
	__u32 fd;
	__u32 gsi;
	__u32 flags;
	__u32 resamplefd;
	__u8  pad[16];
};
```

```c
struct kvm_irq_routing_entry {
	__u32 gsi;
	__u32 type;
	__u32 flags;
	__u32 pad;
	union {
		struct kvm_irq_routing_irqchip irqchip;
		struct kvm_irq_routing_msi msi;
		struct kvm_irq_routing_s390_adapter adapter;
		struct kvm_irq_routing_hv_sint hv_sint;
		struct kvm_irq_routing_xen_evtchn xen_evtchn;
		__u32 pad[8];
	} u;
};
```
kvm_irq_routing_entry::type 中的取值
```c
/* gsi routing entry types */
#define KVM_IRQ_ROUTING_IRQCHIP 1
#define KVM_IRQ_ROUTING_MSI 2
#define KVM_IRQ_ROUTING_S390_ADAPTER 3
#define KVM_IRQ_ROUTING_HV_SINT 4
#define KVM_IRQ_ROUTING_XEN_EVTCHN 5
```

内核中的表述:
```c
struct kvm_irq_routing_table {
	int chip[KVM_NR_IRQCHIPS][KVM_IRQCHIP_NUM_PINS];
	u32 nr_rt_entries;
	/*
	 * Array indexed by gsi. Each entry contains list of irq chips
	 * the gsi is connected to.
	 */
	struct hlist_head map[] __counted_by(nr_rt_entries);
};
```


```diff
diff --git a/accel/kvm/kvm-all.c b/accel/kvm/kvm-all.c
index f85eb42d78b8..2cc04ca84171 100644
--- a/accel/kvm/kvm-all.c
+++ b/accel/kvm/kvm-all.c
@@ -2072,6 +2072,15 @@ void kvm_irqchip_commit_routes(KVMState *s)
     s->irq_routes->flags = 0;
     trace_kvm_irqchip_commit_routes();
     ret = kvm_vm_ioctl(s, KVM_SET_GSI_ROUTING, s->irq_routes);
+    for (size_t i = 0; i < s->irq_routes->nr; i++) {
+      struct kvm_irq_routing_entry *r = &s->irq_routes->entries[i];
+      printf("gsi=%d type=%d ", r->gsi, r->type);
+      if (r->type == KVM_IRQ_ROUTING_IRQCHIP)
+        printf("%d %d\n", r->u.irqchip.irqchip, r->u.irqchip.pin);
+      else
+        printf("\n");
+    }
+    printf("===================\n");
     assert(ret == 0);
 }
```

可以获取如下输出:

不难看到，
1. 这里考虑到了两个 pic 控制器，每一个 pic 一共有 8 个 pin ，
2. 一个 ioapic 有 24 个 pin
3. 可以看到 gsi 0 同时连接到了 pic 和 ioapic
4. type=2 的，也就是 KVM_IRQ_ROUTING_MSI 并不会和 pic / ioapic 重叠，也就是 gsi 调用多个路线完全就是给 legacy 中断控制器使用的
```txt
gsi=0 type=1 0 0
gsi=1 type=1 0 1
gsi=3 type=1 0 3
gsi=4 type=1 0 4
gsi=5 type=1 0 5
gsi=6 type=1 0 6
gsi=7 type=1 0 7
gsi=8 type=1 1 0
gsi=9 type=1 1 1
gsi=10 type=1 1 2
gsi=11 type=1 1 3
gsi=12 type=1 1 4
gsi=13 type=1 1 5
gsi=14 type=1 1 6
gsi=15 type=1 1 7
gsi=0 type=1 2 2
gsi=1 type=1 2 1
gsi=3 type=1 2 3
gsi=4 type=1 2 4
gsi=5 type=1 2 5
gsi=6 type=1 2 6
gsi=7 type=1 2 7
gsi=8 type=1 2 8
gsi=9 type=1 2 9
gsi=10 type=1 2 10
gsi=11 type=1 2 11
gsi=12 type=1 2 12
gsi=13 type=1 2 13
gsi=14 type=1 2 14
gsi=15 type=1 2 15
gsi=16 type=1 2 16
gsi=17 type=1 2 17
gsi=18 type=1 2 18
gsi=19 type=1 2 19
gsi=20 type=1 2 20
gsi=21 type=1 2 21
gsi=22 type=1 2 22
gsi=23 type=1 2 23
gsi=2 type=2
gsi=24 type=2
gsi=25 type=2
gsi=26 type=2
gsi=27 type=2
gsi=28 type=2
gsi=29 type=2
gsi=30 type=2
gsi=31 type=2
gsi=32 type=2
gsi=33 type=2
gsi=34 type=2
gsi=35 type=2
gsi=36 type=2
gsi=37 type=2
gsi=38 type=2
gsi=39 type=2
gsi=40 type=2
gsi=41 type=2
gsi=42 type=2
gsi=43 type=2
gsi=44 type=2
gsi=45 type=2
gsi=46 type=2
gsi=47 type=2
gsi=48 type=2
gsi=49 type=2
gsi=50 type=2
gsi=51 type=2
gsi=52 type=2
gsi=53 type=2
gsi=54 type=2
gsi=55 type=2
gsi=56 type=2
gsi=57 type=2
gsi=58 type=2
gsi=59 type=2
gsi=60 type=2
gsi=61 type=2
gsi=62 type=2
gsi=63 type=2
```

使用 crash 可以观察到(2026-01-10 具体咋看到的，我已经忘记了:)
```c
struct kvm_irq_routing_table {
  chip = {
    {0, 1, -1, 3, 4, 5, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1, -1, -1, 8, 9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1},
    {-1, 1, 0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23}
  },
  nr_rt_entries = 34,
  map = 0xffff88810593bd28
}
```
1. kvm_irq_routing_table::gsi 是用于反向查询的，查询 pin 连接到哪一个 gsi 的
2. `map[gsi]` 是 一个链表，链表中每个节点 = 该 GSI 连接到的一个 irq routing entry


```c
struct kvm_kernel_irq_routing_entry {
	u32 gsi;
	u32 type;
	int (*set)(struct kvm_kernel_irq_routing_entry *e,
		   struct kvm *kvm, int irq_source_id, int level,
		   bool line_status);
	union {
		struct {
			unsigned irqchip;
			unsigned pin;
		} irqchip;
		struct {
			u32 address_lo;
			u32 address_hi;
			u32 data;
			u32 flags;
			u32 devid;
		} msi;
		struct kvm_s390_adapter_int adapter;
		struct kvm_hv_sint hv_sint;
		struct kvm_xen_evtchn xen_evtchn;
	};
	struct hlist_node link;
};
```
是的，是一个 gsi 对应一个链表，也就是触发一个 gsi ，后续可以有多个动作
(kvm_irq_routing_table 要是 crash 可以 dump 链表就好了)

## gsi 和 KVM_SIGNAL_MSI 的关系
<!-- 03950b55-0e4d-4beb-befb-6a8f3a8b46b0 -->

没有关系，例如这个经典路径
```txt
@[
    vmx_deliver_interrupt+5
    __apic_accept_irq+248
    kvm_irq_delivery_to_apic_fast+332
    kvm_irq_delivery_to_apic+103
    kvm_set_msi+146
    kvm_send_userspace_msi+120
    kvm_vm_ioctl+2280
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 42699
```
传递到内核的参数直接就指名道姓了
```c
struct kvm_msi {
	__u32 address_lo;
	__u32 address_hi;
	__u32 data;
	__u32 flags;
	__u32 devid;
	__u8  pad[12];
};
```


## irqfd 的 irqfd_wakeup() 如何和 gsi 机制协同的工作的
<!-- 3da31ae4-dc94-409a-8d4a-8387d4d5b584 -->

例如这个经典中断注入，可以发现在 irqfd_wakeup 中直接使用

kvm_kernel_irqfd::irq_entry ，也就是像是一个 eventfd 仅仅关联一个
kvm_kernel_irq_routing_entry
，但是 gsi 是可以关联多个 kvm_kernel_irq_routing_entry
```txt
@[
    vmx_deliver_interrupt+5
    __apic_accept_irq+244
    kvm_irq_delivery_to_apic_fast+320
    kvm_arch_set_irq_inatomic+184
    irqfd_wakeup+277
    __wake_up_common+117
    eventfd_write+204
    vfs_write+247
    ksys_write+111
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 45307
```
答案在 irqfd_update 中，只有当 gsi 只有一个的时候，
kvm_kernel_irqfd::irq_entry 的 type 才会配置为非 0 ，这个时候
kvm_arch_set_irq_inatomic 才会继续推进

如果 kvm_arch_set_irq_in_atomic 无法注入中断(即非MSI中断或非HV_SINT中断)，
那么就调用 irqfd->inject, 即调用irqfd_inject函数。
参考 : https://www.cnblogs.com/haiyonghao/p/14440723.html


## irq routing
感觉还是历史原因:
```txt
        Device
          |
        IRQ5 (wire)
        /        \
     8259 PIC   IOAPIC
```

指的不是通过编码 ioapic 可以控制中断转发到哪一个 CPU 上，而是因为 x86 上的中断控制的演化导致的，
guest os 可以动态决定其使用哪一个 intc ，但是 host 不知道 guest os 使用哪一个 intc,
甚至 guest os 可以同时打开两个 intc 使用，所以模拟一个中断的时候，只好让两个 intc 都发送中断。

> 虚拟触发了 irq 1，那么需要经过 irq routing：
> irq 1 在 0-7 的范围内，所以会路由到 i8259 master，随后 i8259 master 会向 vCPU 注入中断。
> 同时，irq 1 也会路由到 io apic 一份，io apic 也会向 lapic 继续 delivery。lapic 继续向 vCPU 注入中断。
> linux 在启动阶段，检查到 io apic 后，会选择使用 io apic。尽管经过 irq routing 产生了 i8259 master 和 io apic 两个中断，
但是 Linux 选择 io apic 上的中断。

从 kvm 的这个函数也是可以推测，相当于 x86 的硬件就是实现了，设备发送中断，就是发送给两个中断控制器，
然后 guest 来确定接受谁的信号:
```c
int kvm_apic_accept_pic_intr(struct kvm_vcpu *vcpu)
{
	u32 lvt0 = kvm_lapic_get_reg(vcpu->arch.apic, APIC_LVT0);

	if (!kvm_apic_hw_enabled(vcpu->arch.apic))
		return 1;
	if ((lvt0 & APIC_LVT_MASKED) == 0 &&
	    GET_APIC_DELIVERY_MODE(lvt0) == APIC_MODE_EXTINT)
		return 1;
	return 0;
}
```

### qemu side irq routing
每次调用中断，都是从 X86MachineState::gsi 开始的，在 X86MachineState::gsi 是
其在 pc_gsi_create 中初始化,
handler 为 gsi_handler, gsi_handler 再去调用 GSIState 中三个 irqchip 对应的 handler

```c
typedef struct GSIState {
    qemu_irq i8259_irq[ISA_NUM_IRQS];
    qemu_irq ioapic_irq[IOAPIC_NUM_PINS];
    qemu_irq ioapic2_irq[IOAPIC_NUM_PINS];
} GSIState;

/*
 * Pointer types
 * Such typedefs should be limited to cases where the typedef's users
 * are oblivious of its "pointer-ness".
 * Please keep this list in case-insensitive alphabetical order.
 */
typedef struct IRQState *qemu_irq;

struct IRQState {
    Object parent_obj;

    qemu_irq_handler handler;
    void *opaque;
    int n;
};
```


```c
void gsi_handler(void *opaque, int n, int level)
{
    GSIState *s = opaque;

    trace_x86_gsi_interrupt(n, level);
    switch (n) {
    case 0 ... ISA_NUM_IRQS - 1:
        if (s->i8259_irq[n]) {
            /* Under KVM, Kernel will forward to both PIC and IOAPIC */
            qemu_set_irq(s->i8259_irq[n], level);
        }
        /* fall through */
    case ISA_NUM_IRQS ... IOAPIC_NUM_PINS - 1:
        qemu_set_irq(s->ioapic_irq[n], level);
        break;
    case IO_APIC_SECONDARY_IRQBASE
        ... IO_APIC_SECONDARY_IRQBASE + IOAPIC_NUM_PINS - 1:
        qemu_set_irq(s->ioapic2_irq[n - IO_APIC_SECONDARY_IRQBASE], level);
        break;
    }
}
```

使用键盘为例子, 在 kbd_update_irq_lines 中会调用 `qemu_set_irq(s->irq_kbd, irq_kbd_level);` 来触发中断，
其中的 `KBDState::irq_kbd` 是在 isa_get_irq 中初始化的:

- i8042_realizefn
  - `isa_init_irq(isadev, &s->irq_kbd, 1);`
  - `isa_init_irq(isadev, &s->irq_mouse, 12);`
    - isa_get_irq : 其实获取的就是 X86MachineState::gsi

而 isa_get_irq 获取的就是 X86MachineState::gsi, 而 X86MachineState::gsi 上的 qemu_irq 注册的 handler 就是
gsi_handler 了。

可以明确的看到 `kvm_pic_set_irq` 和 `kvm_ioapic_set_irq` 都会被执行一次。
两个函数调用的内容居然一模一样:
```c
static void kvm_pic_set_irq(void *opaque, int irq, int level)
{
    int delivered;

    pic_stat_update_irq(irq, level);
    delivered = kvm_set_irq(kvm_state, irq, level);
    kvm_report_irq_delivered(delivered);
}
```

```c
static void kvm_ioapic_set_irq(void *opaque, int irq, int level)
{
    KVMIOAPICState *s = opaque;
    IOAPICCommonState *common = IOAPIC_COMMON(s);
    int delivered;

    ioapic_stat_update_irq(common, irq, level);
    delivered = kvm_set_irq(kvm_state, s->kvm_gsi_base + irq, level);
    kvm_report_irq_delivered(delivered);
}
```


现在在 QEMU 中一个中断变为两个，在 kvm 再去处理 irq routing 机制, 那么岂不是一共需要四次? 是的，就是如此。

#### 使用 hpet 作为例子

通过这个可以看的很清楚，
在 qemu 里面， 中断会同时到达两个中断控制器来操作
kvm_pic_set_irq 和 kvm_ioapic_set_irq

```txt
         - 5.96% qemu_clock_run_all_timers
            - qemu_clock_run_timers (inlined)
            - timerlist_run_timers (inlined)
               - timerlist_run_timers (inlined)
                  - 4.19% gsi_handler
                     - 2.86% kvm_pic_set_irq
                        - kvm_set_irq
                           - kvm_vm_ioctl
                              - 2.59% __GI___ioctl
                                 - 2.26% entry_SYSCALL_64_after_hwframe
                                    - do_syscall_64
                                       - 2.08% __x64_sys_ioctl
                                          - 1.62% kvm_vm_ioctl
                                             - 1.36% kvm_vm_ioctl_irq_line
                                                - kvm_set_irq
                                                   - 0.69% kvm_pic_set_irq
                                                        0.62% pic_unlock
                     - 1.23% kvm_ioapic_set_irq
                        - 1.16% kvm_set_irq
                           - kvm_vm_ioctl
                           - __GI___ioctl
                              - 1.01% entry_SYSCALL_64_after_hwframe
                                 - do_syscall_64
                                    - 0.94% __x64_sys_ioctl
                                       - 0.79% kvm_vm_ioctl
                                          - 0.70% kvm_vm_ioctl_irq_line
                                               kvm_set_irq

```
不过我一直以为这个中断在 kvm 中分开为两个的，但是实际上在用户态就分开了。

```txt
🧀  cat /proc/interrupts
# 不去禁用
   0:         24          0          0          0  IO-APIC    2-edge      timer
   1:          0          0          0          9  IO-APIC    1-edge      i8042

# 禁用
            CPU0       CPU1       CPU2       CPU3
   1:          0          0          9          0  IO-APIC    1-edge      i8042
```



## 如何理解 kvm_irqchip_assign_irqfd 的调用路线?
<!-- 09aa931d-b10b-48a7-8bf1-8e6dcf43cdcb -->

直接看 KVM_SET_GSI_ROUTING 的实现
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_init_board
        - machine_run_board_init
          - pc_init1
            - pc_gsi_create
              - kvm_pc_setup_irq_routing
                - kvm_irqchip_commit_routes


- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - flatview_write_continue_step
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - virtio_pci_common_write
                            - virtio_set_status
                              - virtio_net_set_status
                                - virtio_net_vhost_status
                                  - vhost_net_start
                                    - virtio_pci_set_guest_notifiers
                                      - kvm_virtio_pci_vector_vq_use
                                        - kvm_virtio_pci_vector_use_one
                                          - kvm_virtio_pci_vq_vector_use
                                            - kvm_irqchip_commit_route_changes
                                              - kvm_irqchip_commit_routes


每次识别到一个设备的时候，都会重新注册 irq routing 的。
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - flatview_write_continue_step
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - virtio_pci_common_write
                            - virtio_pci_start_ioeventfd
                              - virtio_bus_start_ioeventfd
                                - virtio_scsi_dataplane_start
                                  - virtio_pci_set_guest_notifiers
                                    - kvm_virtio_pci_vector_vq_use
                                      - kvm_virtio_pci_vector_use_one
                                        - kvm_virtio_pci_vq_vector_use
                                          - kvm_irqchip_commit_route_changes
                                            - kvm_irqchip_commit_routes

- virtio_pci_common_write
  - virtio_set_status
    - vuf_set_status
      - vuf_start
        - virtio_pci_set_guest_notifiers
          - kvm_virtio_pci_vector_vq_use
            - kvm_virtio_pci_vector_use_one
              - kvm_virtio_pci_irqfd_use
                - kvm_irqchip_add_irqfd_notifier_gsi
                  - kvm_irqchip_assign_irqfd


所以，当时到底是写了什么东西，然后就是让 qemu 开始注册中断的?

## virtio-blk 的中断如何处理 vCPU 迁移和虚拟机中的中断绑定
<!-- 9c52509d-7338-4d25-830b-e62c162c0743 -->

当到达经典的 vmx_deliver_interrupt 进行中断注入的时候，已经确定了到底
是那个 vCPU 和 vector 了
```c
void vmx_deliver_interrupt(struct kvm_lapic *apic, int delivery_mode,
			   int trig_mode, int vector)
{
	struct kvm_vcpu *vcpu = apic->vcpu;

	if (vmx_deliver_posted_interrupt(vcpu, vector)) {
		kvm_lapic_set_irr(vector, apic);
		kvm_make_request(KVM_REQ_EVENT, vcpu);
		kvm_vcpu_kick(vcpu);
	} else {
		trace_kvm_apicv_accept_irq(vcpu->vcpu_id, delivery_mode,
					   trig_mode, vector);
	}
}
```

分析 gsi 的机制，就很容易回答这个问题了:
1. vCPU 切换不用在乎，因为 vmx_deliver_interrupt 的注入对象就是 vCPU ，随便切换
2. 忽然发现 virtio-blk 和 virtio-scsi 的中断亲和性都是无法修改的，应该是这个地方返回有问题
```c
bool irq_can_set_affinity_usr(unsigned int irq)
{
	struct irq_desc *desc = irq_to_desc(irq);

	return __irq_can_set_affinity(desc) &&
		!irqd_affinity_is_managed(&desc->irq_data);
}
```
需要进一步深入的理解为什么 virtio-blk 的中断不能修改

我现在的猜测还是，既然通过 eventfd 注入，关联到 gsi ，
gsi 关联到 kvm_kernel_irqfd::irq_entry
所以应该是，虚拟机中写 msix-table ，然后 exit ，
然后通过 KVM_SET_GSI_ROUTING 修改 kvm 中的数据。


## 其他话题

### 内核中的 gsi ?
通过这个函数可以很容易的理解了:
```c
static inline u32 mp_pin_to_gsi(int ioapic, int pin)
{
	return mp_ioapic_gsi_routing(ioapic)->gsi_base + pin;
}
```

- secondary_startup_64
  - x86_64_start_kernel
    - x86_64_start_reservations
      - start_kernel
        - setup_arch
          - acpi_boot_init
            - acpi_process_madt
              - acpi_parse_madt_ioapic_entries
                - acpi_table_parse_madt
                  - acpi_table_parse_entries
                    - __acpi_table_parse_entries
                      - acpi_table_parse_entries_array
                        - acpi_parse_entries_array
                          - call_handler
                            - acpi_parse_ioapic
                              - mp_register_ioapic  <-- 注册 ioapic gsi_base

- x86_64_start_kernel
  - x86_64_start_reservations
    - start_kernel
      - x86_late_time_init
        - apic_intr_mode_init
          - apic_bsp_setup
            - setup_IO_APIC
              - setup_IO_APIC_irqs
                - pin_2_irq
                  - mp_pin_to_gsi  <--- 然后通过这个信息来注册

## kvm 中 gsi 如何发送给三个控制器
从公共入口 kvm_set_irq 开始，不过需要知道的是，kvm_set_irq 是一个低俗路径，
也就是虚拟机开机的时候可以观察到一些时间的注入:
```txt
@[
        kvm_set_irq+5
        kvm_vm_ioctl_irq_line+39
        kvm_vm_ioctl+1293
        __x64_sys_ioctl+150
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 134634
```

- kvm_set_ioapic_irq
  - kvm_ioapic_set_irq
    - ioapic_set_irq
      - ioapic_service
        - kvm_irq_delivery_to_apic
          - kvm_irq_delivery_to_apic_fast
            - kvm_apic_set_irq
              - `__apic_accept_irq`
                - kvm_x86_ops.deliver_posted_interrupt(vcpu, vector)

- kvm_set_pic_irq
  - pic_update_irq : 并不会去立刻注入，而是通过 kvm_cpu_has_extint 来检查的，所以是一个被动的方法。

- kvm_set_msi
  - kvm_irq_delivery_to_apic
    - kvm_irq_delivery_to_apic_fast

## 参考

- Linux kernel 的 eventfd 实现 : https://www.cnblogs.com/haiyonghao/p/14440737.html
- QEMU 的 eventfd 实现  https://www.cnblogs.com/haiyonghao/p/14440743.html
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
