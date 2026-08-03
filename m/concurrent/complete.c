#include "internal.h"
#include <linux/completion.h>

static DECLARE_COMPLETION(test);

/**
 * completion 和 wait_event 机制的差别
 * 1. 没有处理信号的接口
 * 2. 没有判断条件的接口
 *
 * complete 和 rwsem 很像，可以提前 complete 来
 *
 *
 *
 */
int test_complete(long action)
{
	bool rc;
	bool to;
	switch (action) {
	case 0:
		// 直接 echo 的程序会卡在这, 并且是 uninterrupt 的
		// 将 done --
		wait_for_completion(&test);
		break;
	case 2:
		rc = try_wait_for_completion(&test);
		pr_info("rc=%d\n", rc);
		break;
	case 3:
		to = wait_for_completion_interruptible_timeout(&test, HZ * 10);
		pr_info("to=%d\n", to);
		break;
	case 4:
		// 一次唤醒一个 ，将 done ++
		complete(&test);
		break;
	case 5:
		// 一次唤醒所有，将 done = INT_MAX
		complete_all(&test);
		break;
	}

	return 0;
}
