#include <linux/task_work.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/task_work.h>
#include "internal.h"

/*
 * 血崩，include/linux/task_work.h 中的符号全部都没有被 export
 * 出来。需要在 kernel/task_work.c:task_work_add 下添加
 * EXPORT_SYMBOL_GPL(task_work_add);
 * 这个内核才可以编译。
 * 
 * 实现原理并不难，就是把任务通过 task_work_add() 挂到链表中
 * 当程序执行 task_work_run() 的时候，会自动来执行这些 callback 。
 */

static struct callback_head demo_work;

static void demo_task_work_callback(struct callback_head *twork)
{
	pr_info("task_work executed in process %s (pid=%d)\n", current->comm,
		current->pid);
	pr_info("in_interrupt=%ld\n", in_interrupt());
	/*
	 * 当 syscall 要结束的时候执行，所以还是在 process context 中:
	 *
	 * [  599.706281] Call Trace:
	 * [  599.706284]  <TASK>
	 * [  599.706301]  dump_stack_lvl+0x6f/0xb0
	 * [  599.706313]  task_work_run+0x59/0xa0
	 * [  599.706319]  get_signal+0x88/0xba0
	 * [  599.706331]  arch_do_signal_or_restart+0x3a/0x200
	 * [  599.706350]  exit_to_user_mode_loop+0x77/0xe0
	 * [  599.706357]  do_syscall_64+0x2b1/0x3a0
	 * [  599.706369]  entry_SYSCALL_64_after_hwframe+0x76/0x7e
	 */
	dump_stack();
}

static int add_work_to_current(void)
{
	int ret;
	init_task_work(&demo_work, demo_task_work_callback);
	// 使用 TWA_SIGNAL 模式：会设置 TIF_NOTIFY_SIGNAL 并尝试唤醒（如果睡眠）
	ret = task_work_add(current, &demo_work, TWA_SIGNAL);
	if (ret)
		return ret;
	for (size_t i = 0; i < 5; i++)
		msleep(1000);
	return 0;
}

int test_task_work(long action)
{
	switch (action) {
	case 0:
		return add_work_to_current();
	}
	return 0;
}
