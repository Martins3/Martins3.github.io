## rsync

```txt
-   22.41%     0.09%  rsync    [kernel.kallsyms]   [k] entry_SYSCALL_64_after_hwframe                                         ▒
     22.32% entry_SYSCALL_64_after_hwframe                                                                                    ▒
      - do_syscall_64                                                                                                         ▒
         - 11.28% ksys_read                                                                                                   ▒
            - 11.22% vfs_read                                                                                                 ▒
               - xfs_file_read_iter                                                                                           ▒
                  - xfs_file_buffered_read                                                                                    ▒
                     - 11.16% filemap_read                                                                                    ▒
                        - 8.83% copy_page_to_iter                                                                             ▒
                           - _copy_to_iter                                                                                    ▒
                                copyout                                                                                       ▒
                        - 2.32% filemap_get_pages                                                                             ▒
                           - 2.10% page_cache_ra_order                                                                        ▒
                              - 0.87% filemap_add_folio                                                                       ▒
                                   0.73% __filemap_add_folio                                                                  ▒
                              - 0.75% read_pages                                                                              ▒
                                   0.53% iomap_readahead                                                                      ▒
         - 8.73% ksys_write                                                                                                   ▒
            - 8.71% vfs_write                                                                                                 ▒
               - 8.51% sock_write_iter                                                                                        ▒
                  - 8.41% unix_stream_sendmsg                                                                                 ▒
                     - 5.09% skb_copy_datagram_from_iter                                                                      ▒
                        - 4.89% copy_page_from_iter                                                                           ▒
                           - _copy_from_iter                                                                                  ▒
                             copyin                                                                                           ▒
                     - 2.10% sock_alloc_send_pskb                                                                             ▒
                        - 2.03% alloc_skb_with_frags                                                                          ▒
                           - 1.14% __alloc_skb                                                                                ▒
                              - 0.71% kmalloc_reserve                                                                         ▒
                                 - __kmalloc_node_track_caller                                                                ▒
                                   __kmem_cache_alloc_node                                                                    ▒
                           - 0.87% __alloc_pages                                                                              ▒
                                0.58% get_page_from_freelist                                                                  ▒
                       1.06% _raw_spin_lock                                                                                   ▒
         - 2.10% __x64_sys_pselect6                                                                                           ▒
            - do_pselect.constprop.0                                                                                          ▒
               - 1.87% core_sys_select                                                                                        ▒
                  - 1.55% do_select                                                                                           ▒
                     - 0.95% schedule_hrtimeout_range_clock                                                                   ▒
                        - 0.89% schedule                                                                                      ▒
                             __schedule
```

## iperf 一个网卡打一个网卡创建出来的 bridge

```txt
  - 99.97% iperf_run_client
      - 97.23% iperf_send
         - 96.84% iperf_tcp_send
            - 96.68% __GI___libc_write
               - 96.61% entry_SYSCALL_64
                  - 96.42% do_syscall_64
                     - 96.14% ksys_write
                        - 96.06% vfs_write
                           - 95.29% sock_write_iter
                              - 94.87% tcp_sendmsg
                                 - 64.79% tcp_sendmsg_locked
                                    - 37.93% tcp_write_xmit
                                       - 32.61% __tcp_transmit_skb
                                          - 26.75% __ip_queue_xmit
                                             - 21.89% ip_finish_output2
                                                - 21.49% __dev_queue_xmit
                                                   - 19.92% __local_bh_enable_ip
                                                      - 19.74% do_softirq.part.0
                                                         - 19.59% __do_softirq
                                                            - 19.45% net_rx_action
                                                               - 18.64% __napi_poll
                                                                  - process_backlog
                                                                     - 17.98% __netif_receive_skb_one_core
                                                                        - 10.69% ip_local_deliver_finish
                                                                           - 10.58% ip_protocol_deliver_rcu
                                                                              - 10.39% tcp_v4_rcv
                                                                                 - 8.32% tcp_v4_do_rcv
                                                                                    - 8.11% tcp_rcv_established
                                                                                       - 5.32% tcp_data_queue
                                                                                          - 2.93% sock_def_readable
                                                                                             - 2.80% __wake_up_common_lock
                                                                                                - 2.67% __wake_up_common
                                                                                                   - 2.64% pollwake
                                                                                                      - 2.60% try_to_wake_up
                                                                                                         - 2.00% __task_rq_lock
                                                                                                            - raw_spin_rq_lock_nested
                                                                                                               - 1.94% _raw_spin_lock
                                                                                                                    native_queued_spin_lock_slowpath
                                                                                          - 1.80% tcp_try_rmem_schedule
                                                                                             - 1.42% __sk_mem_schedule
                                                                                                - __sk_mem_raise_allocated
                                                                                                   - mem_cgroup_charge_skmem
                                                                                                      - 0.82% try_charge_memcg
                                                                                                           0.50% refill_stock
                                                                                       - 2.20% tcp_mstamp_refresh
                                                                                          - ktime_get
                                                                                               1.88% read_hpet
                                                                                   0.55% sk_filter_trim_cap
                                                                        - 5.96% ip_rcv
                                                                           - 5.47% nf_hook_slow
                                                                              - 4.89% nft_do_chain_inet
                                                                                 - nft_do_chain
                                                                                      0.84% nft_meta_get_eval
                                                                        - 1.14% ip_local_deliver
                                                                           - 1.05% nf_hook_slow
                                                                              - 0.66% nft_do_chain_inet
                                                                                   nft_do_chain
                                                   - 1.18% dev_hard_start_xmit
                                                        1.01% loopback_xmit
                                             - 3.35% ip_local_out
                                                - 3.24% __ip_local_out
                                                   - 2.73% nf_hook_slow
                                                      - 1.41% nf_conntrack_in                                                                                                                                                                                  ◆
                                                           0.54% nf_conntrack_tcp_packet                                                                                                                                                                       ▒
                                                      - 0.87% nft_do_chain_inet                                                                                                                                                                                ▒
                                                           0.72% nft_do_chain                                                                                                                                                                                  ▒
                                             - 0.71% ip_output                                                                                                                                                                                                 ▒
                                                  0.54% nf_hook_slow                                                                                                                                                                                           ▒
                                            1.48% __skb_clone                                                                                                                                                                                                  ▒
                                       - 2.93% ktime_get                                                                                                                                                                                                       ▒
                                            2.71% read_hpet                                                                                                                                                                                                    ▒
                                         0.98% tcp_event_new_data_sent                                                                                                                                                                                         ▒
                                       - 0.88% tcp_schedule_loss_probe                                                                                                                                                                                         ▒
                                          - 0.65% sk_reset_timer                                                                                                                                                                                               ▒
                                               0.63% __mod_timer                                                                                                                                                                                               ▒
                                    - 16.47% _copy_from_iter                                                                                                                                                                                                   ▒
                                         16.22% copyin                                                                                                                                                                                                         ▒
                                    - 2.30% sk_page_frag_refill                                                                                                                                                                                                ▒
                                       - 2.17% skb_page_frag_refill                                                                                                                                                                                            ▒
                                          - 1.15% __alloc_pages                                                                                                                                                                                                ▒
                                               0.81% get_page_from_freelist                                                                                                                                                                                    ▒
                                    - 1.74% tcp_wmem_schedule                                                                                                                                                                                                  ▒
                                       - 1.60% __sk_mem_schedule                                                                                                                                                                                               ▒
                                          - __sk_mem_raise_allocated                                                                                                                                                                                           ▒
                                             - 1.21% mem_cgroup_charge_skmem                                                                                                                                                                                   ▒
                                                  0.52% mod_memcg_state                                                                                                                                                                                        ▒
                                                  0.51% try_charge_memcg                                                                                                                                                                                       ▒
                                    - 1.65% tcp_stream_alloc_skb                                                                                                                                                                                               ▒
                                         1.31% __alloc_skb                                                                                                                                                                                                     ▒
                                    - 1.59% __tcp_push_pending_frames                                                                                                                                                                                          ▒
                                       - tcp_write_xmit                                                                                                                                                                                                        ▒
                                          - 0.92% __tcp_transmit_skb                                                                                                                                                                                           ▒
                                               0.76% __ip_queue_xmit                                                                                                                                                                                           ▒
                                            0.51% ktime_get                                                                                                                                                                                                    ▒
                                      0.68% __check_object_size                                                                                                                                                                                                ▒
                                 - 29.81% release_sock                                                                                                                                                                                                         ▒
                                    - 29.68% __release_sock                                                                                                                                                                                                    ▒
                                       - 29.39% tcp_v4_do_rcv                                                                                                                                                                                                  ▒
                                          - tcp_rcv_established
                                             - 14.31% __tcp_push_pending_frames                                                                                                                                                                                ▒
                                                - tcp_write_xmit                                                                                                                                                                                               ▒
                                                   - 12.60% __tcp_transmit_skb                                                                                                                                                                                 ▒
                                                      - 8.73% __ip_queue_xmit                                                                                                                                                                                  ▒
                                                         - 6.53% ip_finish_output2                                                                                                                                                                             ▒
                                                            - 6.40% __dev_queue_xmit                                                                                                                                                                           ▒
                                                               - 5.70% __local_bh_enable_ip                                                                                                                                                                    ▒
                                                                  - do_softirq.part.0                                                                                                                                                                          ▒
                                                                     - __do_softirq                                                                                                                                                                            ▒
                                                                        - 5.51% net_rx_action                                                                                                                                                                  ▒
                                                                           - 5.11% __napi_poll                                                                                                                                                                 ▒
                                                                              - process_backlog                                                                                                                                                                ▒
                                                                                 - 4.75% __netif_receive_skb_one_core                                                                                                                                          ▒
                                                                                    - 2.87% ip_rcv                                                                                                                                                             ▒
                                                                                       - 2.66% nf_hook_slow                                                                                                                                                    ▒
                                                                                          - 2.43% nft_do_chain_inet                                                                                                                                            ▒
                                                                                               nft_do_chain                                                                                                                                                    ▒
                                                                                    - 1.19% ip_local_deliver_finish                                                                                                                                            ▒
                                                                                       - 1.13% ip_protocol_deliver_rcu                                                                                                                                         ▒
                                                                                            1.03% tcp_v4_rcv                                                                                                                                                   ▒
                                                                                      0.56% ip_local_deliver                                                                                                                                                   ▒
                                                                 0.51% dev_hard_start_xmit                                                                                                                                                                     ▒
                                                         - 1.53% ip_local_out                                                                                                                                                                                  ▒
                                                            - 1.49% __ip_local_out                                                                                                                                                                             ▒
                                                               - 1.23% nf_hook_slow                                                                                                                                                                            ▒
                                                                    0.64% nf_conntrack_in                                                                                                                                                                      ▒
                                                   - 0.63% ktime_get                                                                                                                                                                                           ▒
                                                        0.60% read_hpet                                                                                                                                                                                        ▒
                                             - 9.06% tcp_ack                                                                                                                                                                                                   ▒
                                                - 2.65% __kfree_skb                                                                                                                                                                                            ▒
                                                   - 2.57% skb_release_data                                                                                                                                                                                    ▒
                                                      - 1.53% free_unref_page                                                                                                                                                                                  ▒
                                                           1.01% free_unref_page_prepare                                                                                                                                                                       ▒
                                                  0.62% rb_next                                                                                                                                                                                                ▒
                                                  0.58% __sk_mem_reduce_allocated                                                                                                                                                                              ▒
                                             - 5.05% tcp_mstamp_refresh                                                                                                                                                                                        ▒
                                                - ktime_get                                                                                                                                                                                                    ▒
                                                     4.89% read_hpet                                                                                                                                                                                           ▒
      - 1.73% __select                                                                                                                                                                                                                                         ▒
         - entry_SYSCALL_64                                                                                                                                                                                                                                    ▒
            - do_syscall_64                                                                                                                                                                                                                                    ▒
               - 1.54% __x64_sys_pselect6                                                                                                                                                                                                                      ▒
                  - do_pselect.constprop.0                                                                                                                                                                                                                     ▒
                     - 1.18% core_sys_select                                                                                                                                                                                                                   ▒
                        - 0.70% do_select                                                                                                                                                                                                                      ▒
                           - 0.58% sock_poll                                                                                                                                                                                                                   ▒
                              - tcp_poll                                                                                                                                                                                                                       ▒
                                 - 0.54% add_wait_queue                                                                                                                                                                                                        ▒
                                      _raw_spin_lock_irqsave                                                                                                                                                                                                   ▒
      - 0.95% iperf_time_now                                                                                                                                                                                                                                   ▒
         - 0.95% clock_gettime@@GLIBC_2.17                                                                                                                                                                                                                     ▒
            - __vdso_clock_gettime                                                                                                                                                                                                                             ▒
               - 0.95% entry_SYSCALL_64                                                                                                                                                                                                                        ▒
                  - do_syscall_64                                                                                                                                                                                                                              ▒
                       0.81% syscall_exit_to_user_mode
```

## bridge

```txt
-   55.44%     0.39%  qemu-system-x86  [kernel.vmlinux]         [k] entry_SYSCALL_64                                                                                                                                                                                                       ◆
   - 55.05% entry_SYSCALL_64                                                                                                                                                                                                                                                               ▒
      - 54.79% do_syscall_64                                                                                                                                                                                                                                                               ▒
         - 32.94% __x64_sys_ioctl                                                                                                                                                                                                                                                          ▒
            - 31.65% kvm_vcpu_ioctl                                                                                                                                                                                                                                                        ▒
               - kvm_arch_vcpu_ioctl_run                                                                                                                                                                                                                                                   ▒
                  - 19.32% kvm_vcpu_halt                                                                                                                                                                                                                                                   ▒
                     - 13.11% ktime_get                                                                                                                                                                                                                                                    ▒
                          9.68% read_hpet                                                                                                                                                                                                                                                  ▒
                     - 5.10% kvm_vcpu_check_block                                                                                                                                                                                                                                          ▒
                          2.17% kvm_arch_vcpu_runnable                                                                                                                                                                                                                                     ▒
                          1.71% __srcu_read_unlock                                                                                                                                                                                                                                         ▒
                     - 0.63% kvm_vcpu_block                                                                                                                                                                                                                                                ▒
                          0.51% schedule                                                                                                                                                                                                                                                   ▒
                  - 3.96% kvm_emulate_wrmsr                                                                                                                                                                                                                                                ▒
                     - 3.69% __kvm_set_msr                                                                                                                                                                                                                                                 ▒
                        - 3.56% kvm_set_msr_common                                                                                                                                                                                                                                         ▒
                           - 2.88% restart_apic_timer                                                                                                                                                                                                                                      ▒
                              - 2.71% start_sw_timer                                                                                                                                                                                                                                       ▒
                                 - 1.43% ktime_get                                                                                                                                                                                                                                         ▒
                                      1.39% read_hpet                                                                                                                                                                                                                                      ▒
                                 - 1.02% hrtimer_start_range_ns                                                                                                                                                                                                                            ▒
                                    - 0.76% clockevents_program_event                                                                                                                                                                                                                      ▒
                                       - 0.75% ktime_get                                                                                                                                                                                                                                   ▒
                                            read_hpet                                                                                                                                                                                                                                      ▒
                           - 0.56% kvm_set_lapic_tscdeadline_msr                                                                                                                                                                                                                           ▒
                                0.50% hrtimer_cancel                                                                                                                                                                                                                                       ▒
                    2.53% svm_vcpu_run                                                                                                                                                                                                                                                     ▒
                  - 1.31% kvm_complete_insn_gp                                                                                                                                                                                                                                             ▒
                       0.72% __svm_skip_emulated_instruction                                                                                                                                                                                                                               ▒
                    0.83% kvm_check_and_inject_events                                                                                                                                                                                                                                      ▒
            - 1.04% kvm_vm_ioctl                                                                                                                                                                                                                                                           ▒
               - 0.89% kvm_send_userspace_msi                                                                                                                                                                                                                                              ▒
                  - 0.81% kvm_set_msi                                                                                                                                                                                                                                                      ▒
                     - 0.73% kvm_irq_delivery_to_apic                                                                                                                                                                                                                                      ▒
                          0.69% kvm_irq_delivery_to_apic_fast                                                                                                                                                                                                                              ▒
         - 9.44% ksys_read                                                                                                                                                                                                                                                                 ▒
            - 9.27% vfs_read                                                                                                                                                                                                                                                               ▒
               - 8.94% tun_chr_read_iter                                                                                                                                                                                                                                                   ▒
                  - 8.88% tun_do_read                                                                                                                                                                                                                                                      ▒
                     - 8.03% skb_copy_datagram_iter                                                                                                                                                                                                                                        ▒
                        - 7.99% __skb_datagram_iter                                                                                                                                                                                                                                        ▒
                           - 6.30% _copy_to_iter                                                                                                                                                                                                                                           ▒
                                6.25% copyout                                                                                                                                                                                                                                              ▒
                           - 1.62% simple_copy_to_iter                                                                                                                                                                                                                                     ▒
                                __check_object_size                                                                                                                                                                                                                                        ▒
         - 9.21% do_writev                                                                                                                                                                                                                                                                 ▒
            - 9.02% vfs_writev                                                                                                                                                                                                                                                             ▒
               - 8.82% do_iter_write                                                                                                                                                                                                                                                       ▒
                  - 8.65% do_iter_readv_writev                                                                                                                                                                                                                                             ▒
                     - 8.61% tun_chr_write_iter                                                                                                                                                                                                                                            ▒
                        - 8.55% tun_get_user                                                                                                                                                                                                                                               ▒
                           - 7.73% netif_receive_skb                                                                                                                                                                                                                                       ▒
                              - 6.88% __netif_receive_skb_one_core                                                                                                                                                                                                                         ▒
                                 - __netif_receive_skb_core.constprop.0                                                                                                                                                                                                                    ▒
                                    - br_handle_frame                                                                                                                                                                                                                                      ▒
                                       - 6.77% br_handle_frame_finish                                                                                                                                                                                                                      ▒
                                          - 5.24% netif_receive_skb                                                                                                                                                                                                                        ▒
                                             - 5.21% __netif_receive_skb_one_core                                                                                                                                                                                                          ▒
                                                - 2.58% ip_local_deliver_finish                                                                                                                                                                                                            ▒
                                                   - 2.56% ip_protocol_deliver_rcu                                                                                                                                                                                                         ▒
                                                      - 2.51% tcp_v4_rcv                                                                                                                                                                                                                   ▒
                                                         - 1.97% tcp_v4_do_rcv                                                                                                                                                                                                             ▒
                                                            - tcp_rcv_established                                                                                                                                                                                                          ▒
                                                               - 1.59% tcp_ack
                                                                  - 0.71% __kfree_skb                                                                                                                                                                                                      ▒
                                                                       0.69% skb_release_data                                                                                                                                                                                              ▒
                                                - 2.12% ip_rcv                                                                                                                                                                                                                             ▒
                                                   - 1.82% nf_hook_slow                                                                                                                                                                                                                    ▒
                                                      - 1.31% nft_do_chain_inet                                                                                                                                                                                                            ▒
                                                           1.28% nft_do_chain                                                                                                                                                                                                              ▒
                                          - 1.32% br_fdb_update                                                                                                                                                                                                                            ▒
                                               fdb_find_rcu                                                                                                                                                                                                                                ▒
                              - 0.81% ktime_get_with_offset                                                                                                                                                                                                                                ▒
                                   0.79% read_hpet                                                                                                                                                                                                                                         ▒
           1.46% syscall_exit_to_user_mode                                                                                                                                                                                                                                                 ▒
         - 0.97% __x64_sys_clock_gettime                                                                                                                                                                                                                                                   ▒
            - 0.91% posix_get_monotonic_timespec                                                                                                                                                                                                                                           ▒
               - ktime_get_ts64                                                                                                                                                                                                                                                            ▒
                    0.91% read_hpet
```
## 一个主机上两个物理网卡，将一个网卡直通到虚拟机中

guest 中的结果:
```txt
   - iperf_run_client                                                                                                                                                                                                                                        ▒
      - 93.04% iperf_send                                                                                                                                                                                                                                    ▒
         - 92.76% iperf_tcp_send                                                                                                                                                                                                                             ▒
            - Nwrite                                                                                                                                                                                                                                         ▒
            - write                                                                                                                                                                                                                                          ▒
               - entry_SYSCALL_64_after_hwframe                                                                                                                                                                                                              ▒
               - do_syscall_64                                                                                                                                                                                                                               ▒
                  - 88.86% ksys_write                                                                                                                                                                                                                        ▒
                     - 88.58% vfs_write                                                                                                                                                                                                                      ▒
                        - 88.30% sock_write_iter                                                                                                                                                                                                             ▒
                           - tcp_sendmsg                                                                                                                                                                                                                     ▒
                              - 85.79% tcp_sendmsg_locked                                                                                                                                                                                                    ▒
                                   56.82% _copy_from_iter                                                                                                                                                                                                    ▒
                                 - 5.57% sk_page_frag_refill                                                                                                                                                                                                 ▒
                                    - skb_page_frag_refill                                                                                                                                                                                                   ▒
                                       - alloc_pages_mpol                                                                                                                                                                                                    ▒
                                       - __alloc_pages                                                                                                                                                                                                       ▒
                                          - get_page_from_freelist                                                                                                                                                                                           ▒
                                             - 1.67% __rmqueue_pcplist                                                                                                                                                                                       ▒
                                                  _raw_spin_unlock_irqrestore                                                                                                                                                                                ▒
                                               0.56% _raw_spin_trylock                                                                                                                                                                                       ▒
                                 - 5.01% tcp_wmem_schedule                                                                                                                                                                                                   ▒
                                      __sk_mem_schedule                                                                                                                                                                                                      ▒
                                    - __sk_mem_raise_allocated                                                                                                                                                                                               ▒
                                       - 4.18% mem_cgroup_charge_skmem                                                                                                                                                                                       ▒
                                          - try_charge_memcg                                                                                                                                                                                                 ▒
                                               page_counter_try_charge                                                                                                                                                                                       ▒
                                 - 1.95% tcp_stream_alloc_skb                                                                                                                                                                                                ▒
                                    - __alloc_skb                                                                                                                                                                                                            ▒
                                       - 0.84% kmalloc_reserve                                                                                                                                                                                               ▒
                                            kmem_cache_alloc_node                                                                                                                                                                                            ▒
                                         0.84% kmem_cache_alloc_node                                                                                                                                                                                         ▒
                                 - 0.84% sk_stream_wait_memory                                                                                                                                                                                               ▒
                                      0.56% _raw_spin_unlock_irqrestore                                                                                                                                                                                      ▒
                                 - 0.56% __tcp_push_pending_frames                                                                                                                                                                                           ▒
                                      tcp_write_xmit                                                                                                                                                                                                         ▒
                                 - 0.56% tcp_send_mss                                                                                                                                                                                                        ▒
                                      tcp_current_mss                                                                                                                                                                                                        ▒
                              - 0.84% release_sock                                                                                                                                                                                                           ▒
                                   0.56% _raw_spin_lock_bh                                                                                                                                                                                                   ▒
                                0.84% __local_bh_enable_ip                                                                                                                                                                                                   ▒
                    1.11% syscall_enter_from_user_mode                                                                                                                                                                                                       ▒
      - 5.29% __select                                                                                                                                                                                                                                       ▒
         - entry_SYSCALL_64_after_hwframe                                                                                                                                                                                                                    ▒
         - do_syscall_64                                                                                                                                                                                                                                     ▒
            - 3.62% __x64_sys_pselect6                                                                                                                                                                                                                       ▒
               - do_pselect.constprop.0                                                                                                                                                                                                                      ▒
                  - 3.34% core_sys_select                                                                                                                                                                                                                    ▒
                     - do_select                                                                                                                                                                                                                             ▒
                        - 1.95% schedule_hrtimeout_range_clock                                                                                                                                                                                               ▒
                           - 1.11% schedule                                                                                                                                                                                                                  ▒
                                __schedule                                                                                                                                                                                                                   ▒
                                finish_task_switch.isra.0                                                                                                                                                                                                    ▒
                          0.56% sock_poll
```

host 中的结果:

## tap

```txt
      - 49.13% virtio_net_flush_tx                                                                                                                                     ▒
         - 47.49% qemu_net_queue_send_iov                                                                                                                              ▒
            - 47.43% qemu_net_queue_deliver_iov (inlined)                                                                                                              ◆
               - qemu_deliver_packet_iov                                                                                                                               ▒
                  - qemu_deliver_packet_iov                                                                                                                            ▒
                     - nc_sendv_compat (inlined)                                                                                                                       ▒
                        - 47.37% net_slirp_receive                                                                                                                     ▒
                           - 46.11% tcp_input                                                                                                                          ▒
                              - 36.62% sbappend                                                                                                                        ▒
                                 - 36.57% __libc_send (inlined)                                                                                                        ▒
                                    - 36.30% entry_SYSCALL_64_after_hwframe                                                                                            ▒
                                       - do_syscall_64                                                                                                                 ▒
                                          - 36.25% __x64_sys_sendto                                                                                                    ▒
                                             - __sys_sendto                                                                                                            ▒
                                                - 35.83% tcp_sendmsg                                                                                                   ▒
                                                   - 29.28% tcp_sendmsg_locked                                                                                         ▒
                                                      - 27.81% __tcp_push_pending_frames                                                                               ▒
                                                         - tcp_write_xmit                                                                                              ▒
                                                            - 22.83% __tcp_transmit_skb                                                                                ▒
                                                               - 19.66% __ip_queue_xmit                                                                                ▒
                                                                  - 17.60% ip_finish_output2                                                                           ▒
                                                                     - 17.58% __dev_queue_xmit                                                                         ▒
                                                                        - 17.07% __local_bh_enable_ip                                                                  ▒
                                                                           - do_softirq.part.0                                                                         ▒
                                                                              - __do_softirq                                                                           ▒
                                                                                 - net_rx_action                                                                       ▒
                                                                                    - 16.60% __napi_poll                                                               ▒
                                                                                       - process_backlog                                                               ▒
                                                                                          - 16.32% __netif_receive_skb_one_core                                        ▒
                                                                                             - 14.25% ip_local_deliver_finish                                          ▒
                                                                                                - 9.67% ip_protocol_deliver_rcu                                        ▒
                                                                                                   - 9.60% tcp_v4_rcv                                                  ▒
                                                                                                      - 7.16% tcp_v4_do_rcv                                            ▒
                                                                                                         - 7.08% tcp_rcv_established                                   ▒
                                                                                                            - 5.31% tcp_mstamp_refresh                                 ▒
                                                                                                               - ktime_get                                             ▒
                                                                                                                    read_hpet                                          ▒
                                                                                                              1.20% tcp_data_queue                                     ▒
                                                                                                        1.26% __inet_lookup_established                                ▒
                                                                                                        0.64% sk_filter_trim_cap                                       ▒
                                                                                                - 4.47% ktime_get_with_offset                                          ▒
                                                                                                     read_hpet                                                         ▒
                                                                                             - 1.35% ip_local_deliver                                                  ▒
                                                                                                - 1.28% nf_hook_slow                                                   ▒
                                                                                                   - 1.25% nft_do_chain_ipv4                                           ▒
                                                                                                        nft_do_chain                                                   ▒
                                                                                             - 0.71% ip_rcv                                                            ▒
                                                                                                  0.50% nf_hook_slow                                                   ▒
                                                                  - 0.98% ip_local_out                                                                                 ▒
                                                                     - __ip_local_out                                                                                  ▒
                                                                        - 0.75% nf_hook_slow                                                                           ▒
                                                                             0.71% nf_conntrack_in                                                                     ▒
                                                                  - 0.60% ip_finish_output                                                                             ▒
                                                                     - __cgroup_bpf_run_filter_skb                                                                     ▒
                                                                          0.51% __bpf_prog_run_save_cb                                                                 ▒
                                                               - 2.81% __skb_clone                                                                                     ▒
                                                                    __copy_skb_header                                                                                  ▒
                                                            - 3.98% ktime_get
                                                                 3.88% read_hpet                                                                                       ▒
                                                   - 6.30% release_sock                                                                                                ▒
                                                      - 6.19% __release_sock                                                                                           ▒
                                                         - 6.10% tcp_v4_do_rcv                                                                                         ▒
                                                            - tcp_rcv_established                                                                                      ◆
                                                               - 4.02% tcp_mstamp_refresh                                                                              ▒
                                                                  - ktime_get                                                                                          ▒
                                                                       4.01% read_hpet                                                                                 ▒
                                                                 1.58% tcp_ack                                                                                         ▒
                              - 8.58% tcp_output                                                                                                                       ▒
                                 - 7.84% ip_output                                                                                                                     ▒
                                    - 7.67% if_start                                                                                                                   ▒
                                       - 7.59% cpu_get_clock                                                                                                           ▒
                                          - cpu_get_clock_locked                                                                                                       ▒
                                             - get_clock (inlined)                                                                                                     ▒
                                               get_clock (inlined)                                                                                                     ▒
                                             - __clock_gettime_2 (inlined)                                                                                             ▒
                                                - __vdso_clock_gettime                                                                                                 ▒
                                                   - 7.33% entry_SYSCALL_64_after_hwframe                                                                              ▒
                                                      - do_syscall_64                                                                                                  ▒
                                                         - 7.32% __x64_sys_clock_gettime                                                                               ▒
                                                            - 5.43% posix_get_monotonic_timespec                                                                       ▒
                                                               - 5.37% ktime_get_ts64                                                                                  ▒
                                                                    read_hpet                                                                                          ▒
                                                            - 1.89% put_timespec64                                                                                     ▒
                                                                 _copy_to_user                                                                                         ▒
                             0.82% slirp_input                                                                                                                         ▒
         - 0.75% virtio_notify                                                                                                                                         ▒
            - 0.65% glib_autoptr_cleanup_RCUReadAuto (inlined)                                                                                                         ▒
                 glib_autoptr_clear_RCUReadAuto (inlined)                                                                                                              ▒
                 rcu_read_auto_unlock (inlined)                                                                                                                        ▒
                 rcu_read_unlock (inlined)                                                                                                                             ▒
           0.61% virtqueue_push                                                                                                                                        ▒
      - 7.70% virtio_net_receive_rcu                                                                                                                                   ▒
         - 5.62% address_space_stl_le                                                                                                                                  ▒
            - address_space_stl_internal (inlined)                                                                                                                     ▒
               - 4.49% memory_region_dispatch_write                                                                                                                    ▒
                  - 4.46% access_with_adjusted_size                                                                                                                    ▒
                     - memory_region_write_accessor                                                                                                                    ▒
                        - 4.43% kvm_apic_mem_write                                                                                                                     ▒
                           - kvm_send_msi                                                                                                                              ▒
                              - 4.11% kvm_irqchip_send_msi                                                                                                             ▒
                                 - 4.08% kvm_vm_ioctl                                                                                                                  ▒
                                    - 4.06% __GI___ioctl                                                                                                               ▒
                                       - entry_SYSCALL_64_after_hwframe                                                                                                ▒
                                       - do_syscall_64                                                                                                                 ▒
                                          - 3.97% __x64_sys_ioctl                                                                                                      ▒
                                             - 3.83% kvm_vm_ioctl                                                                                                      ▒
                                                - 3.60% kvm_send_userspace_msi                                                                                         ▒
                                                   - kvm_set_msi                                                                                                       ▒
                                                      - 3.54% kvm_irq_delivery_to_apic                                                                                 ▒
                                                         - kvm_irq_delivery_to_apic_fast                                                                               ▒
                                                            - 2.96% __apic_accept_irq                                                                                  ▒
                                                               - 1.12% kvm_vcpu_kick                                                                                   ▒
                                                                    0.57% kvm_arch_vcpu_should_kick                                                                    ▒
                                                                 0.83% svm_deliver_interrupt                                                                           ▒
               - 0.74% address_space_translate (inlined)                                                                                                               ▒
                  - flatview_translate (inlined)                                                                                                                       ▒
                     - flatview_do_translate (inlined)
                          0.57% address_space_translate_internal                                                                                                 ◆
         - 1.24% virtqueue_fill                                                                                                                                  ▒
            - 0.89% virtqueue_split_fill (inlined)                                                                                                               ▒
               - vring_used_write (inlined)                                                                                                                      ▒
                    0.63% invalidate_and_set_dirty                                                                                                               ▒
      - 1.32% virtqueue_split_pop                                                                                                                                ▒
         - 0.74% virtqueue_map_desc                                                                                                                              ▒
            - dma_memory_map (inlined)                                                                                                                           ▒
                 address_space_map                                                                                                                               ▒
      - 0.82% if_encap                                                                                                                                           ▒
         - 0.76% slirp_send_packet_all                                                                                                                           ▒
            - net_slirp_send_packet                                                                                                                              ▒
                 0.58% qemu_net_queue_send
```

## ovs host iperf 到 guest 中

### host 中的 iperf
```txt
     94.22% entry_SYSCALL_64_after_hwframe                                                                                                                          ▒
      - do_syscall_64                                                                                                                                               ▒
         - 93.21% ksys_write                                                                                                                                        ▒
            - 93.05% vfs_write                                                                                                                                      ▒
               - 92.53% sock_write_iter                                                                                                                             ▒
                  - 92.30% tcp_sendmsg                                                                                                                              ▒
                     - 76.14% tcp_sendmsg_locked                                                                                                                    ▒
                        - 29.71% tcp_write_xmit                                                                                                                     ▒
                           - 18.39% __tcp_transmit_skb                                                                                                              ▒
                              - 9.15% __ip_queue_xmit                                                                                                               ▒
                                 - 4.78% ip_finish_output2                                                                                                          ▒
                                    - 4.69% __dev_queue_xmit                                                                                                        ▒
                                       - 4.48% dev_hard_start_xmit                                                                                                  ▒
                                          - 4.34% internal_dev_xmit                                                                                                 ▒
                                             - 4.26% ovs_vport_receive                                                                                              ▒
                                                - 3.02% ovs_dp_process_packet                                                                                       ▒
                                                   - 2.11% ovs_execute_actions                                                                                      ▒
                                                      - do_execute_actions                                                                                          ▒
                                                         - 1.69% __dev_queue_xmit                                                                                   ▒
                                                            - 1.19% sch_direct_xmit                                                                                 ▒
                                                               - 0.58% dev_hard_start_xmit                                                                          ▒
                                                                    tun_net_xmit                                                                                    ▒
                                                   - 0.75% ovs_flow_tbl_lookup_stats                                                                                ▒
                                                      - flow_lookup.constprop.0                                                                                     ▒
                                                           masked_flow_lookup                                                                                       ▒
                                                  0.85% ovs_flow_key_extract                                                                                        ▒
                                 - 2.33% ip_local_out                                                                                                               ▒
                                    - __ip_local_out                                                                                                                ▒
                                       - 1.69% nf_hook_slow                                                                                                         ▒
                                          - 1.64% nf_conntrack_in                                                                                                   ▒
                                               0.80% nf_conntrack_tcp_packet                                                                                        ▒
                                         0.51% ip_send_check                                                                                                        ▒
                                 - 0.99% ip_finish_output                                                                                                           ▒
                                    - __cgroup_bpf_run_filter_skb                                                                                                   ▒
                                         0.80% __bpf_prog_run_save_cb                                                                                               ▒
                                 - 0.96% ip_output                                                                                                                  ▒
                                    - 0.87% nf_hook_slow                                                                                                            ▒
                                       - 0.66% selinux_ip_postroute                                                                                                 ▒
                                            0.51% selinux_ip_postroute_compat                                                                                       ▒
                              - 8.34% __skb_clone                                                                                                                   ▒
                                   __copy_skb_header                                                                                                                ▒
                           - 8.68% ktime_get                                                                                                                        ▒
                                read_hpet                                                                                                                           ▒
                           - 1.84% tcp_event_new_data_sent                                                                                                          ▒
                              - 0.79% sk_reset_timer                                                                                                                ▒
                                   0.78% __mod_timer                                                                                                                ▒
                        - 29.32% _copy_from_iter                                                                                                                    ▒
                             copyin                                                                                                                                 ▒
                        - 9.42% __tcp_push_pending_frames                                                                                                           ▒
                           - tcp_write_xmit                                                                                                                         ▒
                              - 3.56% ktime_get                                                                                                                     ▒
                                   read_hpet                                                                                                                        ▒
                              - 2.75% __tcp_transmit_skb                                                                                                            ▒
                                 - 2.33% __ip_queue_xmit                                                                                                            ▒
                                    - 1.16% ip_finish_output2                                                                                                       ▒
                                       - __dev_queue_xmit                                                                                                           ▒
                                          - 1.05% dev_hard_start_xmit                                                                                               ▒
                                             - internal_dev_xmit
                                                - ovs_vport_receive                                                                                                 ▒
                                                     0.69% ovs_dp_process_packet                                                                                    ▒
                                    - 0.61% ip_local_out                                                                                                            ▒
                                         __ip_local_out                                                                                                             ▒
                              - 2.13% tcp_stream_alloc_skb                                                                                                          ▒
                                 - 2.11% __alloc_skb                                                                                                                ▒
                                    - 1.86% kmalloc_reserve                                                                                                         ▒
                                         kmem_cache_alloc_node                                                                                                      ▒
                        - 3.26% sk_page_frag_refill                                                                                                                 ▒
                           - 3.21% skb_page_frag_refill                                                                                                             ▒
                              - 1.96% __alloc_pages                                                                                                                 ▒
                                   1.70% get_page_from_freelist                                                                                                     ▒
                        - 1.08% tcp_wmem_schedule                                                                                                                   ▒
                           - __sk_mem_schedule                                                                                                                      ▒
                              - __sk_mem_raise_allocated                                                                                                            ▒
                                 - 0.72% mem_cgroup_charge_skmem                                                                                                    ▒
                                      0.55% try_charge_memcg                                                                                                        ▒
                        - 0.90% tcp_stream_alloc_skb                                                                                                                ▒
                             0.81% __alloc_skb                                                                                                                      ▒
                          0.52% __check_object_size                                                                                                                 ▒
                     - 15.78% release_sock                                                                                                                          ▒
                        - 14.04% __release_sock                                                                                                                     ▒
                           - 13.45% tcp_v4_do_rcv                                                                                                                   ▒
                              - 13.39% tcp_rcv_established                                                                                                          ▒
                                 - 5.04% tcp_ack                                                                                                                    ▒
                                    - 1.62% __kfree_skb                                                                                                             ▒
                                       - 1.59% skb_release_data                                                                                                     ▒
                                            0.78% free_unref_page                                                                                                   ▒
                                 - 3.57% __tcp_push_pending_frames                                                                                                  ▒
                                    - tcp_write_xmit                                                                                                                ▒
                                       - 3.40% ktime_get                                                                                                            ▒
                                            read_hpet                                                                                                               ▒
                                 - 3.45% tcp_mstamp_refresh                                                                                                         ▒
                                    - ktime_get                                                                                                                     ▒
                                         read_hpet                                                                                                                  ▒
                                   0.64% tcp_check_space                                                                                                            ▒
                          1.56% _raw_spin_lock_bh                                                                                                                   ▒
           0.84% __x64_sys_clock_gettime
```


### qemu

```txt
         - 13.08% do_writev                                                                                                                                         ▒
            - 13.05% vfs_writev                                                                                                                                     ▒
               - 12.90% do_iter_write                                                                                                                               ▒
                  - 12.86% do_iter_readv_writev                                                                                                                     ▒
                     - 12.84% tun_chr_write_iter                                                                                                                    ▒
                        - 12.81% tun_get_user                                                                                                                       ▒
                           - 7.40% netif_receive_skb                                                                                                                ▒
                              - 5.20% __netif_receive_skb_one_core                                                                                                  ▒
                                 - __netif_receive_skb_core.constprop.0                                                                                             ▒
                                    - netdev_frame_hook                                                                                                             ▒
                                       - 5.14% ovs_vport_receive                                                                                                    ▒
                                          - 3.98% ovs_dp_process_packet                                                                                             ▒
                                             - 3.63% ovs_execute_actions                                                                                            ▒
                                                - do_execute_actions                                                                                                ▒
                                                   - 3.52% internal_dev_recv                                                                                        ▒
                                                      - 3.49% netif_rx                                                                                              ▒
                                                         - netif_rx_internal                                                                                        ▒
                                                            - 2.42% ktime_get_with_offset                                                                           ▒
                                                                 read_hpet                                                                                          ▒
                                                            - 1.06% enqueue_to_backlog                                                                              ▒
                                                                 1.01% _raw_spin_lock_irqsave                                                                       ▒
                                            0.76% ovs_flow_key_extract                                                                                              ▒
                              - 2.18% ktime_get_with_offset                                                                                                         ▒
                                   read_hpet                                                                                                                        ▒
                           - 4.83% __local_bh_enable_ip                                                                                                             ▒
                              - do_softirq.part.0                                                                                                                   ▒
                                 - __do_softirq                                                                                                                     ▒
                                    - 4.77% net_rx_action                                                                                                           ▒
                                       - 4.43% __napi_poll                                                                                                          ▒
                                          - process_backlog                                                                                                         ▒
                                             - 4.34% __netif_receive_skb_one_core                                                                                   ▒
                                                - 2.39% ip_local_deliver_finish                                                                                     ▒
                                                   - 2.39% ip_protocol_deliver_rcu                                                                                  ▒
                                                      - 2.34% tcp_v4_rcv                                                                                            ▒
                                                         - 1.78% tcp_v4_do_rcv                                                                                      ▒
                                                            - tcp_rcv_established                                                                                   ▒
                                                                 1.19% tcp_ack                                                                                      ▒
                                                - 1.32% ip_rcv                                                                                                      ▒
                                                   - 1.04% nf_hook_slow                                                                                             ▒
                                                      - 0.68% nft_do_chain_ipv4                                                                                     ▒
                                                           nft_do_chain                                                                                             ▒
                                                - 0.62% ip_local_deliver                                                                                            ▒
                                                   - 0.61% nf_hook_slow                                                                                             ▒
                                                      - 0.58% nft_do_chain_ipv4                                                                                     ▒
                                                           0.57% nft_do_chain
         - 8.01% ksys_read                                                                                                                                          ▒
            - 7.92% vfs_read                                                                                                                                        ▒
               - 7.63% tun_chr_read_iter                                                                                                                            ▒
                  - 7.56% tun_do_read                                                                                                                               ▒
                     - 6.90% skb_copy_datagram_iter                                                                                                                 ▒
                        - __skb_datagram_iter                                                                                                                       ▒
                           - 6.53% _copy_to_iter                                                                                                                    ▒
                                copyout                                                                                                                             ▒
         - 2.38% __x64_sys_clock_gettime                                                                                                                            ▒
            - 1.28% posix_get_monotonic_timespec                                                                                                                    ▒
               - 1.27% ktime_get_ts64                                                                                                                               ▒
                    read_hpet                                                                                                                                       ▒
            - 1.10% put_timespec64                                                                                                                                  ▒
                 _copy_to_user                                                                                                                                      ▒
         - 1.11% __x64_sys_ppoll                                                                                                                                    ▒
              0.90% do_sys_poll
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
