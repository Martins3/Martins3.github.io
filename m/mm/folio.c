#define pr_fmt(fmt) "folio : " fmt

#include "internal.h"
#include <linux/gfp.h>
#include <linux/mm.h>
#include <asm/io.h>

/*
 * 为什么需要 folio ，因为一个函数以前的时候接受到参数为 page ，那么他需要自己判断，
 * 这个 page 是不是 compound page ，是 head 还是 tail ，就算是传递过来的是 compound page 的 head ，
 * 这个函数到底是要处理整个 compound page 还是只是为了处理第一个 page ，但是如果使用的是 folio ，其
 * 意图将会非常清晰。
 */
static void test_basic(void)
{
	struct folio *folio = folio_alloc(GFP_USER, 2);
	struct page *page = alloc_pages(GFP_USER, 2);
	struct page *page2 = alloc_pages(GFP_USER | __GFP_COMP, 2);
	/* 只有设置了 __GFP_COMP 的才被称之为 compound page */
	pr_info("folio %d \n", folio_order(folio));
	pr_info("page without COMP %d \n", compound_order(page));
	pr_info("page with COMP %d \n", compound_order(page2));
	/* 由于信息丢失，通过 page_folio 转换过来的 folio 不知道发生了什么 */
	pr_info("%d \n", folio_order(page_folio(page)));
	pr_info("%d \n", folio_order(page_folio(page2)));

	pr_info("nr pages");
	pr_info("%ld \n", folio_nr_pages(folio));
	pr_info("%ld \n", folio_nr_pages(page_folio(page)));
	pr_info("%ld \n", folio_nr_pages(page_folio(page2)));

	/* folio_page 不会检查边界，从 folio 中间取到的 page 没有原始信息 */
	/* compound_order 只是适用于 compound page */
	pr_info("compound_order\n");
	pr_info("%d %px\n", compound_order(folio_page(folio, 0)),
		folio_page(folio, 0));
	pr_info("%d %px\n", compound_order(folio_page(folio, 1)),
		folio_page(folio, 1));
	pr_info("%d %px\n", compound_order(folio_page(folio, 2)),
		folio_page(folio, 2));
	pr_info("%d %px\n", compound_order(folio_page(folio, 3)),
		folio_page(folio, 3));
	pr_info("%d %px\n", compound_order(folio_page(folio, 4)),
		folio_page(folio, 4));

	/* compound_order 可以用于 tail page
	 */
	pr_info("PageCompound\n");
	pr_info("%d \n", PageCompound(folio_page(folio, 0)));
	pr_info("%d \n", PageCompound(folio_page(folio, 1)));
	pr_info("%d \n", PageCompound(folio_page(folio, 2)));
	pr_info("%d \n", PageCompound(folio_page(folio, 3)));
	pr_info("%d \n", PageCompound(folio_page(folio, 4)));

	/* 任何一个 tail page 都是经过 page_folio 装换之后，得到都是原始的 folio */
	/* 因为 folio_page 没有检查边界，所以前四个输出可以正确指向到 head ，但是第五个不可以*/
	pr_info("~~\n");
	pr_info("%px \n", page_folio(folio_page(folio, 0)));
	pr_info("%px \n", page_folio(folio_page(folio, 1)));
	pr_info("%px \n", page_folio(folio_page(folio, 2)));
	pr_info("%px \n", page_folio(folio_page(folio, 3)));
	pr_info("%px \n", page_folio(folio_page(folio, 4)));

	pr_info("PageHead\n");
	pr_info("%d \n", PageHead(folio_page(folio, 0)));
	pr_info("%d \n", PageHead(folio_page(folio, 1)));
	pr_info("%d \n", PageHead(folio_page(folio, 2)));
	pr_info("%d \n", PageHead(folio_page(folio, 3)));
	pr_info("%d \n", PageHead(folio_page(folio, 4)));
}

static void test_basic2(void)
{
	// 测试一些基本转换函数
	struct folio *folio = folio_alloc(GFP_USER, 2);

	// page_address 是不考虑 compound 信息的
	pr_info("%px \n", folio_address(folio));
	pr_info("%px \n", page_address(folio_page(folio, 0)));
	pr_info("%px \n", page_address(folio_page(folio, 1)));

	pr_info("page_zone :%s \n", folio_zone(folio)->name);
	pr_info("page_pgdat :%d \n", folio_pgdat(folio)->node_id);

	pr_info("folio_pfn :%ld \n", folio_pfn(folio));

	for (size_t i = 0; i < 5; i++) {
		// 通过 address 对应的 page struct ，这些 page struct 中含有信息告诉自己那个 compound page 中
		pr_info("%ld %px \n", i,
			virt_to_folio(page_address(folio_page(folio, i))));
	}
	/* folio_address 类似 page_address */
}

/*
 * 在驱动中分配 hugepage 有点难:
 *
 * hugetlb 的实现 : dequeue_hugetlb_folio_node_exact
 * transparent huge page 的实现  : do_huge_pmd_anonymous_page 中的 vma_alloc_folio
 *
 * 可以确定 transparent 和 hugepage 都是使用的 folio 来描述一个页
 */
static void test_hugepage(void)
{
	// TODO
	// 测试下 thp_nr_pages  和 folio_nr_pages 什么关系?
	// 为什么 thp 不直接复用 folio 啊？
}

static void test_refcount(void)
{
	struct folio *folio = folio_alloc(GFP_USER, 2);
	pr_info("init ref : %d\n", folio_ref_count(folio));
	pr_info("init ref : %d\n", folio_mapped(folio));
	// folio_get 只是比 folio_ref_inc 多了一个检查
	folio_ref_inc(folio);
	folio_get(folio);
	pr_info("ref : %d\n", folio_ref_count(folio));
	pr_info("map : %d\n", folio_mapped(folio));
	folio_put(folio);
	pr_info("ref : %d\n", folio_ref_count(folio));
	pr_info("map : %d\n", folio_mapped(folio));

	pr_info("map : %d\n", folio_mapcount(folio));
}

static void test_rmap(void)
{
	/*
	 * TODO
	 * 1. 作为一个驱动需要 explicit 的管理 rmap 的问题吗?
	 * 或者说，如果一个驱动将一个区域同时共享给多个用户地址空间，有什么需要注意的吗?
	 * 2. 可以写一些用户态程序来测试 rmap 的行为，例如 anonymous 内存的处理
	 *
	 *
	 * 仔细研究下这个吧:
static const struct file_operations io_uring_fops = {
	.release	= io_uring_release,
	.mmap		= io_uring_mmap,
#ifndef CONFIG_MMU
	.get_unmapped_area = io_uring_nommu_get_unmapped_area,
	.mmap_capabilities = io_uring_nommu_mmap_capabilities,
#else
	.get_unmapped_area = io_uring_mmu_get_unmapped_area,
#endif
	.poll		= io_uring_poll,
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= io_uring_show_fdinfo,
#endif
};
	 *
	 *
	 */
}

int test_folio_init(void)
{
	return 0;
}

int test_folio_exit(void)
{
	return 0;
}

int test_folio(long action)
{
	switch (action) {
	case 0:
		break;
	case 1:
		test_basic();
		break;
	case 2:
		test_basic2();
		break;
	case 3:
		test_refcount();
		break;
	case 5:
		test_rmap();
		break;
	case 6:
		test_hugepage();
		break;
	default:
		break;
	}
	return 0;
}
