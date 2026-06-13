# idle page tracking

- [Idle Page Tracking](https://www.kernel.org/doc/html/latest/admin-guide/mm/idle_page_tracking.html)
- [Idle and stale page tracking](https://lwn.net/Articles/461461/)
- https://lwn.net/Articles/643578/ : 原始的 patch

```diff
commit 33c3fc71c8cfa3cc3a98beaa901c069c177dc295
Author: Vladimir Davydov <vdavydov.dev@gmail.com>
Date:   Wed Sep 9 15:35:45 2015 -0700

    mm: introduce idle page tracking

    Knowing the portion of memory that is not used by a certain application or
    memory cgroup (idle memory) can be useful for partitioning the system
    efficiently, e.g.  by setting memory cgroup limits appropriately.
    Currently, the only means to estimate the amount of idle memory provided
    by the kernel is /proc/PID/{clear_refs,smaps}: the user can clear the
    access bit for all pages mapped to a particular process by writing 1 to
    clear_refs, wait for some time, and then count smaps:Referenced.  However,
    this method has two serious shortcomings:

     - it does not count unmapped file pages
     - it affects the reclaimer logic

    To overcome these drawbacks, this patch introduces two new page flags,
    Idle and Young, and a new sysfs file, /sys/kernel/mm/page_idle/bitmap.
    A page's Idle flag can only be set from userspace by setting bit in
    /sys/kernel/mm/page_idle/bitmap at the offset corresponding to the page,
    and it is cleared whenever the page is accessed either through page tables
    (it is cleared in page_referenced() in this case) or using the read(2)
    system call (mark_page_accessed()). Thus by setting the Idle flag for
    pages of a particular workload, which can be found e.g.  by reading
    /proc/PID/pagemap, waiting for some time to let the workload access its
    working set, and then reading the bitmap file, one can estimate the amount
    of pages that are not used by the workload.

    The Young page flag is used to avoid interference with the memory
    reclaimer.  A page's Young flag is set whenever the Access bit of a page
    table entry pointing to the page is cleared by writing to the bitmap file.
    If page_referenced() is called on a Young page, it will add 1 to its
    return value, therefore concealing the fact that the Access bit was
    cleared.

    Note, since there is no room for extra page flags on 32 bit, this feature
    uses extended page flags when compiled on 32 bit.
```

- 从在用户态中才可以将 page 设置为 idle，如果通过 page_referenced 或者 mark_page_accessed 访问过，那么调用 folio_clear_idle 将这个 bit 清理掉。

- 采用 page young 的原因: 因为 idle page tracking 机制让会清理掉 reference bit，影响 page reclaim 的正常工作


## 为什么需要 young flag

folio_referenced_one

不要 young flags 可以保证 folio_referenced 返回值是正确的，但是 PG_referenced 是存在特殊含义的:

从 folio_check_references 中，不是简单的将

## wss 估算
https://www.brendangregg.com/blog/2018-01-17/measure-working-set-size.html

## 会被 THP 影响吗?
我猜测，看这个代码，似乎完全没有考虑 THP 的意思啊!

```c
static ssize_t page_idle_bitmap_read(struct file *file, struct kobject *kobj,
				     struct bin_attribute *attr, char *buf,
				     loff_t pos, size_t count)
{
	u64 *out = (u64 *)buf;
	struct page *page;
	unsigned long pfn, end_pfn;
	int bit;

	if (pos % BITMAP_CHUNK_SIZE || count % BITMAP_CHUNK_SIZE)
		return -EINVAL;

	pfn = pos * BITS_PER_BYTE;
	if (pfn >= max_pfn)
		return 0;

	end_pfn = pfn + count * BITS_PER_BYTE;
	if (end_pfn > max_pfn)
		end_pfn = max_pfn;

	for (; pfn < end_pfn; pfn++) {
		bit = pfn % BITMAP_CHUNK_BITS;
		if (!bit)
			*out = 0ULL;
		page = page_idle_get_page(pfn);
		if (page) {
			if (page_is_idle(page)) {
				/*
				 * The page might have been referenced via a
				 * pte, in which case it is not idle. Clear
				 * refs and recheck.
				 */
				page_idle_clear_pte_refs(page);
				if (page_is_idle(page))
					*out |= 1ULL << bit;
			}
			put_page(page);
		}
		if (bit == BITMAP_CHUNK_BITS - 1)
			out++;
		cond_resched();
	}
	return (char *)out - buf;
}
```
我猜测是因为 page_idle_clear_pte_refs 无人安排的。

## /proc/self/clear_refs

对于所有的映射的 page 调用:
```c
		/* Clear accessed and referenced bits. */
		ptep_test_and_clear_young(vma, addr, pte);
		test_and_clear_page_young(page);
		ClearPageReferenced(page);
```

## pagemap
https://www.kernel.org/doc/Documentation/vm/pagemap.txt 介绍了四个接口：

- /proc/pid/pagemap
- /proc/kpagecount
- /proc/kpageflags
- /proc/kpagecgroup

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

## idle page tracking 是无法处理大页的
- page_idle_get_page 中 `PageLRU` 无法满足。

## `__split_huge_page_tail` 中

page_idle_clear_pte_refs_one 也是被 referenced 过的啊

```c
	if (page_is_young(head))
		set_page_young(page_tail);
```

## 和 mmu notifer 的关系

```c
#define ptep_clear_young_notify(__vma, __address, __ptep)		\
({									\
	int __young;							\
	struct vm_area_struct *___vma = __vma;				\
	unsigned long ___address = __address;				\
	__young = ptep_test_and_clear_young(___vma, ___address, __ptep);\
	__young |= mmu_notifier_clear_young(___vma->vm_mm, ___address,	\
					    ___address + PAGE_SIZE);	\
	__young;							\
})
```

```c
static const struct mmu_notifier_ops kvm_mmu_notifier_ops = {
	.invalidate_range	= kvm_mmu_notifier_invalidate_range,
	.invalidate_range_start	= kvm_mmu_notifier_invalidate_range_start,
	.invalidate_range_end	= kvm_mmu_notifier_invalidate_range_end,
	.clear_flush_young	= kvm_mmu_notifier_clear_flush_young,
	.clear_young		= kvm_mmu_notifier_clear_young,
	.test_young		= kvm_mmu_notifier_test_young,
	.change_pte		= kvm_mmu_notifier_change_pte,
	.release		= kvm_mmu_notifier_release,
};
```
- [ ] `__collapse_huge_page_isolate` -> test_young : 之后分析透明大页的时候再说吧
- page_idle_clear_pte_refs_one -> clear_young : 收集 page 是否被访问过
```txt
@[
    kvm_tdp_mmu_age_gfn_range+1
    kvm_age_gfn+436
    kvm_mmu_notifier_clear_young+217
    __mmu_notifier_clear_young+84
    page_idle_clear_pte_refs_one+348
    rmap_walk_anon+360
    page_idle_bitmap_write+148
    kernfs_fop_write_iter+289
    vfs_write+702
    __x64_sys_pwrite64+144
    do_syscall_64+56
    entry_SYSCALL_64_after_hwframe+99
]
```
- folio_referenced_one -> clear_flush_young : 为什么这个需要 flush tdp 中的二级 TLB 啊，难道 tdp 中的二级 TLB 中含有 dirty bit 吗?

## idle page tracking 支持嵌套虚拟化吗?
- [ ] 理解下，嵌套虚拟化下，访存是如何进行的。
