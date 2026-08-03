#include "internal.h"
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/list_lru.h>
/**
 * __alloc_pages_noprof 中会检查的，直接失败的
 * */
static void alloc_too_many_pages(void)
{
	for (size_t i = 0; i < MAX_PAGE_ORDER + 2; i++) {
		struct folio *folio = folio_alloc(GFP_USER, i);
		pr_info("%px", folio);
		if (folio)
			folio_put(folio);
		else
			pr_info("failed order %ld\n", i);
	}
}

struct apple {
	struct list_head lru; /* LRU list */
};

static void test_list_lru(void)
{
	// TODO 没有想象的那么简单
	//
	//  1. list_lru_walk_one : walk 是什么含义
	//  2. list_lru_isolate 和 list_lru_del 区别是什么?
	//  3. 如何配合 shrink 使用 ?
	struct list_lru s_dentry_lru;
	struct apple *a = kmalloc(sizeof(struct apple), GFP_USER);
	list_lru_init(&s_dentry_lru);
	list_lru_add_obj(&s_dentry_lru, &a->lru);
}

int test_mm_misc(long action)
{
	switch (action) {
	case 0:
		alloc_too_many_pages();
		break;
	case 1:
		test_list_lru();
		break;
	case 100:
		// 直接 panic
		folio_put(NULL);
		break;
	}
	return 0;
}
