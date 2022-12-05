# idle page tracking

- https://www.kernel.org/doc/html/latest/admin-guide/mm/idle_page_tracking.html
- https://lwn.net/Articles/461461/
- https://lwn.net/Articles/643578/ : 原始的 patch


- 原来 page idle 和 page young 和这个有关系:

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

- 因为默认状态下，将 page 设置为 idle，如果访问了，那么调用 folio_clear_idle 将这个 bit 清理掉。
- 采用 page young 的原因: 因为 idle page tracking 机制让会清理掉 reference bit，影响 page reclaim 的正常工作
所以，将原来的 page reference 放到 page young 中来保存。

## 终于可以理解
- [ ] page_is_young()

## wss 估算
https://www.brendangregg.com/blog/2018-01-17/measure-working-set-size.html
