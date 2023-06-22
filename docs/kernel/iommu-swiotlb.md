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

## debugfs 观察下
```txt
[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_nslabs
32768

[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_used
0
```

```txt
[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_used
2781

[root@nixos:/sys/kernel/debug/swiotlb]# cat io_tlb_nslabs
32768
```

## https://wiki.gentoo.org/wiki/IOMMU_SWIOTLB

## https://lwn.net/Articles/786558/


## gpt4 给出的回答感觉非常正确

> Swiotlb provides a bounce buffer, which is a contiguous region of memory that the limited DMA-capable devices can access. The bounce buffer acts as an intermediary between the device and the actual memory location where the data should be read/written.
> Here's an overview of how swiotlb works:
>
> Initialization: During boot, the kernel reserves a contiguous region of memory for the swiotlb buffer. The size of this buffer can be specified with the swiotlb kernel parameter, e.g., swiotlb=65536 for a 64 MB buffer.
>
> - DMA mapping: When a device driver wants to perform a DMA operation, it calls the DMA mapping functions provided by the kernel, such as dma_map_single() or dma_map_sg(). These functions check if the specified buffer is within the device's DMA addressable range.
>   - If the buffer is within the range, no swiotlb is needed, and the physical address of the buffer is returned to the device driver.
>   - If the buffer is outside the device's addressable range, swiotlb is triggered. The kernel allocates a portion of the swiotlb buffer and copies the data from the original buffer to the bounce buffer (for DMA read operations).
> - DMA transfer: The device performs the DMA operation using the physical address provided by the DMA mapping functions. If swiotlb is involved, the device reads/writes data from/to the bounce buffer.
> - DMA unmapping: After the DMA operation is complete, the device driver calls the DMA unmapping functions, such as dma_unmap_single() or dma_unmap_sg(). If swiotlb was involved, the kernel copies the data from the bounce buffer back to the original buffer (for DMA write operations) and frees the swiotlb buffer space for use by other devices.
>
> Swiotlb is a fallback mechanism, and the kernel prefers using hardware IOMMU (Input/Output Memory Management Unit) when available, as it can provide better performance and more efficient memory management. However, swiotlb is an important compatibility feature for systems without IOMMU or when dealing with devices with DMA limitations.

- dma_map_single : 主要使用这个
  - dma_map_single_attrs
    - dma_map_page_attrs dma_map_direct : 如果是，走 dma_direct_map_page
        - is_swiotlb_force_bounce && swiotlb_map : 如果必须 bounce 的，那么走这里
	      - dma_addr_t dma_addr = phys_to_dma(dev, phys); : 不做任何装换，直接返回的
      - dma_direct_map_page : 如果不是，走 `iommu_dma_ops` 中注册的 hook

- swiotlb_map
  - swiotlb_tbl_map_single
    - swiotlb_find_slots
      - swiotlb_bounce

## 但是, swiotlb 是如何实现保护的?

这里是说和 trusted 有关，但是 iommu 不就是用来做保护的吗?
```c
static bool dev_is_untrusted(struct device *dev)
{
	return dev_is_pci(dev) && to_pci_dev(dev)->untrusted;
}

static bool dev_use_swiotlb(struct device *dev)
{
	return IS_ENABLED(CONFIG_SWIOTLB) && dev_is_untrusted(dev);
}
```
而且

iommu_dma_map_page 是注册在 iommu 下的 hook ，兄弟啊!

## 默认虚拟机中用的 swiotlb 吗?
并不是，似乎 debugfs 下直接是空的。

那么怎么办?


## 分析下
```txt
@[
    swiotlb_tbl_map_single+5
    swiotlb_map+115
    dma_map_page_attrs+268
    page_pool_dma_map+48
    __page_pool_alloc_pages_slow+307
    page_pool_alloc_frag+332
    mt76_dma_rx_fill.isra.0+306
    mt7921_wpdma_reset+408
    mt7921_wpdma_reinit_cond+115
    mt7921e_mcu_drv_pmctrl+33
    mt7921_mcu_drv_pmctrl+56
    mt7921_pm_wake_work+48
    process_one_work+453
    worker_thread+81
    kthread+219
    ret_from_fork+41
]: 42368
```

- dma_map_page_attrs
  - dma_direct_map_page
    - swiotlb_map

看来是 dma_map_direct 判断成功了

但是:
```txt
[    0.497884] pci 0000:00:04.0: Adding to iommu group 3
```
