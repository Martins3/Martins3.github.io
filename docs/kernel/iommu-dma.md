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
