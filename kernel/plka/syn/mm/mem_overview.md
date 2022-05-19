| File                 | blank | comment | code | desc                                                                                                                                                         |
|----------------------|-------|---------|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| page_alloc.c         | 1119  | 1988    | 5012 | 分配page frame                                                                                                                                               |
| memcontrol.c         | 943   | 1399    | 4362 | cgroup                                                                                                                                                              |
| slub.c               | 911   | 991     | 3974 |                                                                                                                                                              |
| hugetlb.c            | 640   | 1130    | 3158 | superpage 相关的内容吧 @todo                                                                                                                                 |
| memory.c             | 548   | 1136    | 3100 | pagefault 以及辅助函数，类似于 pagetable 各种操控函数 @todo 分析的清楚 pgfault 那么这个问题就没有什么意义了                                                  |
| shmem.c              | 478   | 558     | 3012 | @todo shmem 的复杂度现在其实难以估计，到底cow机制还是ipc相关的处理，还是各种节省内存的方法 ?                                                                 |
| slab.c               | 669   | 834     | 2974 | 暂时不用关注的内容，认为已经过时了。                                                                                                                         |
| swapfile.c           | 464   | 632     | 2682 | 应该是处理当 swap 的base是file而不是partion 的情况!                                                                                                          |
| mmap.c               | 467   | 910     | 2332 | mmap 实现                                                                                                                                                    |
| vmscan.c             | 547   | 1364    | 2304 | 扫描确定需要回收页面，也就是 page reclaim 机制                                                                                                               |
| huge_memory.c        | 354   | 458     | 2118 | transparent hugetlb                                                                                                                                          |
| ksm.c                | 336   | 845     | 1998 | https://en.wikipedia.org/wiki/Kernel_same-page_merging ksm 将内容相同的page 合并                                                                             |
| mempolicy.c          | 365   | 585     | 1928 | numa 系统分配内存选择 memory node  @todo 所以难道分配给local不好吗?                                                                                          |
| filemap.c            | 364   | 1052    | 1915 | 实现page cache　也就是无处不在的 address_space                                                                                                               |
| migrate.c            | 395   | 715     | 1844 | 似乎处理的事情是将内存从一个memory node 到另一个 memory node                                                                                                 |
| zsmalloc.c           | 409   | 408     | 1723 | https://lwn.net/Articles/477067/ 新的分配器 @todo 文章顺便提到CMA等                                                                                          |
| vmalloc.c            | 383   | 678     | 1689 | vmalloc 实现物理页面和虚拟页面的建立映射                                                                                                                     |
| vmstat.c             | 325   | 337     | 1479 | 各种统计                                                                                                                                                     |
| page-writeback.c     | 329   | 1031    | 1472 | Contains functions related to writing back dirty pages at the  address_space level.                                                                          |
| khugepaged.c         | 243   | 289     | 1417 | transparent hugetlb 的守护进程                                                                                                                                                             |
| kmemleak.c           | 249   | 615     | 1260 | 识别内核中间内存泄露的工具                                                                                |
| memory_hotplug.c     | 301   | 374     | 1231 | 应该是numa，默认不会编译进去的                                                                                                                               |
| nommu.c              | 288   | 431     | 1231 | nommu 处理没有mmu的情况                                                                                                                                      |
| compaction.c         | 338   | 605     | 1215 | 依托于memory migration 实现减少 external fragmentation，但是page_alloc.c 中间不是已经处理过这一个事情了吗 ?　各自侧重在于何处 ?                              |
| gup.c                | 192   | 511     | 1175 | 访问user 的内存 @todo 也是非常神奇的东西呀 !                                                                                                                 |
| memory-failure.c     | 202   | 604     | 1119 | 处理内存控制器之类的底层错误的                                                                                                                               |
| rmap.c               | 243   | 626     | 1093 | 实现反向映射                                                                                                                                                 |
| memblock.c           | 246   | 593     | 1082 | 启动时候的内存管理                                                                                                                                           |
| slab_common.c        | 247   | 287     | 1027 | 各种slab slob 分配器的公用函数                                                                                                                               |
| hmm.c                | 200   | 278     | 943  | https://www.kernel.org/doc/html/v4.18/vm/hmm.html  处理异构内存                                                                                              |
| zswap.c              | 211   | 240     | 914  | 对于swap的内容进行压缩                                                                                                                                       |
| backing-dev.c        | 174   | 140     | 766  | 应该是用于实现写会到磁盘的内容，@todo 但是具体细节不知道是如何实现的! 如果 backing-dev.c 中间的内容真的是用来实现写回到磁盘，那么 fs/buffer.c 的功能是什么 ? |
| z3fold.c             | 119   | 249     | 762  | z3fold is an special purpose allocator for storing compressed pages                                                                                          |
| kasan/kasan.c        | 155   | 84      | 664  | https://github.com/google/kasan/wiki @todo 和kmemleak.c 做的内容有什么不同呢?                                                                                |
| oom_kill.c           | 142   | 342     | 663  | oom killer 新手入门的好文档                                                                                                                                  |
| swap.c               | 125   | 315     | 598  | reclaim 机制中间，用于提供 pagevec 和 page 在 lrulist 之间倒腾的作用。                                                                                       |
| madvise.c            | 99    | 199     | 597  | Man madvice                                                                                                                                                  |
| swap_state.c         | 95    | 184     | 574  | swap cache ? 熟悉的内容.                                                                                                                                     |
| sparse.c             | 109   | 110     | 572  | 处理sparese memory model                                                                                                                                     |
| bootmem.c            | 150   | 144     | 517  | memblock的出现已经取消掉东西                                                                                                                                 |
| list_lru.c           | 101   | 48      | 511  | 依赖于MEMCG_KMEM，默认不使用，好像也不是                                                                                                                               |
| truncate.c           | 86    | 324     | 507  | 将 page 从 page cache 中间删除                                                                                                                               |
| mlock.c              | 109   | 267     | 492  | 用于mlock系统调用，将有的 vma 需要 lock，到底其上映射的 page 不会被清理掉                                                                                    |
| page_owner.c         | 124   | 59      | 456  | https://www.kernel.org/doc/html/latest/vm/page_owner.html 内核的检查错误机制，默认被 disable 掉                                                              |
| mprotect.c           | 90    | 94      | 452  | 处理mprotect 系统调用，修改制定VMA的权限                                                                                                                     |
| slob.c               | 92    | 124     | 447  | 含有详细的分配器代码，如果开始逐行分析，是一个好入口!                                                                                                        |
| util.c               | 107   | 241     | 442  | 各种辅助函数，@todo 内容有点杂                                                                                                                               |
| mremap.c             | 82    | 115     | 440  | mremap 系统调用支持 @todo 查一下tlpi 系统调用的作用吧!                                                                                                       |
| userfaultfd.c        | 70    | 130     | 387  | 用于支持虚拟机的，似乎defconfig 模式没有将其编译进去                                                                        |
| kasan/kasan_init.c   | 80    | 41      | 368  |                                                                                                                                                              |
| slab.h               | 85    | 86      | 360  |                                                                                                                                                              |
| dmapool.c            | 63    | 112     | 358  | dmapool 用于支持dma 的，                                                                                                     |
| kasan/report.c       | 74    | 34      | 343  |                                                                                                                                                              |
| zbud.c               | 64    | 236     | 336  | zbud is an special purpose allocator for storing compressed pages.                                                                                           |
| mempool.c            | 63    | 158     | 328  | 似乎就是基于kmalloc实现的内存池，@todo 所以为什么我们需要内存池啊!0                                                                                          |
| cma.c                | 80    | 124     | 320  | https://lwn.net/Articles/486301/ 有一个allocator                                                                                                             |
| readahead.c          | 75    | 214     | 319  | 预取文件内容，现在发现只要是处理文件IO的函数，参数都有address_space                                                                                          |
| page_io.c            | 48    | 71      | 315  | swap_readpage 和 swap_writepage 当使用partion 的时候，调用bdev_read_page 否则利用 file 对应的 aops 的 readpage/writepage                                     |
| hugetlb_cgroup.c     | 67    | 59      | 314  |                                                                                                                                                              |
| internal.h           | 77    | 159     | 294  | 貌似定义了一些会被mm/ 下各种文件使用的辅助函数和定义，比如alloc_context 和 compact_control                                                                   |
| frontswap.c          | 57    | 150     | 291  | https://www.kernel.org/doc/html/latest/vm/frontswap.html WTM惊了，用各种其他介质替代swap 机制默认使用的swap                                                  |
| highmem.c            | 68    | 141     | 276  |                                                                                                                                                              |
| page_ext.c           | 64    | 94      | 266  | https://en.wikipedia.org/wiki/Physical_Address_Extension 使用page ext                                                                                        |
| pagewalk.c           | 44    | 62      | 252  | 各种page walk @todo 没有分析谁来调用!                                                                                                                        |
| mmu_notifier.c       | 51    | 135     | 251  | 用于支持虚拟机实现hardwared处理                                                                                             |
| process_vm_access.c  | 46    | 84      | 246  | 支持系统调用process_vm_readv 和 process_vm_writev @todo 但是不知道谁在使用这一个东西!                                                                        |
| nobootmem.c          | 72    | 133     | 240  | 感觉只是为了处理当 bootmem 被取消之后，为了保证interface 的一致而已 ?                                                                                        |
| swap_slots.c         | 38    | 84      | 238  | 为了降低对于swap_info的锁的争用而是用的                                                                                                                      |
| vmpressure.c         | 48    | 189     | 235  |                                                                                                                                                              |
| memfd.c              | 51    | 61      | 233  | memfd_create syscall的支持。                                                                                                                                 |
| early_ioremap.c      | 48    | 28      | 227  | 功能应该和其名称类似，但是不知道ioremap 为什么需要放到 mm/ 下面                                                                                              |
| workingset.c         | 41    | 283     | 215  | 我感觉又是用来支持swap 的 active list 和 inactive list 的                                                                                                    |
| kasan/quarantine.c   | 54    | 70      | 204  |                                                                                                                                                              |
| sparse-vmemmap.c     | 32    | 34      | 197  | sparse 内存模型支持 pfn_to_page 之类简单实现。                                                                                                               |
| page_isolation.c     | 35    | 86      | 194  | 由于没有CONFIG_MEMORY_IOSATION，所以并没有人调用该模块，应该还是处理migration相关的内容                                                                      |
| percpu-vm.c          | 47    | 141     | 191  | @todo                                                                                                                                                        |
| percpu-stats.c       | 41    | 41      | 154  |                                                                                                                                                              |
| percpu-internal.h    | 39    | 60      | 126  |                                                                                                                                                              |
| percpu-km.c          | 22    | 30      | 67   |                                                                                                                                                              |
| percpu.c             | 353   | 957     | 1478 | percpu allocator 就是实现percpu的                                                                                                                            |
| mincore.c            | 27    | 69      | 177  | syscall determine whether pages are resident in memory                                                                                                       |
| page_idle.c          | 29    | 33      | 176  | @todo                                                                                                                                                        |
| page_vma_mapped.c    | 23    | 64      | 168  | `page_vma_mapped_walk` Returns true if the page is mapped in the vma 唯一被使用非static 函数，我感觉没有拆分成为新的static的必要                             |
| usercopy.c           | 43    | 98      | 165  | 各种检查函数用于支持 user 和 kernel 之间的拷贝                                                                                                               |
| cleancache.c         | 30    | 125     | 162  | https://www.kernel.org/doc/html/latest/vm/cleancache.html 还是和page cache 有关的内容                                                                        |
| zpool.c              | 41    | 179     | 161  | This is a common frontend for memory storage pool implementations. Typically, this is used to store compressed memory.                                       |
| swap_cgroup.c        | 32    | 42      | 159  |                                                                                                                                                              |
| pgtable-generic.c    | 24    | 30      | 158  | 对于主流的架构来说，这一个文件是一个空文件，实现在具体的arch中间                                                                                             |
| cma_debug.c          | 42    | 7       | 153  |                                                                                                                                                              |
| mm_init.c            | 27    | 13      | 152  | 初始化mm 子系统，使用@todo postcore_initcall 的                                                                                                              |
| debug.c              | 20    | 17      | 141  | dump_page 和 dump_vma                                                                                                                                        |
| frame_vector.c       | 17    | 86      | 136  | 提供一个函数给驱动使用，具体作用不清楚                                                                                                                       |
| fadvise.c            | 29    | 57      | 134  | Man fadvice 提前通知kernel 将要访存                                                                                                                          |
| page_counter.c       | 35    | 103     | 128  | 配合memcontrol 使用                                                                                                                                          |
| kasan/kasan.h        | 24    | 21      | 122  |                                                                                                                                                              |
| hwpoison-inject.c    | 25    | 17      | 98   | https://www.kernel.org/doc/html/latest/vm/hwpoison.html 辅助错误恢复                                                                                         |
| balloon_compaction.c | 20    | 62      | 95   | 和ballon 机制相关                                                                                                                                            |
| memtest.c            | 16    | 3       | 94   | 测试物理内存是否可用                                                                                                                                         |
| Makefile             | 10    | 7       | 90   |                                                                                                                                                              |
| page_poison.c        | 22    | 13      | 89   | 也是为了处理物理内存错误                                                                                                    |
| interval_tree.c      | 16    | 8       | 88   | interval tree for `mapping->i_mmap`                                                                                                                            |
| gup_benchmark.c      | 21    | 0       | 82   |                                                                                                                                                              |
| mmzone.c             | 20    | 14      | 81   | management codes for pgdats, zones and page flagsmanagement codes for pgdats, zones and page flags                                                           |
| vmacache.c           | 18    | 21      | 79   | mm 访问 vma 依赖于此加速                                                                                                                                     |
| msync.c              | 4     | 30      | 74   | msync syscall 同步 The msync() function shall write all modified data to permanent storage locations                                                         |
| kmemleak-test.c      | 14    | 32      | 65   |                                                                                                                                                              |
| quicklist.c          | 18    | 22      | 63   |                                                                                                                                                              |
| failslab.c           | 14    | 2       | 51   |                                                                                                                                                              |
| maccess.c            | 15    | 43      | 49   | @todo 看似简单，其实根本不知道是干什么的                                                                                                                                                              |
| debug_page_ref.c     | 8     | 1       | 46   |                                                                                                                                                              |
| mmu_context.c        | 7     | 20      | 37   | use_mm 和 unuse_mm 来切换到特定的mm context,并不是非常清楚会有代码使用这种功能                                                                               |
| rodata_test.c        | 8     | 16      | 33   |                                                                                                                                                              |
| init-mm.c            | 3     | 11      | 26   | 定义用于初始化用途的mm                                                                                                                                       |
| cma.h                | 4     | 1       | 21   |                                                                                                                                                              |
| kasan/Makefile       | 2     | 3       | 6    |                                                                                                                                                              |


## 当前的路线

1. SetPageSwapBacked 的作用
    1. unuse -> shmem_unuse -> shmem_writepage(shmem.md : 462)

2. PageDirty 和 PageUptodate 关系是什么，感觉 up to date 其实是 valid 的含义 !

3. `__test_set_page_writeback` 是不是相关的内容在 writeback dirty 相关的存在分析了。
    1. end_page_writeback 和 set_page_writeback 的关系是什么 ?

## 找到这些想法的证据
mmap() 的实现，如果多个 process 都是 map 到同一个文件 : 统一一下 mmap 和 pgfault 的对照关系。
1. 开始的时候注册
2. 之后利用注册的信息实现 pgfault，并且利用 page fault 的方法，找到之后，将该 page 注入到 vma 的 page table 上。
    1. 根据的pgfault 的类型，可能会出现unshared 写操作，那么就会进行拷贝之类的

5. filemap.c 到处都是使用 PageUptodate 检查一下其中的语义 ? 还有 ClearPageError

1. anon 的 unmap 就是释放内存到 swap 中间，但是当我没有swap 如何办 ?
    1. 似乎从来都没有找到 swap 没有任何空间，系统的处理办法!
