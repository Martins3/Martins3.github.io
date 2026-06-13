# vmflags
<!-- fa2bf1f1-69ad-4c48-9d06-17fbda9d42f8 -->

```c
/*
 * vm_flags in vm_area_struct, see mm_types.h.
 * When changing, update also include/trace/events/mmflags.h
 */
#define VM_NONE		0x00000000

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

/* mprotect() hardcodes VM_MAYREAD >> 4 == VM_READ, and so for r/w/x bits. */
#define VM_MAYREAD	0x00000010	/* limits for mprotect() etc */
#define VM_MAYWRITE	0x00000020
#define VM_MAYEXEC	0x00000040
#define VM_MAYSHARE	0x00000080

#define VM_GROWSDOWN	0x00000100	/* general info on the segment */
#ifdef CONFIG_MMU
#define VM_UFFD_MISSING	0x00000200	/* missing pages tracking */
#else /* CONFIG_MMU */
#define VM_MAYOVERLAY	0x00000200	/* nommu: R/O MAP_PRIVATE mapping that might overlay a file mapping */
#define VM_UFFD_MISSING	0
#endif /* CONFIG_MMU */
#define VM_PFNMAP	0x00000400	/* Page-ranges managed without "struct page", just pure PFN */
#define VM_UFFD_WP	0x00001000	/* wrprotect pages tracking */

#define VM_LOCKED	0x00002000
#define VM_IO           0x00004000	/* Memory mapped I/O or similar */

					/* Used by sys_madvise() */
#define VM_SEQ_READ	0x00008000	/* App will access data sequentially */
#define VM_RAND_READ	0x00010000	/* App will not benefit from clustered reads */

#define VM_DONTCOPY	0x00020000      /* Do not copy this vma on fork */
#define VM_DONTEXPAND	0x00040000	/* Cannot expand with mremap() */
#define VM_LOCKONFAULT	0x00080000	/* Lock the pages covered when they are faulted in */
#define VM_ACCOUNT	0x00100000	/* Is a VM accounted object */
#define VM_NORESERVE	0x00200000	/* should the VM suppress accounting */
#define VM_HUGETLB	0x00400000	/* Huge TLB Page VM */
#define VM_SYNC		0x00800000	/* Synchronous page faults */
#define VM_ARCH_1	0x01000000	/* Architecture-specific flag */
#define VM_WIPEONFORK	0x02000000	/* Wipe VMA contents in child. */
#define VM_DONTDUMP	0x04000000	/* Do not include in the core dump */

#ifdef CONFIG_MEM_SOFT_DIRTY
# define VM_SOFTDIRTY	0x08000000	/* Not soft dirty clean area */
#else
# define VM_SOFTDIRTY	0
#endif

#define VM_MIXEDMAP	0x10000000	/* Can contain "struct page" and pure PFN pages */
#define VM_HUGEPAGE	0x20000000	/* MADV_HUGEPAGE marked this vma */
#define VM_NOHUGEPAGE	0x40000000	/* MADV_NOHUGEPAGE marked this vma */
#define VM_MERGEABLE	0x80000000	/* KSM may merge identical pages */
```

### VM_PFNMAP 和 VM_IO 有什么区别?
<!-- cb13c636-8838-4624-950c-1d08e977ff7a -->

- https://stackoverflow.com/questions/22316289/difference-between-vm-io-and-vm-reserved

vma_dump_size
```c
	/* Do not dump I/O mapped devices or special mappings */
	if (vma->vm_flags & VM_IO)
		return 0;
```

意思是，如果 VM_IO ，那么必然是该区域是 VM_PFNMAP 的吧
既然分开，那么就是说，这里的代码可以使用

例如:
1. usbdev_mmap
```c
	vm_flags_set(vma, VM_IO | VM_DONTEXPAND | VM_DONTDUMP);
```
2. mlock_vma_pages_range
3. sg_mmap : 这个例子是可以的，VM_IO 不是该区域一定映射到 mmio 的意思，简单看了下，
这个区域的 page 都是驱动手动分配的，但是最后如何理解，暂时不想理解了。

### 实际使用:  hva_to_pfn_remapped

在 kvm 中 hva_to_pfn 如果遇到 VM_IO | VM_PFNMAP 的 vma ，需要调用
hva_to_pfn_remapped 来建立映射，这两种类型的 page 都是需要走到特殊通道获取

那些 pci bar 的空间就是这样的带有 VM_IO 和 VM_PFNMAP 的 vma :

因为 iommu 将这些地方提前 pin 住，所以，实际上看到的路径如下:
```txt
@[
    fixup_user_fault+5
    vaddr_get_pfns+352
    vfio_pin_pages_remote+259
    vfio_pin_map_dma+245
    vfio_iommu_type1_ioctl+3690
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 7
```

在 iommu 中，从 vaddr_get_pfns 类似的，通过:
首先使用 pin_user_pages_remote 一定会失败，所以通过 follow_fault_pfn 才可以的。

- [ ] 这段注释理解下
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
	if (!kvm_try_get_pfn(pfn))
		r = -EFAULT;
```

### [ ] 为什么 remap_pfn_range 会自动插入 VM_IO 之类的 flags ?
看看 remap_pfn_range 的调用者吧


## TODO
- VM_PFNMAP 可以给用户态使用吗?

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
