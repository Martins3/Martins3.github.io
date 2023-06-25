# Transhuge

涉及文件 huge_memory.c 和 khugepaged.c

- [ ] PageDoubleMap
- [x] THP only support PMD ? so can it support more than 2M space (21bit) ?

使用 transparent hugepage 的原因:
1. TLB 的覆盖更大，可以降低 TLB miss rate
3. hugepage 的出现让原先的 page walk 的路径变短了

几个 THP 需要考虑的核心问题:
1. swap
2. reference 的问题
3. split 和 merge


2. thp page 是 PageLRU 的吗?
  - 是的，例如 move_folios_to_lru
3. 可以主动释放掉 thp 为普通页吗?
  - 可以，例如使用 madvise 和 prctl 可以释放指定进程的，或者特定 vma 的
  - 但是，不存在一下子释放整个系统的 transhuge 的行为

## 内核文档
[用户手册](https://www.kernel.org/doc/html/latest/admin-guide/mm/transhuge.html)

The THP behaviour is controlled via `sysfs` interface and using `madvise(2)` and `prctl(2)` system calls.

1. 其中 prctl 可以让一个程序直接 disable 掉 hugepage ，从而规避系统的设置，具体参考 `PR_SET_THP_DISABLE`
2. shmem_enabled : The mount is used for SysV SHM, memfds, shared anonymous mmaps (of /dev/zero or MAP_ANONYMOUS), GPU drivers' DRM objects, Ashmem.

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

- [ ] 为什么 anonymous 的不支持 transparet hugepage 的啊?
```c
static vm_fault_t wp_huge_pud(struct vm_fault *vmf, pud_t orig_pud)
{
#if defined(CONFIG_TRANSPARENT_HUGEPAGE) &&			\
	defined(CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD)
	vm_fault_t ret;

	/* No support for anonymous transparent PUD pages yet */
	if (vma_is_anonymous(vmf->vma))
		goto split;
	if (vmf->vma->vm_flags & (VM_SHARED | VM_MAYSHARE)) {
		if (vmf->vma->vm_ops->huge_fault) {
			ret = vmf->vma->vm_ops->huge_fault(vmf, PE_SIZE_PUD);
			if (!(ret & VM_FAULT_FALLBACK))
				return ret;
		}
	}
split:
	/* COW or write-notify not handled on PUD level: split pud.*/
	__split_huge_pud(vmf->vma, vmf->pud, vmf->address);
#endif /* CONFIG_TRANSPARENT_HUGEPAGE && CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD */
	return VM_FAULT_FALLBACK;
}
```

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

参考内核文档 [Transparent Hugepage Support](http://127.0.0.1:3434/mm/transhuge.html)

> - get_page()/put_page() and GUP operate on head page's ->_refcount.
>
>  - ->_refcount in tail pages is always zero: get_page_unless_zero() never
>    succeeds on tail pages.
>
>  - map/unmap of PMD entry for the whole compound page increment/decrement
>    ->compound_mapcount, stored in the first tail page of the compound page;
>    and also increment/decrement ->subpages_mapcount (also in the first tail)
>    by COMPOUND_MAPPED when compound_mapcount goes from -1 to 0 or 0 to -1.
>
>  - map/unmap of sub-pages with PTE entry increment/decrement ->_mapcount
>    on relevant sub-page of the compound page, and also increment/decrement
>    ->subpages_mapcount, stored in first tail page of the compound page, when
>    _mapcount goes from -1 to 0 or 0 to -1: counting sub-pages mapped by PTE.

- refcount 只是 head 维护
- 整体的 map count 在 compound_mapcount 中维护
- mapcount 每一个 page 都需要维护，而且 subpages 总数需要在 subpages_mapcount 中维护

#### compound_mapcount
```c
/*
 * If a 16GB hugetlb page were mapped by PTEs of all of its 4kB sub-pages,
 * its subpages_mapcount would be 0x400000: choose the COMPOUND_MAPPED bit
 * above that range, instead of 2*(PMD_SIZE/PAGE_SIZE).  Hugetlb currently
 * leaves subpages_mapcount at 0, but avoid surprise if it participates later.
 */
#define COMPOUND_MAPPED	0x800000
#define SUBPAGES_MAPPED	(COMPOUND_MAPPED - 1)
```
一个 COMPOUND_MAPPED 表示被映射为大页，而被 SUBPAGES_MAPPED 下的，计算 basepage 的映射次数。



#### subpages_mapcount

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

#### compound page 如何创建的
为什么这个根本无法拦截到任何东西：
```sh
sudo bpftrace -e 'kfunc:prep_compound_page { @reads[args->order] = count(); }'
```
但是在 QEMU 中间打点的时候，在这里是可以获取到很多内容的，也许是 ebpf 的问题，也许内核版本有点小区别，也是环境配置不同。


```sh
sudo bpftrace -e 'kfunc:__alloc_pages { @reads[args->order] = count(); }'
```
可以得到:
```txt
@reads[2]: 2185
@reads[1]: 3076
@reads[3]: 6123
@reads[0]: 57084
```

sudo bpftrace -e 'kfunc:alloc_pages { if (args->order >= 3) { @[kstack] = count(); } }'

可以得到如下位置，
```tx
@[
    alloc_pages+5
    alloc_skb_with_frags+183
    sock_alloc_send_pskb+569
    unix_stream_sendmsg+487
    sock_sendmsg+95
    __sys_sendto+252
    __x64_sys_sendto+32
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]: 2688
```

文件系统中的使用:
- ra_alloc_folio
  - filemap_alloc_folio 中是文件系统也是会创建出来 compound page 的

alloc_pages_exact 是分配的时候不要 `__GFP_COMP`，因为其需要的不是 2^^order 个，而是特定大小的，首先分配，然后将页释放掉。
因为 compound page 只能是是 2 ^^ order 分配的。

- [ ] prep_new_page : 如果没有 `__GFP_COMP`，所有的 page 的 flags 是如何初始化的?

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

## THP split
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

## zero page
By default kernel tries to use huge zero page on read page fault to anonymous mapping. It’s possible to disable huge zero page by writing 0 or enable it back by writing 1:

Contrary to the zero page's original preferential use,
some modern operating systems such as FreeBSD, Linux and Microsoft Windows[2] actually make the zero page inaccessible to trap uses of null pointers.

## transhuge 对于其他模块的支持

### copy page
1. copy_huge_pmd
2. copy_huge_pud

dup_mmap 中间逐级下沉下来的

### page fault

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

## prep_transhuge_page 分配 thp 之后的准备

- `__folio_alloc` 一定是分配 hugepage

```sh
sudo bpftrace -e 'kfunc:__folio_alloc { if (args->order > 1) { @reads[kstack] = count(); }}'
```

得到如下内容:
```txt
@reads[
    __folio_alloc+5
    vma_alloc_folio+663
    do_huge_pmd_anonymous_page+179
    __handle_mm_fault+2322
    handle_mm_fault+178
    do_user_addr_fault+460
    exc_page_fault+103
    asm_exc_page_fault+34
]: 3
@reads[
    __folio_alloc+5
    vma_alloc_folio+663
    shmem_alloc_hugefolio+199
    shmem_alloc_and_acct_folio+169
    shmem_get_folio_gfp+499
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
]: 196
```

## anonymous 和 shared 都是如何初始化的

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

没有什么特殊处理，就是当作一个 page 了

## split_huge_page_to_list

transparet hugeapge 会因为 memory pressure 而被拆分吗?

会的，在 shrink_folio_list 中，只要发现一个 transparent hugepage 是 inactive 的，那么首先就会进行拆分，进而就像是普通页一样被 swap 的
```txt
#0  __split_huge_pmd_locked (freeze=<optimized out>, haddr=<optimized out>, pmd=<optimized out>, vma=<optimized out>) at mm/huge_memory.c:2308
#1  __split_huge_pmd (vma=vma@entry=0xffff8883dd144260, pmd=0xffff8883588cb210, address=address@entry=139950574010368, freeze=true, folio=folio@entry=0xffffea000de68000) at mm/huge_memory.c:2308
#2  0xffffffff8138f4dd in split_huge_pmd_address (vma=vma@entry=0xffff8883dd144260, address=address@entry=139950574010368, freeze=freeze@entry=true, folio=folio@entry=0xffffea000de68000) at mm/huge_memory.c:2337
#3  0xffffffff813418c0 in try_to_migrate_one (folio=0xffffea000de68000, vma=0xffff8883dd144260, address=139950574010368, arg=<optimized out>) at mm/rmap.c:1858
#4  0xffffffff8133ebe3 in rmap_walk_anon (folio=folio@entry=0xffffea000de68000, rwc=rwc@entry=0xffffc90003d63a00, locked=locked@entry=true) at mm/rmap.c:2443
#5  0xffffffff81341b66 in rmap_walk_locked (rwc=0xffffc90003d63a00, folio=0xffffea000de68000) at mm/rmap.c:2530
#6  try_to_migrate (folio=folio@entry=0xffffea000de68000, flags=flags@entry=(TTU_SPLIT_HUGE_PMD | TTU_SYNC | TTU_RMAP_LOCKED)) at mm/rmap.c:2173
#7  0xffffffff8138faa8 in unmap_folio (folio=<optimized out>) at mm/huge_memory.c:2388
#8  split_huge_page_to_list (page=page@entry=0xffffea000de68000, list=list@entry=0xffffc90003d63c30) at mm/huge_memory.c:2741
#9  0xffffffff812f8f5a in split_folio_to_list (list=0xffffc90003d63c30, folio=0xffffea000de68000) at ./include/linux/huge_mm.h:444
#10 shrink_folio_list (folio_list=folio_list@entry=0xffffc90003d63c30, pgdat=pgdat@entry=0xffff8883bfffc000, sc=sc@entry=0xffffc90003d63dd8, stat=stat@entry=0xffffc90003d63cb8, ignore_references=ignore_references@entry=false) at mm/vmscan.c:1856
#11 0xffffffff812fa884 in shrink_inactive_list (lru=LRU_INACTIVE_ANON, sc=0xffffc90003d63dd8, lruvec=0xffff88835698e800, nr_to_scan=<optimized out>) at mm/vmscan.c:2526
#12 shrink_list (sc=0xffffc90003d63dd8, lruvec=0xffff88835698e800, nr_to_scan=<optimized out>, lru=LRU_INACTIVE_ANON) at mm/vmscan.c:2767
#13 shrink_lruvec (lruvec=lruvec@entry=0xffff88835698e800, sc=sc@entry=0xffffc90003d63dd8) at mm/vmscan.c:5951
#14 0xffffffff812fb14e in shrink_node_memcgs (sc=0xffffc90003d63dd8, pgdat=0xffff8883bfffc000) at mm/vmscan.c:6138
#15 shrink_node (pgdat=pgdat@entry=0xffff8883bfffc000, sc=sc@entry=0xffffc90003d63dd8) at mm/vmscan.c:6169
#16 0xffffffff812fb897 in kswapd_shrink_node (sc=0xffffc90003d63dd8, pgdat=0xffff8883bfffc000) at mm/vmscan.c:6960
#17 balance_pgdat (pgdat=pgdat@entry=0xffff8883bfffc000, order=order@entry=0, highest_zoneidx=highest_zoneidx@entry=3) at mm/vmscan.c:7150
#18 0xffffffff812fbe6f in kswapd (p=0xffff8883bfffc000) at mm/vmscan.c:7410
#19 0xffffffff811556a4 in kthread (_create=0xffff8883c18e01c0) at kernel/kthread.c:376
#20 0xffffffff81002659 in ret_from_fork () at arch/x86/entry/entry_64.S:308
```

## [ ] 为什么 transparent hugepage 会让 idle page tracking 不准

## [ ] 到底是使用 pmd 还是使用 pud 的

## MADV_COLLAPSE
commit 写的很详细的了；
- 7d8faaf155454f8798ec56404faca29a82689c77
- https://lwn.net/Articles/887753/
7d8faaf155454f8798ec56404faca29a82689c77

## 看看为什么数据库不喜欢
- https://www.mongodb.com/docs/manual/tutorial/transparent-huge-pages/
- https://www.pingcap.com/blog/transparent-huge-pages-why-we-disable-it-for-databases/

## [ ] 这些操作需要解决掉

```txt
AnonHugePages:   3448832 kB
ShmemHugePages:  2045952 kB
ShmemPmdMapped:        0 kB
FileHugePages:         0 kB
FilePmdMapped:         0 kB
```

目前只有第一个成功的实现的调整过，其他的不受控制。

使用这种可以增加: AnonHugePages
```c
  void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
```

但是使用这个不会增加任何数值:
```c
  void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
```

使用 QEMU 测试 share=off 也是类似的效果

但是 shmem_enabled 之后
```txt
🧀  cat /sys/kernel/mm/transparent_hugepage/shmem_enabled
always within_size advise [never] deny force
```

这两个都可以发生变化

```txt
ShmemHugePages:  2998272 kB
ShmemPmdMapped:   862208 kB
```

映射文件总是失败.

似乎有点难触发。

## page cache
根本没办法让其不为 0

```txt
FileHugePages:         0 kB
FilePmdMapped:         0 kB
```

存在两个位置:
- filemap_unaccount_folio : 删除的时候
- `__filemap_add_folio`
  - 其调用位置为 : filemap_add_folio hugetlb_add_to_page_cache

- filemap_add_folio 的调用位置在 filemap.c 和 readahead.c 但是通过调用 **filemap_alloc_folio** 创建 folio ，当 folio 的 order 超过 HPAGE_PMD_ORDER ，那么就认为是
大页了。

但是，fio 以及 ccls index 显示全部都是 order = 1
```txt
🧀  sudo bpftrace -e "kfunc:filemap_alloc_folio { @order = lhist(args->order, 0, 100, 1); }"

Attaching 1 probe...
^C

@order:
[0, 1)            136239 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
```

换言之，这个过程根本无法触发:
```sh
sudo bpftrace -e "kfunc:filemap_alloc_folio { if (args->order > 0) { print(\"good\") } }"
```

通过不断的查找 filemap_alloc_folio 的 reference ，最后可以找到:
- filemap_fault 中，所在的 vma 被 madvise 了 MADV_HUGEPAGE，那么就会使用大页

```c
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

const unsigned long PAGE_SIZE = 4 * 1024;
unsigned long MAP_SIZE = 4000L * 1024 * 1024;

int get_file() {
  int fd = open("/root/huge", O_RDWR | O_CREAT, 0644);
  if (fd == -1)
    goto err;

  if (ftruncate(fd, MAP_SIZE) < 0)
    goto err;
  return fd;
err:
  printf("%s\n", strerror(errno));
  exit(1);
}

int main() {
  void *ptr =
      mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, get_file(), 0);
  if (ptr == MAP_FAILED)
    goto err;

  if (madvise(ptr, MAP_SIZE, MADV_HUGEPAGE) == -1)
    goto err;

  char m = '1';
  for (unsigned long i = 0; i < MAP_SIZE; i += PAGE_SIZE) {
    m += *((char *)(ptr + i));
  }
  return 0;

err:
  printf("munmap failed: %s\n", strerror(errno));
  return 1;
}
```

通过这个测试，的确可以走到 do_sync_mmap_readahead，但是数值还是没有变化。
最后，检查一下，发现 ext4 根本不支持，需要使用 xfs

修改之后，得到如下内容:
```txt
AnonHugePages:     20480 kB
ShmemHugePages:        0 kB
ShmemPmdMapped:        0 kB
FileHugePages:   4096000 kB
FilePmdMapped:   4096000 kB
```

## [ ] 如果 order = 3 这种和文件系统沟通的 folio 暂时不知道如何分配的


## 理解下 transparent hugepage 的 swap

38d8b4e6bdc872f07a3149309ab01719c96f3894

```diff
tree a4bdf8e41a90f49465829b98a46645af64b0103d
parent 9d85e15f1d552653c989dbecf051d8eea5937be8
author Huang Ying <ying.huang@intel.com> Thu Jul 6 15:37:18 2017 -0700
committer Linus Torvalds <torvalds@linux-foundation.org> Thu Jul 6 16:24:31 2017 -0700

mm, THP, swap: delay splitting THP during swap out

Patch series "THP swap: Delay splitting THP during swapping out", v11.

This patchset is to optimize the performance of Transparent Huge Page
(THP) swap.

Recently, the performance of the storage devices improved so fast that
we cannot saturate the disk bandwidth with single logical CPU when do
page swap out even on a high-end server machine.  Because the
performance of the storage device improved faster than that of single
logical CPU.  And it seems that the trend will not change in the near
future.  On the other hand, the THP becomes more and more popular
because of increased memory size.  So it becomes necessary to optimize
THP swap performance.

The advantages of the THP swap support include:

 - Batch the swap operations for the THP to reduce lock
   acquiring/releasing, including allocating/freeing the swap space,
   adding/deleting to/from the swap cache, and writing/reading the swap
   space, etc. This will help improve the performance of the THP swap.

 - The THP swap space read/write will be 2M sequential IO. It is
   particularly helpful for the swap read, which are usually 4k random
   IO. This will improve the performance of the THP swap too.

 - It will help the memory fragmentation, especially when the THP is
   heavily used by the applications. The 2M continuous pages will be
   free up after THP swapping out.

 - It will improve the THP utilization on the system with the swap
   turned on. Because the speed for khugepaged to collapse the normal
   pages into the THP is quite slow. After the THP is split during the
   swapping out, it will take quite long time for the normal pages to
   collapse back into the THP after being swapped in. The high THP
   utilization helps the efficiency of the page based memory management
   too.

There are some concerns regarding THP swap in, mainly because possible
enlarged read/write IO size (for swap in/out) may put more overhead on
the storage device.  To deal with that, the THP swap in should be turned
on only when necessary.  For example, it can be selected via
"always/never/madvise" logic, to be turned on globally, turned off
globally, or turned on only for VMA with MADV_HUGEPAGE, etc.

This patchset is the first step for the THP swap support.  The plan is
to delay splitting THP step by step, finally avoid splitting THP during
the THP swapping out and swap out/in the THP as a whole.

As the first step, in this patchset, the splitting huge page is delayed
from almost the first step of swapping out to after allocating the swap
space for the THP and adding the THP into the swap cache.  This will
reduce lock acquiring/releasing for the locks used for the swap cache
management.

With the patchset, the swap out throughput improves 15.5% (from about
3.73GB/s to about 4.31GB/s) in the vm-scalability swap-w-seq test case
with 8 processes.  The test is done on a Xeon E5 v3 system.  The swap
device used is a RAM simulated PMEM (persistent memory) device.  To test
the sequential swapping out, the test case creates 8 processes, which
sequentially allocate and write to the anonymous pages until the RAM and
part of the swap device is used up.

This patch (of 5):

In this patch, splitting huge page is delayed from almost the first step
of swapping out to after allocating the swap space for the THP
(Transparent Huge Page) and adding the THP into the swap cache.  This
will batch the corresponding operation, thus improve THP swap out
throughput.

This is the first step for the THP swap optimization.  The plan is to
delay splitting the THP step by step and avoid splitting the THP
finally.

In this patch, one swap cluster is used to hold the contents of each THP
swapped out.  So, the size of the swap cluster is changed to that of the
THP (Transparent Huge Page) on x86_64 architecture (512).  For other
architectures which want such THP swap optimization,
ARCH_USES_THP_SWAP_CLUSTER needs to be selected in the Kconfig file for
the architecture.  In effect, this will enlarge swap cluster size by 2
times on x86_64.  Which may make it harder to find a free cluster when
the swap space becomes fragmented.  So that, this may reduce the
continuous swap space allocation and sequential write in theory.  The
performance test in 0day shows no regressions caused by this.

In the future of THP swap optimization, some information of the swapped
out THP (such as compound map count) will be recorded in the
swap_cluster_info data structure.

The mem cgroup swap accounting functions are enhanced to support charge
or uncharge a swap cluster backing a THP as a whole.

The swap cluster allocate/free functions are added to allocate/free a
swap cluster for a THP.  A fair simple algorithm is used for swap
cluster allocation, that is, only the first swap device in priority list
will be tried to allocate the swap cluster.  The function will fail if
the trying is not successful, and the caller will fallback to allocate a
single swap slot instead.  This works good enough for normal cases.  If
the difference of the number of the free swap clusters among multiple
swap devices is significant, it is possible that some THPs are split
earlier than necessary.  For example, this could be caused by big size
difference among multiple swap devices.

The swap cache functions is enhanced to support add/delete THP to/from
the swap cache as a set of (HPAGE_PMD_NR) sub-pages.  This may be
enhanced in the future with multi-order radix tree.  But because we will
split the THP soon during swapping out, that optimization doesn't make
much sense for this first step.

The THP splitting functions are enhanced to support to split THP in swap
cache during swapping out.  The page lock will be held during allocating
the swap cluster, adding the THP into the swap cache and splitting the
THP.  So in the code path other than swapping out, if the THP need to be
split, the PageSwapCache(THP) will be always false.

The swap cluster is only available for SSD, so the THP swap optimization
in this patchset has no effect for HDD.

[ying.huang@intel.com: fix two issues in THP optimize patch]
  Link: http://lkml.kernel.org/r/87k25ed8zo.fsf@yhuang-dev.intel.com
[hannes@cmpxchg.org: extensive cleanups and simplifications, reduce code size]
Link: http://lkml.kernel.org/r/20170515112522.32457-2-ying.huang@intel.com
Signed-off-by: "Huang, Ying" <ying.huang@intel.com>
Signed-off-by: Johannes Weiner <hannes@cmpxchg.org>
Suggested-by: Andrew Morton <akpm@linux-foundation.org> [for config option]
Acked-by: Kirill A. Shutemov <kirill.shutemov@linux.intel.com> [for changes in huge_memory.c and huge_mm.h]
Cc: Andrea Arcangeli <aarcange@redhat.com>
Cc: Ebru Akagunduz <ebru.akagunduz@gmail.com>
Cc: Johannes Weiner <hannes@cmpxchg.org>
Cc: Michal Hocko <mhocko@kernel.org>
Cc: Tejun Heo <tj@kernel.org>
Cc: Hugh Dickins <hughd@google.com>
Cc: Shaohua Li <shli@kernel.org>
Cc: Minchan Kim <minchan@kernel.org>
Cc: Rik van Riel <riel@redhat.com>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>

diff --git a/arch/x86/Kconfig b/arch/x86/Kconfig
index e767ed24aeb4..1dbbe38f6ec0 100644
--- a/arch/x86/Kconfig
+++ b/arch/x86/Kconfig
@@ -72,6 +72,7 @@ config X86
 	select ARCH_WANT_BATCHED_UNMAP_TLB_FLUSH
 	select ARCH_WANT_FRAME_POINTERS
 	select ARCH_WANTS_DYNAMIC_TASK_STRUCT
+	select ARCH_WANTS_THP_SWAP		if X86_64
 	select BUILDTIME_EXTABLE_SORT
 	select CLKEVT_I8253
 	select CLOCKSOURCE_VALIDATE_LAST_CYCLE
diff --git a/include/linux/page-flags.h b/include/linux/page-flags.h
index 6b5818d6de32..d33e3280c8ad 100644
--- a/include/linux/page-flags.h
+++ b/include/linux/page-flags.h
@@ -326,11 +326,14 @@ PAGEFLAG_FALSE(HighMem)
 #ifdef CONFIG_SWAP
 static __always_inline int PageSwapCache(struct page *page)
 {
+#ifdef CONFIG_THP_SWAP
+	page = compound_head(page);
+#endif
 	return PageSwapBacked(page) && test_bit(PG_swapcache, &page->flags);

 }
-SETPAGEFLAG(SwapCache, swapcache, PF_NO_COMPOUND)
-CLEARPAGEFLAG(SwapCache, swapcache, PF_NO_COMPOUND)
+SETPAGEFLAG(SwapCache, swapcache, PF_NO_TAIL)
+CLEARPAGEFLAG(SwapCache, swapcache, PF_NO_TAIL)
 #else
 PAGEFLAG_FALSE(SwapCache)
 #endif
diff --git a/include/linux/swap.h b/include/linux/swap.h
index ba5882419a7d..d18876384de0 100644
--- a/include/linux/swap.h
+++ b/include/linux/swap.h
@@ -386,9 +386,9 @@ static inline long get_nr_swap_pages(void)
 }

 extern void si_swapinfo(struct sysinfo *);
-extern swp_entry_t get_swap_page(void);
+extern swp_entry_t get_swap_page(struct page *page);
 extern swp_entry_t get_swap_page_of_type(int);
-extern int get_swap_pages(int n, swp_entry_t swp_entries[]);
+extern int get_swap_pages(int n, bool cluster, swp_entry_t swp_entries[]);
 extern int add_swap_count_continuation(swp_entry_t, gfp_t);
 extern void swap_shmem_alloc(swp_entry_t);
 extern int swap_duplicate(swp_entry_t);
@@ -515,7 +515,7 @@ static inline int try_to_free_swap(struct page *page)
 	return 0;
 }

-static inline swp_entry_t get_swap_page(void)
+static inline swp_entry_t get_swap_page(struct page *page)
 {
 	swp_entry_t entry;
 	entry.val = 0;
@@ -548,7 +548,7 @@ static inline int mem_cgroup_swappiness(struct mem_cgroup *mem)
 #ifdef CONFIG_MEMCG_SWAP
 extern void mem_cgroup_swapout(struct page *page, swp_entry_t entry);
 extern int mem_cgroup_try_charge_swap(struct page *page, swp_entry_t entry);
-extern void mem_cgroup_uncharge_swap(swp_entry_t entry);
+extern void mem_cgroup_uncharge_swap(swp_entry_t entry, unsigned int nr_pages);
 extern long mem_cgroup_get_nr_swap_pages(struct mem_cgroup *memcg);
 extern bool mem_cgroup_swap_full(struct page *page);
 #else
@@ -562,7 +562,8 @@ static inline int mem_cgroup_try_charge_swap(struct page *page,
 	return 0;
 }

-static inline void mem_cgroup_uncharge_swap(swp_entry_t entry)
+static inline void mem_cgroup_uncharge_swap(swp_entry_t entry,
+					    unsigned int nr_pages)
 {
 }

@@ -577,5 +578,13 @@ static inline bool mem_cgroup_swap_full(struct page *page)
 }
 #endif

+#ifdef CONFIG_THP_SWAP
+extern void swapcache_free_cluster(swp_entry_t entry);
+#else
+static inline void swapcache_free_cluster(swp_entry_t entry)
+{
+}
+#endif
+
 #endif /* __KERNEL__*/
 #endif /* _LINUX_SWAP_H */
diff --git a/include/linux/swap_cgroup.h b/include/linux/swap_cgroup.h
index 145306bdc92f..b2b8ec7bda3f 100644
--- a/include/linux/swap_cgroup.h
+++ b/include/linux/swap_cgroup.h
@@ -7,7 +7,8 @@

 extern unsigned short swap_cgroup_cmpxchg(swp_entry_t ent,
 					unsigned short old, unsigned short new);
-extern unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id);
+extern unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id,
+					 unsigned int nr_ents);
 extern unsigned short lookup_swap_cgroup_id(swp_entry_t ent);
 extern int swap_cgroup_swapon(int type, unsigned long max_pages);
 extern void swap_cgroup_swapoff(int type);
@@ -15,7 +16,8 @@ extern void swap_cgroup_swapoff(int type);
 #else

 static inline
-unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id)
+unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id,
+				  unsigned int nr_ents)
 {
 	return 0;
 }
diff --git a/mm/Kconfig b/mm/Kconfig
index 665cb370ad38..9870baafb096 100644
--- a/mm/Kconfig
+++ b/mm/Kconfig
@@ -446,6 +446,18 @@ choice
 	  benefit.
 endchoice

+config ARCH_WANTS_THP_SWAP
+       def_bool n
+
+config THP_SWAP
+	def_bool y
+	depends on TRANSPARENT_HUGEPAGE && ARCH_WANTS_THP_SWAP
+	help
+	  Swap transparent huge pages in one piece, without splitting.
+	  XXX: For now this only does clustered swap space allocation.
+
+	  For selection by architectures with reasonable THP sizes.
+
 config	TRANSPARENT_HUGE_PAGECACHE
 	def_bool y
 	depends on TRANSPARENT_HUGEPAGE
diff --git a/mm/huge_memory.c b/mm/huge_memory.c
index f4d5f9d0f9b7..1a168e4bac4b 100644
--- a/mm/huge_memory.c
+++ b/mm/huge_memory.c
@@ -2203,7 +2203,7 @@ static void __split_huge_page_tail(struct page *head, int tail,
 	 * atomic_set() here would be safe on all archs (and not only on x86),
 	 * it's safer to use atomic_inc()/atomic_add().
 	 */
-	if (PageAnon(head)) {
+	if (PageAnon(head) && !PageSwapCache(head)) {
 		page_ref_inc(page_tail);
 	} else {
 		/* Additional pin to radix tree */
@@ -2214,6 +2214,7 @@ static void __split_huge_page_tail(struct page *head, int tail,
 	page_tail->flags |= (head->flags &
 			((1L << PG_referenced) |
 			 (1L << PG_swapbacked) |
+			 (1L << PG_swapcache) |
 			 (1L << PG_mlocked) |
 			 (1L << PG_uptodate) |
 			 (1L << PG_active) |
@@ -2276,7 +2277,11 @@ static void __split_huge_page(struct page *page, struct list_head *list,
 	ClearPageCompound(head);
 	/* See comment in __split_huge_page_tail() */
 	if (PageAnon(head)) {
-		page_ref_inc(head);
+		/* Additional pin to radix tree of swap cache */
+		if (PageSwapCache(head))
+			page_ref_add(head, 2);
+		else
+			page_ref_inc(head);
 	} else {
 		/* Additional pin to radix tree */
 		page_ref_add(head, 2);
@@ -2432,7 +2437,7 @@ int split_huge_page_to_list(struct page *page, struct list_head *list)
 			ret = -EBUSY;
 			goto out;
 		}
-		extra_pins = 0;
+		extra_pins = PageSwapCache(page) ? HPAGE_PMD_NR : 0;
 		mapping = NULL;
 		anon_vma_lock_write(anon_vma);
 	} else {
diff --git a/mm/memcontrol.c b/mm/memcontrol.c
index d75b38b66ef6..fc51a33ddcd1 100644
--- a/mm/memcontrol.c
+++ b/mm/memcontrol.c
@@ -2376,10 +2376,9 @@ void mem_cgroup_split_huge_fixup(struct page *head)

 #ifdef CONFIG_MEMCG_SWAP
 static void mem_cgroup_swap_statistics(struct mem_cgroup *memcg,
-					 bool charge)
+				       int nr_entries)
 {
-	int val = (charge) ? 1 : -1;
-	this_cpu_add(memcg->stat->count[MEMCG_SWAP], val);
+	this_cpu_add(memcg->stat->count[MEMCG_SWAP], nr_entries);
 }

 /**
@@ -2405,8 +2404,8 @@ static int mem_cgroup_move_swap_account(swp_entry_t entry,
 	new_id = mem_cgroup_id(to);

 	if (swap_cgroup_cmpxchg(entry, old_id, new_id) == old_id) {
-		mem_cgroup_swap_statistics(from, false);
-		mem_cgroup_swap_statistics(to, true);
+		mem_cgroup_swap_statistics(from, -1);
+		mem_cgroup_swap_statistics(to, 1);
 		return 0;
 	}
 	return -EINVAL;
@@ -5445,7 +5444,7 @@ void mem_cgroup_commit_charge(struct page *page, struct mem_cgroup *memcg,
 		 * let's not wait for it.  The page already received a
 		 * memory+swap charge, drop the swap entry duplicate.
 		 */
-		mem_cgroup_uncharge_swap(entry);
+		mem_cgroup_uncharge_swap(entry, nr_pages);
 	}
 }

@@ -5873,9 +5872,9 @@ void mem_cgroup_swapout(struct page *page, swp_entry_t entry)
 	 * ancestor for the swap instead and transfer the memory+swap charge.
 	 */
 	swap_memcg = mem_cgroup_id_get_online(memcg);
-	oldid = swap_cgroup_record(entry, mem_cgroup_id(swap_memcg));
+	oldid = swap_cgroup_record(entry, mem_cgroup_id(swap_memcg), 1);
 	VM_BUG_ON_PAGE(oldid, page);
-	mem_cgroup_swap_statistics(swap_memcg, true);
+	mem_cgroup_swap_statistics(swap_memcg, 1);

 	page->mem_cgroup = NULL;

@@ -5902,19 +5901,20 @@ void mem_cgroup_swapout(struct page *page, swp_entry_t entry)
 		css_put(&memcg->css);
 }

-/*
- * mem_cgroup_try_charge_swap - try charging a swap entry
+/**
+ * mem_cgroup_try_charge_swap - try charging swap space for a page
  * @page: page being added to swap
  * @entry: swap entry to charge
  *
- * Try to charge @entry to the memcg that @page belongs to.
+ * Try to charge @page's memcg for the swap space at @entry.
  *
  * Returns 0 on success, -ENOMEM on failure.
  */
 int mem_cgroup_try_charge_swap(struct page *page, swp_entry_t entry)
 {
-	struct mem_cgroup *memcg;
+	unsigned int nr_pages = hpage_nr_pages(page);
 	struct page_counter *counter;
+	struct mem_cgroup *memcg;
 	unsigned short oldid;

 	if (!cgroup_subsys_on_dfl(memory_cgrp_subsys) || !do_swap_account)
@@ -5929,25 +5929,27 @@ int mem_cgroup_try_charge_swap(struct page *page, swp_entry_t entry)
 	memcg = mem_cgroup_id_get_online(memcg);

 	if (!mem_cgroup_is_root(memcg) &&
-	    !page_counter_try_charge(&memcg->swap, 1, &counter)) {
+	    !page_counter_try_charge(&memcg->swap, nr_pages, &counter)) {
 		mem_cgroup_id_put(memcg);
 		return -ENOMEM;
 	}

-	oldid = swap_cgroup_record(entry, mem_cgroup_id(memcg));
+	/* Get references for the tail pages, too */
+	if (nr_pages > 1)
+		mem_cgroup_id_get_many(memcg, nr_pages - 1);
+	oldid = swap_cgroup_record(entry, mem_cgroup_id(memcg), nr_pages);
 	VM_BUG_ON_PAGE(oldid, page);
-	mem_cgroup_swap_statistics(memcg, true);
+	mem_cgroup_swap_statistics(memcg, nr_pages);

 	return 0;
 }

 /**
- * mem_cgroup_uncharge_swap - uncharge a swap entry
+ * mem_cgroup_uncharge_swap - uncharge swap space
  * @entry: swap entry to uncharge
- *
- * Drop the swap charge associated with @entry.
+ * @nr_pages: the amount of swap space to uncharge
  */
-void mem_cgroup_uncharge_swap(swp_entry_t entry)
+void mem_cgroup_uncharge_swap(swp_entry_t entry, unsigned int nr_pages)
 {
 	struct mem_cgroup *memcg;
 	unsigned short id;
@@ -5955,18 +5957,18 @@ void mem_cgroup_uncharge_swap(swp_entry_t entry)
 	if (!do_swap_account)
 		return;

-	id = swap_cgroup_record(entry, 0);
+	id = swap_cgroup_record(entry, 0, nr_pages);
 	rcu_read_lock();
 	memcg = mem_cgroup_from_id(id);
 	if (memcg) {
 		if (!mem_cgroup_is_root(memcg)) {
 			if (cgroup_subsys_on_dfl(memory_cgrp_subsys))
-				page_counter_uncharge(&memcg->swap, 1);
+				page_counter_uncharge(&memcg->swap, nr_pages);
 			else
-				page_counter_uncharge(&memcg->memsw, 1);
+				page_counter_uncharge(&memcg->memsw, nr_pages);
 		}
-		mem_cgroup_swap_statistics(memcg, false);
-		mem_cgroup_id_put(memcg);
+		mem_cgroup_swap_statistics(memcg, -nr_pages);
+		mem_cgroup_id_put_many(memcg, nr_pages);
 	}
 	rcu_read_unlock();
 }
diff --git a/mm/shmem.c b/mm/shmem.c
index 9100c4952698..bbb987c58dad 100644
--- a/mm/shmem.c
+++ b/mm/shmem.c
@@ -1291,7 +1291,7 @@ static int shmem_writepage(struct page *page, struct writeback_control *wbc)
 		SetPageUptodate(page);
 	}

-	swap = get_swap_page();
+	swap = get_swap_page(page);
 	if (!swap.val)
 		goto redirty;

diff --git a/mm/swap_cgroup.c b/mm/swap_cgroup.c
index 3405b4ee1757..fcd2740f4ed7 100644
--- a/mm/swap_cgroup.c
+++ b/mm/swap_cgroup.c
@@ -61,21 +61,27 @@ static int swap_cgroup_prepare(int type)
 	return -ENOMEM;
 }

+static struct swap_cgroup *__lookup_swap_cgroup(struct swap_cgroup_ctrl *ctrl,
+						pgoff_t offset)
+{
+	struct page *mappage;
+	struct swap_cgroup *sc;
+
+	mappage = ctrl->map[offset / SC_PER_PAGE];
+	sc = page_address(mappage);
+	return sc + offset % SC_PER_PAGE;
+}
+
 static struct swap_cgroup *lookup_swap_cgroup(swp_entry_t ent,
 					struct swap_cgroup_ctrl **ctrlp)
 {
 	pgoff_t offset = swp_offset(ent);
 	struct swap_cgroup_ctrl *ctrl;
-	struct page *mappage;
-	struct swap_cgroup *sc;

 	ctrl = &swap_cgroup_ctrl[swp_type(ent)];
 	if (ctrlp)
 		*ctrlp = ctrl;
-
-	mappage = ctrl->map[offset / SC_PER_PAGE];
-	sc = page_address(mappage);
-	return sc + offset % SC_PER_PAGE;
+	return __lookup_swap_cgroup(ctrl, offset);
 }

 /**
@@ -108,25 +114,39 @@ unsigned short swap_cgroup_cmpxchg(swp_entry_t ent,
 }

 /**
- * swap_cgroup_record - record mem_cgroup for this swp_entry.
- * @ent: swap entry to be recorded into
+ * swap_cgroup_record - record mem_cgroup for a set of swap entries
+ * @ent: the first swap entry to be recorded into
  * @id: mem_cgroup to be recorded
+ * @nr_ents: number of swap entries to be recorded
  *
  * Returns old value at success, 0 at failure.
  * (Of course, old value can be 0.)
  */
-unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id)
+unsigned short swap_cgroup_record(swp_entry_t ent, unsigned short id,
+				  unsigned int nr_ents)
 {
 	struct swap_cgroup_ctrl *ctrl;
 	struct swap_cgroup *sc;
 	unsigned short old;
 	unsigned long flags;
+	pgoff_t offset = swp_offset(ent);
+	pgoff_t end = offset + nr_ents;

 	sc = lookup_swap_cgroup(ent, &ctrl);

 	spin_lock_irqsave(&ctrl->lock, flags);
 	old = sc->id;
-	sc->id = id;
+	for (;;) {
+		VM_BUG_ON(sc->id != old);
+		sc->id = id;
+		offset++;
+		if (offset == end)
+			break;
+		if (offset % SC_PER_PAGE)
+			sc++;
+		else
+			sc = __lookup_swap_cgroup(ctrl, offset);
+	}
 	spin_unlock_irqrestore(&ctrl->lock, flags);

 	return old;
diff --git a/mm/swap_slots.c b/mm/swap_slots.c
index 58f6c78f1dad..90c1032a8ac3 100644
--- a/mm/swap_slots.c
+++ b/mm/swap_slots.c
@@ -263,7 +263,8 @@ static int refill_swap_slots_cache(struct swap_slots_cache *cache)

 	cache->cur = 0;
 	if (swap_slot_cache_active)
-		cache->nr = get_swap_pages(SWAP_SLOTS_CACHE_SIZE, cache->slots);
+		cache->nr = get_swap_pages(SWAP_SLOTS_CACHE_SIZE, false,
+					   cache->slots);

 	return cache->nr;
 }
@@ -301,11 +302,19 @@ int free_swap_slot(swp_entry_t entry)
 	return 0;
 }

-swp_entry_t get_swap_page(void)
+swp_entry_t get_swap_page(struct page *page)
 {
 	swp_entry_t entry, *pentry;
 	struct swap_slots_cache *cache;

+	entry.val = 0;
+
+	if (PageTransHuge(page)) {
+		if (IS_ENABLED(CONFIG_THP_SWAP))
+			get_swap_pages(1, true, &entry);
+		return entry;
+	}
+
 	/*
 	 * Preemption is allowed here, because we may sleep
 	 * in refill_swap_slots_cache().  But it is safe, because
@@ -317,7 +326,6 @@ swp_entry_t get_swap_page(void)
 	 */
 	cache = raw_cpu_ptr(&swp_slots);

-	entry.val = 0;
 	if (check_cache_active()) {
 		mutex_lock(&cache->alloc_lock);
 		if (cache->slots) {
@@ -337,7 +345,7 @@ swp_entry_t get_swap_page(void)
 			return entry;
 	}

-	get_swap_pages(1, &entry);
+	get_swap_pages(1, false, &entry);

 	return entry;
 }
diff --git a/mm/swap_state.c b/mm/swap_state.c
index 539b8885e3d1..16ff89d058f4 100644
--- a/mm/swap_state.c
+++ b/mm/swap_state.c
@@ -19,6 +19,7 @@
 #include <linux/migrate.h>
 #include <linux/vmalloc.h>
 #include <linux/swap_slots.h>
+#include <linux/huge_mm.h>

 #include <asm/pgtable.h>

@@ -38,6 +39,7 @@ struct address_space *swapper_spaces[MAX_SWAPFILES];
 static unsigned int nr_swapper_spaces[MAX_SWAPFILES];

 #define INC_CACHE_INFO(x)	do { swap_cache_info.x++; } while (0)
+#define ADD_CACHE_INFO(x, nr)	do { swap_cache_info.x += (nr); } while (0)

 static struct {
 	unsigned long add_total;
@@ -90,39 +92,46 @@ void show_swap_cache_info(void)
  */
 int __add_to_swap_cache(struct page *page, swp_entry_t entry)
 {
-	int error;
+	int error, i, nr = hpage_nr_pages(page);
 	struct address_space *address_space;
+	pgoff_t idx = swp_offset(entry);

 	VM_BUG_ON_PAGE(!PageLocked(page), page);
 	VM_BUG_ON_PAGE(PageSwapCache(page), page);
 	VM_BUG_ON_PAGE(!PageSwapBacked(page), page);

-	get_page(page);
+	page_ref_add(page, nr);
 	SetPageSwapCache(page);
-	set_page_private(page, entry.val);

 	address_space = swap_address_space(entry);
 	spin_lock_irq(&address_space->tree_lock);
-	error = radix_tree_insert(&address_space->page_tree,
-				  swp_offset(entry), page);
-	if (likely(!error)) {
-		address_space->nrpages++;
-		__inc_node_page_state(page, NR_FILE_PAGES);
-		INC_CACHE_INFO(add_total);
+	for (i = 0; i < nr; i++) {
+		set_page_private(page + i, entry.val + i);
+		error = radix_tree_insert(&address_space->page_tree,
+					  idx + i, page + i);
+		if (unlikely(error))
+			break;
 	}
-	spin_unlock_irq(&address_space->tree_lock);
-
-	if (unlikely(error)) {
+	if (likely(!error)) {
+		address_space->nrpages += nr;
+		__mod_node_page_state(page_pgdat(page), NR_FILE_PAGES, nr);
+		ADD_CACHE_INFO(add_total, nr);
+	} else {
 		/*
 		 * Only the context which have set SWAP_HAS_CACHE flag
 		 * would call add_to_swap_cache().
 		 * So add_to_swap_cache() doesn't returns -EEXIST.
 		 */
 		VM_BUG_ON(error == -EEXIST);
-		set_page_private(page, 0UL);
+		set_page_private(page + i, 0UL);
+		while (i--) {
+			radix_tree_delete(&address_space->page_tree, idx + i);
+			set_page_private(page + i, 0UL);
+		}
 		ClearPageSwapCache(page);
-		put_page(page);
+		page_ref_sub(page, nr);
 	}
+	spin_unlock_irq(&address_space->tree_lock);

 	return error;
 }
@@ -132,7 +141,7 @@ int add_to_swap_cache(struct page *page, swp_entry_t entry, gfp_t gfp_mask)
 {
 	int error;

-	error = radix_tree_maybe_preload(gfp_mask);
+	error = radix_tree_maybe_preload_order(gfp_mask, compound_order(page));
 	if (!error) {
 		error = __add_to_swap_cache(page, entry);
 		radix_tree_preload_end();
@@ -146,8 +155,10 @@ int add_to_swap_cache(struct page *page, swp_entry_t entry, gfp_t gfp_mask)
  */
 void __delete_from_swap_cache(struct page *page)
 {
-	swp_entry_t entry;
 	struct address_space *address_space;
+	int i, nr = hpage_nr_pages(page);
+	swp_entry_t entry;
+	pgoff_t idx;

 	VM_BUG_ON_PAGE(!PageLocked(page), page);
 	VM_BUG_ON_PAGE(!PageSwapCache(page), page);
@@ -155,12 +166,15 @@ void __delete_from_swap_cache(struct page *page)

 	entry.val = page_private(page);
 	address_space = swap_address_space(entry);
-	radix_tree_delete(&address_space->page_tree, swp_offset(entry));
-	set_page_private(page, 0);
+	idx = swp_offset(entry);
+	for (i = 0; i < nr; i++) {
+		radix_tree_delete(&address_space->page_tree, idx + i);
+		set_page_private(page + i, 0);
+	}
 	ClearPageSwapCache(page);
-	address_space->nrpages--;
-	__dec_node_page_state(page, NR_FILE_PAGES);
-	INC_CACHE_INFO(del_total);
+	address_space->nrpages -= nr;
+	__mod_node_page_state(page_pgdat(page), NR_FILE_PAGES, -nr);
+	ADD_CACHE_INFO(del_total, nr);
 }

 /**
@@ -178,20 +192,12 @@ int add_to_swap(struct page *page, struct list_head *list)
 	VM_BUG_ON_PAGE(!PageLocked(page), page);
 	VM_BUG_ON_PAGE(!PageUptodate(page), page);

-	entry = get_swap_page();
+retry:
+	entry = get_swap_page(page);
 	if (!entry.val)
-		return 0;
-
-	if (mem_cgroup_try_charge_swap(page, entry)) {
-		swapcache_free(entry);
-		return 0;
-	}
-
-	if (unlikely(PageTransHuge(page)))
-		if (unlikely(split_huge_page_to_list(page, list))) {
-			swapcache_free(entry);
-			return 0;
-		}
+		goto fail;
+	if (mem_cgroup_try_charge_swap(page, entry))
+		goto fail_free;

 	/*
 	 * Radix-tree node allocations from PF_MEMALLOC contexts could
@@ -206,17 +212,33 @@ int add_to_swap(struct page *page, struct list_head *list)
 	 */
 	err = add_to_swap_cache(page, entry,
 			__GFP_HIGH|__GFP_NOMEMALLOC|__GFP_NOWARN);
-
-	if (!err) {
-		return 1;
-	} else {	/* -ENOMEM radix-tree allocation failure */
+	/* -ENOMEM radix-tree allocation failure */
+	if (err)
 		/*
 		 * add_to_swap_cache() doesn't return -EEXIST, so we can safely
 		 * clear SWAP_HAS_CACHE flag.
 		 */
-		swapcache_free(entry);
-		return 0;
+		goto fail_free;
+
+	if (PageTransHuge(page)) {
+		err = split_huge_page_to_list(page, list);
+		if (err) {
+			delete_from_swap_cache(page);
+			return 0;
+		}
 	}
+
+	return 1;
+
+fail_free:
+	if (PageTransHuge(page))
+		swapcache_free_cluster(entry);
+	else
+		swapcache_free(entry);
+fail:
+	if (PageTransHuge(page) && !split_huge_page_to_list(page, list))
+		goto retry;
+	return 0;
 }

 /*
@@ -237,8 +259,12 @@ void delete_from_swap_cache(struct page *page)
 	__delete_from_swap_cache(page);
 	spin_unlock_irq(&address_space->tree_lock);

-	swapcache_free(entry);
-	put_page(page);
+	if (PageTransHuge(page))
+		swapcache_free_cluster(entry);
+	else
+		swapcache_free(entry);
+
+	page_ref_sub(page, hpage_nr_pages(page));
 }

 /*
@@ -295,7 +321,7 @@ struct page * lookup_swap_cache(swp_entry_t entry)

 	page = find_get_page(swap_address_space(entry), swp_offset(entry));

-	if (page) {
+	if (page && likely(!PageTransCompound(page))) {
 		INC_CACHE_INFO(find_success);
 		if (TestClearPageReadahead(page))
 			atomic_inc(&swapin_readahead_hits);
@@ -506,7 +532,7 @@ struct page *swapin_readahead(swp_entry_t entry, gfp_t gfp_mask,
 						gfp_mask, vma, addr);
 		if (!page)
 			continue;
-		if (offset != entry_offset)
+		if (offset != entry_offset && likely(!PageTransCompound(page)))
 			SetPageReadahead(page);
 		put_page(page);
 	}
diff --git a/mm/swapfile.c b/mm/swapfile.c
index 4f6cba1b6632..984f0dd94948 100644
--- a/mm/swapfile.c
+++ b/mm/swapfile.c
@@ -199,7 +199,11 @@ static void discard_swap_cluster(struct swap_info_struct *si,
 	}
 }

+#ifdef CONFIG_THP_SWAP
+#define SWAPFILE_CLUSTER	HPAGE_PMD_NR
+#else
 #define SWAPFILE_CLUSTER	256
+#endif
 #define LATENCY_LIMIT		256

 static inline void cluster_set_flag(struct swap_cluster_info *info,
@@ -374,6 +378,14 @@ static void swap_cluster_schedule_discard(struct swap_info_struct *si,
 	schedule_work(&si->discard_work);
 }

+static void __free_cluster(struct swap_info_struct *si, unsigned long idx)
+{
+	struct swap_cluster_info *ci = si->cluster_info;
+
+	cluster_set_flag(ci + idx, CLUSTER_FLAG_FREE);
+	cluster_list_add_tail(&si->free_clusters, ci, idx);
+}
+
 /*
  * Doing discard actually. After a cluster discard is finished, the cluster
  * will be added to free cluster list. caller should hold si->lock.
@@ -394,10 +406,7 @@ static void swap_do_scheduled_discard(struct swap_info_struct *si)

 		spin_lock(&si->lock);
 		ci = lock_cluster(si, idx * SWAPFILE_CLUSTER);
-		cluster_set_flag(ci, CLUSTER_FLAG_FREE);
-		unlock_cluster(ci);
-		cluster_list_add_tail(&si->free_clusters, info, idx);
-		ci = lock_cluster(si, idx * SWAPFILE_CLUSTER);
+		__free_cluster(si, idx);
 		memset(si->swap_map + idx * SWAPFILE_CLUSTER,
 				0, SWAPFILE_CLUSTER);
 		unlock_cluster(ci);
@@ -415,6 +424,34 @@ static void swap_discard_work(struct work_struct *work)
 	spin_unlock(&si->lock);
 }

+static void alloc_cluster(struct swap_info_struct *si, unsigned long idx)
+{
+	struct swap_cluster_info *ci = si->cluster_info;
+
+	VM_BUG_ON(cluster_list_first(&si->free_clusters) != idx);
+	cluster_list_del_first(&si->free_clusters, ci);
+	cluster_set_count_flag(ci + idx, 0, 0);
+}
+
+static void free_cluster(struct swap_info_struct *si, unsigned long idx)
+{
+	struct swap_cluster_info *ci = si->cluster_info + idx;
+
+	VM_BUG_ON(cluster_count(ci) != 0);
+	/*
+	 * If the swap is discardable, prepare discard the cluster
+	 * instead of free it immediately. The cluster will be freed
+	 * after discard.
+	 */
+	if ((si->flags & (SWP_WRITEOK | SWP_PAGE_DISCARD)) ==
+	    (SWP_WRITEOK | SWP_PAGE_DISCARD)) {
+		swap_cluster_schedule_discard(si, idx);
+		return;
+	}
+
+	__free_cluster(si, idx);
+}
+
 /*
  * The cluster corresponding to page_nr will be used. The cluster will be
  * removed from free cluster list and its usage counter will be increased.
@@ -426,11 +463,8 @@ static void inc_cluster_info_page(struct swap_info_struct *p,

 	if (!cluster_info)
 		return;
-	if (cluster_is_free(&cluster_info[idx])) {
-		VM_BUG_ON(cluster_list_first(&p->free_clusters) != idx);
-		cluster_list_del_first(&p->free_clusters, cluster_info);
-		cluster_set_count_flag(&cluster_info[idx], 0, 0);
-	}
+	if (cluster_is_free(&cluster_info[idx]))
+		alloc_cluster(p, idx);

 	VM_BUG_ON(cluster_count(&cluster_info[idx]) >= SWAPFILE_CLUSTER);
 	cluster_set_count(&cluster_info[idx],
@@ -454,21 +488,8 @@ static void dec_cluster_info_page(struct swap_info_struct *p,
 	cluster_set_count(&cluster_info[idx],
 		cluster_count(&cluster_info[idx]) - 1);

-	if (cluster_count(&cluster_info[idx]) == 0) {
-		/*
-		 * If the swap is discardable, prepare discard the cluster
-		 * instead of free it immediately. The cluster will be freed
-		 * after discard.
-		 */
-		if ((p->flags & (SWP_WRITEOK | SWP_PAGE_DISCARD)) ==
-				 (SWP_WRITEOK | SWP_PAGE_DISCARD)) {
-			swap_cluster_schedule_discard(p, idx);
-			return;
-		}
-
-		cluster_set_flag(&cluster_info[idx], CLUSTER_FLAG_FREE);
-		cluster_list_add_tail(&p->free_clusters, cluster_info, idx);
-	}
+	if (cluster_count(&cluster_info[idx]) == 0)
+		free_cluster(p, idx);
 }

 /*
@@ -558,6 +579,60 @@ static bool scan_swap_map_try_ssd_cluster(struct swap_info_struct *si,
 	return found_free;
 }

+static void swap_range_alloc(struct swap_info_struct *si, unsigned long offset,
+			     unsigned int nr_entries)
+{
+	unsigned int end = offset + nr_entries - 1;
+
+	if (offset == si->lowest_bit)
+		si->lowest_bit += nr_entries;
+	if (end == si->highest_bit)
+		si->highest_bit -= nr_entries;
+	si->inuse_pages += nr_entries;
+	if (si->inuse_pages == si->pages) {
+		si->lowest_bit = si->max;
+		si->highest_bit = 0;
+		spin_lock(&swap_avail_lock);
+		plist_del(&si->avail_list, &swap_avail_head);
+		spin_unlock(&swap_avail_lock);
+	}
+}
+
+static void swap_range_free(struct swap_info_struct *si, unsigned long offset,
+			    unsigned int nr_entries)
+{
+	unsigned long end = offset + nr_entries - 1;
+	void (*swap_slot_free_notify)(struct block_device *, unsigned long);
+
+	if (offset < si->lowest_bit)
+		si->lowest_bit = offset;
+	if (end > si->highest_bit) {
+		bool was_full = !si->highest_bit;
+
+		si->highest_bit = end;
+		if (was_full && (si->flags & SWP_WRITEOK)) {
+			spin_lock(&swap_avail_lock);
+			WARN_ON(!plist_node_empty(&si->avail_list));
+			if (plist_node_empty(&si->avail_list))
+				plist_add(&si->avail_list, &swap_avail_head);
+			spin_unlock(&swap_avail_lock);
+		}
+	}
+	atomic_long_add(nr_entries, &nr_swap_pages);
+	si->inuse_pages -= nr_entries;
+	if (si->flags & SWP_BLKDEV)
+		swap_slot_free_notify =
+			si->bdev->bd_disk->fops->swap_slot_free_notify;
+	else
+		swap_slot_free_notify = NULL;
+	while (offset <= end) {
+		frontswap_invalidate_page(si->type, offset);
+		if (swap_slot_free_notify)
+			swap_slot_free_notify(si->bdev, offset);
+		offset++;
+	}
+}
+
 static int scan_swap_map_slots(struct swap_info_struct *si,
 			       unsigned char usage, int nr,
 			       swp_entry_t slots[])
@@ -676,18 +751,7 @@ static int scan_swap_map_slots(struct swap_info_struct *si,
 	inc_cluster_info_page(si, si->cluster_info, offset);
 	unlock_cluster(ci);

-	if (offset == si->lowest_bit)
-		si->lowest_bit++;
-	if (offset == si->highest_bit)
-		si->highest_bit--;
-	si->inuse_pages++;
-	if (si->inuse_pages == si->pages) {
-		si->lowest_bit = si->max;
-		si->highest_bit = 0;
-		spin_lock(&swap_avail_lock);
-		plist_del(&si->avail_list, &swap_avail_head);
-		spin_unlock(&swap_avail_lock);
-	}
+	swap_range_alloc(si, offset, 1);
 	si->cluster_next = offset + 1;
 	slots[n_ret++] = swp_entry(si->type, offset);

@@ -766,6 +830,52 @@ static int scan_swap_map_slots(struct swap_info_struct *si,
 	return n_ret;
 }

+#ifdef CONFIG_THP_SWAP
+static int swap_alloc_cluster(struct swap_info_struct *si, swp_entry_t *slot)
+{
+	unsigned long idx;
+	struct swap_cluster_info *ci;
+	unsigned long offset, i;
+	unsigned char *map;
+
+	if (cluster_list_empty(&si->free_clusters))
+		return 0;
+
+	idx = cluster_list_first(&si->free_clusters);
+	offset = idx * SWAPFILE_CLUSTER;
+	ci = lock_cluster(si, offset);
+	alloc_cluster(si, idx);
+	cluster_set_count_flag(ci, SWAPFILE_CLUSTER, 0);
+
+	map = si->swap_map + offset;
+	for (i = 0; i < SWAPFILE_CLUSTER; i++)
+		map[i] = SWAP_HAS_CACHE;
+	unlock_cluster(ci);
+	swap_range_alloc(si, offset, SWAPFILE_CLUSTER);
+	*slot = swp_entry(si->type, offset);
+
+	return 1;
+}
+
+static void swap_free_cluster(struct swap_info_struct *si, unsigned long idx)
+{
+	unsigned long offset = idx * SWAPFILE_CLUSTER;
+	struct swap_cluster_info *ci;
+
+	ci = lock_cluster(si, offset);
+	cluster_set_count_flag(ci, 0, 0);
+	free_cluster(si, idx);
+	unlock_cluster(ci);
+	swap_range_free(si, offset, SWAPFILE_CLUSTER);
+}
+#else
+static int swap_alloc_cluster(struct swap_info_struct *si, swp_entry_t *slot)
+{
+	VM_WARN_ON_ONCE(1);
+	return 0;
+}
+#endif /* CONFIG_THP_SWAP */
+
 static unsigned long scan_swap_map(struct swap_info_struct *si,
 				   unsigned char usage)
 {
@@ -781,13 +891,17 @@ static unsigned long scan_swap_map(struct swap_info_struct *si,

 }

-int get_swap_pages(int n_goal, swp_entry_t swp_entries[])
+int get_swap_pages(int n_goal, bool cluster, swp_entry_t swp_entries[])
 {
+	unsigned long nr_pages = cluster ? SWAPFILE_CLUSTER : 1;
 	struct swap_info_struct *si, *next;
 	long avail_pgs;
 	int n_ret = 0;

-	avail_pgs = atomic_long_read(&nr_swap_pages);
+	/* Only single cluster request supported */
+	WARN_ON_ONCE(n_goal > 1 && cluster);
+
+	avail_pgs = atomic_long_read(&nr_swap_pages) / nr_pages;
 	if (avail_pgs <= 0)
 		goto noswap;

@@ -797,7 +911,7 @@ int get_swap_pages(int n_goal, swp_entry_t swp_entries[])
 	if (n_goal > avail_pgs)
 		n_goal = avail_pgs;

-	atomic_long_sub(n_goal, &nr_swap_pages);
+	atomic_long_sub(n_goal * nr_pages, &nr_swap_pages);

 	spin_lock(&swap_avail_lock);

@@ -823,10 +937,13 @@ int get_swap_pages(int n_goal, swp_entry_t swp_entries[])
 			spin_unlock(&si->lock);
 			goto nextsi;
 		}
-		n_ret = scan_swap_map_slots(si, SWAP_HAS_CACHE,
-					    n_goal, swp_entries);
+		if (cluster)
+			n_ret = swap_alloc_cluster(si, swp_entries);
+		else
+			n_ret = scan_swap_map_slots(si, SWAP_HAS_CACHE,
+						    n_goal, swp_entries);
 		spin_unlock(&si->lock);
-		if (n_ret)
+		if (n_ret || cluster)
 			goto check_out;
 		pr_debug("scan_swap_map of si %d failed to find offset\n",
 			si->type);
@@ -852,7 +969,8 @@ int get_swap_pages(int n_goal, swp_entry_t swp_entries[])

 check_out:
 	if (n_ret < n_goal)
-		atomic_long_add((long) (n_goal-n_ret), &nr_swap_pages);
+		atomic_long_add((long)(n_goal - n_ret) * nr_pages,
+				&nr_swap_pages);
 noswap:
 	return n_ret;
 }
@@ -1008,32 +1126,8 @@ static void swap_entry_free(struct swap_info_struct *p, swp_entry_t entry)
 	dec_cluster_info_page(p, p->cluster_info, offset);
 	unlock_cluster(ci);

-	mem_cgroup_uncharge_swap(entry);
-	if (offset < p->lowest_bit)
-		p->lowest_bit = offset;
-	if (offset > p->highest_bit) {
-		bool was_full = !p->highest_bit;
-
-		p->highest_bit = offset;
-		if (was_full && (p->flags & SWP_WRITEOK)) {
-			spin_lock(&swap_avail_lock);
-			WARN_ON(!plist_node_empty(&p->avail_list));
-			if (plist_node_empty(&p->avail_list))
-				plist_add(&p->avail_list,
-					  &swap_avail_head);
-			spin_unlock(&swap_avail_lock);
-		}
-	}
-	atomic_long_inc(&nr_swap_pages);
-	p->inuse_pages--;
-	frontswap_invalidate_page(p->type, offset);
-	if (p->flags & SWP_BLKDEV) {
-		struct gendisk *disk = p->bdev->bd_disk;
-
-		if (disk->fops->swap_slot_free_notify)
-			disk->fops->swap_slot_free_notify(p->bdev,
-							  offset);
-	}
+	mem_cgroup_uncharge_swap(entry, 1);
+	swap_range_free(p, offset, 1);
 }

 /*
@@ -1065,6 +1159,33 @@ void swapcache_free(swp_entry_t entry)
 	}
 }

+#ifdef CONFIG_THP_SWAP
+void swapcache_free_cluster(swp_entry_t entry)
+{
+	unsigned long offset = swp_offset(entry);
+	unsigned long idx = offset / SWAPFILE_CLUSTER;
+	struct swap_cluster_info *ci;
+	struct swap_info_struct *si;
+	unsigned char *map;
+	unsigned int i;
+
+	si = swap_info_get(entry);
+	if (!si)
+		return;
+
+	ci = lock_cluster(si, offset);
+	map = si->swap_map + offset;
+	for (i = 0; i < SWAPFILE_CLUSTER; i++) {
+		VM_BUG_ON(map[i] != SWAP_HAS_CACHE);
+		map[i] = 0;
+	}
+	unlock_cluster(ci);
+	mem_cgroup_uncharge_swap(entry, SWAPFILE_CLUSTER);
+	swap_free_cluster(si, idx);
+	spin_unlock(&si->lock);
+}
+#endif /* CONFIG_THP_SWAP */
+
 void swapcache_free_entries(swp_entry_t *entries, int n)
 {
 	struct swap_info_struct *p, *prev;
```
