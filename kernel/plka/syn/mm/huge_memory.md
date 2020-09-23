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



## huge_fault 到底注册的怎样的函数 ?


## anonymous 和 shared 都是如何初始化的 ?


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




