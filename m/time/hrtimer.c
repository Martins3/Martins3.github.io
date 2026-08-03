#include "internal.h"
#include <linux/hrtimer.h>

// TODO 需要使用 hrtimer_cancel 来释放的
static struct hrtimer ht;
static enum hrtimer_restart vmx_preemption_timer_fn(struct hrtimer *timer)
{
	pr_info("-----------------------\n");
	dump_stack();
	pr_info("-----------------------\n");

	return HRTIMER_NORESTART;
}

static void basic_api(void)
{
	hrtimer_setup(&ht, vmx_preemption_timer_fn, CLOCK_MONOTONIC,
		      HRTIMER_MODE_ABS_PINNED);
	hrtimer_start(&ht, ktime_add_ns(ktime_get(), 0),
		      HRTIMER_MODE_ABS_PINNED);
}

static struct hrtimer d;
static enum hrtimer_restart dead_lock_timer_fn(struct hrtimer *timer)
{
	// hrtimer_try_to_cancel(&d);
	hrtimer_cancel(&d);
	return HRTIMER_NORESTART;
}

/*
 * 测试 https://questdb.com/blog/async-profiler-kernel-bug/
 * 中提到的如果在 timer 中 cancel timer ，那么就会卡死
 */
static void dead_lock(void)
{
	hrtimer_setup(&d, dead_lock_timer_fn, CLOCK_MONOTONIC,
		      HRTIMER_MODE_ABS_PINNED);
	hrtimer_start(&d, ktime_add_ns(ktime_get(), 0),
		      HRTIMER_MODE_ABS_PINNED);
}

/*
 *TODO 确认一个事情，crash 中 timer 无法检查 hrtimer ，只能检查 timer 吧 
 */
int test_hrtimer(long action)
{
	switch (action) {
	case 0:
		basic_api();
		break;
	case 1:
		dead_lock();
		break;
	}
	return 0;
}
