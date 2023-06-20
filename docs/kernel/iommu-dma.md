## iommu 在非虚拟化的使用

这是虚拟化在网卡，显卡和
```txt
@[
    iommu_iova_to_phys+5
    iommu_dma_unmap_page+50
    nvme_pci_complete_batch+101
    nvme_irq+114
    __handle_irq_event_percpu+74
    handle_irq_event+62
    handle_edge_irq+157
    __common_interrupt+67
    common_interrupt+129
    asm_common_interrupt+38
    blk_mq_flush_plug_list+65
    __blk_flush_plug+262
    blk_finish_plug+41
    __iomap_dio_rw+1387
    iomap_dio_rw+18
    ext4_file_read_iter+204
    aio_read+306
    io_submit_one+1599
    __x64_sys_io_submit+173
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 1
@[
    iommu_iova_to_phys+5
    vfio_unmap_unpin+258
    vfio_remove_dma+42
    vfio_iommu_type1_ioctl+2815
    __x64_sys_ioctl+145
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 3307
@[
    iommu_iova_to_phys+5
    iommu_dma_unmap_page+50
    mt76_connac_txp_skb_unmap+220
    mt7921_txwi_free+32
    mt7921_mac_tx_free+367
    mt7921_rx_check+45
    mt76_dma_rx_poll+1120
    mt7921_poll_rx+78
    __napi_poll+40
    napi_threaded_poll+343
    kthread+219
    ret_from_fork+41
]: 3555
```

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

- amd_iommu_probe_finalize
  - iommu_setup_dma_ops
    - iommu_dma_init_domain : 参数为 `iommu_dma_ops`
      - iova_domain_init_rcaches
      - iova_reserve_iommu_regions


## 如果关闭 iommu ，路径是怎么样子的
那就是虚拟机中的默认情况?


## 和 /kernel/dma/ 协同分析下


## -device intel-iommu

看上去
```txt
#0  iommu_group_add_device (group=group@entry=0xffff88800509aa00, dev=dev@entry=0xffff8880052710c8) at drivers/iommu/iommu.c:1029
#1  0xffffffff81955a1a in iommu_group_get_for_dev (dev=0xffff8880052710c8) at drivers/iommu/iommu.c:1722
#2  __iommu_probe_device (dev=0xffff8880052710c8, group_list=0xffffc90000017d60) at drivers/iommu/iommu.c:347
#3  0xffffffff81955b35 in probe_iommu_group (dev=<optimized out>, data=<optimized out>) at drivers/iommu/iommu.c:1752
#4  0xffffffff81968aa5 in bus_for_each_dev (bus=bus@entry=0xffffffff82e32a20 <pci_bus_type>, start=start@entry=0x0 <fixed_percpu_data>, data=data@entry=0xffffc90000017d60,
    fn=fn@entry=0xffffffff81955b00 <probe_iommu_group>) at drivers/base/bus.c:368
#5  0xffffffff819561dc in bus_iommu_probe (bus=0xffffffff82e32a20 <pci_bus_type>) at drivers/iommu/iommu.c:1871
#6  0xffffffff819564cc in iommu_device_register (iommu=iommu@entry=0xffff888004176f28, ops=ops@entry=0xffffffff824c0720 <intel_iommu_ops>,
    hwdev=hwdev@entry=0x0 <fixed_percpu_data>) at drivers/iommu/iommu.c:245
#7  0xffffffff835c41e0 in intel_iommu_init () at drivers/iommu/intel/iommu.c:3911
#8  0xffffffff8356ee62 in pci_iommu_init () at arch/x86/kernel/pci-dma.c:193
#9  0xffffffff81001b1a in do_one_initcall (fn=0xffffffff8356ee50 <pci_iommu_init>) at init/main.c:1246
#10 0xffffffff8355f72f in do_initcall_level (command_line=0xffff888005090000 "root", level=5) at init/main.c:1319
#11 do_initcalls () at init/main.c:1335
#12 do_basic_setup () at init/main.c:1354
#13 kernel_init_freeable () at init/main.c:1571
#14 0xffffffff820d3f9a in kernel_init (unused=<optimized out>) at init/main.c:1462
#15 0xffffffff81002ae9 in ret_from_fork () at arch/x86/entry/entry_64.S:308
#16 0x0000000000000000 in ?? ()
```

这里初始化的时候，会让 `get_dma_ops` 修改为返回不会空，而是当时配置的 iommu
```c
	const struct dma_map_ops *ops = get_dma_ops(dev);
```
