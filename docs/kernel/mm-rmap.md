# rmap

## file
关联文件的简单很多，无需考虑各种 clone 的关系，所有的 vma 都关联到映射的 file 即可。

当 unmap 一个文件的时候，可以使用 unlink_file_vma 将该 vma 在 interval tree 中删除

page_add_file_rmap 如果是普通页，其主要工作为: `page->_mapcount`，将 `__page_set_anon_rmap` 对称的工作拆分到如下位置:

1. vma_interval_tree_insert 是将 vma 添加到 interval tree 的时机，两个调用位置
- mmap_region
- `__vma_link_file`
```txt
#0  __vma_link_file (mapping=0xffff888129986318, vma=0xffff88812e467000) at mm/mmap.c:413
#1  __vma_adjust (vma=vma@entry=0xffff88812e467d10, start=94617914904576, end=end@entry=94617915023360, pgoff=0, insert=insert@entry=0xffff88812e467000, expand=expand@entry=0x0 <fixed_percpu_data>) at mm/mmap.c:736
#2  0xffffffff812e479b in vma_adjust (insert=0xffff88812e467000, pgoff=<optimized out>, end=94617915023360, start=<optimized out>, vma=0xffff88812e467d10) at include/linux/mm.h:2663
#3  __split_vma (mm=mm@entry=0xffff888122cb4840, vma=vma@entry=0xffff88812e467d10, addr=addr@entry=94617915023360, new_below=new_below@entry=0) at mm/mmap.c:2240
#4  0xffffffff812e4934 in do_mas_align_munmap (mas=mas@entry=0xffffc90001fc3cf8, vma=0xffff88812e467d10, mm=mm@entry=0xffff888122cb4840, start=start@entry=94617915023360, end=end@entry=94617918484480, uf=uf@entry=0xffffc90001fc3ce8, downgrade=false) at mm/mmap.c:2341
#5  0xffffffff812e4de2 in do_mas_munmap (mas=mas@entry=0xffffc90001fc3cf8, mm=mm@entry=0xffff888122cb4840, start=start@entry=94617915023360, len=len@entry=3461120, uf=uf@entry=0xffffc90001fc3ce8, downgrade=downgrade@entry=false) at mm/mmap.c:2502
#6  0xffffffff812e4ee3 in __vm_munmap (start=start@entry=94617915023360, len=len@entry=3461120, downgrade=downgrade@entry=false) at mm/mmap.c:2775
#7  0xffffffff812e4fa7 in vm_munmap (start=start@entry=94617915023360, len=len@entry=3461120) at mm/mmap.c:2793
#8  0xffffffff813cb256 in elf_map (filep=<optimized out>, addr=addr@entry=94617914904576, eppnt=eppnt@entry=0xffff888026302870, prot=prot@entry=1, type=1048578, total_size=<optimized out>) at fs/binfmt_elf.c:392
#9  0xffffffff813cc5dd in load_elf_binary (bprm=0xffff88812c516c00) at fs/binfmt_elf.c:1166
#10 0xffffffff8136e45e in search_binary_handler (bprm=0xffff88812c516c00) at fs/exec.c:1727
#11 exec_binprm (bprm=0xffff88812c516c00) at fs/exec.c:1768
#12 bprm_execve (flags=<optimized out>, filename=<optimized out>, fd=<optimized out>, bprm=0xffff88812c516c00) at fs/exec.c:1837
#13 bprm_execve (bprm=0xffff88812c516c00, fd=<optimized out>, filename=<optimized out>, flags=<optimized out>) at fs/exec.c:1799
#14 0xffffffff8136e9fd in do_execveat_common (fd=fd@entry=-100, filename=0xffff888127f1a000, flags=0, envp=..., argv=..., envp=..., argv=...) at fs/exec.c:1942
#15 0xffffffff8136ec0e in do_execve (__envp=0x564e869624a0, __argv=0x7f6d42ce62c0, filename=<optimized out>) at fs/exec.c:2016
#16 __do_sys_execve (envp=0x564e869624a0, argv=0x7f6d42ce62c0, filename=<optimized out>) at fs/exec.c:2092
#17 __se_sys_execve (envp=<optimized out>, argv=<optimized out>, filename=<optimized out>) at fs/exec.c:2087
#18 __x64_sys_execve (regs=<optimized out>) at fs/exec.c:2087
#19 0xffffffff81fa4bdb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001fc3f58) at arch/x86/entry/common.c:50
#20 do_syscall_64 (regs=0xffffc90001fc3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#21 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
2. 在 `__filemap_add_folio` 中:
```c
	folio->mapping = mapping;
	folio->index = xas.xa_index;
```

## anon
如果想要知道一个 page 被匿名地址空间映射:
1. 给 page 创建的时候在那个 vma 中间。
2. 该 vma 进一步 fork 出来了那些 vma。

![AVC AV 和 VMA 的关系](https://img2018.cnblogs.com/blog/1771657/202001/1771657-20200108072200806-1580650219.png)

![page->mapping 中指向的是 AV](https://img2018.cnblogs.com/blog/1771657/202001/1771657-20200108072227687-242824885.png)

- 一个 vma 对应一个 AV
- AVC 是 vma 用来挂载到 AV 上的钩子
- 进程每创建一个子进程，父进程的 AV 的红黑树中会增加每一个起“桥梁”作用的 AVC，以此连接到子进程的 VMA
- 第 n 个层次的 child 需要创建创建 n 个 AVC, 分别将 AVC 放到自己和自己的 n - 1 个祖先中间
- 当发生 rmap 遍历，利用 `page->mapping` 找到 anon_vma, 然找到  `anon_vma->rb_root` 来在红黑树中间查找到 avc

```c
/**
 * __page_set_anon_rmap - set up new anonymous rmap
 * @page:	Page or Hugepage to add to rmap
 * @vma:	VM area to add page to.
 * @address:	User virtual address of the mapping
 * @exclusive:	the page is exclusively owned by the current process
 */
static void __page_set_anon_rmap(struct page *page,
	struct vm_area_struct *vma, unsigned long address, int exclusive)
{
	struct anon_vma *anon_vma = vma->anon_vma;

	BUG_ON(!anon_vma);

	if (PageAnon(page))
		goto out;

	/*
	 * If the page isn't exclusively mapped into this vma,
	 * we must use the _oldest_ possible anon_vma for the
	 * page mapping!
	 */
	if (!exclusive)
		anon_vma = anon_vma->root;

	/*
	 * page_idle does a lockless/optimistic rmap scan on page->mapping.
	 * Make sure the compiler doesn't split the stores of anon_vma and
	 * the PAGE_MAPPING_ANON type identifier, otherwise the rmap code
	 * could mistake the mapping for a struct address_space and crash.
	 */
	anon_vma = (void *) anon_vma + PAGE_MAPPING_ANON;
	WRITE_ONCE(page->mapping, (struct address_space *) anon_vma);
	page->index = linear_page_index(vma, address);
out:
	if (exclusive)
		SetPageAnonExclusive(page);
}
```

```txt
#0  __page_set_anon_rmap (page=0xffffea0000997380, vma=0xffff888127fc7688, address=94283249184768, exclusive=1) at mm/rmap.c:1126
#1  0xffffffff812dd09b in do_anonymous_page (vmf=0xffffc90001967df8) at mm/memory.c:4153
#2  handle_pte_fault (vmf=0xffffc90001967df8) at mm/memory.c:4953
#3  __handle_mm_fault (vma=vma@entry=0xffff888127fc7688, address=address@entry=94283249184872, flags=flags@entry=597) at mm/memory.c:5097
#4  0xffffffff812dd620 in handle_mm_fault (vma=0xffff888127fc7688, address=address@entry=94283249184872, flags=flags@entry=597, regs=regs@entry=0xffffc90001967f58) at mm/memory.c:5218
#5  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc90001967f58, error_code=error_code@entry=6, address=address@entry=94283249184872) at arch/x86/mm/fault.c:1428
#6  0xffffffff81fa8e12 in handle_page_fault (address=94283249184872, error_code=6, regs=0xffffc90001967f58) at arch/x86/mm/fault.c:1519
#7  exc_page_fault (regs=0xffffc90001967f58, error_code=6) at arch/x86/mm/fault.c:1575
#8  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

- 如何理解 exclusive ?
  - page_add_anon_rmap : 需要判断是不是新创建的
  - page_add_new_anon_rmap : 调用 `__page_set_anon_rmap` 的时候，本 vma 一定是 root 的，参数 exclusive 是 1
  - 如果是 exclusive 的，那么 `page->mapping` 中指向本 vma 的 `anon_vma`，否则指向 `anon_vma->root`
  - 大多数的时候是 exclusive 的

## anon_vma_fork 和 anon_vma_clone

- unlink_anon_vmas 是 anon_vma_clone 的逆向操作
- anon_vma_clone 的主要调用位置
  - vma_expand
  - `__split_vma`
  - copy_vma
  - `__vma_adjust`
  - `__split_vma`
- anon_vma_fork 的主要内容:
  - anon_vma_clone : 复制 avc ，并且将 avc 添加到红黑树和链表中
  - 创建 avc av ，并且和 vma 链接起来


```txt
@[
    anon_vma_fork+1
    dup_mm+931
    copy_process+6704
    kernel_clone+151
    __do_sys_clone+102
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+99
]: 3953
```

```txt
@[
    anon_vma_clone+1
    copy_vma+385
    move_vma+331
    __do_sys_mremap+820
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+99
]: 1

@[
    anon_vma_clone+1
    __split_vma+145
    __do_munmap+996
    __vm_munmap+120
    __x64_sys_munmap+23
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+99
]: 1132
@[
    anon_vma_clone+1
    anon_vma_fork+51
    dup_mm+931
    copy_process+6704
    kernel_clone+151
    __do_sys_clone+102
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+99
]: 4328
```

## rmap_walk

- vma_address : 返回 page 在一个虚拟地址空间中的地址，从该地址进行 page walk，来进行各种地址

原理上 : `page->index` -  `vm_area_struct->vm_pgoff` + `vm_area_struct->vm_start`

- `page->index`
  - page 在文件中的偏移
  - page 在 root vma 中的偏移
- `vm_area_struct->vm_pgoff`
  - vma 在文件中的偏移
  - 如果是 private anon，创建的默认为 0，但是之后发生 merge 和 split 等操作时，需要进行调整。
- `vm_area_struct->vm_start`
  - vma 在虚拟地址空间中的偏移


- linear_page_index : 返回
  - file : 该 page 在文件中的偏移
  - anon : 该 page 在 vma 中的偏移

```c
static inline pgoff_t linear_page_index(struct vm_area_struct *vma,
					unsigned long address)
{
	pgoff_t pgoff;
	if (unlikely(is_vm_hugetlb_page(vma)))
		return linear_hugepage_index(vma, address);
	pgoff = (address - vma->vm_start) >> PAGE_SHIFT;
	pgoff += vma->vm_pgoff;
	return pgoff;
}
```

分析一下 rmap_walk，如果一个 file 共享给 1000 一个 vma，那么 rmap_walk 是需要
将这 1000 个 vma 全部遍历一次，即使一个 page 在这些 vma 都没有映射进来。
类似的，一个 anon vma 如何共享给 1000 个 vma，即使 1000 个 vma 都对于该 page 进行了 cow，
该 page 的 rmap 还是会遍历 1000 次数。

## page->mapping 的 flags 的含义

```c
/*
 * On an anonymous page mapped into a user virtual memory area,
 * page->mapping points to its anon_vma, not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.  See rmap.h.
 *
 * On an anonymous page in a VM_MERGEABLE area, if CONFIG_KSM is enabled,
 * the PAGE_MAPPING_MOVABLE bit may be set along with the PAGE_MAPPING_ANON
 * bit; and then page->mapping points, not to an anon_vma, but to a private
 * structure which KSM associates with that merged page.  See ksm.h.
 *
 * PAGE_MAPPING_KSM without PAGE_MAPPING_ANON is used for non-lru movable
 * page and then page->mapping points a struct address_space.
 *
 * Please note that, confusingly, "page_mapping" refers to the inode
 * address_space which maps the page from disk; whereas "page_mapped"
 * refers to user virtual address space into which the page is mapped.
 */
#define PAGE_MAPPING_ANON	0x1
#define PAGE_MAPPING_MOVABLE	0x2
#define PAGE_MAPPING_KSM	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
#define PAGE_MAPPING_FLAGS	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
```
使用 `page->mapping` 最后的两位区分 `page->mapping`  到底是指向什么内容:
- 只有 PAGE_MAPPING_ANON : 指向 anon_vma，用于实现 anon vma 的反向映射；
- 只有 PAGE_MAPPING_MOVABLE : 该页被驱动使用，指向 movable_operations，目前，驱动只有 z3fold zsmalloc 和 swap ；
- 没有 flags: 指向 address_space，描述该页和关联的文件的关系；
- 两个都有，表示为 KSM 处理的页。

`page->mapping` 的复用结果，内核中对应的辅助函数:

```c
struct address_space *folio_mapping(struct folio *folio)
struct anon_vma *folio_get_anon_vma(struct folio *folio)
static inline const struct movable_operations *page_movable_ops(struct page *page)
static __always_inline bool folio_test_ksm(struct folio *folio)
```

## 关键参考
- [【原创】（十五）Linux 内存管理之 RMAP](https://www.cnblogs.com/LoyenWang/p/12164683.html)
- [逆向映射的演进](http://www.wowotech.net/memory_management/reverse_mapping.html)

## reuse 机制

9. 不仅仅可以递归的 fork ，而且可以同一个 process 可以 fork 出来大量的内容出来。
    1. 为什么 av 可以不关联 vma (是不是因为其并没有创建)
    2. reuse 不是通常看到的内容，否则根本无法解释 no alive 的含义。
        1. 在常规情况下(A fork B , B fork C, C 无法 reuse A B 的内容)，只会出现其中的

```txt
commit 2555283eb40df89945557273121e9393ef9b542b
Author: Jann Horn <jannh@google.com>
Date:   Wed Aug 31 19:06:00 2022 +0200

    mm/rmap: Fix anon_vma->degree ambiguity leading to double-reuse

    anon_vma->degree tracks the combined number of child anon_vmas and VMAs
    that use the anon_vma as their ->anon_vma.

    anon_vma_clone() then assumes that for any anon_vma attached to
    src->anon_vma_chain other than src->anon_vma, it is impossible for it to
    be a leaf node of the VMA tree, meaning that for such VMAs ->degree is
    elevated by 1 because of a child anon_vma, meaning that if ->degree
    equals 1 there are no VMAs that use the anon_vma as their ->anon_vma.

    This assumption is wrong because the ->degree optimization leads to leaf
    nodes being abandoned on anon_vma_clone() - an existing anon_vma is
    reused and no new parent-child relationship is created.  So it is
    possible to reuse an anon_vma for one VMA while it is still tied to
    another VMA.

    This is an issue because is_mergeable_anon_vma() and its callers assume
    that if two VMAs have the same ->anon_vma, the list of anon_vmas
    attached to the VMAs is guaranteed to be the same.  When this assumption
    is violated, vma_merge() can merge pages into a VMA that is not attached
    to the corresponding anon_vma, leading to dangling page->mapping
    pointers that will be dereferenced during rmap walks.

    Fix it by separately tracking the number of child anon_vmas and the
    number of VMAs using the anon_vma as their ->anon_vma.

    Fixes: 7a3ef208e662 ("mm: prevent endless growth of anon_vma hierarchy")
    Cc: stable@kernel.org
    Acked-by: Michal Hocko <mhocko@suse.com>
    Acked-by: Vlastimil Babka <vbabka@suse.cz>
    Signed-off-by: Jann Horn <jannh@google.com>
    Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```


## TODO : 理解 lock
```c
/*
 * rmap_walk_control: To control rmap traversing for specific needs
 *
 * arg: passed to rmap_one() and invalid_vma()
 * try_lock: bail out if the rmap lock is contended
 * contended: indicate the rmap traversal bailed out due to lock contention
 * rmap_one: executed on each vma where page is mapped
 * done: for checking traversing termination condition
 * anon_lock: for getting anon_lock by optimized way rather than default
 * invalid_vma: for skipping uninterested vma
 */
struct rmap_walk_control {
	void *arg;
	bool try_lock;
	bool contended;
	/*
	 * Return false if page table scanning in rmap_walk should be stopped.
	 * Otherwise, return true.
	 */
	bool (*rmap_one)(struct folio *folio, struct vm_area_struct *vma,
					unsigned long addr, void *arg);
	int (*done)(struct folio *folio);
	struct anon_vma *(*anon_lock)(struct folio *folio,
				      struct rmap_walk_control *rwc);
	bool (*invalid_vma)(struct vm_area_struct *vma, void *arg);
};
```
- 其中的 anon_lock 如何理解?

- anon_vma_clone 中调用 lock_anon_vma_root

- folio_get_anon_vma 和 folio_lock_anon_vma_read

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
