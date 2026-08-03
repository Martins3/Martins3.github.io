#include "internal.h"
#include <linux/mm.h>

// 这几个函数都差不多，注释上有描述了:
// get_user_pages_fast
// pin_user_pages_remote
// pin_user_pages

// TODO 测试 FOLL_PIN 的效果
// enum {
// 	/* mark page accessed */
// 	FOLL_TOUCH = 1 << 16,
// 	/* a retry, previous pass started an IO */
// 	FOLL_TRIED = 1 << 17,
// 	/* we are working on non-current tsk/mm */
// 	FOLL_REMOTE = 1 << 18,
// 	/* pages must be released via unpin_user_page */
// 	FOLL_PIN = 1 << 19,
// 	/* gup_fast: prevent fall-back to slow gup */
// 	FOLL_FAST_ONLY = 1 << 20,
// 	/* allow unlocking the mmap lock */
// 	FOLL_UNLOCKABLE = 1 << 21,
// 	/* VMA lookup+checks compatible with MADV_POPULATE_(READ|WRITE) */
// 	FOLL_MADV_POPULATE = 1 << 22,
// };

// drivers/vfio/vfio_iommu_type1.c 中的 vaddr_get_pfns
// 测试下 pin_user_pages_remote follow_fault_pfn

/*
 *
 * [   14.168918]  out_of_memory+0x3af/0x580
 * [   14.168918]  __alloc_pages_may_oom+0xf8/0x1b0
 * [   14.168918]  __alloc_pages_slowpath+0x435/0x5f0
 * [   14.168918]  __alloc_pages_noprof+0x270/0x350
 * [   14.168918]  alloc_pages_mpol_noprof+0x123/0x210
 * [   14.168918]  vma_alloc_folio_noprof+0x121/0x190
 * [   14.168918]  do_pte_missing+0xae6/0xf90
 * [   14.168918]  ? do_huge_pmd_anonymous_page+0x164/0x7e0
 * [   14.168918]  handle_mm_fault+0x837/0x12d0
 * [   14.168918]  __get_user_pages+0x76b/0xeb0
 * [   14.168918]  __gup_longterm_locked+0x4fc/0x7f0
 * [   14.168918]  ? __vmap_pages_range_noflush+0x4f4/0x520
 * [   14.168918]  gup_fast_fallback+0xd2c/0xf90
 * [   14.168918]  test_gup+0x87/0xb0 [martins3]
 * [   14.168918]  gup_store+0xdb/0x170 [martins3]
 * [   14.168918]  kernfs_fop_write_iter+0xed/0x170
 * [   14.168918]  vfs_write+0x399/0x480
 * [   14.168918]  ksys_write+0x72/0xe0
 * [   14.168918]  do_syscall_64+0xed/0x200
 * [   14.168918]  ? exc_page_fault+0x9e/0x130
 * [   14.168918]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
 */

static struct page **get_pages(ulong nr)
{
	/*
	 * 如果使用 kmalloc_array ，如果 pages 的数量太多，会超过 buddy system 
	 * 单次分配数量的上限。
	 * pages = kmalloc_array(nr, sizeof(struct page *), GFP_KERNEL);
	 */
	struct page **pages;
	int plus_one;
	int array_pages =
		div_u64_rem(sizeof(struct page *) * nr, PAGE_SIZE, &plus_one);
	if (plus_one)
		array_pages++;

	// 存储了一堆 struct page 指针，get_user_pages_fast 的工作是提供 struct page 的地址
	pages = kvcalloc(PAGE_SIZE, array_pages,
			 GFP_KERNEL_ACCOUNT | __GFP_RETRY_MAYFAIL);
	if (!pages)
		return NULL;
	return pages;
}

static int test_get_user_pages_fast(void)
{
	/*
	 * 如果 gup_flags 没有 FOLL_WRITE ，那么 gup 其实不会去 pin 任何 pages
	 */
	int ret;
	int gup_flags = FOLL_WRITE;

	ulong addr = get_parameter(0);
	ulong nr = get_parameter(1);
	struct page **pages = get_pages(nr);

	pr_info("pin %ld pages at %lx\n", nr, addr);
	/*
	 * 被 pin 的 page 挂掉不会自动释放，而是会一直放到哪里
	 * 这是正常的机制下，最后释放的 page 的过程
	 * 
         * vfio_unpin_pages_remote+1
         * vfio_sync_unpin.isra.0+149
         * vfio_unmap_unpin+757
         * vfio_remove_dma+42
         * vfio_iommu_type1_detach_group+1521
         * vfio_group_detach_container+80
         * vfio_group_fops_release+66
         * __fput+222
         * task_work_run+89
         * do_exit+793
         * do_group_exit+48
         * get_signal+2286
         * arch_do_signal_or_restart+62
         * syscall_exit_to_user_mode+487
         * do_syscall_64+206
         * entry_SYSCALL_64_after_hwframe+119
	 */
	// TODO 这两个函数似乎都是一样的，注释上也是这么说的
	/* ret = get_user_pages_fast(addr, nr, gup_flags, pages); */
	ret = get_user_pages_unlocked(addr, nr, pages, gup_flags);
	if (ret != nr) {
		pr_info("pin %ld pages get %d\n", nr, ret);
		return ret;
	}
	for (size_t i = 0; i < 3; i++) {
		struct page *p = pages[i];
		// 这里打印的 refcount 是 2
		// 所以 get_user_pages_fast 得到的 page 都是无法换出的
		pr_info("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__,
			page_ref_count(p));
	}

	// 这里调用了 put_page 之后，当程序结束，那么这些 page 立刻释放
	for (size_t i = 0; i < nr; i++) {
		struct page *p = pages[i];
		put_page(p);
	}
	pr_info("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__, ret);
	return 0;
}

static void test_pin_user_pages_fast(void)
{
	unsigned long npinned;
	ulong uaddr = get_parameter(0);
	ulong npages = get_parameter(1);
	struct page **pages = get_pages(npages);
	npinned = pin_user_pages_fast(uaddr, npages, FOLL_WRITE, pages);
	pr_info("npages=%ld npinned=%ld\n", npages, npinned);
}

int test_gup(long action)
{
	switch (action) {
	case 0:
		test_get_user_pages_fast();
		break;
	case 2:
		test_pin_user_pages_fast();
		break;
	}
	return 0;
}
