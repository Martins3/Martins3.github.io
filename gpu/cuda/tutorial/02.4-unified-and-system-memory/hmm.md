## Linux kernel 是如何支持 hmm 的

demoPageableMemoryDirectAccess 的测试结果:

如果是
1. malloc
2. memset
3. copyKernel
4. memset

```txt
            - 32.13% asm_exc_page_fault
               - exc_page_fault
               - do_user_addr_fault
                  - 32.07% handle_mm_fault
                     - __handle_mm_fault
                        - 32.06% do_swap_page
                           - 31.98% devmem_fault_entry.part.0
                              - 31.98% uvm_va_space_cpu_fault
                                 - 31.92% uvm_va_block_cpu_fault
                                    - 31.92% block_cpu_fault_locked
                                       - 31.90% uvm_va_block_service_locked
                                          - 31.63% uvm_hmm_va_block_service_locked
                                             - hmm_block_cpu_fault_locked
                                                - 21.01% uvm_hmm_devmem_fault_alloc_and_copy
                                                   - 13.44% alloc_page_on_cpu
                                                      - 12.30% block_populate_pages_cpu
                                                         - 5.13% uvm_va_block_map_cpu_chunk_on_gpus
                                                            - uvm_cpu_chunk_map_gpu
                                                               - 2.74% cpu_chunk_map_gpu_phys
                                                                    1.67% mutex_lock
                                                         - 3.39% uvm_cpu_chunk_alloc
                                                            - 2.55% alloc_pages_noprof
                                                               - alloc_pages_mpol
                                                                  + 2.42% __alloc_frozen_pages_noprof
                                                            - 0.79% uvm_cpu_chunk_create
                                                               - 0.72% __uvm_kvmalloc_zero
                                                                    0.65% __kmalloc_noprof
                                                         - 2.56% uvm_cpu_chunk_insert_in_block
                                                        0.58% gpu_chunk_add
                                                   - 7.23% uvm_va_block_service_copy
                                                      - 5.54% uvm_tracker_wait
                                                         - 4.27% uvm_spin_loop
                                                            - 1.42% schedule
                                                               + __schedule
                                                            - 0.86% ktime_get_raw_ts64
                                                                 read_tsc
                                                         - 0.88% uvm_tracker_remove_completed
                                                            - 0.74% uvm_gpu_tracking_semaphore_is_value_completed
                                                                 uvm_gpu_tracking_semaphore_update_completed_value
                                                      - 1.67% uvm_va_block_make_resident_copy
                                                         - 1.42% block_copy_resident_pages
                                                            - 1.41% block_copy_resident_pages_mask
                                                               - 1.39% block_copy_resident_pages_from
                                                                  - 1.37% block_copy_resident_pages_between
                                                                     - 1.09% block_copy_pages
                                                                        - 0.64% block_phys_page_copy_address.isra.0
                                                                             block_phys_page_address.isra.0
                                                   - 3.77% free_zone_device_folio
                                                      - 2.46% devmem_folio_free
                                                         - 1.81% _raw_spin_lock
                                                              1.43% native_queued_spin_lock_slowpath
                                                      - 1.16% _raw_q_schedule
                                                           0.79% __raw_spin_lock_irqsave
                                                   - 1.55% remove_migration_ptes
                                                      - 1.40% rmap_walk_anon
                                                         - 1.14% remove_migration_pte
                                                              0.62% page_vma_mapped_walk
                                                   - 0.96% __folio_batch_add_and_move
                                                      - folio_batch_move_lru
                                                           0.65% lru_add
                                                - 1.42% migrate_vma_setup
                                                   - 1.19% walk_page_range_mm_unsafe
                                                      - 1.18% __walk_page_range
                                                         - walk_pgd_range
                                                         - walk_p4d_range
                                                            - walk_pud_range.isra.0
                                                            - walk_pmd_range
                                                                 migrate_vma_collect_pmd
                                                - 1.19% __migrate_device_pages
                                                     0.89% folio_migrate_flags
                                                  0.85% uvm_va_block_service_finish
         + 8.86% __munmap
         + 5.03% initVector(float*, unsigned long) (inlined)
	 - 0.79% copyKernel(float const*, float*) (inlined)
              __device_stub__Z10copyKernelPKfPf(float const*, float*)
              cudaLaunchKernel
              libcudart_static_4d8b33a106dceb3c07a56e26de61f2d53bb62a68
              libcudart_static_19d132c815f85334ee1756d16281ce95ea02e148
              libcudart_static_ad9262876fc0252a7a95dbc876859d40746b0b56
              libcudart_static_5449a5077e9603031b6d1a024d25647afd27a0ba
              cuDevicePrimaryCtxRetain
              0x7f5af551b425
              0x7f5af544f5a9
              0x7f5af544f37c
```

但是如果是:
1. malloc
2. memset
3. copyKernel
```txt
   - 98.83% demoPageableMemoryDirectAccess()
         - 77.55% cudaDeviceSynchronize
              libcudart_static_ff466e58390a44a174cd1b2084ede65f6033356c
              cuCtxSynchronize
              0x7fba69720485
              0x7fba6964214e
            + 0x7fba696414d9
         + 19.03% __munmap
         - 2.25% copyKernel(float const*, float*) (inlined)
              __device_stub__Z10copyKernelPKfPf(float const*, float*)
              cudaLaunchKernel
              libcudart_static_4d8b33a106dceb3c07a56e26de61f2d53bb62a68
              libcudart_static_19d132c815f85334ee1756d16281ce95ea02e148
              libcudart_static_ad9262876fc0252a7a95dbc876859d40746b0b56
              libcudart_static_5449a5077e9603031b6d1a024d25647afd27a0ba
              cuDevicePrimaryCtxRetain
              0x7fba6971b425
              0x7fba6964f5a9
            - 0x7fba6964f37c
               - 1.37% 0x7fba6964d0ea
                    0.81% 0x7fba696297ec
                    0.55% 0x7fba69629adc
```
也就是，这个分配的空间相当于是被 GPU 自动迁移到显存中， 然后
如果后面 CPU 来访问的时候，在自动的 page fault ，迁移回来。

## 那么 devmem_fault_entry 的 entry 是如何构件的

## 不要被 nvidia-smi 骗了

做这个测试的时候，发现 chapter_demo.out 总是占用 128M 的显存，但是注意，
总的显存使用已经到了 8G ，而我们正好用掉了 8G 的空间:
```txt
+-----------------------------------------------------------------------------------------+
Tue May 26 13:23:54 2026
+-----------------------------------------------------------------------------------------+
| NVIDIA-SMI 595.71.05              Driver Version: 595.71.05      CUDA Version: 13.2     |
+-----------------------------------------+------------------------+----------------------+
| GPU  Name                 Persistence-M | Bus-Id          Disp.A | Volatile Uncorr. ECC |
| Fan  Temp   Perf          Pwr:Usage/Cap |           Memory-Usage | GPU-Util  Compute M. |
|                                         |                        |               MIG M. |
|=========================================+========================+======================|
|   0  NVIDIA GeForce RTX 5060 Ti     Off |   00000000:01:00.0 Off |                  N/A |
|  0%   41C    P1             20W /  180W |    8750MiB /  16311MiB |      0%      Default |
|                                         |                        |                  N/A |
+-----------------------------------------+------------------------+----------------------+

+-----------------------------------------------------------------------------------------+
| Processes:                                                                              |
|  GPU   GI   CI              PID   Type   Process name                        GPU Memory |
|        ID   ID                                                               Usage      |
|=========================================================================================|
|    0   N/A  N/A           27859      G   /usr/bin/gnome-shell                      2MiB |
|    0   N/A  N/A          840886      C   ...data/ComfyUI/.venv/bin/python        128MiB |
|    0   N/A  N/A          885814      G   netease-cloud-music-gtk4                  2MiB |
|    0   N/A  N/A         2160406    C+G   ...am/ubuntu12_64/steamwebhelper          5MiB |
|    0   N/A  N/A         4028576      C   ...ystem-memory/chapter_demo.out        128MiB |
+-----------------------------------------------------------------------------------------+
```

## 原来早已等候多个时间
/home/martins3/data/kernel/linux-drm/Documentation/gpu/rfc/gpusvm.rst

## 原来注册这个函数，仅仅关心 device to host

include/linux/memremap.h

drm 机制存在标准实现:
drivers/gpu/drm/drm_pagemap.c

```c
static const struct dev_pagemap_ops uvm_pmm_devmem_ops =
{
#if defined(NV_PAGEMAP_OPS_HAS_FOLIO_FREE)
    .folio_free = devmem_folio_free,
#else
    .page_free = devmem_page_free,
#endif
    .migrate_to_ram = devmem_fault_entry,
};
```

## 观察到  __gup_longterm_locked 是什么意思?
```txt
@[
        __gup_longterm_locked+1
        pin_user_pages+109
        os_lock_user_pages+188
        RmCreateOsDescriptor+115
        RmIoctl+3023
        rm_ioctl+103
        nvidia_ioctl.isra.0+1505
        nvidia_unlocked_ioctl+34
        __x64_sys_ioctl+185
        do_syscall_64+265
        entry_SYSCALL_64_after_hwframe+118
]: 4
```
