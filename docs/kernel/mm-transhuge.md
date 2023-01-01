# THP
- 原来的那个文章找过来看看

- [ ] PageDoubleMap
- [x] THP only support PMD ? so can it support more than 2M space (21bit) ?

使用 transparent hugepage 的原因:
1. TLB 的覆盖更大，可以降低 TLB miss rate
3. hugepage 的出现让原先的 page walk 的路径变短了

几个 THP 需要考虑的核心问题:
1. swap
2. reference 的问题
3. split 和 merge

## 使用接口

在 /sys/kernel/mm/transparent_hugepage/ 下

```txt
├── defrag
├── enabled
├── hpage_pmd_size
├── khugepaged
│   ├── alloc_sleep_millisecs
│   ├── defrag
│   ├── full_scans
│   ├── max_ptes_none
│   ├── max_ptes_shared
│   ├── max_ptes_swap
│   ├── pages_collapsed
│   ├── pages_to_scan
│   └── scan_sleep_millisecs
├── shmem_enabled
└── use_zero_page
```
/tmp 的 hugepage 模型是 mount 的时候确定，但是 shmat() 和 anon share 是通过这个接口的

🧀  cat shmem_enabled
always within_size advise [never] deny force

🧀  cat defrag
always defer defer+madvise [madvise] never

🧀  cat enabled
always madvise [never]

#### 内核文档
[用户手册](https://www.kernel.org/doc/html/latest/admin-guide/mm/transhuge.html)

The THP behaviour is controlled via `sysfs` interface and using `madvise(2)` and `prctl(2)` system calls.
> 其中 prctl 可以让一个程序直接 disable 掉 hugepage ，从而规避系统的设置

## TODO
```c
/*
 * By default, transparent hugepage support is disabled in order to avoid
 * risking an increased memory footprint for applications that are not
 * guaranteed to benefit from it. When transparent hugepage support is
 * enabled, it is for all mappings, and khugepaged scans all mappings.
 * Defrag is invoked by khugepaged hugepage allocations and by page faults
 * for all hugepage allocations.
 */
unsigned long transparent_hugepage_flags __read_mostly =
#ifdef CONFIG_TRANSPARENT_HUGEPAGE_ALWAYS
	(1<<TRANSPARENT_HUGEPAGE_FLAG)|
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE_MADVISE
	(1<<TRANSPARENT_HUGEPAGE_REQ_MADV_FLAG)|
#endif
	(1<<TRANSPARENT_HUGEPAGE_DEFRAG_REQ_MADV_FLAG)|
	(1<<TRANSPARENT_HUGEPAGE_DEFRAG_KHUGEPAGED_FLAG)|
	(1<<TRANSPARENT_HUGEPAGE_USE_ZERO_PAGE_FLAG);
```

- [ ]  page cache can't work with THP ?

## defrag
  - [ ] /sys/kernel/mm/transparent_hugepage/defrag 的 always 无法理解，或者说，什么时候应该触发 defrag, 不是分配的时候就是决定了吗 ?
- [ ] THP has to defrag pages, so check the compaction.c and find out how thp deal with it !
  - [ ] how defrag wake kcompactd ?

## page cache
- [ ] git show d68eccad370665830e16e5c77611fde78cd749b3
- [ ] 分析下 `__filemap_add_folio`

[Transparent huge pages for filesystems](https://lwn.net/Articles/789159/)

> It is using the [Binary Optimization and Layout Tool (BOLT)](https://github.com/facebookincubator/BOLT) to profile its code in order to identify the hot functions. Those functions are collected up into an 8MB region in the generated executable.

## [ ] swap
顺着 CONFIG_THP_SWAP 找找

## shmem hugepage : 让 tmpfs 使用上 hugepage
https://stackoverflow.com/questions/67991417/how-to-use-hugepages-with-tmpfs

```sh
sudo mount -t tmpfs -osize=6G,mode=1777,huge=always tmpfs mem
```

mount 的过程中的一个调用:
```txt
#0  shmem_parse_one (fc=0xffff8881424ebc00, param=0xffffc90001247e50) at mm/shmem.c:3437
#1  0xffffffff8139c8bb in vfs_parse_fs_param (param=0xffffc90001247e50, fc=0xffff8881424ebc00) at fs/fs_context.c:146
#2  vfs_parse_fs_param (fc=0xffff8881424ebc00, param=0xffffc90001247e50) at fs/fs_context.c:127
#3  0xffffffff8139c9a2 in vfs_parse_fs_string (fc=fc@entry=0xffff8881424ebc00, key=key@entry=0xffffffff8284d0eb "source", value=value@entry=0xffff888144681f58 "tmpfs", v_size=<optimized out>) at fs/fs_context.c:184
#4  0xffffffff8138588b in do_new_mount (data=0xffff888147ac3000, name=0xffff888144681f58 "tmpfs", mnt_flags=32, sb_flags=<optimized out>, fstype=0x20 <fixed_percpu_data+32> <error: Cannot access memory at address 0x20>, path=0xffffc90001247ef8) at fs/namespace.c:3034
#5  path_mount (dev_name=dev_name@entry=0xffff888144681f58 "tmpfs", path=path@entry=0xffffc90001247ef8, type_page=type_page@entry=0xffff888144681f18 "tmpfs", flags=<optimized out>, flags@entry=3236757504, data_page=data_page@entry=0xffff888147ac3000) at fs/namespace.c:3370
#6  0xffffffff81386162 in do_mount (data_page=0xffff888147ac3000, flags=3236757504, type_page=0xffff888144681f18 "tmpfs", dir_name=0x563274087540 "/home/martins3/g", dev_name=0xffff888144681f58 "tmpfs") at fs/namespace.c:3383
#7  __do_sys_mount (data=<optimized out>, flags=3236757504, type=<optimized out>, dir_name=0x563274087540 "/home/martins3/g", dev_name=<optimized out>) at fs/namespace.c:3591
#8  __se_sys_mount (data=<optimized out>, flags=3236757504, type=<optimized out>, dir_name=94774695064896, dev_name=<optimized out>) at fs/namespace.c:3568
#9  __x64_sys_mount (regs=<optimized out>) at fs/namespace.c:3568
#10 0xffffffff81f4270b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001247f58) at arch/x86/entry/common.c:50
#11 do_syscall_64 (regs=0xffffc90001247f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#12 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

1. 显示的 mount :
```txt
#0  shmem_fill_super (sb=0xffff8883d8af0000, fc=0xffff888104de2600) at mm/shmem.c:3753
#1  0xffffffff813cb606 in vfs_get_super (fc=0xffff888104de2600, reconf=<optimized out>, test=<optimized out>, fill_super=0xffffffff81300550 <shmem_fill_super>) at fs/super.c:1128
#2  0xffffffff813c92e1 in vfs_get_tree (fc=0xffff8883d8af0000, fc@entry=0xffff888104de2600) at fs/super.c:1489
#3  0xffffffff813f6157 in do_new_mount (data=0xffff88811d513000, name=0xffff88810006ec58 "tmpfs", mnt_flags=32, sb_flags=<optimized out>, fstype=0x20 <fixed_percpu_data+32> <error: Cannot access memory at address 0x20>, path=0xffffc900431ebef8) at fs/namespace.c:3145
#4  path_mount (dev_name=dev_name@entry=0xffff88810006ec58 "tmpfs", path=path@entry=0xffffc900431ebef8, type_page=type_page@entry=0xffff88810006ec50 "tmpfs", flags=<optimized out>, flags@entry=0, data_page=data_page@entry=0xffff88811d513000) at fs/namespace.c:3475
#5  0xffffffff813f6a16 in do_mount (data_page=0xffff88811d513000, flags=0, type_page=0xffff88810006ec50 "tmpfs", dir_name=0x55b015d3c540 "/root/tmp", dev_name=0xffff88810006ec58 "tmpfs") at fs/namespace.c:3488
#6  __do_sys_mount (data=<optimized out>, flags=0, type=<optimized out>, dir_name=0x55b015d3c540 "/root/tmp", dev_name=<optimized out>) at fs/namespace.c:3697
#7  __se_sys_mount (data=<optimized out>, flags=0, type=<optimized out>, dir_name=94214768805184, dev_name=<optimized out>) at fs/namespace.c:3674
#8  __x64_sys_mount (regs=<optimized out>) at fs/namespace.c:3674
#9  0xffffffff82189c3c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc900431ebf58) at arch/x86/entry/common.c:50
#10 do_syscall_64 (regs=0xffffc900431ebf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#11 0xffffffff822000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
2. shmem_init : 为 anon shared 和 shmem 的构建的，用户态看不到。

#### THP kernel
- mmap 和配合 hugetlb 使用的

- [ ] huge_memory.c 用于处理 split 和 各种参数
- [ ] khugepaged.c 用于 scan page 将 base page 转化为 hugepage
- [ ] 内核态分析: 透明的性质在于 `__handle_mm_fault` 中间就开始检查是否可以 由于 hugepage 会修改 page walk ，所以 pud_none 和 `__transparent_hugepage_enabled`
  - [ ] 检查更多的细节

- [ ] 从 madvise 到启动 THP
    - [ ] hugepage_vma_check : 到底那些 memory 不适合 thp
    - [x] `__khugepaged_enter` : 将所在的 mm_struct 放到 list 上，等待之后 khugepaged 会将该区域清理赶紧

- [ ] collapse_file : 处理 page cache / shmem / tmpfs
  - [ ] *caller*
      - [ ] khugepaged_scan_file
          - [ ] khugepaged_scan_mm_slot

- [ ] /sys/kernel/mm/transparent_hugepage 的真正含义 ?
    - [x] khugepaged_enter : 这是判断是否将该区域用于 transparent 的开始位置，[Transparent huge pages for filesystems](https://lwn.net/Articles/789159/) 中来看，现在支持 THP 只有 transparent hugepage 和 tmp memory 了
        - [x] do_huge_pmd_anonymous_page : 在 page fault 的时候，会首先进行 hugepage 检查，如果是 always, 那么**所有的 vma 都会被转换为 transparent hugepage**
            - [x] create_huge_pmd <= `__handle_mm_fault`

- [ ] 好吧，transparent hugepage 只是支持 pmd(从 /proc/meminfo 的 HugePagesize 和 /sys/kernel/mm/transparent_hugepage/hpage_pmd_size)，但是实际上 pud 也是支持的.

关键问题 B : split_huge_page_to_list

不关键问题 A : vm_operations_struct::huge_fault 和 DAX 的关系不一般
不关键问题 A2 : vm_operations_struct 几乎没有一个可以理解的


## reference counting
- [ ] total_mapcount

[Transparent huge page reference counting](https://lwn.net/Articles/619738/)

> In particular, he has eliminated the hard separation between normal and huge pages in the system.
> In current kernels, a specific 4KB page can be treated as an individual page,
> or it can be part of a huge page, but not both. If a huge page must be split into individual pages, it is split completely for all users,
> the compound page structure is torn down, and the huge page no longer exists.
> The fundamental change in Kirill's patch set is to allow a huge page to be split in one process's address space, while remaining a huge page in any other address space where it is found.

龟龟，split page 让共享的物理页，在一个映射中是 hugepage，在另一个的映射中分散的

```c
/*
 * Mapcount of 0-order page; when compound sub-page, includes
 * compound_mapcount of compound_head of page.
 *
 * Result is undefined for pages which cannot be mapped into userspace.
 * For example SLAB or special types of pages. See function page_has_type().
 * They use this place in struct page differently.
 */
static inline int page_mapcount(struct page *page)
{
	int mapcount = atomic_read(&page->_mapcount) + 1;

	if (likely(!PageCompound(page)))
		return mapcount;
	page = compound_head(page);
	return head_compound_mapcount(page) + mapcount;
}

int total_compound_mapcount(struct page *head);

/**
 * folio_mapcount() - Calculate the number of mappings of this folio.
 * @folio: The folio.
 *
 * A large folio tracks both how many times the entire folio is mapped,
 * and how many times each individual page in the folio is mapped.
 * This function calculates the total number of times the folio is
 * mapped.
 *
 * Return: The number of times this folio is mapped.
 */
static inline int folio_mapcount(struct folio *folio)
{
	if (likely(!folio_test_large(folio)))
		return atomic_read(&folio->_mapcount) + 1;
	return total_compound_mapcount(&folio->page);
}
```

### 深入理解 struct page
```c
		struct {	/* Tail pages of compound page */
			unsigned long compound_head;	/* Bit zero is set */

			/* First tail page only */
			unsigned char compound_dtor;
			unsigned char compound_order;
			atomic_t compound_mapcount;
			atomic_t subpages_mapcount;
			atomic_t compound_pincount;
#ifdef CONFIG_64BIT
			unsigned int compound_nr; /* 1 << compound_order */
#endif
		};
		struct {	/* Second tail page of transparent huge page */
			unsigned long _compound_pad_1;	/* compound_head */
			unsigned long _compound_pad_2;
			/* For both global and memcg */
			struct list_head deferred_list;
		};
		struct {	/* Second tail page of hugetlb page */
			unsigned long _hugetlb_pad_1;	/* compound_head */
			void *hugetlb_subpool;
			void *hugetlb_cgroup;
			void *hugetlb_cgroup_rsvd;
			void *hugetlb_hwpoison;
			/* No more space on 32-bit: use third tail if more */
		};
```

分析下 first tail page 是如何使用的:

- prep_compound_page
  - `__SetPageHead`
  - prep_compound_head
  - prep_compound_tail

#### compound_dtor
- compound_dtor: 可以用来区分是那种类型的 page，例如在 PageHuge

- destroy_large_folio

看来这个时间上是存在三个 page 的；
```c
compound_page_dtor * const compound_page_dtors[NR_COMPOUND_DTORS] = {
	[NULL_COMPOUND_DTOR] = NULL,
	[COMPOUND_PAGE_DTOR] = free_compound_page,
#ifdef CONFIG_HUGETLB_PAGE
	[HUGETLB_PAGE_DTOR] = free_huge_page,
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	[TRANSHUGE_PAGE_DTOR] = free_transhuge_page,
#endif
};
```

观察两个 hook 的使用:

-- free_transhuge_page
```txt
@[
    free_transhuge_page+1
    release_pages+491
    tlb_batch_pages_flush+61
    tlb_finish_mmu+101
    unmap_region+218
    do_mas_align_munmap+800
    do_mas_munmap+215
    mmap_region+260
    do_mmap+980
    vm_mmap_pgoff+218
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 339
```
-- free_compound_page
```txt
@[
    free_compound_page+1
    skb_release_data+202
    consume_skb+57
    unix_stream_read_generic+2326
    unix_stream_recvmsg+136
    ____sys_recvmsg+135
    ___sys_recvmsg+124
    __sys_recvmsg+86
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 6320
```

```txt
@[
    destroy_large_folio+1
    release_pages+491
    __pagevec_release+27
    shmem_undo_range+692
    shmem_evict_inode+262
    evict+204
    __dentry_kill+223
    __fput+221
    task_work_run+86
    do_exit+835
    do_group_exit+45
    get_signal+2423
    arch_do_signal_or_restart+54
    exit_to_user_mode_prepare+267
    syscall_exit_to_user_mode+23
    do_syscall_64+72
    entry_SYSCALL_64_after_hwframe+99
]: 106
@[
    destroy_large_folio+1
    skb_release_data+202
    consume_skb+57
    unix_stream_read_generic+2326
    unix_stream_recvmsg+136
    ____sys_recvmsg+135
    ___sys_recvmsg+124
    __sys_recvmsg+86
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 125
```
### 分析下 compound page

- free_transhuge_page
  - free_compound_page

验证一个基本想法，那就是 compound_page 就是连续的几个小页面而已。
```txt
$ p page[1].compound_order
$2 = 3 '\003'
$ bt
#0  compound_order (page=0xffffea00048d7600) at ./include/linux/mm.h:721
#1  free_compound_page (page=0xffffea00048d7600) at mm/page_alloc.c:773
#2  0xffffffff81bd827d in folio_put (folio=<optimized out>) at ./include/linux/mm.h:1250
#3  put_page (page=0xffffea00048d7600) at ./include/linux/mm.h:1319
#4  page_to_skb (vi=vi@entry=0xffff8883c1af6900, rq=rq@entry=0xffff888102526000, page=page@entry=0xffffea00048d7600, offset=<optimized out>, len=<optimized out>, len@entry=66, truesize=2048, hdr_valid=true, metasize=0, headroom=0) at drivers/net/virtio_net.c:558
#5  0xffffffff81bdb0a6 in receive_mergeable (stats=0xffffc900022d0e80, xdp_xmit=<optimized out>, len=<optimized out>, ctx=<optimized out>, buf=<optimized out>, rq=0xffff888102526000, vi=0xffff8883c1af6900, dev=0xffff8883c1af6000) at drivers/net/virtio_net.c:1126
#6  receive_buf (vi=0xffff8883c1af6900, rq=0xffff888102526000, buf=<optimized out>, len=<optimized out>, ctx=<optimized out>, xdp_xmit=<optimized out>, stats=0xffffc900022d0e80) at drivers/net/virtio_net.c:1265
#7  0xffffffff81bdc910 in virtnet_receive (xdp_xmit=0xffffc900022d0e70, budget=64, rq=0xffff888102526000) at drivers/net/virtio_net.c:1560
#8  virtnet_poll (napi=0xffff888102526008, budget=<optimized out>) at drivers/net/virtio_net.c:1678
#9  0xffffffff81dd8424 in __napi_poll (n=0xffffea00048d7600, n@entry=0xffff888102526008, repoll=repoll@entry=0xffffc900022d0f37) at net/core/dev.c:6485
#10 0xffffffff81dd8974 in napi_poll (repoll=0xffffc900022d0f48, n=0xffff888102526008) at net/core/dev.c:6552
#11 net_rx_action (h=<optimized out>) at net/core/dev.c:6663
#12 0xffffffff821a0e34 in __do_softirq () at kernel/softirq.c:571
#13 0xffffffff811328ca in invoke_softirq () at kernel/softirq.c:445
#14 __irq_exit_rcu () at kernel/softirq.c:650
#15 0xffffffff8218b8dc in common_interrupt (regs=0xffffc900001d7e38, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc900022d1018
```

#### 如何创建的
为什么这个根本无法拦截到任何东西：
```sh
sudo bpftrace -e 'kfunc:prep_compound_page { @reads[args->order] = count(); }'
```


### 判断 page 类型
那么如何判断一个 page 是不是 transparent hugepage 的哇？
```c
/*
 * PageHuge() only returns true for hugetlbfs pages, but not for normal or
 * transparent huge pages.  See the PageTransHuge() documentation for more
 * details.
 */
int PageHuge(struct page *page)
{
	if (!PageCompound(page))
		return 0;

	page = compound_head(page);
	return page[1].compound_dtor == HUGETLB_PAGE_DTOR;
}
```

实际上，这个测试只是测试 head 而已，但是限制了使用范围
```c
static inline bool folio_test_transhuge(struct folio *folio)
{
	return folio_test_head(folio);
}

/*
 * PageHuge() only returns true for hugetlbfs pages, but not for
 * normal or transparent huge pages.
 *
 * PageTransHuge() returns true for both transparent huge and
 * hugetlbfs pages, but not normal pages. PageTransHuge() can only be
 * called only in the core VM paths where hugetlbfs pages can't exist.
 */
static inline int PageTransHuge(struct page *page)
{
	VM_BUG_ON_PAGE(PageTail(page), page);
	return PageHead(page);
}
```

## khugepaged
- [ ] if `kcompactd` compact pages used by hugepage, and defrag pages by `split_huge_page_to_list`, so what's the purpose of khugepaged ?

1. /sys/kernel/mm/transparent_hugepage/enabled => start_stop_khugepaged => khugepaged => khugepaged_do_scan => khugepaged_scan_mm_slot => khugepaged_scan_pmd
2. in `khugepaged_scan_pmd`, we will check pages one by one, if enough base pages are found,  call `collapse_huge_page` to merge base page to huge page
3. `collapse_huge_page` = `khugepaged_alloc_page` + `__collapse_huge_page_copy` + many initialization for huge page + `__collapse_huge_page_isolate` (free base page)

- [x] it seems khugepaged scan pages and collapse it into huge pages, so what's difference between kcompactd
  - khugepaged is consumer of hugepage, it's scan base pages and collapse them
  - [ ] khugepaged 是用于扫描 base page 的 ? It’s the responsibility of khugepaged to then install the THP pages.

#### THP split
这几个文章都是讲解两种方案，很烦!
[Transparent huge pages in the page cache](https://lwn.net/Articles/686690/)
> Finally, a file may be used without being mapped into process memory at all, while anonymous memory is always mapped. So any changes to a filesystem to support transparent huge page mapping must not negatively impact normal read/write performance on an unmapped file.

- [x] 无论是在内核态和用户态中间，一个 huge page 都是可以随意拆分的，在用户态每个人都是不同的映射。在内核态，总是线性映射，pmd page table entry 的修改其实没有任何意义。
- [x] swap cache 的实现根本挑战在于区间的可以随意变化

[Improving huge page handling](https://lwn.net/Articles/636162/)

[Transparent huge page reference counting](https://lwn.net/Articles/619738/)
> In many other situations, Andrea placed a call to split_huge_page(), a function which breaks a huge page down into its component small pages.

> In other words, if split_huge_page() could be replaced by a new function, call it split_huge_pmd(), that would only split up a single process's mapping of a huge page, code needing to deal with individual pages could often be accommodated while preserving the benefits of the huge page for other processes. But, as noted above, the kernel currently does not support different mappings of huge pages; all processes must map the memory in the same way. This restriction comes down to how various parameters — reference counts in particular — are represented in huge pages.

> it must be replaced by a scheme that can track both the mappings to the huge page as a whole and the individual pages that make up that huge page.


```c
#define split_huge_pmd(__vma, __pmd, __address)       \
  do {                \
    pmd_t *____pmd = (__pmd);       \
    if (is_swap_pmd(*____pmd) || pmd_trans_huge(*____pmd) \
          || pmd_devmap(*____pmd))  \
      __split_huge_pmd(__vma, __pmd, __address, \
            false, NULL);   \
  }  while (0)
```

- [ ] split_huge_page_to_list
  - [ ] `__split_huge_page` : 不对劲，似乎 hugepage 只是体现在 struct page 上，而没有体现在 pmd 上
      - [x] 在 huge page 中间拆分出来几个当做其他的 page 正常使用, 虽然从中间抠出来的页面不可以继续当做内核，但是可以给用户使用
          - [ ] 是否存在 flag 说明那些页面可以分配给用户，那些是内核 ?



- [ ] `__split_huge_pmd` : 处理各种 lock 包括 pmd_lock
  - [ ] `__split_huge_pmd_locked`
    - 取回 pmd_huge_pte，向其中填充 pte, 然后将 pmd entry 填充该位置
  - `pgtable_t page::(anonymous union)::(anonymous struct)::pmd_huge_pte`
      - [ ]  从 `__split_huge_pmd_locked` 的代码: `pgtable_trans_huge_withdraw` 看，这一个 page table 从来没有被删除过

## khugepaged

- set_recommended_min_free_kbytes

## zero page
By default kernel tries to use huge zero page on read page fault to anonymous mapping. It’s possible to disable huge zero page by writing 0 or enable it back by writing 1:

Contrary to the zero page's original preferential use,
some modern operating systems such as FreeBSD, Linux and Microsoft Windows[2] actually make the zero page inaccessible to trap uses of null pointers.

## pmd && pud 的含义
1. 如果 vma 是可以写的，那么 pmd 就是设置为可写的

```c
pmd_t maybe_pmd_mkwrite(pmd_t pmd, struct vm_area_struct *vma)
{
	if (likely(vma->vm_flags & VM_WRITE))
		pmd = pmd_mkwrite(pmd);
	return pmd;
}
```

2. 应该是用于 pgfault 的
```c
static void insert_pfn_pud(struct vm_area_struct *vma, unsigned long addr,
		pud_t *pud, pfn_t pfn, pgprot_t prot, bool write)

vm_fault_t vmf_insert_pfn_pud(struct vm_fault *vmf, pfn_t pfn, bool write)
```


## copy_huge_pmd : memory.c 使用，实现 copy_pmd_range
dup_mmap 中间逐级下沉下来的

## follow_devmap_pmd : 被 gup 使用


## deferred_split



## prep_transhuge_page
1. 利用 deferred_split 来收集什么 ?
2. dtor 是什么 ?
```c
void prep_transhuge_page(struct page *page)
{
	/*
	 * we use page->mapping and page->indexlru in second tail page
	 * as list_head: assuming THP order >= 2
	 */
	INIT_LIST_HEAD(page_deferred_list(page));
	set_compound_page_dtor(page, TRANSHUGE_PAGE_DTOR);
}
```

## do_huge_pmd_anonymous_page
```c
/*
 * By the time we get here, we already hold the mm semaphore
 *
 * The mmap_sem may have been released depending on flags and our
 * return value.  See filemap_fault() and __lock_page_or_retry().
 */
static vm_fault_t __handle_mm_fault(struct vm_area_struct *vma,
		unsigned long address, unsigned int flags)
    // pud_none : 只有说 pud 为 none
    // 1. 那么 cow 机制怎么处理 TODO
    // 2. 应该不会处理 swap 机制吧 TODO
	if (pud_none(*vmf.pud) && __transparent_hugepage_enabled(vma)) {  // __transparent_hugepage_enabled 中间会检查其中的各种 enable 机制

static inline vm_fault_t create_huge_pmd(struct vm_fault *vmf)
{
	if (vma_is_anonymous(vmf->vma)) //  只有是 anonymous 或者
		return do_huge_pmd_anonymous_page(vmf); //
	if (vmf->vma->vm_ops->huge_fault)
		return vmf->vma->vm_ops->huge_fault(vmf, PE_SIZE_PMD); // shared
	return VM_FAULT_FALLBACK;
}

static vm_fault_t create_huge_pud(struct vm_fault *vmf)
{
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	/* No support for anonymous transparent PUD pages yet */
	if (vma_is_anonymous(vmf->vma))
		return VM_FAULT_FALLBACK;
	if (vmf->vma->vm_ops->huge_fault)
		return vmf->vma->vm_ops->huge_fault(vmf, PE_SIZE_PUD);
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */
	return VM_FAULT_FALLBACK;
}
```

```c
static vm_fault_t wp_huge_pud(struct vm_fault *vmf, pud_t orig_pud)
{
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	/* No support for anonymous transparent PUD pages yet */  // PUD 一个内容太大了，但是为什么 file 就可以呀 !
	if (vma_is_anonymous(vmf->vma))
		return VM_FAULT_FALLBACK;
	if (vmf->vma->vm_ops->huge_fault)
		return vmf->vma->vm_ops->huge_fault(vmf, PE_SIZE_PUD);
#endif /* CONFIG_TRANSPARENT_HUGEPAGE */
	return VM_FAULT_FALLBACK;
}

/* `inline' is required to avoid gcc 4.1.2 build error */
static inline vm_fault_t wp_huge_pmd(struct vm_fault *vmf, pmd_t orig_pmd)
{
	if (vma_is_anonymous(vmf->vma))
		return do_huge_pmd_wp_page(vmf, orig_pmd);
	if (vmf->vma->vm_ops->huge_fault)
		return vmf->vma->vm_ops->huge_fault(vmf, PE_SIZE_PMD);

	/* COW handled on pte level: split pmd */
	VM_BUG_ON_VMA(vmf->vma->vm_flags & VM_SHARED, vmf->vma);
	__split_huge_pmd(vmf->vma, vmf->pmd, vmf->address, false, NULL);

	return VM_FAULT_FALLBACK;
}
```

## core function : do_huge_pmd_anonymous_page
1. 检查是否 vma 中间是否可以容纳 hugepage
2. 假如可以使用 zero page 机制
3. 利用 alloc_hugepage_direct_gfpmask 计算出来 buddy allocator
4. prep_transhuge_page @todo 不知道干嘛的
5. `__do_huge_pmd_anonymous_page` : 将分配的 page 和 page table 组装

```c
vm_fault_t do_huge_pmd_anonymous_page(struct vm_fault *vmf)


#define alloc_hugepage_vma(gfp_mask, vma, addr, order) \
	alloc_pages_vma(gfp_mask, order, vma, addr, numa_node_id(), true)

void prep_transhuge_page(struct page *page)
{
	/*
	 * we use page->mapping and page->indexlru in second tail page
	 * as list_head: assuming THP order >= 2
	 */

	INIT_LIST_HEAD(page_deferred_list(page));
	set_compound_page_dtor(page, TRANSHUGE_PAGE_DTOR);
}
```

## core function : split_huge_page_to_list

```c
/* Racy check whether the huge page can be split */
bool can_split_huge_page(struct page *page, int *pextra_pins)
{
	int extra_pins;

	/* Additional pins from page cache */
	if (PageAnon(page))
		extra_pins = PageSwapCache(page) ? HPAGE_PMD_NR : 0; // TODO 难道 swap 也要处理 hugepage
	else
		extra_pins = HPAGE_PMD_NR;
	if (pextra_pins)
		*pextra_pins = extra_pins;
	return total_mapcount(page) == page_count(page) - extra_pins - 1;
}
```


## core function : 合并



## anonymous 和 shared 都是如何初始化的


## 如何判断


```c
bool transparent_hugepage_enabled(struct vm_area_struct *vma)
{
	/* The addr is used to check if the vma size fits */
	unsigned long addr = (vma->vm_end & HPAGE_PMD_MASK) - HPAGE_PMD_SIZE;

	if (!transhuge_vma_suitable(vma, addr))
		return false;
	if (vma_is_anonymous(vma))
		return __transparent_hugepage_enabled(vma);
	if (vma_is_shmem(vma))
		return shmem_huge_enabled(vma);

	return false;
}
```

## page fault

```txt
#0  __do_huge_pmd_anonymous_page (gfp=<optimized out>, page=<optimized out>, vmf=<optimized out>) at mm/huge_memory.c:837
#1  do_huge_pmd_anonymous_page (vmf=vmf@entry=0xffffc900017afdf8) at mm/huge_memory.c:837
#2  0xffffffff812dcec8 in create_huge_pmd (vmf=0xffffc900017afdf8) at mm/memory.c:4820
#3  __handle_mm_fault (vma=vma@entry=0xffff888125587da8, address=address@entry=140581166645248, flags=flags@entry=597) at mm/memory.c:5067
#4  0xffffffff812dd680 in handle_mm_fault (vma=0xffff888125587da8, address=address@entry=140581166645248, flags=flags@entry=597, regs=regs@entry=0xffffc900017aff58) at mm/memory.c:5218
#5  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc900017aff58, error_code=error_code@entry=6, address=address@entry=140581166645248) at arch/x86/mm/fault.c:1428
#6  0xffffffff81fa8f72 in handle_page_fault (address=140581166645248, error_code=6, regs=0xffffc900017aff58) at arch/x86/mm/fault.c:1519
#7  exc_page_fault (regs=0xffffc900017aff58, error_code=6) at arch/x86/mm/fault.c:1575
#8  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

- `__handle_mm_fault`
  - do_huge_pmd_anonymous_page
    - transhuge_vma_suitable : 检查是否 vma 中间是否可以容纳 hugepage
    - 假如可以使用 zero page 机制
    - vma_thp_gfp_mask : 根据 vma 获取 gfp
    - vma_alloc_folio : 分配 page
      - `__do_huge_pmd_anonymous_page` : 将分配的 page 和 page table 组装
  - vmf->vma->vm_ops->huge_fault : 文件映射，如果文件系统注册了

## ShmemHugePages 在我的个人机器上总是不为 0

设置参数为 transparent_hugepage=never，但是结果为：

```txt
AnonHugePages:         0 kB
ShmemHugePages:   845824 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
```

检查那些进程在使用大页:
```sh
sudo grep -e AnonHugePages  /proc/*/smaps | awk  '{ if($2>4) print $0} ' |  awk -F "/"  '{print $0; system("ps -fp " $3)} '
```

```sh
sudo grep -e ShmemHugePages /proc/*/smaps | awk  '{ if($2>4) print $0} ' |  awk -F "/"  '{print $0; system("ps -fp " $3)} '
```

检查不到任何进程使用过 thp，但是
- shmem_add_to_page_cache
  - 检查 cat /proc/vmstat 发现有很多

所以，应该是显卡驱动的问题:
```txt
@[
    shmem_add_to_page_cache+1
    shmem_get_folio_gfp+580
    shmem_read_mapping_page_gfp+75
    shmem_sg_alloc_table+364
    shmem_get_pages+182
    __i915_gem_object_get_pages+56
    i915_gem_set_domain_ioctl+616
    drm_ioctl_kernel+178
    drm_ioctl+479
    __x64_sys_ioctl+135
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 64977
```

## Transparent hugepage 是如何进行 lru 的

## rmap 如何支持 thp

## idle page tracking 为什么无法处理 thp ?

## 可以主动释放掉 thp 为普通页吗?
