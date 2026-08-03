#include "internal.h"
#include <linux/mm.h>

static void vmap_stack(void)
{
	// virt_to_folio 对于 vmap 的地址，其实会触发 kernel panic 的，
	// 因为转换过程中会访问 struct page 中内容
	//
	// struct folio *f1 = virt_to_folio(current->stack);
	// pr_info("folio %px\n", f1);

	// 的确是打开了 VMAP_STACK 的，可以看出 stack 落到了 vmap 上了:
	//  ffffc90000000000 |  -55    TB | ffffe8ffffffffff |   32 TB | vmalloc/ioremap space (vmalloc_base)
	// [60804.572715] current->stack ffffc90011180000
	pr_info("current->stack %px\n", current->stack);
}

static void task_struct_in_stack(void)
{
	/*
	 * [  243.263844] current->stack ffffc90003a8c000
	 * [  243.264239] current ffff888118a4c780

	 */
	pr_info("current->stack %px\n", current->stack);
	pr_info("current %px\n", current);
}

int test_stack(long action)
{
	switch (action) {
	case 0:
		task_struct_in_stack();
		break;
	case 1:
		vmap_stack();
		break;
	}
	return 0;
}
