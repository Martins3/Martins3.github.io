## 基本原则
1. page::_mapcount 初始化为 - 1
2. page_mapcount 出示返回 0, 函数计算中加上了 1
3. 增加一个 mapcount 不会导致 refcount 增加 1 : 这是错误的

## 增加 paging 必然增加 refcount

mm/memory.c 中存在 folio_get 的地方就是证据。

## 如何理解 can_split_folio 中的计数问题

```c
/* Racy check whether the huge page can be split */
bool can_split_folio(struct folio *folio, int *pextra_pins)
{
	int extra_pins;

	/* Additional pins from page cache */
	if (folio_test_anon(folio))
		extra_pins = folio_test_swapcache(folio) ?
				folio_nr_pages(folio) : 0;
	else
		extra_pins = folio_nr_pages(folio);
	if (pextra_pins)
		*pextra_pins = extra_pins;
	return folio_mapcount(folio) == folio_ref_count(folio) - extra_pins - 1;
}
```
其实这就是要求 mapcount 等于 refcount ，因为 refcount 初始化为 1 的

- [ ] 但是为什么 mapcount 等于 refcount 的时候才可以。


try_to_unmap_one 中存在类似逻辑

```c
				/*
				 * The only page refs must be one from isolation
				 * plus the rmap(s) (dropped by discard:).
				 */
				if (ref_count == 1 + map_count &&
				    !folio_test_dirty(folio)) {
					/* Invalidate as we cleared the pte */
					mmu_notifier_invalidate_range(mm,
						address, address + PAGE_SIZE);
					dec_mm_counter(mm, MM_ANONPAGES);
					goto discard;
				}
```
那么就说明，map 一个

### 加入 swap 是必须增加 reference 的
注意看: add_to_swap_cache

```c
	folio_ref_add(folio, nr);
	folio_set_swapcache(folio);
```

- 这个 reference 什么时候去掉?
- 为什么加入到 swapcache 中的时候，需要增加额外的 reference ?

## folio_ref_add 以及 page_ref_add 的位置
加入到 page cache 中的时候一定会成为:

- shmem_add_to_page_cache
- add_to_swap_cache
- `__filemap_add_folio`
- folio_migrate_mapping

## folio_test_anon 指的是 anon private 的吧
是的，从 PAGE_MAPPING_ANON 的定义看上面的注释。

## 如何调试一下各种常见的 page 的 refcount 吧
- 应该可以通过那几个 /proc 接口

### 制作一个 mapcount 远远大约 refcount 的时候


## folio 的 ref count 如何计算的

- shmem_add_to_page_cache 就是 folio 中包含的 page 的个数，而不是一个个的计算的
	- folio_ref_add(folio, nr);


## 可以同时是 map 给 userspace ，但是同时又成为 page cache 吗?


## page ref
- [x] `_refcount` 和 `_mapcount` 的关系是什么 ?

[^27] : Situations where `_count` can exceed `_mapcount` include pages mapped for DMA and pages mapped into the kernel's address space with a function like get_user_pages().
Locking a page into memory with mlock() will also increase `_count`.
The relative value of these two counters is important;
if `_count` equals `_mapcount`, the page can be reclaimed by locating and removing all of the page table entries.
  But if `_count` is greater than `_mapcount`, the page is "pinned" and cannot be reclaimed until the extra references are removed.

- [ ] so every time, we have to increase `_count` and `_mapcount` syncronizely ? That's ugly, there are something uncovered yet!

1. page_ref_sub 调查一下，为什么 swap 会使用这个机制

```c
/*
 * Methods to modify the page usage count.
 *
 * What counts for a page usage:
 * - cache mapping   (page->mapping)
 * - private data    (page->private)
 * - page mapped in a task's page tables, each mapping
 *   is counted separately
 *
 * Also, many kernel routines increase the page count before a critical
 * routine so they can be sure the page doesn't go away from under them.
 */

/*
 * Drop a ref, return true if the refcount fell to zero (the page has no users)
 */
static inline int put_page_testzero(struct page *page)
{
  VM_BUG_ON_PAGE(page_ref_count(page) == 0, page);
  return page_ref_dec_and_test(page);
}

/*
 * Try to grab a ref unless the page has a refcount of zero, return false if
 * that is the case.
 * This can be called when MMU is off so it must not access
 * any of the virtual mappings.
 */
static inline int get_page_unless_zero(struct page *page)
{
  return page_ref_add_unless(page, 1, 0);
}
```

- [ ] understand this function and it's reference
```c
static bool is_refcount_suitable(struct page *page)
{
  int expected_refcount;

  expected_refcount = total_mapcount(page);
  if (PageSwapCache(page))
    expected_refcount += compound_nr(page);

  return page_count(page) == expected_refcount;
}
```

- [ ] put_page : rather difficult than expected

1. `_mapcount` 是在 union 中间，当该页面给用户使用的时候，才有意义

## 原来 thp 的 reference counting 是一个专门的问题
https://lwn.net/Articles/619738/

[^27]: https://lwn.net/Articles/619738/
