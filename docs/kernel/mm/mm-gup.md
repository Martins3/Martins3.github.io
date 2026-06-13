# Get User Pages

## TODO
- [ ] 为什么 hugepage 会让问题更加复杂?
- fault_in_writable
- [ ] copy from user 到底是如何进行的，必然需要 pin 吗?

## 官方文档需要看看: Documentation/core-api/pin_user_pages.rst

 pin_user_pages()
 pin_user_pages_fast()
 pin_user_pages_remote()

三个接口有啥区别?

The FOLL_PIN implementation is nearly the same as FOLL_GET, except that FOLL_PIN
uses a different reference counting technique.



## follow_page
- 根据虚拟地址找到物理地址，例如 move_pages(2) 中查找每一个 page 中所在的 Node


- follow_p4d_mask
- follow_page_mask

为什么，这个有时候是从 p4d，有时候直接从 pmd 开始的
```txt
- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - __x64_sys_execve
        - __se_sys_execve
          - __do_sys_execve
            - do_execve
              - do_execveat_common
                - copy_string_kernel
                  - get_arg_page
                    - get_user_pages_remote
                      - __get_user_pages_remote
                        - __get_user_pages_locked
                          - __get_user_pages
                            - follow_page_mask
                              - follow_p4d_mask
```

```txt
- ??
  - entry_SYSCALL_64
    - do_syscall_64
      - do_syscall_x64
        - __x64_sys_execve
          - __se_sys_execve
            - __do_sys_execve
              - do_execve
                - do_execveat_common
                  - copy_string_kernel
                    - get_arg_page
                      - get_user_pages_remote
                        - __get_user_pages_remote
                          - __get_user_pages_locked
                            - __get_user_pages
                              - follow_pmd_mask
                                - follow_page_pte
```

## gup 的接口

类似这种函数:
- gup_*_huge
- gup_*_range

- get_user_pages_fast_only : 用户主要是 kvm
- pin_user_pages_fast : svm
- get_user_pages_fast : futex mempolicy
  - internal_get_user_pages_fast

一个经典调用路径:
- kvm_faultin_pfn
  - `__gfn_to_pfn_memslot`
    - hva_to_pfn
      - hva_to_pfn_fast
        - get_user_page_fast_only
      - hva_to_pfn_slow

## 如何保证被 pin 的页面是不可以换出
- 在 try_grab_page 中，如果 flags 中存在 FOLL_PIN ，那么增加 page 的 refcount，进而让该 page 无法进入到 lruvec 中。

## mlock 是不是也是使用了这种方法

## 使用案例

### /proc/$pid/mem 的实现
- https://offlinemark.com/2021/05/12/an-obscure-quirk-of-proc/

通过 gup 和 cow ，如何实现修改没有权限的页面

- access_remote_vm 是 /proc/$pid/cmdline,mem,environ 的实现基础

- access_remote_vm
  - get_user_pages_remote() : 初始化 flags = FOLL_FORCE | (write ? FOLL_WRITE : 0);

### [kernel](https://stackoverflow.com/questions/36337942/how-does-get-user-pages-work-for-linux-driver)

## 和 cow 的关系

- [ ] comments in `@VM_FAULT_WRITE:     Special case for get_user_pages`
  - [ ] check functin `do_wp_page`

- rebuild it http://martins3.gitee.io/dirtycow and add it to Notes.wiki

## https://docs.kernel.org/core-api/pin_user_pages.html
内核文档[^8]主要分析下面三个接口:
```c
pin_user_pages()
pin_user_pages_fast()
pin_user_pages_remote()
```
1. In other word use `pin_user_pages*()` for DMA-pinned pages, and `get_user_pages*()` for other cases.
2. The `FOLL_PIN` implementation is nearly the same as `FOLL_GET`, except that FOLL_PIN uses a different reference counting technique.
3. `FOLL_PIN` is a prerequisite to `FOLL_LONGTERM`. Another way of saying that is, `FOLL_LONGTERM` is a specific case, more restrictive case of `FOLL_PIN`.
。。。// TO BE CONTINUE


## https://lwn.net/Articles/807108/
lwn 的文章[^9]说明的内容:
To simplify the situation somewhat, the problems with get_user_pages() come about in two ways. One of those happens when the kernel thinks that the contents of a page will not change, but some peripheral device writes new data there. The other arises with memory that is located on **persistent-memory devices** managed by a filesystem; pinning pages into memory deprives the filesystem of the ability to make layout changes involving those pages. The latter problem has been "solved" for now by disallowing long-lasting page pins on persistent-memory devices, but there are use cases calling for creating just that kind of pin, so better solutions are being sought.
get_user_pages 的问题来源于两个部分 :
1. 内核以为 pin 的 page 没有发生修改，但是实际上外设偷偷的对于该内存修改过。(难道不是用户对于这种未经提示的修改感到奇怪吗 ? 类似的事情不是也会发生在 任何将内存映射到设备上的情况吗? )
2. persistent memory device : emmmm 这似乎是 DAX 相关的

Part of the problem comes down to the fact that get_user_pages() does not perform any sort of special tracking of the pages it pins into RAM. It does increment the reference count for each page, preventing it from being evicted from memory, but pages that have been pinned in this way are indistinguishable from pages that have acquired references in any of a vast number of other ways. So, while one can ask whether a page has references, it is not possible for kernel code to ask whether a page has been pinned for purposes like DMA I/O.
1. 虽然通过 reference 可以防止 page 被 evicted (evicted 指的是回收吗 ? 但是这个 page 是用户通过 brk 分配的，如果用户进程 exit 了，内核如何知道这个 page 如何回收啊 !)
2. 而且为什么需要单独区分这个东西啊！
。。。// TO BE CONTINUE 写不错，但是没有耐心了


猜测其中的过程 :
1. page 全部都是用户分配的，page 对应的内核虚拟地址自然确定了。
2. **问题是** 内核映射整个物理地址空间，还是仅仅映射了物理内存的部分。
    3. 猜测仅仅是内核的物理内存部分，不然 gup.c 应该会很简单
    2. 而且浪费了很多内存
    3. **那么** 内核的物理内存的边缘在何处 ?

小问题:
  #if defined(CONFIG_FS_DAX) || defined (CONFIG_CMA) ，DAX 和 CMA 为什么可以影响


- [ ] 猜测 gup.c 的主要问题是处理各种 lock
  - [ ] 处理 hugepage 的位置
  - [x] 实际上做事的位置在于 `_get_user_pages`
  - [ ] 部分位置处理了 mlock 的依赖 : populate_vma_page_range 等
  - [ ] 处理 vsyscall 机制
  - [ ] 很多是处理 follow_page_mask 之类的代码

`_get_user_pages` 分析:
1. 确定 vma
2. follow_page_mask : 依次进行 page walk，直到找到该 page 或者失败
    - 如果失败，faultin_page
    - 将获取的 page 放到参数 : pages 中间

- [x] 并没有找到 将 user page pin 的代码位置，*猜测*防止内核将 page 换出，只是需要将 ref + 1 即可
  - [x] 猜测是 FOLL_PIN 导致的，在 follow_page_pte 中间使用 get_page
  - [ ] 总结一下，到底那些 memory 的 refcount 的作用
  - [ ] get_page 会一定导致 user page 不可以 swap out 吗 ?


## 从 gup 到 dirty cow
- [ ] 把这里的玩意儿整理出来 : http://martins3.gitee.io/dirtycow/#

In particular, each page frame corresponding to a private, writable page (VM_SHARED flag off
and VM_MAYWRITE flag on) is marked as read-only for both the parent and the child, so
that it will be handled with the Copy On Write mechanism.

https://stackoverflow.com/questions/48241187/memory-region-flags-in-linux-why-both-vm-write-and-vm-maywrite-are-needed

/proc/self/mem 向其中写的效果是什么?

- [ ] 是在哪一个位置进行检查权限的，所以就只能 cow 了
  - [ ] 从哪里是告诉 gup 是使用 force 的

从理解上，mmap 一个文件，然后对于一个文件写，必然产生一个原始的 page cache 页面，但是，由于没有权限，所以只能产生一个 cow page 出来，在 cow page 上写，
如果这个 cow page 被 刷掉了，在重新的进行 fault 的时候，没有添加上
但是此时访问请求标记没有了 FOLL_WRITE，所以会认为是一个读访问，不会触发 COW，这次缺页处理会填充 pte 对应原始物理页，再次调用 follow_page 成功获取原始页，所以正常情况会在 cowed page 上进行读写操作，

- [ ] 问题的关键在于，在 `__get_user_pages` 中，在后续的访问中去掉 FOLL_WRITE
  - [x] 为什么 `__get_user_pages` 是一个 while 循环，因为可能需要 gup 很多 page
  - [x] faultin_page 为什么会返回 0 : 当什么错误都没有了的时候，那么就调用 follow_page_mask 重新检查一次, 但是中间由于调用 madvise 很有可能将其删除了
  - 当 handle_mm_fault 的返回值中间有 VM_FAULT_WRITE, 那么说明 handle_mm_fault 进行了一次 cow，当前建立的映射是指向 cow page 的
    - 假如，faultin_page 不去掉 VM_WRITE 的标识符，继续使用之前的 foll_flags 调用 follow_page_mask, follow_page_mask 就会失败，因为这种属性的页面不存在，那么就只好再次 fault

- 这里可以绕过权限访问的根本原因是 : 同一个物理页面，可以从内核访问，也是可以从用户态访问，虽然给用户态的 pte 总是 READ ONLY 的，但是内核态的并不是，所以不可以让用户通过 /proc/self/mem 间接的写入到页面上。

- follow_page_pte 的部分代码如下:
```c
    if ((flags & FOLL_WRITE) && !can_follow_write_pte(pte, flags)) {
        pte_unmap_unlock(ptep, ptl);
        return NULL;
    }
```
因为 maybe_mkwrite 的使用，即使是 cow 出来的一个 page, 也是 read only 的。只有当 vma 上的 flag 决定了 pte 的 flag。
- [x] 如果 pte 上没有 write flags，之后怎么可以写啊!
  - [x] gup 是为了获取一个 struct page，而不是这个 vma 上的 pte
  - **注意** : 当 foll_flags 没有 FOLL_WRITE 的时候，因为是只读，会将 page cache 的页面直接提供出来, 之后写也是直接写入到这个页面上了
  - 但是 FOLL_WRITE 的时候，但是发现这个页面不可写，就只能再次创建一个出来

- [ ] 所以，FOLL_WRITE 创建出来了两个页面 ? 可能也是直接绕过的吧!

```c
/*
 * FOLL_FORCE can write to even unwritable pte's, but only
 * after we've gone through a COW cycle and they are dirty.
 */
static inline bool can_follow_write_pte(pte_t pte, unsigned int flags)
{
    return pte_write(pte) ||
        ((flags & FOLL_FORCE) && (flags & FOLL_COW) && pte_dirty(pte));
}
```

The reason for removing the FOLL_WRITE flag is to take into account the case the FOLL_FORCE flag is applied on a read-only VMA (when the VM_MAYWRITE flag is set in the VMA).
In that case, the `pte_maybe_mkwrite` function won’t set the write bit, however the faulted-in page is indeed ready for writing.

- [ ]  maybe_mkwrite 的作用 : 设置 pte 的 write flag 位置, 但是 gup 有所不同

```txt
       MADV_DONTNEED
              Do not expect access in the near future.  (For the time being, the application is finished with the given range, so the kernel can free resources associated with it.)

              After a successful MADV_DONTNEED operation, the semantics of memory access in the specified region are changed: subsequent accesses of pages in the range will succeed, but will result in either repopulating the memory contents from the up-to-date contents of the
              underlying mapped file (for shared file mappings, shared anonymous mappings, and shmem-based techniques such as System V shared memory segments) or zero-fill-on-demand pages for anonymous private mappings.

              Note that, when applied to shared mappings, MADV_DONTNEED might not lead to immediate freeing of the pages in the range.  The kernel is free to delay freeing the pages until an appropriate moment.  The resident set size (RSS) of the calling process will be imme‐
              diately reduced however.

              MADV_DONTNEED cannot be applied to locked pages, Huge TLB pages, or VM_PFNMAP pages.  (Pages marked with the kernel-internal VM_PFNMAP flag are special memory areas that are not managed by the virtual memory subsystem.  Such pages are typically created by device
              drivers that map the pages into user space.)
```

```c
/*
 * Do pte_mkwrite, but only if the vma says VM_WRITE.  We do this when
 * servicing faults for write access.  In the normal case, do always want
 * pte_mkwrite.  But get_user_pages can cause write faults for mappings
 * that do not have writing enabled, when used by access_process_vm.
 */
static inline pte_t maybe_mkwrite(pte_t pte, struct vm_area_struct *vma)
{
    if (likely(vma->vm_flags & VM_WRITE))
        pte = pte_mkwrite(pte);
    return pte;
}
```

- madvise_dontneed_free : MADV_DONTNEED 似乎将一个区间的 page table 去掉
  - madvise_dontneed_single_vma
    - zap_page_range

- [ ] FOLL_FORCE 的含义:
  - get_arg_page : We are doing an exec().  'current' is the process doing the exec and `bprm->mm` is the new process's mm.
  - ptrace.c 中的 : ptrace_readdata 和 ptrace_writedata 的内容，为了获取被 tracee 的 process 的内容，必须添加上 FOLL_FORCE, 而且如果想要写，那么就需要 FOLL_WRITE
    - 原因很简单，当 ptrace 想要在 tracee 的代码段中间设置断点，就是在 read only 的区间修改

分析 check_vma_flags :
1. 如果有 write 的需求，想要获取的 vm 没有权限，除非拥有 FOLL_FORCE，否则失败
2. 即使是 force 的，比必然让这个 page 是可以 cow 才可以(这个文件不可以被映射为 SHARED, 可以使用 [^3] 作为测试)
也就是说，force 的是存在前提的，创建一个 cow page, 提供一个修改了文件的假象，毕竟，修改了 tracee 的二进制，然后忘记修改回来了，是非常糟糕的
- [ ] check_vma_flags 调用 is_cow_mapping 检测 VM_MAYWRITE 的 flag 为什么可以成功啊, 这个 flag 是什么设置的

## 想不到 gup 在这个时候可以被调用
```txt
- ??
  - entry_SYSCALL_64
    - do_syscall_64
      - do_syscall_x64
        - __x64_sys_execve
          - __se_sys_execve
            - __do_sys_execve
              - do_execve
                - do_execveat_common
                  - copy_string_kernel
                    - get_arg_page
                      - get_user_pages_remote
                        - __get_user_pages_remote
                          - __get_user_pages_locked
                            - __get_user_pages
                              - follow_pmd_mask
                                - follow_page_pte
                                  - mark_page_accessed
```

[^1]: https://v1ckydxp.github.io/2020/04/22/2020-04-22-CVE-2016-5195%20%E6%BC%8F%E6%B4%9E%E5%88%86%E6%9E%90/
[^2]: https://medium.com/bindecy/huge-dirty-cow-cve-2017-1000405-110eca132de0
[^3]: https://github.com/dirtycow/dirtycow.github.io/blob/master/dirtyc0w.c

## 从 kvm 到 zero page

- kvm_pfn_to_refcounted_page

```diff
commit b14b2690c50e02145bb867dfcde8845eb17aa8a4
Author: Sean Christopherson <seanjc@google.com>
Date:   Fri Apr 29 01:04:15 2022 +0000

    KVM: Rename/refactor kvm_is_reserved_pfn() to kvm_pfn_to_refcounted_page()

    Rename and refactor kvm_is_reserved_pfn() to kvm_pfn_to_refcounted_page()
    to better reflect what KVM is actually checking, and to eliminate extra
    pfn_to_page() lookups.  The kvm_release_pfn_*() an kvm_try_get_pfn()
    helpers in particular benefit from "refouncted" nomenclature, as it's not
    all that obvious why KVM needs to get/put refcounts for some PG_reserved
    pages (ZERO_PAGE and ZONE_DEVICE).

    Add a comment to call out that the list of exceptions to PG_reserved is
    all but guaranteed to be incomplete.  The list has mostly been compiled
    by people throwing noodles at KVM and finding out they stick a little too
    well, e.g. the ZERO_PAGE's refcount overflowed and ZONE_DEVICE pages
    didn't get freed.

    No functional change intended.

    Signed-off-by: Sean Christopherson <seanjc@google.com>
    Message-Id: <20220429010416.2788472-10-seanjc@google.com>
    Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

## 如何理解 PageReserved 的含义 ?
在函数 `kvm_pfn_to_refcounted_page` 中为什么需要处理这个
```c
	if (!PageReserved(page))
		return page;
```

## 如何理解这个

```c
	/* FOLL_GET and FOLL_PIN are mutually exclusive. */
	if (WARN_ON_ONCE((flags & (FOLL_PIN | FOLL_GET)) ==
			 (FOLL_PIN | FOLL_GET)))
```

进一步在 `__get_user_pages_locked` 中:
```c
	/*
	 * FOLL_PIN and FOLL_GET are mutually exclusive. Traditional behavior
	 * is to set FOLL_GET if the caller wants pages[] filled in (but has
	 * carelessly failed to specify FOLL_GET), so keep doing that, but only
	 * for FOLL_GET, not for the newer FOLL_PIN.
	 *
	 * FOLL_PIN always expects pages to be non-null, but no need to assert
	 * that here, as any failures will be obvious enough.
	 */
	if (pages && !(flags & FOLL_PIN))
		flags |= FOLL_GET;
```

## 如何理解这个

如何理解 follow_page_pte 中的:
```c
	/* try_grab_page() does nothing unless FOLL_GET or FOLL_PIN is set. */
	ret = try_grab_page(page, flags);
	if (unlikely(ret)) {
		page = ERR_PTR(ret);
		goto out;
	}

```

```c
/**
 * try_grab_folio() - Attempt to get or pin a folio.
 * @page:  pointer to page to be grabbed
 * @refs:  the value to (effectively) add to the folio's refcount
 * @flags: gup flags: these are the FOLL_* flag values.
 *
 * "grab" names in this file mean, "look at flags to decide whether to use
 * FOLL_PIN or FOLL_GET behavior, when incrementing the folio's refcount.
 *
 * Either FOLL_PIN or FOLL_GET (or neither) must be set, but not both at the
 * same time. (That's true throughout the get_user_pages*() and
 * pin_user_pages*() APIs.) Cases:
 *
 *    FOLL_GET: folio's refcount will be incremented by @refs.
 *
 *    FOLL_PIN on large folios: folio's refcount will be incremented by
 *    @refs, and its pincount will be incremented by @refs.
 *
 *    FOLL_PIN on single-page folios: folio's refcount will be incremented by
 *    @refs * GUP_PIN_COUNTING_BIAS.
 *
 * Return: The folio containing @page (with refcount appropriately
 * incremented) for success, or NULL upon failure. If neither FOLL_GET
 * nor FOLL_PIN was set, that's considered failure, and furthermore,
 * a likely bug in the caller, so a warning is also emitted.
 */
struct folio *try_grab_folio(struct page *page, int refs, unsigned int flags)
```

理解下 `FOLL_PIN`

居然这是 internal 的
```c
enum {
	/* mark page accessed */
	FOLL_TOUCH = 1 << 16,
	/* a retry, previous pass started an IO */
	FOLL_TRIED = 1 << 17,
	/* we are working on non-current tsk/mm */
	FOLL_REMOTE = 1 << 18,
	/* pages must be released via unpin_user_page */
	FOLL_PIN = 1 << 19,
	/* gup_fast: prevent fall-back to slow gup */
	FOLL_FAST_ONLY = 1 << 20,
	/* allow unlocking the mmap lock */
	FOLL_UNLOCKABLE = 1 << 21,
};
```
FOLL_PIN 都是内存 mk 的内部用户


```c
/*
 * FOLL_PIN and FOLL_LONGTERM may be used in various combinations with each
 * other. Here is what they mean, and how to use them:
 *
 *
 * FIXME: For pages which are part of a filesystem, mappings are subject to the
 * lifetime enforced by the filesystem and we need guarantees that longterm
 * users like RDMA and V4L2 only establish mappings which coordinate usage with
 * the filesystem.  Ideas for this coordination include revoking the longterm
 * pin, delaying writeback, bounce buffer page writeback, etc.  As FS DAX was
 * added after the problem with filesystems was found FS DAX VMAs are
 * specifically failed.  Filesystem pages are still subject to bugs and use of
 * FOLL_LONGTERM should be avoided on those pages.
 *
 * In the CMA case: long term pins in a CMA region would unnecessarily fragment
 * that region.  And so, CMA attempts to migrate the page before pinning, when
 * FOLL_LONGTERM is specified.
 *
 * FOLL_PIN indicates that a special kind of tracking (not just page->_refcount,
 * but an additional pin counting system) will be invoked. This is intended for
 * anything that gets a page reference and then touches page data (for example,
 * Direct IO). This lets the filesystem know that some non-file-system entity is
 * potentially changing the pages' data. In contrast to FOLL_GET (whose pages
 * are released via put_page()), FOLL_PIN pages must be released, ultimately, by
 * a call to unpin_user_page().
 *
 * FOLL_PIN is similar to FOLL_GET: both of these pin pages. They use different
 * and separate refcounting mechanisms, however, and that means that each has
 * its own acquire and release mechanisms:
 *
 *     FOLL_GET: get_user_pages*() to acquire, and put_page() to release.
 *
 *     FOLL_PIN: pin_user_pages*() to acquire, and unpin_user_pages to release.
 *
 * FOLL_PIN and FOLL_GET are mutually exclusive for a given function call.
 * (The underlying pages may experience both FOLL_GET-based and FOLL_PIN-based
 * calls applied to them, and that's perfectly OK. This is a constraint on the
 * callers, not on the pages.)
 *
 * FOLL_PIN should be set internally by the pin_user_pages*() APIs, never
 * directly by the caller. That's in order to help avoid mismatches when
 * releasing pages: get_user_pages*() pages must be released via put_page(),
 * while pin_user_pages*() pages must be released via unpin_user_page().
 *
 * Please see Documentation/core-api/pin_user_pages.rst for more information.
 */

enum {
	/* check pte is writable */
	FOLL_WRITE = 1 << 0,
	/* do get_page on page */
	FOLL_GET = 1 << 1,
	/* give error on hole if it would be zero */
	FOLL_DUMP = 1 << 2,
	/* get_user_pages read/write w/o permission */
	FOLL_FORCE = 1 << 3,
	/*
	 * if a disk transfer is needed, start the IO and return without waiting
	 * upon it
	 */
	FOLL_NOWAIT = 1 << 4,
	/* do not fault in pages */
	FOLL_NOFAULT = 1 << 5,
	/* check page is hwpoisoned */
	FOLL_HWPOISON = 1 << 6,
	/* don't do file mappings */
	FOLL_ANON = 1 << 7,
	/*
	 * FOLL_LONGTERM indicates that the page will be held for an indefinite
	 * time period _often_ under userspace control.  This is in contrast to
	 * iov_iter_get_pages(), whose usages are transient.
	 */
	FOLL_LONGTERM = 1 << 8,
	/* split huge pmd before returning */
	FOLL_SPLIT_PMD = 1 << 9,
	/* allow returning PCI P2PDMA pages */
	FOLL_PCI_P2PDMA = 1 << 10,
	/* allow interrupts from generic signals */
	FOLL_INTERRUPTIBLE = 1 << 11,

	/* See also internal only FOLL flags in mm/internal.h */
};
```

## is_valid_gup_args() FOLL flags 的进一步说明

但是如何理解这个?
```c
	/* Pages input must be given if using GET/PIN */
	if (WARN_ON_ONCE((gup_flags & (FOLL_GET | FOLL_PIN)) && !pages))
		return false;
```

## [The ongoing trouble with get_user_pages()](https://lwn.net/Articles/930667/)

[LWN：get_user_pages() 仍在带来麻烦！](https://mp.weixin.qq.com/s/FD7XRvyAyUERaad-DBR0TA)

看完更加迷茫了。

## 如果调用 gup 增加了一个 page 的 reference ，这个 page 需要从 lru 中取出来吗?

## 一个 security bug 让复杂变得更加复杂
9471f1f2f50282b9e8f59198ec6bb738b4ccc009

```txt
[ 8371.232795] ------------[ cut here ]------------
[ 8371.232796] WARNING: CPU: 4 PID: 6509 at mm/gup.c:1101 __get_user_pages+0x577/0x670
```

## 看看这个
c8070b78751955e59b42457b974bea4a4fe00187

## 分析下这个，太强了
https://github.com/lrh2000/StackRot

## 这个例子太好了

1. 在 gup 的过程中，通过 page fault 是 `__get_user_pages` 直接调用的
handle_mm_fault 的，其实是相当于软件模拟硬件的 page fault 的。

2. ext4 也使用的是 iomap

```txt
@[
    kvm_flush_tlb_multi+5
    flush_tlb_mm_range+287
    ptep_clear_flush+65
    do_wp_page+3201
    handle_mm_fault+2921
    __get_user_pages+1130
    __gup_longterm_locked+1333
    gup_fast_fallback+4269
    iov_iter_extract_pages+296
    bio_iov_iter_get_pages+282
    iomap_dio_bio_iter+748
    __iomap_dio_rw+639
    iomap_dio_rw+18
    ext4_file_read_iter+242
    aio_read+456
    io_submit_one+1608
    __se_sys_io_submit+189
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 41100
```

## 为什么 hva_to_pfn_slow 中获取的 page 的 refcount 很多都是 3 啊
使用如下的 diff :
```diff
@@ -2821,6 +2821,8 @@ static int hva_to_pfn_slow(unsigned long addr, bool *async, bool write_fault,
 	if (npages != 1)
 		return npages;

+	trace_hi(page_ref_count(page));
+
 	/* map read fault as writable if possible */
 	if (unlikely(!write_fault) && writable) {
 		struct page *wpage;
```

但是可以看到如下的结果
```txt
80.80%  hi : 3
19.20%  hi : 2
```

但是至少，我们可以知道，kvm 映射的 page 之所以可以释放，是因为调用了:
```txt
@[
    kvm_release_pfn_clean+5
    kvm_tdp_page_fault+213
    kvm_mmu_do_page_fault+482
    kvm_mmu_page_fault+137
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+2832
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+145
    do_syscall_64+188
    entry_SYSCALL_64_after_hwframe+119
]: 247319
```

仔细检查 hva_to_pfn_remapped 中的注释:

```c
	/*
	 * Get a reference here because callers of *hva_to_pfn* and
	 * *gfn_to_pfn* ultimately call kvm_release_pfn_clean on the
	 * returned pfn.  This is only needed if the VMA has VM_MIXEDMAP
	 * set, but the kvm_try_get_pfn/kvm_release_pfn_clean pair will
	 * simply do nothing for reserved pfns.
	 *
	 * Whoever called remap_pfn_range is also going to call e.g.
	 * unmap_mapping_range before the underlying pages are freed,
	 * causing a call to our MMU notifier.
	 *
	 * Certain IO or PFNMAP mappings can be backed with valid
	 * struct pages, but be allocated without refcounting e.g.,
	 * tail pages of non-compound higher order allocations, which
	 * would then underflow the refcount when the caller does the
	 * required put_page. Don't allow those pages here.
	 */
```


## 问题是 pin_user_pages_remote 和 pin_user_pages_fast 的区别是什么?
<!-- 57d04013-a3ac-456a-b019-aa58c63f4927 -->

需要继续确认一下。

`pin_user_pages_remote()` 与 `pin_user_pages_fast()` 都是 Linux 内核中用于 **长期 pin 住用户态页（page pinning）** 的接口，典型使用场景包括 RDMA、VFIO、vDPA、GPU 直通等需要防止用户页被回收或迁移的子系统。二者的核心区别在于 **适用的地址空间、实现路径以及可阻塞性/性能特征**。

下面从语义、实现、使用场景三个维度进行系统说明。

## 一、基本语义对比

| 维度             | pin_user_pages_fast() | pin_user_pages_remote()      |
| -------------- | --------------------- | ---------------------------- |
| 地址空间           | **当前进程**（current->mm） | **指定进程**（struct mm_struct *） |
| 是否需要 mmap_lock | 不需要（fast path）        | 需要（内部获取）                     |
| 是否可能睡眠         | 通常 **不可**（快速路径）       | **可以**                       |
| 性能             | 高（无 page fault / 无锁）  | 较低（走完整 GUP 路径）               |
| 可处理缺页          | 否                     | 是                            |
| 典型用途           | 高性能 I/O、热路径           | 跨进程、冷路径、管理型操作                |

---

## 二、pin_user_pages_fast()

### 1. 核心特征

* **仅适用于当前进程的用户地址空间**
* 走的是 **GUP fast path**

  * 使用硬件页表（page table walk）
  * 不触发 page fault
  * 不获取 `mmap_lock`
* 如果遇到以下情况会失败：

  * 页面未建立映射
  * 需要 COW
  * HugeTLB / 特殊映射
  * 权限不足

### 2. 典型使用场景

* 数据路径（data path）上的高频调用
* 当前进程直接发起 I/O
* 对延迟和性能高度敏感

例如：

* RDMA send/recv 的 fast path
* 高速网络、存储栈
* io_uring / splice 类路径（在满足条件时）

### 3. 使用示例（简化）

```c
ret = pin_user_pages_fast(uaddr, npages,
                          FOLL_WRITE | FOLL_LONGTERM,
                          pages);
if (ret < npages) {
    /* fallback 或失败处理 */
}
```

> 实践中，很多子系统会在 fast 失败时 **fallback 到慢路径**。

---

## 三、pin_user_pages_remote()

### 1. 核心特征

* **可 pin 任意进程的用户地址空间**

  * 通过显式传入 `struct mm_struct *mm`
* 内部会：

  * 获取 `mmap_lock`
  * 处理缺页
  * 执行完整的 GUP slow path
* 可以睡眠，适用于进程上下文

### 2. 典型使用场景

* 跨进程访问用户内存
* 管理路径（control path）
* 需要“保证成功率”的 pin 操作

例如：

* VFIO / vDPA 为 QEMU pin Guest RAM
* RDMA CM / 管理线程 pin 用户缓冲区
* 驱动在 ioctl 中 pin 另一个进程的内存

### 3. 使用示例（简化）

```c
ret = pin_user_pages_remote(mm, uaddr, npages,
                            FOLL_WRITE | FOLL_LONGTERM,
                            pages, NULL, NULL);
```

---

## 四、为什么需要两套接口？

### 1. 性能 vs. 语义完整性

* `pin_user_pages_fast()`

  * 极致性能
  * 前提条件苛刻
* `pin_user_pages_remote()`

  * 功能完整
  * 支持缺页、COW、跨进程

### 2. 内核设计原则

> **Fast path 不得睡眠，不得加重锁；Slow path 保证语义正确。**

因此，常见设计模式是：

```c
ret = pin_user_pages_fast(...);
if (ret < npages)
    ret = pin_user_pages_remote(...);
```

---

## 五、与 get_user_pages* 的关系（重要）

* 这两个接口都是 **FOLL_PIN 语义**
* 必须使用 `unpin_user_pages()` 释放
* 不可与 `put_page()` / `get_user_pages()` 混用

这是为了解决：

* 页迁移（page migration）
* CMA
* long-term DMA pin 的一致性问题

---

## 六、选择建议（实战总结）

1. **I/O 热路径、当前进程内存**

   * 优先：`pin_user_pages_fast()`
   * 必须考虑 fallback

2. **跨进程 / ioctl / 管理路径**

   * 直接使用：`pin_user_pages_remote()`

3. **长期 DMA pin**

   * 必须带 `FOLL_LONGTERM`
   * 注意与 filesystem DAX / page migration 的冲突

---

## 七、一句话总结

* **`pin_user_pages_fast()`**

  > 当前进程 + 高性能 + 不处理缺页

* **`pin_user_pages_remote()`**

  > 任意进程 + 语义完整 + 可睡眠

如果你愿意，可以结合你当前在看的 **RDMA / VFIO / io_uring / virtio** 代码路径，我可以直接帮你指出为什么内核在那里选择了其中某一个接口。

(直接在内核中找找这个东西的使用，完全搞不懂使用的规律)

- pin_user_pages_fast
```txt
virt/kvm/kvm_main.c
mm/gup_test.c
mm/gup.c
lib/iov_iter.c
arch/x86/kvm/svm/sev.c
net/rds/rdma.c
net/rds/info.c
kernel/bpf/syscall.c
io_uring/memmap.c
tools/testing/selftests/mm/run_vmtests.sh
Documentation/core-api/pin_user_pages.rst
arch/s390/kvm/pci.c
include/linux/mm.h
drivers/xen/privcmd.c
drivers/xen/gntdev.c
drivers/virt/acrn/mm.c
drivers/virt/acrn/ioreq.c
drivers/virt/acrn/hsm.c
drivers/video/fbdev/pvr2fb.c
drivers/iommu/iommufd/iova_bitmap.c
drivers/iommu/iommufd/pages.c
drivers/vhost/vhost.c
drivers/vfio/vfio_iommu_spapr_tce.c
fs/orangefs/orangefs-bufmap.c
drivers/scsi/st.c
drivers/accel/habanalabs/common/memory.c
drivers/accel/amdxdna/amdxdna_ubuf.c
drivers/fpga/dfl-afu-dma-region.c
drivers/infiniband/hw/hfi1/user_pages.c
drivers/hv/mshv_root_main.c
drivers/infiniband/hw/mthca/mthca_memfree.c
drivers/infiniband/core/umem.c
drivers/sbus/char/oradax.c
drivers/staging/vc04_services/interface/vchiq_arm/vchiq_core.c
drivers/gpu/drm/vmwgfx/vmwgfx_msg.c
drivers/misc/xilinx_sdfec.c
drivers/rapidio/devices/rio_mport_cdev.c
drivers/misc/genwqe/card_utils.c
drivers/gpu/drm/exynos/exynos_drm_g2d.c
drivers/gpu/drm/etnaviv/etnaviv_gem.c
drivers/gpu/drm/i915/gem/i915_gem_userptr.c
drivers/platform/goldfish/goldfish_pipe.c
drivers/media/common/videobuf2/frame_vector.c
```


还有一个 get_user_pages_remote() 函数

```txt
🧀  rg -l pin_user_pages_remote
kernel/trace/trace_events_user.c
Documentation/core-api/pin_user_pages.rst
drivers/vfio/vfio_iommu_type1.c
drivers/iommu/iommufd/pages.c
mm/process_vm_access.c
mm/gup.c
include/linux/mm.h
```

## FOLL_LONGTERM 的语义到底是什么?

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
