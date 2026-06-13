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
跳过这三个页面是非常合理的:
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


## 想不到 2020 的时候，引入了一个新的 syscall : process_madvise

```txt
History:        #0
Commit:         ecb8ac8b1f146915aa6b96449b66dd48984caacc
Author:         Minchan Kim <minchan@kernel.org>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Sat 17 Oct 2020 07:14:59 PM EDT
Committer Date: Sun 18 Oct 2020 12:27:10 PM EDT

mm/madvise: introduce process_madvise() syscall: an external memory hinting API

There is usecase that System Management Software(SMS) want to give a
memory hint like MADV_[COLD|PAGEEOUT] to other processes and in the
case of Android, it is the ActivityManagerService.

The information required to make the reclaim decision is not known to the
app.  Instead, it is known to the centralized userspace
daemon(ActivityManagerService), and that daemon must be able to initiate
reclaim on its own without any app involvement.

To solve the issue, this patch introduces a new syscall
process_madvise(2).  It uses pidfd of an external process to give the
hint.  It also supports vector address range because Android app has
thousands of vmas due to zygote so it's totally waste of CPU and power if
we should call the syscall one by one for each vma.(With testing 2000-vma
syscall vs 1-vector syscall, it showed 15% performance improvement.  I
think it would be bigger in real practice because the testing ran very
cache friendly environment).

Another potential use case for the vector range is to amortize the cost
ofTLB shootdowns for multiple ranges when using MADV_DONTNEED; this could
benefit users like TCP receive zerocopy and malloc implementations.  In
future, we could find more usecases for other advises so let's make it
happens as API since we introduce a new syscall at this moment.  With
that, existing madvise(2) user could replace it with process_madvise(2)
with their own pid if they want to have batch address ranges support
feature.

ince it could affect other process's address range, only privileged
process(PTRACE_MODE_ATTACH_FSCREDS) or something else(e.g., being the same
UID) gives it the right to ptrace the process could use it successfully.
The flag argument is reserved for future use if we need to extend the API.

I think supporting all hints madvise has/will supported/support to
process_madvise is rather risky.  Because we are not sure all hints make
sense from external process and implementation for the hint may rely on
the caller being in the current context so it could be error-prone.  Thus,
I just limited hints as MADV_[COLD|PAGEOUT] in this patch.

If someone want to add other hints, we could hear the usecase and review
it for each hint.  It's safer for maintenance rather than introducing a
buggy syscall but hard to fix it later.

So finally, the API is as follows,

      ssize_t process_madvise(int pidfd, const struct iovec *iovec,
                unsigned long vlen, int advice, unsigned int flags);

    DESCRIPTION
      The process_madvise() system call is used to give advice or directions
      to the kernel about the address ranges from external process as well as
      local process. It provides the advice to address ranges of process
      described by iovec and vlen. The goal of such advice is to improve
      system or application performance.

      The pidfd selects the process referred to by the PID file descriptor
      specified in pidfd. (See pidofd_open(2) for further information)

      The pointer iovec points to an array of iovec structures, defined in
      <sys/uio.h> as:

        struct iovec {
            void *iov_base;         /* starting address */
            size_t iov_len;         /* number of bytes to be advised */
        };

      The iovec describes address ranges beginning at address(iov_base)
      and with size length of bytes(iov_len).

      The vlen represents the number of elements in iovec.

      The advice is indicated in the advice argument, which is one of the
      following at this moment if the target process specified by pidfd is
      external.

        MADV_COLD
        MADV_PAGEOUT

      Permission to provide a hint to external process is governed by a
      ptrace access mode PTRACE_MODE_ATTACH_FSCREDS check; see ptrace(2).

      The process_madvise supports every advice madvise(2) has if target
      process is in same thread group with calling process so user could
      use process_madvise(2) to extend existing madvise(2) to support
      vector address ranges.

    RETURN VALUE
      On success, process_madvise() returns the number of bytes advised.
      This return value may be less than the total number of requested
      bytes, if an error occurred. The caller should check return value
      to determine whether a partial advice occurred.

FAQ:

Q.1 - Why does any external entity have better knowledge?

Quote from Sandeep

"For Android, every application (including the special SystemServer)
are forked from Zygote.  The reason of course is to share as many
libraries and classes between the two as possible to benefit from the
preloading during boot.

After applications start, (almost) all of the APIs end up calling into
this SystemServer process over IPC (binder) and back to the
application.

In a fully running system, the SystemServer monitors every single
process periodically to calculate their PSS / RSS and also decides
which process is "important" to the user for interactivity.

So, because of how these processes start _and_ the fact that the
SystemServer is looping to monitor each process, it does tend to *know*
which address range of the application is not used / useful.

Besides, we can never rely on applications to clean things up
themselves.  We've had the "hey app1, the system is low on memory,
please trim your memory usage down" notifications for a long time[1].
They rely on applications honoring the broadcasts and very few do.

So, if we want to avoid the inevitable killing of the application and
restarting it, some way to be able to tell the OS about unimportant
memory in these applications will be useful.

- ssp

Q.2 - How to guarantee the race(i.e., object validation) between when
giving a hint from an external process and get the hint from the target
process?

process_madvise operates on the target process's address space as it
exists at the instant that process_madvise is called.  If the space
target process can run between the time the process_madvise process
inspects the target process address space and the time that
process_madvise is actually called, process_madvise may operate on
memory regions that the calling process does not expect.  It's the
responsibility of the process calling process_madvise to close this
race condition.  For example, the calling process can suspend the
target process with ptrace, SIGSTOP, or the freezer cgroup so that it
doesn't have an opportunity to change its own address space before
process_madvise is called.  Another option is to operate on memory
regions that the caller knows a priori will be unchanged in the target
process.  Yet another option is to accept the race for certain
process_madvise calls after reasoning that mistargeting will do no
harm.  The suggested API itself does not provide synchronization.  It
also apply other APIs like move_pages, process_vm_write.

The race isn't really a problem though.  Why is it so wrong to require
that callers do their own synchronization in some manner?  Nobody
objects to write(2) merely because it's possible for two processes to
open the same file and clobber each other's writes --- instead, we tell
people to use flock or something.  Think about mmap.  It never
guarantees newly allocated address space is still valid when the user
tries to access it because other threads could unmap the memory right
before.  That's where we need synchronization by using other API or
design from userside.  It shouldn't be part of API itself.  If someone
needs more fine-grained synchronization rather than process level,
there were two ideas suggested - cookie[2] and anon-fd[3].  Both are
applicable via using last reserved argument of the API but I don't
think it's necessary right now since we have already ways to prevent
the race so don't want to add additional complexity with more
fine-grained optimization model.

To make the API extend, it reserved an unsigned long as last argument
so we could support it in future if someone really needs it.

Q.3 - Why doesn't ptrace work?

Injecting an madvise in the target process using ptrace would not work
for us because such injected madvise would have to be executed by the
target process, which means that process would have to be runnable and
that creates the risk of the abovementioned race and hinting a wrong
VMA.  Furthermore, we want to act the hint in caller's context, not the
callee's, because the callee is usually limited in cpuset/cgroups or
even freezed state so they can't act by themselves quick enough, which
causes more thrashing/kill.  It doesn't work if the target process are
ptraced(e.g., strace, debugger, minidump) because a process can have at
most one ptracer.

[1] https://developer.android.com/topic/performance/memory"

[2] process_getinfo for getting the cookie which is updated whenever
    vma of process address layout are changed - Daniel Colascione -
    https://lore.kernel.org/lkml/20190520035254.57579-1-minchan@kernel.org/T/#m7694416fd179b2066a2c62b5b139b14e3894e224

[3] anonymous fd which is used for the object(i.e., address range)
    validation - Michal Hocko -
    https://lore.kernel.org/lkml/20200120112722.GY18451@dhcp22.suse.cz/

[minchan@kernel.org: fix process_madvise build break for arm64]
  Link: http://lkml.kernel.org/r/20200303145756.GA219683@google.com
[minchan@kernel.org: fix build error for mips of process_madvise]
  Link: http://lkml.kernel.org/r/20200508052517.GA197378@google.com
[akpm@linux-foundation.org: fix patch ordering issue]
[akpm@linux-foundation.org: fix arm64 whoops]
[minchan@kernel.org: make process_madvise() vlen arg have type size_t, per Florian]
[akpm@linux-foundation.org: fix i386 build]
[sfr@canb.auug.org.au: fix syscall numbering]
  Link: https://lkml.kernel.org/r/20200905142639.49fc3f1a@canb.auug.org.au
[sfr@canb.auug.org.au: madvise.c needs compat.h]
  Link: https://lkml.kernel.org/r/20200908204547.285646b4@canb.auug.org.au
[minchan@kernel.org: fix mips build]
  Link: https://lkml.kernel.org/r/20200909173655.GC2435453@google.com
[yuehaibing@huawei.com: remove duplicate header which is included twice]
  Link: https://lkml.kernel.org/r/20200915121550.30584-1-yuehaibing@huawei.com
[minchan@kernel.org: do not use helper functions for process_madvise]
  Link: https://lkml.kernel.org/r/20200921175539.GB387368@google.com
[akpm@linux-foundation.org: pidfd_get_pid() gained an argument]
[sfr@canb.auug.org.au: fix up for "iov_iter: transparently handle compat iovecs in import_iovec"]
  Link: https://lkml.kernel.org/r/20200928212542.468e1fef@canb.auug.org.au

Signed-off-by: Minchan Kim <minchan@kernel.org>
Signed-off-by: YueHaibing <yuehaibing@huawei.com>
Signed-off-by: Stephen Rothwell <sfr@canb.auug.org.au>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Reviewed-by: Suren Baghdasaryan <surenb@google.com>
Reviewed-by: Vlastimil Babka <vbabka@suse.cz>
Acked-by: David Rientjes <rientjes@google.com>
Cc: Alexander Duyck <alexander.h.duyck@linux.intel.com>
Cc: Brian Geffon <bgeffon@google.com>
Cc: Christian Brauner <christian@brauner.io>
Cc: Daniel Colascione <dancol@google.com>
Cc: Jann Horn <jannh@google.com>
Cc: Jens Axboe <axboe@kernel.dk>
Cc: Joel Fernandes <joel@joelfernandes.org>
Cc: Johannes Weiner <hannes@cmpxchg.org>
Cc: John Dias <joaodias@google.com>
Cc: Kirill Tkhai <ktkhai@virtuozzo.com>
Cc: Michal Hocko <mhocko@suse.com>
Cc: Oleksandr Natalenko <oleksandr@redhat.com>
Cc: Sandeep Patil <sspatil@google.com>
Cc: SeongJae Park <sj38.park@gmail.com>
Cc: SeongJae Park <sjpark@amazon.de>
Cc: Shakeel Butt <shakeelb@google.com>
Cc: Sonny Rao <sonnyrao@google.com>
Cc: Tim Murray <timmurray@google.com>
Cc: Christian Brauner <christian.brauner@ubuntu.com>
Cc: Florian Weimer <fw@deneb.enyo.de>
Cc: <linux-man@vger.kernel.org>
Link: http://lkml.kernel.org/r/20200302193630.68771-3-minchan@kernel.org
Link: http://lkml.kernel.org/r/20200508183320.GA125527@google.com
Link: http://lkml.kernel.org/r/20200622192900.22757-4-minchan@kernel.org
Link: https://lkml.kernel.org/r/20200901000633.1920247-4-minchan@kernel.org
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

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
