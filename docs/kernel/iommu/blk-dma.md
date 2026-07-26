# blk-mq-dma

block/blk-mq-dma.c

## 基本原理 : codex
这个文件是 Linux 块层的 DMA 映射辅助代码：把一个 struct request 中分散的内存页，转换成设备能够访问的 DMA 地址，当前主要由 NVMe PCI 驱动使用。

核心数据流：

```txt
request
  └─ bio 链
      └─ bio_vec（内存页片段）
           ↓ 合并物理连续片段
       phys_vec
           ↓ DMA 映射
       DMA 地址 + 长度
           ↓
       NVMe PRP / SGL 描述符
```

它主要做四件事：

1. 遍历和合并内存片段

block/blk-mq-dma.c:29 遍历 request 的 bio/bvec，并依据队列限制合并物理连续的片段，减少最终 DMA segment 数量。

2. 选择 DMA 映射方式

block/blk-mq-dma.c:135 会在三种路径中选择：

- PCI P2P：设备直接访问另一个 PCI 设备的内存。
- IOVA 合并：存在 IOMMU 时，把多个物理片段映射成一个连续 DMA 地址区间。
- 普通映射：逐段调用 dma_map_phys()。

IOVA 合并是这个文件较重要的优化：

物理地址： [page A] [page C] [page F]
DMA/IOVA： [连续的一个地址区间       ]

这样 NVMe 只需保存一个 [dma_addr, len]，不用为每个物理片段构建传统 scatterlist。

3. 支持传统 scatterlist

block/blk-mq-dma.c:279 仍然可以把 request 转换成传统 scatterlist，供尚未使用新迭代接口的驱动使用。

4. 映射完整性元数据

在启用 CONFIG_BLK_DEV_INTEGRITY 时，它还会映射 DIF/DIX 等数据完整性元数据：

- blk_rq_integrity_dma_map_iter_start()
- blk_rq_integrity_dma_map_iter_next()
- blk_rq_map_integrity_sg()

当前最直接的调用者是 drivers/nvme/host/pci.c:1240。NVMe 驱动拿到这些 DMA 地址后，再构建 PRP 或 SGL 提交给控制器。

## 是将不连续的物理页面变为连续的吗?

是的。即使物理页不连续，IOMMU 也可以把它们映射到一段连续的 IOVA。

```txt
物理内存：
PA 0x1000 ── page A
PA 0x9000 ── page C
PA 0xF000 ── page F

IOMMU 页表：
IOVA 0x4000 → PA 0x1000
IOVA 0x5000 → PA 0x9000
IOVA 0x6000 → PA 0xF000
```

设备看到：
IOVA [0x4000, 0x7000) —— 连续 12 KiB

关键点：

- 物理内存没有变连续。
- 数据没有被移动或复制。
- 只是建立了 IOMMU 页表映射。
- 连续的是设备使用的 DMA/IOVA 地址。
- CPU 仍然通过原来的虚拟地址和物理页访问内存。

在此文件中的过程是：

1. drivers/iommu/dma-iommu.c:dma_iova_try_alloc 先分配一整段连续 IOVA。
2. drivers/iommu/dma-iommu.c:dma_iova_link 把每个不连续物理段依次映射到该 IOVA。
3. drivers/iommu/dma-iommu.c:dma_iova_sync 最后统一同步 IOTLB。
4. NVMe 得到一个 iter.addr + iter.len，可以把整个 request 当作一个 DMA segment。

对应代码是：

```c
iter->addr = state->addr;
iter->len = dma_iova_size(state);

mapped = 0;
for_each_phys_segment(vec) {
        dma_iova_link(dev, state,
                      vec->paddr,  /* 不连续 PA */
                      mapped,     /* 连续 IOVA offset */
                      vec->len, ...);
        mapped += vec->len;
}
```

但并非所有请求都能这样合并。至少需要：

- 设备位于支持 DMA API 的 IOMMU domain；
- 能够成功分配足够大的连续 IOVA；
- 每个物理段的边界满足 IOMMU 最小页粒度；
- request 的物理 gap 与 dma_get_merge_boundary() 兼容。

这些条件由 block/blk-mq-dma.c:74 和 dma_iova_try_alloc() 检查。条件不满足时，就退回逐物理段 dma_map_phys()。

因此，这项优化的本质是：

```txt
多个不连续 PA
        ↓ IOMMU 重映射
一个连续 IOVA
        ↓
NVMe 只需要一个 PRP/SGL 范围
```

它主要减少 NVMe 描述符数量和驱动保存映射信息的内存开销，而不是让物理内存真正连续。


## nvme 做 iommu 类型判断的地方

blk_dma_map_iter_start 是做选择的地方

dma_iova_try_alloc 如果发现不使用 iommu ，那么会直接跳过:
```c
	if (blk_can_dma_map_iova(req, dma_dev) &&
	    dma_iova_try_alloc(dma_dev, state, vec.paddr, total_len))
		return blk_rq_dma_map_iova(req, dma_dev, state, iter, &vec);
	memset(state, 0, sizeof(*state));
	return blk_dma_map_direct(req, dma_dev, iter, &vec);
```

做 16k 的 write 的时候可以触发 iova 合并
```txt
@[
        dma_iova_sync+5 / dma_iova_link+5
        blk_dma_map_iter_start+831
        nvme_map_data+211
        nvme_prep_rq.part.0+34
        nvme_queue_rqs+284
        blk_mq_dispatch_queue_requests+376
        blk_mq_flush_plug_list+136
        __blk_flush_plug+274
        __submit_bio+412
        submit_bio_noacct_nocheck+255
        __blkdev_direct_IO_async+428
        blkdev_write_iter+599
        aio_write+408
        io_submit_one+310
        __x64_sys_io_submit+202
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 589815
```

如果是 16k write 的 iova 合并的 perf 结果:
```txt
-   84.55%     0.81%  fio      libc.so.6          [.] syscall
   - 83.74% syscall
      - 80.40% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 67.26% __x64_sys_io_submit
               - 58.92% io_submit_one
                  - 52.75% aio_write
                     - 52.03% blkdev_write_iter
                        - 50.83% __blkdev_direct_IO_async
                           - 43.84% submit_bio_noacct_nocheck
                              - 43.52% __submit_bio
                                 - 39.50% __blk_flush_plug
                                    - 39.44% blk_mq_flush_plug_list
                                       - 39.37% blk_mq_dispatch_queue_requests
                                          - 39.23% nvme_queue_rqs
                                             - 38.40% nvme_prep_rq.part.0
                                                - 37.84% nvme_map_data
                                                   - 36.85% blk_dma_map_iter_start
                                                      - 33.69% iommu_map_nosync
                                                         - 33.35% vtdss_map_range
                                                            - 31.70% do_map
                                                               + 31.53% __map_range
                                                              0.87% make_range_ul
                                                        1.01% blk_map_iter_next
                                                      - 0.86% dma_iova_try_alloc
                                                         - 0.69% iommu_dma_alloc_iova
                                                              0.57% alloc_iova_fast
                                                        0.61% dma_iova_link
                                                     0.74% nvme_pci_setup_data_prp
                                                  0.51% blk_mq_start_request
                                 - 3.93% blk_mq_submit_bio
                                    - 1.39% __blk_mq_alloc_requests
                                       - 0.90% blk_mq_get_tag
                                            0.70% sbitmap_get
                                    - 0.79% bio_split_rw
                                         0.71% bio_split_io_at
                                      0.62% blk_account_io_start
                           - 5.57% bio_iov_iter_get_pages
```

当 nvme 所在的 domain 是 identity 的时候
```txt
@[
        dma_map_phys+5
        blk_rq_dma_map_iter_next+140
        nvme_pci_prp_iter_next.part.0+27
        nvme_pci_setup_data_prp+422
        nvme_map_data+1036
        nvme_prep_rq.part.0+34
        nvme_queue_rqs+284
        blk_mq_dispatch_queue_requests+376
        blk_mq_flush_plug_list+136
        __blk_flush_plug+274
        __submit_bio+412
        __submit_bio_noacct+142
        iomap_ioend_writeback_submit+101
        iomap_add_to_ioend+412
        xfs_writeback_range+93
        iomap_writeback_folio+634
        iomap_writepages+102
        xfs_vm_writepages+215
        do_writepages+208
        filemap_writeback+194
        file_write_and_wait_range+68
        xfs_file_fsync+80
        do_fsync+95
        __x64_sys_fsync+19
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 2800
```

### dma_map_phys 的作用是获取 iova ，然后提供给设备
总体来说，发起 dma 的时候，需要获取一个物理地址发送给设备，

例如网卡的结果:
```txt
@[
        dma_map_phys+5
        iwl_txq_gen2_build_tx+425
        iwl_txq_gen2_build_tfd+237
        iwl_txq_gen2_tx+325
        iwl_mvm_tx_mpdu+547
        iwl_mvm_tx_skb_sta+603
        iwl_mvm_tx_skb+23
        iwl_mvm_mac_itxq_xmit+162
        ieee80211_queue_skb+557
        __ieee80211_xmit_fast+591
        ieee80211_xmit_fast+329
        __ieee80211_subif_start_xmit+324
        ieee80211_subif_start_xmit+67
        xmit_one.constprop.0+94
        dev_hard_start_xmit+86
        __dev_queue_xmit+2012
        ip_finish_output2+577
        ip_output+99
        ip_send_skb+137
        udp_send_skb+423
        udp_sendmsg+2930
        sock_write_iter+391
        vfs_write+1108
        ksys_write+207
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 8
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
