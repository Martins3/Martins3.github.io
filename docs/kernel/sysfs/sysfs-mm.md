# sysfs memory
- fs/proc/task_mmu.c
- fs/proc/meminfo.c


## bdi

对应文件在 mm/backing-dev.c 中:

/sys/devices/virtual/bdi/252:0/min_ratio_fine

使用说明:
Documentation/ABI/testing/sysfs-class-bdi

```txt
➜  ~ ls -la /sys/class/bdi
total 0
drwxr-xr-x  2 root root 0 Oct 22 14:06 .
drwxr-xr-x 78 root root 0 Oct 22 11:09 ..
lrwxrwxrwx  1 root root 0 Oct 22 11:09 2:0 -> ../../devices/virtual/bdi/2:0
lrwxrwxrwx  1 root root 0 Oct 22 11:09 250:0 -> ../../devices/virtual/bdi/250:0
lrwxrwxrwx  1 root root 0 Oct 22 11:09 250:1 -> ../../devices/virtual/bdi/250:1
lrwxrwxrwx  1 root root 0 Oct 22 14:06 250:2 -> ../../devices/virtual/bdi/250:2
lrwxrwxrwx  1 root root 0 Oct 22 11:09 252:0 -> ../../devices/virtual/bdi/252:0
lrwxrwxrwx  1 root root 0 Oct 22 11:09 253:0 -> ../../devices/virtual/bdi/253:0
lrwxrwxrwx  1 root root 0 Oct 22 11:09 253:16 -> ../../devices/virtual/bdi/253:16
lrwxrwxrwx  1 root root 0 Oct 22 11:09 259:2 -> ../../devices/virtual/bdi/259:2
lrwxrwxrwx  1 root root 0 Oct 22 11:09 259:4 -> ../../devices/virtual/bdi/259:4
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:0 -> ../../devices/virtual/bdi/43:0
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:128 -> ../../devices/virtual/bdi/43:128
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:160 -> ../../devices/virtual/bdi/43:160
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:192 -> ../../devices/virtual/bdi/43:192
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:224 -> ../../devices/virtual/bdi/43:224
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:256 -> ../../devices/virtual/bdi/43:256
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:288 -> ../../devices/virtual/bdi/43:288
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:32 -> ../../devices/virtual/bdi/43:32
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:320 -> ../../devices/virtual/bdi/43:320
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:352 -> ../../devices/virtual/bdi/43:352
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:384 -> ../../devices/virtual/bdi/43:384
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:416 -> ../../devices/virtual/bdi/43:416
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:448 -> ../../devices/virtual/bdi/43:448
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:480 -> ../../devices/virtual/bdi/43:480
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:64 -> ../../devices/virtual/bdi/43:64
lrwxrwxrwx  1 root root 0 Oct 22 11:09 43:96 -> ../../devices/virtual/bdi/43:96
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:0 -> ../../devices/virtual/bdi/7:0
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:1 -> ../../devices/virtual/bdi/7:1
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:2 -> ../../devices/virtual/bdi/7:2
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:3 -> ../../devices/virtual/bdi/7:3
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:4 -> ../../devices/virtual/bdi/7:4
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:5 -> ../../devices/virtual/bdi/7:5
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:6 -> ../../devices/virtual/bdi/7:6
lrwxrwxrwx  1 root root 0 Oct 22 11:09 7:7 -> ../../devices/virtual/bdi/7:7
lrwxrwxrwx  1 root root 0 Oct 22 11:09 8:0 -> ../../devices/virtual/bdi/8:0
lrwxrwxrwx  1 root root 0 Oct 22 11:09 8:16 -> ../../devices/virtual/bdi/8:16
lrwxrwxrwx  1 root root 0 Oct 22 11:09 8:32 -> ../../devices/virtual/bdi/8:32
lrwxrwxrwx  1 root root 0 Oct 22 11:09 8:48 -> ../../devices/virtual/bdi/8:48
lrwxrwxrwx  1 root root 0 Oct 22 11:09 8:64 -> ../../devices/virtual/bdi/8:64
lrwxrwxrwx  1 root root 0 Oct 22 11:09 8:80 -> ../../devices/virtual/bdi/8:80
lrwxrwxrwx  1 root root 0 Oct 22 11:09 8:96 -> ../../devices/virtual/bdi/8:96
lrwxrwxrwx  1 root root 0 Oct 22 14:06 9p-1 -> ../../devices/virtual/bdi/9p-1
```

### 这个
```txt
cat /sys/kernel/debug/bdi/*/wb_stats
```

### /sys/devices/virtual/bdi

### /sys/class/bdi/

```txt
min_ratio:0
max_bytes:6614859776
strict_limit:0
max_ratio:100
max_ratio_fine:1000000
read_ahead_kb:128
stable_pages_required:0
min_bytes:0
min_ratio_fine:0
```

## [ ] bdi 的 debugfs : /sys/kernel/debug/bdi/

我靠，感觉这个数值完全不对啊

/sys/kernel/debug/bdi/259:0/stats
home 分区的盘

```txt
BdiWriteback:                0 kB
BdiReclaimable:              0 kB
BdiDirtyThresh:              0 kB
DirtyThresh:           6636860 kB
BackgroundThresh:      3314376 kB
BdiDirtied:                  0 kB
BdiWritten:                  0 kB
BdiWriteBandwidth:      102400 kBps
b_dirty:                     0
b_io:                        0
b_more_io:                   0
b_dirty_time:                0
bdi_list:                    1
state:                       1
```

/sys/kernel/debug/bdi/259:4/stats
/home/martins3/hack 的盘，
```txt
BdiWriteback:             1920 kB
BdiReclaimable:           1536 kB
BdiDirtyThresh:              0 kB
DirtyThresh:           6639564 kB
BackgroundThresh:      3315728 kB
BdiDirtied:            1143360 kB
BdiWritten:            1122624 kB
BdiWriteBandwidth:        1112 kBps
b_dirty:                     0
b_io:                        0
b_more_io:                   0
b_dirty_time:                0
bdi_list:                    1
state:                       1
```

为什么这里的 BdiWriteback / BdiReclaimable 总是 1920 kB 啊!
也就是当 state 不对的时候，这个数值也不对

- [ ] 不知道在 nixos 中测试的时候，state 总是 1, 从来不会变为 7


在虚拟机中如果停止了 io ，其内容为
```txt
BdiWriteback:             2880 kB
BdiReclaimable:           2080 kB
BdiDirtyThresh:              0 kB
DirtyThresh:         313103044 kB
BackgroundThresh:     52120124 kB
BdiDirtied:          175838560 kB
BdiWritten:          175836160 kB
BdiWriteBandwidth:     2329656 kBps
b_dirty:                     0
b_io:                        0
b_more_io:                   0
b_dirty_time:                0
bdi_list:                    1
state:                       1
```

417K 的 dirty io 的时候:
```txt
BdiWriteback:             2720 kB
BdiReclaimable:           1760 kB
BdiDirtyThresh:      313075212 kB
DirtyThresh:         313075212 kB
BackgroundThresh:     52115488 kB
BdiDirtied:          201575200 kB
BdiWritten:          201572960 kB
BdiWriteBandwidth:     3049300 kBps
b_dirty:                     2
b_io:                        0
b_more_io:                   0
b_dirty_time:                0
bdi_list:                    1
state:                       5
```

- [ ] BdiWriteBandwidth 这个数值完全不对啊
- [ ] BdiWriteback:             2720 kB : 当数值还存在 2M 左右的，永远不为 0，很奇怪

调试一下 read_ahead_kb ，似乎各有不同

- cgroup_writeback_by_id
  - bdi_get_by_id

# 观测 Linux 的内存系统

表格中的 `含义` 栏目如果为空，表示同上。

## 问题
- [ ] node_page_state 和 lruvec_page_state 是什么关系？

## free -m
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

## vmstat
```txt
🧀  vmstat
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache   si   so    bi    bo   in   cs us sy id wa st
 0  0      0 32780892 1269736 19007984    0    0     6    11   24    2  1  0 99  0  0
```

## htop
memory : used/buffer/shared/cache/

- [ ] buffer 指的是
- [x] anonymous shared 是放到什么地方的
- [ ] 如果是放到 tmpfs 中，如何

似乎是将 share map 算作是 cache

unshared map 算作是 cache

## /proc/pid/smaps

- 和 proc/pid/maps 的有啥区别？

## /proc/pid/status
- VmData: 主要是 `vm_stat_account` 负责统计，就是 vma 中那些权限为代码的数据

## /proc/pid/numa_maps

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
主要是关于进程调度，信号的信息，关于内存的部分，包括:
```txt
              (10) minflt  %lu
                     The number of minor faults the process has made
                     which have not required loading a memory page from
                     disk.

              (11) cminflt  %lu
                     The number of minor faults that the process's
                     waited-for children have made.

              (12) majflt  %lu
                     The number of major faults the process has made
                     which have required loading a memory page from
                     disk.

              (13) cmajflt  %lu
                     The number of major faults that the process's
                     waited-for children have made.

              (23) vsize  %lu
                     Virtual memory size in bytes.

              (24) rss  %ld
                     Resident Set Size: number of pages the process has
                     in real memory.  This is just the pages which count
                     toward text, data, or stack space.  This does not
                     include pages which have not been demand-loaded in,
                     or which are swapped out.  This value is
                     inaccurate; see /proc/[pid]/statm below.

              (25) rsslim  %lu
                     Current soft limit in bytes on the rss of the
                     process; see the description of RLIMIT_RSS in
                     getrlimit(2).

              (36) nswap  %lu
                     Number of pages swapped (not maintained).

              (37) cnswap  %lu
                     Cumulative nswap for child processes (not
                     maintained).

```
信息来源: https://man7.org/linux/man-pages/man5/procfs.5.html

- [ ] cminflt 和 cmajflt 是啥意思？

## /proc/pid/statm

## /proc/meminfo
具体内容都在: https://github.com/torvalds/linux/blob/master/Documentation/filesystems/proc.rst#meminfo

具体函数在: meminfo_proc_show

| 名称            | 数值           | 含义                                                                                                                                      |
|-----------------|----------------|-------------------------------------------------------------------------------------------------------------------------------------------|
| MemTotal        | 32690904 kB    | 所有的物理页面，调用 totalram_pages_add ，在启动的时候通过 memblock 获取，在 virtio mem ，virtio balloon 以及 memory hotplug 的时候修改。 |
| MemFree         | 25368728 kB    |                                                                                                                                           |
| MemAvailable    | 28847852 kB    |                                                                                                                                           |
| Buffers         | 251576 kB      |                                                                                                                                |
| Cached          | 3339012 kB     | page cache + shmem                                                                                                                                           |
| SwapCached      | 0 kB           |                                                                                                                                           |
| Active          | 2638452 kB     |                                                                                                                                           |
| Inactive        | 3996836 kB     |                                                                                                                                           |
| Active(anon)    | 68656 kB       |                                                                                                                                           |
| Inactive(anon)  | 2896668 kB     |                                                                                                                                           |
| Active(file)    | 2569796 kB     |                                                                                                                                           |
| Inactive(file)  | 1100168 kB     |                                                                                                                                           |
| Unevictable     | 3408 kB        |                                                                                                                                           |
| Mlocked         | 1872 kB        |                                                                                                                                           |
| SwapTotal       | 0 kB           |                                                                                                                                           |
| SwapFree        | 0 kB           |                                                                                                                                           |
| Zswap           | 0 kB           |                                                                                                                                           |
| Zswapped        | 0 kB           |                                                                                                                                           |
| Dirty           | 104 kB         |                                                                                                                                           |
| Writeback       | 0 kB           |                                                                                                                                           |
| AnonPages       | 3040592 kB     |                                                                                                                                           |
| Mapped          | 341680 kB      |                                                                                                                                           |
| Shmem           | 10912 kB       |                                                                                                                                           |
| KReclaimable    | 274160 kB      |                                                                                                                                           |
| Slab            | 412500 kB      |                                                                                                                                           |
| SReclaimable    | 274160 kB      |                                                                                                                                           |
| SUnreclaim      | 138340 kB      |                                                                                                                                           |
| KernelStack     | 16304 kB       |                                                                                                                                           |
| PageTables      | 23136 kB       |                                                                                                                                           |
| NFS_Unstable    | 0 kB           |                                                                                                                                           |
| Bounce          | 0 kB           |                                                                                                                                           |
| WritebackTmp    | 0 kB           |                                                                                                                                           |
| CommitLimit     | 16345452 kB    | 可以 overmmit 的数量                                                                                                                      |
| Committed_AS    | 12958700 kB    | 已经提交的量，如果 mmap 是 MAP_NORESERVE 的，那么将不会统计在此处                                                                         |
| VmallocTotal    | 34359738367 kB |                                                                                                                                           |
| VmallocUsed     | 102404 kB      |                                                                                                                                           |
| VmallocChunk    | 0 kB           |                                                                                                                                           |
| Percpu          | 114240 kB      |                                                                                                                                           |
| AnonHugePages   | 301056 kB      |                                                                                                                                           |
| ShmemHugePages  | 0 kB           |                                                                                                                                           |
| ShmemPmdMapped  | 0 kB           |                                                                                                                                           |
| FileHugePages   | 0 kB           |                                                                                                                                           |
| FilePmdMapped   | 0 kB           |                                                                                                                                           |
| CmaTotal        | 0 kB           |                                                                                                                                           |
| CmaFree         | 0 kB           |                                                                                                                                           |
| HugePages_Total | 0              |                                                                                                                                           |
| HugePages_Free  | 0              |                                                                                                                                           |
| HugePages_Rsvd  | 0              |                                                                                                                                           |
| HugePages_Surp  | 0              |                                                                                                                                           |
| Hugepagesize    | 2048 kB        |                                                                                                                                           |
| Hugetlb         | 0 kB           |                                                                                                                                           |
| DirectMap4k     | 294756 kB      |                                                                                                                                           |
| DirectMap2M     | 4947968 kB     |                                                                                                                                           |
| DirectMap1G     | 30408704 kB    |                                                                                                                                           |

- [x] Cached 和 Shmem : 前者是包含了部分后者吗？
  - Cache 指的是文件映射的，而 Shmem 包括 anon 的，两者不是完全包含的

从 htop 中观测，显示 swap 只是使用为: 1.44G/9.77G
而实际上观测到：
```txt
➜  ~ grep Swap /proc/meminfo
SwapCached:      2161984 kB
SwapTotal:      10239996 kB
SwapFree:        5998016 kB
```
所以，htop 是将 SwapCached 不算的:

### MemAvailable
细节在 docs/kernel/mm-watermark.md 中

### Direct Map
```txt
DirectMap4k:       36680 kB
DirectMap2M:     3108864 kB
DirectMap1G:    11534336 kB
```

### Cached
1. Cached 不仅仅是我们理解的 page cache，htop 中的 cache 才是我们想要
```c
	cached = global_node_page_state(NR_FILE_PAGES) -
			total_swapcache_pages() - i.bufferram;
```

测试方法 : /dev/shm 中 fallocate -l 10G abc

通过 htop 的注释 https://github.com/htop-dev/htop/blob/278ebbbfafe51a5040fc55169fb8ef50314938c7/linux/LinuxMachine.c#L201

kernel 当时的讨论来看，这是一个技术失误:
https://lore.kernel.org/lkml/1455827801-13082-1-git-send-email-hannes@cmpxchg.org/

anonymous shared memory 也是关联文件的，但是不会增加上 Cached
```c
void *ptr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
```

### bufferram
就是 ram 的

在 virtio balloon 中，disk_caches 指的是:
```c
	caches = global_node_page_state(NR_FILE_PAGES);
```
所以其内容是 meminfo-cache + meminfo-buffer + meminfo-swapcache

### Buffers
就是 raw disk 的 cache

制作方法:
```txt
[global]
ioengine=libaio
iodepth=128

[trash]
bs=4k
direct=0
filename=/dev/nvme1n1
rw=randread
runtime=30000
time_based
```

```c
long nr_blockdev_pages(void)
{
	struct inode *inode;
	long ret = 0;

	spin_lock(&blockdev_superblock->s_inode_list_lock);
	list_for_each_entry(inode, &blockdev_superblock->s_inodes, i_sb_list)
		ret += inode->i_mapping->nrpages;
	spin_unlock(&blockdev_superblock->s_inode_list_lock);

	return ret;
}
```

这里应该增加数值的地方
```txt
    __filemap_add_folio+5
    filemap_add_folio+60
    page_cache_ra_unbounded+180
    force_page_cache_ra+152
    filemap_get_pages+269
    filemap_read+223
    blkdev_read_iter+226
    aio_read+306
    io_submit_one+1451
    __x64_sys_io_submit+173
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
```

__filemap_add_folio 中的
```txt
		mapping->nrpages += nr;
```

如果真的是 buffer cache 中的内容，此外就是 metadata 的 io
```txt
Buffers:           79492 kB
```
路径是: `__find_get_block_slow` -> `__filemap_get_folio`


### LRU_INACTIVE_ANON LRU_INACTIVE_ANON

表示这些页是和文件无关的，如果想要写回，需要进入到 swap 空间中。

### AnonPages Mapped Shmem

将所有的页划分为三种:
1. AnonPages : private 的映射，无论是否为文件
2. Mapped : shared 的映射，无论是否为文件
3. Shmem : anonymous shared 的时候也是算到其中的，但是初次之外，在 /dev/shm 中创建文件，占用的空间是放到这里的

所以，当 anonymous shared 的时候，Mapped 和 Shmem 都会增加。

如果在 /dev/shm/ 中创建一个 20G 文件，并且
1. 如果 shared mmap 该文件: 那么 Mapped (20G) 和 Shmem (20G) 增加，内存总消耗 20G
2. 如果 private mmap 该文件: 那么 AnonPages (20G) 和 Shmem (20G) 增加, 内存总消耗 20G
因为 /dev/shm 创建了文件，所以 Shmem 必然增加 20G ，

在 shmem 上创建文件无论那种映射，最后都只是影响到:
```txt
Active(anon):   17125840 kB
Inactive(anon):  4126904 kB
```

不会影响到:
```txt
Active(file):   22979456 kB
Inactive(file):  3947452 kB
```

- [ ] 虽然测试结果的确如此，但是没有代码支持啊

### SReclaimable

cgroup 的统计下统计，显示 /sys/fs/cgroup/mem/memory.stat
- slab_post_alloc_hook
  - memcg_slab_free_hook
  - memcg_slab_post_alloc_hook
    - mod_objcg_state

全局的统计，在 account_slab 中:
```txt
#0  account_slab (gfp=796704, s=0xffff888100042d00, order=3, slab=0xffffea0004119000) at mm/slab.h:635
#1  allocate_slab (s=s@entry=0xffff888100042d00, flags=796704, node=node@entry=-1) at mm/slub.c:2027
#2  0xffffffff813fb59a in new_slab (node=-1, flags=<optimized out>, s=0xffff888100042d00) at mm/slub.c:2062
#3  ___slab_alloc (s=s@entry=0xffff888100042d00, gfpflags=gfpflags@entry=534560, node=<optimized out>, node@entry=-1, addr=addr@entry=18446744071593159020, c=0xffff88843ba31d50, orig_size=orig_size@entry=4096) at mm/slub.c:3215
#4  0xffffffff813fe61f in __slab_alloc (c=<optimized out>, orig_size=4096, addr=18446744071593159020, node=-1, gfpflags=534560, s=0xffff888100042d00) at mm/slub.c:3314
#5  __slab_alloc_node (orig_size=<optimized out>, addr=<optimized out>, node=<optimized out>, gfpflags=<optimized out>, s=<optimized out>) at mm/slub.c:3367
#6  slab_alloc_node (orig_size=4096, addr=18446744071593159020, node=-1, gfpflags=534560, lru=0x0 <fixed_percpu_data>, s=0xffff888100042d00) at mm/slub.c:3460
#7  __kmem_cache_alloc_node (s=s@entry=0xffff888100042d00, gfpflags=gfpflags@entry=534560, node=node@entry=-1, orig_size=orig_size@entry=4096, caller=caller@entry=18446744071593159020) at mm/slub.c:3509
#8  0xffffffff8138409e in __do_kmalloc_node (caller=18446744071593159020, node=-1, flags=534560, size=4096)
```

## /proc/vmstat
<!-- 82bdb207-cb86-4424-b656-d2da5ac52f4d -->

proc/vmstat 中的所有的字段都是需要检查下的，慢慢来吧

还是需要写一个 c 文件，专门来做这种测试，让 ai 去搞搞

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
| nr_zspages                     | 5        | zsmalloc                                                         |
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
| nr_slab_reclaimable            | 100760   |                                                                   |
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

### NR_FILE_PAGES 就是 page cache 的使用量?
<!-- a98bca53-347e-4578-bbe4-f538d125b940 -->

```txt
1. /proc/meminfo 中 Cached 的计算方式

// fs/proc/meminfo.c:48-51
cached = global_node_page_state(NR_FILE_PAGES) -
         total_swapcache_pages() - i.bufferram;
if (cached < 0)
    cached = 0;

关键发现：Cached（用户看到的 page cache）是通过 NR_FILE_PAGES 减去 swapcache 和 buffer 得到的。这说明 NR_FILE_PAGES 本身包含
了这两部分。

2. NR_FILE_PAGES 包含 swap cache

// mm/swap_state.c:165
node_stat_mod_folio(folio, NR_FILE_PAGES, nr_pages);  // 加入 swap cache 时增加

// mm/swap_state.c:210
node_stat_mod_folio(folio, NR_FILE_PAGES, -nr_pages); // 移除 swap cache 时减少

注释证实（mm/migrate.c:667-668）：

* Note that anonymous folios are accounted for
* via NR_FILE_PAGES and NR_ANON_MAPPED if they
* are mapped to swap space.

3. NR_FILE_PAGES 包含 buffer cache

Buffer cache 在现代内核中也通过 page cache 机制实现（block/bdev.c:533-544）：

long nr_blockdev_pages(void)
{
    struct inode *inode;
    long ret = 0;

    list_for_each_entry(inode, &blockdev_superblock->s_inodes, i_sb_list)
        ret += inode->i_mapping->nrpages;  // 统计块设备的 page cache

    return ret;
}

4. NR_FILE_PAGES 的统计位置汇总

NR_FILE_PAGES = 真正的文件页缓存 + tmpfs/shmem + swap cache
              = (Cached + Buffers + SwapCached)  // 近似

三者关系：

• NR_FILE_PAGES：内核统计计数器，范围最广
• /proc/meminfo 中的 Cached：用户可见的"纯"文件缓存（已减去 swapcache 和 buffers）
• Page Cache：通常指文件内容的缓存（不含 swap cache）

所以严格来说，NR_FILE_PAGES > Page Cache，说它"就是 page cache 的使用量"是不准确的。
```
测试验证 : docs/kernel/sysfs/sysfs-mm-nr-files-pages.c


show_free_areas 中可以见
```txt
	printk("%ld total pagecache pages\n", global_node_page_state(NR_FILE_PAGES));
```

从主线中的:
node_pagecache_reclaimable

理解下这个代码是什么含义:
```c
	/*
	 * If RECLAIM_UNMAP is set, then all file pages are considered
	 * potentially reclaimable. Otherwise, we have to worry about
	 * pages like swapcache and node_unmapped_file_pages() provides
	 * a better estimate
	 */
	if (node_reclaim_mode & RECLAIM_UNMAP)
		nr_pagecache_reclaimable = node_page_state(pgdat, NR_FILE_PAGES);
	else
		nr_pagecache_reclaimable = node_unmapped_file_pages(pgdat);
```

```c
static inline unsigned long node_unmapped_file_pages(struct pglist_data *pgdat)
{
	unsigned long file_mapped = node_page_state(pgdat, NR_FILE_MAPPED);
	unsigned long file_lru = node_page_state(pgdat, NR_INACTIVE_FILE) +
		node_page_state(pgdat, NR_ACTIVE_FILE);

	/*
	 * It's possible for there to be more file mapped pages than
	 * accounted for by the pages on the file LRU lists because
	 * tmpfs pages accounted for as ANON can also be FILE_MAPPED
	 */
	return (file_lru > file_mapped) ? (file_lru - file_mapped) : 0;
}
```

/proc/vmstat 中的这三个:
```txt
nr_anon_pages 962779
nr_mapped 170736
nr_file_pages 507217
```

#### 通过这个，岂不是可以知道 process 的 pagecache 使用量

需要这么间接的方法吗?

如果我知道他没有使用 share memory
```c
unsigned long task_statm(struct mm_struct *mm,
			 unsigned long *shared, unsigned long *text,
			 unsigned long *data, unsigned long *resident)
{
	*shared = get_mm_counter_sum(mm, MM_FILEPAGES) +
			get_mm_counter_sum(mm, MM_SHMEMPAGES);
	*text = (PAGE_ALIGN(mm->end_code) - (mm->start_code & PAGE_MASK))
								>> PAGE_SHIFT;
	*data = mm->data_vm + mm->stack_vm;
	*resident = *shared + get_mm_counter_sum(mm, MM_ANONPAGES);
	return mm->total_vm;
}
```

这个 cgroup-v2 中的 file_mapped 解释了:
https://docs.kernel.org/admin-guide/cgroup-v2.html

这个可以深究一下:
```txt
file_mapped
Amount of cached filesystem data mapped with mmap(). Note that some kernel configurations might account complete larger allocations (e.g., THP) if only some, but not not all the memory of such an allocation is mapped.
```

## pgscan && pgsteal

```txt
🧀  cat /proc/vmstat | grep pgscan
pgscan_kswapd 2151257518
pgscan_direct 112182953
pgscan_khugepaged 6781511
pgscan_direct_throttle 0
pgscan_anon 97388903
pgscan_file 2172833079

🧀  cat /proc/vmstat | grep pgsteal
pgsteal_kswapd 1260187354
pgsteal_direct 48696647
pgsteal_khugepaged 6591977
pgsteal_anon 55265878
pgsteal_file 1260210100
```

- pgsteal 表示已经分配出来的 page
- pgscan 表示扫描过的 page

shrink_inactive_list 中，通过调用 reclaimer_offset() 来确定到底是说明哪一个环境。

### pgpgin && pgpgout

/proc/vmstat 的 pgpgin 和 pgpgout 表示进行 io 的总量，统计在 submit_bio()

但是在 cgroup 的统计中，是 charge  来多少，统计在
mem_cgroup_charge_statistics()

- Documentation/admin-guide/cgroup-v1/memory.rst 说的就是 charge 的数量:

1. __remove_mapping() -> mem_cgroup_swapout() 的时候，调用 mem_cgroup_charge_statistics()
2. 但是


看看 commit c449deb2b99f ("mm: memcg: fix swapcached stat accounting")
到底在修理什么问题？

### pgmajfault

pgmajfault 增加了多少，其实是不能代表系统中内存数量的使用的。

构建一次内核，pgmajfault
1. 1439
2. 从 42064 -> 44227 : 增加 2163

但是我的编译的文件数量都有 3195 的，所以说，这是因为存 folio 的大页导致的吗? (并不是，filemap_fault -> filemap_alloc_folio 的 order 总是 1)
但是，构建完成之后，我的 page cache 增加绝对不只是 8M 而已。
这些 page cache 都是哪里来的?

相反的，minor pgfault 增加非常迅速，没有什么实际意义。

```txt
@[
    filemap_get_pages+1
    filemap_read+223
    vfs_read+499
    __x64_sys_pread64+152
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+114
]: 238014
```
大多数时候读取文件，其实读取到自己的 anon 内存中的，


一次 ccls 索引，一共会产生的:
```txt
@[
    filemap_fault+5
    __do_fault+46
    do_fault+862
    __handle_mm_fault+1621
    handle_mm_fault+341
    do_user_addr_fault+515
    exc_page_fault+109
    asm_exc_page_fault+38
]: 14955
```
- 44398 -> 50549 : 增加 6151

不懂，但是看来数值是确定的


增加的三个位置:
1. filemap_fault()
2. shmem_swapin_folio()
3. do_swap_page()


## /proc/slabinfo

## /proc/zoneinfo

- [ ] 从这里看，存在一个 zone 居然是 device



## /proc/pagetypeinfo

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


## /proc/buddyinfo

## /proc/sys/vm
```txt
.rw-r--r--     0 root 11 Nov 12:43   admin_reserve_kbytes
.-w-------     0 root 11 Nov 12:43   compact_memory
.rw-r--r--     0 root 11 Nov 12:43   compact_unevictable_allowed
.rw-r--r--     0 root 11 Nov 12:43   compaction_proactiveness
.rw-r--r--     0 root 11 Nov 12:43   dirty_background_bytes
.rw-r--r--     0 root 11 Nov 11:54   dirty_background_ratio
.rw-r--r--     0 root 11 Nov 12:43   dirty_bytes
.rw-r--r--     0 root 11 Nov 12:43   dirty_expire_centisecs
.rw-r--r--     0 root 11 Nov 11:54   dirty_ratio
.rw-r--r--     0 root 11 Nov 12:43   dirty_writeback_centisecs
.rw-r--r--     0 root 11 Nov 12:43   dirtytime_expire_seconds
.-w-------     0 root 11 Nov 12:43   drop_caches
.rw-r--r--     0 root 11 Nov 12:43   enable_soft_offline
.rw-r--r--     0 root 11 Nov 12:43   extfrag_threshold
.rw-r--r--     0 root 11 Nov 12:43   hugetlb_optimize_vmemmap
.rw-r--r--     0 root 11 Nov 12:43   hugetlb_shm_group
.rw-r--r--     0 root 11 Nov 12:43   laptop_mode
.rw-r--r--     0 root 11 Nov 12:43   legacy_va_layout
.rw-r--r--     0 root 11 Nov 12:43   lowmem_reserve_ratio
.rw-r--r--     0 root 11 Nov 11:55   max_map_count
.rw-r--r--     0 root 11 Nov 12:43   memfd_noexec
.rw-r--r--     0 root 11 Nov 12:43   memory_failure_early_kill
.rw-r--r--     0 root 11 Nov 12:43   memory_failure_recovery
.rw-r--r--     0 root 11 Nov 12:43   min_free_kbytes
.rw-r--r--     0 root 11 Nov 12:43   min_slab_ratio
.rw-r--r--     0 root 11 Nov 12:43   min_unmapped_ratio
.rw-r--r--     0 root 11 Nov 12:43   mmap_min_addr
.rw-------     0 root 11 Nov 12:43   mmap_rnd_bits
.rw-------     0 root 11 Nov 12:43   mmap_rnd_compat_bits
.rw-r--r--     0 root 11 Nov 12:43   nr_hugepages
.rw-r--r--     0 root 11 Nov 12:43   nr_hugepages_mempolicy
.rw-r--r--     0 root 11 Nov 12:43   nr_overcommit_hugepages
.rw-r--r--     0 root 11 Nov 12:43   numa_stat
.rw-r--r--     0 root 11 Nov 12:43   numa_zonelist_order
.rw-r--r--     0 root 11 Nov 12:43   oom_dump_tasks
.rw-r--r--     0 root 11 Nov 12:43   oom_kill_allocating_task
.rw-r--r--     0 root 11 Nov 12:43   overcommit_kbytes
.rw-r--r--     0 root 11 Nov 11:55   overcommit_memory
.rw-r--r--     0 root 11 Nov 12:43   overcommit_ratio
.rw-r--r--     0 root 11 Nov 12:43   page-cluster
.rw-r--r--     0 root 11 Nov 12:43   page_lock_unfairness
.rw-r--r--     0 root 11 Nov 12:43   panic_on_oom
.rw-r--r--     0 root 11 Nov 12:43   percpu_pagelist_high_fraction
.rw-r--r--     0 root 11 Nov 12:43   stat_interval
.rw-------     0 root 11 Nov 12:43   stat_refresh
.rw-r--r--     0 root 11 Nov 11:54   swappiness
.rw-r--r--     0 root 11 Nov 12:43   unprivileged_userfaultfd
.rw-r--r--     0 root 11 Nov 12:43   user_reserve_kbytes
.rw-r--r--     0 root 11 Nov 12:43   vfs_cache_pressure
.rw-r--r--     0 root 11 Nov 12:43   watermark_boost_factor
.rw-r--r--     0 root 11 Nov 12:43   watermark_scale_factor
.rw-r--r--     0 root 11 Nov 12:43   zone_reclaim_mode
```

🧀  cat dirtytime_expire_seconds
43200

有趣


## /sys/kernel/mm

### hugepages


## /sys/devices/system/node/node0

```txt
Permissions Size User Date Modified Name
.-w-------  4.1k root 22 Aug 10:07   compact
lrwxrwxrwx     0 root 22 Aug 10:07   cpu0 -> ../../cpu/cpu0
lrwxrwxrwx     0 root 22 Aug 10:07   cpu1 -> ../../cpu/cpu1
lrwxrwxrwx     0 root 22 Aug 10:07   cpu2 -> ../../cpu/cpu2
lrwxrwxrwx     0 root 22 Aug 10:07   cpu3 -> ../../cpu/cpu3
lrwxrwxrwx     0 root 22 Aug 10:07   cpu4 -> ../../cpu/cpu4
lrwxrwxrwx     0 root 22 Aug 10:07   cpu5 -> ../../cpu/cpu5
lrwxrwxrwx     0 root 22 Aug 10:07   cpu6 -> ../../cpu/cpu6
lrwxrwxrwx     0 root 22 Aug 10:07   cpu7 -> ../../cpu/cpu7
lrwxrwxrwx     0 root 22 Aug 10:07   cpu8 -> ../../cpu/cpu8
lrwxrwxrwx     0 root 22 Aug 10:07   cpu9 -> ../../cpu/cpu9
lrwxrwxrwx     0 root 22 Aug 10:07   cpu10 -> ../../cpu/cpu10
lrwxrwxrwx     0 root 22 Aug 10:07   cpu11 -> ../../cpu/cpu11
lrwxrwxrwx     0 root 22 Aug 10:07   cpu12 -> ../../cpu/cpu12
lrwxrwxrwx     0 root 22 Aug 10:07   cpu13 -> ../../cpu/cpu13
lrwxrwxrwx     0 root 22 Aug 10:07   cpu14 -> ../../cpu/cpu14
lrwxrwxrwx     0 root 22 Aug 10:07   cpu15 -> ../../cpu/cpu15
lrwxrwxrwx     0 root 22 Aug 10:07   cpu16 -> ../../cpu/cpu16
lrwxrwxrwx     0 root 22 Aug 10:07   cpu17 -> ../../cpu/cpu17
lrwxrwxrwx     0 root 22 Aug 10:07   cpu18 -> ../../cpu/cpu18
lrwxrwxrwx     0 root 22 Aug 10:07   cpu19 -> ../../cpu/cpu19
lrwxrwxrwx     0 root 22 Aug 10:07   cpu20 -> ../../cpu/cpu20
lrwxrwxrwx     0 root 22 Aug 10:07   cpu21 -> ../../cpu/cpu21
lrwxrwxrwx     0 root 22 Aug 10:07   cpu22 -> ../../cpu/cpu22
lrwxrwxrwx     0 root 22 Aug 10:07   cpu23 -> ../../cpu/cpu23
lrwxrwxrwx     0 root 22 Aug 10:07   cpu24 -> ../../cpu/cpu24
lrwxrwxrwx     0 root 22 Aug 10:07   cpu25 -> ../../cpu/cpu25
lrwxrwxrwx     0 root 22 Aug 10:07   cpu26 -> ../../cpu/cpu26
lrwxrwxrwx     0 root 22 Aug 10:07   cpu27 -> ../../cpu/cpu27
lrwxrwxrwx     0 root 22 Aug 10:07   cpu28 -> ../../cpu/cpu28
lrwxrwxrwx     0 root 22 Aug 10:07   cpu29 -> ../../cpu/cpu29
lrwxrwxrwx     0 root 22 Aug 10:07   cpu30 -> ../../cpu/cpu30
lrwxrwxrwx     0 root 22 Aug 10:07   cpu31 -> ../../cpu/cpu31
.r--r--r--  4.1k root 22 Aug 10:07   cpulist
.r--r--r--  4.1k root 22 Aug 10:07   cpumap
.r--r--r--  4.1k root 22 Aug 10:07   distance
drwxr-xr-x     - root 22 Aug 10:07   hugepages
.r--r--r--  4.1k root 21 Aug 20:30   meminfo
lrwxrwxrwx     0 root 22 Aug 10:07   memory0 -> ../../memory/memory0
lrwxrwxrwx     0 root 22 Aug 10:07   memory2 -> ../../memory/memory2
lrwxrwxrwx     0 root 22 Aug 10:07   memory3 -> ../../memory/memory3
lrwxrwxrwx     0 root 22 Aug 10:07   memory4 -> ../../memory/memory4
lrwxrwxrwx     0 root 22 Aug 10:07   memory5 -> ../../memory/memory5
lrwxrwxrwx     0 root 22 Aug 10:07   memory6 -> ../../memory/memory6
lrwxrwxrwx     0 root 22 Aug 10:07   memory7 -> ../../memory/memory7
lrwxrwxrwx     0 root 22 Aug 10:07   memory8 -> ../../memory/memory8
lrwxrwxrwx     0 root 22 Aug 10:07   memory9 -> ../../memory/memory9
lrwxrwxrwx     0 root 22 Aug 10:07   memory10 -> ../../memory/memory10
lrwxrwxrwx     0 root 22 Aug 10:07   memory11 -> ../../memory/memory11
lrwxrwxrwx     0 root 22 Aug 10:07   memory12 -> ../../memory/memory12
lrwxrwxrwx     0 root 22 Aug 10:07   memory13 -> ../../memory/memory13
lrwxrwxrwx     0 root 22 Aug 10:07   memory14 -> ../../memory/memory14
lrwxrwxrwx     0 root 22 Aug 10:07   memory15 -> ../../memory/memory15
lrwxrwxrwx     0 root 22 Aug 10:07   memory16 -> ../../memory/memory16
lrwxrwxrwx     0 root 22 Aug 10:07   memory17 -> ../../memory/memory17
lrwxrwxrwx     0 root 22 Aug 10:07   memory18 -> ../../memory/memory18
lrwxrwxrwx     0 root 22 Aug 10:07   memory19 -> ../../memory/memory19
lrwxrwxrwx     0 root 22 Aug 10:07   memory20 -> ../../memory/memory20
lrwxrwxrwx     0 root 22 Aug 10:07   memory21 -> ../../memory/memory21
lrwxrwxrwx     0 root 22 Aug 10:07   memory22 -> ../../memory/memory22
lrwxrwxrwx     0 root 22 Aug 10:07   memory23 -> ../../memory/memory23
lrwxrwxrwx     0 root 22 Aug 10:07   memory24 -> ../../memory/memory24
lrwxrwxrwx     0 root 22 Aug 10:07   memory25 -> ../../memory/memory25
lrwxrwxrwx     0 root 22 Aug 10:07   memory26 -> ../../memory/memory26
lrwxrwxrwx     0 root 22 Aug 10:07   memory27 -> ../../memory/memory27
lrwxrwxrwx     0 root 22 Aug 10:07   memory28 -> ../../memory/memory28
lrwxrwxrwx     0 root 22 Aug 10:07   memory29 -> ../../memory/memory29
lrwxrwxrwx     0 root 22 Aug 10:07   memory30 -> ../../memory/memory30
lrwxrwxrwx     0 root 22 Aug 10:07   memory31 -> ../../memory/memory31
lrwxrwxrwx     0 root 22 Aug 10:07   memory32 -> ../../memory/memory32
.r--r--r--  4.1k root 22 Aug 10:07   numastat
drwxr-xr-x     - root 22 Aug 10:07   power
lrwxrwxrwx     0 root 22 Aug 10:07   subsystem -> ../../../../bus/node
.rw-r--r--  4.1k root 22 Aug 10:07   uevent
.r--r--r--  4.1k root 22 Aug 10:07   vmstat
drwxr-xr-x     - root 22 Aug 10:07   x86
```

## /sys/fs/cgroup/mem/memory.stat

```txt
🧀  cat memory.stat
anon 0
file 0
kernel 0
kernel_stack 0
pagetables 0
sec_pagetables 0
percpu 0
sock 0
vmalloc 0
shmem 0
zswap 0
zswapped 0
file_mapped 0
file_dirty 0
file_writeback 0
swapcached 0
anon_thp 0
file_thp 0
shmem_thp 0
inactive_anon 0
active_anon 0
inactive_file 0
active_file 0
unevictable 0
slab_reclaimable 0
slab_unreclaimable 0
slab 0
workingset_refault_anon 0
workingset_refault_file 0
workingset_activate_anon 0
workingset_activate_file 0
workingset_restore_anon 0
workingset_restore_file 0
workingset_nodereclaim 0
pgscan 0
pgsteal 0
pgscan_kswapd 0
pgscan_direct 0
pgscan_khugepaged 0
pgsteal_kswapd 0
pgsteal_direct 0
pgsteal_khugepaged 0
pgfault 0
pgmajfault 0
pgrefill 0
pgactivate 0
pgdeactivate 0
pglazyfree 0
pglazyfreed 0
zswpin 0
zswpout 0
thp_fault_alloc 0
thp_collapse_alloc 0
```

## /sys/devices/system/node

对应的代码
```c
typedef struct { DECLARE_BITMAP(bits, MAX_NUMNODES); } nodemask_t;
nodemask_t node_states[NR_NODE_STATES] __read_mostly;
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

### memory.events

统计内容如下:
```c
enum memcg_memory_event {
	MEMCG_LOW,
	MEMCG_HIGH,
	MEMCG_MAX,
	MEMCG_OOM,
	MEMCG_OOM_KILL,
	MEMCG_OOM_GROUP_KILL,
	MEMCG_SWAP_HIGH,
	MEMCG_SWAP_MAX,
	MEMCG_SWAP_FAIL,
	MEMCG_NR_MEMORY_EVENTS,
};
```
- MEMCG_LOW shrink_node_memcgs
- MEMCG_HIGH reclaim_high
- MEMCG_MAX try_charge_memcg : 将要超过数值

- 具体的参考这里: https://facebookmicrosites.github.io/cgroup2/docs/memory-controller.html

## 综合问题

### 如果获取 page table size
- cat /proc/meminfo | grep PageTables
- /proc/<pid>/status

**kvm_tdp_mmu_map** 中存在一个 tracepoint 为 trace_kvm_mmu_spte_requested

```c
sudo perf stat -e 'kvm:kvm_mmu_spte_requested' -a sleep infinity
sudo bpftrace -e 'tracepoint:kvmmmu:kvm_mmu_spte_requested { @[comm] = count(); }'
```

12G 内存中，进行一次数值统计:
```txt
🧀  sudo bpftrace -e 'tracepoint:kvmmmu:kvm_mmu_spte_requested { @[comm] = count(); }'
@[qemu-system-x86]: 3247337
```
就是 ept page fault 的数量

但是
```c
static struct kvm_mmu_page *tdp_mmu_alloc_sp(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu_page *sp;

	sp = kvm_mmu_memory_cache_alloc(&vcpu->arch.mmu_page_header_cache);
	sp->spt = kvm_mmu_memory_cache_alloc(&vcpu->arch.mmu_shadow_page_cache);

	return sp;
}
```
- kvm_total_used_mmu_pages : 靠，这个 counter 是给 shadow page 用的，最后用来 shrink 的

其他的初始化流程:
```c
struct kmem_cache *mmu_page_header_cache;
```
1. kvm_mmu_vendor_module_init 中初始化 cache
2. 然后在 kvm_mmu_create 中赋值给具体的 vcpu 吧

- 但是关于 page 的唯独没有统计，因为其中是直接获取 page 数量的:
```c
static inline void *mmu_memory_cache_alloc_obj(struct kvm_mmu_memory_cache *mc,
					       gfp_t gfp_flags)
{
	gfp_flags |= mc->gfp_zero;

	if (mc->kmem_cache)
		return kmem_cache_alloc(mc->kmem_cache, gfp_flags);
	else
		return (void *)__get_free_page(gfp_flags);
}
```

如果向看到底使用了多少个 ept page，可以查看 `trace_kvm_mmu_get_page` 来分析
sudo bpftrace -e 'tracepoint:kvmmmu:kvm_mmu_get_page {  @[comm] = count(); }'

```txt
🧀  sudo bpftrace -e 'tracepoint:kvmmmu:kvm_mmu_get_page {  @[comm] = count(); }'
@[qemu-system-x86]: 7064
```

### 如何获取到 page struct 占用的空间
cat /proc/zoneinfo | grep managed | awk -F' ' '{sum+=$2;} END{print sum;}'

- 获取 struct page 的大小？

```txt
sudo bpftrace -e 'tracepoint:syscalls:sys_exit_openat {  printf("%ld\n", sizeof(struct page)); exit(); }'
```
64 byte

在 512G 的机器上，这需要 8G 的空间。

### 分析存在内存是不被物理内存管理的
memblock=debug


## 统计技术细节
cat /proc/zoneinfo 的时候还可以展示 zone 的 nr_inactive_file，
但是一个 cgroup 的数值是可以放到多个 zone 中间的，也就是说这些数值不是重叠的
这个统计起来似乎不容易吧。

可以观察下，是怎么把数值同时加到两个维度的统计中的

## DirectMap2M 的含义
- https://stackoverflow.com/questions/38318414/directmap1g-display-a-wired-huge-number
- https://unix.stackexchange.com/questions/204286/what-does-mean-by-hardwarecorrupted-directmap4k-directmap2m-fields-in-proc-m
- https://unix.stackexchange.com/questions/375880/in-linux-given-directmap4k-directmap2m-directmap4m-and-directmap1g-why-nr-h

## dimm 是怎么回事?

想不到两个东西居然是一个东西
```txt
/sys/devices/system/edac/mc/mc0/dimm28
/sys/bus/mc0/devices/dimm0
```
drivers/edac/edac_mc_sysfs.c

## 问题
- /sys/devices/system/node/node2 : 这个是如何创建出来的
## /proc/self/clear_refs

对于所有的映射的 page 调用:
```c
		/* Clear accessed and referenced bits. */
		ptep_test_and_clear_young(vma, addr, pte);
		test_and_clear_page_young(page);
		ClearPageReferenced(page);
```

## pagemap
https://www.kernel.org/doc/Documentation/vm/pagemap.txt
介绍了四个接口：

- /proc/pid/pagemap
- /proc/kpagecount
- /proc/kpageflags
- /proc/kpagecgroup

参考 docs/kernel/mm-failure.sh 来处理一下

### kpagecgroup 可以处理大页吗？似乎不行吧

虽然测试的时候为 0，但是按照道理来说
```c
static void commit_charge(struct folio *folio, struct mem_cgroup *memcg)
{
	VM_BUG_ON_FOLIO(folio_memcg(folio), folio);
	/*
	 * Any of the following ensures page's memcg stability:
	 *
	 * - the page lock
	 * - LRU isolation
	 * - lock_page_memcg()
	 * - exclusive reference
	 * - mem_cgroup_trylock_pages()
	 */
	folio->memcg_data = (unsigned long)memcg;
}
```
难道 hugepage 的 charge 是怎么走的哇?

sudo cgcreate -g memory,hugetlb:mem

才意识到，原来，memcg 和 cgroup 走的是两个道路啊
```txt
@[
    hugetlb_cgroup_commit_charge+1
    alloc_huge_page+1108
    hugetlb_fault+2995
    handle_mm_fault+637
    do_user_addr_fault+460
    exc_page_fault+103
    asm_exc_page_fault+34
]: 1024
```
- charge_memcg -> commit_charge 中会设置，但是 hugetlb 中不会。


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
