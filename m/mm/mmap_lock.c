#include "internal.h"
#include <linux/mmap_lock.h>

static void block_page_fualt(void)
{
	/*
	 * mod mmap_lock 0
	 *
	 * 想不到做这个测试的时候，会有这个错误
	 *
	 * TODO 我极度怀疑 acct_collect 中是否真的需要  mmap_read_lock ，他不是在退出吗?
	 *
	 * __schedule+0x55a/0x1380
	 * ? schedule_preempt_disabled+0x54/0xa0
	 * schedule+0xca/0x170
	 * ? down_read+0x239/0x380
	 * ? down_read+0x239/0x380
	 * schedule_preempt_disabled+0x54/0xa0
	 * down_read+0x239/0x380
	 * acct_collect+0xca/0x290
	 * do_exit+0x1ea/0x9b0
	 * ? _raw_spin_unlock_irq+0x29/0x50
	 * do_group_exit+0x92/0xa0
	 * __x64_sys_exit_group+0x17/0x20
	 * x64_sys_call+0x2131/0x2140
	 * do_syscall_64+0xed/0x200
	 * ? exc_page_fault+0xb2/0x1f0
	 * entry_SYSCALL_64_after_hwframe+0x77/0x7f
	 */
	pr_info("before lock mmap_lock_is_contended=%d\n",
		mmap_lock_is_contended(current->mm));
	mmap_write_lock(current->mm);
	pr_info("after lock mmap_lock_is_contended=%d\n",
		mmap_lock_is_contended(current->mm));
}


/* 其他的接口
 * mmap_write_trylock
 * mmap_write_lock_killable
 * mmap_write_lock
 * mmap_write_lock_nested
 */

int test_mmap_lock_init(void)
{
	return 0;
}

int test_mmap_lock_exit(void)
{
	return 0;
}

int test_mmap_lock(long action)
{
	switch (action) {
		// TODO 配合 -user 使用的，现在看，可以 page fault ，但是不可以 mmap
		// 看来 per vma lock 我们有错误的假设，继续看看
		// https://mp.weixin.qq.com/s/yLM5FlYxT06axDoyTeECKg
	case 100:
		block_page_fualt();
		break;
	case 101:
		mmap_write_unlock(current->mm);
		break;
	}
	return 0;
}
