| File                 | blank | comment | code | desc                                                                                                                                                         |
|----------------------|-------|---------|------|--------------------------------------------------------------------------------------------------------------------------------------------------------------|
| page_alloc.c         | 1119  | 1988    | 5012 | 分配 page frame                                                                                                                                               |
| memcontrol.c         | 943   | 1399    | 4362 | cgroup                                                                                                                                                       |
| slub.c               | 911   | 991     | 3974 |                                                                                                                                                              |
| hugetlb.c            | 640   | 1130    | 3158 | superpage 相关的内容吧 @todo                                                                                                                                 |
| memory.c             | 548   | 1136    | 3100 | pagefault 以及辅助函数，类似于 pagetable 各种操控函数 @todo 分析的清楚 pgfault 那么这个问题就没有什么意义了                                                  |
| shmem.c              | 478   | 558     | 3012 | 用于支持 tmpfs 和 sysv shm 的实现                                                                 |
| slab.c               | 669   | 834     | 2974 | 暂时不用关注的内容，认为已经过时了。                                                                                                                         |
| swapfile.c           | 464   | 632     | 2682 | 应该是处理当 swap 的 base 是 file 而不是 partion 的情况!                                                                                                          |
| mmap.c               | 467   | 910     | 2332 | mmap munmap 实现                                                                                                                                                    |
| vmscan.c             | 547   | 1364    | 2304 | 扫描确定需要回收页面，也就是 page reclaim 机制                                                                                                               |
| huge_memory.c        | 354   | 458     | 2118 | transparent hugetlb                                                                                                                                          |
| ksm.c                | 336   | 845     | 1998 | https://en.wikipedia.org/wiki/Kernel_same-page_merging ksm 将内容相同的 page 合并                                                                             |
| mempolicy.c          | 365   | 585     | 1928 | numa 系统分配内存选择 memory node  @todo 所以难道分配给 local 不好吗?                                                                                          |
| filemap.c            | 364   | 1052    | 1915 | 实现 page cache　也就是无处不在的 address_space                                                                                                               |
| migrate.c            | 395   | 715     | 1844 | 似乎处理的事情是将内存从一个 memory node 到另一个 memory node                                                                                                 |
| zsmalloc.c           | 409   | 408     | 1723 | https://lwn.net/Articles/477067/ 新的分配器                                                                                          |
| vmalloc.c            | 383   | 678     | 1689 | vmalloc 实现物理页面和虚拟页面的建立映射                                                                                                                     |
| vmstat.c             | 325   | 337     | 1479 | 各种统计                                                                                                                                                     |
| page-writeback.c     | 329   | 1031    | 1472 | Contains functions related to writing back dirty pages at the  address_space level.                                                                          |
| khugepaged.c         | 243   | 289     | 1417 | transparent hugetlb 的守护进程                                                                                                                               |
| kmemleak.c           | 249   | 615     | 1260 | 识别内核中间内存泄露的工具                                                                                                                                   |
| memory_hotplug.c     | 301   | 374     | 1231 | 内核热插拔                                                                                                                               |
| nommu.c              | 288   | 431     | 1231 | nommu 处理没有 mmu 的情况                                                                                                                                      |
| compaction.c         | 338   | 605     | 1215 | 依托于 memory migration 实现减少 external fragmentation，但是 page_alloc.c 中间不是已经处理过这一个事情了吗 ?　各自侧重在于何处 ?                              |
| gup.c                | 192   | 511     | 1175 | 访问 user 的内存 @todo 也是非常神奇的东西呀 !                                                                                                                 |
| memory-failure.c     | 202   | 604     | 1119 | 处理内存控制器之类的底层错误的                                                                                                                               |
| rmap.c               | 243   | 626     | 1093 | 反向映射                                                                                                                                                 |
| memblock.c           | 246   | 593     | 1082 | 启动时候的内存管理                                                                                                                                           |
| slab_common.c        | 247   | 287     | 1027 | 各种 slab slob 分配器的公用函数                                                                                                                               |
| hmm.c                | 200   | 278     | 943  | https://www.kernel.org/doc/html/v4.18/vm/hmm.html  处理异构内存                                                                                              |
| zswap.c              | 211   | 240     | 914  | 对于 swap 的内容进行压缩                                                                                                                                       |
| backing-dev.c        | 174   | 140     | 766  | 应该是用于实现写会到磁盘的内容，@todo 但是具体细节不知道是如何实现的! 如果 backing-dev.c 中间的内容真的是用来实现写回到磁盘，那么 fs/buffer.c 的功能是什么 ? |
| z3fold.c             | 119   | 249     | 762  | z3fold is an special purpose allocator for storing compressed pages                                                                                          |
| kasan/kasan.c        | 155   | 84      | 664  | https://github.com/google/kasan/wiki @todo 和 kmemleak.c 做的内容有什么不同呢?                                                                                |
| oom_kill.c           | 142   | 342     | 663  | oom killer 新手入门的好文档                                                                                                                                  |
| swap.c               | 125   | 315     | 598  | reclaim 机制中间，用于提供 pagevec 和 page 在 lrulist 之间倒腾的作用。                                                                                       |
| madvise.c            | 99    | 199     | 597  | Man madvice                                                                                                                                                  |
| swap_state.c         | 95    | 184     | 574  | swap cache ? 熟悉的内容.                                                                                                                                     |
| sparse.c             | 109   | 110     | 572  | 处理 sparese memory model                                                                                                                                     |
| bootmem.c            | 150   | 144     | 517  | memblock 的出现已经取消掉东西                                                                                                                                 |
| list_lru.c           | 101   | 48      | 511  | 依赖于 MEMCG_KMEM，默认不使用，好像也不是                                                                                                                     |
| truncate.c           | 86    | 324     | 507  | 将 page 从 page cache 中间删除                                                                                                                               |
| mlock.c              | 109   | 267     | 492  | 用于 mlock 系统调用，将有的 vma 需要 lock，到底其上映射的 page 不会被清理掉                                                                                    |
| page_owner.c         | 124   | 59      | 456  | https://www.kernel.org/doc/html/latest/vm/page_owner.html 内核的检查错误机制，默认被 disable 掉                                                              |
| mprotect.c           | 90    | 94      | 452  | 处理 mprotect 系统调用，修改制定 VMA 的权限                                                                                                                     |
| slob.c               | 92    | 124     | 447  | 含有详细的分配器代码，如果开始逐行分析，是一个好入口!                                                                                                        |
| util.c               | 107   | 241     | 442  | 各种辅助函数，@todo 内容有点杂                                                                                                                               |
| mremap.c             | 82    | 115     | 440  | mremap 系统调用支持 @todo 查一下 tlpi 系统调用的作用吧!                                                                                                       |
| userfaultfd.c        | 70    | 130     | 387  | 用于支持虚拟机的，似乎 defconfig 模式没有将其编译进去                                                                                                         |
| kasan/kasan_init.c   | 80    | 41      | 368  |                                                                                                                                                              |
| slab.h               | 85    | 86      | 360  |                                                                                                                                                              |
| dmapool.c            | 63    | 112     | 358  | dmapool 用于支持 dma 的，                                                                                                                                     |
| kasan/report.c       | 74    | 34      | 343  |                                                                                                                                                              |
| zbud.c               | 64    | 236     | 336  | zbud is an special purpose allocator for storing compressed pages.                                                                                           |
| mempool.c            | 63    | 158     | 328  | 似乎就是基于 kmalloc 实现的内存池，@todo 所以为什么我们需要内存池啊!0                                                                                          |
| cma.c                | 80    | 124     | 320  | https://lwn.net/Articles/486301/ 有一个 allocator                                                                                                             |
| readahead.c          | 75    | 214     | 319  | 预取文件内容，现在发现只要是处理文件 IO 的函数，参数都有 address_space                                                                                          |
| page_io.c            | 48    | 71      | 315  | swap_readpage 和 swap_writepage 当使用 partion 的时候，调用 bdev_read_page 否则利用 file 对应的 aops 的 readpage/writepage                                     |
| hugetlb_cgroup.c     | 67    | 59      | 314  |                                                                                                                                                              |
| internal.h           | 77    | 159     | 294  | 貌似定义了一些会被 mm/ 下各种文件使用的辅助函数和定义，比如 alloc_context 和 compact_control                                                                   |
| frontswap.c          | 57    | 150     | 291  | https://www.kernel.org/doc/html/latest/vm/frontswap.html WTM 惊了，用各种其他介质替代 swap 机制默认使用的 swap                                                  |
| highmem.c            | 68    | 141     | 276  |                                                                                                                                                              |
| page_ext.c           | 64    | 94      | 266  | https://en.wikipedia.org/wiki/Physical_Address_Extension 使用 page ext                                                                                        |
| pagewalk.c           | 44    | 62      | 252  | 各种 page walk @todo 没有分析谁来调用!                                                                                                                        |
| mmu_notifier.c       | 51    | 135     | 251  | 用于支持虚拟机实现 hardwared 处理                                                                                                                              |
| process_vm_access.c  | 46    | 84      | 246  | 支持系统调用 process_vm_readv 和 process_vm_writev @todo 但是不知道谁在使用这一个东西!                                                                        |
| nobootmem.c          | 72    | 133     | 240  | 感觉只是为了处理当 bootmem 被取消之后，为了保证 interface 的一致而已 ?                                                                                        |
| swap_slots.c         | 38    | 84      | 238  | 为了降低对于 swap_info 的锁的争用而是用的                                                                                                                      |
| vmpressure.c         | 48    | 189     | 235  |                                                                                                                                                              |
| memfd.c              | 51    | 61      | 233  | memfd_create syscall 的支持。                                                                                                                                 |
| early_ioremap.c      | 48    | 28      | 227  | 功能应该和其名称类似，但是不知道 ioremap 为什么需要放到 mm/ 下面                                                                                              |
| workingset.c         | 41    | 283     | 215  | 我感觉又是用来支持 swap 的 active list 和 inactive list 的                                                                                                    |
| kasan/quarantine.c   | 54    | 70      | 204  |                                                                                                                                                              |
| sparse-vmemmap.c     | 32    | 34      | 197  | sparse 内存模型支持 pfn_to_page 之类简单实现。                                                                                                               |
| page_isolation.c     | 35    | 86      | 194  | 由于没有 CONFIG_MEMORY_IOSATION，所以并没有人调用该模块，应该还是处理 migration 相关的内容                                                                      |
| percpu-vm.c          | 47    | 141     | 191  | @todo                                                                                                                                                        |
| percpu-stats.c       | 41    | 41      | 154  |                                                                                                                                                              |
| percpu-internal.h    | 39    | 60      | 126  |                                                                                                                                                              |
| percpu-km.c          | 22    | 30      | 67   |                                                                                                                                                              |
| percpu.c             | 353   | 957     | 1478 | percpu allocator 就是实现 percpu 的                                                                                                                            |
| mincore.c            | 27    | 69      | 177  | syscall determine whether pages are resident in memory                                                                                                       |
| page_idle.c          | 29    | 33      | 176  | Idle Page Tracking : https://www.kernel.org/doc/html/latest/admin-guide/mm/idle_page_tracking.html                                                           |
| page_vma_mapped.c    | 23    | 64      | 168  | `page_vma_mapped_walk` Returns true if the page is mapped in the vma 唯一被使用非 static 函数，我感觉没有拆分成为新的 static 的必要                             |
| usercopy.c           | 43    | 98      | 165  | 各种检查函数用于支持 user 和 kernel 之间的拷贝                                                                                                               |
| cleancache.c         | 30    | 125     | 162  | https://www.kernel.org/doc/html/latest/vm/cleancache.html 还是和 page cache 有关的内容                                                                        |
| zpool.c              | 41    | 179     | 161  | This is a common frontend for memory storage pool implementations. Typically, this is used to store compressed memory.                                       |
| swap_cgroup.c        | 32    | 42      | 159  |                                                                                                                                                              |
| pgtable-generic.c    | 24    | 30      | 158  | 对于主流的架构来说，这一个文件是一个空文件，实现在具体的 arch 中间                                                                                             |
| cma_debug.c          | 42    | 7       | 153  |                                                                                                                                                              |
| mm_init.c            | 27    | 13      | 152  | 初始化 mm 子系统，使用@todo postcore_initcall 的                                                                                                              |
| debug.c              | 20    | 17      | 141  | dump_page 和 dump_vma                                                                                                                                        |
| frame_vector.c       | 17    | 86      | 136  | 提供一个函数给驱动使用，具体作用不清楚                                                                                                                       |
| fadvise.c            | 29    | 57      | 134  | Man fadvice 提前通知 kernel 将要访存                                                                                                                          |
| page_counter.c       | 35    | 103     | 128  | 配合 memcontrol 使用                                                                                                                                          |
| kasan/kasan.h        | 24    | 21      | 122  |                                                                                                                                                              |
| hwpoison-inject.c    | 25    | 17      | 98   | https://www.kernel.org/doc/html/latest/vm/hwpoison.html 辅助错误恢复                                                                                         |
| balloon_compaction.c | 20    | 62      | 95   | 和 balloon 机制相关                                                                                                                                            |
| memtest.c            | 16    | 3       | 94   | 测试物理内存是否可用                                                                                                                                         |
| Makefile             | 10    | 7       | 90   |                                                                                                                                                              |
| page_poison.c        | 22    | 13      | 89   | 也是为了处理物理内存错误                                                                                                                                     |
| interval_tree.c      | 16    | 8       | 88   | interval tree for `mapping->i_mmap`                                                                                                                          |
| gup_benchmark.c      | 21    | 0       | 82   |                                                                                                                                                              |
| mmzone.c             | 20    | 14      | 81   | management codes for pgdats, zones and page flagsmanagement codes for pgdats, zones and page flags                                                           |
| vmacache.c           | 18    | 21      | 79   | mm 访问 vma 依赖于此加速                                                                                                                                     |
| msync.c              | 4     | 30      | 74   | msync syscall 同步 The msync() function shall write all modified data to permanent storage locations                                                         |
| kmemleak-test.c      | 14    | 32      | 65   |                                                                                                                                                              |
| quicklist.c          | 18    | 22      | 63   |                                                                                                                                                              |
| failslab.c           | 14    | 2       | 51   |                                                                                                                                                              |
| maccess.c            | 15    | 43      | 49   | @todo 看似简单，其实根本不知道是干什么的                                                                                                                     |
| debug_page_ref.c     | 8     | 1       | 46   |                                                                                                                                                              |
| mmu_context.c        | 7     | 20      | 37   | use_mm 和 unuse_mm 来切换到特定的 mm context,并不是非常清楚会有代码使用这种功能                                                                               |
| rodata_test.c        | 8     | 16      | 33   |                                                                                                                                                              |
| init-mm.c            | 3     | 11      | 26   | 定义用于初始化用途的 mm                                                                                                                                       |
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
    1. 根据的 pgfault 的类型，可能会出现 unshared 写操作，那么就会进行拷贝之类的

5. filemap.c 到处都是使用 PageUptodate 检查一下其中的语义 ? 还有 ClearPageError

1. anon 的 unmap 就是释放内存到 swap 中间，但是当我没有 swap 如何办 ?
    1. 似乎从来都没有找到 swap 没有任何空间，系统的处理办法!
# linux Memory Management

布局: introduction 写一个综述，然后 reference 各个 section 和 subsection 中间的内容。

// TODO 经过讲解 PPT 的内容之后，可以整体框架重做为 物理内存，虚拟内存，page cache 和 swap cache 四个部分来分析

## introduction
大致分析一下和内存相关的 syscall
https://thevivekpandey.github.io/posts/2017-09-25-linux-system-calls.html
1. mmap munmap mremap mprotec brk
2. shmget shmat shmctl
3. membarrier
4. madvise msync mlock munlock mincore
5. mbind set_mempoliyc get_mempolicy


1. 一个干净的地址空间 : virtual memory。
    1. 历史上存在使用段式，现代操作系统使用页式虚实映射，x86 对于段式保持兼容，为了节省物理内存，所以虚实翻译是一个多级的。
    2. 访存需要进行一个 page walk ，原先一次访存，现在需要进行多次，所以存在 TLB 加快速度。为了减少 TLB miss rate，使用 superpage 是一种补救方法。
2. 加载磁盘的内容到内存的时机，linux 使用 page fault 机制，当访问到该页面在加载内存(demand paging)。
2. 哪一个物理页面是空闲，哪一个物理页面正在被使用: buddy allocator
    1. 伙伴系统的分配粒度是 : 2^n * page size 的，但是内核需要更小粒度的分配器，linux 使用 slub slob slab 分配器
    2. 物理内存碎片化会导致即使内存充足，但是 buddy allocator 依据无法分配足够的内存，因此需要 [compaction](#compaction) 机制和 [page reclaim](#page-reclaim) 机制
    3. 当缺乏连续的物理页面，可以通过修改内核 page table 的方法获取虚拟的连续地址，这是通过 vmalloc 实现的。
2. 不同的程序片段的属性不同，代码，数据等不同，linux 使用 vma 来描述。
3. 程序需要访问文件，内存比磁盘快很多，所以需要使用内存作为磁盘的缓存: [page cache](#page-cache)
    1. dirty 缓存什么时候写回磁盘，到底让谁写回到内存。
    4. 如果不加控制，缓存将会占据大量的物理内存，所以需要 page reclaim 机制释放一些内存出来。
4. 当内存不够的时候，利用磁盘进行缓存。
    1. 物理页面可能被多个进程共享，当物理页面被写回磁盘的时候，linux 使用反向映射的机制来告知所有的内存。
    2. 不仅仅可以使用 disk 进行缓存，也可以使用一些异构硬件或者压缩内存的方法
5. 不同进程之间需要进行信息共享，利用内存进行共享是一个高效的方法，linux 支持 Posix 和 sysv 的 shmem。
    1. 父子进程之间由于 fork 也会进行内存共享，使用 cow 机制实现更加高效的拷贝(没有拷贝就是最高效的拷贝)
6. 虚拟机中如何实现内存虚拟化
7. 内存是关键的资源，类似于 docker 之类的容器技术需要利用内核提供的 cgroup 技术来限制一个容器内内存使用。

硬件对于内存的管理提出的挑战：
1. 由于 IO 映射以及 NUMA，内存不是连续的。linux 提供了多个内存模型来解决由于空洞导致的无效 struct page
2. NUMA 系统中间，访问非本地的内存延迟比访问本地的延迟要高，如何让 CPU 尽可能访问本地的内存。
    1. 内存分配器应该确立分配的优先级。
    2. 将经常访问的内存迁移过来。
3. 现在操作系统中间，每一个 core 都存在自己的 local cache，为了让 CPU 尽可能访问自己 local cache 的内容，linux 使用 percpu 机制。
4. 内存是操作系统的运行基础，包括内存的分配，为了解决这个鸡生蛋的问题，linux 使用架构相关的代码探测内存，并且使用 memblock 来实现早期的内存管理。
5. 现代处理器处于性能的考虑，对于访存提出 memory consistency 和 cache coherence 协议，其中 memory consistency 让内核的代码需要特殊注意来避免错误。

克服内核开发人员的疏忽产生的错误:
1. kmemleak @todo
2. kasan @todo
3. vmstat 获取必要的统计数据 https://www.linuxjournal.com/article/8178

克服恶意攻击:
1. stack 随机 ?
2. cow 机制的漏洞是什么 ?
3. 内核的虚拟地址空间 和 用户的虚拟地址空间 互相分离。copy_to_user 和 copy_from_user 如何实现 ?
    1. 内核的物理地址是否也是局限于特定的范围中 ? 否则似乎难以建立 linear 映射。
    2. 猜测，对于 amd64, 内核虚拟地址映射了所有的物理地址，这导致其可以访问任何物理地址，而不会出现 page fault 的情况。
        1. 但是用户的看到的地址空间不仅仅包括内核(线性映射)，也包含自己
        2. 用户进程 syscall 之后，需要切换使用内核的 mm_struct 吗 ?
    3. 对于 x86 32bit 利用 highmem 到底实现了什么内容 ?

那么这些东西具有怎样的联系:(将上面的内容整理成为一个表格)
1. page fault 需要的页面可能是是被 swap 出去的
2. shmem 的内存可能被 swap
3. superpage 需要被纳入到 dirty 和 page claim 中间
4. 进行 page reclaim 可以辅助完成 compaction
5. page reclaim 和 swap 都需要使用反向映射。

现在从一个物理页面的角度将上述的内容串联起来。

> 确立那些是基本要素，然后之间的交互是什么:

| virtual memory | swap | allocator | numa | multicore | hugetlb | page cache | page fault | cgroup | shmem | page reclaim | migrate |
|----------------|------|-----------|------|-----------|---------|------------|------------|--------|-------|--------------|---------|
| virtual memory |
| swap           |
| allocator      |
| numa           |

总结内容主要来自于 lwn [^3], (几本书)，wowotech ，几个试验

#### copy_from_user
从这里看，copy_from_user 和 copy_to_user 并不是检查 vma 的方法，而是和架构实现息息相关, TODO
https://stackoverflow.com/questions/8265657/how-does-copy-from-user-from-the-linux-kernel-work-internally


```c
ssize_t cdev_fops_write(struct file *flip, const char __user *ubuf,
                        size_t count, loff_t *f_pos)
{
    unsigned int *kbuf;
    copy_from_user(kbuf, ubuf, count);
    printk(KERN_INFO "Data: %d",*kbuf);
}
```
ubuf 用户提供的指针，在执行该函数的时候，当前的进程地址空间就是该用户的，所以使用 ubuf 并不需要什么奇怪的装换。


1. copy_from_user 和 copy_to_user


```c
size_t iov_iter_copy_from_user_atomic(struct page *page,
    struct iov_iter *i, unsigned long offset, size_t bytes)
{
  char *kaddr = kmap_atomic(page), *p = kaddr + offset;
  if (unlikely(!page_copy_sane(page, offset, bytes))) {
    kunmap_atomic(kaddr);
    return 0;
  }
  if (unlikely(iov_iter_is_pipe(i) || iov_iter_is_discard(i))) {
    kunmap_atomic(kaddr);
    WARN_ON(1);
    return 0;
  }
  iterate_all_kinds(i, bytes, v,
    copyin((p += v.iov_len) - v.iov_len, v.iov_base, v.iov_len),
    memcpy_from_page((p += v.bv_len) - v.bv_len, v.bv_page,
         v.bv_offset, v.bv_len),
    memcpy((p += v.iov_len) - v.iov_len, v.iov_base, v.iov_len)
  )
  kunmap_atomic(kaddr);
  return bytes;
}
EXPORT_SYMBOL(iov_iter_copy_from_user_atomic);
```

## pmem
DAX 设置 : 到时候在分析吧!
1. https://www.intel.co.uk/content/www/uk/en/it-management/cloud-analytic-hub/pmem-next-generation-storage.html
2. https://nvdimm.wiki.kernel.org/
3. https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/storage_administration_guide/ch-persistent-memory-nvdimms

目前观察到的 generic_file_read_iter 和 file_operations::mmap 的内容对于 DAX 区分对待的，但是内容远远不该如此，不仅仅可以越过 page cache 机制，而且 page reclaim 全部可以跳过。

## lock
- [ ] mm_take_all_locks

## mremap

## dmapool
https://lwn.net/Articles/69402/

Some very obscure driver bugs have been traced down to cache coherency problems with structure fields adjacent to small DMA areas. [^17]
> DMA 为什么会导致附近的内存的 cache coherency 的问题 ?

- [ ] dma_pool_create() - Creates a pool of consistent memory blocks, for dma.

- [ ] https://www.kernel.org/doc/html/latest/driver-api/dmaengine/index.html#dmaengine-documentation
- [ ] https://www.kernel.org/doc/html/latest/core-api/index.html#memory-management
- [ ] https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt

## hmm
Provide infrastructure and helpers to integrate non-conventional memory (device memory like GPU on board memory) into regular kernel path, with the cornerstone of this being specialized struct page for such memory.
HMM also provides optional helpers for SVM (Share Virtual Memory) [^19]

## zsmalloc
slub 分配器处理 size > page_size / 2 会浪费非常多的内容，zsmalloc 就是为了解决这个问题 [^20]

## z3fold
z3fold is a special purpose allocator for storing compressed pages. [^23]

## zud
和 z3fold 类似的东西

## msync
存在系统调用 msync，实现应该很简单吧!

## mpage
fs/mpage.c : 为毛是需要使用这个机制 ? 猜测其中的机制是为了实现

```c
static int ext2_readpage(struct file *file, struct page *page)
{
  return mpage_readpage(page, ext2_get_block);
}

static int
ext2_readpages(struct file *file, struct address_space *mapping,
    struct list_head *pages, unsigned nr_pages)
{
  return mpage_readpages(mapping, pages, nr_pages, ext2_get_block);
}

/*
 * This is the worker routine which does all the work of mapping the disk
 * blocks and constructs largest possible bios, submits them for IO if the
 * blocks are not contiguous on the disk.
 *
 * We pass a buffer_head back and forth and use its buffer_mapped() flag to
 * represent the validity of its disk mapping and to decide when to do the next
 * get_block() call.
 */
static struct bio *do_mpage_readpage(struct mpage_readpage_args *args)
```
> 无论是 ext2_readpage 还是 ext2_readpages 最后都是走到 do_mpage_readpage

## memblock


## profiler
用户层的:
1. https://github.com/KDE/heaptrack
2. https://github.com/koute/memory-profiler

## mprotect
[changing memory protection](https://perception-point.io/changing-memory-protection-in-an-arbitrary-process/)

> - The `vm_area_struct` contains the field `vm_flags` which represents the protection flags of the memory region in an architecture-independent manner, and `vm_page_prot` which represents it in an architecture-dependent manner.

> After some reading and digging into the kernel code, we detected the most essential work needed to really change the protection of a memory region:
> - Change the field `vm_flags` to the desired protection.
> - Call the function `vma_set_page_prot` to update the field vm_page_prot according to the vm_flags field.
> - Call the function `change_protection` to actually update the protection bits in the page table.

check the code in `mprotect.c:mprotect_fixup`, above claim can be verified

- except what three steps meantions above, mprotect also splitting and joining memory regions by their protection flags
## vmalloc
[TO BE CONTINUE](https://www.cnblogs.com/LoyenWang/p/11965787.html)

## mincore

## pageblock
https://richardweiyang-2.gitbook.io/kernel-exploring/00-memory_a_bottom_up_view/13-physical-layer-partition

- pgdat
- zone
- memory_block : 热插拔
- mem_section
- pageblock
- page

## user address space
/home/maritns3/core/vn/hack/lab/proc-self-maps/main.c
```plain
00400000-00401000 r--p 00000000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00401000-00402000 r-xp 00001000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00402000-00403000 r--p 00002000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00403000-00404000 r--p 00002000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
00404000-00405000 rw-p 00003000 103:02 13252000                          /home/maritns3/core/vn/hack/lab/proc-self-maps/main.out
007fa000-0081b000 rw-p 00000000 00:00 0                                  [heap]
7fd3e0f16000-7fd3e0f19000 rw-p 00000000 00:00 0
7fd3e0f19000-7fd3e0f3e000 r--p 00000000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e0f3e000-7fd3e10b6000 r-xp 00025000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e10b6000-7fd3e1100000 r--p 0019d000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1100000-7fd3e1101000 ---p 001e7000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1101000-7fd3e1104000 r--p 001e7000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1104000-7fd3e1107000 rw-p 001ea000 103:02 4982896                   /usr/lib/x86_64-linux-gnu/libc-2.31.so
7fd3e1107000-7fd3e110b000 rw-p 00000000 00:00 0
7fd3e110b000-7fd3e111a000 r--p 00000000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e111a000-7fd3e11c1000 r-xp 0000f000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e11c1000-7fd3e1258000 r--p 000b6000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e1258000-7fd3e1259000 r--p 0014c000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e1259000-7fd3e125a000 rw-p 0014d000 103:02 4982898                   /usr/lib/x86_64-linux-gnu/libm-2.31.so
7fd3e125a000-7fd3e125c000 rw-p 00000000 00:00 0
7fd3e1273000-7fd3e1274000 r--p 00000000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e1274000-7fd3e1297000 r-xp 00001000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e1297000-7fd3e129f000 r--p 00024000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e12a0000-7fd3e12a1000 r--p 0002c000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e12a1000-7fd3e12a2000 rw-p 0002d000 103:02 4982891                   /usr/lib/x86_64-linux-gnu/ld-2.31.so
7fd3e12a2000-7fd3e12a3000 rw-p 00000000 00:00 0
7ffcb622f000-7ffcb6250000 rw-p 00000000 00:00 0                          [stack]
7ffcb6374000-7ffcb6377000 r--p 00000000 00:00 0                          [vvar]
7ffcb6377000-7ffcb6378000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 --xp 00000000 00:00 0                  [vsyscall]
```

- [ ] why there are more section for `main.out` than expected ? There are five entry whose path is `proc-self-maps/main.out`.
  - [ ] check the main.out with section header
  - [ ] why two entry has same offset ? third and forth

- [x] why text segment start at 0x40000 ?
  - [ ] read this : https://stackoverflow.com/questions/39689516/why-is-address-0x400000-chosen-as-a-start-of-text-segment-in-x86-64-abi

- [ ] why some area with no names ?

- [x] check inode
  - [ ] https://unix.stackexchange.com/questions/35292/quickly-find-which-files-belongs-to-a-specific-inode-number
    - In fact, we can find file name with inode by checking file one by one, but **debufs** impressed my

## kaslr
- [ ] https://unix.stackexchange.com/questions/469016/do-the-virtual-address-spaces-of-all-the-processes-have-the-same-content-in-thei
  - [ ] https://en.wikipedia.org/wiki/Kernel_page-table_isolation
  - [ ] https://lwn.net/Articles/738975/

- [ ] https://bneuburg.github.io/
  - [ ] he has writen three post about it

- [ ] https://lwn.net/Articles/569635/


- [ ] Sometimes /proc/$pid/maps show text address start at 0x400000, sometimes 0x055555555xxx,
maybe because of user space address randomization
    - [  ] https://www.theurbanpenguin.com/aslr-address-space-layout-randomization/

## CXL
- CXL 2.0 的基本概念: https://www.zhihu.com/question/531720207/answer/2521601976
- 显存为什么不能当内存使？内存、Cache 和 Cache 一致性: https://zhuanlan.zhihu.com/p/63494668

## 基本的测试方法

### stress-ng
```sh
cgexec -g memory:mem stress-ng --vm-bytes 150M --vm-keep --vm 1
```
- --vm N : worker 的数量
- --vm-keep
- --vm-bytes : 文档上写的是 per worker 的样子，但是实际上并不是这样的

```c
stress-ng --memrate 1 --memrate-wr 200 --memrate-rd 100 --memrate-bytes 6000M -v
```
似乎这个内存写完了就结束了，而且看上去 memrate-wr 的限制好像是根本就没有起任何作用。

[^1]: [lwn : Huge pages part 1 (Introduction)](https://lwn.net/Articles/374424/)
[^2]: [lwn : An end to high memory?](https://lwn.net/Articles/813201/)
[^3]: [lwn#memory management](https://lwn.net/Kernel/Index/#Memory_management)
[^5]: [Complete virtual memory map of x86_64](https://www.kernel.org/doc/html/latest/x86/x86_64/mm.html)
[^8]: [kernel doc : pin_user_pages() and related calls](https://www.kernel.org/doc/html/latest/core-api/pin_user_pages.html)
[^9]: [lwn : Explicit pinning of user-space pages](https://lwn.net/Articles/807108/)
[^13]: [lwn : Smarter shrinkers](https://lwn.net/Articles/550463/)
[^17]: [stackoverflow : Why do we need DMA pool ?](https://stackoverflow.com/questions/60574054/why-do-we-need-dma-pool)
[^18]: [kernel doc : Kernel Memory Leak Detector](https://www.kernel.org/doc/html/latest/dev-tools/kmemleak.html)
[^19]: [kernel doc : Heterogeneous Memory Management (HMM)](https://www.kernel.org/doc/html/latest/vm/hmm.html)
[^20]: [lwn : The zsmalloc allocator](https://lwn.net/Articles/477067/)
[^21]: [lwn : A reworked contiguous memory allocator](https://lwn.net/Articles/447405/)
[^22]: [lwn : A deep dive into dma](https://lwn.net/Articles/486301/)
[^23]: [kernel doc : z3fold](https://www.kernel.org/doc/html/latest/vm/z3fold.html)
[^29]: https://my.oschina.net/u/3857782/blog/1854548
