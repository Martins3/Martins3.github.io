#include "internal.h"
#include <linux/mm.h>
#include <linux/swap.h>

// 在内存回收时扫描 active 和 inactive LRU 链表进行。
// lruvec 结构体中有一个自旋锁保护LRU链表的操作过程的并发问题。
//
// TODO 作为一个 driver 申请的 folio 添加到 lru 中，然后 page 被 swap out :
//
// 那么我的 swap entry 应该放到哪里 ?
static struct folio *folio;
int test_lru_lock_init(void)
{
	folio = folio_alloc(GFP_USER, 2);
	return !folio;
}

int test_lru_lock_exit(void)
{
	folio_put(folio);
	return 0;
}

int test_lru_lock(long action)
{
	switch (action) {
	case 0:
		// 只能添加
		pr_info("lru bit %d", folio_test_lru(folio));
		folio_add_lru(folio);
		pr_info("lru bit %d", folio_test_lru(folio));
		break;
	case 1:
		break;
	}
	return 0;
}
