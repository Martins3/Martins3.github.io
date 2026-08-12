# IOMMU 杂谈

将一些无法分类的内容放到这里发:

## 文档
- https://luohao-brian.gitbooks.io/interrupt-virtualization/content/vt-d-interrupt-remapping-fen-xi.html
- https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/specifications/48882_IOMMU.pdf
- https://events19.linuxfoundation.cn/wp-content/uploads/2017/11/Shared-Virtual-Addressing_Yisheng-Xie-_-Bob-Liu.pdf
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/app-iommu
  - redhat 的这个应该也是不错的
- Documentation/arch/x86/iommu.rst
- https://docs.kernel.org/driver-api/vfio.html
- https://blog.kernel.love/intel_iommu.html
	- 解释了一下 intel iommu 启动的过程
- [An Introduction to IOMMU Infrastructure in the Linux Kernel](https://lenovopress.lenovo.com/lp1467.pdf)

### [isca_iommu_tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf)

> Extraneous IPI adds overheads => Each extra interrupt can add 5-10K cycles ==> Needs dynamic remapping of interrupts

似乎是 core 1 setup 了 io device 的中断，那么之后，io device 的中断到其他的 core 都需要额外的 ipi.
然后使用 iommu 之后，这个中断 remap 的事情不需要软件处理了

在异构计算中间，可以实现 GPU 共享 CPU 的 page table 之后，获取相同的虚拟地址空间。

> IOMMU IS PART OF PROCESSOR COMPLEX

io device 经过各级 pci hub 到达 root complex,  进入 iommu 翻译，然后到达 mmu controller

> Better solution: IOMMU remaps 32bit device physical
> address to system physical address beyond 32bit
> ‒ DMA goes directly into 64bit memory
> ‒ No CPU transfer
> ‒ More efficient

> If access occurs, OS gets notified and can shut the device & driver down and notifies the user or administrator

> Some I/O devices can issue DMA requests to system memory
> directly, without OS or Firmware intervention
> ‒ e.g.,1394/Firewire, network cards, as part of network boot
> ‒ That allows attacks to modify memory before even the OS has a chance to protect against the attacks

> IOMMU redirects device physical address set up by Guest OS driver (= Guest Physical Addresses) to the actual Host System Physical Address (SPA)

> Some memory copies are gone, because the same memory is accessed
>
> ‒ But the memory is not accessible concurrently, because of cache policies
>
> Two memory pools remain (cache coherent + non-coherent memory regions)
>
> Jobs are still queued through the OS driver chain and suffer from overhead
>
> Still requires expert programmers to get performance

> IOMMU Driver (running on CPU) issues commands to IOMMU
> ‒ e.g., Invalidate IOMMU TLB Entry, Invalidate IOTLB Entry
> ‒ e.g., Invalidate Device Table Entry
> ‒ e.g., Complete PPR, Completion Wait , etc.
>
> Issued via Command Buffer
> ‒ Memory resident circular buffer
> ‒ MMIO registers: Base, Head, and Tail register

> ![](./img/c.png)
> device remapping table
> ![](./img/b.png)
> interrupt remapping table
> ![](./img/a.png)

## IOMMU 不能解决 CMA 想要解决的问题?

设备看到连续的 DMA 地址，而实际物理页可以完全离散。当前内核的 DMA-IOMMU 分配路径就是先分配离散页面，再申请连续 IOVA，最后通过
iommu_map_sg() 映射。

cpu_addr = dma_alloc_coherent(dev, size, &dma_addr, GFP_KERNEL);

不要求底层的物理页面连续的。

Linux 代码也明确这样处理：允许睡眠且没有指定 DMA_ATTR_FORCE_CONTIGUOUS 时，走离散页加 IOMMU remap；
指定 DMA_ATTR_FORCE_CONTIGUOUS 才要求物理连续。

## 存在 iommu 的时候探测

似乎，普通的 pci 的探测过程首先探测 iommu ，然后让 iommu 来探测设备

有证据显示 iommu 探测的更早，至少还是 do_initcalls
```txt
- ??
  - ret_from_fork_asm
    - ret_from_fork
      - kernel_init
        - kernel_init_freeable
          - do_basic_setup
            - do_initcalls
              - do_initcall_level
                - do_one_initcall
                  - pci_iommu_init
                    - intel_iommu_init
                      - iommu_device_register
                        - bus_iommu_probe
                          - bus_for_each_dev
                            - probe_iommu_group
                              - __iommu_probe_device
                                - iommu_init_device
                                  - pci_device_group
                                    - iommu_group_alloc
```

```txt
- ??
  - ret_from_fork_asm
    - ret_from_fork
      - kernel_init
        - kernel_init_freeable
          - do_basic_setup
            - do_initcalls
              - do_initcall_level
                - do_one_initcall
                  - pci_iommu_init
                    - intel_iommu_init
                      - iommu_device_register
                        - bus_iommu_probe
                          - bus_for_each_dev
                            - probe_iommu_group
                              - __iommu_probe_device
```

而 pci 的探测这是通过 thread fn 的:
```txt
- ret_from_fork_asm
  - ret_from_fork
    - kthread
      - worker_thread
        - process_scheduled_works
          - process_one_work
            - async_run_entry_fn
              - __driver_attach_async_helper
                - driver_probe_device
                  - __driver_probe_device
                    - really_probe
                      - call_driver_probe
                        - pci_device_probe
```

## AMD 中的 IRQ_REMAP_X2APIC_MODE 和 IRQ_REMAP_XAPIC_MODE 和 iommu 有关

```c
static int iommu_init_irq(struct amd_iommu *iommu)
{
	int ret;

	if (iommu->int_enabled)
		goto enable_faults;

	if (amd_iommu_xt_mode == IRQ_REMAP_X2APIC_MODE)
		ret = iommu_setup_intcapxt(iommu);
	else if (iommu->dev->msi_cap)
		ret = iommu_setup_msi(iommu);
	else
		ret = -ENODEV;

	if (ret)
		return ret;

	iommu->int_enabled = true;
enable_faults:

	if (amd_iommu_xt_mode == IRQ_REMAP_X2APIC_MODE)
		iommu_feature_enable(iommu, CONTROL_INTCAPXT_EN);

	iommu_feature_enable(iommu, CONTROL_EVT_INT_EN);

	return 0;
}
```

## TODO
1. 如果理解 CONFIG_VFIO_IOMMU_TYPE1
1. [ ] 如果 iommu=on，那么是不是自动意味着 intel_iommu=on 啊
- [ ] 可能是多个 IOMMU 的问题吗?
	- https://stackoverflow.com/questions/60219639/kernel-error-irq-remapping-doesnt-support-x2apic-mode-disabled-x2apic

3. 不知道为什么将 R9000P 的网卡直通之后，重启之后网卡无法使用，需要进入到 windows 去一下

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
