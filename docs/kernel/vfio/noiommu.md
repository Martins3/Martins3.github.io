# noiommu
[User Space Network Drivers](https://arxiv.org/pdf/1901.10664.pdf)
看似是一篇文章，但是实际上代码分析

https://www.alibabacloud.com/help/en/ecs/user-guide/replace-uio-drivers-with-vfio-drivers

https://doc.dpdk.org/guides/linux_gsg/linux_drivers.html

为什么 noiommu 的代码在框架上几乎没有增加什么内容
- https://lwn.net/Articles/660745/

## 基本的测试方法


使用 /home/martins3/data/vfio-host-test/src_test/noiommu.c 来测试

vfio_group_find_or_alloc

绑定的过程中，结果如下:
```txt
@[
        vfio_group_find_or_alloc+1
        vfio_device_set_group+25
        __vfio_register_dev+78
        vfio_pci_core_register_device+433
        vfio_pci_probe+79
        local_pci_probe+69
        work_for_cpu_fn+26
        process_one_work+395
        worker_thread+598
        kthread+252
        ret_from_fork+240
        ret_from_fork_asm+26
]: 1
```
如果在 bind 的时候，就会执行这个路径，感觉




## 如何实现的?

不能保证安全 -> 驱动随意的进行 dma


但是是怎么实现 HVA 和 HPA 的映射的 ? 其实如果是大页，似乎也不是很难

通过分析 dpdk 的代码:

vfio_noiommu_dma_mem_map 中什么都不需要做，

dpdk/lib/eal/linux/eal_memory.c:rte_mem_virt2phy 中获取到物理地址

所以，还是猜测就是因为获取到了物理地址，所以可以让驱动自动来进行物理地址和虚拟地址的装换

## 确认一下
1. dpdk 是可以使用 noiommu 模式的，但是直通到虚拟机中就很难了

因为虚拟机的驱动无法处理，不过可以测试一下

```txt
pci_bind_to_vfio 0000:00:03.0
```

似乎有这个日志:
```txt
[  331.209780] vfio-pci 0000:00:03.0: Adding to iommu group 0
[  331.216037] vfio-pci 0000:00:03.0: Adding kernel taint for vfio-noiommu group on device
```

也就是，vfio 的工作只是用来做映射，和中断的接受了。


基本的观察:
```txt
localhost login: [   33.982641][ T2930] vfio-pci 0000:00:07.0: Adding to iommu group 0
[   33.984146][ T2930] vfio-pci 0000:00:07.0: Adding kernel taint for vfio-noiommu group on device
[  121.857250][ T3387] vfio-pci 0000:00:07.0: Removing from iommu group 0
[  122.113976][ T3407] vfio-pci 0000:00:07.0: Adding to iommu group 0
[  122.115403][ T3407] vfio-pci 0000:00:07.0: Adding kernel taint for vfio-noiommu group on device
[  122.938061][ T3501] vfio-pci 0000:00:07.0: vfio-noiommu device opened by user (hct_test:3501)
```

```txt
/sys/kernel/iommu_groups/0/devices
lrwxrwxrwx 1 root root 0 Dec 25 14:25 0000:00:07.0 -> ../../../../devices/pci0000:00/0000:00:07.0
```

## noiommu 的基本实现机制

```c
static const struct vfio_iommu_driver_ops vfio_iommu_driver_ops_type1 = {
	.name			= "vfio-iommu-type1",
	.owner			= THIS_MODULE,
	.open			= vfio_iommu_type1_open,
	.release		= vfio_iommu_type1_release,
	.ioctl			= vfio_iommu_type1_ioctl,
	.attach_group		= vfio_iommu_type1_attach_group,
	.detach_group		= vfio_iommu_type1_detach_group,
	.pin_pages		= vfio_iommu_type1_pin_pages,
	.unpin_pages		= vfio_iommu_type1_unpin_pages,
	.register_device	= vfio_iommu_type1_register_device,
	.unregister_device	= vfio_iommu_type1_unregister_device,
	.dma_rw			= vfio_iommu_type1_dma_rw,
	.group_iommu_domain	= vfio_iommu_type1_group_iommu_domain,
};
```

作为对比，也就说 vfio 不提供 dma 映射相关内容，关于 mmio 空间 ，从来都是可以映射的:
```c
static const struct vfio_iommu_driver_ops vfio_noiommu_ops = {
	.name = "vfio-noiommu",
	.owner = THIS_MODULE,
	.open = vfio_noiommu_open,
	.release = vfio_noiommu_release,
	.ioctl = vfio_noiommu_ioctl,
	.attach_group = vfio_noiommu_attach_group,
	.detach_group = vfio_noiommu_detach_group,
};
```
和 vfio_iommu_type1_attach_group 平级的是什么? vfio_noiommu_attach_group ，这个什么都不需要做

显然，这里我们发现了一个很奇怪的事情，就是为什么都不去处理中断啊? 再次回忆一下，iommu 的功能是什么?

先回忆一下，iommu

### vfio 如何提供没有 iommu 支持的中断吗?

```c
struct vfio_irq_info {
	__u32	argsz;
	__u32	flags;
#define VFIO_IRQ_INFO_EVENTFD		(1 << 0)
#define VFIO_IRQ_INFO_MASKABLE		(1 << 1)
#define VFIO_IRQ_INFO_AUTOMASKED	(1 << 2)
#define VFIO_IRQ_INFO_NORESIZE		(1 << 3)
	__u32	index;		/* IRQ index */
	__u32	count;		/* Number of IRQs within this index */
};

ioctl(dev->device_fd, VFIO_DEVICE_GET_IRQ_INFO, irq);

struct vfio_irq_set {
	__u32	argsz;
	__u32	flags;
#define VFIO_IRQ_SET_DATA_NONE		(1 << 0) /* Data not present */
#define VFIO_IRQ_SET_DATA_BOOL		(1 << 1) /* Data is bool (u8) */
#define VFIO_IRQ_SET_DATA_EVENTFD	(1 << 2) /* Data is eventfd (s32) */
#define VFIO_IRQ_SET_ACTION_MASK	(1 << 3) /* Mask interrupt */
#define VFIO_IRQ_SET_ACTION_UNMASK	(1 << 4) /* Unmask interrupt */
#define VFIO_IRQ_SET_ACTION_TRIGGER	(1 << 5) /* Trigger interrupt */
	__u32	index;
	__u32	start;
	__u32	count;
	__u8	data[];
};

ioctl(device, VFIO_DEVICE_SET_IRQS, irq_set);
```

## 想不到又杀出来一个程咬金了
```txt
enum vfio_group_type {
	/*
	 * Physical device with IOMMU backing.
	 */
	VFIO_IOMMU,

	/*
	 * Virtual device without IOMMU backing. The VFIO core fakes up an
	 * iommu_group as the iommu_group sysfs interface is part of the
	 * userspace ABI.  The user of these devices must not be able to
	 * directly trigger unmediated DMA.
	 */
	VFIO_EMULATED_IOMMU,

	/*
	 * Physical device without IOMMU backing. The VFIO core fakes up an
	 * iommu_group as the iommu_group sysfs interface is part of the
	 * userspace ABI.  Users can trigger unmediated DMA by the device,
	 * usage is highly dangerous, requires an explicit opt-in and will
	 * taint the kernel.
	 */
	VFIO_NO_IOMMU,
};
```

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
