# 解释 folio->mapping 的含义

1. folio_mapping
  - slab : This happens if someone calls flush_dcache_page on slab page
  - swapcache : 有自己的特有的 address_space
  - file
2. 定义
```c
/*
 * On an anonymous folio mapped into a user virtual memory area,
 * folio->mapping points to its anon_vma, not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.  See rmap.h.
 *
 * On an anonymous page in a VM_MERGEABLE area, if CONFIG_KSM is enabled,
 * the PAGE_MAPPING_MOVABLE bit may be set along with the PAGE_MAPPING_ANON
 * bit; and then folio->mapping points, not to an anon_vma, but to a private
 * structure which KSM associates with that merged page.  See ksm.h.
 *
 * PAGE_MAPPING_KSM without PAGE_MAPPING_ANON is used for non-lru movable
 * page and then folio->mapping points to a struct movable_operations.
 *
 * Please note that, confusingly, "folio_mapping" refers to the inode
 * address_space which maps the folio from disk; whereas "folio_mapped"
 * refers to user virtual address space into which the folio is mapped.
 *
 * For slab pages, since slab reuses the bits in struct page to store its
 * internal states, the folio->mapping does not exist as such, nor do
 * these flags below.  So in order to avoid testing non-existent bits,
 * please make sure that folio_test_slab(folio) actually evaluates to
 * false before calling the following functions (e.g., folio_test_anon).
 * See mm/slab.h.
 */
#define PAGE_MAPPING_ANON	0x1
#define PAGE_MAPPING_MOVABLE	0x2
#define PAGE_MAPPING_KSM	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
#define PAGE_MAPPING_FLAGS	(PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
```
使用 `page->mapping` 最后的两位区分 `page->mapping`  到底是指向什么内容:
- 只有 `PAGE_MAPPING_ANON` : 指向 anon_vma，用于实现 anon vma 的反向映射；
- 只有 `PAGE_MAPPING_MOVABLE` : 该页被驱动使用，指向 movable_operations，目前，驱动只有 z3fold zsmalloc 和 swap ；
- 没有 flags: 指向 address_space，描述该页和关联的文件的关系；
- 两个都有，表示为 KSM 处理的页。

```c
struct address_space *folio_mapping(struct folio *folio);
struct anon_vma *folio_get_anon_vma(struct folio *folio);
static inline const struct movable_operations *page_movable_ops(struct page *page);
static __always_inline bool folio_test_ksm(struct folio *folio);
```

关于 movable_operations 的使用，参考 `move_to_new_folio` ，类似 balloon 之类的 page
不是常规的 anonymous page 或者 file mapped page ，所以 move 有特殊做法。

## movable_operations 原始 patch
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


https://linuxplumbersconf.org/event/2/contributions/65/attachments/15/171/slides-expanded.pdf
> 似乎解释了 compaction 的含义

- Isolates movable pages from their LRU lists
- Fallback to other type when matching pageblocks full (实际上就是划分为一个有一个区域，一个区域使用完成，然后向下一个区域，而不是说都是真的)
- Each marked as MOVABLE, UNMOVABLE or RECLAIMABLE migratetype (there are few more for other purposes
- Zones divided to pageblocks (order-9 = 2MB on x86) 我怀疑是 page block 中间含有两个内容
- Isolates free pages from buddy allocator (splits as needed)

> 解释一下 isolate movable 以及 lru list

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
