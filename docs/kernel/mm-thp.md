# THP
- 原来的那个文章找过来看看

- [ ] PageDoubleMap
- [ ] THP only support PMD ? so can it support more than 2M space (21bit) ?
- [ ] https://gist.github.com/shino/5d9aac68e7ebf03d4962a4c07c503f7d, check references in it
- [ ] 提供的硬件支持是什么 ?
    - [ ] 除了在 pml4 pud pmd 的 page table 上的 flags
        - [ ] /sys/kernel/mm/transparent_hugepage/hpage_pmd_size 的含义看，实际上，内核只是支持一共大小的 hugepage
    - [ ] 需要提供 TLB 知道自己正在访问虚拟地址是否被 hugetlb 映射


使用 transparent hugepage 的原因:
1. TLB 的覆盖更大，可以降低 TLB miss rate
3. hugepage 的出现让原先的 page walk 的路径变短了

几个 THP 需要考虑的核心问题:
1. swap
2. reference 的问题
3. split 和 merge

## transparent hugepage 和 swap 是相关的吗

## How to use hugepages with tmpfs
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

## [The transparent huge page shrinker](https://lwn.net/Articles/906511/)

lwn 作者认为如果加上这个，那么 thp 就可以成为默认参数。

## how to disable thp
- https://www.thegeekdiary.com/centos-rhel-7-how-to-disable-transparent-huge-pages-thp/
  - 实际上，不仅仅需要在 grub 中 disable 的，而且需要考虑 tune


#### THP admin manual
[用户手册](https://www.kernel.org/doc/html/latest/admin-guide/mm/transhuge.html)

The THP behaviour is controlled via `sysfs` interface and using `madvise(2)` and `prctl(2)` system calls.

- [ ] how madvise and prctl control the THP

Currently THP **only works for** anonymous memory mappings and tmpfs/shmem. But in the future it can expand to other filesystems.

- [ ] so page cache can't work with THP ?

THP 相对于 hugetlbfs 的优势:
- Transparent Hugepage Support maximizes the usefulness of free memory if compared to the reservation approach of hugetlbfs by allowing all unused memory to be used as cache or other movable (or even unmovable entities).
- It doesn’t require reservation to prevent hugepage allocation failures to be noticeable from userland. *It allows paging and all other advanced VM features to be available on the hugepages.*
- It requires no modifications for applications to take advantage of it.

- [x] 在 hugepage 上可以使用 paging 等 advanced VM feaures. ( Paging is a mechanism that translates a linear memory address to a physical address.)
    - [x] paging sometimes meaning page fault

interface in sysfs :
1. /sys/kernel/mm/transparent_hugepage : always madvise never
2. /sys/kernel/mm/transparent_hugepage/defrag : always defer defer + madvise madvise never
3. You can control hugepage allocation policy in tmpfs with mount option huge=. It can have following values: always never advise deny force

- [ ] 应该按照手册，将手册中间的说明在内核中间一个个的找到
  - [ ] /sys/kernel/mm/transparent_hugepage
    - [ ] always 指的是任何位置都需要 hugepage 处理吗?
  - [ ] /sys/kernel/mm/transparent_hugepage/defrag 的 always 无法理解，或者说，什么时候应该触发 defrag, 不是分配的时候就是决定了吗 ?
- [ ] THP has to defrag pages, so check the compaction.c and find out how thp deal with it !
  - [ ] how defrag wake kcompactd ?

- [x] mmap 添加上 hugepage 的参数，是不是几乎等价于普通 mmap，然后 madvice
  - 不是，一个是 madvise， 一个是 thp

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

khugepaged.c 中间的 hugepage 守护进程的工作是什么 ?

[Transparent huge page reference counting](https://lwn.net/Articles/619738/)

> In particular, he has eliminated the hard separation between normal and huge pages in the system. In current kernels, a specific 4KB page can be treated as an individual page, or it can be part of a huge page, but not both. If a huge page must be split into individual pages, it is split completely for all users, the compound page structure is torn down, and the huge page no longer exists. The fundamental change in Kirill's patch set is to allow a huge page to be split in one process's address space, while remaining a huge page in any other address space where it is found.

- [ ] what's the flag in PMD page table entry used to suggest the page is huge page ? verify it in intel manual.

- [ ] page_trans_huge_mapcount
- [ ] total_mapcount

[Transparent huge pages for filesystems](https://lwn.net/Articles/789159/)

> It is using the [Binary Optimization and Layout Tool (BOLT)](https://github.com/facebookincubator/BOLT) to profile its code in order to identify the hot functions. Those functions are collected up into an 8MB region in the generated executable.


#### THP khugepaged
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

# sysfs

由于 linux-kernel-labs 的试验基础，sysfs 实现机制简单，

sysfs_create_group 的含义暂时无法理解。


```c
static int __init hugepage_init_sysfs(struct kobject **hugepage_kobj)
{
	int err;

	*hugepage_kobj = kobject_create_and_add("transparent_hugepage", mm_kobj);
	if (unlikely(!*hugepage_kobj)) {
		pr_err("failed to create transparent hugepage kobject\n");
		return -ENOMEM;
	}

	err = sysfs_create_group(*hugepage_kobj, &hugepage_attr_group);
	if (err) {
		pr_err("failed to register transparent hugepage group\n");
		goto delete_obj;
	}

	err = sysfs_create_group(*hugepage_kobj, &khugepaged_attr_group);
	if (err) {
		pr_err("failed to register transparent hugepage group\n");
		goto remove_hp_group;
	}

	return 0;

remove_hp_group:
	sysfs_remove_group(*hugepage_kobj, &hugepage_attr_group);
delete_obj:
	kobject_put(*hugepage_kobj);
	return err;
}

static struct attribute *hugepage_attr[] = {
	&enabled_attr.attr,
	&defrag_attr.attr,
	&use_zero_page_attr.attr,
	&hpage_pmd_size_attr.attr,
#if defined(CONFIG_SHMEM) && defined(CONFIG_TRANSPARENT_HUGE_PAGECACHE)
	&shmem_enabled_attr.attr, // 除了这个以外，其他都是处理 transparent_hugepage_flags 的标志位
#endif
	NULL,
};

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
