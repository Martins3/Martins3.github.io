# pagefault

主要分析 mm/memory.c 中的内容

```txt
@[
    handle_mm_fault+5
    do_user_addr_fault+464
    exc_page_fault+107
    asm_exc_page_fault+38
]: 72
```
- handle_page_fault
  - do_kern_addr_fault
  - do_user_addr_fault


- handle_mm_fault
  - hugetlb_fault : 如果缺页是大页
  - `__handle_mm_fault` : 逐级向下，但是需要考虑是不是透明大页
    - create_huge_pud
    - wp_huge_pud
    - create_huge_pmd
    - wp_huge_pmd
    - handle_pte_fault
      - do_pte_missing
        - do_anonymous_page : 处理匿名映射
        - do_fault : 处理文件映射
          - do_read_fault
          - do_cow_fault : 如果 vma 不是共享的
          - do_shared_fault
      - do_swap_page : 主要处理被换出的页面
      - do_numa_page : numa 之前的迁移

- do_read_fault / do_cow_fault / do_shared_fault
  - `__do_fault`
    - vma->vm_ops->fault(vmf);
      - filemap_fault : 一般注册的是这个
        - filemap_get_folio : 询问 page cache
        - folio_file_page : 访问文件系统之类的
  - finish_fault
    - do_set_pte : 设置上 pte

- finish_fault
  - do_set_pte
    - page_add_file_rmap : 完全无法理解


## KeyNote
1. swapbacked 似乎用于表示 : 这个 page 是 anon vma 中间的，所有 anon vma 中间 page 都是需要走这一个(tmpfs 暂时不知道怎么回事)，
    page_add_new_anon_rmap(page, vma, vmf->address, false); // 无条件调用 `__SetPageSwapBacked`
2. 相对比的是 : PG_swapcache 表示当前 page 在 swap cache 中间，也即是当前的 page 在 radix tree 中间的
    1. PageSwapCache 函数 : 也可以说明，必须首先是 anon vma 中间的 page ，然后才可以是 swap cache 的内容
    2. PageAnon 和 PageSwapBacked 有什么区别吗 ?

## 问题
2. 检查一下使用 pte_same 的位置，是不是为了处理 page table 上锁的情况 ?
3. pte_mkdirty pte_mkyoung
4. pte_lockptr


- cow 如何处理这种场景:
例如 anon vma 连续 fork 1000 个 child，然后修改 page ，因为 child 需要观察到原来的 page 的样子，称之为 page A
此时触发 page fault，
将会产生一个新的 page B，那么是不是要拷贝 1000 个 page A 出来给 child 使用


## handle_pte_fault : 万恶之源

2. vmf 的中间的每一个参数的含义是什么 ?
    1. orig_pte : 当发生 pgfault 的时候的 pte, hardware 如何对其初始化的
    2. pte : 那我是什么 ?
    3. flags :
    4. address : 这个在 需要被 `__page_set_anon_rmap` 用于设置 `page->index`

## do_swap_page

- [ ] 其中调用的 migration_entry_wait 和 do_numa_page 什么关系?

```c
/*
 * We enter with non-exclusive mmap_lock (to exclude vma changes,
 * but allow concurrent faults), and pte mapped but not yet locked.
 * We return with pte unmapped and unlocked.
 *
 * We return with the mmap_lock locked or unlocked in the same cases
 * as does filemap_fault().
 */
vm_fault_t do_swap_page(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct folio *swapcache, *folio = NULL;
	struct page *page;
	struct swap_info_struct *si = NULL;
	rmap_t rmap_flags = RMAP_NONE;
	bool exclusive = false;
	swp_entry_t entry;
	pte_t pte;
	int locked;
	vm_fault_t ret = 0;
	void *shadow = NULL;

	if (!pte_unmap_same(vmf))
		goto out;

	if (vmf->flags & FAULT_FLAG_VMA_LOCK) {
		ret = VM_FAULT_RETRY;
		goto out;
	}

	entry = pte_to_swp_entry(vmf->orig_pte);
	if (unlikely(non_swap_entry(entry))) {                   // 不仅仅 swap 还有其他情况
		if (is_migration_entry(entry)) {
			migration_entry_wait(vma->vm_mm, vmf->pmd,
					     vmf->address);
		} else if (is_device_exclusive_entry(entry)) {
			vmf->page = pfn_swap_entry_to_page(entry);
			ret = remove_device_exclusive_entry(vmf);
		} else if (is_device_private_entry(entry)) {
			vmf->page = pfn_swap_entry_to_page(entry);
			vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd,
					vmf->address, &vmf->ptl);
			if (unlikely(!pte_same(*vmf->pte, vmf->orig_pte))) {
				spin_unlock(vmf->ptl);
				goto out;
			}

			/*
			 * Get a page reference while we know the page can't be
			 * freed.
			 */
			get_page(vmf->page);
			pte_unmap_unlock(vmf->pte, vmf->ptl);
			ret = vmf->page->pgmap->ops->migrate_to_ram(vmf);
			put_page(vmf->page);
		} else if (is_hwpoison_entry(entry)) {
			ret = VM_FAULT_HWPOISON;
		} else if (is_pte_marker_entry(entry)) {
			ret = handle_pte_marker(vmf);
		} else {
			print_bad_pte(vma, vmf->address, vmf->orig_pte, NULL);
			ret = VM_FAULT_SIGBUS;
		}
		goto out;
	}

	/* Prevent swapoff from happening to us. */
	si = get_swap_device(entry);
	if (unlikely(!si))
		goto out;

	folio = swap_cache_get_folio(entry, vma, vmf->address); // 查询 swap cache : 位于 swap_state.c
	if (folio)
		page = folio_file_page(folio, swp_offset(entry));
	swapcache = folio;

	if (!folio) {
		if (data_race(si->flags & SWP_SYNCHRONOUS_IO) &&    // 如果只是被一个 process 映射，并且必须同步 IO ，那么就不使用 swap cache
		    __swap_count(entry) == 1) {                     // 因为 swapin_readahead 是会处理 cache 的
			/* skip swapcache */
			folio = vma_alloc_folio(GFP_HIGHUSER_MOVABLE, 0,
						vma, vmf->address, false);
			page = &folio->page;
			if (folio) {
				__folio_set_locked(folio);
				__folio_set_swapbacked(folio);

				if (mem_cgroup_swapin_charge_folio(folio,
							vma->vm_mm, GFP_KERNEL,
							entry)) {
					ret = VM_FAULT_OOM;
					goto out_page;
				}
				mem_cgroup_swapin_uncharge_swap(entry);

				shadow = get_shadow_from_swap_cache(entry);
				if (shadow)
					workingset_refault(folio, shadow);

				folio_add_lru(folio);

				/* To provide entry to swap_readpage() */
				folio_set_swap_entry(folio, entry);
				swap_readpage(page, true, NULL);
				folio->private = NULL;
			}
		} else {
			page = swapin_readahead(entry, GFP_HIGHUSER_MOVABLE, // 会经过 swap cache 的
						vmf);
			if (page)
				folio = page_folio(page);
			swapcache = folio;
		}

		if (!folio) {
			/*
			 * Back out if somebody else faulted in this pte
			 * while we released the pte lock.
			 */
			vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd,
					vmf->address, &vmf->ptl);
			if (likely(pte_same(*vmf->pte, vmf->orig_pte)))
				ret = VM_FAULT_OOM;
			goto unlock;
		}

		/* Had to read the page from swap area: Major fault */
		ret = VM_FAULT_MAJOR;
		count_vm_event(PGMAJFAULT);
		count_memcg_event_mm(vma->vm_mm, PGMAJFAULT);
	} else if (PageHWPoison(page)) {
		/*
		 * hwpoisoned dirty swapcache pages are kept for killing
		 * owner processes (which may be unknown at hwpoison time)
		 */
		ret = VM_FAULT_HWPOISON;
		goto out_release;
	}

	locked = folio_lock_or_retry(folio, vma->vm_mm, vmf->flags);

	if (!locked) {
		ret |= VM_FAULT_RETRY;
		goto out_release;
	}

	if (swapcache) {
		/*
		 * Make sure folio_free_swap() or swapoff did not release the
		 * swapcache from under us.  The page pin, and pte_same test
		 * below, are not enough to exclude that.  Even if it is still
		 * swapcache, we need to check that the page's swap has not
		 * changed.
		 */
		if (unlikely(!folio_test_swapcache(folio) ||
			     page_private(page) != entry.val))
			goto out_page;

		/*
		 * KSM sometimes has to copy on read faults, for example, if
		 * page->index of !PageKSM() pages would be nonlinear inside the
		 * anon VMA -- PageKSM() is lost on actual swapout.
		 */
		page = ksm_might_need_to_copy(page, vma, vmf->address);
		if (unlikely(!page)) {
			ret = VM_FAULT_OOM;
			goto out_page;
		} else if (unlikely(PTR_ERR(page) == -EHWPOISON)) {
			ret = VM_FAULT_HWPOISON;
			goto out_page;
		}
		folio = page_folio(page);

		/*
		 * If we want to map a page that's in the swapcache writable, we
		 * have to detect via the refcount if we're really the exclusive
		 * owner. Try removing the extra reference from the local LRU
		 * pagevecs if required.
		 */
		if ((vmf->flags & FAULT_FLAG_WRITE) && folio == swapcache &&
		    !folio_test_ksm(folio) && !folio_test_lru(folio))
			lru_add_drain();
	}

	folio_throttle_swaprate(folio, GFP_KERNEL);

	/*
	 * Back out if somebody else already faulted in this pte.
	 */
	vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd, vmf->address,
			&vmf->ptl);
	if (unlikely(!pte_same(*vmf->pte, vmf->orig_pte)))
		goto out_nomap;

	if (unlikely(!folio_test_uptodate(folio))) { // 一般是因为 swap device 出现故障了
		ret = VM_FAULT_SIGBUS;
		goto out_nomap;
	}

	/*
	 * PG_anon_exclusive reuses PG_mappedtodisk for anon pages. A swap pte
	 * must never point at an anonymous page in the swapcache that is
	 * PG_anon_exclusive. Sanity check that this holds and especially, that
	 * no filesystem set PG_mappedtodisk on a page in the swapcache. Sanity
	 * check after taking the PT lock and making sure that nobody
	 * concurrently faulted in this page and set PG_anon_exclusive.
	 */
	BUG_ON(!folio_test_anon(folio) && folio_test_mappedtodisk(folio));
	BUG_ON(folio_test_anon(folio) && PageAnonExclusive(page));

	/*
	 * Check under PT lock (to protect against concurrent fork() sharing
	 * the swap entry concurrently) for certainly exclusive pages.
	 */
	if (!folio_test_ksm(folio)) {
		exclusive = pte_swp_exclusive(vmf->orig_pte);
		if (folio != swapcache) {
			/*
			 * We have a fresh page that is not exposed to the
			 * swapcache -> certainly exclusive.
			 */
			exclusive = true;
		} else if (exclusive && folio_test_writeback(folio) &&
			  data_race(si->flags & SWP_STABLE_WRITES)) {
			/*
			 * This is tricky: not all swap backends support
			 * concurrent page modifications while under writeback.
			 *
			 * So if we stumble over such a page in the swapcache
			 * we must not set the page exclusive, otherwise we can
			 * map it writable without further checks and modify it
			 * while still under writeback.
			 *
			 * For these problematic swap backends, simply drop the
			 * exclusive marker: this is perfectly fine as we start
			 * writeback only if we fully unmapped the page and
			 * there are no unexpected references on the page after
			 * unmapping succeeded. After fully unmapped, no
			 * further GUP references (FOLL_GET and FOLL_PIN) can
			 * appear, so dropping the exclusive marker and mapping
			 * it only R/O is fine.
			 */
			exclusive = false;
		}
	}

	/*
	 * Remove the swap entry and conditionally try to free up the swapcache.
	 * We're already holding a reference on the page but haven't mapped it
	 * yet.
	 */
	swap_free(entry); // 加载了一个 page 进来，那么对应的 swap 空间就可以释放了
	if (should_try_to_free_swap(folio, vma, vmf->flags)) // 如果可以释放 swap cache 的空间
		folio_free_swap(folio);

	inc_mm_counter(vma->vm_mm, MM_ANONPAGES);
	dec_mm_counter(vma->vm_mm, MM_SWAPENTS);
	pte = mk_pte(page, vma->vm_page_prot);

	/*
	 * Same logic as in do_wp_page(); however, optimize for pages that are
	 * certainly not shared either because we just allocated them without
	 * exposing them to the swapcache or because the swap entry indicates
	 * exclusivity.
	 */
	if (!folio_test_ksm(folio) &&
	    (exclusive || folio_ref_count(folio) == 1)) {
		if (vmf->flags & FAULT_FLAG_WRITE) {
			pte = maybe_mkwrite(pte_mkdirty(pte), vma);
			vmf->flags &= ~FAULT_FLAG_WRITE;
		}
		rmap_flags |= RMAP_EXCLUSIVE;
	}
	flush_icache_page(vma, page);
	if (pte_swp_soft_dirty(vmf->orig_pte))
		pte = pte_mksoft_dirty(pte);
	if (pte_swp_uffd_wp(vmf->orig_pte))
		pte = pte_mkuffd_wp(pte);
	vmf->orig_pte = pte;

	/* ksm created a completely new copy */
	if (unlikely(folio != swapcache && swapcache)) {
		page_add_new_anon_rmap(page, vma, vmf->address);
		folio_add_lru_vma(folio, vma);
	} else {
		page_add_anon_rmap(page, vma, vmf->address, rmap_flags);
	}

	VM_BUG_ON(!folio_test_anon(folio) ||
			(pte_write(pte) && !PageAnonExclusive(page)));
	set_pte_at(vma->vm_mm, vmf->address, vmf->pte, pte); // 更新 page table
	arch_do_swap_page(vma->vm_mm, vma, vmf->address, pte, vmf->orig_pte);

	folio_unlock(folio);
	if (folio != swapcache && swapcache) {
		/*
		 * Hold the lock to avoid the swap entry to be reused
		 * until we take the PT lock for the pte_same() check
		 * (to avoid false positives from pte_same). For
		 * further safety release the lock after the swap_free
		 * so that the swap count won't change under a
		 * parallel locked swapcache.
		 */
		folio_unlock(swapcache);
		folio_put(swapcache);
	}

	if (vmf->flags & FAULT_FLAG_WRITE) {
		ret |= do_wp_page(vmf);
		if (ret & VM_FAULT_ERROR)
			ret &= VM_FAULT_ERROR;
		goto out;
	}

	/* No need to invalidate - it was non-present before */
	update_mmu_cache(vma, vmf->address, vmf->pte);
unlock:
	pte_unmap_unlock(vmf->pte, vmf->ptl);
out:
	if (si)
		put_swap_device(si);
	return ret;
out_nomap:
	pte_unmap_unlock(vmf->pte, vmf->ptl);
out_page:
	folio_unlock(folio);
out_release:
	folio_put(folio);
	if (folio != swapcache && swapcache) {
		folio_unlock(swapcache);
		folio_put(swapcache);
	}
	if (si)
		put_swap_device(si);
	return ret;
}
```

## do_wp_page
1. do_swap_page 为什么会调用这一个 ?
2. 和 do_cow_fault 含有根本的区别 : do_cow_fault 仅仅限于处理 mmap file 中间的 cow，而且是首次遇到 ，而 do_wp_page 一般的 cow

3. 猜测实现:
    1. 复制 pte : 整个 page walk
    2. 去掉 flag
    3. 分配新的 page , 拷贝原来的 page
    4. rmap

4. 实际上:
    1. struct page 也需要拷贝

```c
/*
 * This routine handles present pages, when users try to write
 * to a shared page. It is done by copying the page to a new address
 * and decrementing the shared-page counter for the old page.
 *
 * Note that this routine assumes that the protection checks have been
 * done by the caller (the low-level page fault routine in most cases).
 * Thus we can safely just mark it writable once we've done any necessary
 * COW.
 *
 * We also mark the page dirty at this point even though the page will
 * change only once the write actually happens. This avoids a few races,
 * and potentially makes it more efficient.
 *
 * We enter with non-exclusive mmap_sem (to exclude vma changes,
 * but allow concurrent faults), with pte both mapped and locked.
 * We return with mmap_sem still held, but pte unmapped and unlocked.
 */
static vm_fault_t do_wp_page(struct vm_fault *vmf)
	__releases(vmf->ptl)
{
	struct vm_area_struct *vma = vmf->vma;

	vmf->page = vm_normal_page(vma, vmf->address, vmf->orig_pte); // 不考虑各种射频情况，等价于通过 pte_pfn pfn_to_page 将 vmf->orig_pte 装换得到 page
	if (!vmf->page) { // 暂时不用考虑
		/*
		 * VM_MIXEDMAP !pfn_valid() case, or VM_SOFTDIRTY clear on a
		 * VM_PFNMAP VMA.
		 *
		 * We should not cow pages in a shared writeable mapping.
		 * Just mark the pages writable and/or call ops->pfn_mkwrite.
		 */
		if ((vma->vm_flags & (VM_WRITE|VM_SHARED)) ==
				     (VM_WRITE|VM_SHARED))
			return wp_pfn_shared(vmf);

		pte_unmap_unlock(vmf->pte, vmf->ptl);
		return wp_page_copy(vmf);
	}

	/*
	 * Take out anonymous pages first, anonymous shared vmas are
	 * not dirty accountable.
	 */
	if (PageAnon(vmf->page)) {
		int total_map_swapcount;
		if (PageKsm(vmf->page) && (PageSwapCache(vmf->page) ||
					   page_count(vmf->page) != 1)) // todo page_count != 1 表示多人共享的，所以下面的一大段的内容其实是表示，当这个page 无人使用的时候，直接复用
			goto copy;
		if (!trylock_page(vmf->page)) { // 如果没有成功上锁
			get_page(vmf->page);
			pte_unmap_unlock(vmf->pte, vmf->ptl);
			lock_page(vmf->page); // 上锁，sleep ，直到锁被打开
			vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd,
					vmf->address, &vmf->ptl); // vmf->pte 保存的是指向 pte 的地址，而 orig_pte 是硬件告知的
			if (!pte_same(*vmf->pte, vmf->orig_pte)) { // todo 是不是在说明在进行处理的过程中间，这个 entry 已经被修改了，所以，我们可以直接退出了。
				unlock_page(vmf->page);
				pte_unmap_unlock(vmf->pte, vmf->ptl);
				put_page(vmf->page);
				return 0;
			}
			put_page(vmf->page);
		}
		if (reuse_swap_page(vmf->page, &total_map_swapcount)) {
			if (total_map_swapcount == 1) {
				/*
				 * The page is all ours. Move it to
				 * our anon_vma so the rmap code will
				 * not search our parent or siblings.
				 * Protected against the rmap code by
				 * the page lock.
				 */
				page_move_anon_rmap(vmf->page, vma);
			}
			unlock_page(vmf->page);
			wp_page_reuse(vmf);
			return VM_FAULT_WRITE;
		}
		unlock_page(vmf->page);
	} else if (unlikely((vma->vm_flags & (VM_WRITE|VM_SHARED)) == // todo 不能理解，既然出现在 vm flags 表示是 WRITE && SHARED，当初的 pte 的 flags 为什么不是 wirtable 的 ?
					(VM_WRITE|VM_SHARED))) {
		return wp_page_shared(vmf); // 之所以出现 pte 和 vma 的权限不一致， 我猜测其中的原因是 : vma 的权限是可以动态修改的，比如 mprotect, 分析代码，此处完成的工作也非常的简单，通知一下 我要 dirty 一个 RO page 了　todo 等待验证
	}
copy:
	/*
	 * Ok, we need to copy. Oh, well..
	 */
	get_page(vmf->page);

	pte_unmap_unlock(vmf->pte, vmf->ptl);
	return wp_page_copy(vmf); // todo 估计不麻烦，除了 统计 和 PTL lock 的
}
```

## do_anonymous_page

```c
/*
 * We enter with non-exclusive mmap_sem (to exclude vma changes,
 * but allow concurrent faults), and pte mapped but not yet locked.
 * We return with mmap_sem still held, but pte unmapped and unlocked.
 */
static vm_fault_t do_anonymous_page(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct mem_cgroup *memcg;
	struct page *page;
	vm_fault_t ret = 0;
	pte_t entry;

	/* File mapping without ->vm_ops ? */
	if (vma->vm_flags & VM_SHARED) // anon vma 是不是永远不会持有 VM_SHARED
		return VM_FAULT_SIGBUS;

	/*
	 * Use pte_alloc() instead of pte_alloc_map().  We can't run
	 * pte_offset_map() on pmds where a huge pmd might be created
	 * from a different thread.
	 *
	 * pte_alloc_map() is safe to use under down_write(mmap_sem) or when
	 * parallel threads are excluded by other means.
	 *
	 * Here we only have down_read(mmap_sem).
	 */
	if (pte_alloc(vma->vm_mm, vmf->pmd)) // 用于分配 page table
		return VM_FAULT_OOM;

	/* See the comment in pte_alloc_one_map() */
	if (unlikely(pmd_trans_unstable(vmf->pmd)))
		return 0;

	/* Use the zero-page for reads */
	if (!(vmf->flags & FAULT_FLAG_WRITE) && // read 的时候总是 zero-page , todo 可以关注一下 zero page 在 anon vma 的 read 操作中间的含义是什么 ? 之后对于此处写，怎么办 ? 再次触发 page fault 吗 ?
			!mm_forbids_zeropage(vma->vm_mm)) { // mm_forbids_zeropage 不用在乎
		entry = pte_mkspecial(pfn_pte(my_zero_pfn(vmf->address),
						vma->vm_page_prot));
		vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd,
				vmf->address, &vmf->ptl);
		if (!pte_none(*vmf->pte)) // 之所以可以到达此处，是因为 vmf->pte 的内容为 null ，如果 !pte_none ，说明其他的地方对于此处发生过修改
			goto unlock;
		ret = check_stable_address_space(vma->vm_mm);
		if (ret)
			goto unlock;
		/* Deliver the page fault to userland, check inside PT lock */
		if (userfaultfd_missing(vma)) { // 当前 config 总是 false
			pte_unmap_unlock(vmf->pte, vmf->ptl);
			return handle_userfault(vmf, VM_UFFD_MISSING);
		}
		goto setpte; // XXX 任务完成
	}

	/* Allocate our own private page. */
	if (unlikely(anon_vma_prepare(vma))) // todo 细节
		goto oom;
	page = alloc_zeroed_user_highpage_movable(vma, vmf->address); // todo 也是创建一个 page ，这个函数有什么神奇的地方吗?
	if (!page)
		goto oom;

	if (mem_cgroup_try_charge_delay(page, vma->vm_mm, GFP_KERNEL, &memcg,
					false))
		goto oom_free_page;

	/*
	 * The memory barrier inside __SetPageUptodate makes sure that
	 * preceding stores to the page contents become visible before
	 * the set_pte_at() write.
	 */
	__SetPageUptodate(page);

	entry = mk_pte(page, vma->vm_page_prot);
	if (vma->vm_flags & VM_WRITE)
		entry = pte_mkwrite(pte_mkdirty(entry));

	vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd, vmf->address,
			&vmf->ptl);
	if (!pte_none(*vmf->pte))
		goto release;

	ret = check_stable_address_space(vma->vm_mm);
	if (ret)
		goto release;

	/* Deliver the page fault to userland, check inside PT lock */
	if (userfaultfd_missing(vma)) {
		pte_unmap_unlock(vmf->pte, vmf->ptl);
		mem_cgroup_cancel_charge(page, memcg, false);
		put_page(page);
		return handle_userfault(vmf, VM_UFFD_MISSING);
	}

	inc_mm_counter_fast(vma->vm_mm, MM_ANONPAGES);
	page_add_new_anon_rmap(page, vma, vmf->address, false); // 无条件调用 __SetPageSwapBacked
	mem_cgroup_commit_charge(page, memcg, false, false);
	lru_cache_add_active_or_unevictable(page, vma);
setpte:
	set_pte_at(vma->vm_mm, vmf->address, vmf->pte, entry);

	/* No need to invalidate - it was non-present before */
	update_mmu_cache(vma, vmf->address, vmf->pte);
unlock:
	pte_unmap_unlock(vmf->pte, vmf->ptl);
	return ret;
release:
	mem_cgroup_cancel_charge(page, memcg, false);
	put_page(page);
	goto unlock;
oom_free_page:
	put_page(page);
oom:
	return VM_FAULT_OOM;
}
```

## do_fault
1. 需要提供所在的 inode
    1. 建立 ramp
    2. 读取文件
    3. readahead 操作
    4. 统计信息

```c
/*
 * We enter with non-exclusive mmap_sem (to exclude vma changes,
 * but allow concurrent faults).
 * The mmap_sem may have been released depending on flags and our
 * return value.  See filemap_fault() and __lock_page_or_retry().
 * If mmap_sem is released, vma may become invalid (for example
 * by other thread calling munmap()).
 */
static vm_fault_t do_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct mm_struct *vm_mm = vma->vm_mm;
	vm_fault_t ret;

	/*
	 * The VMA was not fully populated on mmap() or missing VM_DONTEXPAND
	 */
	if (!vma->vm_ops->fault) { // todo 什么叫做 not fully populated on mmap())
		/*
		 * If we find a migration pmd entry or a none pmd entry, which
		 * should never happen, return SIGBUS
		 */
		if (unlikely(!pmd_present(*vmf->pmd))) // todo 为什么单单 pmd 和 migration 有关的
			ret = VM_FAULT_SIGBUS;
		else {
			vmf->pte = pte_offset_map_lock(vmf->vma->vm_mm, // 获取 pte 所在的地址，具体作用不知道
						       vmf->pmd,
						       vmf->address,
						       &vmf->ptl);
			/*
			 * Make sure this is not a temporary clearing of pte
			 * by holding ptl and checking again. A R/M/W update
			 * of pte involves: take ptl, clearing the pte so that
			 * we don't have concurrent modification by hardware
			 * followed by an update.
			 */
			if (unlikely(pte_none(*vmf->pte)))
				ret = VM_FAULT_SIGBUS;
			else
				ret = VM_FAULT_NOPAGE;

			pte_unmap_unlock(vmf->pte, vmf->ptl);
		}
	} else if (!(vmf->flags & FAULT_FLAG_WRITE))
		ret = do_read_fault(vmf); // 最简单的内容，读取
	else if (!(vma->vm_flags & VM_SHARED))
		ret = do_cow_fault(vmf); // 复制一份
	else
		ret = do_shared_fault(vmf); // 所有人可见的shared

	/* preallocated pagetable is unused: free it */
	if (vmf->prealloc_pte) {
		pte_free(vm_mm, vmf->prealloc_pte);
		vmf->prealloc_pte = NULL;
	}
	return ret;
}
```

1. 获取 page

```c
/*
 * The mmap_sem must have been held on entry, and may have been
 * released depending on flags and vma->vm_ops->fault() return value.
 * See filemap_fault() and __lock_page_retry().
 */
static vm_fault_t __do_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	vm_fault_t ret;

	/*
	 * Preallocate pte before we take page_lock because this might lead to
	 * deadlocks for memcg reclaim which waits for pages under writeback:
	 *				lock_page(A)
	 *				SetPageWriteback(A)
	 *				unlock_page(A)
	 * lock_page(B)
	 *				lock_page(B)
	 * pte_alloc_pne
	 *   shrink_page_list
	 *     wait_on_page_writeback(A)
	 *				SetPageWriteback(B)
	 *				unlock_page(B)
	 *				# flush A, B to clear the writeback
	 */
	if (pmd_none(*vmf->pmd) && !vmf->prealloc_pte) {
		vmf->prealloc_pte = pte_alloc_one(vmf->vma->vm_mm); // 获取 pte
		if (!vmf->prealloc_pte)
			return VM_FAULT_OOM;
		smp_wmb(); /* See comment in __pte_alloc() */
	}

	ret = vma->vm_ops->fault(vmf); // 利用 vma 当初注册的 io 函数进行读入
	if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY |
			    VM_FAULT_DONE_COW)))
		return ret;

	if (unlikely(!(ret & VM_FAULT_LOCKED)))
		lock_page(vmf->page);
	else
		VM_BUG_ON_PAGE(!PageLocked(vmf->page), vmf->page);
	return ret;
}


vm_fault_t ext4_filemap_fault(struct vm_fault *vmf) // 使用 ext4 作为例子讲解，其他的函数也值得分析
{
	struct inode *inode = file_inode(vmf->vma->vm_file);
	vm_fault_t ret;

	down_read(&EXT4_I(inode)->i_mmap_sem);
	ret = filemap_fault(vmf); // 到 filemap.md 中间查找吧!
	up_read(&EXT4_I(inode)->i_mmap_sem);
	return ret;
}
```

2. 完成 page 和 pte 的映射，并且进入到 rmap 中间
```c
/**
 * finish_fault - finish page fault once we have prepared the page to fault
 *
 * @vmf: structure describing the fault
 *
 * This function handles all that is needed to finish a page fault once the
 * page to fault in is prepared. It handles locking of PTEs, inserts PTE for
 * given page, adds reverse page mapping, handles memcg charges and LRU
 * addition.
 *
 * The function expects the page to be locked and on success it consumes a
 * reference of a page being mapped (for the PTE which maps it).
 *
 * Return: %0 on success, %VM_FAULT_ code in case of error.
 */
vm_fault_t finish_fault(struct vm_fault *vmf)
{
	struct page *page;
	vm_fault_t ret = 0;

	/* Did we COW the page? */
	if ((vmf->flags & FAULT_FLAG_WRITE) &&
	    !(vmf->vma->vm_flags & VM_SHARED)) // COW 的含义非常清晰 : 写，同时对于别人不可见
		page = vmf->cow_page; // 虽然 __do_fault 从 page cache 中间读取的时候，会创建一个 page ，但是用于加入到 page table 的 cow 哪一个
	else
		page = vmf->page;

	/*
	 * check even for read faults because we might have lost our CoWed
	 * page
	 */
	if (!(vmf->vma->vm_flags & VM_SHARED))
		ret = check_stable_address_space(vmf->vma->vm_mm); // 暂时不用考虑，当做 ret = 0
	if (!ret)
		ret = alloc_set_pte(vmf, vmf->memcg, page);
	if (vmf->pte)
		pte_unmap_unlock(vmf->pte, vmf->ptl);
	return ret;
}
```

三个函数功能的猜测:
1. 首先至少都是需要完成读入操作吧，读入的到 page cache 中间里面去 (正确)
2. do_shared_fault 似乎不用做任何事情吧，相对于 do_read_fault (正确)
3. cow 会分配两次 page frame ，一次在 page cache 中间，一次自己使，(正确)

实际: 全部调用
1. `__do_fault` : 到 page cache 中间获取 page
2. finish_fault : page 和 pte 挂钩工作

#### (3) do_read_fault

```c

static vm_fault_t do_read_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	vm_fault_t ret = 0;

	/*
	 * Let's call ->map_pages() first and use ->fault() as fallback
	 * if page by the offset is not ready to be mapped (cold cache or
	 * something).
	 */
	if (vma->vm_ops->map_pages && fault_around_bytes >> PAGE_SHIFT > 1) {
		ret = do_fault_around(vmf);
		if (ret)
			return ret;
	}

	ret = __do_fault(vmf);
	if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY)))
		return ret;

	ret |= finish_fault(vmf);
	unlock_page(vmf->page);
	if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY)))
		put_page(vmf->page);
	return ret;
}
```


#### (2) do_cow_fault
```c
static vm_fault_t do_cow_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	vm_fault_t ret;

	if (unlikely(anon_vma_prepare(vma)))
		return VM_FAULT_OOM;

	vmf->cow_page = alloc_page_vma(GFP_HIGHUSER_MOVABLE, vma, vmf->address); // 分配 cow page  todo 所以，为什么需要使用这一个函数进行分配啊 !
	if (!vmf->cow_page)
		return VM_FAULT_OOM;

	ret = __do_fault(vmf);
	if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY)))
		goto uncharge_out;
	if (ret & VM_FAULT_DONE_COW)
		return ret;

	copy_user_highpage(vmf->cow_page, vmf->page, vmf->address, vma); // 将 page 拷贝到 cow_page 中间 todo 但是其中的实现，不是非常理解
	__SetPageUptodate(vmf->cow_page); // todo 用于都不知道 PageUptodate 之类的 flag 什么时候插上去

	ret |= finish_fault(vmf);
	unlock_page(vmf->page);
	put_page(vmf->page);
	if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY)))
		goto uncharge_out;
	return ret;
uncharge_out:
	mem_cgroup_cancel_charge(vmf->cow_page, vmf->memcg, false);
	put_page(vmf->cow_page);
	return ret;
}
```


#### (1) do_shared_fault

```c
static vm_fault_t do_shared_fault(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	vm_fault_t ret, tmp;

	ret = __do_fault(vmf);
	if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE | VM_FAULT_RETRY)))
		return ret;

	/*
	 * Check if the backing address space wants to know that the page is
	 * about to become writable
	 */
	if (vma->vm_ops->page_mkwrite) { // todo 相对于 do_read_fault 多出来的内容，其实不是必须的，只是做一些通知工作而已。
		unlock_page(vmf->page);
		tmp = do_page_mkwrite(vmf); // 神奇啊 !
		if (unlikely(!tmp ||
				(tmp & (VM_FAULT_ERROR | VM_FAULT_NOPAGE)))) {
			put_page(vmf->page);
			return tmp;
		}
	}

	ret |= finish_fault(vmf);
	if (unlikely(ret & (VM_FAULT_ERROR | VM_FAULT_NOPAGE |
					VM_FAULT_RETRY))) {
		unlock_page(vmf->page);
		put_page(vmf->page);
		return ret;
	}

	ret |= fault_dirty_shared_page(vmf);
	return ret;
}
```

## fault_dirty_shared_page : do_shared_fault 最后调用
1. 和 page-writeback.c 有关
```c
/*
 * Handle dirtying of a page in shared file mapping on a write fault.
 *
 * The function expects the page to be locked and unlocks it.
 */
static vm_fault_t fault_dirty_shared_page(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct address_space *mapping;
	struct page *page = vmf->page;
	bool dirtied;
	bool page_mkwrite = vma->vm_ops && vma->vm_ops->page_mkwrite; // todo 检查 page_mkwrite 的作用是什么 ?

	dirtied = set_page_dirty(page); // todo
	VM_BUG_ON_PAGE(PageAnon(page), page);
	/*
	 * Take a local copy of the address_space - page.mapping may be zeroed
	 * by truncate after unlock_page().   The address_space itself remains
	 * pinned by vma->vm_file's reference.  We rely on unlock_page()'s
	 * release semantics to prevent the compiler from undoing this copying.
	 */
	mapping = page_rmapping(page); // todo 神奇的注释
	unlock_page(page);

	if (!page_mkwrite)
		file_update_time(vma->vm_file);

	/*
	 * Throttle page dirtying rate down to writeback speed.
	 *
	 * mapping may be NULL here because some device drivers do not
	 * set page.mapping but still dirty their pages
	 *
	 * Drop the mmap_sem before waiting on IO, if we can. The file
	 * is pinning the mapping, as per above.
	 */
	if ((dirtied || page_mkwrite) && mapping) {
		struct file *fpin;

		fpin = maybe_unlock_mmap_for_io(vmf, NULL);
		balance_dirty_pages_ratelimited(mapping); // todo 神奇
		if (fpin) {
			fput(fpin);
			return VM_FAULT_RETRY;
		}
	}

	return 0;
}
```

## page fault
- [ ] vmf_insert_pfn : 给驱动使用的直接，在 vma 连接 va 和 pa

[TO BE CONTINUE](https://www.cnblogs.com/LoyenWang/p/12116570.html), this is a awesome post.

handle_pte_fault 的调用路径图:
1. do_anonymous_page : anon page
2. do_fault : 和 file 相关的
5. do_numa_page
3. do_swap_page
    4. do_wp_page : 如果是 cow 一个在 disk 的 page ，其实不能理解，如果 cow ，那么为什么不是直接复制 swp_entry_t ，为什么还会有别的蛇皮东西 !
    2. @todo 由于 cow 机制的存在, 岂不是需要将所有的 pte 全部标记一遍，找到证据!
4. do_wp_page


- [ ] `enum vm_fault_reason` : check it's entry one by one

- [ ] I guess the only user of `struct vm_operations_struct` is page fault

```c
static const struct vm_operations_struct xfs_file_vm_ops = {
  .fault    = xfs_filemap_fault,
  .huge_fault = xfs_filemap_huge_fault,
  .map_pages  = xfs_filemap_map_pages,
  .page_mkwrite = xfs_filemap_page_mkwrite,
  .pfn_mkwrite  = xfs_filemap_pfn_mkwrite,
};
```

#### page table
- [ ] https://stackoverflow.com/questions/32943129/how-does-arm-linux-emulate-the-dirty-accessed-and-file-bits-of-a-pte


#### paging
> 准备知识
- [todo 首先深入理解 x86 paging 机制](https://cirosantilli.com/x86-paging)
- [todo](https://0xax.gitbooks.io/linux-insides/content/Theory/linux-theory-1.html)
- [todo](https://stackoverflow.com/questions/12557267/linux-kernel-memory-management-paging-levels)

A. 到底存在多少级 ?
arch/x86/include/asm/pgtable_types.h
一共 5 级，每一级的作用都是相同的
1. 如果处理模拟的各种数量的 level : CONFIG_PGTABLE_LEVELS
2. 似乎 获取 address 的，似乎各种 flag 占用的 bit 数量太多了，应该问题不大，反正这些 table 的高位都是在内核的虚拟地址空间，所有都是


B. 通过分析 `__handle_mm_fault` 说明其中的机制：
由于 page walk 需要硬件在 TLB 和 tlb miss 的情况下提供额外的支持。

// 有待处理的
1. vm_fault 所有成员解释 todo
3. devmap : pud_devmap 的作用是什么 ?


```c
/*
 * By the time we get here, we already hold the mm semaphore
 *
 * The mmap_sem may have been released depending on flags and our
 * return value.  See filemap_fault() and __lock_page_or_retry().
 */
static vm_fault_t __handle_mm_fault(struct vm_area_struct *vma,
    unsigned long address, unsigned int flags)
{
  struct vm_fault vmf = {
    .vma = vma,
    .address = address & PAGE_MASK,
    .flags = flags,
    .pgoff = linear_page_index(vma, address),
    .gfp_mask = __get_fault_gfp_mask(vma),
  };
  unsigned int dirty = flags & FAULT_FLAG_WRITE;
  struct mm_struct *mm = vma->vm_mm;
  pgd_t *pgd;
  p4d_t *p4d;
  vm_fault_t ret;

  pgd = pgd_offset(mm, address); // 访问 mm_struct::pgd 以及 address 的偏移，但是可以从此处获取到
  p4d = p4d_alloc(mm, pgd, address); // 如果 pgd 指向 p4d entry 是无效的，首先分配。如果有效，只是简单的计算地址。
  if (!p4d)
    return VM_FAULT_OOM;

  vmf.pud = pud_alloc(mm, p4d, address); // vmf.pud 指向 pmd。vmf.pud 对应的映射范围 : pmd 的 entry *  page table 的 entry * PAGE_SIZE
  if (!vmf.pud)
    return VM_FAULT_OOM;
retry_pud:
  if (pud_none(*vmf.pud) && __transparent_hugepage_enabled(vma)) {
    ret = create_huge_pud(&vmf);
    if (!(ret & VM_FAULT_FALLBACK))
      return ret;
  } else {
    pud_t orig_pud = *vmf.pud;

    barrier(); // TODO 现在不清楚为什么需要添加 barrier
    if (pud_trans_huge(orig_pud) || pud_devmap(orig_pud)) {

      /* NUMA case for anonymous PUDs would go here */

      if (dirty && !pud_write(orig_pud)) {
        ret = wp_huge_pud(&vmf, orig_pud); //
        if (!(ret & VM_FAULT_FALLBACK))
          return ret;
      } else {
        huge_pud_set_accessed(&vmf, orig_pud);
        return 0;
      }
    }
  }

  vmf.pmd = pmd_alloc(mm, vmf.pud, address); // 如果处理的不是 vmf.pud 指向的不是 pgfault
  if (!vmf.pmd)
    return VM_FAULT_OOM;

  /* Huge pud page fault raced with pmd_alloc? */
  if (pud_trans_unstable(vmf.pud)) // 当线程同时在进行 page fault
    goto retry_pud;

  if (pmd_none(*vmf.pmd) && __transparent_hugepage_enabled(vma)) {
    ret = create_huge_pmd(&vmf);
    if (!(ret & VM_FAULT_FALLBACK))
      return ret;
  } else {
    pmd_t orig_pmd = *vmf.pmd;

    barrier();
    if (unlikely(is_swap_pmd(orig_pmd))) { // TODO swap 的关系是什么
      VM_BUG_ON(thp_migration_supported() &&
            !is_pmd_migration_entry(orig_pmd));
      if (is_pmd_migration_entry(orig_pmd))
        pmd_migration_entry_wait(mm, vmf.pmd);
      return 0;
    }
    if (pmd_trans_huge(orig_pmd) || pmd_devmap(orig_pmd)) {
      if (pmd_protnone(orig_pmd) && vma_is_accessible(vma))
        return do_huge_pmd_numa_page(&vmf, orig_pmd); // TODO 处理内容

      if (dirty && !pmd_write(orig_pmd)) {
        ret = wp_huge_pmd(&vmf, orig_pmd);
        if (!(ret & VM_FAULT_FALLBACK))
          return ret;
      } else {
        huge_pmd_set_accessed(&vmf, orig_pmd);
        return 0;
      }
    }
  }

  return handle_pte_fault(&vmf);
}
```


```txt
#0  do_set_pte (vmf=vmf@entry=0xffffc90001a2bb88, page=page@entry=0xffffea0004a5c0c0, addr=94425197883392) at mm/memory.c:4321
#1  0xffffffff812d7aff in finish_fault (vmf=vmf@entry=0xffffc90001a2bb88) at mm/memory.c:4422
#2  0xffffffff812d7f3e in do_cow_fault (vmf=0xffffc90001a2bb88) at mm/memory.c:4593
#3  do_fault (vmf=vmf@entry=0xffffc90001a2bb88) at mm/memory.c:4685
#4  0xffffffff812dc944 in handle_pte_fault (vmf=0xffffc90001a2bb88) at mm/memory.c:4955
#5  __handle_mm_fault (vma=vma@entry=0xffff8881239052f8, address=address@entry=94425197883412, flags=flags@entry=533) at mm/memory.c:5097
#6  0xffffffff812dd620 in handle_mm_fault (vma=0xffff8881239052f8, address=address@entry=94425197883412, flags=flags@entry=533, regs=regs@entry=0xffffc90001a2bce8) at mm/memory.c:5218
#7  0xffffffff810f3ca3 in do_user_addr_fault (regs=regs@entry=0xffffc90001a2bce8, error_code=error_code@entry=2, address=address@entry=94425197883412) at arch/x86/mm/fault.c:1428
#8  0xffffffff81fa8e12 in handle_page_fault (address=94425197883412, error_code=2, regs=0xffffc90001a2bce8) at arch/x86/mm/fault.c:1519
#9  exc_page_fault (regs=0xffffc90001a2bce8, error_code=2) at arch/x86/mm/fault.c:1575
#10 0xffffffff82000b62 in asm_exc_page_fault () at ./arch/x86/include/asm/idtentry.h:570
```

## 是 segfault 还是 oom
```txt
#0  show_signal_msg (tsk=<optimized out>, address=<optimized out>, error_code=<optimized out>, regs=<optimized out>) at arch/x86/mm/fault.c:841
#1  __bad_area_nosemaphore (regs=0xffffc90002633e47, error_code=7, address=7, pkey=0x0 <irq_stack_union>, si_code=1) at arch/x86/mm/fault.c:904
#2  0xffffffff81064bb0 in bad_area_nosemaphore (regs=<optimized out>, error_code=<optimized out>, address=<optimized out>, pkey=<optimized out>) at arch/x86/mm/fault.c:925
#3  0xffffffff81064f48 in __do_page_fault (regs=0xffffc90002633f58, error_code=6, address=18446744073709551615) at arch/x86/mm/fault.c:1344
#4  0xffffffff81065381 in do_page_fault (regs=0xffffc90002633f58, error_code=6) at arch/x86/mm/fault.c:1485
#5  0xffffffff81a0119e in async_page_fault () at arch/x86/entry/entry_64.S:1207
```
