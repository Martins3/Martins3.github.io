#include "internal.h"

#include <linux/timer.h>
#include <linux/timerfd.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/version.h>

static struct timer_list tl;
static struct timer_list endless;

static void timer_fn(struct timer_list *timer)
{
	pr_info("-----------------------\n");
	dump_stack();
}

// 如果在 softirq 中继续触发中断
// 这个是 jiffies 驱动，如果中间没有事情做，很难卡主的
static unsigned long jiffies_at_begin;
static unsigned long counter;
static void endless_timer_fn(struct timer_list *timer)
{
	if (time_before(jiffies, jiffies_at_begin + HZ * 1000))
		mod_timer(&endless, jiffies + usecs_to_jiffies(100));
	else
		pr_info("looped %ld\n", counter);
	mdelay(3);
	counter ++;
}

int test_timer_init(void)
{
	timer_setup(&tl, timer_fn, 0);
	timer_setup(&endless, endless_timer_fn, 0);
	return 0;
}

int test_timer_exit(void)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(6, 15, 0))
	timer_delete_sync(&tl);
	timer_delete_sync(&endless);
#else
	del_timer_sync(&tl);
	del_timer_sync(&endless);
#endif
	return 0;
}

int test_timer(long action)
{
	switch (action) {
	case 1:
		// 其实从这个接口就可以看出来，timer 的时间最小单位 1s / HZ
		tl.expires = jiffies + usecs_to_jiffies(1000 * 3);
		add_timer(&tl);
		break;
	case 2:
		// 测试在特定的 cpu 上执行
		tl.expires = jiffies + usecs_to_jiffies(1000 * 3);
		add_timer_on(&tl, 2);
		break;
	case 3:
		// 测试在 timer 中触发 softirq 从而反复触发中断
		jiffies_at_begin = jiffies;
		counter = 0;
		add_timer_on(&endless, 2);
		break;
	}
	return 0;
}
