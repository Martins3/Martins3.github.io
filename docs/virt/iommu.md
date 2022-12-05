# IOMMU

- https://luohao-brian.gitbooks.io/interrupt-virtualization/content/vt-d-interrupt-remapping-fen-xi.html

## 问题 && TODO
- [ ] drivers/iommu/hyperv-iommu.c 是个什么概念 ?
- [ ] 能不能 hacking 一个 minimal 的用户态 nvme 驱动，能够读取一个 block 上来的那种
- VFIO 中，是如何处理中断的
- [ ] 在代码中找到 device page table 的内容，以及 IOMMU 防护恶意驱动
- [ ] QEMU 中存在 3000 行处理 Intel IOMMU 的模拟
  - [ ] virtio iommu 是做什么的，和 vIOMMU 是什么关系?
- [ ] 据说 IOMMU 对于性能会存在影响。
- [ ] IOMMU 能不能解决 cma 的问题，也就是，使用离散的物理内存提供给设备，让设备以为自己访问的是连续的物理内存


1. https://wiki.qemu.org/Features/VT-d 分析了下为什么 guest 需要 vIOMMU
2. [oracle 的 blog](https://blogs.oracle.com/linux/post/a-study-of-the-linux-kernel-pci-subsystem-with-qemu) 告诉了 iommu 打开的方法 : `-device intel-iommu` + `-machine q35`
  - iommu 在 QEMU 中就是调试的作用
  - 还分析过 pcie switch 的

## https://compas.cs.stonybrook.edu/~nhonarmand/courses/sp17/cse506/slides/hw_io_virtualization.pdf
- 直通的设备对于 pci 配置空间的读写，需要经过 Host 吗?
  - 需要
- 直通的设备对于 MMIO，需要经过 Host 吗?

guest 可以编辑 MSI ，从而让 guest 发送任意的中断给 Host 机器。

## Documentation/x86/iommu.rst

## https://events19.linuxfoundation.cn/wp-content/uploads/2017/11/Shared-Virtual-Addressing_Yisheng-Xie-_-Bob-Liu.pdf

## [isca_iommu_tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf)

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

## https://kernelgo.org/intel_iommu.html
- 解释了一下 intel iommu 启动的过程

## https://kernelgo.org/interrupt-remapping.html

- https://zhuanlan.zhihu.com/p/372385232 ：分析了初始化的过程

- 在 guest 中以为持有了某一个设备，那么如何才可以真正的使用

这两个函数正好描述了 MSI 的样子:
- `__irq_msi_compose_msg` 是普通的 irq 组装的样子
- linux/drivers/iommu/intel/irq_remapping.c 中的 fill_msi_msg

- [ ] 是不是只有在直通的时候，interrupt remapping 才需要，似乎打开 IOMMU 和使用 interrupt remapping 不是总是绑定的?

```c
static const struct irq_domain_ops intel_ir_domain_ops = {
    .select = intel_irq_remapping_select,
    .alloc = intel_irq_remapping_alloc,
    .free = intel_irq_remapping_free,
    .activate = intel_irq_remapping_activate,
    .deactivate = intel_irq_remapping_deactivate,
};
```

在其中 intel_irq_remapping_alloc 的位置将会来创建 IRTE


在 intel_setup_irq_remapping 中，调用 arch_get_ir_parent_domain
```c
/* Get parent irqdomain for interrupt remapping irqdomain */
static inline struct irq_domain *arch_get_ir_parent_domain(void)
{
    return x86_vector_domain;
}
```


## https://kernelgo.org/posted-interrupt.html


## 有趣

intremap=nosid

- https://www.reddit.com/r/linuxquestions/comments/8te134/what_do_nomodeset_intremapnosid_and_other_grub/
- https://www.greenbone.net/finder/vt/results/1.3.6.1.4.1.25623.1.0.870999
- https://serverfault.com/questions/1077297/ilo4-and-almalinux-centos8-do-not-work-properly

## AMD 手册
- https://www.amd.com/system/files/TechDocs/48882_IOMMU.pdf

## vfio interrupt

- https://stackoverflow.com/questions/29461518/interrupt-handling-for-assigned-device-through-vfio
  - 中断让 vfio 机制来注册

- [ ] 还是需要使用 bpftrace 来分析一下吧

## [An Introduction to IOMMU Infrastructure in the Linux Kernel](https://lenovopress.lenovo.com/lp1467.pdf)

- 主要是，这 DMA coherent 是什么关系哇

```c
#define dma_map_page(d, p, o, s, r) dma_map_page_attrs(d, p, o, s, r, 0)
```

- 这是一个直接映射的驱动的样子:

```txt
#0  dma_map_page_attrs (dev=0xffff8881007ac8c8, page=0xffffea0004067580, offset=0, size=4096, dir=DMA_FROM_DEVICE, attrs=0) at kernel/dma/mapping.c:145
#1  0xffffffff819dde37 in nvme_setup_prp_simple (dev=0xffff8881019d3000, dev=0xffff8881019d3000, bv=<synthetic pointer>, cmnd=0xffff88814046c928, req=0xffff88814046c800) at include/linux/blk-mq.h:203
#2  nvme_map_data (dev=dev@entry=0xffff8881019d3000, req=req@entry=0xffff88814046c800, cmnd=cmnd@entry=0xffff88814046c928) at drivers/nvme/host/pci.c:840
#3  0xffffffff819de0b0 in nvme_prep_rq (req=<optimized out>, dev=0xffff8881019d3000) at drivers/nvme/host/pci.c:910
#4  nvme_prep_rq (req=<optimized out>, dev=0xffff8881019d3000) at drivers/nvme/host/pci.c:896
#5  nvme_queue_rq (hctx=<optimized out>, bd=0xffffc90000c73bb8) at drivers/nvme/host/pci.c:952
#6  0xffffffff8161412f in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff88814053a600, list=list@entry=0xffffc90000c73c08, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:1902
#7  0xffffffff8161a423 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88814053a600) at block/blk-mq-sched.c:306
#8  0xffffffff8161a500 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff88814053a600) at block/blk-mq-sched.c:339
#9  0xffffffff81610f50 in __blk_mq_run_hw_queue (hctx=0xffff88814053a600) at block/blk-mq.c:2020
#10 0xffffffff81611200 in __blk_mq_delay_run_hw_queue (hctx=<optimized out>, async=<optimized out>, msecs=msecs@entry=0) at block/blk-mq.c:2096
#11 0xffffffff81611469 in blk_mq_run_hw_queue (hctx=<optimized out>, async=<optimized out>) at block/blk-mq.c:2144
#12 0xffffffff8161a6c8 in blk_mq_sched_insert_request (rq=rq@entry=0xffff88814046c800, at_head=<optimized out>, at_head@entry=false, run_queue=run_queue@entry=true, async=async@entry=false) at block/blk-mq-sched.c:458
#13 0xffffffff81611a55 in blk_execute_rq (rq=rq@entry=0xffff88814046c800, at_head=at_head@entry=false) at block/blk-mq.c:1278
#14 0xffffffff819cf8a8 in nvme_execute_rq (at_head=false, rq=0xffff88814046c800) at drivers/nvme/host/core.c:1005
#15 __nvme_submit_sync_cmd (q=<optimized out>, cmd=cmd@entry=0xffffc90000c73d08, result=result@entry=0x0 <fixed_percpu_data>, buffer=0xffff8881019d6000, bufflen=bufflen@entry=4096, qid=qid@entry=-1, at_head=0, flags=0) at drivers/nvme/host/core.c:1041
#16 0xffffffff819d2b74 in nvme_submit_sync_cmd (bufflen=4096, buffer=<optimized out>, cmd=<optimized out>, q=<optimized out>) at drivers/nvme/host/core.c:1053
#17 nvme_identify_ctrl (dev=dev@entry=0xffff8881019d3210, id=id@entry=0xffffc90000c73d80) at drivers/nvme/host/core.c:1295
#18 0xffffffff819d50ce in nvme_init_identify (ctrl=0xffff8881019d3210) at drivers/nvme/host/core.c:3081
#19 nvme_init_ctrl_finish (ctrl=ctrl@entry=0xffff8881019d3210) at drivers/nvme/host/core.c:3246
#20 0xffffffff819df784 in nvme_reset_work (work=0xffff8881019d3608) at drivers/nvme/host/pci.c:2870
#21 0xffffffff81123d37 in process_one_work (worker=worker@entry=0xffff88814049ca80, work=0xffff8881019d3608) at kernel/workqueue.c:2289
#22 0xffffffff811242c8 in worker_thread (__worker=0xffff88814049ca80) at kernel/workqueue.c:2436
#23 0xffffffff8112ac73 in kthread (_create=0xffff8881404a0d40) at kernel/kthread.c:376
#24 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#25 0x0000000000000000 in ?? ()
```

- [ ] 即使不是直通，那么存在保护的功能吗?


如果不是直接 iommu_dma_map_page 的，

## iommu_dma_ops

```c
static const struct dma_map_ops iommu_dma_ops = {
	.flags			= DMA_F_PCI_P2PDMA_SUPPORTED,
	.alloc			= iommu_dma_alloc,
	.free			= iommu_dma_free,
	.alloc_pages		= dma_common_alloc_pages,
	.free_pages		= dma_common_free_pages,
	.alloc_noncontiguous	= iommu_dma_alloc_noncontiguous,
	.free_noncontiguous	= iommu_dma_free_noncontiguous,
	.mmap			= iommu_dma_mmap,
	.get_sgtable		= iommu_dma_get_sgtable,
	.map_page		= iommu_dma_map_page,
	.unmap_page		= iommu_dma_unmap_page,
	.map_sg			= iommu_dma_map_sg,
	.unmap_sg		= iommu_dma_unmap_sg,
	.sync_single_for_cpu	= iommu_dma_sync_single_for_cpu,
	.sync_single_for_device	= iommu_dma_sync_single_for_device,
	.sync_sg_for_cpu	= iommu_dma_sync_sg_for_cpu,
	.sync_sg_for_device	= iommu_dma_sync_sg_for_device,
	.map_resource		= iommu_dma_map_resource,
	.unmap_resource		= iommu_dma_unmap_resource,
	.get_merge_boundary	= iommu_dma_get_merge_boundary,
	.opt_mapping_size	= iommu_dma_opt_mapping_size,
};
```

## 如果是给 guest 映射的 DMA 空间，需要同步的修改 IOMMU 才对


## swiotlb

- kernel/dma/swiotlb.c

```txt
#0  swiotlb_map (dev=dev@entry=0xffff8880041a68c8, paddr=paddr@entry=4339060736, size=size@entry=128, dir=dir@entry=DMA_FROM_DEVICE, attrs=attrs@entry=0) at kernel/dma/swiotlb.c:863
#1  0xffffffff81185ddc in dma_direct_map_page (attrs=0, dir=DMA_FROM_DEVICE, size=128, offset=<optimized out>, page=<optimized out>, dev=0xffff8880041a68c8) at kernel/dma/direct.h:101
#2  dma_direct_map_sg (dev=0xffff8880041a68c8, sgl=0xffff8881412f5230, nents=2, dir=DMA_FROM_DEVICE, attrs=attrs@entry=0) at kernel/dma/direct.c:504
#3  0xffffffff8118439b in __dma_map_sg_attrs (dev=<optimized out>, sg=<optimized out>, nents=<optimized out>, dir=<optimized out>, attrs=attrs@entry=0) at kernel/dma/mapping.c:193
#4  0xffffffff811843f5 in dma_map_sg_attrs (dev=<optimized out>, sg=<optimized out>, nents=<optimized out>, dir=<optimized out>, attrs=attrs@entry=0) at kernel/dma/mapping.c:228
#5  0xffffffff819ef038 in ata_sg_setup (qc=0xffff88810015c130) at drivers/ata/libata-core.c:4541
#6  ata_qc_issue (qc=0xffff88810015c130) at drivers/ata/libata-core.c:4842
#7  0xffffffff819f7890 in ata_scsi_translate (xlat_func=<optimized out>, cmd=0xffff8881412f5108, dev=<optimized out>) at drivers/ata/libata-scsi.c:1733
#8  __ata_scsi_queuecmd (scmd=scmd@entry=0xffff8881412f5108, dev=<optimized out>) at drivers/ata/libata-scsi.c:3999
#9  0xffffffff819f7b22 in ata_scsi_queuecmd (shost=<optimized out>, cmd=0xffff8881412f5108) at drivers/ata/libata-scsi.c:4044
#10 0xffffffff819b570e in scsi_dispatch_cmd (cmd=0xffff8881412f5108) at drivers/scsi/scsi_lib.c:1516
#11 scsi_queue_rq (hctx=<optimized out>, bd=<optimized out>) at drivers/scsi/scsi_lib.c:1752
#12 0xffffffff8161412f in blk_mq_dispatch_rq_list (hctx=hctx@entry=0xffff8881418b0000, list=list@entry=0xffffc9000080f970, nr_budgets=nr_budgets@entry=0) at block/blk-mq.c:1902
#13 0xffffffff8161a423 in __blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8881418b0000) at block/blk-mq-sched.c:306
#14 0xffffffff8161a500 in blk_mq_sched_dispatch_requests (hctx=hctx@entry=0xffff8881418b0000) at block/blk-mq-sched.c:339
#15 0xffffffff81610f50 in __blk_mq_run_hw_queue (hctx=0xffff8881418b0000) at block/blk-mq.c:2020
#16 0xffffffff81611200 in __blk_mq_delay_run_hw_queue (hctx=<optimized out>, async=<optimized out>, msecs=msecs@entry=0) at block/blk-mq.c:2096
#17 0xffffffff81611469 in blk_mq_run_hw_queue (hctx=<optimized out>, async=<optimized out>) at block/blk-mq.c:2144
#18 0xffffffff8161a6c8 in blk_mq_sched_insert_request (rq=rq@entry=0xffff8881412f5000, at_head=<optimized out>, at_head@entry=true, run_queue=run_queue@entry=true, async=async@entry=false) at block/blk-mq-sched.c:458
#19 0xffffffff81611a55 in blk_execute_rq (rq=rq@entry=0xffff8881412f5000, at_head=at_head@entry=true) at block/blk-mq.c:1278
#20 0xffffffff819b38f4 in __scsi_execute (sdev=sdev@entry=0xffff8881006f9000, cmd=cmd@entry=0xffffc9000080fabc "Z", data_direction=data_direction@entry=2, buffer=buffer@entry=0xffff8881418b0600, bufflen=bufflen@entry=128, sense=sense@entry=0x0 <fixed_percpu_data>, sshdr=<optimized out>, timeout=<optimized out>, retries=<optimized out>, flags=<optimized out>, rq_flags=<optimized out>, resid=<optimized out>) at drivers/scsi/scsi_lib.c:241
#21 0xffffffff819b3e2c in scsi_execute_req (resid=0x0 <fixed_percpu_data>, retries=3, timeout=30000, sshdr=0xffffc9000080fab4, bufflen=128, buffer=0xffff8881418b0600, data_direction=2, cmd=0xffffc9000080fabc "Z", sdev=0xffff8881006f9000) at include/scsi/scsi_device.h:477
#22 scsi_mode_sense (sdev=0xffff8881006f9000, dbd=<optimized out>, dbd@entry=0, modepage=modepage@entry=42, buffer=buffer@entry=0xffff8881418b0600 "", len=len@entry=128, timeout=timeout@entry=30000, retries=<optimized out>, data=<optimized out>, sshdr=0xffffc9000080fab4) at drivers/scsi/scsi_lib.c:2196
#23 0xffffffff819c8c23 in get_capabilities (cd=0xffff888100b09900) at drivers/scsi/sr.c:830
#24 sr_probe (dev=0xffff8881006f91b0) at drivers/scsi/sr.c:673
#25 0xffffffff81968ad3 in call_driver_probe (drv=0xffffffff82c24a60 <sr_template>, dev=0xffff8881006f91b0) at drivers/base/dd.c:560
#26 really_probe (dev=dev@entry=0xffff8881006f91b0, drv=drv@entry=0xffffffff82c24a60 <sr_template>) at drivers/base/dd.c:639
#27 0xffffffff81968b7d in __driver_probe_device (drv=drv@entry=0xffffffff82c24a60 <sr_template>, dev=dev@entry=0xffff8881006f91b0) at drivers/base/dd.c:778
#28 0xffffffff81968bf9 in driver_probe_device (drv=drv@entry=0xffffffff82c24a60 <sr_template>, dev=dev@entry=0xffff8881006f91b0) at drivers/base/dd.c:808
#29 0xffffffff8196920a in __device_attach_driver (drv=0xffffffff82c24a60 <sr_template>, _data=0xffffc9000080fc68) at drivers/base/dd.c:936
#30 0xffffffff819669c9 in bus_for_each_drv (bus=<optimized out>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffc9000080fc68, fn=fn@entry=0xffffffff81969190 <__device_attach_driver>) at drivers/base/bus.c:427
#31 0xffffffff81968eb7 in __device_attach (dev=dev@entry=0xffff8881006f91b0, allow_async=allow_async@entry=true) at drivers/base/dd.c:1008
#32 0xffffffff8196943a in device_initial_probe (dev=dev@entry=0xffff8881006f91b0) at drivers/base/dd.c:1057
#33 0xffffffff81967c29 in bus_probe_device (dev=dev@entry=0xffff8881006f91b0) at drivers/base/bus.c:487
#34 0xffffffff819644fc in device_add (dev=dev@entry=0xffff8881006f91b0) at drivers/base/core.c:3517
#35 0xffffffff819ba530 in scsi_sysfs_add_sdev (sdev=sdev@entry=0xffff8881006f9000) at drivers/scsi/scsi_sysfs.c:1393
#36 0xffffffff819b7598 in scsi_add_lun (async=0, bflags=<synthetic pointer>, inq_result=0xffff888100b09600 "\005\200\005\062\037", sdev=0xffff8881006f9000) at drivers/scsi/scsi_scan.c:1095
#37 scsi_probe_and_add_lun (starget=starget@entry=0xffff888004423400, lun=lun@entry=0, bflagsp=bflagsp@entry=0x0 <fixed_percpu_data>, sdevp=sdevp@entry=0xffffc9000080fe10, rescan=rescan@entry=SCSI_SCAN_RESCAN, hostdata=hostdata@entry=0x0 <fixed_percpu_data>) at drivers/scsi/scsi_scan.c:1261
#38 0xffffffff819b7d44 in __scsi_add_device (shost=0xffff88800417d000, channel=<optimized out>, id=<optimized out>, lun=lun@entry=0, hostdata=hostdata@entry=0x0 <fixed_percpu_data>) at drivers/scsi/scsi_scan.c:1583
#39 0xffffffff819f7d0e in ata_scsi_scan_host (ap=0xffff88810015c000, sync=1) at drivers/ata/libata-scsi.c:4281
#40 0xffffffff811304db in async_run_entry_fn (work=0xffff8880044a8aa0) at kernel/async.c:127
#41 0xffffffff81123d37 in process_one_work (worker=worker@entry=0xffff88810013acc0, work=0xffff8880044a8aa0) at kernel/workqueue.c:2289
#42 0xffffffff811242c8 in worker_thread (__worker=0xffff88810013acc0) at kernel/workqueue.c:2436
#43 0xffffffff8112ac73 in kthread (_create=0xffff8881006742c0) at kernel/kthread.c:376
#44 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#45 0x0000000000000000 in ?? ()
```

# iommu

- [ ] iommu 是可以实现每一个 device 都存在各自的映射，而不是一个虚拟机使用一个映射的。

- 为什么 IOMMU 需要搞出来 iommu group，需要更加详细的解释。

## [ ] vIOMMU 和 virtio iommu 是一个东西吗

## [ ] IOMMU interrupt remapping 是如何实现的
参考 intel vt-d 标准吧！

# vIOMMU

- QEMU 的解释 : https://wiki.qemu.org/Features/VT-d

# VT-d
How to enable `IRQ_REMAP` in `make menuconfig`:
Device Drivers ==> IOMMU Hareware Support ==> Support for Interrupt Remapping

intel_setup_irq_remapping ==> iommu_set_irq_remapping, setup `Interrupt Remapping Table Address Register` which hold address **IRET** locate [^3] 163,164

好家伙，才意识到 ITRE 其实存在两种格式，remapped interrupt 的格式下，其功能相当于 IO-APIC 的功能，作为设备和 CPU 之间的联系，而 Posted-interrupt 的格式下，就是我们熟悉的内容。
在 Posted-interrupt 格式下，IRET 中间没有目标 CPU 等字段，而是 posted-interrupt descriptor 的地址

### 5 Interrupt Remapping
This chapter discuss architecture and hardware details for interrupt-remapping and interruptposting.These features collectively are called the interrupt remapping architecture.


## my question
- [ ] mmio 可以 remap 吗 ?
- [ ] dma engine 是一个需要的硬件支持放在哪里了 ?
- [ ] 怎么知道一个设备在进行 dma ?
  - [x] 一个真正的物理设备，当需要发起 dma 的时候，进行的 IO 地址本来应该在 pa, 由于 vm 的存在，实际上是在 gpa 上，需要进行在 hpa 上


## [VT-d Posted Interrupts](https://events.static.linuxfound.org/sites/events/files/slides/VT-d%20Posted%20Interrupts-final%20.pdf)
1. Motivation
  - Interrupt virtualization efficiency
  - *Interrupt migration complexity*
  - *Big requirement of host vector for different assigned devices*

- [ ] migration ?
- [ ] host **vector** for different assigned devices ?

![](../img/vt-d-1.png)

Xen Implementation Details:
- Update IRET according to guest’s modification to the interrupt configuration (MSI address, data)
- Interrupt migration during VCPU scheduling

[^3]: Inside the Linux Virtualization : Principle and Implementation
