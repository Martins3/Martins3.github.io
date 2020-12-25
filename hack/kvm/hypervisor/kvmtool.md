# kvmtool
[kvmtool - a QEMU alternative?](https://elinux.org/images/4/44/Przywara.pdf)
https://github.com/adamdunkels/uip

- [ ] 阅读资料 : https://mp.weixin.qq.com/s/CWqUagksabj4kDFQhTlgUA

## how to use
```
lkvm run -k ../linux/arch/x86/boot/bzImage -d /home/maritns3/core/linux-kernel-labs/tools/labs/core-image-minimal-qemux86.ext4
```

- [ ] lkvm setup minimi 可以创建一个 rootfs, 但是无法启动

https://www.96boards.org/blog/running-kvm-guest-hikey/



## vfio
- [ ] device__register is a magic, I believe any device register here will be probe by kernel
  - [ ] so, I can provide a fake device driver
    - [ ] provide a tutorial for beginner to learn device model

## pci
- [ ]  pci__init
    - [ ] 如果 pci__init 的时候告诉了那些位置是 mmio 或者 pio 的，是不是说，在 guest 中间 /proc/meminfo 的结构相应的发生变化
    - [ ] 去 stackoverflow 问一个问题，为什么不让物理地址空间到处都是洞，我猜测是历史遗留问题，但是真的到了没有办法修改吗 ?

- [ ] virtio_pci__init : bar 寄存器设置 mmio 的大小都是 PCI_IO_SIZE, 这好吗 ?

## vhost

## virtio
- [ ] 终极问题，数据流是什么样子的 ?
  - [ ] guest 内核的驱动发送消息出去 ?

```c
static struct virtio_ops blk_dev_virtio_ops = {
	.get_config		= get_config,
	.get_host_features	= get_host_features,
	.set_guest_features	= set_guest_features,
	.get_vq_count		= get_vq_count,
	.init_vq		= init_vq,
	.exit_vq		= exit_vq,
	.notify_status		= notify_status,
	.notify_vq		= notify_vq,
	.get_vq			= get_vq,
	.get_size_vq		= get_size_vq,
	.set_size_vq		= set_size_vq,
};
```
初始化的项目是 ?

- [ ] virtio_blk__init 
  - [ ] virtio_blk__init_one

virtio_pci__data_out
init_vq : virt_queue , 尤其是 vring 的初始化

virtio_pci__bar_activate:
- [ ]  bar 的注册规则 ？
- [ ] ioport__register : 

virtio_pci__io_ops : 

kvm__emulate_io : 然后进入到
```c
static struct ioport_operations virtio_pci__io_ops = {
	.io_in	= virtio_pci__io_in,
	.io_out	= virtio_pci__io_out,
};

// 进入调用 vdev 的工作，也就是具体的设备, 最后没有涉及到具体的 port
vq = vdev->ops->get_vq(kvm, vpci->dev, vpci->queue_selector);

// 只是 out 没有 in ?
```
- [ ] 好吧，那么和 pci 的关系 ?
  - [ ] virtio_init : 给设备初始化
    - [ ] irtio_pci__init : 设备初始化之后, 初始化其关联的 pci 总线，将其中的 bar 初始化，并且注册到 pci 上
    - device__register : 最后通过 device register 注册到总线上，从而可以通过 ioport 找到对应的设备，那么就可以找到的对应应该处理的事情

virt_queue__get_iov :


- [ ] desc
    - [ ] 那么 desc 中间的内容其实是内核驱动传输过来的
    - [ ] 如何传入到 guest 中间

- [x] `bdev->io_efd`
    - [x] guest 如何通过其告知 host 有来自于 devices 的数据 : notify_vq : 向 `bdev->io_efd` 写入数值，通过 guest 有事情找 host
      - virtio_pci__data_out : guest 写约定的位置，告诉其数据好了，此时需要 vmm 退出的
      - ioeventfd : virtio_pci__ioevent_callback : 调用对应的设备的 ioeventfd
          - virtio_pci__init_ioeventfd : 中间，将 MMIO 和 Port address 的位置注册上，当 guest 对于这两个位置写入的时候，将会自动通过 eventfd 通知 kvmtool, kvmtool 接收到之后调用回调函数
          - ioeventfd : 当 eventfd 到 guest 写入数据之后 检测到存在事件的发生，那么需要使用 `ioeventfd->vdev->ops->notify_vq(kvm, vpci->dev, ioeventfd->vq);`
          - virtio_blk_thread 被 `bdev->io_efd` 通知之后，然后 virtio_blk_do_io_request 进行真正的处理，IN 和 OUT 都是存在的
          - The purpose of this mechanism is to make guest notify host in a lightweight way. 
- [x] KVM 通过注册 eventfd，那么 guest 如何听话:
    - [ ] 通过 VMEXIT 的方式告知 : virtio_pci__init 的时候，会初始化 pci 的端口地址
    - [ ] eventfd : 是 bar 的地址


将 virt queue 的队列装换为 iov :
- [x] virt queue 如何从 guest 到 host 的传递 ?
  - [x] guest host 如何共同得到 virt queue  : guest 通过 virtio_pci__data_out 中间的 VIRTIO_PCI_QUEUE_PFN 提供了用于创建 vring, 这个位置是共同协定的
  - [x] 如何让 host 知道，数据到了?
      - [ ] virt_queue__get_iov  以及 virt_queue__get_head_iov : 也就是 guest 首先获取数据，然后得到数据的性能。

那么另外记得 desc 是做什么的 ?
```c
struct virt_queue {
	struct vring	vring;
	u32		pfn;
	/* The last_avail_idx field is an index to ->ring of struct vring_avail.
	   It's where we assume the next request index is at.  */
	u16		last_avail_idx;
	u16		last_used_signalled;
	u16		endian;
	bool		use_event_idx;
	bool		enabled;
};
```
last_used_signalled 和 last_avail_idx 是不是给 host 使用的 ?

## i8042
- [ ] 如何从 guest 的 i8042 chip 到 host 的 i8042

从 kvm__irq_line 的参数
```c
	kvm__irq_line(state.kvm, KBD_IRQ, klevel);
	kvm__irq_line(state.kvm, AUX_IRQ, mlevel);
```
到
```c
void kvm__irq_line(struct kvm *kvm, int irq, int level)
{
	struct kvm_irq_level irq_level;

	irq_level	= (struct kvm_irq_level) {
		{
			.irq		= irq,
		},
		.level		= level,
	};

	if (ioctl(kvm->vm_fd, KVM_IRQ_LINE, &irq_level) < 0)
		die_perror("KVM_IRQ_LINE failed");
}
```
可以知道，其实知道，ioctl 的就是 linux irq，其实，不然也没有什么可能了

