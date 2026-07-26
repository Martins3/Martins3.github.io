# vIOMMU

## vfio + viommu 的场景
思考一个有趣的场景，提供 vIOMMU ，并且内核为 iommu=notpt ，
然后直通一个设备给 QEMU ，大致的流程是怎么样子的?

vfio 速度直接从 600K 下降到 100K ，似乎主要是因为这个路径，
```txt
- clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - address_space_rw
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - memory_region_dispatch_write
                    - access_with_adjusted_size
                      - memory_region_write_accessor
                        - vtd_fetch_inv_desc
                          - vtd_process_inv_desc
                            - vtd_process_inv_iec_desc
                              - vtd_iec_notify_all
                                - x86_iommu_iec_notify_all
                                  - kvm_update_msi_routes_all
                                    - kvm_irqchip_update_msi_route
                                      - kvm_arch_fixup_msi_route
                                        - vtd_int_remap
```
并不会走这个路径: vtd_iommu_translate
在 guest kernel 的 cmdline 中设置 iommu=pt 之后，性能又恢复了。

```txt
-   94.14%     0.29%  .qemu-system-x8  .qemu-system-x86_64-wrapped  [.] kvm_cpu_exec                                                                                             ◆
   - 93.84% kvm_cpu_exec                                                                                                                                                         ▒
      - 48.16% kvm_vcpu_ioctl                                                                                                                                                    ▒
         - 47.49% __GI___ioctl                                                                                                                                                   ▒
            - 42.33% entry_SYSCALL_64_after_hwframe                                                                                                                              ▒
               - do_syscall_64                                                                                                                                                   ▒
                  - 41.04% __x64_sys_ioctl                                                                                                                                       ▒
                     - 40.11% kvm_vcpu_ioctl                                                                                                                                     ▒
                        - 39.01% kvm_arch_vcpu_ioctl_run                                                                                                                         ▒
                           - 13.04% vmx_handle_exit                                                                                                                              ▒
                              - 11.40% x86_emulate_instruction                                                                                                                   ▒
                                 - 7.83% x86_decode_emulated_instruction                                                                                                         ▒
                                    - 6.83% x86_decode_insn                                                                                                                      ▒
                                       - 5.70% __do_insn_fetch_bytes                                                                                                             ▒
                                          - 4.81% kvm_fetch_guest_virt                                                                                                           ▒
                                             - 2.87% paging64_gva_to_gpa                                                                                                         ▒
                                                - 2.39% paging64_walk_addr_generic                                                                                               ▒
                                                     0.80% __get_user_nocheck_8                                                                                                  ▒
                                                     0.50% kvm_vcpu_gfn_to_hva_prot                                                                                              ▒
                                             - 1.37% __kvm_read_guest_page                                                                                                       ▒
                                                  0.64% __check_object_size                                                                                                      ▒
                                         0.51% decode_operand                                                                                                                    ▒
                                      0.84% init_emulate_ctxt                                                                                                                    ▒
                                 - 2.84% x86_emulate_insn                                                                                                                        ▒
                                    - 1.20% emulator_read_write                                                                                                                  ▒
                                       - 1.05% emulator_read_write_onepage                                                                                                       ▒
                                            0.72% write_mmio                                                                                                                     ▒
                              - 0.76% handle_ept_misconfig                                                                                                                       ▒
                                 - kvm_io_bus_write                                                                                                                              ▒
                                      0.53% __kvm_io_bus_write                                                                                                                   ▒
                           - 9.82% vmx_vcpu_run                                                                                                                                  ▒
                              - 3.28% kvm_load_host_xsave_state                                                                                                                  ▒
                                   native_write_msr                                                                                                                              ▒
                              - 3.09% kvm_load_guest_xsave_state                                                                                                                 ▒
                                   native_write_msr                                                                                                                              ▒
                                0.71% vmx_vcpu_enter_exit                                                                                                                        ▒
                           - 6.81% fpu_swap_kvm_fpstate                                                                                                                          ▒
                                3.10% save_fpregs_to_fpstate                                                                                                                     ▒
                                3.02% restore_fpregs_from_fpstate                                                                                                                ▒
                           - 1.49% vmx_prepare_switch_to_guest                                                                                                                   ▒
                                0.70% kvm_set_user_return_msr                                                                                                                    ▒
                           - 1.45% vcpu_put                                                                                                                                      ▒
                              - 1.36% kvm_arch_vcpu_put                                                                                                                          ▒
                                   1.21% vmx_vcpu_put                                                                                                                            ▒
                           - 1.14% vcpu_load                                                                                                                                     ▒
                              - 1.04% kvm_arch_vcpu_load                                                                                                                         ▒
                                   0.77% vmx_vcpu_load                                                                                                                           ▒
                             0.77% record_steal_time                                                                                                                             ▒
                             0.73% vmx_sync_pir_to_irr                                                                                                                           ▒
                           - 0.65% kvm_vcpu_halt                                                                                                                                 ▒
                                0.55% kvm_vcpu_block                                                                                                                             ▒
                  - 0.84% syscall_exit_to_user_mode                                                                                                                              ▒
                     - 0.79% exit_to_user_mode_prepare                                                                                                                           ▒
                        - 0.73% fire_user_return_notifiers                                                                                                                       ▒
                             kvm_on_user_return                                                                                                                                  ▒
              3.04% vmx_vmexit
      - 41.58% address_space_rw                                                                                                                                                  ◆
         - 41.30% flatview_write                                                                                                                                                 ▒
            - 40.27% flatview_write_continue                                                                                                                                     ▒
               - 39.27% memory_region_dispatch_write                                                                                                                             ▒
                  - 39.16% access_with_adjusted_size                                                                                                                             ▒
                     - 38.96% memory_region_write_accessor                                                                                                                       ▒
                        - 38.56% vtd_fetch_inv_desc                                                                                                                              ▒
                           - 36.22% vtd_iotlb_page_invalidate                                                                                                                    ▒
                              - 30.28% vtd_sync_shadow_page_table_range.isra.0                                                                                                   ▒
                                 - 30.23% vtd_page_walk.isra.0                                                                                                                   ▒
                                    - vtd_page_walk_level                                                                                                                        ▒
                                       - 29.81% vtd_page_walk_level                                                                                                              ▒
                                          - 29.52% vtd_page_walk_level                                                                                                           ▒
                                             - 25.17% vtd_sync_shadow_page_hook                                                                                                  ▒
                                                - memory_region_notify_iommu                                                                                                     ▒
                                                   - 24.88% memory_region_notify_iommu_one                                                                                       ▒
                                                      - 24.84% vfio_iommu_map_notify                                                                                             ▒
                                                         - 14.89% vfio_dma_unmap                                                                                                 ▒
                                                            - 14.71% __GI___ioctl                                                                                                ▒
                                                               - 14.37% entry_SYSCALL_64_after_hwframe                                                                           ▒
                                                                  - do_syscall_64                                                                                                ▒
                                                                     - 14.31% __x64_sys_ioctl                                                                                    ▒
                                                                        - 13.11% vfio_iommu_type1_ioctl                                                                          ▒
                                                                           - 12.11% vfio_remove_dma                                                                              ▒
                                                                              - 11.76% vfio_unmap_unpin                                                                          ▒
                                                                                 - 9.02% vfio_sync_unpin.isra.0                                                                  ▒
                                                                                    - 8.57% intel_iommu_tlb_sync                                                                 ▒
                                                                                       - 8.24% iommu_flush_iotlb_psi                                                             ▒
                                                                                          - 8.10% qi_flush_iotlb                                                                 ▒
                                                                                               qi_submit_sync                                                                    ▒
                                                                                 - 1.13% __iommu_unmap                                                                           ▒
                                                                                    - 1.00% intel_iommu_unmap_pages                                                              ▒
                                                                                       - 0.84% domain_unmap                                                                      ▒
                                                                                          - dma_pte_clear_level                                                                  ▒
                                                                                             - dma_pte_clear_level                                                               ▒
                                                                                                - dma_pte_clear_level                                                            ▒
                                                                                                     0.66% clflush_cache_range                                                   ▒
                                                                                 - 0.89% intel_iommu_iova_to_phys                                                                ▒
                                                                                      pfn_to_dma_pte                                                                             ▒
                                                         - 9.65% vfio_dma_map                                                                                                    ▒
                                                            - 9.57% __GI___ioctl                                                                                                 ▒
                                                               - 9.23% entry_SYSCALL_64_after_hwframe                                                                            ▒
                                                                  - do_syscall_64                                                                                                ▒
                                                                     - 9.10% __x64_sys_ioctl                                                                                     ▒
                                                                        - 7.78% vfio_iommu_type1_ioctl                                                                           ▒
                                                                           - 2.53% vfio_pin_pages_remote                                                                         ▒
                                                                              - 2.09% vaddr_get_pfns                                                                             ▒
                                                                                 - 1.88% pin_user_pages_remote                                                                   ▒
                                                                                    - 1.82% __gup_longterm_locked                                                                ▒
                                                                                       - 1.68% __get_user_pages                                                                  ▒
                                                                                          - 0.63% gup_vma_lookup                                                                 ▒
                                                                                             - find_vma                                                                          ▒
                                                                                                  mt_find                                                                        ▒
                                                                                            0.61% follow_page_pte                                                                ▒
                                                                           - 2.38% iommu_map                                                                                     ▒
                                                                              - 1.83% __iommu_map
                                                                                 - 1.67% intel_iommu_map_pages                                                                   ▒
                                                                                    - __domain_mapping                                                                           ▒
                                                                                         0.65% clflush_cache_range                                                               ▒
                                                                           - 0.79% __get_free_pages                                                                              ▒
                                                                                0.66% __alloc_pages                                                                              ▒
                                             - 1.52% vtd_get_slpte                                                                                                               ▒
                                                - 1.46% address_space_rw                                                                                                         ▒
                                                   - 1.45% flatview_read                                                                                                         ▒
                                                      - 1.36% flatview_read_continue                                                                                             ▒
                                                           1.32% __memcpy_avx_unaligned_erms (inlined)                                                                           ▒
                                             - 1.03% iova_tree_insert                                                                                                            ▒
                                                  0.55% g_malloc0                                                                                                                ▒
                                             - 0.86% iova_tree_remove                                                                                                            ▒
                                                  0.75% g_tree_remove_internal                                                                                                   ▒
                                             - 0.68% g_tree_lookup                                                                                                               ▒
                                                  0.65% g_tree_find_node                                                                                                         ▒
                              - 3.69% g_hash_table_foreach_remove_or_steal                                                                                                       ▒
                                   1.39% vtd_hash_remove_by_page                                                                                                                 ▒
                                0.73% vtd_dev_to_context_entry                                                                                                                   ▒
                           - 1.63% address_space_rw                                                                                                                              ▒
                              - 1.04% flatview_read                                                                                                                              ▒
                                   0.67% flatview_read_continue                                                                                                                  ▒
               - 0.55% qemu_mutex_lock_iothread_impl                                                                                                                             ▒
                    qemu_mutex_lock_impl                                                                                                                                         ▒
            - 0.98% flatview_do_translate.isra.0                                                                                                                                 ▒
               - address_space_translate_internal                                                                                                                                ▒
                    phys_page_find                                                                                                                                               ▒
      - 2.32% kvm_arch_post_run                                                                                                                                                  ▒
         - 1.51% cpu_set_apic_tpr                                                                                                                                                ▒
            - 1.34% object_class_dynamic_cast_assert                                                                                                                             ▒
               - object_class_dynamic_cast                                                                                                                                       ▒
                    1.09% g_hash_table_lookup                                                                                                                                    ▒
      - 1.68% kvm_arch_pre_run                                                                                                                                                   ▒
         - 1.03% cpu_get_apic_tpr                                                                                                                                                ▒
            - 0.86% object_class_dynamic_cast_assert                                                                                                                             ▒
               - object_class_dynamic_cast                                                                                                                                       ▒
                    0.80% g_hash_table_lookup
```
考虑一个场景，如果 guest 使用的 dpdk 的。
guest 使用 vfio ，告诉 QEMU 模拟的 iommu ，之后进行的所有 DMA 操作的地址是需要装换的。

出现 dma 操作，都是需要 exit 出来，并且经过 vt-d 的模拟操作的，vt-d 模拟自己的页表从而知道到底是
操作什么硬件地方，看上面的操作，每次操作之前都是首先使用 vfio ioctl 映射一段区域，使用完成之后再去 unmap 掉，
之所以 unmap 掉，是因为每次进行 dma 操作就是需要将对应的页 unmap 掉的。
```c
static size_t amd_iommu_unmap_pages(struct iommu_domain *dom, unsigned long iova,
				    size_t pgsize, size_t pgcount,
				    struct iommu_iotlb_gather *gather)
{
	struct protection_domain *domain = to_pdomain(dom);
	struct io_pgtable_ops *ops = &domain->iop.iop.ops;
	size_t r;

	if ((amd_iommu_pgtable == AMD_IOMMU_V1) &&
	    (domain->iop.mode == PAGE_MODE_NONE)) // <----------- 这个路径什么时候会走到
		return 0;

	r = (ops->unmap_pages) ? ops->unmap_pages(ops, iova, pgsize, pgcount, NULL) : 0;

	if (r)
		amd_iommu_iotlb_gather_add_page(dom, gather, iova, r);

	return r;
}
```

```txt
@[
    iommu_v1_unmap_pages+5
    amd_iommu_unmap_pages+64
    __iommu_unmap+164
    __iommu_dma_unmap+189
    iommu_dma_unmap_page+72
    page_pool_release_page+55
    page_pool_put_defragged_page+275
    mt76_dma_rx_cleanup.part.0+232
    mt7921_wpdma_reset+180
    mt7921_wpdma_reinit_cond+115
    mt7921e_mcu_drv_pmctrl+33
    mt7921_mcu_drv_pmctrl+56
    mt7921_pm_wake_work+48
    process_one_work+479
    worker_thread+81
    kthread+229
    ret_from_fork+49
    ret_from_fork_asm+27
]: 2007
```

这里模拟的 nvme 的结果对比:
```txt
         - 11.37% g_main_context_dispatch                                                                                                                                        ▒
            - 11.08% aio_ctx_dispatch                                                                                                                                            ▒
               - 10.34% aio_dispatch                                                                                                                                             ◆
                  - 8.69% aio_bh_poll                                                                                                                                            ▒
                     - 7.32% aio_bh_call                                                                                                                                         ▒
                        - 4.49% nvme_process_sq                                                                                                                                  ▒
                           - 2.37% nvme_io_cmd                                                                                                                                   ▒
                              - 2.12% dma_blk_read                                                                                                                               ▒
                                 - 2.09% dma_blk_io                                                                                                                              ▒
                                    - 2.00% dma_blk_cb                                                                                                                           ▒
                                       - 1.59% address_space_map                                                                                                                 ▒
                                          - 1.54% flatview_do_translate.isra.0                                                                                                   ▒
                                             - 1.53% address_space_translate_iommu                                                                                               ▒
                                                - 1.50% vtd_iommu_translate                                                                                                      ▒
                                                   - 0.63% vtd_get_slpte                                                                                                         ▒
                                                      - address_space_rw                                                                                                         ▒
                                                         - flatview_read                                                                                                         ▒
                                                              0.51% flatview_read_continue                                                                                       ▒
                           - 1.23% nvme_update_sq_tail                                                                                                                           ▒
                              - 1.09% address_space_rw                                                                                                                           ▒
                                 - flatview_read                                                                                                                                 ▒
                                    - 0.82% flatview_do_translate.isra.0                                                                                                         ▒
                                         0.71% address_space_translate_iommu                                                                                                     ▒
                             0.74% address_space_rw                                                                                                                              ▒
                        - 1.47% address_space_stl_le                                                                                                                             ▒
                           - 1.26% memory_region_dispatch_write                                                                                                                  ▒
                              - 1.25% access_with_adjusted_size                                                                                                                  ▒
                                 - 1.13% vtd_mem_ir_write                                                                                                                        ▒
                                    - 0.92% kvm_send_msi                                                                                                                         ▒
                                       - 0.90% kvm_irqchip_send_msi                                                                                                              ▒
                                          - kvm_vm_ioctl                                                                                                                         ▒
                                             - 0.88% __GI___ioctl                                                                                                                ▒
                                                - 0.76% entry_SYSCALL_64_after_hwframe                                                                                           ▒
                                                   - do_syscall_64                                                                                                               ▒
                                                      - 0.70% __x64_sys_ioctl                                                                                                    ▒
                                                           0.55% kvm_vm_ioctl                                                                                                    ▒
                        - 1.15% nvme_post_cqes                                                                                                                                   ▒
                             0.71% address_space_rw                                                                                                                              ▒
                     - 1.15% blk_aio_complete_bh                                                                                                                                 ▒
                        - 1.07% dma_blk_cb                                                                                                                                       ▒
                             0.63% qemu_bh_schedule                                                                                                                              ▒
                  - 1.29% aio_dispatch_handler                                                                                                                                   ▒
                     - 1.08% event_notifier_test_and_clear                                                                                                                       ▒
                        - 1.07% __libc_read (inlined)                                                                                                                            ▒
                           - 0.75% entry_SYSCALL_64_after_hwframe                                                                                                                ▒
                              - do_syscall_64                                                                                                                                    ▒
                                   0.65% ksys_read
```

## 简单的对比
### 看看输出的 sysfs 下的结果

#### virtio vIOMMU

ls /sys/class/iommu
```txt
total 0
drwxr-xr-x.  2 root root 0 Nov 18 13:22 .
drwxr-xr-x. 75 root root 0 Nov 18 13:22 ..
lrwxrwxrwx.  1 root root 0 Nov 18 13:22 0000:00:06.0 -> ../../devices/pci0000:00/0000:00:06.0/virtio0/iommu/0000:00:06.0
```

find /sys -name iommu 结果中的部分内容:
```txt
/sys/devices/pci0000:00/0000:00:08.0/iommu
/sys/devices/pci0000:00/0000:00:04.0/iommu
/sys/devices/pci0000:00/0000:00:07.0/iommu
/sys/devices/pci0000:00/0000:00:0c.0/iommu
/sys/devices/pci0000:00/0000:00:03.0/iommu
/sys/devices/pci0000:00/0000:00:01.1/iommu
/sys/devices/pci0000:00/0000:00:06.0/virtio0/iommu
/sys/devices/pci0000:00/0000:00:0b.0/iommu
/sys/devices/pci0000:00/0000:00:09.0/iommu
/sys/devices/pci0000:00/0000:00:05.0/iommu
/sys/devices/pci0000:00/0000:00:0a.0/iommu
```
#### intel

```txt
➜  ~  ls -la /sys/class/iommu
total 0
drwxr-xr-x.  2 root root 0 Nov 18 13:34 .
drwxr-xr-x. 75 root root 0 Nov 18 13:34 ..
lrwxrwxrwx.  1 root root 0 Nov 18 13:34 dmar0 -> ../../devices/virtual/iommu/dmar0
```

find /sys -name iommu 结果中的部分内容:
```txt
/sys/devices/pci0000:00/0000:00:1f.2/iommu
/sys/devices/pci0000:00/0000:00:08.0/iommu
/sys/devices/pci0000:00/0000:00:1f.0/iommu
/sys/devices/pci0000:00/0000:00:01.0/iommu
/sys/devices/pci0000:00/0000:00:04.0/iommu
/sys/devices/pci0000:00/0000:00:07.0/iommu
/sys/devices/pci0000:00/0000:00:1f.3/iommu
/sys/devices/pci0000:00/0000:00:00.0/iommu
/sys/devices/pci0000:00/0000:00:03.0/0000:01:01.0/iommu -> 这是那个 pci bridge 下的 nvme
/sys/devices/pci0000:00/0000:00:03.0/iommu
/sys/devices/pci0000:00/0000:00:06.0/iommu
/sys/devices/pci0000:00/0000:00:09.0/iommu
/sys/devices/pci0000:00/0000:00:02.0/iommu
/sys/devices/pci0000:00/0000:00:05.0/iommu
/sys/devices/pci0000:00/0000:00:0a.0/iommu
```

这个结果和物理机上的相同，看上去内核为了实现

### 中断
因为 virtio-iommu 没有实现 irq remapping ，cat /proc/interrupts
PCI-MSIX-0000:00:0c.0 的差别

### 设备的探测过程

使用了 virtio-iommu 后，是由 virtio-iommu 来探测 pci 设备吗?

## virtio-iommu

也是可以报告 page fault 的: virtio_iommu_report_fault

iommu=nopt 的时候，可以触发这个路径:
```txt
- main_loop_wait
  - os_host_main_loop_wait
    - glib_pollfds_poll
      - g_main_context_dispatch
        - aio_ctx_dispatch
          - aio_dispatch
            - aio_bh_poll
              - aio_bh_call
                - nvme_process_sq
                  - dma_memory_rw_relaxed
                    - address_space_rw
                      - address_space_read_full
                        - flatview_read
                          - flatview_translate
                            - flatview_do_translate
                              - address_space_translate_iommu
                                - virtio_iommu_translate
```

但是，如果设置为 iommu=pt 之后，virtio_iommu_translate 根本不会被调用


修改 QEMU 的代码，可以注入 page fault 异常到内核中:
```txt
[   32.950723] nvme nvme0: I/O 192 (Read) QID 8 timeout, aborting
[   32.951222] viommu_fault_handler: 1330 callbacks suppressed
[   32.951225] virtio_iommu virtio0: unknown fault from EP 32 at 0x103231000 []
[   32.951702] virtio_iommu virtio0: page fault from EP 32 at 0x103231000 []
[   32.951949] virtio_iommu virtio0: unknown fault from EP 32 at 0x103231000 []
[   32.952212] virtio_iommu virtio0: page fault from EP 32 at 0x103231000 []
[   32.952460] virtio_iommu virtio0: unknown fault from EP 32 at 0x1032271c0 []
[   32.952715] virtio_iommu virtio0: page fault from EP 32 at 0x1032271c0 []
[   32.952962] virtio_iommu virtio0: unknown fault from EP 32 at 0x1032271c4 []
[   32.953224] virtio_iommu virtio0: page fault from EP 32 at 0x1032271c4 []
[   32.953471] virtio_iommu virtio0: unknown fault from EP 32 at 0x1032271c8 []
[   32.953725] virtio_iommu virtio0: page fault from EP 32 at 0x1032271c8 []
[   63.154593] nvme nvme0: controller is down; will reset: CSTS=0x2, PCI_STATUS=0x10
[   63.179185] nvme0n1: Read(0x2) @ LBA 209715072, 8 blocks, Host Aborted Command (sct 0x3 / sc 0x71)
[   63.179544] I/O error, dev nvme0n1, sector 209715072 op 0x0:(READ) flags 0x80700 phys_seg 1 prio class 2
[   63.179895] nvme nvme0: Abort status: 0x371
[   63.203455] viommu_fault_handler: 26 callbacks suppressed
[   63.203457] virtio_iommu virtio0: unknown fault from EP 32 at 0x103227000 []
[   63.203935] virtio_iommu virtio0: page fault from EP 32 at 0x103227000 []
[   63.204189] virtio_iommu virtio0: unknown fault from EP 32 at 0x103227004 []
[   63.204443] virtio_iommu virtio0: page fault from EP 32 at 0x103227004 []
[   63.204690] virtio_iommu virtio0: unknown fault from EP 32 at 0x103227008 []
[   63.204944] virtio_iommu virtio0: page fault from EP 32 at 0x103227008 []
[   63.205191] virtio_iommu virtio0: unknown fault from EP 32 at 0x10322700c []
[   63.205448] virtio_iommu virtio0: page fault from EP 32 at 0x10322700c []
[   63.205692] virtio_iommu virtio0: unknown fault from EP 32 at 0x103227010 []
[   63.205947] virtio_iommu virtio0: page fault from EP 32 at 0x103227010 []
```

在 kernel 中， viommu_attach_dev 会导致 QEMU 中设置到 virtio_iommu_switch_address_space, 进而影响设备的属性。

## intel viommu

除了 translate 的过程中会发生

- vtd_mem_ir_fault_write
- vtd_mem_ir_fault_read
  - vtd_report_ir_illegal_access

但是
```c
        /*
         * This region is used for catching fault to access interrupt
         * range via passthrough + PASID. See also
         * vtd_switch_address_space(). We can't use alias since we
         * need to know the sid which is valid for MSI who uses
         * bus_master_as (see msi_send_message()).
         */
        memory_region_init_io(&vtd_dev_as->iommu_ir_fault, OBJECT(s),
                              &vtd_mem_ir_fault_ops, vtd_dev_as, "vtd-no-ir",
                              VTD_INTERRUPT_ADDR_SIZE);
```

但是，这个 memory region 配置不会走，走的总是 "vtd-ir" 注册的 vtd_mem_ir_write ，
测试内核参数 intremap=off 也是没有效果，最后走的还是 vtd-ir 的。

vtd_do_iommu_translate 和 virtio_iommu_translate 一样

## amd iommu
amdvi_page_walk 直接注入 amdvi_page_fault ，但是在 host 中接受不到任何日志。

为什么 amd 即使在 iommu=pt 的时候，还是会走 page walk ，是 guest 操作系统的 bug 吗?

把 intel 下 iommu=pt 和 iommu=nopt 的结果放到
- docs/kernel/iommu-nopt-mtree.txt
- docs/kernel/iommu-pt-mtree.txt

看上去差别只是
```txt
memory-region: vtd-03.0
  0000000000000000-ffffffffffffffff (prio 0, i/o): vtd-03.0
    0000000000000000-ffffffffffffffff (prio 0, i/o): vtd-03.0-dmar
      00000000fee00000-00000000feefffff (prio 1, i/o): alias vtd-ir @vtd-ir 0000000000000000-00000000000fffff
```
和
```txt
memory-region: vtd-03.0
  0000000000000000-ffffffffffffffff (prio 0, i/o): vtd-03.0
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias vtd-nodmar @vtd-nodmar 0000000000000000-ffffffffffffffff
      00000000fee00000-00000000feefffff (prio 1, i/o): alias vtd-ir @vtd-ir 0000000000000000-00000000000fffff
```

intel 初始化的位置:
```c
        /*
         * Build the DMAR-disabled container with aliases to the
         * shared MRs.  Note that aliasing to a shared memory region
         * could help the memory API to detect same FlatViews so we
         * can have devices to share the same FlatView when DMAR is
         * disabled (either by not providing "intel_iommu=on" or with
         * "iommu=pt").  It will greatly reduce the total number of
         * FlatViews of the system hence VM runs faster.
         */
        memory_region_init_alias(&vtd_dev_as->nodmar, OBJECT(s),
                                 "vtd-nodmar", &s->mr_nodmar, 0,
                                 memory_region_size(&s->mr_nodmar));

        /*
         * Build the per-device DMAR-enabled container.
         *
         * TODO: currently we have per-device IOMMU memory region only
         * because we have per-device IOMMU notifiers for devices.  If
         * one day we can abstract the IOMMU notifiers out of the
         * memory regions then we can also share the same memory
         * region here just like what we've done above with the nodmar
         * region.
         */
        strcat(name, "-dmar");
        memory_region_init_iommu(&vtd_dev_as->iommu, sizeof(vtd_dev_as->iommu),
                                 TYPE_INTEL_IOMMU_MEMORY_REGION, OBJECT(s),
                                 name, UINT64_MAX);
        memory_region_init_alias(&vtd_dev_as->iommu_ir, OBJECT(s), "vtd-ir",
                                 &s->mr_ir, 0, memory_region_size(&s->mr_ir));
        memory_region_add_subregion_overlap(MEMORY_REGION(&vtd_dev_as->iommu),
                                            VTD_INTERRUPT_ADDR_FIRST,
                                            &vtd_dev_as->iommu_ir, 1);
```

调整的位置 : vtd_switch_address_space

vtd_switch_address_space 调用的位置:
```txt
[    0.477355] PCI: CLS 0 bytes, default 64
[    0.477543] DMAR: No RMRR found
[    0.477594] Unpacking initramfs...
[    0.477671] DMAR: No ATSR found
[    0.477940] DMAR: No SATC found
[    0.478067] DMAR: dmar0: Using Queued invalidation
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
DEBUGPRINT[1]: vtd_switch_address_space (after assert(as);)
[    0.478320] DMAR: IOMMU batching disallowed due to virtualization
[    0.478589] pci 0000:00:00.0: Adding to iommu group 0
[    0.478798] pci 0000:00:01.0: Adding to iommu group 1
[    0.479002] pci 0000:00:02.0: Adding to iommu group 2
[    0.479209] pci 0000:00:03.0: Adding to iommu group 3
[    0.479413] pci 0000:00:04.0: Adding to iommu group 4
[    0.479621] pci 0000:00:05.0: Adding to iommu group 5
[    0.479832] pci 0000:00:06.0: Adding to iommu group 6
[    0.480038] pci 0000:00:07.0: Adding to iommu group 7
[    0.480242] pci 0000:00:08.0: Adding to iommu group 8
[    0.480446] pci 0000:00:09.0: Adding to iommu group 9
[    0.480654] pci 0000:00:0a.0: Adding to iommu group 10
[    0.480869] pci 0000:00:1f.0: Adding to iommu group 11
[    0.481076] pci 0000:00:1f.2: Adding to iommu group 11
[    0.481284] pci 0000:00:1f.3: Adding to iommu group 11
[    0.481490] pci 0000:01:01.0: Adding to iommu group 3
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
DEBUGPRINT[1]:  vtd_switch_address_space  (after assert(as);)
[    0.481794] DMAR: Intel(R) Virtualization Technology for Directed I/O
[    0.482042] PCI-DMA: Using software bounce buffering for IO (SWIOTLB)
[    0.482293] software IO TLB: mapped [mem 0x000000007a39a000-0x000000007e39a000] (64MB)
```
这样最后保证是正常执行的。

而 virtio-iommu 也是存在这种类似的操作 : virtio_iommu_switch_address_space , 所以也是可以区分的
但是 hw/i386/amd_iommu.c 没有这种操作, 这导致了一个很搞笑的事情，那就是，即便是 iommu=pt ，
pci 设备，例如 nvme 做的任何操作 dma 操作，硬件上都是需要走一遍 page table 的。

观察 iommu_group_store_type 的实现，居然所谓的设置 iommu_group 的 type 是设置 defautl domain
```txt
	ret = iommu_setup_default_domain(group, req_type);
```
其实在探测设备的时候，居然内核中

## 使用
当打开了 intermap 之后，每敲个键盘，都是可以看到 amdvi_mem_ir_write 被调用的

```c
static MemTxResult amdvi_mem_ir_write(void *opaque, hwaddr addr,
                                      uint64_t value, unsigned size,
                                      MemTxAttrs attrs)
{
    int ret;
    MSIMessage from = { 0, 0 }, to = { 0, 0 };
    uint16_t sid = AMDVI_IOAPIC_SB_DEVID;

    from.address = (uint64_t) addr + AMDVI_INT_ADDR_FIRST;
    from.data = (uint32_t) value;

    trace_amdvi_mem_ir_write_req(addr, value, size);

    if (!attrs.unspecified) {
        /* We have explicit Source ID */
        sid = attrs.requester_id;
    }

    ret = amdvi_int_remap_msi(opaque, &from, &to, sid);
    if (ret < 0) {
        /* TODO: log the event using IOMMU log event interface */
        error_report_once("failed to remap interrupt from devid 0x%x", sid);
        return MEMTX_ERROR;
    }

    apic_get_class(NULL)->send_msi(&to); // 最后这个来发送中断给 geust 的

    trace_amdvi_mem_ir_write(to.address, to.data);
    return MEMTX_OK;
}
```
所以，这个是到底谁在写?


### QEMU 中间中原来存在这么多选项啊

/home/martins3/core/qemu/qemu-options.hx
```txt
``-device intel-iommu[,option=...]``
    This is only supported by ``-machine q35``, which will enable Intel VT-d
    emulation within the guest.  It supports below options:

    ``intremap=on|off`` (default: auto)
        This enables interrupt remapping feature.  It's required to enable
        complete x2apic.  Currently it only supports kvm kernel-irqchip modes
        ``off`` or ``split``, while full kernel-irqchip is not yet supported.
        The default value is "auto", which will be decided by the mode of
        kernel-irqchip.

    ``caching-mode=on|off`` (default: off)
        This enables caching mode for the VT-d emulated device.  When
        caching-mode is enabled, each guest DMA buffer mapping will generate an
        IOTLB invalidation from the guest IOMMU driver to the vIOMMU device in
        a synchronous way.  It is required for ``-device vfio-pci`` to work
        with the VT-d device, because host assigned devices requires to setup
        the DMA mapping on the host before guest DMA starts.

    ``device-iotlb=on|off`` (default: off)
        This enables device-iotlb capability for the emulated VT-d device.  So
        far virtio/vhost should be the only real user for this parameter,
        paired with ats=on configured for the device.

    ``aw-bits=39|48`` (default: 39)
        This decides the address width of IOVA address space.  The address
        space has 39 bits width for 3-level IOMMU page tables, and 48 bits for
        4-level IOMMU page tables.

    Please also refer to the wiki page for general scenarios of VT-d
    emulation in QEMU: https://wiki.qemu.org/Features/VT-d.
```

## vIOMMU 如何实现 intremap 的?
对于 msi 信息的翻译为:

amdvi_mem_ir_write 中增加调试:
```txt
[** :amdvi_mem_ir_write:1359] to.address=fee00000 to.data=2 <---- 这个 2 的含义 : 这个其实是 CPU 编号，如果想要发送给 CPU 1，那么这个 data 为 1 ，然后找到地址为 1 的 itre ，哪里重新组装为新的 message
[->:amdvi_mem_ir_write:1368] to.address=fee07000 to.data=4023 <---- 普通的 msi : 第一个是 CPU 7 ，第二个是 vector 0x23=35
```

按照 hw/intc/apic.c 来解释就可以了:
```c
static void apic_send_msi(MSIMessage *msi)
{
    uint64_t addr = msi->address;
    uint32_t data = msi->data;
    uint8_t dest = (addr & MSI_ADDR_DEST_ID_MASK) >> MSI_ADDR_DEST_ID_SHIFT;
    uint8_t vector = (data & MSI_DATA_VECTOR_MASK) >> MSI_DATA_VECTOR_SHIFT;
    uint8_t dest_mode = (addr >> MSI_ADDR_DEST_MODE_SHIFT) & 0x1;
    uint8_t trigger_mode = (data >> MSI_DATA_TRIGGER_SHIFT) & 0x1;
    uint8_t delivery = (data >> MSI_DATA_DELIVERY_MODE_SHIFT) & 0x7;
    /* XXX: Ignore redirection hint. */
    apic_deliver_irq(dest, dest_mode, delivery, vector, trigger_mode);
}
```

好吧，amdvi_mem_ir_write 到 amdvi_int_remap_msi 过程中，会根据每一个设备，找到 device table ，
进而找到该 device table 的
```c
/* get a device table entry given the devid */
static bool amdvi_get_dte(AMDVIState *s, int devid, uint64_t *entry)
{
    uint32_t offset = devid * AMDVI_DEVTAB_ENTRY_SIZE;

    if (dma_memory_read(&address_space_memory, s->devtab + offset, entry,
                        AMDVI_DEVTAB_ENTRY_SIZE, MEMTXATTRS_UNSPECIFIED)) {
        trace_amdvi_dte_get_fail(s->devtab, offset);
        /* log error accessing dte */
        amdvi_log_devtab_error(s, devid, s->devtab + offset, 0);
        return false;
    }

    *entry = le64_to_cpu(*entry);
    if (!amdvi_validate_dte(s, devid, entry)) {
        trace_amdvi_invalid_dte(entry[0]);
        return false;
    }

    return true;
}
```

## 参考一下这个
https://mysummary.readthedocs.io/zh/latest/%E8%BD%AF%E4%BB%B6%E6%9E%84%E6%9E%B6%E8%AE%BE%E8%AE%A1/%E4%B8%80%E4%B8%AA%E6%9E%B6%E6%9E%84%E8%AE%BE%E8%AE%A1%E5%AE%9E%E4%BE%8B%EF%BC%9Aqemu%20iommu.html

他这里思考了一个很重要的问题：IOMMU 引入了 address space 如何理解。

## 如果虚拟机中继续使用 iommu ，也就是 viommu ，真的有代理模式吗?
<!-- bff2e62c-dec9-4109-818f-ab4f88f1ffc8 -->

也就是，虚拟机的中的 viommu 的操作，最后是让物理机中的 iommu 完成代理

似乎也不是不可能，例如虚拟机中使用用户态的 nvme 驱动，虚拟机中想要 viommu
从 GVA 到 GPA 的映射，
最后物理机 iommu 直接完成从 GVA 到 HPA 的映射



## intel-iommu 如何设置
```plain
       -device intel-iommu[,option=...]
              This  is  only  supported by -machine q35, which will enable Intel VT-d emulation within the guest.  It sup‐
              ports below options:

              intremap=on|off (default: auto)
                     This enables interrupt remapping feature.  It's required to enable  complete  x2apic.   Currently  it
                     only  supports kvm kernel-irqchip modes off or split, while full kernel-irqchip is not yet supported.
                     The default value is "auto", which will be decided by the mode of kernel-irqchip.

              caching-mode=on|off (default: off)
                     This enables caching mode for the VT-d emulated device.  When caching-mode is enabled, each guest DMA
                     buffer  mapping  will generate an IOTLB invalidation from the guest IOMMU driver to the vIOMMU device
                     in a synchronous way.  It is required for -device vfio-pci to work with the VT-d device, because host
                     assigned devices requires to setup the DMA mapping on the host before guest DMA starts.

              device-iotlb=on|off (default: off)
                     This enables device-iotlb capability for the emulated VT-d device.  So far virtio/vhost should be the
                     only real user for this parameter, paired with ats=on configured for the device.

              aw-bits=39|48 (default: 39)
                     This decides the address width of IOVA address space.  The  address  space  has  39  bits  width  for
                     3-level IOMMU page tables, and 48 bits for 4-level IOMMU page tables.

              Please   also   refer   to   the   wiki   page   for   general   scenarios   of   VT-d  emulation  in  QEMU:
              https://wiki.qemu.org/Features/VT-d.
```

## vtd_realize 为什么从来不会被调用 ?

## qemu 的 vIOMMU 是如何模拟

我理解这个东西没有完全体现出现 posted interrupt ，只是体现了
interrupt remapping 而已。

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_bh_poll
                        - aio_bh_call
                          - virtio_net_tx_bh
                            - virtio_net_flush_tx
                              - address_space_stl_le
                                - address_space_stl_internal
                                  - memory_region_dispatch_write
                                    - access_with_adjusted_size
                                      - amdvi_mem_ir_write

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - notifier_list_notify
              - slirp_pollfds_poll
                - sorecvfrom
                  - udp_output
                    - ip_output
                      - if_start
                        - if_encap
                          - slirp_send_packet_all
                            - net_slirp_send_packet
                              - qemu_net_queue_send
                                - qemu_deliver_packet_iov
                                  - virtio_net_do_receive
                                    - virtio_net_receive_rcu
                                      - address_space_stl_le
                                        - memory_region_dispatch_write
                                          - access_with_adjusted_size
                                            - amdvi_mem_ir_write

### 地址空间到底是怎么样子的

虚拟机中 cat /proc/iomem 的结果:
```txt
fed80000-fed83fff : amd_iommu
```

qemu 中 info mtree 的结果:
```txt
memory-region: amdvi_root
  0000000000000000-ffffffffffffffff (prio 0, i/o): amdvi_root
    0000000000000000-ffffffffffffffff (prio 1, i/o): amd_iommu
    00000000fee00000-00000000feefffff (prio 64, i/o): amd_iommu_ir
```

联想机器上观察到的:
```txt
e0400000-e047ffff : amd_iommu

fee00000-fee00fff : Local APIC
  fee00000-fee00fff : pnp 00:01

```

类似的物理机机器:
```txt
fee00000-feefffff : Reserved
  fee00000-fee00fff : Local APIC
    fee00000-fee00fff : pnp 00:04
```

在 QEMU 中 hw/i386/pc.c
```c
/*
 * AMD systems with an IOMMU have an additional hole close to the
 * 1Tb, which are special GPAs that cannot be DMA mapped. Depending
 * on kernel version, VFIO may or may not let you DMA map those ranges.
 * Starting Linux v5.4 we validate it, and can't create guests on AMD machines
 * with certain memory sizes. It's also wrong to use those IOVA ranges
 * in detriment of leading to IOMMU INVALID_DEVICE_REQUEST or worse.
 * The ranges reserved for Hyper-Transport are:
 *
 * FD_0000_0000h - FF_FFFF_FFFFh
 *
 * The ranges represent the following:
 *
 * Base Address   Top Address  Use
 *
 * FD_0000_0000h FD_F7FF_FFFFh Reserved interrupt address space
 * FD_F800_0000h FD_F8FF_FFFFh Interrupt/EOI IntCtl
 * FD_F900_0000h FD_F90F_FFFFh Legacy PIC IACK
 * FD_F910_0000h FD_F91F_FFFFh System Management
 * FD_F920_0000h FD_FAFF_FFFFh Reserved Page Tables
 * FD_FB00_0000h FD_FBFF_FFFFh Address Translation
 * FD_FC00_0000h FD_FDFF_FFFFh I/O Space
 * FD_FE00_0000h FD_FFFF_FFFFh Configuration
 * FE_0000_0000h FE_1FFF_FFFFh Extended Configuration/Device Messages
 * FE_2000_0000h FF_FFFF_FFFFh Reserved
 *
 * See AMD IOMMU spec, section 2.1.2 "IOMMU Logical Topology",
 * Table 3: Special Address Controls (GPA) for more information.
 */
#define AMD_HT_START         0xfd00000000UL
#define AMD_HT_END           0xffffffffffUL
#define AMD_ABOVE_1TB_START  (AMD_HT_END + 1)
#define AMD_HT_SIZE          (AMD_ABOVE_1TB_START - AMD_HT_START)
```

在 kernel 中 drivers/iommu/amd/iommu.c
```c
/* IO virtual address start page frame number */
#define IOVA_START_PFN		(1)
#define IOVA_PFN(addr)		((addr) >> PAGE_SHIFT)

/* Reserved IOVA ranges */
#define MSI_RANGE_START		(0xfee00000)
#define MSI_RANGE_END		(0xfeefffff)
#define HT_RANGE_START		(0xfd00000000ULL)
#define HT_RANGE_END		(0xffffffffffULL)
```

搞不清楚，这里的 FD_F800_0000h

## 所以 virtio iommu 到底有没有 int remapping 的功能
之前写到 "virtio iommu 就是没有 int remapping 的，不是一样正常使用吗?"


## IOMMU
IOMMU 的学习可以参考 ASPLOS 提供的 ppt[^1], 简单来说，以前设备是可以直接访问物理内存，增加了 IOMMU 之后，设备访问物理内存类似 CPU 也是需要经过一个虚实地址映射。
所以，每一个 PCI 设备都会创建对应的 AddressSpace

默认没有配置 IOMMU 也就是直接访问物理内存，所以就是直接 alias 到 `system_memory`(就是 `address_space_memory` 关联的那个 MemoryRegion) 上。
```txt
address-space: nvme
  0000000000000000-ffffffffffffffff (prio 0, i/o): bus master container
    0000000000000000-ffffffffffffffff (prio 0, i/o): alias bus master @system 0000000000000000-ffffffffffffffff
```

函数 pci_device_iommu_address_space : 如果一个 device 被用于直通，那么其进行 IO 的 address space 就不再是 address_space_memory 的，而是需要经过一层映射。

[^1]: [ASPLOS IOMMU tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf)


## 在看看这里记录的疑惑是什么
```sh
function setup_balloon() {
	# TODO iommu_platform 到底是什么
	# https://www.reddit.com/r/qemu_kvm/comments/1nby753/how_to_properly_use_iommu_platform_with_virtio/
	# virtio-balloon 不能带 ,iommu_platform=on
	# qemu-system-x86_64: -device virtio-balloon,id=balloon0,deflate-on-oom=true,iommu_platform=on: VIRTIO_F_IOMMU_PLATFORM was supported by neither legacy nor transitional device
	# 目前看，网卡是带的，这个的确是关联到 virtio 的 flags VIRTIO_F_ACCESS_PLATFORM 功能的
	#
	# arg_mem_balloon="-device virtio-balloon,id=balloon0,deflate-on-oom=true,free-page-reporting=true"
	# arg_mem_balloon="-object iothread,id=io_balloon -device virtio-balloon,id=balloon0,deflate-on-oom=true,page-poison=false,free-page-reporting=true,free-page-hint=true,iothread=io_balloon "
	#
	#
	# 参考 hw/virtio/virtio-balloon.c:virtio_balloon_properties 一共定义四个开关和 iothread
	arg_mem_balloon="-device virtio-balloon,id=balloon0,deflate-on-oom=true"
	# arg_mem_balloon="-device virtio-balloon-pci,id=balloon0"
	:
}
```

## -device intel-iommu

看上去

```txt
- ??
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
                              - iommu_group_get_for_dev
                                - iommu_group_add_device
```

这里初始化的时候，会让 `get_dma_ops` 修改为返回不会空，而是当时配置的 iommu

```c
	const struct dma_map_ops *ops = get_dma_ops(dev);
```

仔细分析下，iommu 如何初始化 pci 设备的

```txt
[    2.554817] pci 0000:00:01.0: Adding to iommu group 1
[    2.555020] pci 0000:00:02.0: Adding to iommu group 2
[    2.555223] pci 0000:00:03.0: Adding to iommu group 3
[    2.555421] pci 0000:00:04.0: Adding to iommu group 4
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
