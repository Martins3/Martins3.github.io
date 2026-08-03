// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 *
 * userswap core file
 */

#include <linux/swap.h>
#include <linux/swapops.h>
#include <linux/rmap.h>
#include <linux/mmu_notifier.h>
#include <linux/hugetlb.h>
#include "userswap.h"

#include "internal.h"

/*
 * 只能编译不能链接
 * - 来源: https://gitee.com/openeuler/kernel/blob/OLK-6.6/mm/userswap.c
 */

// ----- 一些 openEuler 特有的函数拷贝
void page_add_new_anon_rmap(struct page *page, struct vm_area_struct *vma,
			    unsigned long address);
void page_add_new_anon_rmap(struct page *page, struct vm_area_struct *vma,
			    unsigned long address)
{
	VM_BUG_ON_PAGE(PageTail(page), page);

	return folio_add_new_anon_rmap((struct folio *)page, vma, address);
}
// -----  拷贝结束

DEFINE_STATIC_KEY_FALSE(userswap_enabled);

static bool vma_uswap_compatible(struct vm_area_struct *vma)
{
	if (!vma || !vma_is_anonymous(vma) || vma->vm_file ||
	    (vma->vm_flags & (VM_SHARED | VM_LOCKED | VM_STACK | VM_IO |
			      VM_PFNMAP | VM_HUGETLB)))
		return false;
	return true;
}

/*
 * Check if pages between 'addr ~ addr+len' can be user swapped. If so, get
 * the reference of the pages and return the pages through input parameters
 * 'ppages'.
 */
static unsigned long pages_can_be_swapped(struct mm_struct *mm,
					  unsigned long addr, unsigned long len,
					  struct page ***ppages)
{
	struct vm_area_struct *vma;
	struct page *page = NULL;
	struct page **pages = NULL;
	unsigned long addr_end = addr + len;
	unsigned long ret;
	unsigned long i, page_num = 0;
	*ppages = NULL;

	pages = kvzalloc(sizeof(struct page *) * (len / PAGE_SIZE), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	while (addr < addr_end) {
		vma = find_vma(mm, addr);
		if (!vma || addr < vma->vm_start ||
		    !(vma->vm_flags & VM_USWAP) || !vma_uswap_compatible(vma)) {
			ret = -EINVAL;
			goto out_err;
		}

		if (!(vma->vm_flags & VM_UFFD_MISSING)) {
			ret = -EAGAIN;
			goto out_err;
		}
get_again:
		/*
		 * follow_page will inc page ref, dec the ref after we remap
		 * the page.
		 */
		page = follow_page(vma, addr, FOLL_GET | FOLL_DUMP);
		if (IS_ERR_OR_NULL(page)) {
			ret = -ENODEV;
			goto out_err;
		}

		pages[page_num++] = page;
		if (!PageAnon(page) || !PageSwapBacked(page) ||
		    PageHuge(page) || PageSwapCache(page)) {
			ret = -EINVAL;
			goto out_err;
		}

		if (PageTransCompound(page)) {
			if (trylock_page(page)) {
				if (!split_huge_page(page)) {
					unlock_page(page);
					put_page(page);
					page_num--;
					goto get_again;
				} else {
					unlock_page(page);
				}
			}
			ret = -EINVAL;
			goto out_err;
		}

		/*
		 * Check that no O_DIRECT or similar I/O is in progress on the
		 * page
		 */
		if (page_mapcount(page) > 1 ||
		    page_mapcount(page) + 1 != page_count(page)) {
			ret = -EBUSY;
			goto out_err;
		}
		addr += PAGE_SIZE;
	}

	*ppages = pages;
	return 0;

out_err:
	for (i = 0; i < page_num; i++)
		put_page(pages[i]);
	kvfree(pages);
	return ret;
}

static bool is_thp_or_huge(struct mm_struct *mm, unsigned long addr)
{
	pud_t *pud;
	pmd_t *pmd;

	pud = get_old_pud(mm, addr);
	if (!pud)
		return false;
	else if (pud_huge(*pud))
		return true;

	pmd = pmd_offset(pud, addr);
	if (!pmd)
		return false;
	else if (pmd_huge(*pmd) || pmd_trans_huge(*pmd))
		return true;

	return false;
}

static int uswap_unmap_anon_page(struct mm_struct *mm,
				 struct vm_area_struct *vma, unsigned long addr,
				 struct page *page, pmd_t *pmd, pte_t *old_pte,
				 bool set_to_swp)
{
	struct mmu_notifier_range range;
	spinlock_t *ptl;
	pte_t *pte, _old_pte;
	int ret = 0;

	mmu_notifier_range_init(&range, MMU_NOTIFY_UNMAP, 0, vma->vm_mm, addr,
				addr + PAGE_SIZE);
	mmu_notifier_invalidate_range_start(&range);
	pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
	if (!pte_present(*pte)) {
		ret = -EINVAL;
		goto out_release_unlock;
	}
	flush_cache_page(vma, addr, pte_pfn(*pte));
	_old_pte = ptep_clear_flush(vma, addr, pte);
	if (old_pte)
		*old_pte = _old_pte;
	if (set_to_swp)
		set_pte_at(mm, addr, pte,
			   swp_entry_to_pte(swp_entry(SWP_USERSWAP_ENTRY,
						      page_to_pfn(page))));

	dec_mm_counter(mm, MM_ANONPAGES);
	add_reliable_page_counter(page, mm, -1);
	folio_remove_rmap_pte(page_folio(page), page, vma);
	page->mapping = NULL;

out_release_unlock:
	pte_unmap_unlock(pte, ptl);
	mmu_notifier_invalidate_range_end(&range);
	return ret;
}

static unsigned long vm_insert_anon_page(struct vm_area_struct *vma,
					 unsigned long addr, struct page *page)
{
	struct mm_struct *mm = vma->vm_mm;
	int ret = 0;
	pte_t *pte, dst_pte;
	spinlock_t *ptl;

	if (unlikely(anon_vma_prepare(vma)))
		return -ENOMEM;

	flush_dcache_page(page);
	pte = get_locked_pte(mm, addr, &ptl);
	if (!pte)
		return -ENOMEM;
	if (!pte_none(*pte)) {
		ret = -EBUSY;
		goto out_unlock;
	}

	inc_mm_counter(mm, MM_ANONPAGES);
	add_reliable_page_counter(page, mm, 1);
	page_add_new_anon_rmap(page, vma, addr);
	dst_pte = mk_pte(page, vma->vm_page_prot);
	if (vma->vm_flags & VM_WRITE)
		dst_pte = pte_mkwrite_novma(pte_mkdirty(dst_pte));
	set_pte_at(mm, addr, pte, dst_pte);

out_unlock:
	pte_unmap_unlock(pte, ptl);
	return ret;
}

static void uswap_map_anon_page(struct mm_struct *mm,
				struct vm_area_struct *vma, unsigned long addr,
				struct page *page, pmd_t *pmd, pte_t old_pte)
{
	spinlock_t *ptl;
	pte_t *pte;

	pte = pte_offset_map_lock(mm, pmd, addr, &ptl);
	flush_cache_page(vma, addr, pte_pfn(*pte));
	set_pte_at(mm, addr, pte, old_pte);
	inc_mm_counter(mm, MM_ANONPAGES);
	add_reliable_page_counter(page, mm, 1);
	page_add_new_anon_rmap(page, vma, addr);
	pte_unmap_unlock(pte, ptl);
}

static void uswapout_recover(struct mm_struct *mm, unsigned long old_addr_start,
			     unsigned long len, struct page **pages,
			     unsigned long new_addr_start, pte_t *ptes)
{
	unsigned long unmap_old_addr = old_addr_start;
	unsigned long unmap_new_addr = new_addr_start;
	struct page *page;
	pmd_t *old_pmd, *new_pmd;
	pte_t pte;
	unsigned long i;

	for (i = 0; i < len; i++) {
		page = pages[i];
		pte = ptes[i];
		new_pmd = mm_find_pmd(mm, new_addr_start);
		old_pmd = mm_find_pmd(mm, unmap_old_addr);

		uswap_unmap_anon_page(mm, find_vma(mm, unmap_new_addr),
				      unmap_new_addr, page, new_pmd, NULL,
				      false);
		uswap_map_anon_page(mm, find_vma(mm, unmap_old_addr),
				    unmap_old_addr, page, old_pmd, pte);
		unmap_old_addr += PAGE_SIZE;
		unmap_new_addr += PAGE_SIZE;
	}
	if (pte_val(ptes[len]) != 0) {
		page = pages[len];
		pte = ptes[len];
		old_pmd = mm_find_pmd(mm, unmap_old_addr);

		uswap_map_anon_page(mm, find_vma(mm, unmap_old_addr),
				    unmap_old_addr, page, old_pmd, pte);
		get_page(page);
	}
}

/* unmap the pages between 'addr ~ addr+len' and remap them to a new address */
static unsigned long do_user_swap(struct mm_struct *mm,
				  unsigned long old_addr_start,
				  unsigned long len, struct page **pages,
				  unsigned long new_addr_start)
{
	struct vm_area_struct *old_vma, *new_vma;
	unsigned long old_addr = old_addr_start;
	unsigned long new_addr = new_addr_start;
	struct page *page;
	pmd_t *pmd;
	pte_t old_pte, *ptes;
	bool pages_dirty = false;
	unsigned long i = 0, j;
	int ret;

	ptes = kvzalloc(sizeof(pte_t) * (len / PAGE_SIZE), GFP_KERNEL);
	if (!ptes)
		return -ENOMEM;
	lru_add_drain();
	for (j = 0; j < len; j += PAGE_SIZE) {
		page = pages[i];
		ret = -EINVAL;
		if (!page)
			goto out_recover;
		if (is_thp_or_huge(mm, new_addr))
			goto out_recover;
		old_vma = find_vma(mm, old_addr);
		if (!old_vma || old_addr < old_vma->vm_start)
			goto out_recover;
		new_vma = find_vma(mm, new_addr);
		if (!new_vma || new_addr < new_vma->vm_start)
			goto out_recover;
		if (!vma_uswap_compatible(new_vma))
			goto out_recover;

		ret = -EACCES;
		if (!(old_vma->vm_flags & VM_WRITE) &&
		    (new_vma->vm_flags & VM_WRITE))
			goto out_recover;

		ret = -ENXIO;
		pmd = mm_find_pmd(mm, old_addr);
		if (!pmd)
			goto out_recover;
		ret = uswap_unmap_anon_page(mm, old_vma, old_addr, page, pmd,
					    &old_pte, true);
		if (ret)
			goto out_recover;
		ptes[i] = old_pte;
		if (pte_dirty(old_pte) || PageDirty(page))
			pages_dirty = true;
		put_page(page);

		ret = vm_insert_anon_page(new_vma, new_addr, page);
		if (ret)
			goto out_recover;
		get_page(page);

		old_addr += PAGE_SIZE;
		new_addr += PAGE_SIZE;
		i++;
	}

	if (pages_dirty)
		new_addr_start = new_addr_start | USWAP_PAGES_DIRTY;
	kvfree(ptes);
	return new_addr_start;

out_recover:
	uswapout_recover(mm, old_addr_start, i, pages, new_addr_start, ptes);
	kvfree(ptes);
	return ret;
}

/*
 * When flags is MREMAP_USWAP_SET_PTE, uswap_mremap() is called in syscall
 * mremap.
 * Unmap the pages between 'addr ~ addr + old_len' and remap them to 'new_addr
 * ~ new_addr + new_len'. Set the pte of old_addr to SWP_USERSWAP_ENTRY.
 */
unsigned long uswap_mremap(unsigned long old_addr, unsigned long old_len,
			   unsigned long new_addr, unsigned long new_len)
{
	struct page **pages = NULL;
	struct mm_struct *mm = current->mm;
	unsigned long len = old_len;
	unsigned long ret = -EINVAL;
	unsigned long i;

	if (!static_branch_unlikely(&userswap_enabled))
		goto out;

	if (offset_in_page(old_addr))
		goto out;

	old_len = PAGE_ALIGN(old_len);
	new_len = PAGE_ALIGN(new_len);

	if (!new_len || old_len != new_len || offset_in_page(new_addr))
		goto out;

	if (new_len > TASK_SIZE || new_addr > TASK_SIZE - new_len ||
	    old_addr > TASK_SIZE - old_len)
		goto out;

	/* Ensure the old/new locations do not overlap */
	if (old_addr + old_len > new_addr && new_addr + new_len > old_addr)
		goto out;

	lru_add_drain_all();
	mmap_write_lock(mm);
	ret = pages_can_be_swapped(mm, old_addr, len, &pages);
	if (ret)
		goto out_release_unlock;

	ret = do_user_swap(mm, old_addr, len, pages, new_addr);
	/* follow_page() above increased the reference */
	for (i = 0; i < len / PAGE_SIZE; i++)
		if (pages[i])
			put_page(pages[i]);

	kvfree(pages);

out_release_unlock:
	mmap_write_unlock(mm);
out:
	return ret;
}

bool uswap_register(struct uffdio_register *uffdio_register, bool *uswap_mode)
{
	if (!static_branch_unlikely(&userswap_enabled))
		return true;
	if (!(uffdio_register->mode & UFFDIO_REGISTER_MODE_USWAP))
		return true;
	uffdio_register->mode &= ~UFFDIO_REGISTER_MODE_USWAP;
	if (uffdio_register->mode != UFFDIO_REGISTER_MODE_MISSING)
		return false;
	*uswap_mode = true;
	return true;
}

/*
 * register the whole vma overlapping with the address range to avoid splitting
 * the vma which could reduce fragmentation.
 */
bool uswap_adjust_uffd_range(struct uffdio_register *uffdio_register,
			     unsigned long *vm_flags, struct mm_struct *mm)
{
	struct vm_area_struct *vma, *cur;
	unsigned long end;
	bool ret = false;

	VMA_ITERATOR(vmi, mm, uffdio_register->range.start);

	end = uffdio_register->range.start + uffdio_register->range.len - 1;

	mmap_read_lock(mm);
	vma = find_vma(mm, uffdio_register->range.start);
	if (!vma || vma->vm_start >= end)
		goto out_unlock;
	for_each_vma_range(vmi, cur, end)
		if (!vma_uswap_compatible(cur))
			goto out_unlock;

	uffdio_register->range.start = vma->vm_start;
	vma = find_vma(mm, end);
	if (vma && end >= vma->vm_start)
		uffdio_register->range.len =
			vma->vm_end - uffdio_register->range.start;

	*vm_flags |= VM_USWAP;

	ret = true;
out_unlock:
	mmap_read_unlock(mm);
	return ret;
}

vm_fault_t do_uswap_page(swp_entry_t entry, struct vm_fault *vmf,
			 struct vm_area_struct *vma)
{
	const char *process_prefix = "uswap";

	/* print error if we come across a nested fault */
	if (!strncmp(current->comm, process_prefix, strlen(process_prefix))) {
		pr_err("USWAP: fault %lx is triggered by %s\n", vmf->address,
		       current->comm);
		return VM_FAULT_SIGBUS;
	}

	if (!(vma->vm_flags & VM_UFFD_MISSING)) {
		pr_err("USWAP: addr %lx flags %lx is not a user swap page",
		       vmf->address, vma->vm_flags);
		return VM_FAULT_SIGBUS;
	}

	return handle_userfault(vmf, VM_UFFD_MISSING);
}

int mfill_atomic_pte_nocopy(struct mm_struct *mm, pmd_t *dst_pmd,
			    struct vm_area_struct *dst_vma,
			    unsigned long dst_addr, unsigned long src_addr)
{
	struct vm_area_struct *src_vma;
	pte_t dst_pte, *pte, src_pte;
	struct page *page;
	spinlock_t *ptl;
	pmd_t *src_pmd;
	int ret;

	src_vma = find_vma(mm, src_addr);
	if (!src_vma || src_addr < src_vma->vm_start)
		return -EINVAL;

	if (!vma_uswap_compatible(src_vma))
		return -EINVAL;

	page = follow_page(src_vma, src_addr, FOLL_GET | FOLL_DUMP);
	if (IS_ERR_OR_NULL(page))
		return -ENODEV;

	ret = -ENXIO;
	src_pmd = mm_find_pmd(mm, src_addr);
	if (!src_pmd)
		goto out_put_page;

	if (!PageLRU(page))
		lru_add_drain_all();

	ret = -EBUSY;
	if (page_mapcount(page) > 1 ||
	    page_mapcount(page) + 1 != page_count(page))
		goto out_put_page;

	ret = uswap_unmap_anon_page(mm, src_vma, src_addr, page, src_pmd,
				    &src_pte, false);
	if (ret)
		goto out_put_page;
	if (dst_vma->vm_flags & VM_USWAP)
		ClearPageDirty(page);
	/*
	 * The memory barrier inside __SetPageUptodate makes sure that
	 * preceding stores to the page contents become visible before
	 * the set_pte_at() write.
	 */
	__SetPageUptodate(page);

	dst_pte = mk_pte(page, dst_vma->vm_page_prot);
	if (dst_vma->vm_flags & VM_WRITE)
		dst_pte = pte_mkwrite_novma(pte_mkdirty(dst_pte));
	if (dst_vma->vm_flags & VM_USWAP)
		dst_pte = pte_mkclean(dst_pte);

	pte = pte_offset_map_lock(mm, dst_pmd, dst_addr, &ptl);
	/*
	 * The userspace may swap in a large area. Part of the area is not
	 * swapped out. If concurrent execution, PTE may be present. Skip those
	 * pages (pte_present).
	 * No other scenes should be handled except first pagefault (pte_none)
	 * and after userswap out (SWP_USERSWAP_ENTRY).
	 */
	if (pte_present(*pte) ||
	    (!pte_none(*pte) && !is_userswap_entry(pte_to_swp_entry(*pte)))) {
		pte_unmap_unlock(pte, ptl);
		uswap_map_anon_page(mm, src_vma, src_addr, page, src_pmd,
				    src_pte);
		ret = -EEXIST;
		goto out_put_page;
	}

	inc_mm_counter(mm, MM_ANONPAGES);
	add_reliable_page_counter(page, mm, 1);
	page_add_new_anon_rmap(page, dst_vma, dst_addr);
	set_pte_at(mm, dst_addr, pte, dst_pte);

	/* No need to invalidate - it was non-present before */
	update_mmu_cache(dst_vma, dst_addr, pte);
	pte_unmap_unlock(pte, ptl);
	ret = 0;

out_put_page:
	put_page(page);
	return ret;
}
