# folio
- [ ] 将页表示为连续的一组，具体的好处体现在什么地方?
- [ ] 似乎需要首先了解一下 transparent 的内容

## 快捷参考
```c

/**
 * struct folio - Represents a contiguous set of bytes.
 * @flags: Identical to the page flags.
 * @lru: Least Recently Used list; tracks how recently this folio was used.
 * @mlock_count: Number of times this folio has been pinned by mlock().
 * @mapping: The file this page belongs to, or refers to the anon_vma for
 *    anonymous memory.
 * @index: Offset within the file, in units of pages.  For anonymous memory,
 *    this is the index from the beginning of the mmap.
 * @private: Filesystem per-folio data (see folio_attach_private()).
 *    Used for swp_entry_t if folio_test_swapcache().
 * @_mapcount: Do not access this member directly.  Use folio_mapcount() to
 *    find out how many times this folio is mapped by userspace.
 * @_refcount: Do not access this member directly.  Use folio_ref_count()
 *    to find how many references there are to this folio.
 * @memcg_data: Memory Control Group data.
 * @_flags_1: For large folios, additional page flags.
 * @__head: Points to the folio.  Do not use.
 * @_folio_dtor: Which destructor to use for this folio.
 * @_folio_order: Do not use directly, call folio_order().
 * @_total_mapcount: Do not use directly, call folio_entire_mapcount().
 * @_pincount: Do not use directly, call folio_maybe_dma_pinned().
 * @_folio_nr_pages: Do not use directly, call folio_nr_pages().
 *
 * A folio is a physically, virtually and logically contiguous set
 * of bytes.  It is a power-of-two in size, and it is aligned to that
 * same power-of-two.  It is at least as large as %PAGE_SIZE.  If it is
 * in the page cache, it is at a file offset which is a multiple of that
 * power-of-two.  It may be mapped into userspace at an address which is
 * at an arbitrary page offset, but its kernel virtual address is aligned
 * to its size.
 */
struct folio {
	/* private: don't document the anon union */
	union {
		struct {
	/* public: */
			unsigned long flags;
			union {
				struct list_head lru;
	/* private: avoid cluttering the output */
				struct {
					void *__filler;
	/* public: */
					unsigned int mlock_count;
	/* private: */
				};
	/* public: */
			};
			struct address_space *mapping;
			pgoff_t index;
			void *private;
			atomic_t _mapcount;
			atomic_t _refcount;
#ifdef CONFIG_MEMCG
			unsigned long memcg_data;
#endif
	/* private: the union with struct page is transitional */
		};
		struct page page;
	};
	unsigned long _flags_1;
	unsigned long __head;
	unsigned char _folio_dtor;
	unsigned char _folio_order;
	atomic_t _total_mapcount;
	atomic_t _pincount;
#ifdef CONFIG_64BIT
	unsigned int _folio_nr_pages;
#endif
};
```

```c
		struct {	/* Tail pages of compound page */
			unsigned long compound_head;	/* Bit zero is set */

			/* First tail page only */
			unsigned char compound_dtor;
			unsigned char compound_order;
			atomic_t compound_mapcount;
			atomic_t compound_pincount;
#ifdef CONFIG_64BIT
			unsigned int compound_nr; /* 1 << compound_order */
#endif
		};
```

## 阅读一下各种资料
- [An introduction to compound pages](https://lwn.net/Articles/619514/)
  - Compound pages can serve as anonymous memory or be used as buffers within the kernel; they cannot, however, appear in the page cache, which is only prepared to deal with singleton pages.

- [Clarifying memory management with page folios](https://lwn.net/Articles/849538/)
  - `get_folio` and `put_folio` will manage references to the folio much like `get_page` and `put_page`, but without the unneeded calls to `compound_head`

> In an attempt to clarify the situation, Wilcox has come up with the concept of a "page folio", which is really just a page structure that is guaranteed not to be a tail page. Any function accepting a folio will operate on the full compound page (if, indeed, it is a compound page) with no ambiguity. The result is greater clarity in the kernel's memory-management subsystem; as functions are converted to take folios as arguments, it will become clear that they are not meant to operate on tail pages.

如果参数是 folio，那么表示不是 tail page


## 创建
prep_compound_page : 初始化和 page 相关的各种参数

```txt
#0  prep_compound_page (order=9, page=0xffffea0004c58000) at mm/page_alloc.c:818
#1  prep_new_page (alloc_flags=<optimized out>, gfp_flags=<optimized out>, order=9, page=0xffffea0004c58000) at mm/page_alloc.c:2542
#2  get_page_from_freelist (gfp_mask=<optimized out>, order=order@entry=9, alloc_flags=<optimized out>, ac=ac@entry=0xffffc900017afce0) at mm/page_alloc.c:4288
#3  0xffffffff8130032d in __alloc_pages (gfp=gfp@entry=4006090, order=order@entry=9, preferred_nid=preferred_nid@entry=0, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/page_alloc.c:5555
#4  0xffffffff81300d62 in __folio_alloc (gfp=gfp@entry=4006090, order=order@entry=9, preferred_nid=preferred_nid@entry=0, nodemask=nodemask@entry=0x0 <fixed_percpu_data>) at mm/page_alloc.c:5587
#5  0xffffffff8132050d in __folio_alloc_node (nid=0, order=<optimized out>, gfp=4006090) at include/linux/gfp.h:232
#6  vma_alloc_folio (gfp=gfp@entry=1843402, order=order@entry=9, vma=vma@entry=0xffff8881238675f0, addr=addr@entry=140047663759360, hugepage=hugepage@entry=true) at mm/mempolicy.c:2227
#7  0xffffffff8133635e in do_huge_pmd_anonymous_page (vmf=vmf@entry=0xffffc900017afdf8) at mm/huge_memory.c:832
#8  0xffffffff812dce68 in create_huge_pmd (vmf=0xffffc900017afdf8) at mm/memory.c:4820
#9  __handle_mm_fault (vma=vma@entry=0xffff8881238675f0, address=address@entry=140047663759360, flags=flags@entry=597) at mm/memory.c:5067
#10 0xffffffff812dd620 in handle_mm_fault (vma=0xffff8881238675f0, address=address@entry=140047663759360, flags=flags@entry=597, regs=regs@entry=0xffffc900017aff58) at mm/memory.c:5218
#11 0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc900017aff58, error_code=error_code@entry=6, address=address@entry=140047663759360) at arch/x86/mm/fault.c:1428
#12 0xffffffff81fa8e12 in handle_page_fault (address=140047663759360, error_code=6, regs=0xffffc900017aff58) at arch/x86/mm/fault.c:1519
#13 exc_page_fault (regs=0xffffc900017aff58, error_code=6) at arch/x86/mm/fault.c:1575
#14 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

## 在 sublime merge 使用 Merge tag 'folio 来搜索

分析结束，根本没有进行任何实质性改变，只是将接口改动了。

顺便修改了

- mlock_pagevec
- PG_anon_exclusive 6c287605fd56466e645693eff3ae7c08fba56e0
- migrate_device 是做啥的，最近拆分出来的

- [ ] 能不能测试一下 folio 的效果，什么时候，将多个 page 聚合为一个

## 之前的笔记
```c
static inline struct page *compound_head(struct page *page)
{
	unsigned long head = READ_ONCE(page->compound_head);

	if (unlikely(head & 1))
		return (struct page *) (head - 1);
	return page;
}
```

在 page 几个并行的 struct 中间:
```c
		struct {	/* Tail pages of compound page */
			unsigned long compound_head;	/* Bit zero is set */

			/* First tail page only */
			unsigned char compound_dtor;
			unsigned char compound_order;
			atomic_t compound_mapcount;
			atomic_t compound_pincount;
#ifdef CONFIG_64BIT
			unsigned int compound_nr; /* 1 << compound_order */
#endif
		};
```

> https://lwn.net/Articles/619333/


 a call to PageCompound() will return a true value if the passed-in page is a compound page. Head and tail pages can be distinguished, should the need arise, with PageHead() and PageTail()

```c
static __always_inline int PageCompound(struct page *page)
{
	return test_bit(PG_head, &page->flags) ||
	       READ_ONCE(page->compound_head) & 1;
}
```

Every tail page has a pointer to the head page stored in the `first_page` field of struct page.
This field occupies the same storage as the private field, the spinlock used when the page holds page table entries, or the slab_cache pointer used when the page is owned by a slab allocator. The compound_head() helper function can be used to find the head page associated with any tail page.

## compound page
- [An introduction to compound pages](https://lwn.net/Articles/619514/)
> A compound page is simply a grouping of two or more physically contiguous pages into a unit that can, in many ways, be treated as a single, larger page. They are most commonly used to create huge pages, used within hugetlbfs or the transparent huge pages subsystem, *but they show up in other contexts as well*. *Compound pages can serve as anonymous memory or be used as buffers within the kernel*; *they cannot, however, appear in the page cache, which is only prepared to deal with singleton pages.*

- [x] so why page cache is only prepared to deal with singleton pages ? I think it's rather reasonable to use huge page as backend for page cache.
  - https://lwn.net/Articles/619738/ suggests page cache can use thp too.


- [ ] find the use case the compound page is buffer within the kernel
- [ ] 是不是 compound_head 出现的位置，就是和 huge memory 相关的 ?

> Allocating a compound page is a matter of calling a normal memory allocation function like alloc_pages() with the `__GFP_COMP` allocation flag set and an order of at least one
> The difference is that creating a compound page involves the creation of a fair amount of metadata; much of the time, **that metadata is unneeded so the expense of creating it can be avoided.**

> Let's start with the page flags. The first (normal) page in a compound page is called the "head page"; it has the PG_head flag set. All other pages are "tail pages"; they are marked with PG_tail. At least, that is the case on systems where page flags are not in short supply — 64-bit systems, in other words. On 32-bit systems, there are no page flags to spare, so a different scheme is used; all pages in a compound page have the PG_compound flag set, and the tail pages have PG_reclaim set as well. The PG_reclaim bit is normally used by the page cache code, but, since compound pages cannot be represented in the page cache, that flag can be reused here.
>
> Head and tail pages can be distinguished, should the need arise, with PageHead() and PageTail().

- [ ] verify the complications in 32bit in PageHead() and PageTail()

> Every tail page has a pointer to the head page stored in the `first_page` field of struct page. This field occupies the same storage as the private field, the spinlock used when the page holds page table entries, or the slab_cache pointer used when the page is owned by a slab allocator. The `compound_head()` helper function can be used to find the head page associated with any tail page.

- [ ] 了解一下函数 : PageTransHuge，以及附近的定义，似乎 hugepagefs 和 transparent hugepage 谁采用使用 compound_head 的

- [Minimizing the use of tail pages](https://lwn.net/Articles/787388/)

- [ ] read the article

## 更多资料
- https://lwn.net/Articles/565097/

## 阅读其他章节想到的问题
- struct folio 将两个 page 的两部分放到一起的
- 因为 page struct 本身就是两个连续的

## 为什么 folio 会和文件系统有关
