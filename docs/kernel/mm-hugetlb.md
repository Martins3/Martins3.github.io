# hugetlb

- /dev/hugepages 是做啥的?

- 无需考虑碎片化的问题。
- 内核不用使用这些页面。
- 不用 swap 的。
- 其 cgroup 是单独分析的。
- 不会用在 page cache 上的。

首先，注意区分一下
```txt
obj-$(CONFIG_HUGETLBFS) += hugetlb.o
obj-$(CONFIG_CGROUP_HUGETLB) += hugetlb_cgroup.o
obj-$(CONFIG_TRANSPARENT_HUGEPAGE) += huge_memory.o khugepaged.o
```

- 如果，hugepage 中的页可以 overcommit 的，但是和 memory 的 overcommit 不是一个东西。
- https://www.kernel.org/doc/html/latest/admin-guide/mm/hugetlbpage.html
- https://github.com/lagopus/lagopus/blob/master/docs/how-to-allocate-1gb-hugepages.md

- 外部接口:
  - /proc/meminfo
  - /proc/sys/vm/nr_hugepages_mempolicy
  - /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
  - /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages_mempolicy
- 这些外部接口太混乱了，使用 nuamstat -m 会更加清晰


默认的 mount 点:
```txt
hugetlbfs /dev/hugepages hugetlbfs rw,seclabel,relatime,pagesize=1024M 0 0
```
libhugetlbfs 中有个函数 : `hugetlbfs_find_path_for_size`

- hugetlb_file_setup
  - hugetlb_reserve_pages
  - alloc_file_pseudo ：如果成功，该文件关联的的 hugetlbfs_file_operations

```c
struct hugetlbfs_inode_info {
    struct shared_policy policy;
    struct inode vfs_inode;
    unsigned int seals;
};
```
- `hugetlbfs_setattr` 中为什么需要修改 inode 的大小
  - 应该是修改文件的大小的

- [ ] 我是没有想到，居然 2021 才支持的: https://lwn.net/Articles/872070/
  - https://stackoverflow.com/questions/27997934/mremap2-with-hugetlb-to-change-virtual-address : 直接可以通过 hugetlbfs 来实现

- VM_MAYSHARE 是什么时候创建的
  - do_mmap 的时候，参数 MMAP_SHARE

- hugetlbfs_get_inode 中会创建  resv_map

- hugetlb_reserve_pages 的两个调用源头:
  - hugetlb_file_setup
  - hugetlbfs_file_mmap

- hugetlb_reserve_pages
  - hugetlb_acct_memory ：来检查 reservation 是否能够通过限制，例如 cpuset , numa 等


- [ ] 预留机制和 cpuset 似乎是互相冲突的，详情参考 `hugetlb_acct_memory`
- [ ] 为什么 hugetlb 的预留机制这么复杂，在 memory overcommit 中也是存在预留的哇。

## hugetlb 的使用

- 如何检查系统中，到底谁在使用 hugepage 的
  - https://unix.stackexchange.com/questions/167451/how-to-monitor-use-of-huge-pages-per-process

## hugetlb 是如何影响文件系统的
- 不能作为 page cache 的？
- 对于文件系统是透明的吗?

## [ ] 如果一个共享的 vma ，两个 thread 同时读一个位置，如何保证不会出现错误

## alloc_huge_page && hugetlb_reserve_pages
都是 hugepage_subpool_get_pages 打交道，只是一个在 page fault 的时候处理，一个是在 mmap 的时候

alloc_huge_page 调用位置 : hugetlb_cow(被 hugetlb_no_page 调用) hugetlb_no_page(被 hugetlb_fault 调用)

hugetlb_reserve_pages :  hugetlb_file_setup 和 hugetlbfs_file_mmap

## file_operations::mmap 和 vm_area_struct::vm_operations_struct::fault 的关系

- hugetlbfs_file_mmap 中会根据文件的大小预留内存 hugetlb_reserve_pages

调用 hugetlb_reserve_pages 的两个位置:
- hugetlbfs_file_mmap
  - newseg : shm.c
  - ksys_mmap_pgoff : mmap.c
  - memfd_create : memfd.c
- hugetlb_file_setup

## 普通的 page 和 hugepage 是如何转换的

```c
#define persistent_huge_pages(h) (h->nr_huge_pages - h->surplus_huge_pages)
```

最后全部在:

- set_max_huge_pages
  - [ ] try_to_free_low : 没有太看懂，但是 highmem 的时候才需要
  - remove_pool_huge_page
    - `__remove_hugetlb_page` ：从其中看，hugepage 和 普通的 page 都是可以释放的
  - update_and_free_pages_bulk

free 2M 的
```txt
#0  __remove_hugetlb_page (h=h@entry=0xffffffff834abe20 <hstates>, page=page@entry=0xffffea0004838000, adjust_surplus=adjust_surplus@entry=false, demote=demote@entry=false) at mm/hugetlb.c:1434
#1  0xffffffff812eef0d in remove_hugetlb_page (adjust_surplus=false, page=0xffffea0004838000, h=0xffffffff834abe20 <hstates>, h@entry=0xffffea0004838000) at mm/hugetlb.c:1479
#2  remove_pool_huge_page (h=h@entry=0xffffffff834abe20 <hstates>, nodes_allowed=nodes_allowed@entry=0xffffffff82cf8218 <node_states+24>, acct_surplus=acct_surplus@entry=false) at mm/hugetlb.c:2050
#3  0xffffffff812f0521 in set_max_huge_pages (h=h@entry=0xffffffff834abe20 <hstates>, count=count@entry=1, nid=nid@entry=-1, nodes_allowed=0xffffffff82cf8218 <node_states+24>) at mm/hugetlb.c:3393
#4  0xffffffff812f07c8 in __nr_hugepages_store_common (len=2, count=1, nid=-1, h=0xffffffff834abe20 <hstates>, obey_mempolicy=false) at mm/hugetlb.c:3582
#5  hugetlb_sysctl_handler_common (obey_mempolicy=<optimized out>, table=<optimized out>, write=<optimized out>, buffer=<optimized out>, length=<optimized out>, ppos=<optimized out>) at mm/hugetlb.c:4385
#6  0xffffffff813a9e2e in proc_sys_call_handler (iocb=0xffffc900015d3ea0, iter=0xffffc900015d3e78, write=<optimized out>) at fs/proc/proc_sysctl.c:611
#7  0xffffffff8131d509 in call_write_iter (iter=0xffffea0004838000, kio=0xffffffff834abe20 <hstates>, file=0xffff8881225c5100) at include/linux/fs.h:2187
#8  new_sync_write (ppos=0xffffc900015d3f08, len=2, buf=0x7ff38a997000 "1\n", filp=0xffff8881225c5100) at fs/read_write.c:491
#9  vfs_write (file=file@entry=0xffff8881225c5100, buf=buf@entry=0x7ff38a997000 "1\n", count=count@entry=2, pos=pos@entry=0xffffc900015d3f08) at fs/read_write.c:578
#10 0xffffffff8131d8da in ksys_write (fd=<optimized out>, buf=0x7ff38a997000 "1\n", count=2) at fs/read_write.c:631
#11 0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900015d3f58) at arch/x86/entry/common.c:50
#12 do_syscall_64 (regs=0xffffc900015d3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#13 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

free 1G 也是类似的，但是需要注意的，当把数值降低之后，是没有办法再将数值升高的。
如果 hugetlb 永远不会增加了。

## [ ] 大页如何 KSM

## [ ] 如何查看系统中启动预留的内存，其中的内存是不是永远不会被 buddy 使用的

## [ ] 一个 mmap 的时候，其中是否可以同时包含两种 size 大小的 page
- 不是有一个 mask 吗, 是有好几个的吗?

## 分配过程中，如何逐个地被 memory policy ，cpuset 和 cgroup 管理

## 理解一下核心结构体

```c
struct hstate {
    struct mutex resize_lock;
    int next_nid_to_alloc;
    int next_nid_to_free;
    unsigned int order;
    unsigned int demote_order;
    unsigned long mask;
    unsigned long max_huge_pages;
    unsigned long nr_huge_pages;
    unsigned long free_huge_pages;
    unsigned long resv_huge_pages;
    unsigned long surplus_huge_pages;
    unsigned long nr_overcommit_huge_pages;
    struct list_head hugepage_activelist;
    struct list_head hugepage_freelists[MAX_NUMNODES];
    unsigned int max_huge_pages_node[MAX_NUMNODES];
    unsigned int nr_huge_pages_node[MAX_NUMNODES];
    unsigned int free_huge_pages_node[MAX_NUMNODES];
    unsigned int surplus_huge_pages_node[MAX_NUMNODES];
#ifdef CONFIG_CGROUP_HUGETLB
    /* cgroup control files */
    struct cftype cgroup_files_dfl[8];
    struct cftype cgroup_files_legacy[10];
#endif
    char name[HSTATE_NAME_LEN];
};
```
总共的统计数据和所有的统计数据:

- [ ] hugepage_activelist ：没有搞懂这个和 memory policy , migrate 和 cgroup 的关系

- nr_overcommit_huge_pages ：当前可以超过使用的 page
- surplus_huge_pages : 当前实际上使用的 page
  - 两者的关系可以从 alloc_surplus_huge_page 轻松的看到

## hstate_is_gigantic
- 为什么到处都是这个东西的判断?
- alloc_surplus_huge_page : 如果是 gigantic ，那么直接失败，因为 surplus 的需要从 buddy 中申请
- [ ] 从 nr_hugepages_store 上分析现在 gigantic 的大页是如果通过 alloc_contig_pages 实现分配的

## [ ] demote 如何工作的

## [ ] 大页应该更加小心的处理 memory poison 才对吧，毕竟更加容易出现错误

## [ ] folio 是如何影响 hugetlb 的

## [ ] alloc_buddy_huge_page_with_mpol 为什么正好在  dequeue_huge_page_vma 分配不出来的时候进行

dequeue_huge_page_vma 中还是有 memory policy 的代码的啊

之后还有
```c
    hugetlb_cgroup_commit_charge(idx, pages_per_huge_page(h), h_cg, page);
```

## cgroup 是如何影响的
- hugetlb_cgroup_charge_cgroup 和 hugetlb_cgroup_commit_charge_rsvd 在 alloc_huge_page 的时候被检查

## cpuset 是如何影响的
- dequeue_huge_page_nodemask 中会检查 cpuset_zone_allowed
- hugetlb_reserve_pages 中，会调用 hugetlb_acct_memory ->  allowed_mems_nr 的，在预留的时候会检查 cpuset / cgroup 的限制的

## TODO
- [ ] /home/martins3/core/linux/Documentation/translations/zh_CN/mm/hugetlbfs_reserv.rst
- [ ] https://lwn.net/Articles/839737/
  - https://lwn.net/ml/linux-kernel/20201210035526.38938-1-songmuchun@bytedance.com/
- https://zhuanlan.zhihu.com/p/392703566

## CONFIG_CONTIG_ALLOC 是做什么

```txt
config CONTIG_ALLOC
    def_bool (MEMORY_ISOLATION && COMPACTION) || CMA
```
是否可以分配连续的内存

- alloc_fresh_huge_page，从 buddy 中获取，如果没有被这个打开，无法获取连续的 page，直接失败。

## [ ] dissolve_free_huge_page 被 memory_failure 和 memory hotplug 有关的

## [ ] 是在什么时间点预留的
应该很清楚了，但是整理一下

## 分配的时候如何考虑 numa 的

通过 interleave 的方法分配的:
```txt
                          Node 0          Node 1          Node 2           Total
                 --------------- --------------- --------------- ---------------
MemTotal                 6990.34          972.48            0.00         7962.82
MemFree                  3596.47           39.74            0.00         3636.21
MemUsed                  3393.88          932.74            0.00         4326.61
```

```c
/*
 * Allocates a fresh page to the hugetlb allocator pool in the node interleaved
 * manner.
 */
static int alloc_pool_huge_page(struct hstate *h, nodemask_t *nodes_allowed,
				nodemask_t *node_alloc_noretry)
```

## [ ] 预留的时候需要制定每一种大小的 page 的数量吗

## [x] 如果 hugetlb_file_setup 是入口，那个文件系统还需要使用吗
ksys_mmap_pgoff 中处理过，


## [ ] vm_operations_struct

```txt
#0  hugetlb_vm_op_close (vma=0xffff8883086d03c0) at include/linux/hugetlb.h:720
#1  0xffffffff812c58c9 in remove_vma (vma=vma@entry=0xffff8883086d03c0) at mm/mmap.c:143
#2  0xffffffff812c8363 in exit_mmap (mm=mm@entry=0xffff888300244800) at mm/mmap.c:3121
#3  0xffffffff810ff6ed in __mmput (mm=0xffff888300244800) at kernel/fork.c:1187
#4  mmput (mm=mm@entry=0xffff888300244800) at kernel/fork.c:1208
#5  0xffffffff811085db in exit_mm () at kernel/exit.c:510
#6  do_exit (code=code@entry=0) at kernel/exit.c:782
#7  0xffffffff81108e58 in do_group_exit (exit_code=0) at kernel/exit.c:925
#8  0xffffffff81108ecf in __do_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:936
#9  __se_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:934
#10 __x64_sys_exit_group (regs=<optimized out>) at kernel/exit.c:934
#11 0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900005a7f58) at arch/x86/entry/common.c:50
#12 do_syscall_64 (regs=0xffffc900005a7f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#13 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#14 0x0000000000000000 in ?? ()
```

```txt
const struct vm_operations_struct hugetlb_vm_ops = {
    .fault = hugetlb_vm_op_fault,
    .open = hugetlb_vm_op_open,
    .close = hugetlb_vm_op_close,
    .may_split = hugetlb_vm_op_split,
    .pagesize = hugetlb_vm_op_pagesize,
};
```
- hugetlb_vm_op_open 不会被调用，是因为其只是在被 copy 的时候才有用的。

下面几个都是什么意思:
- [ ] open
- [ ] close
- [ ] may_split
- [ ] pagesize

## gup
- follow_huge_pud 和类似的一堆函数 follow 函数

## nr_hugepages_mempolicy 的含义

```txt
numactl -m <node-list> echo 20 >/proc/sys/vm/nr_hugepages_mempolicy
```
和
```txt
echo 20 > /proc/sys/vm/nr_hugepages_mempolicy
```
的区别是什么 ?

在函数 `__nr_hugepages_store_common` 中调用 `init_nodemask_of_mempolicy` 会根据 current 的 mempolicy 来构建。

是不是说，新增加的 page 的是按照 memory policy 来分配

会出现 /proc/sys/vm/nr_hugepages 和 /proc/sys/vm/nr_hugepages_mempolicy 的数值不同的情况吗?

按照当前配置，强行提高 2M 的页面 /proc/sys/vm/nr_hugepages，其最大可以设置为:
```plain
HugePages_Total:    1835
HugePages_Free:     1835
```
而同时大小为 1G 的页面还是那么多，看来 demote 是有点难以触发的。

## [ ] copy_hugetlb_page_range

## memory policy 只是在调整 nr_hugepages 的时候有用，在 fault 的时候应该也是效果的才对吧
- 是的，在分配的环节中，dequeue_huge_page_vma => huge_node ，在其中分配的时候，将会

## [ ] hugetlbfs_inode_info::policy 似乎根本没有用过
```c
struct hugetlbfs_inode_info {
    struct shared_policy policy;
    struct inode vfs_inode;
    unsigned int seals;
};
```
- 什么叫做 shared policy ?

## seal

- [mm: Add an F_SEAL_FUTURE_WRITE seal to memfd](https://lwn.net/Articles/768785/)

hugetlbfs_inode_info 中的 policy 是没有用吧的发
```c
struct hugetlbfs_inode_info {
    struct shared_policy policy;
    struct inode vfs_inode;
    unsigned int seals;
};
```

- hugetlbfs_get_inode 和 shmem_get_inode 相同，默认初始化 seals 为  F_SEAL_SEAL，表示所有的操作都可以，但是之后可以通过 memfd 实现。

- hugetlbfs_inode_info::seals :  在 hugetlbfs_get_inode

## hugepage 的动态伸缩

- /sys/devices/system/node/node[0-9]*/hugepages/
- /proc/sys/vm/nr_hugepages
- /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages_mempolicy

最终进入到 set_max_huge_pages 中。

```txt
#0  set_max_huge_pages (h=h@entry=0xffffffff834abe20 <hstates>, count=2, nid=0, nodes_allowed=0xffffc90001617e10) at mm/hugetlb.c:3271
#1  0xffffffff812f087c in __nr_hugepages_store_common (len=2, count=<optimized out>, nid=<optimized out>, h=0xffffffff834abe20 <hstates>, obey_mempolicy=false) atmm/hugetlb.c:3582
#2  nr_hugepages_store_common (obey_mempolicy=<optimized out>, kobj=<optimized out>, buf=<optimized out>, len=2) at mm/hugetlb.c:3601
#3  0xffffffff813b212b in kernfs_fop_write_iter (iocb=0xffffc90001617ea0, iter=<optimized out>) at fs/kernfs/file.c:354
#4  0xffffffff8131d509 in call_write_iter (iter=0x2 <fixed_percpu_data+2>, kio=0xffffffff834abe20 <hstates>, file=0xffff888205e3a000) at include/linux/fs.h:2187
#5  new_sync_write (ppos=0xffffc90001617f08, len=2, buf=0x7fd94a3e8000 "2\n", filp=0xffff888205e3a000) at fs/read_write.c:491
#6  vfs_write (file=file@entry=0xffff888205e3a000, buf=buf@entry=0x7fd94a3e8000 "2\n", count=count@entry=2, pos=pos@entry=0xffffc90001617f08) at fs/read_write.c:578
#7  0xffffffff8131d8da in ksys_write (fd=<optimized out>, buf=0x7fd94a3e8000 "2\n", count=2) at fs/read_write.c:631
#8  0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001617f58) at arch/x86/entry/common.c:50
#9  do_syscall_64 (regs=0xffffc90001617f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#10 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#11 0x0000000000000000 in ?? ()
```

- set_max_huge_pages
  - alloc_fresh_huge_page
    - alloc_buddy_huge_page
      - `__alloc_pages`

- [ ] /proc/sys/vm/nr_hugepages_mempolicy 中，最后 nid 传递给 `__nr_hugepages_store_common` 的参数是 NUMA_NO_NODE，

## 分配和回收是如何进行的

- enqueue_huge_page
```txt
#0  enqueue_huge_page (h=h@entry=0xffffffff834abe20 <hstates>, page=page@entry=0xffffea000b000000) at mm/hugetlb.c:1114
#1  0xffffffff812f0d4f in free_huge_page (page=0xffffea000b000000) at mm/hugetlb.c:1734
#2  0xffffffff8128c8bb in __folio_put_large (folio=<optimized out>) at mm/swap.c:118
#3  release_pages (pages=pages@entry=0xffffc9000054fe38, nr=nr@entry=8) at mm/swap.c:978
#4  0xffffffff812e62e5 in free_pages_and_swap_cache (pages=pages@entry=0xffffc9000054fe38, nr=nr@entry=8) at mm/swap_state.c:311
#5  0xffffffff812ca608 in tlb_batch_pages_flush (tlb=tlb@entry=0xffffc9000054fdf8) at mm/mmu_gather.c:58
#6  0xffffffff812cabb0 in tlb_flush_mmu_free (tlb=0xffffc9000054fdf8) at mm/mmu_gather.c:255
#7  tlb_flush_mmu (tlb=0xffffc9000054fdf8) at mm/mmu_gather.c:262
#8  tlb_finish_mmu (tlb=tlb@entry=0xffffc9000054fdf8) at mm/mmu_gather.c:353
#9  0xffffffff812c8346 in exit_mmap (mm=mm@entry=0xffff88830211fc00) at mm/mmap.c:3115
#10 0xffffffff810ff6ed in __mmput (mm=0xffff88830211fc00) at kernel/fork.c:1187
#11 mmput (mm=mm@entry=0xffff88830211fc00) at kernel/fork.c:1208
#12 0xffffffff811085db in exit_mm () at kernel/exit.c:510
#13 do_exit (code=code@entry=0) at kernel/exit.c:782
#14 0xffffffff81108e58 in do_group_exit (exit_code=0) at kernel/exit.c:925
#15 0xffffffff81108ecf in __do_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:936
#16 __se_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:934
#17 __x64_sys_exit_group (regs=<optimized out>) at kernel/exit.c:934
#18 0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9000054ff58) at arch/x86/entry/common.c:50
#19 do_syscall_64 (regs=0xffffc9000054ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#20 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#21 0x0000000000000000 in ?? ()
```

```txt
#0  dequeue_huge_page_nodemask (h=h@entry=0xffffffff834abe20 <hstates>, gfp_mask=1051842, nid=0, nmask=nmask@entry=0x0 <fixed_percpu_data>) at include/linux/gfp.h
:170
#1  0xffffffff812f1f74 in dequeue_huge_page_vma (chg=<optimized out>, avoid_reserve=<optimized out>, address=140629040431104, vma=0xffff88830ed12000, h=0xffffffff
834abe20 <hstates>) at mm/hugetlb.c:1220
#2  alloc_huge_page (vma=vma@entry=0xffff88830ed12000, addr=addr@entry=140629040431104, avoid_reserve=avoid_reserve@entry=0) at mm/hugetlb.c:2925
#3  0xffffffff812f5a25 in hugetlb_no_page (flags=597, old_pte=..., ptep=0xffff888300351cd8, address=140629040431104, idx=0, mapping=0xffff8883003f4188, vma=0xffff
88830ed12000, mm=0xffff888301a8bc00) at mm/hugetlb.c:5545
#4  hugetlb_fault (mm=0xffff888301a8bc00, vma=vma@entry=0xffff88830ed12000, address=address@entry=140629040431104, flags=flags@entry=597) at mm/hugetlb.c:5763
#5  0xffffffff812bdf9b in handle_mm_fault (vma=0xffff88830ed12000, address=address@entry=140629040431104, flags=flags@entry=597, regs=regs@entry=0xffffc9000057ff5
8) at mm/memory.c:5149
#6  0xffffffff810f29a3 in do_user_addr_fault (regs=regs@entry=0xffffc9000057ff58, error_code=error_code@entry=6, address=address@entry=140629040431104) at arch/x8
6/mm/fault.c:1397
#7  0xffffffff81ead672 in handle_page_fault (address=140629040431104, error_code=6, regs=0xffffc9000057ff58) at arch/x86/mm/fault.c:1488
#8  exc_page_fault (regs=0xffffc9000057ff58, error_code=6) at arch/x86/mm/fault.c:1544
#9  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#10 0x0000000000000000 in ?? ()
```

hugetlb_vm_op_fault 是一定不会触发的，因为很早的时候就已经被 handle_mm_fault 中被劫持了。

- [ ] 但是为什么不使用常规路径哇，而是非要改动主线代码?

## reservation
- [ ] hugetlb_vm_op_open 处理 reservation 的

```txt
#0  region_add (resv=resv@entry=0xffff8883063e73c0, f=0, t=1, in_regions_needed=in_regions_needed@entry=1, h=h@entry=0x0 <fixed_percpu_data>, h_cg=h_cg@entry=0x0
<fixed_percpu_data>) at mm/hugetlb.c:531
#1  0xffffffff812ef70c in __vma_reservation_common (h=<optimized out>, vma=0xffff88830631de40, addr=<optimized out>, mode=VMA_COMMIT_RESV) at mm/hugetlb.c:2532
#2  0xffffffff812f210e in vma_commit_reservation (addr=140508781346816, vma=0xffff88830631de40, h=0xffffffff834abe20 <hstates>) at mm/hugetlb.c:2597
#3  alloc_huge_page (vma=vma@entry=0xffff88830631de40, addr=addr@entry=140508781346816, avoid_reserve=avoid_reserve@entry=0) at mm/hugetlb.c:2952
#4  0xffffffff812f5a25 in hugetlb_no_page (flags=597, old_pte=..., ptep=0xffff88830085a958, address=140508781346816, idx=0, mapping=0xffff888302fbc410, vma=0xffff
88830631de40, mm=0xffff888300246c00) at mm/hugetlb.c:5545
#5  hugetlb_fault (mm=0xffff888300246c00, vma=vma@entry=0xffff88830631de40, address=address@entry=140508781346816, flags=flags@entry=597) at mm/hugetlb.c:5763
#6  0xffffffff812bdf9b in handle_mm_fault (vma=0xffff88830631de40, address=address@entry=140508781346816, flags=flags@entry=597, regs=regs@entry=0xffffc900005c7f5
8) at mm/memory.c:5149
#7  0xffffffff810f29a3 in do_user_addr_fault (regs=regs@entry=0xffffc900005c7f58, error_code=error_code@entry=6, address=address@entry=140508781346816) at arch/x8
6/mm/fault.c:1397
#8  0xffffffff81ead672 in handle_page_fault (address=140508781346816, error_code=6, regs=0xffffc900005c7f58) at arch/x86/mm/fault.c:1488
```

## vma_resv_map 到底是做啥用的
```txt
#0  0xffffffff812ef611 in vma_resv_map (vma=<optimized out>) at mm/hugetlb.c:974
#1  __vma_reservation_common (h=0xffffffff834abe20 <hstates>, vma=0xffff88830631de40, addr=140508781346816, mode=VMA_NEEDS_RESV) at mm/hugetlb.c:2517
#2  0xffffffff812f623a in vma_needs_reservation (addr=140508781346816, vma=0xffff88830631de40, h=0xffffffff834abe20 <hstates>) at mm/hugetlb.c:2591
#3  hugetlb_no_page (flags=597, old_pte=..., ptep=0xffff88830085a958, address=140508781346816, idx=<optimized out>, mapping=0xffff888302fbc410, vma=0xffff88830631
de40, mm=0xffff888300246c00) at mm/hugetlb.c:5617
#4  hugetlb_fault (mm=0xffff888300246c00, vma=vma@entry=0xffff88830631de40, address=address@entry=140508781346816, flags=flags@entry=597) at mm/hugetlb.c:5763
#5  0xffffffff812bdf9b in handle_mm_fault (vma=0xffff88830631de40, address=address@entry=140508781346816, flags=flags@entry=597, regs=regs@entry=0xffffc900005c7f5
8) at mm/memory.c:5149
#6  0xffffffff810f29a3 in do_user_addr_fault (regs=regs@entry=0xffffc900005c7f58, error_code=error_code@entry=6, address=address@entry=140508781346816) at arch/x8
6/mm/fault.c:1397
#7  0xffffffff81ead672 in handle_page_fault (address=140508781346816, error_code=6, regs=0xffffc900005c7f58) at arch/x86/mm/fault.c:1488
#8  exc_page_fault (regs=0xffffc900005c7f58, error_code=6) at arch/x86/mm/fault.c:1544
#9  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#10 0x0000000000000000 in ?? ()
```
- 在 page fault 的时候 reserve 有点说不过吧

```txt
#0  0xffffffff812ef611 in vma_resv_map (vma=<optimized out>) at mm/hugetlb.c:974
#1  __vma_reservation_common (h=0xffffffff834abe20 <hstates>, vma=0xffff88830631de40, addr=140508781346816, mode=VMA_END_RESV) at mm/hugetlb.c:2517
#2  0xffffffff812f6252 in vma_end_reservation (addr=140508781346816, vma=0xffff88830631de40, h=0xffffffff834abe20 <hstates>) at mm/hugetlb.c:2603
#3  hugetlb_no_page (flags=597, old_pte=..., ptep=0xffff88830085a958, address=140508781346816, idx=<optimized out>, mapping=0xffff888302fbc410, vma=0xffff88830631
de40, mm=0xffff888300246c00) at mm/hugetlb.c:5622
#4  hugetlb_fault (mm=0xffff888300246c00, vma=vma@entry=0xffff88830631de40, address=address@entry=140508781346816, flags=flags@entry=597) at mm/hugetlb.c:5763
#5  0xffffffff812bdf9b in handle_mm_fault (vma=0xffff88830631de40, address=address@entry=140508781346816, flags=flags@entry=597, regs=regs@entry=0xffffc900005c7f5
8) at mm/memory.c:5149
#6  0xffffffff810f29a3 in do_user_addr_fault (regs=regs@entry=0xffffc900005c7f58, error_code=error_code@entry=6, address=address@entry=140508781346816) at arch/x8
6/mm/fault.c:1397
#7  0xffffffff81ead672 in handle_page_fault (address=140508781346816, error_code=6, regs=0xffffc900005c7f58) at arch/x86/mm/fault.c:1488
#8  exc_page_fault (regs=0xffffc900005c7f58, error_code=6) at arch/x86/mm/fault.c:1544
#9  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#10 0x0000000000000000 in ?? ()
```

```txt
#0  vma_resv_map (vma=0xffff88830631de40) at mm/hugetlb.c:974
#1  hugetlb_vm_op_close (vma=0xffff88830631de40) at mm/hugetlb.c:4589
#2  0xffffffff812c58c9 in remove_vma (vma=vma@entry=0xffff88830631de40) at mm/mmap.c:143
#3  0xffffffff812c8363 in exit_mmap (mm=mm@entry=0xffff888300246c00) at mm/mmap.c:3121
#4  0xffffffff810ff6ed in __mmput (mm=0xffff888300246c00) at kernel/fork.c:1187
#5  mmput (mm=mm@entry=0xffff888300246c00) at kernel/fork.c:1208
#6  0xffffffff811085db in exit_mm () at kernel/exit.c:510
#7  do_exit (code=code@entry=0) at kernel/exit.c:782
#8  0xffffffff81108e58 in do_group_exit (exit_code=0) at kernel/exit.c:925
#9  0xffffffff81108ecf in __do_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:936
#10 __se_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:934
#11 __x64_sys_exit_group (regs=<optimized out>) at kernel/exit.c:934
#12 0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900005c7f58) at arch/x86/entry/common.c:50
#13 do_syscall_64 (regs=0xffffc900005c7f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#14 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#15 0x0000000000000000 in ?? ()
```

reservation 应该是创建的时候就存在的，但是为什么要设计出来 commit 之类的操作

## pool

通过触发 /proc/sys/vm/nr_hugepages 来控制 pool 中的 pages 的数量
```txt
#0  remove_pool_huge_page (h=h@entry=0xffffffff834abe20 <hstates>, nodes_allowed=nodes_allowed@entry=0xffffffff82cf8218 <node_states+24>, acct_surplus=acct_surplu
s@entry=false) at mm/hugetlb.c:2041
#1  0xffffffff812f0321 in set_max_huge_pages (h=h@entry=0xffffffff834abe20 <hstates>, count=count@entry=4, nid=nid@entry=-1, nodes_allowed=0xffffffff82cf8218 <nod
e_states+24>) at mm/hugetlb.c:3393
#2  0xffffffff812f05c8 in __nr_hugepages_store_common (len=2, count=4, nid=-1, h=0xffffffff834abe20 <hstates>, obey_mempolicy=false) at mm/hugetlb.c:3582
#3  hugetlb_sysctl_handler_common (obey_mempolicy=<optimized out>, table=<optimized out>, write=<optimized out>, buffer=<optimized out>, length=<optimized out>, p
pos=<optimized out>) at mm/hugetlb.c:4385
#4  0xffffffff813a9c2e in proc_sys_call_handler (iocb=0xffffc90000a63ea0, iter=0xffffc90000a63e78, write=<optimized out>) at fs/proc/proc_sysctl.c:611
#5  0xffffffff8131d309 in call_write_iter (iter=0xffffffff82cf8218 <node_states+24>, kio=0xffffffff834abe20 <hstates>, file=0xffff88830ec92200) at include/linux/f
s.h:2187
#6  new_sync_write (ppos=0xffffc90000a63f08, len=2, buf=0x7f90eb632000 "4\n", filp=0xffff88830ec92200) at fs/read_write.c:491
#7  vfs_write (file=file@entry=0xffff88830ec92200, buf=buf@entry=0x7f90eb632000 "4\n", count=count@entry=2, pos=pos@entry=0xffffc90000a63f08) at fs/read_write.c:5
78
#8  0xffffffff8131d6da in ksys_write (fd=<optimized out>, buf=0x7f90eb632000 "4\n", count=2) at fs/read_write.c:631
#9  0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000a63f58) at arch/x86/entry/common.c:50
#10 do_syscall_64 (regs=0xffffc90000a63f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#11 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#12 0x0000000000000000 in ?? ()
```

## subpool

通过 vma 必然找到其关联的文件，然后找到对应的 fs, 然后就是关联的 subpool
```c
static inline struct hugepage_subpool *subpool_inode(struct inode *inode)
{
    return HUGETLBFS_SB(inode->i_sb)->spool;
}

static inline struct hugepage_subpool *subpool_vma(struct vm_area_struct *vma)
{
    return subpool_inode(file_inode(vma->vm_file));
}
```
从 hugetlbfs_fill_super 看，默认的 subpool 为 NULL，这导致 hugepage_subpool_get_pages 必然成功。


## hugepage_subpool_get_pages

居然是在 mmap 的时候创建分配的
```txt
#0  hugepage_subpool_get_pages (delta=1, spool=0x0 <fixed_percpu_data>) at mm/hugetlb.c:169
#1  hugetlb_reserve_pages (inode=inode@entry=0xffff888302fbc298, from=0, to=1, vma=vma@entry=0xffff888309e5b240, vm_flags=<optimized out>) at mm/hugetlb.c:6510
#2  0xffffffff81432f48 in hugetlbfs_file_mmap (file=0xffff888301e4cc00, vma=0xffff888309e5b240) at fs/hugetlbfs/inode.c:167
#3  0xffffffff812c9800 in call_mmap (vma=0xffff888309e5b240, file=0xffff888301e4cc00) at include/linux/fs.h:2192
#4  mmap_region (file=file@entry=0xffff888301e4cc00, addr=addr@entry=139922518310912, len=len@entry=1073741824, vm_flags=vm_flags@entry=115, pgoff=<optimized out>
, uf=uf@entry=0xffffc90000c43eb0) at mm/mmap.c:1749
#5  0xffffffff812c9dbe in do_mmap (file=file@entry=0xffff888301e4cc00, addr=139922518310912, addr@entry=0, len=len@entry=1073741824, prot=<optimized out>, prot@en
try=3, flags=flags@entry=262178, pgoff=<optimized out>, pgoff@entry=0, populate=0xffffc90000c43ea8, uf=0xffffc90000c43eb0) at mm/mmap.c:1540
#6  0xffffffff8129ec35 in vm_mmap_pgoff (file=file@entry=0xffff888301e4cc00, addr=addr@entry=0, len=len@entry=1073741824, prot=prot@entry=3, flag=flag@entry=26217
8, pgoff=pgoff@entry=0) at mm/util.c:552
#7  0xffffffff812c7133 in ksys_mmap_pgoff (addr=0, len=1073741824, prot=3, flags=262178, fd=<optimized out>, pgoff=0) at mm/mmap.c:1586
#8  0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000c43f58) at arch/x86/entry/common.c:50
#9  do_syscall_64 (regs=0xffffc90000c43f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#10 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#11 0x0000000000000003 in fixed_percpu_data ()
#12 0xffffffffffffffff in ?? ()
#13 0x0000000000000000 in ?? ()
```

这个 backtrace 这个结果:
```c
  void *ptr = mmap(NULL, 8 * TwoMega, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
```
但是，的确是有逻辑的，hugetlb 的 mapping 总是带有文件，所以，总是会掉用到
`hugepage_subpool_get_pages` 上。


## alloc_huge_page 中间干了什么

- alloc_huge_page
  - vma_needs_reservation : 检查将要分配的虚拟地址上是否存在，如果返回为 0，说明之前已经预留过
    - `__vma_reservation_common`
      - region_chg
  - dequeue_huge_page_vma


- `__vma_reservation_common` 到底需要多少次调用，在一次分配的时候

```txt
#0  __vma_reservation_common (h=0xffffffff834abe20 <hstates>, vma=0xffff88822998a180, addr=140245337112576, mode=VMA_COMMIT_RESV) at mm/hugetlb.c:2511
#1  0xffffffff812f230e in vma_commit_reservation (addr=140245337112576, vma=0xffff88822998a180, h=0xffffffff834abe20 <hstates>) at mm/hugetlb.c:2597
#2  alloc_huge_page (vma=vma@entry=0xffff88822998a180, addr=addr@entry=140245337112576, avoid_reserve=avoid_reserve@entry=0) at mm/hugetlb.c:2952
#3  0xffffffff812f5c25 in hugetlb_no_page (flags=597, old_pte=..., ptep=0xffff8882281d1a60, address=140245337112576, idx=0, mapping=0xffff888228304188, vma=0xffff88822998a180, mm=0xffff88823951d400) at mm/hugetlb.c:5545
#4  hugetlb_fault (mm=0xffff88823951d400, vma=vma@entry=0xffff88822998a180, address=address@entry=140245337112576, flags=flags@entry=597) at mm/hugetlb.c:5763
#5  0xffffffff812be19b in handle_mm_fault (vma=0xffff88822998a180, address=address@entry=140245337112576, flags=flags@entry=597, regs=regs@entry=0xffffc90001d4ff58) at mm/memory.c:5149
#6  0xffffffff810f2983 in do_user_addr_fault (regs=regs@entry=0xffffc90001d4ff58, error_code=error_code@entry=6, address=address@entry=140245337112576) at arch/x86/mm/fault.c:1397
#7  0xffffffff81ead672 in handle_page_fault (address=140245337112576, error_code=6, regs=0xffffc90001d4ff58) at arch/x86/mm/fault.c:1488
#8  exc_page_fault (regs=0xffffc90001d4ff58, error_code=6) at arch/x86/mm/fault.c:1544
#9  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#10 0x0000000000000000 in ?? ()
```

的确是下面的场景:
- NEED
- COMMIT
- NEED
- END : 5622

- copy_hugetlb_page_range
  - restore_reserve_on_error
    - vma_del_reservation
    - vma_add_reservation ：这是 end 唯一的调用的位置，但是和 rmap 放到一起，痛苦面具

## [ ] region_chg && region_add
```txt
#0  region_chg (resv=resv@entry=0xffff888228c272a0, f=0, t=1, out_regions_needed=out_regions_needed@entry=0xffffc90001abfd60) at include/linux/spinlock.h:349
#1  0xffffffff812ef935 in __vma_reservation_common (h=<optimized out>, vma=0xffff8882257ec480, addr=<optimized out>, mode=VMA_NEEDS_RESV) at mm/hugetlb.c:2524
#2  0xffffffff812f1f0f in vma_needs_reservation (addr=<optimized out>, vma=0xffff8882257ec480, h=0xffffffff834abe20 <hstates>) at mm/hugetlb.c:2591
#3  alloc_huge_page (vma=vma@entry=0xffff8882257ec480, addr=addr@entry=139629120454656, avoid_reserve=avoid_reserve@entry=0) at mm/hugetlb.c:2875
#4  0xffffffff812f5c25 in hugetlb_no_page (flags=597, old_pte=..., ptep=0xffff88822a000c08, address=139629120454656, idx=0, mapping=0xffff888228c48410, vma=0xffff8882257ec480, mm=0xffff8881226ee800) at mm/hugetlb.c:5545
#5  hugetlb_fault (mm=0xffff8881226ee800, vma=vma@entry=0xffff8882257ec480, address=address@entry=139629120454656, flags=flags@entry=597) at mm/hugetlb.c:5763
#6  0xffffffff812be19b in handle_mm_fault (vma=0xffff8882257ec480, address=address@entry=139629120454656, flags=flags@entry=597, regs=regs@entry=0xffffc90001abff58) at mm/memory.c:5149
#7  0xffffffff810f2983 in do_user_addr_fault (regs=regs@entry=0xffffc90001abff58, error_code=error_code@entry=6, address=address@entry=139629120454656) at arch/x86/mm/fault.c:1397
#8  0xffffffff81ead672 in handle_page_fault (address=139629120454656, error_code=6, regs=0xffffc90001abff58) at arch/x86/mm/fault.c:1488
#9  exc_page_fault (regs=0xffffc90001abff58, error_code=6) at arch/x86/mm/fault.c:1544
#10 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#11 0x0000000000000000 in ?? ()
```

```txt
#0  region_add (resv=resv@entry=0xffff888228c272a0, f=0, t=1, in_regions_needed=in_regions_needed@entry=1, h=h@entry=0x0 <fixed_percpu_data>, h_cg=h_cg@entry=0x0 <fixed_percpu_data>) at mm/hugetlb.c:531
#1  0xffffffff812ef90c in __vma_reservation_common (h=<optimized out>, vma=0xffff8882257ec480, addr=<optimized out>, mode=VMA_COMMIT_RESV) at mm/hugetlb.c:2532
#2  0xffffffff812f230e in vma_commit_reservation (addr=139629120454656, vma=0xffff8882257ec480, h=0xffffffff834abe20 <hstates>) at mm/hugetlb.c:2597
#3  alloc_huge_page (vma=vma@entry=0xffff8882257ec480, addr=addr@entry=139629120454656, avoid_reserve=avoid_reserve@entry=0) at mm/hugetlb.c:2952
#4  0xffffffff812f5c25 in hugetlb_no_page (flags=597, old_pte=..., ptep=0xffff88822a000c08, address=139629120454656, idx=0, mapping=0xffff888228c48410, vma=0xffff8882257ec480, mm=0xffff8881226ee800) at mm/hugetlb.c:5545
#5  hugetlb_fault (mm=0xffff8881226ee800, vma=vma@entry=0xffff8882257ec480, address=address@entry=139629120454656, flags=flags@entry=597) at mm/hugetlb.c:5763
#6  0xffffffff812be19b in handle_mm_fault (vma=0xffff8882257ec480, address=address@entry=139629120454656, flags=flags@entry=597, regs=regs@entry=0xffffc90001abff58) at mm/memory.c:5149
#7  0xffffffff810f2983 in do_user_addr_fault (regs=regs@entry=0xffffc90001abff58, error_code=error_code@entry=6, address=address@entry=139629120454656) at arch/x86/mm/fault.c:1397
#8  0xffffffff81ead672 in handle_page_fault (address=139629120454656, error_code=6, regs=0xffffc90001abff58) at arch/x86/mm/fault.c:1488
#9  exc_page_fault (regs=0xffffc90001abff58, error_code=6) at arch/x86/mm/fault.c:1544
#10 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
#11 0x0000000000000000 in ?? ()
```

分析 hugetlb_reserve_pages 中调用 region_chg 的位置，可以发现，如果是之所以创建出来各种 region 出来，是为了处理
共享的情况，如果一个 mmap 一个共享的，第一个进程使用一部分，这需要 charge，然后第二个函数使用一部分，那么如果和之前的重叠了。

在 hugetlb_no_page 中的这个位置，也有两个，调用这两个函数的位置:
```c
    /*
     * If we are going to COW a private mapping later, we examine the
     * pending reservations for this page now. This will ensure that
     * any allocations necessary to record that reservation occur outside
     * the spinlock.
     */
    if ((flags & FAULT_FLAG_WRITE) && !(vma->vm_flags & VM_SHARED)) {
        if (vma_needs_reservation(h, vma, haddr) < 0) {
            ret = VM_FAULT_OOM;
            goto backout_unlocked;
        }
        /* Just decrements count, does not deallocate */
        vma_end_reservation(h, vma, haddr);
    }
```
为什么设置出来

## [ ] unmap_ref_private

## mremap 带来的挑战

mremap 可以修改映射的虚拟地址和大小，其粒度是 page 的，也就是可以将一部分 vma 的一部分修改位置。

## vma_resv_map

```c
    if (vma->vm_flags & VM_MAYSHARE) {
```
- [x] 在 fs/hugetlbfs/inode.c 中创建出来的，一定会带上 `VM_MAYSHARE` 吗?
  - 是的
- 不是在 mmap 的时候也是创建了文件吗? hugetlb_file_setup
    - 是的，即使是匿名映射，也是会关联文件的，因为是为了 : hugetlb_vm_ops

```txt
#0  hugetlbfs_file_mmap (file=0xffff8882213f7f00, vma=0xffff88822579b900) at include/linux/fs.h:1333
#1  0xffffffff812c9a00 in call_mmap (vma=0xffff88822579b900, file=0xffff8882213f7f00) at include/linux/fs.h:2192
#2  mmap_region (file=file@entry=0xffff8882213f7f00, addr=addr@entry=139783313555456, len=len@entry=16777216, vm_flags=vm_flags@entry=115, pgoff=<optimized out>, uf=uf@entry=0xffffc90001c0feb0) at mm/mmap.c:1749
#3  0xffffffff812c9fbe in do_mmap (file=file@entry=0xffff8882213f7f00, addr=139783313555456, addr@entry=0, len=len@entry=16777216, prot=<optimized out>, prot@entry=3, flags=flags@entry=262178, pgoff=<optimized out>, pgoff@entry=0, populate=0xffffc90001c0fea8, uf=0xffffc90001c0feb0) at mm/mmap.c:1540
#4  0xffffffff8129ee35 in vm_mmap_pgoff (file=file@entry=0xffff8882213f7f00, addr=addr@entry=0, len=len@entry=16777216, prot=prot@entry=3, flag=flag@entry=262178, pgoff=pgoff@entry=0) at mm/util.c:552
#5  0xffffffff812c7333 in ksys_mmap_pgoff (addr=0, len=16777216, prot=3, flags=262178, fd=<optimized out>, pgoff=0) at mm/mmap.c:1586
#6  0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001c0ff58) at arch/x86/entry/common.c:50
#7  do_syscall_64 (regs=0xffffc90001c0ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#8  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#9  0x0000000000000003 in fixed_percpu_data ()
#10 0xffffffffffffffff in ?? ()
#11 0x0000000000000000 in ?? ()
```

- [ ] 这个注释没有看懂
```c
/*
 * These helpers are used to track how many pages are reserved for
 * faults in a MAP_PRIVATE mapping. Only the process that called mmap()
 * is guaranteed to have their future faults succeed.
 *
 * With the exception of reset_vma_resv_huge_pages() which is called at fork(),
 * the reserve counters are updated with the hugetlb_lock held. It is safe
 * to reset the VMA at fork() time as it is not in use yet and there is no
 * chance of the global counters getting corrupted as a result of the values.
 *
 * The private mapping reservation is represented in a subtly different
 * manner to a shared mapping.  A shared mapping has a region map associated
 * with the underlying file, this region map represents the backing file
 * pages which have ever had a reservation assigned which this persists even
 * after the page is instantiated.  A private mapping has a region map
 * associated with the original mmap which is attached to all VMAs which
 * reference it, this region map represents those offsets which have consumed
 * reservation ie. where pages have been instantiated.
 */
static unsigned long get_vma_private_data(struct vm_area_struct *vma)
{
    return (unsigned long)vma->vm_private_data;
}
```

- hugetlb_reserve_pages
```c
    /*
     *
     * Shared mappings base their reservation on the number of pages that
     * are already allocated on behalf of the file. Private mappings need
     * to reserve the full area even if read-only as mprotect() may be
     * called to make the mapping read-write. Assume !vma is a shm mapping
     */
    if (!vma || vma->vm_flags & VM_MAYSHARE) {
        /*
         * resv_map can not be NULL as hugetlb_reserve_pages is only
         * called for inodes for which resv_maps were created (see
         * hugetlbfs_get_inode).
         */
        resv_map = inode_resv_map(inode);

        chg = region_chg(resv_map, from, to, &regions_needed);

    } else {
        /* Private mapping. */
        resv_map = resv_map_alloc();
        if (!resv_map)
            return false;

        chg = to - from;

        set_vma_resv_map(vma, resv_map);
        set_vma_resv_flags(vma, HPAGE_RESV_OWNER);
    }
```
- priave 的直接整个 map，而 shared 的需要考虑其他人的感受，所以只能 charge 一个部分。
  - 对于 file-map 的来说，这个必然的，但是对于 anon 的来说，这种处理也是可以接受的

- hugetlbfs_file_mmap 为什么会调用 hugetlb_reserve_pages，既然已经将 pages 直接分配了，为什么还是需要调用 reserve
  - hugetlb 的普通 mmap 也是会调用的

- [ ] shm 暂时不做讨论

## vma_has_reserves : 可以理解这个吗

## huge_add_to_page_cache

两个位置调用 huge_add_to_page_cache
1. hugetlbfs_fallocate : 的确是对于每一个 hugepage 都是需要调用的
2. hugetlb_no_page : 分配的时候，如果发现其所在的 VM_MAYSHARE 的，那么将会

- hugetlbfs_pagecache_page : 在一个 vma 中，找到对应位置的 page cache 是什么

在 fallocate 的时候，就会分配 page，但是没有建立 mapping 的，然后 hugetlbfs_file_mmap 之后，那么会出现

- hugetlbfs_file_mmap 中还是需要 hugetlb_reserve_pages，但是对于 share mapping 是有检查的

最后，当 page fault 的时候，hugetlb_no_page 中，首先在 page table 中查询。

## 到底是如何分配的
- hugetlb_hstate_alloc_pages ：这个是初始化的注册的函数

- alloc_fresh_huge_page

## 启动过程中，如何预留的

这是参数解析的时候
```txt
#0  hugetlb_hstate_alloc_pages (h=0xffffffff834abe20 <hstates>) at mm/hugetlb.c:3088
#1  0xffffffff832fbd53 in hugepages_setup (s=0xffff88833fff186a "8") at mm/hugetlb.c:4221
#2  0xffffffff832cf89b in obsolete_checksetup (line=0xffff88833fff1860 "hugepages=8") at init/main.c:219
#3  unknown_bootoption (param=0xffff88833fff1860 "hugepages=8", val=val@entry=0xffff88833fff186a "8", unused=unused@entry=0xffffffff827eb41c "Booting kernel", arg
=arg@entry=0x0 <fixed_percpu_data>) at init/main.c:539
#4  0xffffffff81127893 in parse_one (handle_unknown=0xffffffff832cf801 <unknown_bootoption>, arg=0x0 <fixed_percpu_data>, max_level=-1, min_level=-1, num_params=5
84, params=0xffffffff829aecb8 <__param_initcall_debug>, doing=0xffffffff827eb41c "Booting kernel", val=0xffff88833fff186a "8", param=0xffff88833fff1860 "hugepages
=8") at kernel/params.c:153
#5  parse_args (doing=doing@entry=0xffffffff827eb41c "Booting kernel", args=0xffff88833fff186b "", params=0xffffffff829aecb8 <__param_initcall_debug>, num=584, mi
n_level=min_level@entry=-1, max_level=max_level@entry=-1, arg=0x0 <fixed_percpu_data>, unknown=0xffffffff832cf801 <unknown_bootoption>) at kernel/params.c:188
#6  0xffffffff832cfe00 in start_kernel () at init/main.c:967
#7  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
#8  0x0000000000000000 in ?? ()
```

这是 initcalls 得到的
```txt
#0  hugetlb_hstate_alloc_pages (h=h@entry=0xffffffff834ad5e8 <hstates+6088>) at mm/hugetlb.c:3088
#1  0xffffffff832fc0d1 in hugetlb_init_hstates () at mm/hugetlb.c:3157
#2  hugetlb_init () at mm/hugetlb.c:4069
#3  0xffffffff81000e7c in do_one_initcall (fn=0xffffffff832fbf41 <hugetlb_init>) at init/main.c:1296
#4  0xffffffff832d0491 in do_initcall_level (command_line=0xffff888300804180 "root", level=4) at init/main.c:1369
#5  do_initcalls () at init/main.c:1385
#6  do_basic_setup () at init/main.c:1404
#7  kernel_init_freeable () at init/main.c:1611
#8  0xffffffff81eae271 in kernel_init (unused=<optimized out>) at init/main.c:1500
#9  0xffffffff81001a8f in ret_from_fork () at arch/x86/entry/entry_64.S:306
#10 0x0000000000000000 in ?? ()
```

-  [ ] 为什么要调用两次哇

## hugetlb

1. 为了实现简单，那么 hugetlb 减少处理什么东西 ?

https://www.ibm.com/developerworks/cn/linux/l-cn-hugetlb/
https://www.ibm.com/developerworks/cn/linux/1305_zhangli_hugepage/index.html

总结一下 :
1. subpool, resv_map , enqueue 机制
2. hugetlb_file_setup hugetlb_fault 和对外提供的关键接口
3. 利用 sys 提供了很多接口

Huge pages can improve performance through reduced page faults (a single fault brings in a large chunk of memory at once) and by reducing the cost of virtual to physical address translation (fewer levels of page tables must be traversed to get to the physical address).

用户层 : https://lwn.net/Articles/375096/ 中间的使用首先理解清楚吧 !

https://github.com/libhugetlbfs/libhugetlbfs
> 其中包含有大量的测试
The library provides support for automatically backing text, data, heap and shared memory segments with huge pages.
In addition, this package also provides a programming API and manual pages. The behaviour of the library is controlled by environment variables (as described in the libhugetlbfs.7 manual page) with a launcher utility hugectl that knows how to configure almost all of the variables. hugeadm, hugeedit and pagesize provide information about the system and provide support to system administration. tlbmiss_cost.sh automatically calculates the average cost of a TLB miss. cpupcstat and oprofile_start.sh provide help with monitoring the current behaviour of the system. Manual pages are available describing in further detail each utility.

1. shmget() : SHM_HUGETLB
2. hugetlbfs : 似乎用户共享的，同时可以用于实现

```c
       #include <hugetlbfs.h>
       int hugetlbfs_unlinked_fd(void);
       int hugetlbfs_unlinked_fd_for_size(long page_size);
       // hugetlbfs_unlinked_fd, hugetlbfs_unlinked_fd_for_size - Obtain a file descriptor for a new unlinked file in hugetlbfs
```


One important common point between them all is how huge pages are faulted and when the huge pages are allocated.
Further, there are important differences between shared and private mappings depending on the exact kernel version used. [^1]
> 重点处理的方面

1. fault
2. shared/private
3. hugetlb 不处理 swap

和正常大小的 page 的比较
1. hugetlb_fault

2. include/asm-generic/hugetlb.h : 如果架构含有关于 page table 的不同处理，
那么就可以使用

- [ ] 了解一下，从 mmap 的进入到 hugetlb
  - [ ] 似乎还可以在 hugetlb 的文件系统中间创建文件，然后 open ?

[HugeTLB Pages](https://www.kernel.org/doc/html/latest/admin-guide/mm/hugetlbpage.html) 的阅读结果 ：

> /proc/sys/vm/nr_hugepages indicates the current number of “persistent” huge pages in the kernel’s huge page pool. “Persistent” huge pages will be returned to the huge page pool when freed by a task. A user with root privileges can dynamically allocate more or free some persistent huge pages by increasing or decreasing the value of nr_hugepages.
>
> Pages that are used as huge pages are reserved inside the kernel and **cannot** be used for other purposes. Huge pages cannot be swapped out under memory pressure.
>
> Once a number of huge pages have been pre-allocated to the kernel huge page pool, a user with appropriate privilege can use either the mmap system call or shared memory system calls to use the huge pages.

- [ ] 是不是没有 preallocated 的 page 会导致分配失败 ？

**TO BE CONTINUE**
- [ ] 这个文档还是没有看完的，感觉 hugetlb 设计有点问题

# hugetlbfs
```c
/*
 * node_hstate/s - associate per node hstate attributes, via their kobjects,
 * with node devices in node_devices[] using a parallel array.  The array
 * index of a node device or _hstate == node id.
 * This is here to avoid any static dependency of the node device driver, in
 * the base kernel, on the hugetlb module.
 */
struct node_hstate {
    struct kobject      *hugepages_kobj;
    struct kobject      *hstate_kobjs[HUGE_MAX_HSTATE];
};
static struct node_hstate node_hstates[MAX_NUMNODES];

struct hugepage_subpool {
    spinlock_t lock;
    long count;
    long max_hpages;    /* Maximum huge pages or -1 if no maximum. */
    long used_hpages;   /* Used count against maximum, includes */
                /* both alloced and reserved pages. */
    struct hstate *hstate;
    long min_hpages;    /* Minimum huge pages or -1 if no minimum. */
    long rsv_hpages;    /* Pages reserved against global pool to */
                /* sasitfy minimum size. */
};

struct resv_map {
    struct kref refs;
    spinlock_t lock;
    struct list_head regions;
    long adds_in_progress;
    struct list_head region_cache;
    long region_cache_count;
};
```


## hugepage_subpool

hugepage_put_subpool 和 hugepage_new_subpool 是对应的:
```txt
#0  hugepage_new_subpool (h=0xffffffff834abe20 <hstates>, max_hpages=-1, min_hpages=2) at include/linux/slab.h:600
#1  0xffffffff81432d2b in hugetlbfs_fill_super (sb=0xffff888302e7e000, fc=<optimized out>) at fs/hugetlbfs/inode.c:1359
#2  0xffffffff813207c9 in vfs_get_super (fill_super=0xffffffff81432c90 <hugetlbfs_fill_super>, keying=vfs_get_independent_super, fc=0xffff8883042c29c0) at fs/super.c:1168
#3  get_tree_nodev (fc=0xffff8883042c29c0, fill_super=0xffffffff81432c90 <hugetlbfs_fill_super>) at fs/super.c:1198
#4  0xffffffff8131ee8d in vfs_get_tree (fc=0xffffffff834abe20 <hstates>, fc@entry=0xffff8883042c29c0) at fs/super.c:1530
#5  0xffffffff813482d3 in do_new_mount (data=0xffff88830a771000, name=0xffff888300d273d0 "none", mnt_flags=32, sb_flags=<optimized out>, fstype=0x20 <fixed_percpu_data+32> <error: Cannot acc
ess memory at address 0x20>, path=0xffffc90000cb7ef8) at fs/namespace.c:3040
#6  path_mount (dev_name=dev_name@entry=0xffff888300d273d0 "none", path=path@entry=0xffffc90000cb7ef8, type_page=type_page@entry=0xffff8883086e9f10 "hugetlbfs", flags=<optimized out>, flags@
entry=3236757504, data_page=data_page@entry=0xffff88830a771000) at fs/namespace.c:3370
#7  0xffffffff81348b72 in do_mount (data_page=0xffff88830a771000, flags=3236757504, type_page=0xffff8883086e9f10 "hugetlbfs", dir_name=0x555d2fa642f0 "/mnt/huge", dev_name=0xffff888300d273d0
 "none") at fs/namespace.c:3383
#8  __do_sys_mount (data=<optimized out>, flags=3236757504, type=<optimized out>, dir_name=0x555d2fa642f0 "/mnt/huge", dev_name=<optimized out>) at fs/namespace.c:3591
#9  __se_sys_mount (data=<optimized out>, flags=3236757504, type=<optimized out>, dir_name=93858719744752, dev_name=<optimized out>) at fs/namespace.c:3568
#10 __x64_sys_mount (regs=<optimized out>) at fs/namespace.c:3568
#11 0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000cb7f58) at arch/x86/entry/common.c:50
```

- [ ] hugepage_subpool_get_pages 和 hugepage_subpool_put_pages 是如何使用的

## inode

```txt
#0  hugetlbfs_create (mnt_userns=0xffffffff82a618e0 <init_user_ns>, dir=0xffff88830c764010, dentry=0xffff8883081b39c0, mode=33188, excl=false) at fs/hugetlbfs/inode.c:931
#1  0xffffffff8132eb98 in lookup_open (op=0xffffc9000237fedc, op=0xffffc9000237fedc, got_write=true, file=0xffff88830591ff00, nd=0xffffc9000237fdc0) at fs/namei.c :3413
#2  open_last_lookups (op=0xffffc9000237fedc, file=0xffff88830591ff00, nd=0xffffc9000237fdc0) at fs/namei.c:3481
#3  path_openat (nd=nd@entry=0xffffc9000237fdc0, op=op@entry=0xffffc9000237fedc, flags=flags@entry=65) at fs/namei.c:3688
#4  0xffffffff8132fd0d in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff8883020a5000, op=op@entry=0xffffc9000237fedc) at fs/namei.c:3718
#5  0xffffffff813198d5 in do_sys_openat2 (dfd=dfd@entry=-100, filename=<optimized out>, how=how@entry=0xffffc9000237ff18) at fs/open.c:1311
#6  0xffffffff81319cb0 in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=-100) at fs/open.c:1327
#7  __do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>) at fs/open.c:1335
#8  __se_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>) at fs/open.c:1331
#9  __x64_sys_open (regs=<optimized out>) at fs/open.c:1331
#10 0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc9000237ff58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc9000237ff58, nr=<optimized out>) at arch/x86/entry/common.c:80
#12 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

- `hugetlbfs_file_operations` 是没有注册对应的 write 操作的，只有 `hugetlbfs_read_iter` 和 `hugetlbfs_file_mmap` 的操作，

使用 echo aaa > a 会失败的

```txt
#0  hugetlbfs_read_iter (iocb=0xffffc90002e67e98, to=0xffffc90002e67e70) at fs/hugetlbfs/inode.c:290
#1  0xffffffff8131ce0c in call_read_iter (iter=0xffffc90002e67e70, kio=0xffffc90002e67e98, file=0xffff88830c7d5300) at include/linux/fs.h:2181
#2  new_sync_read (ppos=0xffffc90002e67f08, len=1073741824, buf=0x7f8f4d05f000 <error: Cannot access memory at address 0x7f8f4d05f000>, filp=0xffff88830c7d5300) at fs/read_write.c:389
#3  vfs_read (file=file@entry=0xffff88830c7d5300, buf=buf@entry=0x7f8f4d05f000 <error: Cannot access memory at address 0x7f8f4d05f000>, count=count@entry=1073741824, pos=pos@entry=0xffffc90002e67f08) at fs/read_write.c:470
#4  0xffffffff8131d5da in ksys_read (fd=<optimized out>, buf=0x7f8f4d05f000 <error: Cannot access memory at address 0x7f8f4d05f000>, count=1073741824) at fs/read_write.c:607
#5  0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90002e67f58) at arch/x86/entry/common.c:50
#6  do_syscall_64 (regs=0xffffc90002e67f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#7  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#8  0x0000000000000fff in ?? ()
#9  0x0000000000000000 in ?? ()
```

## [ ] 如何处理 rmap 的情况，需要单独处理吗

## resv_map 面对文件的伸缩，如何处理
- [x] file : 直接预留，根本无需 reserve 的
- [ ] shared 的
- [ ] private 的

在 remap.c 中的确是处理过

- hugetlb_reserve_pages : 如果不是 shared 的，那么此时才创建 resv_map

- [ ] 这段注释我无法看懂 ?
```c
/*
 * These helpers are used to track how many pages are reserved for
 * faults in a MAP_PRIVATE mapping. Only the process that called mmap()
 * is guaranteed to have their future faults succeed.
 *
 * With the exception of reset_vma_resv_huge_pages() which is called at fork(),
 * the reserve counters are updated with the hugetlb_lock held. It is safe
 * to reset the VMA at fork() time as it is not in use yet and there is no
 * chance of the global counters getting corrupted as a result of the values.
 *
 * The private mapping reservation is represented in a subtly different
 * manner to a shared mapping.  A shared mapping has a region map associated
 * with the underlying file, this region map represents the backing file
 * pages which have ever had a reservation assigned which this persists even
 * after the page is instantiated.  A private mapping has a region map
 * associated with the original mmap which is attached to all VMAs which
 * reference it, this region map represents those offsets which have consumed
 * reservation ie. where pages have been instantiated.
 */
static unsigned long get_vma_private_data(struct vm_area_struct *vma)
{
    return (unsigned long)vma->vm_private_data;
}
```

## zero page

hugetlb_reserve_pages 中有一个注释
```c
    /*
     * Shared mappings base their reservation on the number of pages that
     * are already allocated on behalf of the file. Private mappings need
     * to reserve the full area even if read-only as mprotect() may be
     * called to make the mapping read-write. Assume !vma is a shm mapping
     */
```
所以，无论是 fallcoate 还是 anon 映射，都是有 reserve 的。

## syscontrol
```c
int hugetlb_sysctl_handler(struct ctl_table *, int, void __user *, size_t *, loff_t *);
int hugetlb_overcommit_handler(struct ctl_table *, int, void __user *, size_t *, loff_t *);
int hugetlb_treat_movable_handler(struct ctl_table *, int, void __user *, size_t *, loff_t *);

#ifdef CONFIG_NUMA
int hugetlb_mempolicy_sysctl_handler(struct ctl_table *, int,
                    void __user *, size_t *, loff_t *);
#endif

unsigned long hugetlb_total_pages(void);
```

## misc
```c
int hugetlb_reserve_pages(struct inode *inode, long from, long to,
                        struct vm_area_struct *vma,
                        vm_flags_t vm_flags);
long hugetlb_unreserve_pages(struct inode *inode, long start, long end,
                        long freed);
bool isolate_huge_page(struct page *page, struct list_head *list);
void putback_active_hugepage(struct page *page);
void move_hugetlb_state(struct page *oldpage, struct page *newpage, int reason);
void free_huge_page(struct page *page);
void hugetlb_fix_reserve_counts(struct inode *inode);
```





## 从用户层角度分析

```c
static inline bool is_file_hugepages(struct file *file)
{
    if (file->f_op == &hugetlbfs_file_operations) // 首先阅读一下 hugetlbfs 的作用吧
        return true;

    return is_file_shm_hugepages(file);
}
```

## `hugetlbfs_setattr` 的操作比想象的复杂
- 似乎只是看到了 memory 的大小，同时没有看到

## 和 vm 打交道的关键的外部接口
- unmap_single_vma : 将其中的 pagetable 拆掉
  - `__unmap_hugepage_range`
- vm_operations_struct::close -> hugetlb_vm_op_close 处理 cgroup 之类的事情

```txt
#0  __unmap_hugepage_range (tlb=tlb@entry=0xffffc90001d27df8, vma=vma@entry=0xffff888228c85480, start=start@entry=139726484930560, end=end@entry=139726501707776, ref_page=ref_page@entry=0x0 <fixed_percpu_data>, zap_flags=<optimized out>) at mm/hugetlb.c:4997
#1  0xffffffff812f6c59 in __unmap_hugepage_range_final (tlb=tlb@entry=0xffffc90001d27df8, vma=vma@entry=0xffff888228c85480, start=start@entry=139726484930560, end=end@entry=139726501707776, ref_page=ref_page@entry=0x0 <fixed_percpu_data>, zap_flags=zap_flags@entry=1) at mm/hugetlb.c:5140
#2  0xffffffff812bad8a in unmap_single_vma (tlb=tlb@entry=0xffffc90001d27df8, vma=vma@entry=0xffff888228c85480, start_addr=start_addr@entry=0, end_addr=end_addr@entry=18446744073709551615, details=details@entry=0xffffc90001d27d88) at mm/memory.c:1689
#3  0xffffffff812bb21d in unmap_vmas (tlb=tlb@entry=0xffffc90001d27df8, vma=0xffff888228c85480, vma@entry=0xffff888228c859c0, start_addr=start_addr@entry=0, end_addr=end_addr@entry=18446744073709551615) at mm/memory.c:1731
#4  0xffffffff812c852f in exit_mmap (mm=mm@entry=0xffff8881001ad800) at mm/mmap.c:3113
#5  0xffffffff810ff6cd in __mmput (mm=0xffff8881001ad800) at kernel/fork.c:1187
#6  mmput (mm=mm@entry=0xffff8881001ad800) at kernel/fork.c:1208
#7  0xffffffff811085bb in exit_mm () at kernel/exit.c:510
#8  do_exit (code=code@entry=0) at kernel/exit.c:782
#9  0xffffffff81108e38 in do_group_exit (exit_code=0) at kernel/exit.c:925
#10 0xffffffff81108eaf in __do_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:936
#11 __se_sys_exit_group (error_code=<optimized out>) at kernel/exit.c:934
#12 __x64_sys_exit_group (regs=<optimized out>) at kernel/exit.c:934
#13 0xffffffff81ea93c8 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001d27f58) at arch/x86/entry/common.c:50
#14 do_syscall_64 (regs=0xffffc90001d27f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#15 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#16 0x0000000000000000 in ?? ()
```

- [x] 普通的页面没有 hugetlb_vm_op_close，那么其是如何被 cgroup 控制的

应该是在 mem_cgroup_uncharge 中处理的，当释放 page 的时候，是可以知道该 page 被那个 memcg 管理，从而知道进行释放。
```txt
#0  mem_cgroup_uncharge (folio=0xffffea0004897740) at include/linux/memcontrol.h:706
#1  __folio_put_small (folio=0xffffea0004897740) at mm/swap.c:104
#2  __folio_put (folio=0xffffea0004897740) at mm/swap.c:128
#3  0xffffffff812e648b in folio_put (folio=<optimized out>) at include/linux/mm.h:1125
#4  0xffffffff812ca7be in __tlb_remove_table (table=<optimized out>) at arch/x86/include/asm/tlb.h:34
#5  __tlb_remove_table_free (batch=0xffff88812251d000) at mm/mmu_gather.c:114
#6  tlb_remove_table_rcu (head=0xffff88812251d000) at mm/mmu_gather.c:169
#7  0xffffffff811812b4 in rcu_do_batch (rdp=0xffff88813bc2bd00) at kernel/rcu/tree.c:2245
#8  rcu_core () at kernel/rcu/tree.c:2505
#9  0xffffffff822000e1 in __do_softirq () at kernel/softirq.c:571
#10 0xffffffff81109dda in invoke_softirq () at kernel/softirq.c:445
#11 __irq_exit_rcu () at kernel/softirq.c:650
#12 0xffffffff81ead0b2 in sysvec_apic_timer_interrupt (regs=0xffffc90001d27d98) at arch/x86/kernel/apic/apic.c:1106
```
但是，由于 hugetlb 的 free 过程是自营的，也就是 `__unmap_hugepage_range`，所以需要  vm_operations_struct::close

## 细节
- flush_free_hpage_work ：为什么额外的需要 workfn 来处理

## TODO
- https://biscuitos.github.io/blog/Hugetlbfs/
- https://www.cnblogs.com/arnoldlu/p/14028825.html

通过这个可以检查是否支持 1GB 大页:
- cat /proc/cpuinfo | grep pdpe1gb | head -n 1

但是不知道为什么，现在的 alpine.sh 拉起来的虚拟机中并不能支持 1gb 大页

## hugepage anon mmap 的时候，大页的大小是不可以混合使用的

虽然 mmap 的时候，可以创建多个 flags 的:
```c
#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)
```

- ksys_mmap_pgoff
  - hstate_sizelog
    - for_each_hstate : 从小到大匹配，找到第一个就是返回的

## 动态申请
```c
// 继续分析其调用者
unsigned long reclaim_clean_pages_from_list(struct zone *zone,
                        struct list_head *page_list)
```

## 测试一下性能
- https://easyperf.net/blog/2022/09/01/Utilizing-Huge-Pages-For-Code

## 测试一下，这集中方法是不是可以帮助分配更加多的大页
- 减少失败的方法:
  - migration ?
  - drop cache ? 可以每一个 node 上操作吗?
  - 手动触发 compaction 直接

## 看错了，构建大页的过程中，会导致 cache 的出现的
但是也不是将所有的内存都赶到 swap 中，现在不知道是什么原因

## 大页的 migration 如何
