# buffer io
## 如何理解 aio 和 io uring 处理的 buffer io 的差异
1. io_uring 提交之后给内核中的 wq，然后让 wq 来将任务执行完之后，来通知任务已经结束了。
所以 aio 无法实现  buffer write 的 async ，
2. buffer read : 我认为纯粹是没必要，其实也是可以类似的

## aio perf 结果

### big file without cache
用 fio io xfs randread 上的一个超级大文件, 将 direct 设置为 0 ，
测试的性能只有 11k，如果使用 direct=1 性能大致可以到 340k

从下面的 trace 可以看到， filemap_get_pages 中等待到 folio_wait_bit_common 这里:

```txt
@[
    io_schedule+5
    folio_wait_bit_common+317
    filemap_get_pages+1535
    filemap_read+217
    xfs_file_buffered_read+82
    xfs_file_read_iter+113
    aio_read+312
    io_submit_one+1406
    __x64_sys_io_submit+173
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 39282
```


```txt
-   34.40%     0.09%  fio              [k] entry_SYSCALL_64_after_hwframe                                                                                  ▒
   - 34.31% entry_SYSCALL_64_after_hwframe                                                                                                                 ▒
      - do_syscall_64                                                                                                                                      ▒
         - 32.06% __x64_sys_io_submit                                                                                                                      ◆
            - 31.62% io_submit_one                                                                                                                         ▒
               - 30.22% aio_read                                                                                                                           ▒
                  - 29.51% xfs_file_read_iter                                                                                                              ▒
                     - xfs_file_buffered_read                                                                                                              ▒
                        - 29.27% filemap_read                                                                                                              ▒
                           - 23.62% filemap_get_pages                                                                                                      ▒
                              - 14.15% force_page_cache_ra                                                                                                 ▒
                                 - 14.09% page_cache_ra_unbounded                                                                                          ▒
                                    - 8.42% read_pages                                                                                                     ▒
                                       - 4.30% iomap_readahead                                                                                             ▒
                                          - 1.74% iomap_iter                                                                                               ▒
                                             - xfs_read_iomap_begin                                                                                        ▒
                                                - 1.28% xfs_bmapi_read                                                                                     ▒
                                                     1.09% xfs_iext_lookup_extent                                                                          ▒
                                          - 1.27% submit_bio_noacct_nocheck                                                                                ▒
                                             - 1.11% blk_mq_submit_bio                                                                                     ▒
                                                  0.56% __blk_mq_alloc_requests                                                                            ▒
                                          - 0.94% iomap_readpage_iter                                                                                      ▒
                                               0.74% bio_alloc_bioset                                                                                      ▒
                                       - 4.07% blk_finish_plug                                                                                             ▒
                                          - __blk_flush_plug                                                                                               ▒
                                             - 4.03% blk_mq_flush_plug_list.part.0                                                                         ▒
                                                - 3.96% nvme_queue_rqs                                                                                     ▒
                                                   - 3.72% nvme_prep_rq.part.0                                                                             ▒
                                                      - 3.42% iommu_dma_map_page                                                                           ▒
                                                         - 3.37% __iommu_dma_map                                                                           ▒
                                                            - 3.13% iommu_map                                                                              ▒
                                                               - 2.71% __iommu_map                                                                         ▒
                                                                  - 2.60% intel_iommu_map_pages                                                            ▒
                                                                     - __domain_mapping                                                                    ▒
                                                                          1.06% clflush_cache_range                                                        ▒
                                    - 2.96% filemap_add_folio                                                                                              ▒
                                       - 2.23% __filemap_add_folio                                                                                         ▒
                                          - 0.93% xas_store                                                                                                ▒
                                             - 0.81% xas_create                                                                                            ▒
                                                - 0.77% xas_alloc                                                                                          ▒
                                                     0.71% kmem_cache_alloc_lru                                                                            ▒
                                            0.50% __mem_cgroup_charge                                                                                      ▒
                                       - 0.70% folio_add_lru                                                                                               ▒
                                          - folio_batch_move_lru                                                                                           ▒
                                               0.50% lru_add_fn                                                                                            ▒
                                      1.32% up_read                                                                                                        ▒
                                    - 1.13% folio_alloc                                                                                                    ▒
                                       - 1.08% __alloc_pages                                                                                               ▒
                                          - 0.93% get_page_from_freelist                                                                                   ▒
                                             - 0.71% rmqueue_bulk                                                                                          ▒
                                                  0.59% __list_del_entry_valid                                                                             ▒
                              - 5.19% folio_wait_bit_common                                                                                                ▒
                                 - 4.77% io_schedule                                                                                                       ▒
                                    - schedule                                                                                                             ▒
                                       - __schedule                                                                                                        ▒
                                          - 2.08% dequeue_task_fair                                                                                        ▒
                                             - dequeue_entity                                                                                              ▒
                                                  0.64% update_load_avg
                                                  0.58% update_curr                                                                                        ▒
                                          - 1.21% psi_task_switch                                                                                          ▒
                                               1.01% psi_group_change                                                                                      ▒
                              - 3.89% filemap_get_read_batch                                                                                               ▒
                                 - 3.51% xas_load                                                                                                          ▒
                                      1.46% xas_descend                                                                                                    ▒
                           - 4.81% copy_page_to_iter                                                                                                       ▒
                              - _copy_to_iter                                                                                                              ▒
                                   copyout                                                                                                                 ▒
         - 1.03% __x64_sys_io_getevents                                                                                                                    ▒
            - do_io_getevents                                                                                                                              ▒
               - 0.62% read_events                                                                                                                         ▒
                    0.58% aio_read_events_ring                                                                                                             ▒
         - 0.76% syscall_exit_to_user_mode                                                                                                                 ▒
              0.67% exit_to_user_mode_prepare
```

### small file with cache
fio seq read 一个小文件(保证 pagecache 的命中率)，大约可以达到 840k 的样子:

```txt
-   75.53%     2.39%  fio              libc.so.6                      [.] syscall
   - 73.14% syscall
      - 70.12% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 62.18% __x64_sys_io_submit
               - 59.34% io_submit_one
                  - 50.37% aio_read
                     - 43.42% xfs_file_read_iter
                        - 43.09% xfs_file_buffered_read
                           - 41.46% filemap_read
                              - 30.20% copy_page_to_iter
                                 - 29.89% _copy_to_iter
                                      29.41% copyout
                              - 8.55% filemap_get_pages
                                 - 5.15% page_cache_ra_order
                                    - 2.99% read_pages
                                       - 2.18% iomap_readahead
                                          - 1.16% submit_bio_noacct_nocheck
                                             - 1.12% blk_mq_submit_bio
                                                - 0.68% blk_add_rq_to_plug
                                                   - blk_mq_flush_plug_list.part.0
                                                      - 0.67% nvme_queue_rqs
                                                         - 0.65% nvme_prep_rq.part.0
                                                              0.50% dma_map_sgtable
                                            0.77% iomap_readpage_iter
                                       - 0.80% blk_finish_plug
                                          - __blk_flush_plug
                                             - 0.80% blk_mq_flush_plug_list.part.0
                                                - nvme_queue_rqs
                                                   - 0.77% nvme_prep_rq.part.0
                                                      - 0.55% dma_map_sgtable
                                                         - __dma_map_sg_attrs
                                                              0.54% iommu_dma_map_sg
                                    - 1.38% filemap_add_folio
                                         1.10% __filemap_add_folio
                                 - 2.31% filemap_get_read_batch
                                      0.80% xas_load
                                 - 0.57% folio_wait_bit_common
                                    - 0.54% io_schedule
                                       - schedule
                                            __schedule
                              - 0.84% touch_atime
                                   atime_needs_update
                           - 0.70% xfs_ilock
                                down_read
                             0.60% xfs_iunlock
                     - 3.42% __fsnotify_parent
                        - 0.93% dget_parent
                             0.56% lockref_get_not_zero
                          0.74% dput
                          0.68% fsnotify
                     - 0.78% security_file_permission
                          0.54% selinux_file_permission
                       0.68% aio_prep_rw
                       0.55% aio_complete_rw
                  - 1.91% aio_complete
                       0.59% _raw_spin_lock_irqsave
                    1.64% _copy_from_user
                    1.12% kmem_cache_alloc
                    0.74% __put_user_4
                    0.73% kmem_cache_free
                    0.70% fget
               - 1.70% lookup_ioctx
                    0.65% __get_user_4
                 0.54% __get_user_8
            - 5.70% __x64_sys_io_getevents
               - 5.50% do_io_getevents
                  - 3.51% read_events
                     - 3.30% aio_read_events_ring
                          0.94% _copy_to_user
                          0.78% __check_object_size
                  - 1.75% lookup_ioctx
                       0.67% __get_user_4
              0.96% syscall_exit_to_user_mode
        0.59% entry_SYSCALL_64
        0.53% entry_SYSCALL_64_safe_stack
```
看上去，最主要的工作就是从 pagecache 中拷贝到 aio 设置的 buffer 中。

aio direct=1 randread 一个文件，性能也可以达到 800K 左右:
```txt
  - 76.47% syscall
      - 73.73% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 66.39% __x64_sys_io_submit
               - 64.23% io_submit_one
                  - 58.84% aio_read
                     - 53.80% xfs_file_read_iter
                        - 53.45% xfs_file_dio_read
                           - 51.54% iomap_dio_rw
                              - __iomap_dio_rw
                                 - 21.45% blk_finish_plug
                                    - 21.40% __blk_flush_plug
                                       - 21.27% blk_mq_flush_plug_list.part.0
                                          - 20.86% nvme_queue_rqs
                                             - 19.67% nvme_prep_rq.part.0
                                                - 17.93% iommu_dma_map_page
                                                   - 17.57% __iommu_dma_map
                                                      - 16.31% iommu_map
                                                         - 14.31% __iommu_map
                                                            - 13.70% intel_iommu_map_pages
                                                               - 13.52% __domain_mapping
                                                                  - 6.06% clflush_cache_range
                                                                     - 1.42% asm_common_interrupt
                                                                        - 1.41% common_interrupt
                                                                           - __common_interrupt
                                                                              - handle_edge_irq
                                                                                 - 1.38% handle_irq_event
                                                                                    - 1.37% __handle_irq_event_percpu
                                                                                       - nvme_irq
                                                                                          - 0.77% nvme_pci_complete_batch
                                                                                               0.76% iommu_dma_unmap_page
                                                                  - 1.36% asm_common_interrupt
                                                                       common_interrupt
                                                                     - __common_interrupt
                                                                        - 1.35% handle_edge_irq
                                                                           - 1.33% handle_irq_event
                                                                              - 1.33% __handle_irq_event_percpu
                                                                                 - nvme_irq
                                                                                    - 0.77% nvme_pci_complete_batch
                                                                                         0.76% iommu_dma_unmap_page
                                                                    1.00% pfn_to_dma_pte
                                                         - 1.83% intel_iommu_iotlb_sync_map
                                                              0.85% xa_find
                                                              0.64% xa_find_after
                                                      - 1.03% iommu_dma_alloc_iova
                                                           0.79% alloc_iova_fast
                                                - 1.24% blk_mq_start_request
                                                   - 0.81% ktime_get
                                                        0.52% read_tsc
                                 - 17.19% iomap_dio_bio_iter
                                    - 6.96% submit_bio_noacct_nocheck
                                       - 5.68% blk_mq_submit_bio
                                          - 3.45% __blk_mq_alloc_requests
                                             - 1.85% blk_mq_get_tag
                                                - 1.08% sbitmap_get
                                                     0.82% sbitmap_find_bit
                                             - 0.83% ktime_get
                                                  0.56% read_tsc
                                       - 0.73% ktime_get
                                            0.53% read_tsc
                                       - 0.73% ktime_get
                                            0.53% read_tsc
                                    - 4.80% bio_iov_iter_get_pages
                                       - 4.20% iov_iter_extract_pages
                                          - 3.75% pin_user_pages_fast
                                             - 3.56% internal_get_user_pages_fast
                                                  0.92% try_grab_folio
                                                - 0.84% asm_common_interrupt
                                                   - common_interrupt
                                                     __common_interrupt
                                                   - handle_edge_irq
                                                      - 0.83% handle_irq_event
                                                         - __handle_irq_event_percpu
                                                              nvme_irq
                                    - 3.09% bio_alloc_bioset
                                       - 1.55% bio_associate_blkg
                                            1.28% bio_associate_blkg_from_css
                                       - 1.10% mempool_alloc
                                            0.85% kmem_cache_alloc
                                    - 1.15% bio_set_pages_dirty
                                         0.96% set_page_dirty_lock
                                 - 4.23% iomap_iter
                                    - 3.37% xfs_read_iomap_begin
                                       - 1.43% xfs_bmapi_read
                                            0.66% xfs_iext_lookup_extent
                                         0.68% xfs_ilock_for_iomap
                                 - 1.70% asm_common_interrupt
                                    - 1.70% common_interrupt
                                       - 1.69% __common_interrupt
                                          - handle_edge_irq
                                             - 1.66% handle_irq_event
                                                - 1.65% __handle_irq_event_percpu
                                                   - nvme_irq
                                                      - 0.93% nvme_pci_complete_batch
                                                         - 0.92% iommu_dma_unmap_page
                                                              0.55% __iommu_dma_unmap
                                                        0.60% blk_mq_end_request_batch
                                 - 0.82% kmalloc_trace
                                      0.72% __kmem_cache_alloc_node
                           - 0.67% touch_atime
                                0.56% atime_needs_update
                             0.58% xfs_ilock
                     - 2.58% __fsnotify_parent
                          0.63% dget_parent
                          0.61% dput
                          0.51% fsnotify
                       0.62% security_file_permission
                       0.60% aio_prep_rw
                    1.36% _copy_from_user
                    1.05% kmem_cache_alloc
                    0.72% fget
                    0.66% __put_user_4
                 1.27% lookup_ioctx
            - 4.94% __x64_sys_io_getevents
               - 4.80% do_io_getevents
                  - 3.22% read_events
                     - 2.57% aio_read_events_ring
                          0.75% _copy_to_user
                          0.51% __check_object_size
                  - 1.40% lookup_ioctx
                       0.53% __get_user_4
                 1.27% lookup_ioctx
            - 4.94% __x64_sys_io_getevents
               - 4.80% do_io_getevents
                  - 3.22% read_events
                     - 2.57% aio_read_events_ring
                          0.75% _copy_to_user
                          0.51% __check_object_size
                  - 1.40% lookup_ioctx
                       0.53% __get_user_4
            - 1.10% syscall_enter_from_user_mode
               - 0.90% asm_common_interrupt
                    common_interrupt
                  - __common_interrupt
                     - handle_edge_irq
                        - 0.87% handle_irq_event
                           - 0.86% __handle_irq_event_percpu
                              - nvme_irq
                                 - 0.54% nvme_pci_complete_batch
                                      0.53% iommu_dma_unmap_page
              0.74% syscall_exit_to_user_mode
      - 0.53% asm_common_interrupt
         - 0.52% common_interrupt
              __common_interrupt
            - handle_edge_irq
               - 0.51% handle_irq_event
                  - 0.50% __handle_irq_event_percpu
                       nvme_irq
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
