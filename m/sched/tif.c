#define pr_fmt(fmt) "tif: " fmt
#include <linux/sched/signal.h>
#include "internal.h"
// 测试下 task_struct::thread_info 里面的各个 flags

/*
 *
 * # cond_resched 不一定会导致被切换走
 * <!-- cc1d8215-ed21-4379-a6c8-1c37f0b34632 -->
 *
 * 哦，终于理解 TIF_NEED_RESCHED 的意义了，当没有 CPU 竞争的时候，
 * test_need_resched 会 while 循环较长时间 (5s 左右) ，直到 scheduler 
 * 把他换走。
 *
 * 也就是 cond_resched() 会调用到 __schedule() ，可能导致当前的任务被切换走
 * 
 * 也就是，为了防止 softlock ，那么就使用 need_resched() 来检查，
 * 如果没事情做了，需要等待，那么调用 cond_resched()
 *
 * 这个例子还说明了一个事情，当内核是非抢占的，那么即便是检查到了
 * need_resched() ，但是可以坚持不放弃，
 */
static void test_need_resched(void)
{
	pr_info("start\n");
	while (!need_resched()) {
		cpu_relax();
	}
	pr_info("done\n");
}

/*
 * 然后来检查一下 /proc/$pid/status 看 context switch
 * 
 * 然后使用 sudo perf top -e sched:sched_switch ，
 * 调用 cond_resched() 不一定会导致当前程序被切换走。
 *
 */
static void test_need_resched2(void)
{
	u64 loop_times = 0;
	pr_info("start\n");
	u64 v0 = current->nvcsw;
	u64 iv0 = current->nivcsw;
	for (;;) {
		cond_resched();
		if (signal_pending(current))
			break;
		loop_times++;
	}
	u64 v1 = current->nvcsw;
	u64 iv1 = current->nivcsw;
	/*
	 * 测试结果:
	 * [223852.614966] tif: done, looped=1312964044, nvcsw=0, nivcsw=41
	 */
	pr_info("done, looped=%lld, nvcsw=%llu, nivcsw=%llu\n", loop_times,
		v1 - v0, iv1 - iv0);
}

int test_tif(long action)
{
	switch (action) {
	case 0:
		test_need_resched();
		break;
	case 1:
		test_need_resched2();
		break;
	}
	return 0;
}
