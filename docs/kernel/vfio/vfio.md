## vfio 基础
<!-- cf9eaa4d-893e-450b-a3b3-db28db06ac53 -->

就是从这个项目开始了:
- https://github.com/Martins3/vfio-host-test
- https://docs.kernel.org/driver-api/vfio.html

source tree 中的这个也是极好的: tools/testing/selftests/vfio/ ，这里也是带有一个驱动例子的
- https://github.com/soleen/iova_stress
- https://github.com/rust-vmm/vfio

- vfio 到底提供了那些资源的封装?
	- bar 寄存器读写
	- mmio 的映射
	- 操作 IOMMU 来映射 DMA
	- 操作 IOMMU 来做中断重映射

## iova 的含义是什么？
vfio 使用也是如此:
```c
/**
 * VFIO_IOMMU_MAP_DMA - _IOW(VFIO_TYPE, VFIO_BASE + 13, struct vfio_dma_map)
 *
 * Map process virtual addresses to IO virtual addresses using the
 * provided struct vfio_dma_map. Caller sets argsz. READ &/ WRITE required.
 *
 * If flags & VFIO_DMA_MAP_FLAG_VADDR, update the base vaddr for iova. The vaddr
 * must have previously been invalidated with VFIO_DMA_UNMAP_FLAG_VADDR.  To
 * maintain memory consistency within the user application, the updated vaddr
 * must address the same memory object as originally mapped.  Failure to do so
 * will result in user memory corruption and/or device misbehavior.  iova and
 * size must match those in the original MAP_DMA call.  Protection is not
 * changed, and the READ & WRITE flags must be 0.
 */
struct vfio_iommu_type1_dma_map {
	__u32	argsz;
	__u32	flags;
#define VFIO_DMA_MAP_FLAG_READ (1 << 0)		/* readable from device */
#define VFIO_DMA_MAP_FLAG_WRITE (1 << 1)	/* writable from device */
#define VFIO_DMA_MAP_FLAG_VADDR (1 << 2)
	__u64	vaddr;			/* Process virtual address */
	__u64	iova;				/* IO virtual address */
	__u64	size;				/* Size of mapping (bytes) */
};
```

看 qemu 的这个调用而已

vfio_legacy_dma_map

具体就不测试了，似乎这个是 qemu 特殊的需求，对于 dpdk 没有这种要求

iova : 实际上就是 gpa ，这个是和 PCIe 设备沟通使用的，通过 iommu 自动将 gpa 映射到物理地址上
同时提供 vaddr ，这个用于 hva -> gpa 。当 Guest os 和 PCI 设备沟通的时候，命令中
需要 iova ，实际上是 gpa 。设备获取到 iova ，HPA 。当 QEMU

所以，这里的关键，就是 iova 是 DMA command 中需要的。

### iova 和 vaddr 是重复的吗?
有重复的，iova 可以通过 memslot 计算出来，但是 vfio 不是总是和 kvm 工作

### iova 和 vaddr 如何和 dpdk 之类的设备工作的

很容易，让 iova 和 vaddr 相等，当提交命令的时候和数据拷贝始终都是
使用的 dpdk 的虚拟地址就可以了。

## 2026-02-14 似乎 6.19 内核开始出现这个报错 ?
似乎现在直通有 bug ?

```c
static void vfio_device_error_append(VFIODevice *vbasedev, Error **errp)
{
    /*
     * MMIO region mapping failures are not fatal but in this case PCI
     * peer-to-peer transactions are broken.
     */
    if (vfio_pci_from_vfio_device(vbasedev)) {
        error_append_hint(errp, "%s: PCI peer-to-peer transactions "
                          "on BARs are not supported.\n", vbasedev->name);
    }
}
```

然后直接无法开机了:
```txt
qemu-system-x86_64: -device vfio-pci,host=0000:01:00.0: warning: VFIO dma-buf not supported in kernel: PCI BAR IOMMU mappings may fail
```

drivers/vfio/pci/vfio_pci_dmabuf.c 刚刚添加的问题，似乎是这个配置

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
