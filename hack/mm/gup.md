## get user page
[使用 gup 的例子](https://stackoverflow.com/questions/36337942/how-does-get-user-pages-work-for-linux-driver)
1. 为什么 hugepage 会更加麻烦 ?


- [ ] comments in `@VM_FAULT_WRITE:		Special case for get_user_pages`
  - [ ] check functin `do_wp_page`

- rebuild it http://martins3.gitee.io/dirtycow and add it to Notes.wiki

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


lwn 的文章[^9]说明的内容:
To simplify the situation somewhat, the problems with get_user_pages() come about in two ways. One of those happens when the kernel thinks that the contents of a page will not change, but some peripheral device writes new data there. The other arises with memory that is located on **persistent-memory devices** managed by a filesystem; pinning pages into memory deprives the filesystem of the ability to make layout changes involving those pages. The latter problem has been "solved" for now by disallowing long-lasting page pins on persistent-memory devices, but there are use cases calling for creating just that kind of pin, so better solutions are being sought.
get_user_pages 的问题来源于两个部分 :
1. 内核以为pin 的 page 没有发生修改，但是实际上外设偷偷的对于该内存修改过。(难道不是用户对于这种未经提示的修改感到奇怪吗 ? 类似的事情不是也会发生在 任何将内存映射到设备上的情况吗? )
2. persistent memory device : emmmm 这似乎是 DAX 相关的

Part of the problem comes down to the fact that get_user_pages() does not perform any sort of special tracking of the pages it pins into RAM. It does increment the reference count for each page, preventing it from being evicted from memory, but pages that have been pinned in this way are indistinguishable from pages that have acquired references in any of a vast number of other ways. So, while one can ask whether a page has references, it is not possible for kernel code to ask whether a page has been pinned for purposes like DMA I/O.
1. 虽然通过 reference 可以防止 page 被 evicted (evicted 指的是回收吗 ? 但是这个page 是用户通过 brk 分配的，如果用户进程 exit 了，内核如何知道这个 page 如何回收啊 !)
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
  - 实际上现在只有 2000 多行的样子，根本吓不到人
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

- [ ] 为什么在 dune 中间 gup 会触发 mmu_notifier ?
```
[25556.799013] Hardware name: Timi TM1701/TM1701, BIOS XMAKB5R0P0603 02/02/2018
[25556.799013] Call Trace:
[25556.799019]  dump_stack+0x6d/0x9a
[25556.799037]  ept_mmu_notifier_invalidate_range_start.cold+0x5/0xfe [dune]
[25556.799039]  __mmu_notifier_invalidate_range_start+0x5e/0xa0
[25556.799041]  wp_page_copy+0x6be/0x790
[25556.799042]  ? vsnprintf+0x39e/0x4e0
[25556.799043]  do_wp_page+0x94/0x6a0
[25556.799045]  ? sched_clock+0x9/0x10
[25556.799046]  __handle_mm_fault+0x771/0x7a0
[25556.799047]  handle_mm_fault+0xca/0x200
[25556.799048]  __get_user_pages+0x251/0x7d0
[25556.799049]  get_user_pages_unlocked+0x145/0x1f0
[25556.799050]  get_user_pages_fast+0x180/0x1a0
[25556.799051]  ? ept_lookup_gpa.isra.0+0xb2/0x1a0 [dune]
[25556.799053]  vmx_do_ept_fault+0xe3/0x450 [dune]
```

## 从 gup 到 dirty cow
- [ ] 把这里的傻吊玩意儿整理出来 : http://martins3.gitee.io/dirtycow/#

In particular, each page frame corresponding to a private, writable page (VM_SHARED flag off
and VM_MAYWRITE flag on) is marked as read-only for both the parent and the child, so
that it will be handled with the Copy On Write mechanism.

https://stackoverflow.com/questions/48241187/memory-region-flags-in-linux-why-both-vm-write-and-vm-maywrite-are-needed

/proc/self/mem 向其中写的效果是什么?

- [ ] 是在哪一个位置进行检查权限的，所以就只能 cow 了
  - [ ] 从哪里是告诉 gup 是使用 force 的

从理解上，mmap 一个文件，然后对于一个文件写，必然产生一个原始的 page cache 页面，但是，由于没有权限，所以只能产生一个 cow page 出来，在 cow page 上写，
如果这个 cow page 被 刷掉了，在重新的进行 fault 的时候，没有添加上
但是此时访问请求标记没有了FOLL_WRITE，所以会认为是一个读访问，不会触发COW，这次缺页处理会填充pte对应原始物理页，再次调用follow_page成功获取原始页，所以正常情况会在cowed page上进行读写操作，

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

       MADV_DONTNEED
              Do not expect access in the near future.  (For the time being, the application is finished with the given range, so the kernel can free resources associated with it.)

              After a successful MADV_DONTNEED operation, the semantics of memory access in the specified region are changed: subsequent accesses of pages in the range will succeed, but will result in either repopulating the memory contents from the up-to-date contents of the
              underlying mapped file (for shared file mappings, shared anonymous mappings, and shmem-based techniques such as System V shared memory segments) or zero-fill-on-demand pages for anonymous private mappings.

              Note that, when applied to shared mappings, MADV_DONTNEED might not lead to immediate freeing of the pages in the range.  The kernel is free to delay freeing the pages until an appropriate moment.  The resident set size (RSS) of the calling process will be imme‐
              diately reduced however.

              MADV_DONTNEED cannot be applied to locked pages, Huge TLB pages, or VM_PFNMAP pages.  (Pages marked with the kernel-internal VM_PFNMAP flag are special memory areas that are not managed by the virtual memory subsystem.  Such pages are typically created by device
              drivers that map the pages into user space.)

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


[^1]: https://v1ckydxp.github.io/2020/04/22/2020-04-22-CVE-2016-5195%20%E6%BC%8F%E6%B4%9E%E5%88%86%E6%9E%90/
[^2]: https://medium.com/bindecy/huge-dirty-cow-cve-2017-1000405-110eca132de0
[^3]: https://github.com/dirtycow/dirtycow.github.io/blob/master/dirtyc0w.c
