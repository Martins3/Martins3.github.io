# 经典 flamegraph 流程
使用 flamegraph 还是太不方便了，逐步替换为 perf 吧

## 存储
基本操作方法:
```sh
sudo taskset -ac 1 fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio
sudo perf record -g -C 1 -- sleep 10
```

## ioengine=libaio direct=1 rw=randread bs=4k filename=/dev/nvme1n1
```txt
   - 61.41% syscall
      - 58.34% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 40.46% __x64_sys_io_submit
               - 38.32% io_submit_one
                  - 27.32% aio_read
                     - 22.82% blkdev_read_iter
                        - 22.34% __blkdev_direct_IO_async
                           - 12.16% submit_bio_noacct_nocheck
                              - 11.09% blk_mq_submit_bio
                                 - 3.65% blk_mq_try_issue_directly
                                    - 3.43% __blk_mq_issue_directly
                                       - nvme_queue_rq
                                          - 1.26% nvme_prep_rq.part.0
                                               0.75% blk_mq_start_request
                                            0.99% _raw_spin_lock
                                 - 3.12% __blk_mq_alloc_requests
                                    - blk_mq_get_tag
                                       - 2.39% sbitmap_get
                                          - sbitmap_find_bit
                                               _find_next_zero_bit
                                 - 1.86% __rq_qos_track
                                      wbt_track
                                 - 1.04% blk_mq_rq_ctx_init.isra.0
                                      ktime_get
                                0.64% ktime_get
                           - 6.05% bio_iov_iter_get_pages
                              - 5.46% iov_iter_get_pages
                                 - __iov_iter_get_pages_alloc
                                    - 5.10% get_user_pages_fast
                                       - 4.91% internal_get_user_pages_fast
                                            3.00% try_grab_folio
                           - 2.62% bio_alloc_bioset
                              - 1.28% mempool_alloc
                                   0.99% kmem_cache_alloc
                              - 0.97% bio_associate_blkg
                                   0.78% bio_associate_blkg_from_css
                           - 0.92% bio_set_pages_dirty
                                set_page_dirty_lock
                     - 2.32% __fsnotify_parent
                          0.57% dget_parent
                          0.53% fsnotify
                       0.58% security_file_permission
                       0.51% aio_prep_rw
                    1.53% fget
                    1.39% kmem_cache_alloc
                    1.09% __get_reqs_available
                    1.07% _copy_from_user
                    0.54% __put_user_4
               - 1.27% lookup_ioctx
                    0.63% __get_user_4
            - 15.45% __x64_sys_io_getevents
               - do_io_getevents
                  - 13.45% read_events
                     - 5.89% schedule
                        - __schedule
                           - 2.23% dequeue_task_fair
                              - dequeue_entity
                                   0.66% update_curr
                                   0.62% update_load_avg
                           - 1.04% psi_task_switch
                                0.82% psi_group_change
                             0.65% finish_task_switch.isra.0
                     - 4.12% aio_read_events_ring
                          1.58% _copy_to_user
                          0.99% mutex_lock
                     - 1.46% aio_read_events
                        - aio_read_events_ring
                             0.59% _copy_to_user
                     - 1.00% prepare_to_wait_event
                          0.67% _raw_spin_lock_irqsave
                  - 1.31% lookup_ioctx
                       0.50% __get_user_4
            - 1.46% syscall_exit_to_user_mode
                 1.12% exit_to_user_mode_prepare
```

## ioengine=sync direct=0 rw=randread bs=4k filename=/dev/nvme1n1
一共会显示两段来:

```txt
                  - 27.99% read
                     - 27.99% entry_SYSCALL_64_after_hwframe
                        - do_syscall_64
                           - 26.67% ksys_read
                              - 26.60% vfs_read
                                 - 23.97% blkdev_read_iter
                                    - filemap_read
                                       - 15.40% filemap_get_pages
                                          - 7.96% filemap_get_read_batch
                                             - 6.46% xas_load
                                                  2.59% xas_descend
                                          - 3.45% force_page_cache_ra
                                             - 3.36% page_cache_ra_unbounded
                                                  1.85% up_read
                                                  0.67% filemap_add_folio
                                          - 3.18% folio_wait_bit_common
                                             - 1.21% io_schedule
                                                - schedule
                                                     __schedule
                                       - 7.43% copy_page_to_iter
                                          - _copy_to_iter
                                               copyout
                                   1.75% __fsnotify_parent
                           - 1.31% syscall_exit_to_user_mode
                              - 1.25% exit_to_user_mode_prepare
                                   0.79% __rseq_handle_notify_resume
```

```txt
            - 19.53% read
                 entry_SYSCALL_64_after_hwframe
               - do_syscall_64
                  - 19.53% ksys_read
                       vfs_read
                       blkdev_read_iter
                     - filemap_read
                        - 19.51% filemap_get_pages
                           - 10.76% force_page_cache_ra
                              - page_cache_ra_unbounded
                                 - 7.33% read_pages
                                    - 5.69% mpage_readahead
                                       - 3.69% submit_bio_noacct_nocheck
                                          - 3.27% blk_mq_submit_bio
                                             - 1.12% __blk_mq_alloc_requests
                                                - blk_mq_get_tag
                                                   - 0.90% sbitmap_get
                                                      - sbitmap_find_bit
                                                           _find_next_zero_bit
                                             - 0.88% __rq_qos_track
                                                  wbt_track
                                       - 1.55% do_mpage_readpage
                                            0.80% bio_alloc_bioset
                                    - 1.63% blk_finish_plug
                                       - __blk_flush_plug
                                          - blk_mq_flush_plug_list
                                             - 1.50% nvme_queue_rqs
                                                  0.69% _raw_spin_lock
                                 - 2.66% filemap_add_folio
                                    - 1.86% __filemap_add_folio
                                         0.65% __mem_cgroup_charge
                                    - 0.80% folio_add_lru
                                       - folio_batch_move_lru
                                            0.63% lru_add_fn
                                 - 0.77% folio_alloc
                                    - 0.76% __alloc_pages
                                         0.61% get_page_from_freelist
                           - 8.75% folio_wait_bit_common
                                io_schedule
                                schedule
                              - __schedule
                                 - 3.55% dequeue_task_fair
                                    - dequeue_entity
                                         1.12% update_load_avg
                                         0.76% update_curr
                                 - 2.09% psi_task_switch
                                      1.65% psi_group_change
                                 - 1.76% finish_task_switch.isra.0
                                    - 0.80% __perf_event_task_sched_in
                                         0.71% __rcu_read_lock
```

## xfs

### ioengine=libaio direct=1 rw=randread bs=4k filename=/mnt/x

赶到迷茫，居然可以超过裸盘的性能:
```txt
Jobs: 1 (f=1): [r(1)][0.1%][r=3783MiB/s][r=968k IOPS][eta 08h:19m:36s]
```

```txt
-   79.00%     2.73%  fio              libc.so.6          [.] syscall
   - 76.27% syscall
      - 73.34% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 62.96% __x64_sys_io_submit
               - 59.78% io_submit_one
                  - 50.99% aio_read
                     - 47.99% xfs_file_read_iter
                        - xfs_file_dio_read
                           - 45.16% iomap_dio_rw
                              - __iomap_dio_rw
                                 - 22.64% iomap_dio_bio_iter
                                    - 8.92% submit_bio_noacct_nocheck
                                       - 7.43% blk_mq_submit_bio
                                          - 2.82% __blk_mq_alloc_requests
                                             - blk_mq_get_tag
                                                - 2.02% sbitmap_get
                                                   - 1.84% sbitmap_find_bit
                                                        0.99% _find_next_zero_bit
                                          - 1.38% blk_mq_rq_ctx_init.isra.0
                                             - ktime_get
                                                  read_tsc
                                          - 1.16% __rq_qos_track
                                               wbt_track
                                       - 0.78% ktime_get
                                            read_tsc
                                    - 7.77% bio_iov_iter_get_pages
                                       - 6.84% iov_iter_get_pages
                                          - __iov_iter_get_pages_alloc
                                             - 6.35% get_user_pages_fast
                                                - 6.08% internal_get_user_pages_fast
                                                     3.84% try_grab_folio
                                    - 2.95% bio_alloc_bioset
                                       - 1.52% mempool_alloc
                                          - 1.23% kmem_cache_alloc
                                               0.59% ___slab_alloc
                                       - 1.03% bio_associate_blkg
                                            0.81% bio_associate_blkg_from_css
                                    - 1.64% bio_set_pages_dirty
                                         set_page_dirty_lock
                                 - 6.87% blk_finish_plug
                                    - __blk_flush_plug
                                       - blk_mq_flush_plug_list
                                          - 6.18% nvme_queue_rqs
                                               1.98% _raw_spin_lock
                                             - 1.86% nvme_prep_rq.part.0
                                                - 1.27% blk_mq_start_request
                                                   - 0.89% ktime_get
                                                        read_tsc
                                 - 4.37% iomap_iter
                                    - xfs_read_iomap_begin
                                         1.13% xfs_bmapi_read
                                       - 0.89% xfs_ilock_for_iomap
                                            0.60% down_read
                                         0.57% xfs_iunlock
                                 - 1.22% kmalloc_trace
                                      __kmem_cache_alloc_node
                           - 0.86% touch_atime
                                atime_needs_update
                           - 0.67% xfs_ilock
                                down_read
                             0.54% xfs_iunlock
                       0.94% aio_prep_rw
                     - 0.81% security_file_permission
                          0.61% selinux_file_permission
                    2.04% fget
                  - 1.67% kmem_cache_alloc
                       0.53% ___slab_alloc
                    1.54% _copy_from_user
                    1.48% __get_reqs_available
                    0.70% __put_user_4
               - 2.12% lookup_ioctx
                    1.13% __get_user_4
            - 8.47% __x64_sys_io_getevents
               - do_io_getevents
                  - 6.42% read_events
                     - 5.98% aio_read_events_ring
                          1.72% mutex_lock
                          1.65% _copy_to_user
                          0.70% __check_object_size
                          0.64% mutex_unlock
                  - 1.60% lookup_ioctx
                       0.64% __get_user_4
              0.87% syscall_exit_to_user_mode
        0.70% __entry_text_start
        0.58% entry_SYSCALL_64_safe_stack
```

### ioengine=sync direct=0 rw=randread bs=4k filename=/mnt/x

也是划分为两段的:
```txt
                  - 25.18% read
                     - 25.18% entry_SYSCALL_64_after_hwframe
                        - do_syscall_64
                           - 23.81% ksys_read
                              - 23.76% vfs_read
                                 - 23.04% xfs_file_read_iter
                                    - xfs_file_buffered_read
                                       - 21.95% filemap_read
                                          - 13.19% filemap_get_pages
                                             - 7.70% filemap_get_read_batch
                                                - 6.17% xas_load
                                                     2.33% xas_descend
                                             - 2.45% force_page_cache_ra
                                                - 2.37% page_cache_ra_unbounded
                                                     1.93% up_read
                                               2.33% folio_wait_bit_common
                                          - 7.63% copy_page_to_iter
                                             - _copy_to_iter
                                                  copyout
                           - 1.37% syscall_exit_to_user_mode
                              - 1.33% exit_to_user_mode_prepare
                                   0.87% __rseq_handle_notify_resume
```

```txt
               - do_syscall_64
                  - 23.48% ksys_read
                       vfs_read
                       xfs_file_read_iter
                       xfs_file_buffered_read
                     - filemap_read
                        - 23.48% filemap_get_pages
                           - 14.71% force_page_cache_ra
                              - page_cache_ra_unbounded
                                 - 9.22% read_pages
                                    - 7.55% iomap_readahead
                                       - 3.84% submit_bio_noacct_nocheck
                                          - 3.51% blk_mq_submit_bio
                                             - 1.27% __blk_mq_alloc_requests
                                                - blk_mq_get_tag
                                                   - 1.06% sbitmap_get
                                                      - sbitmap_find_bit
                                                           _find_next_zero_bit
                                             - 0.95% __rq_qos_track
                                                  wbt_track
                                       - 1.63% iomap_readpage_iter
                                          - 1.19% bio_alloc_bioset
                                               0.58% bio_associate_blkg
                                       - 1.41% iomap_iter
                                            xfs_read_iomap_begin
                                    - 1.64% blk_finish_plug
                                       - __blk_flush_plug
                                          - blk_mq_flush_plug_list
                                             - 1.41% nvme_queue_rqs
                                                  0.68% _raw_spin_lock
                                 - 3.45% filemap_add_folio
                                    - 2.39% __filemap_add_folio
                                         0.66% __mem_cgroup_charge
                                    - 1.05% folio_add_lru
                                       - folio_batch_move_lru
                                            0.70% lru_add_fn
                                 - 1.93% folio_alloc
                                    - 1.87% __alloc_pages
                                       - 1.64% get_page_from_freelist
                                          - 1.35% rmqueue_bulk
                                               1.19% __list_del_entry_valid
                           - 8.77% folio_wait_bit_common
                              - 8.77% io_schedule
                                   schedule
                                 - __schedule
                                    - 3.35% dequeue_task_fair
                                       - dequeue_entity
                                            1.16% update_load_avg
                                            0.80% update_curr
                                    - 1.99% psi_task_switch
                                         1.70% psi_group_change
                                    - 1.34% finish_task_switch.isra.0
                                       - 0.57% __perf_event_task_sched_in
                                            0.53% __rcu_read_lock
```

### ioengine=libaio direct=1 rw=randwrite bs=4k filename=/mnt/x

## nfs

### ioengine=libaio direct=1 rw=randread bs=4k filename=/mnt/x

这仅仅是 fio 的进程:

```txt
-   60.10%     1.04%  fio              libc.so.6                                       [.] syscall
   - 59.06% syscall
      - 56.67% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 39.13% __x64_sys_io_submit
               - 38.09% io_submit_one
                  - 34.65% aio_read
                     - 33.08% nfs_file_direct_read
                        - 28.78% nfs_direct_read_schedule_iovec
                           - 21.93% nfs_pageio_complete
                              - 21.79% nfs_pageio_doio
                                 - nfs_generic_pg_pgios
                                    - 17.38% nfs_initiate_pgio
                                       - 16.65% rpc_run_task
                                          - 12.56% rpc_execute
                                             - 11.95% queue_work_on
                                                - 11.58% __queue_work
                                                   - 4.58% _raw_spin_lock
                                                        native_queued_spin_lock_slowpath
                                                   - 4.04% try_to_wake_up
                                                      - 1.36% ttwu_queue_wakelist
                                                         - 0.91% __smp_call_single_queue
                                                              0.57% llist_add_batch
                                                        1.23% _raw_spin_lock_irqsave
                                                        0.62% select_task_rq_fair
                                                     1.26% insert_work
                                          - 2.56% rpc_task_set_transport
                                             - 2.03% xprt_iter_get_next
                                                  0.92% xprt_iter_first_entry
                                                  0.69% xprt_get
                                            0.62% rpc_new_task
                                         0.51% rpc_put_task
                                    - 2.76% nfs_pgheader_init
                                         2.64% nfs_direct_pgio_init
                                      0.83% nfs_generic_pgio
                                    - 0.63% nfs_readhdr_alloc
                                         kmem_cache_alloc
                           - 2.65% iov_iter_get_pages_alloc2
                              - __iov_iter_get_pages_alloc
                                 - 2.04% get_user_pages_fast
                                    - 1.93% internal_get_user_pages_fast
                                         1.16% try_grab_folio
                           - 1.67% nfs_page_create_from_page
                              - 1.00% nfs_page_create
                                   kmem_cache_alloc
                           - 0.78% nfs_pageio_init_read
                                nfs_pageio_init
                           - 0.69% nfs_pageio_add_request
                                __nfs_pageio_add_request
                        - 0.99% nfs_get_lock_context
                             0.89% __nfs_find_lock_context
                          0.66% kmem_cache_alloc
                          0.65% get_nfs_open_context
                       0.52% aio_prep_rw
                       0.50% security_file_permission
                    0.97% fget
                    0.63% kmem_cache_alloc
                    0.53% _copy_from_user
                    0.51% __get_reqs_available
                 0.62% lookup_ioctx
            - 15.46% __x64_sys_io_getevents
               - do_io_getevents
                  - 14.15% read_events
                     - 8.38% schedule
                        - __schedule
                           - 3.02% dequeue_task_fair
                              - dequeue_entity
                                   1.02% update_load_avg
                                   0.74% update_curr
                           - 1.41% psi_task_switch
                                1.14% psi_group_change
                           - 1.24% finish_task_switch.isra.0
                                0.55% __perf_event_task_sched_in
                     - 2.23% aio_read_events
                        - aio_read_events_ring
                             0.84% _copy_to_user
                       1.36% aio_read_events_ring
                     - 1.17% prepare_to_wait_event
                          0.82% _raw_spin_lock_irqsave
                    0.63% lookup_ioctx
            - 1.37% syscall_exit_to_user_mode
               - 1.23% exit_to_user_mode_prepare
                    0.52% __rseq_handle_notify_resume
```


```txt
-   24.31%     0.05%  nfsd             [kernel.kallsyms]                               [k] nfsd                                                          ▒
   - 24.26% nfsd                                                                                                                                         ▒
      - 11.40% svc_send                                                                                                                                  ▒
         - 11.38% svc_tcp_sendto                                                                                                                         ▒
            - 4.33% tcp_sock_set_cork                                                                                                                    ▒
               - 3.81% __tcp_push_pending_frames                                                                                                         ▒
                  - tcp_write_xmit                                                                                                                       ▒
                     - 3.53% __tcp_transmit_skb                                                                                                          ▒
                        - 3.32% __ip_queue_xmit                                                                                                          ▒
                           - 2.61% ip_finish_output2                                                                                                     ▒
                              - 2.59% __dev_queue_xmit                                                                                                   ▒
                                 - 2.43% __local_bh_enable_ip                                                                                            ▒
                                    - do_softirq.part.0                                                                                                  ▒
                                       - 2.41% __do_softirq                                                                                              ▒
                                          - 2.39% net_rx_action                                                                                          ▒
                                             - 2.03% __napi_poll                                                                                         ▒
                                                - process_backlog                                                                                        ▒
                                                   - 1.98% __netif_receive_skb_one_core                                                                  ▒
                                                      - 1.25% ip_local_deliver_finish                                                                    ▒
                                                         - 1.24% ip_protocol_deliver_rcu                                                                 ▒
                                                            - 1.21% tcp_v4_rcv                                                                           ▒
                                                                 0.59% tcp_v4_do_rcv                                                                     ▒
            - 3.56% __mutex_lock.constprop.0                                                                                                             ▒
                 2.59% mutex_spin_on_owner                                                                                                               ▒
                 0.51% osq_lock                                                                                                                          ▒
            - 2.11% svc_tcp_sendmsg                                                                                                                      ▒
               - 1.13% kernel_sendmsg                                                                                                                    ▒
                  - 1.09% tcp_sendmsg                                                                                                                    ▒
                       0.72% tcp_sendmsg_locked                                                                                                          ▒
               - 0.91% kernel_sendpage                                                                                                                   ▒
                  - inet_sendpage                                                                                                                        ▒
                       0.88% tcp_sendpage                                                                                                                ▒
            - 0.86% release_sock                                                                                                                         ▒
                 0.51% __wake_up_common_lock                                                                                                             ▒
      - 6.92% svc_recv                                                                                                                                   ▒
         - 4.34% svc_tcp_recvfrom                                                                                                                        ▒
            - 2.35% svc_tcp_read_marker.constprop.0                                                                                                      ▒
               - 2.19% svc_tcp_sock_recv_cmsg                                                                                                            ▒
                  - sock_recvmsg                                                                                                                         ▒
                     - 2.12% inet_recvmsg                                                                                                                ▒
                        - 2.11% tcp_recvmsg                                                                                                              ▒
                           - 1.28% lock_sock_nested                                                                                                      ▒
                              - 1.09% __lock_sock                                                                                                        ▒
                                 - 0.79% schedule                                                                                                        ▒
                                      0.66% __schedule                                                                                                   ▒
            - 1.12% svc_xprt_received                                                                                                                    ▒
               - 1.07% svc_xprt_enqueue                                                                                                                  ▒
                    0.54% try_to_wake_up                                                                                                                 ▒
            - 0.66% svc_tcp_read_msg                                                                                                                     ▒
               - 0.55% svc_tcp_sock_recv_cmsg                                                                                                            ▒
                    sock_recvmsg                                                                                                                         ▒
         - 0.90% schedule_timeout                                                                                                                        ▒
            - 0.67% schedule                                                                                                                             ▒
                 0.64% __schedule
           0.82% __alloc_pages_bulk                                                                                                                      ▒
      - 5.66% svc_process                                                                                                                                ▒
         - 5.62% svc_process_common                                                                                                                      ▒
            - 4.51% nfsd_dispatch                                                                                                                        ▒
               - 4.10% nfsd3_proc_read                                                                                                                   ▒
                  - 4.05% nfsd_read                                                                                                                      ▒
                     - 2.13% nfsd_file_do_acquire                                                                                                        ▒
                        - 1.86% fh_verify                                                                                                                ▒
                           - 0.55% nfsd_setuser_and_check_port                                                                                           ▒
                                0.54% nfsd_setuser                                                                                                       ▒
                     - 1.77% nfsd_splice_read                                                                                                            ▒
                        - 1.41% splice_direct_to_actor                                                                                                   ▒
                           - 1.18% generic_file_splice_read                                                                                              ▒
                              - 1.09% filemap_read                                                                                                       ▒
                                 - 0.96% filemap_get_pages                                                                                               ▒
                                    - filemap_get_read_batch
```
### ioengine=sync rw=randread bs=4k filename=/mnt/x

```txt
  - 17.32% ksys_read
      - 17.28% vfs_read
         - 16.90% nfs_file_read
            - 16.29% filemap_read
               - 13.47% filemap_get_pages
                  - 7.60% force_page_cache_ra
                     - 7.56% page_cache_ra_unbounded
                        - 5.77% read_pages
                           - 5.72% nfs_readahead
                              - 4.18% nfs_pageio_complete_read
                                 - nfs_pageio_complete
                                    - 4.15% nfs_pageio_doio
                                       - nfs_generic_pg_pgios
                                          - 3.38% nfs_initiate_pgio
                                             - 3.28% rpc_run_task
                                                - 2.41% rpc_execute
                                                   - 2.28% queue_work_on
                                                      - 2.19% __queue_work
                                                           1.27% try_to_wake_up
                                                  0.59% rpc_task_set_transport
                                          - 0.64% nfs_generic_pgio
                                               0.57% get_nfs_open_context
                              - 1.01% nfs_read_add_folio
                                   0.80% nfs_page_create_from_folio
                        - 1.22% filemap_add_folio
                             0.70% __filemap_add_folio
                             0.51% folio_add_lru
                  - 3.67% folio_wait_bit_common
                     - 3.03% io_schedule
                        - schedule
                           - __schedule
                              - 1.10% dequeue_task_fair
                                   dequeue_entity
                                0.63% psi_task_switch
                  - 1.92% filemap_get_read_batch
                     - 1.46% xas_load
                          0.55% xas_descend
               - 2.29% copy_page_to_iter
                  - _copy_to_iter
                       copyout
```

### ioengine=sync rw=randwrite bs=4k filename=/mnt/x

```txt
-   66.66%     0.87%  fio              libc.so.6                                [.] __GI___libc_write
   - 65.79% __GI___libc_write
      - 64.40% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 63.82% ksys_write
               - 63.58% vfs_write
                  - 62.38% nfs_file_write
                     - 59.50% generic_perform_write
                        - 32.82% nfs_write_end
                           - 23.84% nfs_update_folio
                              - 10.82% nfs_page_create_from_folio
                                 - 7.66% nfs_get_lock_context
                                      7.19% __nfs_find_lock_context
                                 - 2.24% nfs_page_create
                                    - kmem_cache_alloc
                                         0.99% ___slab_alloc
                              - 6.97% filemap_dirty_folio
                                 - 5.54% __folio_mark_dirty
                                    - 2.39% __xa_set_mark
                                         1.88% xas_set_mark
                                    - 1.48% _raw_spin_lock_irqsave
                                         native_queued_spin_lock_slowpath
                                   0.56% __mark_inode_dirty
                                1.00% _raw_spin_lock
                                0.62% nfs_unlock_and_release_request
                        - 23.12% nfs_write_begin
                           - 22.40% __filemap_get_folio
                              - 14.47% filemap_get_entry
                                 - 10.55% xas_load
                                      4.75% xas_descend
                              - 4.20% filemap_add_folio
                                 - 2.75% __filemap_add_folio
                                      0.62% _raw_spin_lock_irq
                                      0.58% __mem_cgroup_charge
                                 - 1.40% folio_add_lru
                                    - folio_batch_move_lru
                                       - 0.83% lru_add_fn
                                            0.52% lru_gen_add_folio
                              - 3.08% folio_alloc
                                 - 2.85% __alloc_pages
                                    - 2.57% get_page_from_freelist
                                       - 1.93% rmqueue_bulk
                                            1.69% __list_del_entry_valid
                        - 2.06% copy_page_from_iter_atomic
                             copyin
                        - 0.61% fault_in_iov_iter_readable
                             fault_in_readable
                       0.57% nfs_clear_invalid_mapping
        0.81% __entry_text_start
```

## [ ] ext4

## ext2

### ioengine=libaio direct=1 rw=randread bs=4k filename=/mnt/x
```txt
   - io_wq_worker
      - 60.69% io_worker_handle_work
         - 51.22% io_wq_submit_work
            - io_issue_sqe
               - io_read
                  - __io_read
                     - 49.07% ext2_file_read_iter
                        - 46.39% iomap_dio_rw
                           - __iomap_dio_rw
                              - 23.33% iomap_iter
                                 - 22.33% ext2_iomap_begin
                                    - ext2_get_blocks
                                       - 19.72% ext2_get_branch
                                          - 13.06% __bread_gfp
                                             - 12.66% __find_get_block
                                                - 5.07% __filemap_get_folio
                                                   - 4.95% filemap_get_entry
                                                        3.15% xas_load
                                                  1.00% _raw_spin_lock
                                                  0.58% _raw_spin_unlock
                                            1.73% _raw_read_lock
                                            1.26% _raw_read_unlock
                                         0.73% __brelse
                                         0.57% ext2_block_to_path.isra.0
                              - 14.22% iomap_dio_bio_iter
                                 - 6.04% submit_bio_noacct_nocheck
                                    - 4.46% __submit_bio
                                       - blk_mq_submit_bio
                                          - 1.57% __blk_mq_alloc_requests
                                             - 0.95% blk_mq_get_tag
                                                - 0.66% sbitmap_get
                                                     sbitmap_find_bit
                                            0.84% blk_account_io_start
                                    - 1.00% ktime_get
                                       - 0.74% kvm_clock_get_cycles
                                            pvclock_clocksource_read_nowd
                                 - 4.34% bio_iov_iter_get_pages
                                       - 3.67% pin_user_pages_fast
                                            3.53% gup_fast_fallback
                                 - 1.35% bio_alloc_bioset
                                    - 0.77% bio_associate_blkg
                                         0.59% bio_associate_blkg_from_css
                                   1.03% bio_set_pages_dirty
                              - 6.36% blk_finish_plug
                                 - __blk_flush_plug
                                    - blk_mq_flush_plug_list
                                       - 5.67% virtio_queue_rqs
                                            3.95% _raw_spin_unlock_irqrestore
                                          - 1.36% virtblk_prep_rq.isra.0
                                               0.51% sg_alloc_table_chained
                          1.23% up_read
                          1.19% down_read
                       0.72% io_rw_init_file
         - 2.74% _raw_spin_lock
              0.88% __pv_queued_spin_lock_slowpath
           2.62% _raw_spin_unlock_irq
         - 2.09% _raw_spin_unlock
              1.45% __raw_callee_save___pv_queued_spin_unlock
              0.52% preempt_count_sub
      - 6.76% schedule_timeout
         - 5.29% schedule
            - 4.83% __schedule
                 3.74% finish_task_switch.isra.0
           0.59% io_wq_worker_running
        0.87% _raw_spin_lock
```

## bcache

### randwrite
xfs randwrite sata ssd 作为 cache ，hdd 作为 backstore , writeback

看上去，大多数时候都是命中的 ssd ，bcache 的函数都没出现
```txt
 - 69.72% syscall
      - 68.48% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 64.53% __x64_sys_io_submit
               - 62.18% io_submit_one
                  - 59.23% aio_write
                     - 58.00% xfs_file_write_iter
                        - xfs_file_dio_write_aligned
                           - 56.45% iomap_dio_rw
                              - __iomap_dio_rw
                                 - 29.59% iomap_dio_bio_iter
                                    - 24.03% submit_bio_noacct_nocheck
                                       - 18.84% __submit_bio
                                          - 18.53% cached_dev_submit_bio
                                             - 12.07% bch_data_insert_start
                                                  10.31% bch_alloc_sectors
                                                  0.73% __bch_submit_bbio
                                               0.75% bio_init_clone
                                             - 0.71% bio_associate_blkg
                                                  0.63% bio_associate_blkg_from_css
                                               0.67% bch_keybuf_check_overlapping
                                       - 4.65% blk_mq_submit_bio
                                          - 1.26% __rq_qos_throttle
                                             - wbt_wait
                                                - 1.15% rq_qos_wait
                                                   - 0.91% io_schedule
                                                      - schedule
                                                           0.87% __schedule
                                          - 1.23% __blk_mq_alloc_requests
                                             - 0.74% blk_mq_get_tag
                                                  0.54% sbitmap_get_shallow
                                          - 0.94% dd_bio_merge
                                               0.71% blk_mq_sched_try_merge
                                    - 3.30% bio_iov_iter_get_pages
                                       - 3.01% iov_iter_extract_pages
                                          - 2.78% pin_user_pages_fast
                                             - 2.62% internal_get_user_pages_fast
                                                  1.62% try_grab_folio
                                    - 1.32% bio_alloc_bioset
                                       - 0.64% bio_associate_blkg
                                            0.51% bio_associate_blkg_from_css
                                         0.54% mempool_alloc
                                 - 13.14% iomap_iter
                                    - xfs_direct_write_iomap_begin
                                       - 11.67% xfs_bmapi_read
                                            11.14% xfs_iext_lookup_extent
                                 - 8.95% blk_finish_plug
                                    - __blk_flush_plug
                                       - 8.87% blk_mq_flush_plug_list.part.0
                                          - 7.87% blk_mq_run_hw_queue
                                             - 7.64% blk_mq_sched_dispatch_requests
                                                - __blk_mq_sched_dispatch_requests
                                                   - 6.00% blk_mq_dispatch_rq_list
                                                      - 5.70% scsi_queue_rq
                                                         - 4.17% ata_scsi_queuecmd
                                                            - 2.87% _raw_spin_lock_irqsave
                                                                 native_queued_spin_lock_slowpath
                                                            - 1.14% __ata_scsi_queuecmd
                                                                 0.68% ata_qc_issue
                                                           0.69% sd_init_command
                                            0.77% dd_insert_requests
                           - 0.68% xfs_file_write_checks
                                0.51% file_modified_flags
                    0.69% _copy_from_user
                    0.64% kmem_cache_alloc
                    0.63% fget
                 1.44% lookup_ioctx
            - 2.80% __x64_sys_io_getevents
               - do_io_getevents
                  - 2.07% read_events
                     - 1.62% aio_read_events_ring
                          0.58% _copy_to_user
                    0.63% lookup_ioctx
              0.56% syscall_exit_to_user_mode
```

### randread

从性能行，randread 的性能要差很多，似乎很多没没有命中?
```txt
   62.87%     0.96%  fio              libc.so.6                        [.] syscall
   - 61.92% syscall
      - 60.24% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 49.21% __x64_sys_io_submit
               - 48.44% io_submit_one
                  - 45.69% aio_read
                     - 44.75% xfs_file_read_iter
                        - xfs_file_dio_read
                           - 43.84% iomap_dio_rw
                              - __iomap_dio_rw
                                 - 29.99% iomap_dio_bio_iter
                                    - 25.37% submit_bio_noacct_nocheck
                                       - 21.48% __submit_bio
                                          - 21.13% cached_dev_submit_bio
                                             - 17.30% cache_lookup
                                                - 16.82% bch_btree_map_keys
                                                   - 16.28% bch_btree_map_keys_recurse
                                                      - 12.58% bch_btree_map_keys_recurse
                                                         - 8.66% __bch_btree_iter_init
                                                              8.17% __bch_bset_search
                                                         - 3.06% cache_lookup_fn
                                                            - 1.60% cached_dev_cache_miss
                                                               - 1.47% bch_btree_insert_check_key
                                                                  - 1.37% bch_btree_insert_node
                                                                     - 1.25% bch_btree_init_next
                                                                          bch_btree_sort_partial
                                                                        - __btree_sort
                                                                             1.09% btree_mergesort
                                                              0.72% __bch_submit_bbio
                                                         - 0.71% bch_btree_iter_next_filter
                                                              0.58% bch_extent_bad
                                                      - 1.82% __bch_btree_iter_init
                                                           1.65% __bch_bset_search
                                                        1.13% bch_btree_node_get.part.0
                                       - 3.33% blk_mq_submit_bio
                                          - 1.24% dd_bio_merge
                                               0.58% _raw_spin_lock
                                               0.55% blk_mq_sched_try_merge
                                          - 0.79% __blk_mq_alloc_requests
                                               0.51% blk_mq_get_tag
                                    - 2.80% bio_iov_iter_get_pages
                                       - 2.54% iov_iter_extract_pages
                                          - 2.39% pin_user_pages_fast
                                             - 2.24% internal_get_user_pages_fast
                                                  1.29% try_grab_folio
                                      0.90% bio_alloc_bioset
                                 - 8.87% iomap_iter
                                    - xfs_read_iomap_begin
                                       - 7.85% xfs_bmapi_read
                                            7.55% xfs_iext_lookup_extent
                                 - 3.17% blk_finish_plug
                                    - __blk_flush_plug
                                       - 3.09% blk_mq_flush_plug_list.part.0
                                          - 1.93% blk_mq_run_hw_queue
                                             - 1.69% blk_mq_sched_dispatch_requests
                                                - __blk_mq_sched_dispatch_requests
                                                   - 1.34% scsi_mq_get_budget
                                                      - 1.08% sbitmap_get
                                                         - sbitmap_find_bit
                                                              _find_next_zero_bit
                                            0.94% dd_insert_requests
                                 - 0.58% kmalloc_trace
                                      __kmem_cache_alloc_node
                    0.68% fget
                    0.67% kmem_cache_alloc
                 0.55% lookup_ioctx
            - 9.82% __x64_sys_io_getevents
               - do_io_getevents
                  - 9.13% read_events
                     - 5.97% schedule
                        - __schedule
                           - 2.84% dequeue_task_fair
                              - dequeue_entity
                                   0.92% update_curr
                                   0.63% update_load_avg
                           - 1.15% psi_task_switch
                                0.89% psi_group_change
                             0.66% finish_task_switch.isra.0
                     - 1.33% aio_read_events
                        - aio_read_events_ring
                             0.65% _copy_to_user
                     - 0.77% prepare_to_wait_event
                          0.54% _raw_spin_lock_irqsave
                    0.52% lookup_ioctx
            - 0.99% syscall_exit_to_user_mode
                 0.93% exit_to_user_mode_prepare
```

## scsi debug

scsi_debug 在搞什么飞机，对于内存写，居然只有 60k 的 iops

难道是因为存在 1ms 的延迟导致的，我靠，延迟增加到 10ms ，然后性能就只有 6400 了，100ms 就只有 640
这样看来完全是受延迟的影响。 （背后的理论是什么

如果 delay 设置为 0 ，为 400K 的

```txt
-   41.42%     0.80%  fio              libc.so.6                               [.] syscall
   - 40.62% syscall
      - 38.16% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 15.27% __x64_sys_io_submit
               - 14.50% io_submit_one
                  - 12.75% aio_read
                     - 11.15% blkdev_read_iter
                        - 10.98% __blkdev_direct_IO_async
                           - 8.08% submit_bio_noacct_nocheck
                              - 7.63% blk_mq_submit_bio
                                 - 4.38% kblockd_mod_delayed_work_on
                                    - 4.37% mod_delayed_work_on
                                       - 3.49% __queue_work
                                          - 2.88% try_to_wake_up
                                             - 1.99% ttwu_do_activate
                                                - 1.32% enqueue_task
                                                   - 0.78% enqueue_task_fair
                                                        0.57% enqueue_entity
                                                     0.51% psi_task_change
                                                - 0.63% check_preempt_curr
                                                     check_preempt_wakeup
                                         0.56% asm_sysvec_apic_timer_interrupt
                                 - 1.26% __blk_mq_alloc_requests
                                    - 0.75% blk_mq_get_tag
                                       - 0.65% sbitmap_get
                                            0.55% sbitmap_find_bit
                                   0.53% blk_mq_insert_request
                           - 1.53% bio_iov_iter_get_pages
                              - 1.31% iov_iter_extract_pages
                                 - 1.15% pin_user_pages_fast
                                      1.09% internal_get_user_pages_fast
                             0.85% bio_alloc_bioset
                       0.75% __fsnotify_parent
            - 15.04% __x64_sys_io_getevents
               - 14.95% do_io_getevents
                  - 14.44% read_events
                     - 12.43% schedule
                        - __schedule
                           - 9.34% pick_next_task_fair
                              - newidle_balance
                                 - 8.07% load_balance
                                    - 7.89% find_busiest_group
                                       - update_sd_lb_stats.constprop.0
                                            1.40% cpu_util.constprop.0
                                            0.74% _find_next_and_bit
                                            0.65% idle_cpu
                                   0.92% update_blocked_averages
                           - 1.33% dequeue_task_fair
                                dequeue_entity
                           - 0.80% psi_task_switch
                                0.69% psi_group_change
                       0.68% aio_read_events_ring
                     - 0.59% aio_read_events
                          0.56% aio_read_events_ring
            - 7.51% syscall_exit_to_user_mode
               - 7.37% exit_to_user_mode_prepare
                  - 5.60% schedule
                     - 5.55% __schedule
                        - 2.46% pick_next_task_fair
                           - 1.99% put_prev_entity
                                1.01% update_load_avg
                                0.70% update_curr
                        - 1.31% psi_task_switch
                             0.98% psi_group_change
                          0.79% finish_task_switch.isra.0
                    1.05% __rseq_handle_notify_resume
        0.52% save_fpregs_to_fpstate
```

## nullblk

使用 aio 可以达到 1300K

```txt
-   72.12%     2.83%  fio              libc.so.6          [.] syscall
   - 69.29% syscall
      - 65.61% entry_SYSCALL_64_after_hwframe
         - do_syscall_64
            - 56.11% __x64_sys_io_submit
               - 52.72% io_submit_one
                  - 41.81% aio_write
                     - 38.74% blkdev_write_iter
                        - 38.03% __generic_file_write_iter
                           - 36.75% generic_file_direct_write
                              - 34.47% __blkdev_direct_IO_async
                                 - 21.38% submit_bio_noacct_nocheck
                                    - 19.67% blk_mq_submit_bio
                                       - 11.54% blk_mq_try_issue_directly
                                          - 11.29% __blk_mq_issue_directly
                                             - 9.34% null_handle_cmd
                                                - 6.83% blk_mq_end_request
                                                   - 4.87% blk_update_request
                                                      - 2.20% blkdev_bio_end_io_async
                                                         - 1.42% __bio_release_pages
                                                              0.98% gup_put_folio
                                                           0.57% aio_complete_rw
                                                        0.87% kmem_cache_free
                                                        0.75% bio_endio
                                                   - 1.19% ktime_get
                                                        read_tsc
                                                - 1.01% __blk_mq_free_request
                                                     0.74% sbitmap_queue_clear
                                                  0.59% blk_mq_free_request
                                             - 1.79% null_queue_rq
                                                - blk_mq_start_request
                                                   - 1.20% ktime_get
                                                        read_tsc
                                       - 4.50% __blk_mq_alloc_requests
                                          - 2.15% blk_mq_get_tag
                                             - 1.61% sbitmap_get
                                                  sbitmap_find_bit
                                          - 1.33% ktime_get
                                               read_tsc
                                         0.64% blk_mq_sched_bio_merge
                                    - 0.87% ktime_get
                                         read_tsc
                                 - 7.76% bio_iov_iter_get_pages
                                    - 6.71% iov_iter_extract_pages
                                       - 6.04% pin_user_pages_fast
                                          - 5.71% internal_get_user_pages_fast
                                               1.61% try_grab_folio
                                 - 3.71% bio_alloc_bioset
                                    - 1.70% bio_associate_blkg
                                         1.23% bio_associate_blkg_from_css
                                    - 1.33% mempool_alloc
                                         0.90% kmem_cache_alloc
                                1.40% invalidate_inode_pages2_range
                             0.80% file_update_time
                     - 0.92% security_file_permission
                          0.62% selinux_file_permission
                       0.63% aio_prep_rw
                  - 2.30% aio_complete
                       0.89% _raw_spin_lock_irqsave
                    1.97% _copy_from_user
                    1.17% kmem_cache_alloc
                    0.96% __put_user_4                                                                                               ▒
                    0.88% kmem_cache_free                                                                                            ▒
                    0.86% fget                                                                                                       ▒
               - 1.93% lookup_ioctx                                                                                                  ▒
                    0.74% __get_user_4                                                                                               ▒
                 0.71% __get_user_8                                                                                                  ▒
            - 6.79% __x64_sys_io_getevents                                                                                           ▒
               - do_io_getevents                                                                                                     ▒
                  - 4.18% read_events                                                                                                ▒
                     - 3.95% aio_read_events_ring                                                                                    ▒
                          1.18% _copy_to_user                                                                                        ▒
                          1.00% __check_object_size                                                                                  ▒
                  - 2.07% lookup_ioctx                                                                                               ▒
                       0.86% __get_user_4                                                                                            ▒
            - 1.17% syscall_exit_to_user_mode                                                                                        ▒
                 0.55% exit_to_user_mode_prepare                                                                                     ▒
        0.78% entry_SYSCALL_64                                                                                                       ▒
        0.75% entry_SYSCALL_64_safe_stack
```

## kvm

### guest fio, io_uring backend

```txt
-   73.17%     0.10%  qemu-system-x86  [k] entry_SYSCALL_64_after_hwframe
   - 73.08% entry_SYSCALL_64_after_hwframe
      - do_syscall_64
         - 37.15% __x64_sys_ppoll
            - 36.85% do_sys_poll
               - 8.26% poll_freewait
                  - 5.19% remove_wait_queue
                       3.91% _raw_spin_lock_irqsave
                       0.51% __list_del_entry_valid
                    1.63% fput
                    0.69% _raw_spin_unlock_irqrestore
               - 7.10% sock_poll
                  - 5.78% udp_poll
                     - datagram_poll
                        - 1.93% add_wait_queue
                             1.35% _raw_spin_lock_irqsave
                          1.24% __pollwait
               - 6.55% __fget_light
                    0.69% __rcu_read_unlock
               - 4.37% eventfd_poll
                  - 2.36% add_wait_queue
                       1.72% _raw_spin_lock_irqsave
                    1.32% __pollwait
                 3.13% fput
               - 2.73% schedule_hrtimeout_range_clock
                  - 2.40% schedule
                     - __schedule
                        - 1.07% dequeue_task_fair
                             dequeue_entity
                        - 0.63% psi_task_switch
                             0.53% psi_group_change
               - 0.80% tty_poll
                    0.56% n_tty_poll
         - 29.87% __x64_sys_ioctl
            - 29.87% kvm_vcpu_ioctl
               - kvm_arch_vcpu_ioctl_run
                  - 22.06% kvm_vcpu_halt
                     - 11.58% kvm_vcpu_check_block
                        - 8.26% kvm_arch_vcpu_runnable
                           - 6.21% kvm_cpu_has_interrupt
                              - 3.21% apic_has_interrupt_for_ppr
                                 - 2.98% vmx_sync_pir_to_irr
                                      1.43% kvm_lapic_find_highest_irr
                                      0.65% vmx_set_rvi
                              - 2.30% kvm_apic_has_interrupt
                                   __apic_update_ppr
                           - 1.02% vmx_interrupt_allowed
                                vmx_interrupt_blocked
                          1.49% __srcu_read_lock
                          0.94% __srcu_read_unlock
                     - 3.48% ktime_get
                          1.94% read_tsc
                  - 3.95% vmx_vcpu_run
                       0.90% native_write_msr
                     - 0.85% kvm_load_host_xsave_state
                          native_write_msr
                     - 0.74% kvm_load_guest_xsave_state
                          native_write_msr
                  - 1.10% vmx_handle_exit
                     - 0.91% handle_ept_misconfig
                        - kvm_io_bus_write
                           - __kvm_io_bus_write
                              - 0.65% ioeventfd_write
                                 - 0.61% eventfd_signal_mask
                                      0.50% __wake_up_common
                    0.84% handle_fastpath_set_msr_irqoff
         - 4.45% __do_sys_io_uring_enter
            - 4.28% io_submit_sqes
               - 4.07% io_issue_sqe
                  - 4.00% io_read
                     - 3.65% xfs_file_read_iter
                        - xfs_file_buffered_read
                           - 3.51% filemap_read
                              - 1.52% copy_page_to_iter
                                 - _copy_to_iter
                                      copyout
                              - 1.45% filemap_get_pages
                                 - filemap_get_read_batch
                                      0.97% xas_load
         - 0.69% ksys_write
              0.58% vfs_write
```

## 利用 code/module/user/src/create_file_in_dir.c 进行测试

```txt
-   95.64%     0.01%  create_file_in_  [kernel.kallsyms]                               [k] entry_SYSCALL_64_after_hwframe                                            ◆
     95.63% entry_SYSCALL_64_after_hwframe                                                                                                                           ▒
      - do_syscall_64                                                                                                                                                ▒
         - 80.15% __x64_sys_openat                                                                                                                                   ▒
            - do_sys_openat2                                                                                                                                         ▒
               - 79.11% do_filp_open                                                                                                                                 ▒
                  - 79.01% path_openat                                                                                                                               ▒
                     - 31.56% ext4_create                                                                                                                            ▒
                        - 18.23% ext4_add_nondir                                                                                                                     ▒
                           - 16.60% ext4_add_entry                                                                                                                   ▒
                              - 16.47% ext4_dx_add_entry                                                                                                             ▒
                                 - 11.89% add_dirent_to_buf                                                                                                          ▒
                                    - 7.78% ext4_find_dest_de                                                                                                        ▒
                                         4.56% __ext4_check_dir_entry                                                                                                ▒
                                       - 2.42% ext4_match                                                                                                            ▒
                                            0.52% fscrypt_match_name                                                                                                 ▒
                                    - 2.06% __ext4_mark_inode_dirty                                                                                                  ▒
                                       - 1.13% ext4_reserve_inode_write                                                                                              ▒
                                          - 0.72% ext4_get_inode_loc                                                                                                 ▒
                                             - __ext4_get_inode_loc                                                                                                  ▒
                                                - 0.70% __getblk_gfp                                                                                                 ▒
                                                     __find_get_block                                                                                                ▒
                                       - 0.92% ext4_mark_iloc_dirty                                                                                                  ▒
                                          - 0.67% ext4_fill_raw_inode                                                                                                ▒
                                             - 0.58% ext4_inode_csum_set                                                                                             ▒
                                                  ext4_inode_csum                                                                                                    ▒
                                    - 1.03% __ext4_handle_dirty_metadata                                                                                             ▒
                                       - 1.02% jbd2_journal_dirty_metadata                                                                                           ▒
                                          - 0.92% _raw_spin_lock                                                                                                     ▒
                                               0.88% native_queued_spin_lock_slowpath                                                                                ▒
                                    - 0.65% ext4_handle_dirty_dirblock                                                                                               ▒
                                         0.64% crc32c_pcl_intel_update                                                                                               ▒
                                 - 1.70% __ext4_journal_get_write_access                                                                                             ▒
                                    - 1.68% jbd2_journal_get_write_access                                                                                            ▒
                                       - 1.50% do_get_write_access                                                                                                   ▒
                                          - 1.32% _raw_spin_lock                                                                                                     ▒
                                               native_queued_spin_lock_slowpath                                                                                      ▒
                                 - 1.30% dx_probe                                                                                                                    ▒
                                    - 1.10% __ext4_read_dirblock                                                                                                     ▒
                                       - 1.04% ext4_bread                                                                                                            ▒
                                          - ext4_getblk                                                                                                              ▒
                                             - 0.87% ext4_map_blocks                                                                                                 ▒
                                                  0.66% ext4_es_lookup_extent                                                                                        ▒
                                   0.79% do_split                                                                                                                    ▒
                                 - 0.57% __ext4_read_dirblock                                                                                                        ▒
                                    - 0.55% ext4_bread                                                                                                               ▒
                                         ext4_getblk                                                                                                                 ▒
                           - 1.00% d_instantiate_new                                                                                                                 ▒
                                0.72% _raw_spin_lock                                                                                                                 ▒
                             0.62% __ext4_mark_inode_dirty                                                                                                           ▒
                        - 12.86% __ext4_new_inode
                           - 3.46% __ext4_mark_inode_dirty                                                                                                                    ▒
                              - 1.76% ext4_mark_iloc_dirty                                                                                                                    ▒
                                 - 0.86% ext4_fill_raw_inode                                                                                                                  ▒
                                    - 0.75% ext4_inode_csum_set                                                                                                               ▒
                                       - ext4_inode_csum                                                                                                                      ▒
                                            0.68% crc32c_pcl_intel_update                                                                                                     ▒
                              - 1.69% ext4_reserve_inode_write                                                                                                                ▒
                                 - 1.31% __ext4_journal_get_write_access                                                                                                      ▒
                                    - 1.26% jbd2_journal_get_write_access                                                                                                     ▒
                                       - 0.75% do_get_write_access                                                                                                            ▒
                                          - 0.73% _raw_spin_lock                                                                                                              ▒
                                               native_queued_spin_lock_slowpath                                                                                               ▒
                           - 1.60% new_inode                                                                                                                                  ▒
                              - 1.45% alloc_inode                                                                                                                             ▒
                                 - 1.19% ext4_alloc_inode                                                                                                                     ▒
                                    - 1.07% kmem_cache_alloc_lru                                                                                                              ▒
                                       - 0.72% ___slab_alloc                                                                                                                  ▒
                                          - 0.69% allocate_slab                                                                                                               ▒
                                               0.52% setup_object                                                                                                             ▒
                             1.52% insert_inode_locked                                                                                                                        ▒
                           - 1.23% __ext4_journal_start_sb                                                                                                                    ▒
                              - 1.17% jbd2__journal_start                                                                                                                     ▒
                                   1.06% start_this_handle                                                                                                                    ▒
                             0.63% get_random_u32                                                                                                                             ▒
                             0.62% ext4_get_group_desc                                                                                                                        ▒
                     - 25.65% rwsem_down_write_slowpath                                                                                                                       ▒
                          24.99% rwsem_spin_on_owner                                                                                                                          ▒
                     - 13.96% ext4_lookup                                                                                                                                     ▒
                        - 13.35% __ext4_find_entry                                                                                                                            ▒
                           - 13.33% ext4_dx_find_entry                                                                                                                        ▒
                              - 6.26% ext4_search_dir                                                                                                                         ▒
                                 - 5.61% ext4_match                                                                                                                           ▒
                                      2.65% fscrypt_match_name                                                                                                                ▒
                              - 3.49% dx_probe                                                                                                                                ▒
                                 - 2.68% __ext4_read_dirblock                                                                                                                 ▒
                                    - 2.64% ext4_bread                                                                                                                        ▒
                                       - ext4_getblk                                                                                                                          ▒
                                          - 1.42% __getblk_gfp                                                                                                                ▒
                                               1.39% __find_get_block                                                                                                         ▒
                                          - 1.04% ext4_map_blocks                                                                                                             ▒
                                               0.56% ext4_es_lookup_extent                                                                                                    ▒
                                   0.62% ext4fs_dirhash                                                                                                                       ▒
                              - 3.38% __ext4_read_dirblock                                                                                                                    ▒
                                 - 3.34% ext4_bread
                                    - ext4_getblk                                                                                                                  ▒
                                       - 2.14% __getblk_gfp                                                                                                        ▒
                                          - 2.13% __find_get_block                                                                                                 ▒
                                             - 0.72% pagecache_get_page                                                                                            ▒
                                                  0.71% __filemap_get_folio                                                                                        ▒
                                       - 1.20% ext4_map_blocks                                                                                                     ▒
                                            0.84% ext4_es_lookup_extent                                                                                            ▒
                     - 1.59% do_dentry_open                                                                                                                        ▒
                        - 0.87% ext4_file_open                                                                                                                     ▒
                             0.63% fscrypt_file_open                                                                                                               ▒
                     - 1.56% d_alloc_parallel                                                                                                                      ▒
                        - 1.14% d_alloc                                                                                                                            ▒
                           - 0.71% __d_alloc                                                                                                                       ▒
                                0.66% kmem_cache_alloc_lru                                                                                                         ▒
                     - 1.01% alloc_empty_file                                                                                                                      ▒
                          1.01% __alloc_file                                                                                                                       ▒
                       0.59% link_path_walk.part.0.constprop.0                                                                                                     ▒
                       0.54% try_to_unlazy
```

看上去在 vfs 中就已经上锁了，非常符合逻辑

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
