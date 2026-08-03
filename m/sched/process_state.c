#include "internal.h"
#include <linux/module.h>
#include <linux/poll.h>

// msleep 可以替换为
// if(schedule_timeout_interruptible){
//     break;
// }

static void loop_wait1(void)
{
	/*
	 * 执行到 schedule 后卡主，当使用 control c 之后，
	 * 剩下的 9 次立刻执行完
	 */
	for (size_t i = 0; i < 10; i++) {
		set_current_state(TASK_INTERRUPTIBLE);
		pr_info("[%s:%d] %d\n", __FUNCTION__, __LINE__, current->pid);
		schedule();
	}
}

static void loop_wait2(void)
{
	/* 立刻执行完，原理参考 ./sched.c
	 */
	for (size_t i = 0; i < 10; i++) {
		pr_info("[%s:%d] %d\n", __FUNCTION__, __LINE__, current->pid);
		schedule();
	}
}

static void loop_wait4(void)
{
	/*
	 * 不在响应 control c ，也不响应 kill -9
	 *
	 * 这种状态除了重启，或被其他 process wake ，没其他办法了
	 */
	for (size_t i = 0; i < 10; i++) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		pr_info("[%s:%d] %d\n", __FUNCTION__, __LINE__, current->pid);
		schedule();
	}
}

static void loop_wait5(void)
{
	/*
	 * 也 loop_wait1 一样，如果 ctrl-c ，那么后续的 schedule 不在阻塞
	 */
	for (size_t i = 0; i < 10; i++) {
		set_current_state(TASK_KILLABLE);
		pr_info("[%s:%d] %d\n", __FUNCTION__, __LINE__, current->pid);
		schedule();
	}
}

int test_process_state(long action)
{
	switch (action) {
	case 1:
		loop_wait1();
		break;
	case 2:
		loop_wait2();
		break;
	case 4:
		loop_wait4();
		break;
	case 5:
		loop_wait5();
		break;
	default:
		return 1;
	}

	return 0;
}
