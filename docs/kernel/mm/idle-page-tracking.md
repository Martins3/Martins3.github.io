# Idle Page Tracking

- [Idle Page Tracking](https://www.kernel.org/doc/html/latest/admin-guide/mm/idle_page_tracking.html)

解释了，为什么 idle page tracking 不会影响 reclaim 运行的原因。

> When a page is marked idle, the Accessed bit must be cleared in all PTEs it is mapped to, otherwise we will not be able to detect accesses to the page coming from a process address space. To avoid interference with the reclaimer, which, as noted above, uses the Accessed bit to promote actively referenced pages, one more page flag is introduced, the Young flag. When the PTE Accessed bit is cleared as a result of setting or updating a page’s Idle flag, the Young flag is set on the page. The reclaimer treats the Young flag as an extra PTE Accessed bit and therefore will consider such a page as referenced.


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
- [ ] 理解下，嵌套虚拟化下，这些 bit 是如何设置的

https://patchwork.kernel.org/project/kvm/patch/1490867732-16743-5-git-send-email-pbonzini@redhat.com/

## 思考清楚基本问题

不是这样设计的:
| opts  | idle page tracking                                    | vmscan                                  |
|-------|-------------------------------------------------------|-----------------------------------------|
| check | 直接检查 access bit 即可                              | 无论是 access bit 还是 young bit 都可以 |
| clear | 将 access bit 移动到 young bit ，将 access bit 清理掉 | ... 必然干扰                            |

需要使用 page->flags 中的两个 flag 才可以:
1. 无论是 idle page tracking 还是 vmscan ，都是需要将 pte 的 access 清理掉的
2. 只能说，vmscan 对于 idle page tracking 没有影响，但是不可以反过来
除非在使用一个 page flags 来记录。
3. PG_idle 和 PG_young 都是仅仅给 idle page tracking 和 damon 用的

实现方法: vmscan 每次都直接将 PG_young 和 pte 都清理，再次检查的时候，如果 folio_test_clear_young 或者 pte 有，那是就是访问过。

idle page tracking 只是用来估算的。

- [ ] 所以，PG_idle 和 PG_young 不是简单的互斥吗? 什么搞两个

具体实现在: page_idle_clear_pte_refs_one 中

```txt
	if (referenced) {
		folio_clear_idle(folio);
		/*
		 * We cleared the referenced bit in a mapping to this page. To
		 * avoid interference with page reclaim, mark it young so that
		 * folio_referenced() will return > 0.
		 */
		folio_set_young(folio); // 除了 damon 外，唯一使用的地方
	}
```

和 page_idle_bitmap_read 中:
```txt
				page_idle_clear_pte_refs(folio);
				if (folio_test_idle(folio))
					*out |= 1ULL << bit;
```

而 vmscan 的逻辑是，在 folio_referenced_one 中:
```txt
	if (referenced)
		folio_clear_idle(folio);
	if (folio_test_clear_young(folio))
		referenced++;
```



基本位置: folio_referenced_one -> ptep_clear_flush_young_notify


### mmu notifier 的问题，flush 的区别是什么?
- rmpa.c 中的 folio_referenced_one  : ptep_clear_flush_young_notify
- page_idle.c 中的 page_idle_clear_pte_refs_one -> ptep_clear_young_notify

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
