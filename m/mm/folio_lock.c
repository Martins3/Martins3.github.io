#include "internal.h"
#include <linux/mm.h>
#include <linux/pagemap.h>

static struct folio *folio;
// TODO folio_lock 上的注释是极好的，使用范围基本说了
// 只等验证一下。
//
// folio_lock 的有趣之处在于，可以用这个来实现 kernel 和 block device 的同步
// filemap_fault ->  __filemap_get_folio -> filemap_get_pages -> filemap_add_folio -> __folio_set_locked(设置page的PG_locked)
// filemap_read_folio -> folio_wait_locked_killable -> folio_wait_bit_killable(folio, PG_locked) –> folio_wait_bit_common(等IO完成PG_locked被清除)
// mpage_read_end_io -> folio_mark_uptodate -> folio_unlock(IO完成时会标记page的PG_update，同时清除PG_locked)。
int test_folio_lock_init(void)
{
	folio = folio_alloc(GFP_USER, 2);
	return !folio;
}

int test_folio_lock_exit(void)
{
	folio_put(folio);
	return 0;
}

int test_folio_lock(long action)
{
	switch (action) {
	case 1:
		folio_test_locked(folio);
		break;
	case 2:
		folio_lock(folio);
		break;
	case 3:
		folio_unlock(folio);
		break;

	// 这个是不太相关的一个，其实是 mprotect
	case 4:
		folio_clear_mlocked(folio);
		break;

	case 5:
		folio_test_mlocked(folio);
		break;
	}
	return 0;
}
