/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2023. All rights reserved.
 */

#ifndef _LINUX_USERSWAP_H
#define _LINUX_USERSWAP_H

#include <linux/mman.h>
#include <linux/userfaultfd.h>
#include <linux/userfaultfd_k.h>
#include <linux/rmap.h>
#include <linux/swapops.h>

extern struct static_key_false userswap_enabled;

// ----- 各种依赖，暂时先拷贝到这里
#define VM_USWAP_BIT 62
#define VM_USWAP BIT(VM_USWAP_BIT)
#define MFILL_ATOMIC_DIRECT_MAP 4
#define UFFDIO_REGISTER_MODE_USWAP ((__u64)1 << 10)

static inline void add_reliable_page_counter(struct page *page,
					     struct mm_struct *mm, int val)
{
}

pmd_t *mm_find_pmd(struct mm_struct *mm, unsigned long address);
pud_t *get_old_pud(struct mm_struct *mm, unsigned long addr);

#define SWP_USERSWAP_NUM 1
#define SWP_USERSWAP_ENTRY                                      \
	(MAX_SWAPFILES + SWP_HWPOISON_NUM + SWP_MIGRATION_NUM + \
	 SWP_DEVICE_NUM + SWP_PTE_MARKER_NUM)

static inline int is_userswap_entry(swp_entry_t entry)
{
	if (!static_branch_unlikely(&userswap_enabled))
		return 0;
	return unlikely(swp_type(entry) == SWP_USERSWAP_ENTRY);
}
// -----  拷贝结束

/*
 * In uswap situation, we use the bit 0 of the returned address to indicate
 * whether the pages are dirty.
 */
#define USWAP_PAGES_DIRTY 1

unsigned long uswap_mremap(unsigned long old_addr, unsigned long old_len,
			   unsigned long new_addr, unsigned long new_len);

bool uswap_register(struct uffdio_register *uffdio_register, bool *uswap_mode);

bool uswap_adjust_uffd_range(struct uffdio_register *uffdio_register,
			     unsigned long *vm_flags, struct mm_struct *mm);

vm_fault_t do_uswap_page(swp_entry_t entry, struct vm_fault *vmf,
			 struct vm_area_struct *vma);

int mfill_atomic_pte_nocopy(struct mm_struct *dst_mm, pmd_t *dst_pmd,
			    struct vm_area_struct *dst_vma,
			    unsigned long dst_addr, unsigned long src_addr);

static inline void uswap_must_wait(unsigned long reason, pte_t pte, bool *ret)
{
	if (!static_branch_unlikely(&userswap_enabled))
		return;
	if ((reason & VM_USWAP) && (!pte_present(pte)))
		*ret = true;
}

static inline bool uswap_check_copy(struct vm_area_struct *vma,
				    uffd_flags_t flags)
{
	if (!!uffd_flags_mode_is(flags, MFILL_ATOMIC_DIRECT_MAP) ^
	    !!(vma->vm_flags & VM_USWAP))
		return false;
	return true;
}

static inline void uswap_get_cpu_id(unsigned long reason, struct uffd_msg *msg)
{
	if (reason & VM_USWAP)
		msg->reserved3 = smp_processor_id();
}

#endif /* CONFIG_USERSWAP */
