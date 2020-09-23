# mm/memory.c

## KeyNote
1. swapbacked 似乎用于表示 : 这个 page 是 anon vma 中间的，所有 anon vma 中间 page 都是需要走这一个(tmpfs 暂时不知道怎么回事)，
    page_add_new_anon_rmap(page, vma, vmf->address, false); // 无条件调用 __SetPageSwapBacked 
2. 相对比的是 : PG_swapcache 表示当前 page 在 swap cache 中间，也即是当前的 page 在 radix tree 中间的
    1. PageSwapCache 函数 : 也可以说明，必须首先是 anon vma 中间的page ，然后才可以是 swap cache 的内容
    2. PageAnon 和 PageSwapBacked 有什么区别吗 ?

## 问题
1. do_wp_page 的作用是什么
    1. anon 和 file 都被处理，其需要处理什么位置的 ?

2. 检查一下使用 pte_same 的位置，是不是为了处理 page table 上锁的情况 ?

3. pte_mkdirty pte_mkyoung

4. pte_lockptr


## page table 的操作
```c
// 似乎并没有什么神奇的，page 和 权限，page 提供物理地址

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 *
 * (Currently stuck as a macro because of indirect forward reference
 * to linux/mm.h:page_to_nid())
 */
#define mk_pte(page, pgprot)   pfn_pte(page_to_pfn(page), (pgprot))


static inline pte_t pfn_pte(unsigned long page_nr, pgprot_t pgprot)
{
	phys_addr_t pfn = (phys_addr_t)page_nr << PAGE_SHIFT;
	pfn ^= protnone_mask(pgprot_val(pgprot));
	pfn &= PTE_PFN_MASK;
	return __pte(pfn | check_pgprot(pgprot));
}


#define __pte(x)	native_make_pte(x)

static inline pte_t native_make_pte(pteval_t val)
{
	return (pte_t) { .pte = val };
}

typedef struct { pteval_t pte; } pte_t;

// /home/shen/linux/arch/x86/include/asm/pgtable_64_types.h 最后的封装类型 unsigned long
```

> mm/memory.c 中间，其实是一些熟悉的内容。
1. pte 和 tlb 的操作，主要是为pg_fault 支持的
2. page_fault

> 总是都是修改page table 之类的事情


```c
int __pte_alloc(struct mm_struct *mm, pmd_t *pmd, unsigned long address)
{
	spinlock_t *ptl;
  // 申请一个内存
	pgtable_t new = pte_alloc_one(mm, address);
	if (!new)
		return -ENOMEM;

	/*
	 * Ensure all pte setup (eg. pte page lock and page clearing) are
	 * visible before the pte is made visible to other CPUs by being
	 * put into page tables.
	 *
	 * The other side of the story is the pointer chasing in the page
	 * table walking code (when walking the page table without locking;
	 * ie. most of the time). Fortunately, these data accesses consist
	 * of a chain of data-dependent loads, meaning most CPUs (alpha
	 * being the notable exception) will already guarantee loads are
	 * seen in-order. See the alpha page table accessors for the
	 * smp_read_barrier_depends() barriers in page table walking code.
	 */
	smp_wmb(); /* Could be smp_wmb__xxx(before|after)_spin_lock */

	ptl = pmd_lock(mm, pmd);
	if (likely(pmd_none(*pmd))) {	/* Has another populated it ? */
		mm_inc_nr_ptes(mm);
		pmd_populate(mm, pmd, new);
		new = NULL;
	}
	spin_unlock(ptl);
	if (new)
		pte_free(mm, new);
	return 0;
}


typedef struct page *pgtable_t;

pgtable_t pte_alloc_one(struct mm_struct *mm, unsigned long address)
{
	struct page *pte;

	pte = alloc_pages(__userpte_alloc_gfp, 0);
	if (!pte)
		return NULL;
	if (!pgtable_page_ctor(pte)) {
		__free_page(pte);
		return NULL;
	}
	return pte;
}
```


## handle_pte_fault : 万恶之源
1. 谁在调用我 ?
2. vmf 的中间的每一个参数的含义是什么 ?
    1. orig_pte : 当发生 pgfault 的时候的 pte, hardware 如何对其初始化的
    2. pte : 那我是什么 ?
    3. flags :
    4. address : 这个在 需要被 `__page_set_anon_rmap` 用于设置 `page->index`

```c
/*
 * These routines also need to handle stuff like marking pages dirty
 * and/or accessed for architectures that don't do it in hardware (most
 * RISC architectures).  The early dirtying is also good on the i386.
 *
 * There is also a hook called "update_mmu_cache()" that architectures
 * with external mmu caches can use to update those (ie the Sparc or
 * PowerPC hashed page tables that act as extended TLBs).
 *
 * We enter with non-exclusive mmap_sem (to exclude vma changes, but allow
 * concurrent faults).
 *
 * The mmap_sem may have been released depending on flags and our return value.
 * See filemap_fault() and __lock_page_or_retry().
 */
static vm_fault_t handle_pte_fault(struct vm_fault *vmf)
{
	pte_t entry;

	if (unlikely(pmd_none(*vmf->pmd))) { // todo 一共存在五级，为什么只是询问这一个姐比的
		/*
		 * Leave __pte_alloc() until later: because vm_ops->fault may
		 * want to allocate huge page, and if we expose page table
		 * for an instant, it will be difficult to retract from
		 * concurrent faults and from rmap lookups.
		 */
		vmf->pte = NULL;
	} else {
		/* See comment in pte_alloc_one_map() */
		if (pmd_devmap_trans_unstable(vmf->pmd)) // todo 
			return 0;
		/*
		 * A regular pmd is established and it can't morph into a huge
		 * pmd from under us anymore at this point because we hold the
		 * mmap_sem read mode and khugepaged takes it in write mode.
		 * So now it's safe to run pte_offset_map().
		 */
		vmf->pte = pte_offset_map(vmf->pmd, vmf->address);
		vmf->orig_pte = *vmf->pte;

		/*
		 * some architectures can have larger ptes than wordsize,
		 * e.g.ppc44x-defconfig has CONFIG_PTE_64BIT=y and
		 * CONFIG_32BIT=y, so READ_ONCE cannot guarantee atomic
		 * accesses.  The code below just needs a consistent view
		 * for the ifs and we later double check anyway with the
		 * ptl lock held. So here a barrier will do.
		 */
		barrier();
		if (pte_none(vmf->orig_pte)) { // todo pte_none 的原理
			pte_unmap(vmf->pte); // nop
			vmf->pte = NULL;
		}
	}

	if (!vmf->pte) { // 没有 pte
		if (vma_is_anonymous(vmf->vma))
			return do_anonymous_page(vmf); // XXX
		else
			return do_fault(vmf); // XXX
	}

	if (!pte_present(vmf->orig_pte))
		return do_swap_page(vmf); // XXX

	if (pte_protnone(vmf->orig_pte) && vma_is_accessible(vmf->vma)) // todo 神奇啊，numa 也是可以支持 page fault 的
		return do_numa_page(vmf);

	vmf->ptl = pte_lockptr(vmf->vma->vm_mm, vmf->pmd); // todo plt 的作用是什么 ? 为什么上锁 ?
	spin_lock(vmf->ptl);
	entry = vmf->orig_pte;
	if (unlikely(!pte_same(*vmf->pte, entry)))
		goto unlock;
	if (vmf->flags & FAULT_FLAG_WRITE) {
		if (!pte_write(entry)) // 测试 entry 上的 flag, 查看是否可以写，如果不能写，那么就是 cow 
			return do_wp_page(vmf);
		entry = pte_mkdirty(entry); // 可以，那就通知一下
	}

  // todo 下面的部分的作用是什么 ?
	entry = pte_mkyoung(entry);
	if (ptep_set_access_flags(vmf->vma, vmf->address, vmf->pte, entry,
				vmf->flags & FAULT_FLAG_WRITE)) {
		update_mmu_cache(vmf->vma, vmf->address, vmf->pte);
	} else {
		/*
		 * This is needed only for protection faults but the arch code
		 * is not yet telling us if this is a protection fault or not.
		 * This still avoids useless tlb flushes for .text page faults
		 * with threads.
		 */
		if (vmf->flags & FAULT_FLAG_WRITE)
			flush_tlb_fix_spurious_fault(vmf->vma, vmf->address);
	}
unlock:
	pte_unmap_unlock(vmf->pte, vmf->ptl);
	return 0;
}
```


## do_swap_page 
1. 为什么需要调用 do_wp_page ?

```c
/*
 * We enter with non-exclusive mmap_sem (to exclude vma changes,
 * but allow concurrent faults), and pte mapped but not yet locked.
 * We return with pte unmapped and unlocked.
 *
 * We return with the mmap_sem locked or unlocked in the same cases
 * as does filemap_fault().
 */
vm_fault_t do_swap_page(struct vm_fault *vmf)
{
	struct vm_area_struct *vma = vmf->vma;
	struct page *page = NULL, *swapcache;
	struct mem_cgroup *memcg;
	swp_entry_t entry;
	pte_t pte;
	int locked;
	int exclusive = 0;
	vm_fault_t ret = 0;

	if (!pte_unmap_same(vma->vm_mm, vmf->pmd, vmf->pte, vmf->orig_pte)) // 检查 vmf->pte 和 vmf->orig_pte 相等
		goto out;

	entry = pte_to_swp_entry(vmf->orig_pte);
	if (unlikely(non_swap_entry(entry))) { // 处理各种骚操作
		if (is_migration_entry(entry)) {
			migration_entry_wait(vma->vm_mm, vmf->pmd,
					     vmf->address);
		} else if (is_device_private_entry(entry)) {
			vmf->page = device_private_entry_to_page(entry);
			ret = vmf->page->pgmap->ops->migrate_to_ram(vmf);
		} else if (is_hwpoison_entry(entry)) {
			ret = VM_FAULT_HWPOISON;
		} else {
			print_bad_pte(vma, vmf->address, vmf->orig_pte, NULL);
			ret = VM_FAULT_SIGBUS;
		}
		goto out;
	}


	delayacct_set_flag(DELAYACCT_PF_SWAPIN);
	page = lookup_swap_cache(entry, vma, vmf->address); // 查询 swap cache : 位于 swap_state.c
	swapcache = page;

	if (!page) {
		struct swap_info_struct *si = swp_swap_info(entry);

		if (si->flags & SWP_SYNCHRONOUS_IO &&
				__swap_count(entry) == 1) { // __swap_count 表示当前在 page table 中间还有多少 swp_entry_t 指向这个 page , 此处，表示，该 page 生前没有被共享。todo 当然，为什么在这种条件下需要，就是 skip swapcache ，以及 skip swapcache 的含义是什么，我是不清楚的
			/* skip swapcache */ // XXX 表示新创建的 page 不要出现在 swap cache 中间，
			page = alloc_page_vma(GFP_HIGHUSER_MOVABLE, vma, // 具体不知道，也许是持有 policy 的 alloc 一个 page 吧 !
							vmf->address);
			if (page) {
				__SetPageLocked(page);  // todo 为什么需要这一个标志
				__SetPageSwapBacked(page); // 因为 skip swap cache，所以仅仅设置 SwapBacked 
				set_page_private(page, entry.val); 
				lru_cache_add_anon(page); // 的确是无人幸免，比如加入到 lru cache 中间
				swap_readpage(page, true);
			}
		} else {
			page = swapin_readahead(entry, GFP_HIGHUSER_MOVABLE, // 这里会自动将 
						vmf); // swapin_readahead 将上面的操作也搞过一边，只是自带 readahead 而已
			swapcache = page;
		}

		if (!page) { // 两种方法都失败
			/*
			 * Back out if somebody else faulted in this pte
			 * while we released the pte lock.
			 */
			vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd,
					vmf->address, &vmf->ptl);
			if (likely(pte_same(*vmf->pte, vmf->orig_pte)))
				ret = VM_FAULT_OOM;
			delayacct_clear_flag(DELAYACCT_PF_SWAPIN);
			goto unlock;
		}

		/* Had to read the page from swap area: Major fault */
		ret = VM_FAULT_MAJOR;
		count_vm_event(PGMAJFAULT);
		count_memcg_event_mm(vma->vm_mm, PGMAJFAULT);
	} else if (PageHWPoison(page)) { // 几乎不可能的事情吧!
		/*
		 * hwpoisoned dirty swapcache pages are kept for killing
		 * owner processes (which may be unknown at hwpoison time)
		 */
		ret = VM_FAULT_HWPOISON;
		delayacct_clear_flag(DELAYACCT_PF_SWAPIN);
		goto out_release;
	}

  // 对于刚刚得到的 page 当然不会出现问题，当对于从 page cache 中间找到的需要上锁的
	locked = lock_page_or_retry(page, vma->vm_mm, vmf->flags); // todo 这个函数很诡异的!

	delayacct_clear_flag(DELAYACCT_PF_SWAPIN);
	if (!locked) {
		ret |= VM_FAULT_RETRY;
		goto out_release;
	}

	/*
	 * Make sure try_to_free_swap or reuse_swap_page or swapoff did not
	 * release the swapcache from under us.  The page pin, and pte_same
	 * test below, are not enough to exclude that.  Even if it is still
	 * swapcache, we need to check that the page's swap has not changed.
	 */
   // todo try_to_free_swap reuse_swap_page swapoff 可以消除掉 PageSwapCache 的 flags
	if (unlikely((!PageSwapCache(page) ||
			page_private(page) != entry.val)) && swapcache)
		goto out_page;

	/*
	 * Back out if somebody else already faulted in this pte.
	 */
	vmf->pte = pte_offset_map_lock(vma->vm_mm, vmf->pmd, vmf->address,
			&vmf->ptl);
	if (unlikely(!pte_same(*vmf->pte, vmf->orig_pte)))
		goto out_nomap;

	if (unlikely(!PageUptodate(page))) { // 这个 flag 什么时候处理过 ?
		ret = VM_FAULT_SIGBUS;
		goto out_nomap;
	}

	/*
	 * The page isn't present yet, go ahead with the fault.
	 *
	 * Be careful about the sequence of operations here.
	 * To get its accounting right, reuse_swap_page() must be called
	 * while the page is counted on swap but not yet in mapcount i.e.
	 * before page_add_anon_rmap() and swap_free(); try_to_free_swap()
	 * must be called after the swap_free(), or it will never succeed.
	 */

	inc_mm_counter_fast(vma->vm_mm, MM_ANONPAGES);
	dec_mm_counter_fast(vma->vm_mm, MM_SWAPENTS);
	pte = mk_pte(page, vma->vm_page_prot); // 根据 page 的物理地址 和 权限创建出来 pte
	if ((vmf->flags & FAULT_FLAG_WRITE) && reuse_swap_page(page, NULL)) { // todo reuse_swap_page 的含义非常诡异
		pte = maybe_mkwrite(pte_mkdirty(pte), vma);
		vmf->flags &= ~FAULT_FLAG_WRITE;
		ret |= VM_FAULT_WRITE;
		exclusive = RMAP_EXCLUSIVE; // todo 这个flag的添加是在关于 huge page 之类添加的，够神奇啊!
	}
	flush_icache_page(vma, page);
	if (pte_swp_soft_dirty(vmf->orig_pte))
		pte = pte_mksoft_dirty(pte);
	set_pte_at(vma->vm_mm, vmf->address, vmf->pte, pte); // 更新 page table
	arch_do_swap_page(vma->vm_mm, vma, vmf->address, pte, vmf->orig_pte); // nop
	vmf->orig_pte = pte;

	/* ksm created a completely new copy */
	if (unlikely(page != swapcache && swapcache)) {
    //  只有新创建的 anon 才会到此处吗 ?
		page_add_new_anon_rmap(page, vma, vmf->address, false); // 无论如何，都是需要添加到 anon 的，
		lru_cache_add_active_or_unevictable(page, vma);
	} else {
    // todo 独享的时候，其实也不是很懂为什么会出现独享
		do_page_add_anon_rmap(page, vma, vmf->address, exclusive); // do_swap_page 特供的, todo 存在什么特别之处
		activate_page(page); 
	}

	swap_free(entry); // 加载了一个 page 进来，todo 当初写入的时候，计数的初始化，应该在 unmap 之类的地方吧!
	if (mem_cgroup_swap_full(page) ||
	    (vma->vm_flags & VM_LOCKED) || PageMlocked(page))
		try_to_free_swap(page);  // todo swap_free 难道没有清理掉这个蛇皮吗 ? 表示两个内容
	unlock_page(page);
	if (page != swapcache && swapcache) {
		/*
		 * Hold the lock to avoid the swap entry to be reused
		 * until we take the PT lock for the pte_same() check
		 * (to avoid false positives from pte_same). For
		 * further safety release the lock after the swap_free
		 * so that the swap count won't change under a
		 * parallel locked swapcache.
		 */
		unlock_page(swapcache);
		put_page(swapcache);
	}

	if (vmf->flags & FAULT_FLAG_WRITE) {
		ret |= do_wp_page(vmf); // 其实此处非常的有道理的，即使是写入disk 中间还是可以发生
		if (ret & VM_FAULT_ERROR)
			ret &= VM_FAULT_ERROR;
		goto out;
	}

	/* No need to invalidate - it was non-present before */
	update_mmu_cache(vma, vmf->address, vmf->pte);
unlock:
	pte_unmap_unlock(vmf->pte, vmf->ptl);
out:
	return ret;
out_nomap:
	mem_cgroup_cancel_charge(page, memcg, false);
	pte_unmap_unlock(vmf->pte, vmf->ptl);
out_page:
	unlock_page(page);
out_release:
	put_page(page);
	if (page != swapcache && swapcache) {
		unlock_page(swapcache);
		put_page(swapcache);
	}
	return ret;
}
```
1. 其中关于 swapcache 变量的使用，感到非常的迷惑


## do_wp_page
1. do_swap_page 为什么会调用这一个 ?
2. 和 do_cow_fault 含有根本的区别 : do_cow_fault 仅仅限于处理 mmap file 中间的 cow，而且是首次遇到 ，而 do_wp_page 一般的 cow

3. 猜测实现:
    1. 复制 pte : 整个 page walk
    2. 去掉 flag
    3. 分配新的page , 拷贝原来的 page 
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
3. cow 会分配两次page frame ，一次在 page cache 中间，一次自己使，(正确)

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

## alloc_set_pte : 为 page 建立 pte entry 以及 reverse page
1. 主要被 finish_fault 调用
2. @todo 细节满满

```c
/**
 * alloc_set_pte - setup new PTE entry for given page and add reverse page
 * mapping. If needed, the fucntion allocates page table or use pre-allocated.
 *
 * @vmf: fault environment
 * @memcg: memcg to charge page (only for private mappings)
 * @page: page to map
 *
 * Caller must take care of unlocking vmf->ptl, if vmf->pte is non-NULL on
 * return.
 *
 * Target users are page handler itself and implementations of
 * vm_ops->map_pages.
 *
 * Return: %0 on success, %VM_FAULT_ code in case of error.
 */
vm_fault_t alloc_set_pte(struct vm_fault *vmf, struct mem_cgroup *memcg,
		struct page *page)
{
	struct vm_area_struct *vma = vmf->vma;
	bool write = vmf->flags & FAULT_FLAG_WRITE;
	pte_t entry;
	vm_fault_t ret;

	if (pmd_none(*vmf->pmd) && PageTransCompound(page) &&
			IS_ENABLED(CONFIG_TRANSPARENT_HUGE_PAGECACHE)) {
		/* THP on COW? */
		VM_BUG_ON_PAGE(memcg, page);

		ret = do_set_pmd(vmf, page);
		if (ret != VM_FAULT_FALLBACK)
			return ret;
	}

	if (!vmf->pte) {
		ret = pte_alloc_one_map(vmf);
		if (ret)
			return ret;
	}

	/* Re-check under ptl */
	if (unlikely(!pte_none(*vmf->pte)))
		return VM_FAULT_NOPAGE;

	flush_icache_page(vma, page);
	entry = mk_pte(page, vma->vm_page_prot);
	if (write)
		entry = maybe_mkwrite(pte_mkdirty(entry), vma);
	/* copy-on-write page */
	if (write && !(vma->vm_flags & VM_SHARED)) {
		inc_mm_counter_fast(vma->vm_mm, MM_ANONPAGES);
		page_add_new_anon_rmap(page, vma, vmf->address, false);
		mem_cgroup_commit_charge(page, memcg, false, false);
		lru_cache_add_active_or_unevictable(page, vma);
	} else {
		inc_mm_counter_fast(vma->vm_mm, mm_counter_file(page));
		page_add_file_rmap(page, false);
	}
	set_pte_at(vma->vm_mm, vmf->address, vmf->pte, entry);

	/* no need to invalidate: a not-present page won't be cached */
	update_mmu_cache(vma, vmf->address, vmf->pte);

	return 0;
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

