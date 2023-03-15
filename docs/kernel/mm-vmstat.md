# 观测 Linux 的内存系统

表格中的 `含义` 栏目如果为空，表示同上。

## 问题
- [ ] node_page_state 和 lruvec_page_state 是什么关系？

### free -m 都表示的是什么意思
```txt
       total  Total installed memory (MemTotal and SwapTotal in /proc/meminfo)

       used   Used memory (calculated as total - free - buffers - cache)

       free   Unused memory (MemFree and SwapFree in /proc/meminfo)

       shared Memory used (mostly) by tmpfs (Shmem in /proc/meminfo)

       buffers
              Memory used by kernel buffers (Buffers in /proc/meminfo)

       cache  Memory used by the page cache and slabs (Cached and SReclaimable in /proc/meminfo)

       buff/cache
              Sum of buffers and cache

       available
              Estimation of how much memory is available for starting new applications, without swapping. Unlike the data provided by the cache or free fields,  this  field  takes
              into  account page cache and also that not all reclaimable memory slabs will be reclaimed due to items being in use (MemAvailable in /proc/meminfo, available on ker‐
              nels 3.14, emulated on kernels 2.6.27+, otherwise the same as free)

```

让内核迅速的收缩大页:
```txt
➜  ~ free -m
               total        used        free      shared  buff/cache   available
Mem:            7962        2596         606           6        4759        4908
Swap:           2104          39        2065

➜  ~ free -m
               total        used        free      shared  buff/cache   available
Mem:            7962        7592         316           1          53         154
Swap:           2104         126        1978
```
## proc/pid/smaps

- 和 proc/pid/maps 的有啥区别？

## proc/pid/numa_maps

代码:
```c
static const struct mm_walk_ops show_numa_ops = {
	.hugetlb_entry = gather_hugetlb_stats,
	.pmd_entry = gather_pte_stats,
};
```
这个 walk 是分析每一个真正被使用的物理页面的，但是如果一个文件被反复映射，这个数值就不会正确的统计。

调用路径:
```txt
@[
    gather_pte_stats+5
    walk_pgd_range+1263
    __walk_page_range+400
    walk_page_vma+90
    show_numa_map+286
    seq_read_iter+711
    seq_read+167
    vfs_read+163
    ksys_read+99
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 42
```
使用 ./code/numa_maps.c 来测试:
```txt
00400000 default file=/home/martins3/core/vn/docs/kernel/code/a.out mapped=1 active=0 N0=1 kernelpagesize_kB=4
00401000 default file=/home/martins3/core/vn/docs/kernel/code/a.out mapped=1 active=0 N0=1 kernelpagesize_kB=4
00402000 default file=/home/martins3/core/vn/docs/kernel/code/a.out mapped=1 active=0 N0=1 kernelpagesize_kB=4
00403000 default file=/home/martins3/core/vn/docs/kernel/code/a.out anon=1 dirty=1 active=0 N0=1 kernelpagesize_kB=4
00404000 default file=/home/martins3/core/vn/docs/kernel/code/a.out anon=1 dirty=1 active=0 N0=1 kernelpagesize_kB=4
01d3f000 default heap anon=1 dirty=1 active=0 N0=1 kernelpagesize_kB=4
7fd249e00000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd2ca000000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd34a200000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd3ca400000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd44a600000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd4ca800000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd54aa00000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd5cac00000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd64ae00000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd6cb000000 default file=/home/martins3/hack/vm/windows8.img mapped=524288 mapmax=10 active=0 N0=524288 kernelpagesize_kB=4
7fd74b200000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6 mapped=40 mapmax=152 active=0 N0=40 kernelpagesize_kB=4
7fd74b228000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6 mapped=228 mapmax=158 active=0 N0=228 kernelpagesize_kB=4
7fd74b39e000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6 mapped=16 mapmax=158 active=0 N0=16 kernelpagesize_kB=4
7fd74b3f6000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6 anon=4 dirty=4 active=0 N0=4 kernelpagesize_kB=4
7fd74b3fa000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/libc.so.6 anon=2 dirty=2 active=0 N0=2 kernelpagesize_kB=4
7fd74b3fc000 default anon=5 dirty=5 active=0 N0=5 kernelpagesize_kB=4
7fd74b5c0000 default anon=3 dirty=3 active=0 N0=3 kernelpagesize_kB=4
7fd74b5c5000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/ld-linux-x86-64.so.2 mapped=2 mapmax=152 active=0 N0=2 kernelpagesize_kB=4
7fd74b5c7000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/ld-linux-x86-64.so.2 mapped=39 mapmax=157 active=0 N0=39 kernelpagesize_kB=4
7fd74b5ee000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/ld-linux-x86-64.so.2 mapped=12 mapmax=152 active=0 N0=12 kernelpagesize_kB=4
7fd74b5fa000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/ld-linux-x86-64.so.2 anon=2 dirty=2 active=0 N0=2 kernelpagesize_kB=4
7fd74b5fc000 default file=/nix/store/2j8jqmnd9l7plihhf713yf291c9vyqjm-glibc-2.35-224/lib/ld-linux-x86-64.so.2 anon=2 dirty=2 active=0 N0=2 kernelpagesize_kB=4
7ffdc21c5000 default stack anon=6 dirty=6 active=0 N0=6 kernelpagesize_kB=4
7ffdc21fa000 default
7ffdc21fe000 default
```

numad 就是通过这个来实现的。

## /proc/pid/stat

## /proc/meminfo

| 名称            | 数值           | 含义                                                                                                                                                                                |
|-----------------|----------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| MemTotal        | 32690904 kB    | 所有的物理页面，调用 totalram_pages_add ，在启动的时候通过 memblock 获取，在 virtio mem ，virtio balloon 以及 memory hotplug 的时候修改。                                           |
| MemFree         | 25368728 kB    |                                                                                                                                                                                     |
| MemAvailable    | 28847852 kB    |                                                                                                                                                                                     |
| Buffers         | 251576 kB      | buffer 就是 inode cache 之类的，如果进行一次 disk 使用统计，就很刺激了，https://stackoverflow.com/questions/6345020/what-is-the-difference-between-buffer-and-cache-memory-in-linux |
| Cached          | 3339012 kB     |                                                                                                                                                                                     |
| SwapCached      | 0 kB           |                                                                                                                                                                                     |
| Active          | 2638452 kB     |                                                                                                                                                                                     |
| Inactive        | 3996836 kB     |                                                                                                                                                                                     |
| Active(anon)    | 68656 kB       |                                                                                                                                                                                     |
| Inactive(anon)  | 2896668 kB     |                                                                                                                                                                                     |
| Active(file)    | 2569796 kB     |                                                                                                                                                                                     |
| Inactive(file)  | 1100168 kB     |                                                                                                                                                                                     |
| Unevictable     | 3408 kB        |                                                                                                                                                                                     |
| Mlocked         | 1872 kB        |                                                                                                                                                                                     |
| SwapTotal       | 0 kB           |                                                                                                                                                                                     |
| SwapFree        | 0 kB           |                                                                                                                                                                                     |
| Zswap           | 0 kB           |                                                                                                                                                                                     |
| Zswapped        | 0 kB           |                                                                                                                                                                                     |
| Dirty           | 104 kB         |                                                                                                                                                                                     |
| Writeback       | 0 kB           |                                                                                                                                                                                     |
| AnonPages       | 3040592 kB     |                                                                                                                                                                                     |
| Mapped          | 341680 kB      |                                                                                                                                                                                     |
| Shmem           | 10912 kB       |                                                                                                                                                                                     |
| KReclaimable    | 274160 kB      |                                                                                                                                                                                     |
| Slab            | 412500 kB      |                                                                                                                                                                                     |
| SReclaimable    | 274160 kB      |                                                                                                                                                                                     |
| SUnreclaim      | 138340 kB      |                                                                                                                                                                                     |
| KernelStack     | 16304 kB       |                                                                                                                                                                                     |
| PageTables      | 23136 kB       |                                                                                                                                                                                     |
| NFS_Unstable    | 0 kB           |                                                                                                                                                                                     |
| Bounce          | 0 kB           |                                                                                                                                                                                     |
| WritebackTmp    | 0 kB           |                                                                                                                                                                                     |
| CommitLimit     | 16345452 kB    | 可以 overmmit 的数量                                                                                                                                                                                     |
| Committed_AS    | 12958700 kB    | 已经提交的量，如果 mmap 是 MAP_NORESERVE 的，那么将不会统计在此处                                                                                                                                                                                     |
| VmallocTotal    | 34359738367 kB |                                                                                                                                                                                     |
| VmallocUsed     | 102404 kB      |                                                                                                                                                                                     |
| VmallocChunk    | 0 kB           |                                                                                                                                                                                     |
| Percpu          | 114240 kB      |                                                                                                                                                                                     |
| AnonHugePages   | 301056 kB      |                                                                                                                                                                                     |
| ShmemHugePages  | 0 kB           |                                                                                                                                                                                     |
| ShmemPmdMapped  | 0 kB           |                                                                                                                                                                                     |
| FileHugePages   | 0 kB           |                                                                                                                                                                                     |
| FilePmdMapped   | 0 kB           |                                                                                                                                                                                     |
| CmaTotal        | 0 kB           |                                                                                                                                                                                     |
| CmaFree         | 0 kB           |                                                                                                                                                                                     |
| HugePages_Total | 0              |                                                                                                                                                                                     |
| HugePages_Free  | 0              |                                                                                                                                                                                     |
| HugePages_Rsvd  | 0              |                                                                                                                                                                                     |
| HugePages_Surp  | 0              |                                                                                                                                                                                     |
| Hugepagesize    | 2048 kB        |                                                                                                                                                                                     |
| Hugetlb         | 0 kB           |                                                                                                                                                                                     |
| DirectMap4k     | 294756 kB      |                                                                                                                                                                                     |
| DirectMap2M     | 4947968 kB     |                                                                                                                                                                                     |
| DirectMap1G     | 30408704 kB    |                                                                                                                                                                                     |

```txt
$ free -m
              total        used        free      shared  buff/cache   available
Mem:        5527225     5524113        1559          29        1552         135
Swap:             0           0           0
```
- [ ] shared 是个什么概念，是 used 的一部分吗?

这里， available 比 free 和 buffer/cache 少
1. 因为 available 表示创建新的程序可以使用的内存，而内核会将部分内存预留下来

- si_mem_available


从 htop 中观测，显示 swap 只是使用为: 1.44G/9.77G
而实际上观测到：
```txt
➜  ~ grep Swap /proc/meminfo
SwapCached:      2161984 kB
SwapTotal:      10239996 kB
SwapFree:        5998016 kB
```
所以，htop 是将 SwapCached 不算的:

## /proc/vmstat

- vmstat_start
  - global_zone_page_state
  - global_numa_event_state
  - global_node_page_state_pages
```c
/*
 * Zone and node-based page accounting with per cpu differentials.
 */
extern atomic_long_t vm_zone_stat[NR_VM_ZONE_STAT_ITEMS];
extern atomic_long_t vm_node_stat[NR_VM_NODE_STAT_ITEMS];
extern atomic_long_t vm_numa_event[NR_VM_NUMA_EVENT_ITEMS];
```
| 名称                           | 数值     | 说明                                                             |
|--------------------------------|----------|------------------------------------------------------------------|
| nr_free_pages                  | 3691871  | 伙伴系统中持有空闲页面，不包含大页                               |
| nr_zone_inactive_anon          | 2316004  | 一个 node 中各种类型的 zone 的 lru 的统计的总和，每一个 zone 具体统计在 @todo |
| nr_zone_active_anon            | 105248   |                                                                  |
| nr_zone_inactive_file          | 621102   |                                                                  |
| nr_zone_active_file            | 1223692  |                                                                  |
| nr_zone_unevictable            | 852      |                                                                  |
| nr_zone_write_pending          | 53       |                                                                  |
| nr_mlock                       | 468      | mlock(2)                                                         |
| nr_bounce                      | 0        | 和 highmem 相关，不用关注                                        |
| nr_zspages                     | 0        | zsmalloc                                                         |
| nr_free_cma                    | 0        | 统计 buddy 从 CMA 中借用的内存                                   |
| numa_hit                       | 39288712 | numa 远程访问的相关统计                                          |
| numa_miss                      | 0        |                                                                  |
| numa_foreign                   | 0        |                                                                  |
| numa_interleave                | 1781     |                                                                  |
| numa_local                     | 39288712 |                                                                  |
| numa_other                     | 0        |                                                                  |
| nr_inactive_anon               | 2316004  | 所有的 node 的数据合并结果，如果只有一个 Node，那么数据和 nr_zone_\* 相同                                  |
| nr_active_anon                 | 105248   |                                                                  |
| nr_inactive_file               | 621102   |                                                                  |
| nr_active_file                 | 1223692  |                                                                  |
| nr_unevictable                 | 852      |                                                                  |
| nr_slab_reclaimable            | 100760   | @todo 不知道在什么                                                                  |
| nr_slab_unreclaimable          | 39601    |                                                                   |
| nr_isolated_anon               | 0        | reclaim_clean_pages_from_list @todo 关注这个函数的调用路径                                                                 |
| nr_isolated_file               | 0        |                                                                  |
| workingset_nodes               | 0        | @todo workingset.c 相关                                                                  |
| workingset_refault_anon        | 0        |                                                                  |
| workingset_refault_file        | 0        |                                                                  |
| workingset_activate_anon       | 0        |                                                                  |
| workingset_activate_file       | 0        |                                                                  |
| workingset_restore_anon        | 0        |                                                                  |
| workingset_restore_file        | 0        |                                                                  |
| workingset_nodereclaim         | 0        |                                                                  |
| nr_anon_pages                  | 2439954  |                                                                  |
| nr_mapped                      | 90232    |                                                                  |
| nr_file_pages                  | 1825096  |                                                                  |
| nr_dirty                       | 53       |                                                                  |
| nr_writeback                   | 0        |                                                                  |
| nr_writeback_temp              | 0        |                                                                  |
| nr_shmem                       | 2755     |                                                                  |
| nr_shmem_hugepages             | 0        |                                                                  |
| nr_shmem_pmdmapped             | 0        |                                                                  |
| nr_file_hugepages              | 0        |                                                                  |
| nr_file_pmdmapped              | 0        |                                                                  |
| nr_anon_transparent_hugepages  | 365      |                                                                  |
| nr_vmscan_write                | 0        |                                                                  |
| nr_vmscan_immediate_reclaim    | 0        |                                                                  |
| nr_dirtied                     | 1096249  |                                                                  |
| nr_written                     | 1073014  |                                                                  |
| nr_throttled_written           | 0        |                                                                  |
| nr_kernel_misc_reclaimable     | 0        | @todo NR_KERNEL_MISC_RECLAIMABLE 又是之见过 reference ，但是没有修改的地方                                                                  |
| nr_foll_pin_acquired           | 0        |                                                                  |
| nr_foll_pin_released           | 0        |                                                                  |
| nr_kernel_stack                | 17072    |                                                                  |
| nr_page_table_pages            | 10630    |                                                                  |
| nr_swapcached                  | 0        |                                                                  |
| nr_dirty_threshold             | 1093840  |                                                                  |
| nr_dirty_background_threshold  | 546252   |                                                                  |
| pgpgin                         | 4671361  |                                                                  |
| pgpgout                        | 4567364  |                                                                  |
| pswpin                         | 0        |                                                                  |
| pswpout                        | 0        |                                                                  |
| pgalloc_dma                    | 0        |                                                                  |
| pgalloc_dma32                  | 12       |                                                                  |
| pgalloc_normal                 | 39296096 |                                                                  |
| pgalloc_movable                | 0        |                                                                  |
| allocstall_dma                 | 0        |                                                                  |
| allocstall_dma32               | 0        |                                                                  |
| allocstall_normal              | 0        |                                                                  |
| allocstall_movable             | 0        |                                                                  |
| pgskip_dma                     | 0        |                                                                  |
| pgskip_dma32                   | 0        |                                                                  |
| pgskip_normal                  | 0        |                                                                  |
| pgskip_movable                 | 0        |                                                                  |
| pgfree                         | 43277874 |                                                                  |
| pgactivate                     | 2122820  |                                                                  |
| pgdeactivate                   | 0        |                                                                  |
| pglazyfree                     | 111434   |                                                                  |
| pgfault                        | 47802452 |                                                                  |
| pgmajfault                     | 10494    |                                                                  |
| pglazyfreed                    | 0        |                                                                  |
| pgrefill                       | 0        |                                                                  |
| pgreuse                        | 11553684 |                                                                  |
| pgsteal_kswapd                 | 0        |                                                                  |
| pgsteal_direct                 | 0        |                                                                  |
| pgdemote_kswapd                | 0        |                                                                  |
| pgdemote_direct                | 0        |                                                                  |
| pgscan_kswapd                  | 0        |                                                                  |
| pgscan_direct                  | 0        |                                                                  |
| pgscan_direct_throttle         | 0        |                                                                  |
| pgscan_anon                    | 0        |                                                                  |
| pgscan_file                    | 0        |                                                                  |
| pgsteal_anon                   | 0        |                                                                  |
| pgsteal_file                   | 0        |                                                                  |
| zone_reclaim_failed            | 0        |                                                                  |
| pginodesteal                   | 0        |                                                                  |
| slabs_scanned                  | 0        |                                                                  |
| kswapd_inodesteal              | 0        |                                                                  |
| kswapd_low_wmark_hit_quickly   | 0        |                                                                  |
| kswapd_high_wmark_hit_quickly  | 0        |                                                                  |
| pageoutrun                     | 0        |                                                                  |
| pgrotated                      | 5        |                                                                  |
| drop_pagecache                 | 0        |                                                                  |
| drop_slab                      | 0        |                                                                  |
| oom_kill                       | 0        |                                                                  |
| pgmigrate_success              | 0        |                                                                  |
| pgmigrate_fail                 | 0        |                                                                  |
| thp_migration_success          | 0        |                                                                  |
| thp_migration_fail             | 0        |                                                                  |
| thp_migration_split            | 0        |                                                                  |
| compact_migrate_scanned        | 0        |                                                                  |
| compact_free_scanned           | 0        |                                                                  |
| compact_isolated               | 0        |                                                                  |
| compact_stall                  | 0        |                                                                  |
| compact_fail                   | 0        |                                                                  |
| compact_success                | 0        |                                                                  |
| compact_daemon_wake            | 0        |                                                                  |
| compact_daemon_migrate_scanned | 0        |                                                                  |
| compact_daemon_free_scanned    | 0        |                                                                  |
| htlb_buddy_alloc_success       | 0        |                                                                  |
| htlb_buddy_alloc_fail          | 0        |                                                                  |
| cma_alloc_success              | 0        |                                                                  |
| cma_alloc_fail                 | 0        |                                                                  |
| unevictable_pgs_culled         | 852      |                                                                  |
| unevictable_pgs_scanned        | 0        |                                                                  |
| unevictable_pgs_rescued        | 0        |                                                                  |
| unevictable_pgs_mlocked        | 468      |                                                                  |
| unevictable_pgs_munlocked      | 0        |                                                                  |
| unevictable_pgs_cleared        | 0        |                                                                  |
| unevictable_pgs_stranded       | 0        |                                                                  |
| thp_fault_alloc                | 2035     |                                                                  |
| thp_fault_fallback             | 0        |                                                                  |
| thp_fault_fallback_charge      | 0        |                                                                  |
| thp_collapse_alloc             | 3        |                                                                  |
| thp_collapse_alloc_failed      | 0        |                                                                  |
| thp_file_alloc                 | 0        |                                                                  |
| thp_file_fallback              | 0        |                                                                  |
| thp_file_fallback_charge       | 0        |                                                                  |
| thp_file_mapped                | 0        |                                                                  |
| thp_split_page                 | 0        |                                                                  |
| thp_split_page_failed          | 0        |                                                                  |
| thp_deferred_split_page        | 7        |                                                                  |
| thp_split_pmd                  | 15       |                                                                  |
| thp_scan_exceed_none_pte       | 0        |                                                                  |
| thp_scan_exceed_swap_pte       | 0        |                                                                  |
| thp_scan_exceed_share_pte      | 0        |                                                                  |
| thp_split_pud                  | 0        |                                                                  |
| thp_zero_page_alloc            | 1        |                                                                  |
| thp_zero_page_alloc_failed     | 0        |                                                                  |
| thp_swpout                     | 0        |                                                                  |
| thp_swpout_fallback            | 0        |                                                                  |
| balloon_inflate                | 0        |                                                                  |
| balloon_deflate                | 0        |                                                                  |
| balloon_migrate                | 0        |                                                                  |
| swap_ra                        | 0        |                                                                  |
| swap_ra_hit                    | 0        |                                                                  |
| ksm_swpin_copy                 | 0        |                                                                  |
| cow_ksm                        | 0        |                                                                  |
| zswpin                         | 0        |                                                                  |
| zswpout                        | 0        |                                                                  |
| direct_map_level2_splits       | 134      |                                                                  |
| direct_map_level3_splits       | 1        |                                                                  |
| nr_unstable                    | 0        |                                                                  |

```txt
#0  refresh_cpu_vm_stats (do_pagesets=do_pagesets@entry=true) at mm/vmstat.c:807
#1  0xffffffff812ba71e in vmstat_update (w=<optimized out>) at mm/vmstat.c:1931
#2  0xffffffff8112bd34 in process_one_work (worker=worker@entry=0xffff8881001320c0, work=0xffff88813bc28320) at kernel/workqueue.c:2289
#3  0xffffffff8112bf48 in worker_thread (__worker=0xffff8881001320c0) at kernel/workqueue.c:2436
#4  0xffffffff81133850 in kthread (_create=0xffff888100133040) at kernel/kthread.c:376
#5  0xffffffff81001a6f in ret_from_fork () at arch/x86/entry/entry_64.S:306
#6  0x0000000000000000 in ?? ()
```

## /proc/slabinfo

## /proc/zoneinfo

- [ ] 从这里看，存在一个 zone 居然是 device

## /proc/pagetypeinfo

## /proc/buddyinfo

## /proc/sys/vm

### hugepages 相关的
- nr_hugepages
- nr_overcommit_hugepages

## /sys/kernel/mm

- hugepages/

### hugepages

## /sys/devices/system/node

对应的代码
```c
typedef struct { DECLARE_BITMAP(bits, MAX_NUMNODES); } nodemask_t;
nodemask_t node_states[NR_NODE_STATES] __read_mostly;
```

- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
  - MemAvailable  和 MemFree 是什么关系
- https://lwn.net/Articles/178850/

- [ ] /proc/meminfo 的 HugePages_Rsvd 的含义是什么 ? 下面的代码，为什么不会导致 HugePages_Free 减少，而是 HugePages_Rsvd 增加
```c
#include <stdio.h>
#include <stdlib.h> // malloc
#include <sys/mman.h>
#include <asm/mman.h>
#include <sys/types.h>
#include <unistd.h> // sleep

int main(int argc, char *argv[]) {
  size_t SIZE_2M = 1 << 21;
  char *addr = (char *)mmap(0, SIZE_2M, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
  if (addr == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
  for (int i = 0; i < SIZE_2M; ++i) {
    addr[i] = 'a';
  }
  sleep(100);
  return 0;
}
```



## 可以调查的函数
- int hugetlb_report_node_meminfo(int, char *);
- void hugetlb_report_meminfo(struct seq_file *);
- void hugetlb_show_meminfo(void);

## 忽然发现这是一个大主题
- MIGRATE_PCPTYPES

这两个是做啥用的:
- zone_batchsize
- zone_highsize

## /proc/meminfo 中这些是啥
```txt
DirectMap4k:       36680 kB
DirectMap2M:     3108864 kB
DirectMap1G:    11534336 kB
```

- sudo cat /proc/pagetypeinfo
```txt
Page block order: 9
Pages per block:  512

Free pages count per migrate type at order       0      1      2      3      4      5      6      7      8      9     10
Node    0, zone      DMA, type    Unmovable      0      0      0      0      0      0      0      0      1      0      0
Node    0, zone      DMA, type      Movable      0      0      0      0      0      0      0      0      0      1      3
Node    0, zone      DMA, type  Reclaimable      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone      DMA, type   HighAtomic      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone      DMA, type          CMA      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone      DMA, type      Isolate      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone    DMA32, type    Unmovable     62    106    153    134    119     72     44     15      5      0      0
Node    0, zone    DMA32, type      Movable    417    197     98     86     84     90     64     65     71     90    137
Node    0, zone    DMA32, type  Reclaimable     41     57     62     61     64     59     48     35     27      0      0
Node    0, zone    DMA32, type   HighAtomic      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone    DMA32, type          CMA      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone    DMA32, type      Isolate      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type    Unmovable     53    105    830    360     80     48     52     30     18     14     16
Node    0, zone   Normal, type      Movable >100000  93493  41359  17188   5937   3527   2269   1339    842   1269   1170
Node    0, zone   Normal, type  Reclaimable     88     86    457      3    198    466    360    245    106      1      0
Node    0, zone   Normal, type   HighAtomic      0      0     16     15      6      3      1      0      0      0      0
Node    0, zone   Normal, type          CMA      0      0      0      0      0      0      0      0      0      0      0
Node    0, zone   Normal, type      Isolate      0      0      0      0      0      0      0      0      0      0      0

Number of blocks type     Unmovable      Movable  Reclaimable   HighAtomic          CMA      Isolate
Node 0, zone      DMA            1            7            0            0            0            0
Node 0, zone    DMA32           25         1466           37            0            0            0
Node 0, zone   Normal          298        10165          288            1            0            0
```
- [ ] 为什么 Movable 中有那么多 order=10 的页

## /proc/meminfo 中关于 hugepage 的统计真奇怪啊

## 3. zone 和 node 中间都含有统计信息，分别统计什么
1. 这些统计信息是通过什么接口提供给用户程序的，或者内核如何使用它们?
2. zone 和 node 统计内容有什么侧重?

node 统计信息定义
```txt
	/* Per-node vmstats */
	struct per_cpu_nodestat __percpu *per_cpu_nodestats;
	atomic_long_t		vm_stat[NR_VM_NODE_STAT_ITEMS];
} pg_data_t;

struct zone {
...
	/* Zone statistics */
	atomic_long_t		vm_stat[NR_VM_ZONE_STAT_ITEMS];
	atomic_long_t		vm_numa_stat[NR_VM_NUMA_STAT_ITEMS];
} ____cacheline_internodealigned_in_smp;

struct per_cpu_nodestat {
	s8 stat_threshold;
	s8 vm_node_stat_diff[NR_VM_NODE_STAT_ITEMS];
};
```



```txt
AnonHugePages:    798720 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
CmaTotal:              0 kB
CmaFree:               0 kB
HugePages_Total:    1024
HugePages_Free:     1024
HugePages_Rsvd:        0
HugePages_Surp:        0
Hugepagesize:       2048 kB
```

```txt

enum node_stat_item {
	NR_LRU_BASE,
	NR_INACTIVE_ANON = NR_LRU_BASE, /* must match order of LRU_[IN]ACTIVE */
	NR_ACTIVE_ANON,		/*  "     "     "   "       "         */
	NR_INACTIVE_FILE,	/*  "     "     "   "       "         */
	NR_ACTIVE_FILE,		/*  "     "     "   "       "         */
	NR_UNEVICTABLE,		/*  "     "     "   "       "         */
	NR_SLAB_RECLAIMABLE,
	NR_SLAB_UNRECLAIMABLE,
	NR_ISOLATED_ANON,	/* Temporary isolated pages from anon lru */
	NR_ISOLATED_FILE,	/* Temporary isolated pages from file lru */
	WORKINGSET_REFAULT,
	WORKINGSET_ACTIVATE,
	WORKINGSET_NODERECLAIM,
	NR_ANON_MAPPED,	/* Mapped anonymous pages */
	NR_FILE_MAPPED,	/* pagecache pages mapped into pagetables.
			   only modified from process context */
	NR_FILE_PAGES,
	NR_FILE_DIRTY,
	NR_WRITEBACK,
	NR_WRITEBACK_TEMP,	/* Writeback using temporary buffers */
	NR_SHMEM,		/* shmem pages (included tmpfs/GEM pages) */
	NR_SHMEM_THPS,
	NR_SHMEM_PMDMAPPED,
	NR_ANON_THPS,
	NR_UNSTABLE_NFS,	/* NFS unstable pages */
	NR_VMSCAN_WRITE,
	NR_VMSCAN_IMMEDIATE,	/* Prioritise for reclaim when writeback ends */
	NR_DIRTIED,		/* page dirtyings since bootup */
	NR_WRITTEN,		/* page writings since bootup */
	NR_INDIRECTLY_RECLAIMABLE_BYTES, /* measured in bytes */
	NR_VM_NODE_STAT_ITEMS
};
```

## zone_stat_item
```c
enum zone_stat_item {
	/* First 128 byte cacheline (assuming 64 bit words) */
	NR_FREE_PAGES,
	NR_ZONE_LRU_BASE, /* Used only for compaction and reclaim retry */
	NR_ZONE_INACTIVE_ANON = NR_ZONE_LRU_BASE,
	NR_ZONE_ACTIVE_ANON,
	NR_ZONE_INACTIVE_FILE,
	NR_ZONE_ACTIVE_FILE,
	NR_ZONE_UNEVICTABLE,
	NR_ZONE_WRITE_PENDING,	/* Count of dirty, writeback and unstable pages */
	NR_MLOCK,		/* mlock()ed pages found and moved off LRU */
	/* Second 128 byte cacheline */
	NR_BOUNCE,
#if IS_ENABLED(CONFIG_ZSMALLOC)
	NR_ZSPAGES,		/* allocated in zsmalloc */
#endif
	NR_FREE_CMA_PAGES,
	NR_VM_ZONE_STAT_ITEMS };
```

## 简单分析一下 vmstat.c 中内容

为了处理锁和 batch 写了很多代码。

实现了:

```c
	proc_create_seq("buddyinfo", 0444, NULL, &fragmentation_op);
	proc_create_seq("pagetypeinfo", 0400, NULL, &pagetypeinfo_op);
	proc_create_seq("vmstat", 0444, NULL, &vmstat_op);
	proc_create_seq("zoneinfo", 0444, NULL, &zoneinfo_op);
```

## https://www.tecmint.com/clear-ram-memory-cache-buffer-and-swap-space-on-linux/

## code
```c
atomic_long_t vm_zone_stat[NR_VM_ZONE_STAT_ITEMS] __cacheline_aligned_in_smp;
atomic_long_t vm_node_stat[NR_VM_NODE_STAT_ITEMS] __cacheline_aligned_in_smp;
atomic_long_t vm_numa_event[NR_VM_NUMA_EVENT_ITEMS] __cacheline_aligned_in_smp;
```

### mm_struct 中的统计
```c
struct mm_struct {
        // ..
		unsigned long total_vm;	   /* Total pages mapped */
		unsigned long locked_vm;   /* Pages that have PG_mlocked set */
		atomic64_t    pinned_vm;   /* Refcount permanently increased */
		unsigned long data_vm;	   /* VM_WRITE & ~VM_SHARED & ~VM_STACK */
		unsigned long exec_vm;	   /* VM_EXEC & ~VM_WRITE & ~VM_STACK */
		unsigned long stack_vm;	   /* VM_STACK */
        // ...
		struct percpu_counter rss_stat[NR_MM_COUNTERS];
```



## /proc/pid/status : 进行每一个 process 的内存统计
- [ ] cat /proc/self/stat 是什么关系

- 这两个成员是做什么的
```c
		unsigned long hiwater_rss; /* High-watermark of RSS usage */
		unsigned long hiwater_vm;  /* High-water virtual memory usage */
```

```txt
😀  cat /proc/self/status
Name:   cat
Umask:  0022
State:  R (running)
Tgid:   3510141
Ngid:   0
Pid:    3510141
PPid:   3508944
TracerPid:      0
Uid:    1000    1000    1000    1000
Gid:    100     100     100     100
FDSize: 64
Groups: 1 67 100 131
NStgid: 3510141
NSpid:  3510141
NSpgid: 3510141
NSsid:  3508944
VmPeak:   224484 kB
VmSize:   224484 kB
VmLck:         0 kB
VmPin:         0 kB
VmHWM:       392 kB
VmRSS:       392 kB
RssAnon:             100 kB
RssFile:             292 kB
RssShmem:              0 kB
VmData:      452 kB
VmStk:       164 kB
VmExe:      1212 kB
VmLib:      1636 kB
VmPTE:        52 kB
VmSwap:        0 kB
HugetlbPages:          0 kB
CoreDumping:    0
THP_enabled:    1
Threads:        1
SigQ:   0/95382
SigPnd: 0000000000000000
ShdPnd: 0000000000000000
SigBlk: 0000000000000000
SigIgn: 0000000000000000
SigCgt: 0000000000000000
CapInh: 0000000000000000
CapPrm: 0000000000000000
CapEff: 0000000000000000
CapBnd: 000001ffffffffff
CapAmb: 0000000000000000
NoNewPrivs:     0
Seccomp:        0
Seccomp_filters:        0
Speculation_Store_Bypass:       vulnerable
SpeculationIndirectBranch:      always enabled
Cpus_allowed:   0000,00000000,00000000,00000000,00000000,00000000,0000ffff,ffffffff
Cpus_allowed_list:      0-47
Mems_allowed:   00000000,00000001
Mems_allowed_list:      0
voluntary_ctxt_switches:        0
nonvoluntary_ctxt_switches:     0
```

```c
		/*
		 * Special counters, in some configurations protected by the
		 * page_table_lock, in other configurations by being atomic.
		 */
		struct mm_rss_stat rss_stat;
```

MM_SWAPENTS 的使用非常奇怪:
- 有时候是直接修改，有时候是调用 inc_mm_counter，关系是什么?

## vmstate.c
// 搞清楚如何使用这个代码吧 !

`vmstat.h/vmstat.c`
```c
static inline void count_vm_event(enum vm_event_item item)
{
  this_cpu_inc(vm_event_states.event[item]);
}
// @todo 以此为机会找到内核实现暴露接口的位置
```

```c
    __mod_zone_page_state(page_zone(page), NR_MLOCK,
            hpage_nr_pages(page));
    count_vm_event(UNEVICTABLE_PGMLOCKED);
    // 两个统计的机制，但是并不清楚各自统计的内容是什么包含什么区别
```

## cgroup
### memory.numa_stat
### memory.stat

```txt
[root@nixos:/sys/fs/cgroup/mem]# cat memory.stat
anon 2579693568
file 3929878528
kernel 145862656
kernel_stack 212992
pagetables 8470528
percpu 0
sock 36864
vmalloc 8749056
shmem 4096
zswap 0
zswapped 0
file_mapped 1589248
file_dirty 0
file_writeback 0
swapcached 44138496
anon_thp 2390753280
file_thp 0
shmem_thp 0
inactive_anon 1023037440
active_anon 1556824064
inactive_file 678453248
active_file 3251408896
unevictable 0
slab_reclaimable 125010320
slab_unreclaimable 1122064
slab 126132384
workingset_refault_anon 919214
workingset_refault_file 3427513
workingset_activate_anon 438729
workingset_activate_file 2565873
workingset_restore_anon 437633
workingset_restore_file 817979
workingset_nodereclaim 0
pgscan 38044843
pgsteal 14675136
pgscan_kswapd 0
pgscan_direct 38044843
pgsteal_kswapd 0
pgsteal_direct 14675136
pgfault 2706141
pgmajfault 708882
pgrefill 28919794
pgactivate 34762326
pgdeactivate 28917444
pglazyfree 0
pglazyfreed 0
zswpin 0
zswpout 0
thp_fault_alloc 14594
thp_collapse_alloc 1103
```

- 在 shrink_inactive_list 中统计的时候，判断当前是否为
```c
static bool cgroup_reclaim(struct scan_control *sc)
{
	return sc->target_mem_cgroup;
}
```
这导致的现象就是

在 cgroup 中的 memory.stat 的 pgsteal_direct 不是 0，但是/proc/vmstat 中可以发现 pgsteal_direct 是 0
```txt
pgsteal_kswapd 0
pgsteal_direct 0
pgdemote_kswapd 0
pgdemote_direct 0
pgscan_kswapd 0
pgscan_direct 0
pgscan_direct_throttle 0
pgscan_anon 24184580
pgscan_file 13860263
pgsteal_anon 1025690
pgsteal_file 13649446
```
