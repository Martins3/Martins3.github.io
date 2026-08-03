#include "internal.h"
#include <linux/swait.h>

/*
 * 先记住结论吧:
 * The most important difference is that the simple waitqueue allows
 * for deterministic behaviour -- IOW it has strictly bounded IRQ and lock hold
 * times.
 *
 * TODO 而所谓的效果是需要测试一下才可以知道。
 */

struct swait_queue_head wq;
static bool signal = false;

/*
 * 参考 : kvm_async_pf_task_wait_schedule
 */
static void basic_test(void)
{
	/*
	 * TODO 包括
	 */
	DECLARE_SWAITQUEUE(wait);
	for (;;) {
		prepare_to_swait_exclusive(&wq, &wait, TASK_UNINTERRUPTIBLE);
		if (signal)
			break;
		schedule();
	}
	finish_swait(&wq, &wait);
}

int test_swait_init(void)
{
	init_swait_queue_head(&wq);
	return 0;
}

int test_swait_exit(void)
{
	return 0;
}

int test_swait(long action)
{
	switch (action) {
	case 0:
		basic_test();
		break;
	case 1:
		signal = true;
		break;
	case 2:
		swake_up_one(&wq);
		break;
	}
	return 0;
}
