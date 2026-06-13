## 基本原则
1. page::_mapcount 初始化为 - 1
2. page_mapcount 出示返回 0, 函数计算中加上了 1
3. 增加一个 mapcount 不会导致 refcount 增加 1 : 这是错误的

## 增加 paging 必然增加 refcount

mm/memory.c 中存在 folio_get 的地方就是证据。

## 增加 mapcount 的位置

这两个在 13900K 静态的环境中，调用次数都非常多，
1. folio_add_new_anon_rmap
2. page_add_file_rmap

```txt
@[
    page_add_file_rmap+5
    do_set_pte+460
    finish_fault+545
    do_fault+798
    __handle_mm_fault+1618
    handle_mm_fault+341
    do_user_addr_fault+561
    exc_page_fault+109
    asm_exc_page_fault+38
]: 3200
```

```txt
@[
    folio_add_new_anon_rmap+5
    do_anonymous_page+755
    __handle_mm_fault+2090
    handle_mm_fault+341
    do_user_addr_fault+342
    exc_page_fault+109
    asm_exc_page_fault+38
]: 2601
```
其实 anon private 的 mapcount 总是 0 所以，在
page_add_file_rmap 是可以看到 mapcount 增加的，但是
folio_add_new_anon_rmap 不会增加。

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


## [ ]  分析下，当决定换出的时候，首先 check 一下 mapcount 和 refcount
- shrink_folio_list : 这里是最终处理的位置


应该就是这部分了:
```c
		/*
		 * If the folio has buffers, try to free the buffer
		 * mappings associated with this folio. If we succeed
		 * we try to free the folio as well.
		 *
		 * We do this even if the folio is dirty.
		 * filemap_release_folio() does not perform I/O, but it
		 * is possible for a folio to have the dirty flag set,
		 * but it is actually clean (all its buffers are clean).
		 * This happens if the buffers were written out directly,
		 * with submit_bh(). ext3 will do this, as well as
		 * the blockdev mapping.  filemap_release_folio() will
		 * discover that cleanness and will drop the buffers
		 * and mark the folio clean - it can be freed.
		 *
		 * Rarely, folios can have buffers and no ->mapping.
		 * These are the folios which were not successfully
		 * invalidated in truncate_cleanup_folio().  We try to
		 * drop those buffers here and if that worked, and the
		 * folio is no longer mapped into process address space
		 * (refcount == 1) it can be freed.  Otherwise, leave
		 * the folio on the LRU so it is swappable.
		 */
		if (folio_has_private(folio)) {
			if (!filemap_release_folio(folio, sc->gfp_mask))
				goto activate_locked;
			if (!mapping && folio_ref_count(folio) == 1) {
				folio_unlock(folio);
				if (folio_put_testzero(folio))
					goto free_it;
				else {
					/*
					 * rare race with speculative reference.
					 * the speculative reference will free
					 * this folio shortly, so we may
					 * increment nr_reclaimed here (and
					 * leave it off the LRU).
					 */
					nr_reclaimed += nr_pages;
					continue;
				}
			}
		}


		if (folio_test_anon(folio) && !folio_test_swapbacked(folio)) {
			/* follow __remove_mapping for reference */
			if (!folio_ref_freeze(folio, 1))
				goto keep_locked;
			/*
			 * The folio has only one reference left, which is
			 * from the isolation. After the caller puts the
			 * folio back on the lru and drops the reference, the
			 * folio will be freed anyway. It doesn't matter
			 * which lru it goes on. So we don't bother checking
			 * the dirty flag here.
			 */
			count_vm_events(PGLAZYFREED, nr_pages);
			count_memcg_folio_events(folio, PGLAZYFREED, nr_pages);
		} else if (!mapping || !__remove_mapping(mapping, folio, true,
							 sc->target_mem_cgroup))
			goto keep_locked;

```

但是不是完全的正确，对于 folio_has_private 的理解不到位啊。
