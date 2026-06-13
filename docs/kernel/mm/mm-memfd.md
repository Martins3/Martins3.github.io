# memfd

memfd 的主要功能体现在
- [Keeping secrets in memfd areas](https://mp.weixin.qq.com/s/CI9ZZurCXjY9xF75lK8NjA)
- [Private memory for KVM guests](https://mp.weixin.qq.com/s/orDF_F2iTY-M5tYKY7cvCQ)


## 类似，但是处理安全
- mm/secretmem.c

更多参考 [Two address-space-isolation patches get closer](https://lwn.net/Articles/835342/)

偶尔遇到的警告:
```txt
[132088.600448] memfd_create() without MFD_EXEC nor MFD_NOEXEC_SEAL, pid=5625 'X11 Bell'
```

https://github.com/a-darwish/memfd-examples

- [ ] vhost 和 qemu 之间可以通过 memfd 工作吗？
  - 应该没问


## 简单看看其中的数据流

使用 core/vn/code/src/c/mm/memfd.c ，使用 perf 观测，结果如下:
```txt
-   89.60%     0.00%  memfd.out  libc.so.6          [.] __libc_start_call_main
     __libc_start_call_main
   - main
      - 85.26% asm_exc_page_fault
         - exc_page_fault
            - 85.04% do_user_addr_fault
               - 70.21% handle_mm_fault
                  - 68.19% do_pte_missing
                     - 63.10% __do_fault
                        - 62.70% shmem_fault
                           - shmem_get_folio_gfp
                                34.91% clear_page_erms
                              - 14.71% shmem_alloc_and_add_folio
                                 - 4.09% shmem_add_to_page_cache
                                      3.41% _raw_spin_unlock_irq
                                 - 2.92% folio_alloc_mpol_noprof
                                    - alloc_pages_mpol_noprof
                                       - 2.74% __alloc_pages_noprof
                                          - 2.44% get_page_from_freelist
                                             - 0.69% __rmqueue_pcplist
                                                  _raw_spin_unlock_irqrestore
                                 - 2.72% folio_add_lru
                                    - 2.03% folio_batch_move_lru
                                         1.77% _raw_spin_unlock_irqrestore
                                 - 1.98% __mem_cgroup_charge
                                      0.81% mem_cgroup_commit_charge
                                      0.50% get_mem_cgroup_from_mm
                                 - 1.83% shmem_inode_acct_blocks
                                    - 1.21% __dquot_alloc_space
                                       - 0.63% inode_add_bytes
                                            _raw_spin_lock
                              - 1.06% filemap_get_entry
                                   0.96% xas_load
                     - 2.58% finish_fault
                        - 0.85% __pte_offset_map_lock
                             0.56% _raw_spin_lock
                        - 0.81% set_pte_range
                             0.61% folio_add_file_rmap_ptes
                     - 2.17% fault_dirty_shared_page
                          0.50% up_read
                    0.61% __rcu_read_unlock
               - 1.56% lock_vma_under_rcu
                    0.69% down_read_trylock
                    0.67% mas_walk
```

cat /proc/pid/maps 可以看到:
```txt
7f14616cf000-7f1e616cf000 rw-s 00000000 00:01 5132                       /memfd:Server memfd (deleted)
```

观测 /proc/self/fd 的目录可以看到:
```txt
🧀  l fd
Permissions Size User     Date Modified Name
lrwx------     - martins3  9 Nov 16:01   0 -> /dev/pts/5
lrwx------     - martins3  9 Nov 16:01   1 -> /dev/pts/5
lrwx------     - martins3  9 Nov 16:01   2 -> /dev/pts/5
lrwx------     - martins3  9 Nov 16:01   3 -> '/memfd:Server memfd (deleted)'
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
