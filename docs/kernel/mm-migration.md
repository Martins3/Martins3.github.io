# migration

- Documentation/vm/page_migration.rst

- https://man7.org/linux/man-pages/man2/migrate_pages.2.html

- [ ] 搜索一下 migration ，还是存在很多类似的积累的

- [ ] 似乎 ARM 和 大页都会影响 migration 的实现。


- move_to_new_folio
  - migrate_folio : 如果是 anon 映射
  - mapping->a_ops->migrate_folio : 取决于文件系统
  - fallback_migrate_folio : 如果文件系统没有注册

## 测试工具
> migratepages pid from-nodes to-nodes
>
> https://man7.org/linux/man-pages/man8/migratepages.8.html

## 两个 syscall 对比 migrate_pages move_pages
- migrate_pages : 在 mempolicy.c 中
- move_pages : 在 migrate.c 中 ，只是粒度更加细

使用 migrate pages 来计算得到的:
```txt
#0  migrate_pages (from=from@entry=0xffffc90001a8be08, get_new_page=0xffffffff813315c0 <alloc_migration_target>, put_new_page=put_new_page@entry=0x0 <fixed_percpu_data>, private=private@entry=18446683600597859864, mode=mode@entry=MIGRATE_SYNC, reason=reason@entry=3, ret_succeeded=0x0 <fixed_percpu_data>) at mm/migrate.c:1417
#1  0xffffffff8131e445 in migrate_to_node (mm=mm@entry=0xffff8881620a1100, source=source@entry=0, dest=dest@entry=1, flags=flags@entry=4) at mm/mempolicy.c:1087
#2  0xffffffff8131f554 in do_migrate_pages (mm=mm@entry=0xffff8881620a1100, from=from@entry=0xffffc90001a8bf00, to=to@entry=0xffffc90001a8bf08, flags=4) at mm/mempolicy.c:1186
#3  0xffffffff8131f894 in kernel_migrate_pages (pid=<optimized out>, maxnode=<optimized out>, old_nodes=<optimized out>, new_nodes=<optimized out>) at mm/mempolicy.c:1663
#4  0xffffffff8131f934 in __do_sys_migrate_pages (new_nodes=<optimized out>, old_nodes=<optimized out>, maxnode=<optimized out>, pid=<optimized out>) at mm/mempolicy.c:1682
#5  __se_sys_migrate_pages (new_nodes=<optimized out>, old_nodes=<optimized out>, maxnode=<optimized out>, pid=<optimized out>) at mm/mempolicy.c:1678
#6  __x64_sys_migrate_pages (regs=<optimized out>) at mm/mempolicy.c:1678
#7  0xffffffff81fa4bcb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001a8bf58) at arch/x86/entry/common.c:50
#8  do_syscall_64 (regs=0xffffc90001a8bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#9  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:12
```

## 记录一下 syscall 相关的内容

```txt
#0  migrate_pages (from=from@entry=0xffffc9000179fd88, get_new_page=get_new_page@entry=0xffffffff81331550 <alloc_misplaced_dst_page>, put_new_page=put_new_page@entry=0x0 <fixed_percpu_data>, private=private@entry=1, mode=mode@entry=MIGRATE_ASYNC, reason=reason@entry=5, ret_succeeded=0xffffc9000179fd84) at mm/migrate.c:1417
#1  0xffffffff81334631 in migrate_misplaced_page (page=page@entry=0xffffea0005a3f440, vma=vma@entry=0xffff88816549fbe0, node=node@entry=1) at mm/migrate.c:2193
#2  0xffffffff812dcf5a in do_numa_page (vmf=0xffffc9000179fdf8) at mm/memory.c:4783
#3  handle_pte_fault (vmf=0xffffc9000179fdf8) at mm/memory.c:4962
#4  __handle_mm_fault (vma=vma@entry=0xffff88816549fbe0, address=address@entry=140728201447760, flags=flags@entry=596) at mm/memory.c:5097
#5  0xffffffff812dd620 in handle_mm_fault (vma=0xffff88816549fbe0, address=address@entry=140728201447760, flags=flags@entry=596, regs=regs@entry=0xffffc9000179ff58) at mm/memory.c:5218
#6  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc9000179ff58, error_code=error_code@entry=4, address=address@entry=140728201447760) at arch/x86/mm/fault.c:1428
#7  0xffffffff81fa8e02 in handle_page_fault (address=140728201447760, error_code=4, regs=0xffffc9000179ff58) at arch/x86/mm/fault.c:1519
#8  exc_page_fault (regs=0xffffc9000179ff58, error_code=4) at arch/x86/mm/fault.c:1575
#9  0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

## migrate
- [ ] make_migration_entry()， 看来 migrate 的类型甚至可以出现在 pte 上，看来 migrate 不是简单的复制粘贴了

1. migrate 并不是为了实现 numa 而设计的，其实在 numa 节点之间迁移并没有什么难度，
虽然 numa 系统在访问速度上存在区别，但是寻址空间都是同一个，
所以完成迁移的工作只是拷贝而已。
2. 为了用户可以控制分配的内存，系统调用 move_pages(2) 和 migrate_pages
3. 在 migrate 实现 compaction 的基础

内核文档讲解的很清晰 : [^11]

migrate 什么类型的 page ?
1. 如果这个 page 是被内核数据，比如 page cache，inode cache 之类的 ? 应该没有办法 migrate 吧 ?
    1. 这不是透明，但是 page cache 之类的不能迁移还是太浪费了，但是需要 address_space_operations::migratepage 和 address_space_operations::isolate_page 辅助

看似只是拷贝，但是为什么写了好几千行
1. 解除一个 page 的联系，并且重新建立。
    1. page 可能在 TLB 中间，应该需要 invalid 特定地址上的 tlb
    2. page table 需要修改
2. hugepage 如何迁移 (似乎)[^10]


核心函数 A : migrate_pages 被 compaction 使用:
```c
/*
 * migrate_pages - migrate the pages specified in a list, to the free pages
 *       supplied as the target for the page migration
 *
 * @from:   The list of pages to be migrated.
 * @get_new_page: The function used to allocate free pages to be used
 *      as the target of the page migration.
 * @put_new_page: The function used to free target pages if migration
 *      fails, or NULL if no special handling is necessary.
 * @private:    Private data to be passed on to get_new_page()
 * @mode:   The migration mode that specifies the constraints for
 *      page migration, if any.
 * @reason:   The reason for page migration.
 *
 * The function returns after 10 attempts or if no pages are movable any more
 * because the list has become empty or no retryable pages exist any more.
 * The caller should call putback_movable_pages() to return pages to the LRU
 * or free list only if ret != 0.
 *
 * Returns the number of pages that were not migrated, or an error code.
 */
int migrate_pages(struct list_head *from, new_page_t get_new_page,
    free_page_t put_new_page, unsigned long private,
    enum migrate_mode mode, int reason)
// 调用 : unmap_and_move_huge_page 或者 unmap_and_move 维持一下生活
```
其实可以对于函数调用进行

对于实现 isolate 的猜测:
1. 按照 pageblock 的单位进行标记: 系统初始化的时候，其中的内容早就标记好了
2. 其他的 pageblock 根据 alloc_page 的 flags 确定。
> 不知道是否会选择合适的 pageblock 进行

```c
// 几乎是唯一初始化 ac->migratetype 的地方
// 另一个在 unreserve_highatomic_pageblock
static inline int gfpflags_to_migratetype(const gfp_t gfp_flags) {
  return (gfp_flags & GFP_MOVABLE_MASK) >> GFP_MOVABLE_SHIFT;
}
```


- [ ] comments below in /home/maritns3/core/linux/include/linux/page-flags.h
  - [ ] PAGE_MAPPING_MOVABLE : I think all anon page is movable

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
#define PAGE_MAPPING_ANON 0x1
#define PAGE_MAPPING_MOVABLE  0x2
#define PAGE_MAPPING_KSM  (PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
#define PAGE_MAPPING_FLAGS  (PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
```

```diff
History:        #0
Commit:         bda807d4445414e8e77da704f116bb0880fe0c76
Author:         Minchan Kim <minchan@kernel.org>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Wed 27 Jul 2016 06:23:05 AM CST
Committer Date: Wed 27 Jul 2016 07:19:19 AM CST

mm: migrate: support non-lru movable page migration

We have allowed migration for only LRU pages until now and it was enough
to make high-order pages.  But recently, embedded system(e.g., webOS,
android) uses lots of non-movable pages(e.g., zram, GPU memory) so we
have seen several reports about troubles of small high-order allocation.
For fixing the problem, there were several efforts (e,g,.  enhance
compaction algorithm, SLUB fallback to 0-order page, reserved memory,
vmalloc and so on) but if there are lots of non-movable pages in system,
their solutions are void in the long run.

So, this patch is to support facility to change non-movable pages with
movable.  For the feature, this patch introduces functions related to
migration to address_space_operations as well as some page flags.

If a driver want to make own pages movable, it should define three
functions which are function pointers of struct
address_space_operations.

1. bool (*isolate_page) (struct page *page, isolate_mode_t mode);

What VM expects on isolate_page function of driver is to return *true*
if driver isolates page successfully.  On returing true, VM marks the
page as PG_isolated so concurrent isolation in several CPUs skip the
page for isolation.  If a driver cannot isolate the page, it should
return *false*.

Once page is successfully isolated, VM uses page.lru fields so driver
shouldn't expect to preserve values in that fields.

2. int (*migratepage) (struct address_space *mapping,
   struct page *newpage, struct page *oldpage, enum migrate_mode);

After isolation, VM calls migratepage of driver with isolated page.  The
function of migratepage is to move content of the old page to new page
and set up fields of struct page newpage.  Keep in mind that you should
indicate to the VM the oldpage is no longer movable via
__ClearPageMovable() under page_lock if you migrated the oldpage
successfully and returns 0.  If driver cannot migrate the page at the
moment, driver can return -EAGAIN.  On -EAGAIN, VM will retry page
migration in a short time because VM interprets -EAGAIN as "temporal
migration failure".  On returning any error except -EAGAIN, VM will give
up the page migration without retrying in this time.

Driver shouldn't touch page.lru field VM using in the functions.

3. void (*putback_page)(struct page *);

If migration fails on isolated page, VM should return the isolated page
to the driver so VM calls driver's putback_page with migration failed
page.  In this function, driver should put the isolated page back to the
own data structure.

4. non-lru movable page flags

There are two page flags for supporting non-lru movable page.

* PG_movable

Driver should use the below function to make page movable under
page_lock.

 void __SetPageMovable(struct page *page, struct address_space *mapping)

It needs argument of address_space for registering migration family
functions which will be called by VM.  Exactly speaking, PG_movable is
not a real flag of struct page.  Rather than, VM reuses page->mapping's
lower bits to represent it.

 #define PAGE_MAPPING_MOVABLE 0x2
 page->mapping = page->mapping | PAGE_MAPPING_MOVABLE;

so driver shouldn't access page->mapping directly.  Instead, driver
should use page_mapping which mask off the low two bits of page->mapping
so it can get right struct address_space.

For testing of non-lru movable page, VM supports __PageMovable function.
However, it doesn't guarantee to identify non-lru movable page because
page->mapping field is unified with other variables in struct page.  As
well, if driver releases the page after isolation by VM, page->mapping
doesn't have stable value although it has PAGE_MAPPING_MOVABLE (Look at
__ClearPageMovable).  But __PageMovable is cheap to catch whether page
is LRU or non-lru movable once the page has been isolated.  Because LRU
pages never can have PAGE_MAPPING_MOVABLE in page->mapping.  It is also
good for just peeking to test non-lru movable pages before more
expensive checking with lock_page in pfn scanning to select victim.

For guaranteeing non-lru movable page, VM provides PageMovable function.
Unlike __PageMovable, PageMovable functions validates page->mapping and
mapping->a_ops->isolate_page under lock_page.  The lock_page prevents
sudden destroying of page->mapping.

Driver using __SetPageMovable should clear the flag via
__ClearMovablePage under page_lock before the releasing the page.

* PG_isolated

To prevent concurrent isolation among several CPUs, VM marks isolated
page as PG_isolated under lock_page.  So if a CPU encounters PG_isolated
non-lru movable page, it can skip it.  Driver doesn't need to manipulate
the flag because VM will set/clear it automatically.  Keep in mind that
if driver sees PG_isolated page, it means the page have been isolated by
VM so it shouldn't touch page.lru field.  PG_isolated is alias with
PG_reclaim flag so driver shouldn't use the flag for own purpose.

[opensource.ganesh@gmail.com: mm/compaction: remove local variable is_lru]
  Link: http://lkml.kernel.org/r/20160618014841.GA7422@leo-test
Link: http://lkml.kernel.org/r/1464736881-24886-3-git-send-email-minchan@kernel.org
Signed-off-by: Gioh Kim <gi-oh.kim@profitbricks.com>
Signed-off-by: Minchan Kim <minchan@kernel.org>
Signed-off-by: Ganesh Mahendran <opensource.ganesh@gmail.com>
Acked-by: Vlastimil Babka <vbabka@suse.cz>
Cc: Sergey Senozhatsky <sergey.senozhatsky@gmail.com>
Cc: Rik van Riel <riel@redhat.com>
Cc: Joonsoo Kim <iamjoonsoo.kim@lge.com>
Cc: Mel Gorman <mgorman@suse.de>
Cc: Hugh Dickins <hughd@google.com>
Cc: Rafael Aquini <aquini@redhat.com>
Cc: Jonathan Corbet <corbet@lwn.net>
Cc: John Einar Reitan <john.reitan@foss.arm.com>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

[^10]: [stackoverflow : Using move_pages() to move hugepages?](https://stackoverflow.com/questions/59726288/using-move-pages-to-move-hugepages)
[^11]: [kernel doc : page migratin](https://www.kernel.org/doc/html/latest/vm/page_migration.html)
