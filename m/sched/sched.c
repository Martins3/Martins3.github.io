#include "internal.h"
#include <linux/delay.h>
/**
 *
 * 参考 schedule_timeout 的注释，但是这一段分析显然是错的:
 *
 * Make the current task sleep until @timeout jiffies have elapsed.
 * The function behavior depends on the current task state
 * (see also set_current_state() description):
 *
 * 问题的关键是 : pick next task 之类的，但是暂时没找到
 *
 * long schedule_timeout(long timeout);
 * long schedule_timeout_interruptible(long timeout);
 * long schedule_timeout_killable(long timeout);
 * long schedule_timeout_uninterruptible(long timeout);
 * long schedule_timeout_idle(long timeout);
 *
 * msleep 只是 schedule_timeout 的封装
 *
 * */

static void test_schedule_timeout(void)
{
	schedule_timeout_interruptible(HZ);
	schedule_timeout(HZ);
	schedule_timeout_uninterruptible(HZ);
}

static void list_all(void)
{
	struct task_struct *p;
	struct task_struct *t;
	rcu_read_lock();
	for_each_process_thread(p, t)
		pr_info("PID: %d, comm: %s\n", t->pid, t->comm);
	rcu_read_unlock();
}

int test_sched(long action)
{
	switch (action) {
	case 1:
		test_schedule_timeout();
		break;
	case 2:
		list_all();
		break;
	case 3:
		break;
	}
	return 0;
}
