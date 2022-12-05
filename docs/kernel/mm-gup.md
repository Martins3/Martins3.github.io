# get user page

## TODO
- [ ] 为什么 hugepage 会更加麻烦 ?
- fault_in_writable
- [ ] copy from user 到底是如何进行的，必然需要 pin 吗?

## follow_page
- 根据虚拟地址找到物理地址，例如 move_pages(2) 中查找每一个 page 中所在的 Node


- follow_p4d_mask
- follow_page_mask

为什么，这个有时候是从 p4d，有时候直接从 pmd 开始的
```txt
#0  follow_p4d_mask (ctx=<optimized out>, flags=<optimized out>, pgdp=<optimized out>, address=<optimized out>, vma=<optimized out>) at mm/gup.c:836
#1  follow_page_mask (vma=vma@entry=0xffff888166ec4da8, address=140737488351202, flags=flags@entry=8215, ctx=ctx@entry=0xffffc90001a8bda0) at mm/gup.c:900
#2  0xffffffff812cfbab in __get_user_pages (mm=mm@entry=0xffff8881620a2ec0, start=<optimized out>, start@entry=140737488351202, nr_pages=<optimized out>, nr_pages@entry=1, gup_flags=gup_flags@entry=8215, pages=pages@entry=0xffffc90001a8be60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:1228
#3  0xffffffff812d18f5 in __get_user_pages_locked (flags=8215, locked=0x0 <fixed_percpu_data>, vmas=0x0 <fixed_percpu_data>, pages=0xffffc90001a8be60, nr_pages=1, start=140737488351202, mm=0xffff8881620a2ec0) at mm/gup.c:1434
#4  __get_user_pages_remote (mm=0xffff8881620a2ec0, start=start@entry=140737488351202, nr_pages=nr_pages@entry=1, gup_flags=gup_flags@entry=17, pages=pages@entry=0xffffc90001a8be60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:2187
#5  0xffffffff812d1b65 in get_user_pages_remote (mm=<optimized out>, start=start@entry=140737488351202, nr_pages=nr_pages@entry=1, gup_flags=gup_flags@entry=17, pages=pages@entry=0xffffc90001a8be60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:2260
#6  0xffffffff8136d28a in get_arg_page (bprm=bprm@entry=0xffff888165b3ea00, pos=pos@entry=140737488351202, write=write@entry=1) at fs/exec.c:220
#7  0xffffffff8136d3f0 in copy_string_kernel (arg=0xffff888164c1a020 "/usr/bin/migratepages", bprm=bprm@entry=0xffff888165b3ea00) at fs/exec.c:637
#8  0xffffffff8136e979 in do_execveat_common (fd=fd@entry=-100, filename=0xffff888164c1a000, flags=0, envp=..., argv=..., envp=..., argv=...) at fs/exec.c:1916
#9  0xffffffff8136ec0e in do_execve (__envp=0x5558dcccc390, __argv=0x7f1a9c21b890, filename=<optimized out>) at fs/exec.c:2016
#10 __do_sys_execve (envp=0x5558dcccc390, argv=0x7f1a9c21b890, filename=<optimized out>) at fs/exec.c:2092
#11 __se_sys_execve (envp=<optimized out>, argv=<optimized out>, filename=<optimized out>) at fs/exec.c:2087
#12 __x64_sys_execve (regs=<optimized out>) at fs/exec.c:2087
#13 0xffffffff81fa4bcb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001a8bf58) at arch/x86/entry/common.c:50
#14 do_syscall_64 (regs=0xffffc90001a8bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#15 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

```txt
#0  follow_page_pte (vma=vma@entry=0xffff888166ec4da8, address=140737488351202, pmd=0xffff88816ccbfff8, flags=flags@entry=8215, pgmap=pgmap@entry=0xffffc90001a8bda0) at mm/gup.c:530
#1  0xffffffff812cf684 in follow_pmd_mask (pudp=<optimized out>, pudp=<optimized out>, ctx=0xffffc90001a8bda0, flags=8215, address=<optimized out>, vma=0xffff888166ec4da8) at mm/gup.c:765
#2  0xffffffff812cfbab in __get_user_pages (mm=mm@entry=0xffff8881620a2ec0, start=<optimized out>, start@entry=140737488351202, nr_pages=<optimized out>, nr_pages@entry=1, gup_flags=gup_flags@entry=8215, pages=pages@entry=0xffffc90001a8be60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:1228
#3  0xffffffff812d18f5 in __get_user_pages_locked (flags=8215, locked=0x0 <fixed_percpu_data>, vmas=0x0 <fixed_percpu_data>, pages=0xffffc90001a8be60, nr_pages=1, start=140737488351202, mm=0xffff8881620a2ec0) at mm/gup.c:1434
#4  __get_user_pages_remote (mm=0xffff8881620a2ec0, start=start@entry=140737488351202, nr_pages=nr_pages@entry=1, gup_flags=gup_flags@entry=17, pages=pages@entry=0xffffc90001a8be60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:2187
#5  0xffffffff812d1b65 in get_user_pages_remote (mm=<optimized out>, start=start@entry=140737488351202, nr_pages=nr_pages@entry=1, gup_flags=gup_flags@entry=17, pages=pages@entry=0xffffc90001a8be60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:2260
#6  0xffffffff8136d28a in get_arg_page (bprm=bprm@entry=0xffff888165b3ea00, pos=pos@entry=140737488351202, write=write@entry=1) at fs/exec.c:220
#7  0xffffffff8136d3f0 in copy_string_kernel (arg=0xffff888164c1a020 "/usr/bin/migratepages", bprm=bprm@entry=0xffff888165b3ea00) at fs/exec.c:637
#8  0xffffffff8136e979 in do_execveat_common (fd=fd@entry=-100, filename=0xffff888164c1a000, flags=0, envp=..., argv=..., envp=..., argv=...) at fs/exec.c:1916
#9  0xffffffff8136ec0e in do_execve (__envp=0x5558dcccc390, __argv=0x7f1a9c21b890, filename=<optimized out>) at fs/exec.c:2016
#10 __do_sys_execve (envp=0x5558dcccc390, argv=0x7f1a9c21b890, filename=<optimized out>) at fs/exec.c:2092
#11 __se_sys_execve (envp=<optimized out>, argv=<optimized out>, filename=<optimized out>) at fs/exec.c:2087
#12 __x64_sys_execve (regs=<optimized out>) at fs/exec.c:2087
#13 0xffffffff81fa4bcb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001a8bf58) at arch/x86/entry/common.c:50
#14 do_syscall_64 (regs=0xffffc90001a8bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#15 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#16 0x0000000000000000 in ?? ()
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
#0  mark_page_accessed (page=page@entry=0xffffea0004e92940) at mm/folio-compat.c:50
#1  0xffffffff812cf171 in follow_page_pte (vma=vma@entry=0xffff8881230a3390, address=<optimized out>, pmd=<optimized out>, flags=flags@entry=8215, pgmap=pgmap@entry=0xffffc90001a93da0) at mm/gup.c:653
#2  0xffffffff812cf664 in follow_pmd_mask (pudp=<optimized out>, pudp=<optimized out>, ctx=0xffffc90001a93da0, flags=8215, address=<optimized out>, vma=0xffff8881230a3390) at mm/gup.c:765
#3  0xffffffff812cfb8b in __get_user_pages (mm=mm@entry=0xffff888125b22200, start=<optimized out>, start@entry=140737488351206, nr_pages=<optimized out>, nr_pages@entry=1, gup_flags=gup_flags@entry=8215, pages=pages@entry=0xffffc90001a93e60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:1228
#4  0xffffffff812d18d5 in __get_user_pages_locked (flags=8215, locked=0x0 <fixed_percpu_data>, vmas=0x0 <fixed_percpu_data>, pages=0xffffc90001a93e60, nr_pages=1, start=140737488351206, mm=0xffff888125b22200) at mm/gup.c:1434
#5  __get_user_pages_remote (mm=0xffff888125b22200, start=start@entry=140737488351206, nr_pages=nr_pages@entry=1, gup_flags=gup_flags@entry=17, pages=pages@entry=0xffffc90001a93e60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:2187
#6  0xffffffff812d1b45 in get_user_pages_remote (mm=<optimized out>, start=start@entry=140737488351206, nr_pages=nr_pages@entry=1, gup_flags=gup_flags@entry=17, pages=pages@entry=0xffffc90001a93e60, vmas=vmas@entry=0x0 <fixed_percpu_data>, locked=0x0 <fixed_percpu_data>) at mm/gup.c:2260
#7  0xffffffff8136d26a in get_arg_page (bprm=bprm@entry=0xffff88812221b600, pos=pos@entry=140737488351206, write=write@entry=1) at fs/exec.c:220
#8  0xffffffff8136d3d0 in copy_string_kernel (arg=0xffff8881212b7020 "/usr/local/sbin/c", bprm=bprm@entry=0xffff88812221b600) at fs/exec.c:637
#9  0xffffffff8136e959 in do_execveat_common (fd=fd@entry=-100, filename=0xffff8881212b7000, flags=0, envp=..., argv=..., envp=..., argv=...) at fs/exec.c:1916
#10 0xffffffff8136ebee in do_execve (__envp=0x5626a6f602a0, __argv=0x7f47952d96b0, filename=<optimized out>) at fs/exec.c:2016
#11 __do_sys_execve (envp=0x5626a6f602a0, argv=0x7f47952d96b0, filename=<optimized out>) at fs/exec.c:2092
#12 __se_sys_execve (envp=<optimized out>, argv=<optimized out>, filename=<optimized out>) at fs/exec.c:2087
#13 __x64_sys_execve (regs=<optimized out>) at fs/exec.c:2087
#14 0xffffffff81fa3bdb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001a93f58) at arch/x86/entry/common.c:50
#15 do_syscall_64 (regs=0xffffc90001a93f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#16 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#17 0x0000000000000000 in ?? ()
```

[^1]: https://v1ckydxp.github.io/2020/04/22/2020-04-22-CVE-2016-5195%20%E6%BC%8F%E6%B4%9E%E5%88%86%E6%9E%90/
[^2]: https://medium.com/bindecy/huge-dirty-cow-cve-2017-1000405-110eca132de0
[^3]: https://github.com/dirtycow/dirtycow.github.io/blob/master/dirtyc0w.c
