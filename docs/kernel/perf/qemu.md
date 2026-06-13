## 在 QEMU 中测试 nvme : 观察 nvme 的写入和中断到了哪里

nvme 的参数为:
arg_nvme="-device nvme,drive=nvme1,serial=foo,bus=mybridge,addr=0x1,id=nvme1 -drive
file=${workstation}/img1,format=qcow2,if=none,id=nvme1"

```txt
-   53.20%     0.04%  .qemu-system-x8  .qemu-system-x86_64-wrapped  [.] qemu_main_loop                                                                                                                                                                       ▒
   - 53.16% qemu_main_loop                                                                                                                                                                                                                                   ▒
      - 53.05% main_loop_wait                                                                                                                                                                                                                                ▒
         + 23.38% qemu_poll_ns                                                                                                                                                                                                                               ▒
         - 17.47% g_main_context_dispatch                                                                                                                                                                                                                    ▒
            - 17.28% aio_ctx_dispatch                                                                                                                                                                                                                        ▒
               - 16.67% aio_dispatch                                                                                                                                                                                                                         ▒
                  - 15.23% aio_bh_poll                                                                                                                                                                                                                       ▒
                     - 10.69% aio_bh_call                                                                                                                                                                                                                    ▒
                        - 7.36% nvme_process_sq                                                                                                                                                                                                              ▒
                           - 4.64% nvme_io_cmd                                                                                                                                                                                                               ▒
                              - 3.66% dma_blk_read                                                                                                                                                                                                           ▒
                                 - 3.63% dma_blk_io                                                                                                                                                                                                          ▒
                                    - 2.81% dma_blk_cb                                                                                                                                                                                                       ▒
                                       - 2.04% blk_aio_preadv                                                                                                                                                                                                ▒
                                          - blk_aio_prwv                                                                                                                                                                                                     ▒
                                               1.05% aio_bh_schedule_oneshot_full                                                                                                                                                                            ▒
                                               0.53% aio_co_enter                                                                                                                                                                                            ▒
                             1.20% address_space_rw                                                                                                                                                                                                          ▒
                           - 0.90% nvme_update_sq_tail                                                                                                                                                                                                       ▒
                              - 0.76% address_space_rw                                                                                                                                                                                                       ▒
                                   0.52% flatview_read                                                                                                                                                                                                       ▒
                        - 1.68% nvme_post_cqes                                                                                                                                                                                                               ▒
                             0.80% address_space_rw                                                                                                                                                                                                          ▒
                        - 1.34% address_space_stl_le                                                                                                                                                                                                         ▒
                           - 1.16% memory_region_dispatch_write                                                                                                                                                                                              ▒
                              - 1.14% access_with_adjusted_size                                                                                                                                                                                              ▒
                                 - memory_region_write_accessor                                                                                                                                                                                              ▒
                                    - kvm_apic_mem_write                                                                                                                                                                                                     ▒
                                       - kvm_send_msi                                                                                                                                                                                                        ▒
                                          - 1.05% kvm_irqchip_send_msi                                                                                                                                                                                       ▒
                                             - kvm_vm_ioctl                                                                                                                                                                                                  ▒
                                                - 1.02% __GI___ioctl                                                                                                                                                                                         ▒
                                                   - 0.87% entry_SYSCALL_64_after_hwframe                                                                                                                                                                    ▒
                                                      - do_syscall_64                                                                                                                                                                                        ▒
                                                         - 0.81% __x64_sys_ioctl                                                                                                                                                                             ▒
                                                            - 0.64% kvm_vm_ioctl                                                                                                                                                                             ▒
                                                               - 0.55% kvm_send_userspace_msi                                                                                                                                                                ▒
                                                                  - 0.53% kvm_set_msi                                                                                                                                                                        ▒
                                                                       0.51% kvm_irq_delivery_to_apic                                                                                                                                                        ▒
                     - 3.96% blk_aio_complete_bh                                                                                                                                                                                                             ▒
                        - 3.72% dma_blk_cb                                                                                                                                                                                                                   ▒
                           - 2.38% qemu_bh_schedule                                                                                                                                                                                                          ▒
                              - 2.04% event_notifier_set                                                                                                                                                                                                     ▒
                                 - 2.04% __GI___write (inlined)                                                                                                                                                                                              ▒
                                    - 1.20% entry_SYSCALL_64_after_hwframe                                                                                                                                                                                   ▒
                                       - do_syscall_64                                                                                                                                                                                                       ▒
                                          - 0.98% ksys_write                                                                                                                                                                                                 ▒
                                               0.62% vfs_write                                                                                                                                                                                               ▒
                  - 1.12% aio_dispatch_handler                                                                                                                                                                                                               ▒
                     - 0.93% event_notifier_test_and_clear                                                                                                                                                                                                   ▒
                        - 0.91% __libc_read (inlined)                                                                                                                                                                                                        ▒
                           - 0.67% entry_SYSCALL_64_after_hwframe                                                                                                                                                                                            ▒
                              - do_syscall_64                                                                                                                                                                                                                ▒
                                   0.59% ksys_read                                                                                                                                                                                                           ▒
               - 0.60% timerlistgroup_run_timers                                                                                                                                                                                                             ▒
                    timerlist_run_timers.part.0                                                                                                                                                                                                              ▒
         - 3.24% qemu_mutex_unlock_impl                                                                                                                                                                                                                      ▒
            - 3.22% __GI___pthread_mutex_unlock_usercnt                                                                                                                                                                                                      ▒
               - __GI___lll_lock_wake
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
