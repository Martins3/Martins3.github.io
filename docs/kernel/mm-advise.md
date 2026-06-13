# madvise

- madvise 告知内核该范围的内存如何访问
- fadvise 告知内核该范围的文件如何访问，内核从而可以调节 readahead 的参数，或者清理掉该范围的 page cache

fadvise 很简单，阅读 2. mm/fadvise.c 的源代码只有 200 行，具体可以看看 Man fadvise(2)

## 总体分析


| Name                 | Comment |
|----------------------|---------|--------------------------------------------------------------------------|
| MADV_NORMAL          | 0       | no further special treatment                                             |
| MADV_RANDOM          | 1       | expect random page references                                            |
| MADV_SEQUENTIAL      | 2       | expect sequential page references                                        |
| MADV_WILLNEED        | 3       | will need these pages                                                    |
| MADV_DONTNEED        | 4       | don't need these pages                                                   |
| MADV_FREE            | 8       | free pages only if memory pressure                                       |
| MADV_REMOVE          | 9       | remove these pages & resources                                           |
| MADV_DONTFORK        | 10      | don't inherit across fork                                                |
| MADV_DOFORK          | 11      | do inherit across fork                                                   |
| MADV_HWPOISON        | 100     | poison a page for testing                                                |
| MADV_SOFT_OFFLINE    | 101     | soft offline page for testing                                            |
| MADV_MERGEABLE       | 12      | KSM may merge identical pages                                            |
| MADV_UNMERGEABLE     | 13      | KSM may not merge identical pages                                        |
| MADV_HUGEPAGE        | 14      | Worth backing with hugepages                                             |
| MADV_NOHUGEPAGE      | 15      | Not worth backing with hugepages                                         |
| MADV_DONTDUMP        | 16      | Explicity exclude from the core dump, overrides the coredump filter bits |
| MADV_DODUMP          | 17      | Clear the MADV_DONTDUMP flag                                             |
| MADV_WIPEONFORK      | 18      | Zero memory on fork, child only                                          |
| MADV_KEEPONFORK      | 19      | Undo MADV_WIPEONFORK                                                     |
| MADV_COLD            | 20      | deactivate these pages                                                   |
| MADV_PAGEOUT         | 21      | reclaim these pages                                                      |
| MADV_POPULATE_READ   | 22      | populate (prefault) page tables readable                                 |
| MADV_POPULATE_WRITE  | 23      | populate (prefault) page tables writable                                 |
| MADV_DONTNEED_LOCKED | 24      | like DONTNEED, but drop locked pages too                                 |
| MADV_COLLAPSE        | 25      | Synchronous hugepage collapse                                            |

- MADV_COLD : deactivate_page : 将 page active 的 lru 中放到 inactive 中
- MADV_PAGEOUT : reclaim_pages -> shrink_list -> reclaim_page_list -> shrink_page_list 将 page 写回。
## madvise_collapse

- madvise_collapse
  - hpage_collapse_scan_file : 文件映射，或者匿名映射
  - hpage_collapse_scan_pmd : anon 映射

khugepaged 的替代品，让一个区域映射之后进行，当然，需要一个范围中已经存在足够多的页才可以。

## MAP_COLD

其中的 page walk 框架可以分析一下。

- 核心: madvise_cold_or_pageout_pte_range

- [ ] 似乎很多位置都是这个类似的这种结构

```c
static long madvise_cold(struct vm_area_struct *vma,
            struct vm_area_struct **prev,
            unsigned long start_addr, unsigned long end_addr)
{
    struct mm_struct *mm = vma->vm_mm;
    struct mmu_gather tlb;

    *prev = vma;
    if (!can_madv_lru_vma(vma))
        return -EINVAL;

    lru_add_drain(); // @todo 为什么首先让 cpu 释放自己的 page ?
    tlb_gather_mmu(&tlb, mm); // 猜测是，将范围中 需要特殊处理的 page 放到此处
    madvise_cold_page_range(&tlb, vma, start_addr, end_addr);
    tlb_finish_mmu(&tlb);

    return 0;
}
```

- madvise_cold_page_range
  - tlb_start_vma
  - walk_page_range
    - `__walk_page_range` : 这个进入到常规的 page walk 的过程中
  - tlb_end_vma

```c
static inline bool can_madv_lru_vma(struct vm_area_struct *vma)
{
    return !(vma->vm_flags & (VM_LOCKED|VM_PFNMAP|VM_HUGETLB));
}
```

- [ ] 为什么这三种都不可以来作为?
- [ ] VM_LOCKED ?
- [ ] VM_PFNMAP ?

```txt
man madvise(2)
              MADV_DONTNEED cannot be applied to locked pages, Huge TLB pages, or VM_PFNMAP pages.  (Pages marked with the kernel-in‐
              ternal VM_PFNMAP flag are special memory areas that are not managed by the virtual memory subsystem.   Such  pages  are
              typically created by device drivers that map the pages into user space.)

```

- [ ] 为什么是在 pmd 上注册的哇?

```c
static const struct mm_walk_ops cold_walk_ops = {
    .pmd_entry = madvise_cold_or_pageout_pte_range,
};
```

- MADV_COLD 和 MADV_PAGEOUT 共用这一个 handler : madvise_cold_or_pageout_pte_range

```diff
History:        #0
Commit:         1a4e58cce84ee88129d5d49c064bd2852b481357
Author:         Minchan Kim <minchan@kernel.org>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Thu 26 Sep 2019 07:49:15 AM CST
Committer Date: Thu 26 Sep 2019 08:51:41 AM CST

mm: introduce MADV_PAGEOUT

When a process expects no accesses to a certain memory range for a long
time, it could hint kernel that the pages can be reclaimed instantly but
data should be preserved for future use.  This could reduce workingset
eviction so it ends up increasing performance.

This patch introduces the new MADV_PAGEOUT hint to madvise(2) syscall.
MADV_PAGEOUT can be used by a process to mark a memory range as not
expected to be used for a long time so that kernel reclaims *any LRU*
pages instantly.  The hint can help kernel in deciding which pages to
evict proactively.

A note: It doesn't apply SWAP_CLUSTER_MAX LRU page isolation limit
intentionally because it's automatically bounded by PMD size.  If PMD
size(e.g., 256) makes some trouble, we could fix it later by limit it to
SWAP_CLUSTER_MAX[1].

- man-page material

MADV_PAGEOUT (since Linux x.x)

Do not expect access in the near future so pages in the specified
regions could be reclaimed instantly regardless of memory pressure.
Thus, access in the range after successful operation could cause
major page fault but never lose the up-to-date contents unlike
MADV_DONTNEED. Pages belonging to a shared mapping are only processed
if a write access is allowed for the calling process.

MADV_PAGEOUT cannot be applied to locked pages, Huge TLB pages, or
VM_PFNMAP pages.

[1] https://lore.kernel.org/lkml/20190710194719.GS29695@dhcp22.suse.cz/
```

## MADV_POPULATE_WRITE

使用多线程的性能就是会比单线程性能更好吗?

- madvise_populate 会申请 mmap_read_lock

- faultin_vma_page_range 中，同时出现这两个代码，看来 lock 的含义是我不懂的

```c
	mmap_assert_locked(mm);

	ret = __get_user_pages(mm, start, nr_pages, gup_flags,
				NULL, NULL, locked);
```

其实 QEMU 是存在考虑的:

```c
    /*
     * On Linux, the page faults from the loop below can cause mmap_sem
     * contention with allocation of the thread stacks.  Do not start
     * clearing until all threads have been created.
     */
    qemu_mutex_lock(&page_mutex);
    while (!memset_args->context->all_threads_created) {
        qemu_cond_wait(&page_cond, &page_mutex);
    }
    qemu_mutex_unlock(&page_mutex);
```

调查一下这个，是不是没有考虑到这个问题哇?
https://mp.weixin.qq.com/s/R8BVIrps5UPXVncvbGkkug

## [ ] 我一时间分不清楚 MAP_NONEED 和 MADV_FREE 了

- https://news.ycombinator.com/item?id=23216590
