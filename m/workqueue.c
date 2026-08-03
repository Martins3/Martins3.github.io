#include "internal.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/ktime.h>
#include <linux/jiffies.h>

static struct workqueue_struct *wq;

/*
 * 一个 work 中的函数是不可以动态切换，使用 DECLARE_DELAYED_WORK 或者 INIT_DELAYED_WORK 之后无法修改
 *
 * 利用 workqueue 执行周期性操作
 * */
static void loop_handler(struct work_struct *w);
static DECLARE_DELAYED_WORK(wq_struct, loop_handler);
static void loop_handler(struct work_struct *w)
{
	pr_info("loop work %u jiffies\n", (unsigned)HZ);
	queue_delayed_work(wq, &wq_struct, HZ);
}

static void system_work_handler(struct work_struct *w);
static DECLARE_DELAYED_WORK(sys_work, system_work_handler);
static void system_work_handler(struct work_struct *w)
{
	pr_info("loop work %u jiffies\n", HZ);
	schedule_delayed_work(&sys_work, HZ);
}

static void sleep_work(struct work_struct *w);
static DECLARE_DELAYED_WORK(sleep_work_struct, sleep_work);
static void sleep_work(struct work_struct *w)
{
	pr_info("sleep work start\n");
	msleep(10000);
	pr_info("sleep work finished\n");
}
static void no_sleep_work(struct work_struct *w);
static DECLARE_DELAYED_WORK(no_sleep_work_struct, no_sleep_work);
static void no_sleep_work(struct work_struct *w)
{
	pr_info("no sleep work finished\n");
}

static void test_max_active_in_workqueue(void)
{
	queue_delayed_work(wq, &sleep_work_struct, HZ);
	queue_delayed_work(wq, &no_sleep_work_struct, 2 * HZ);
}

/*
 * 并没有什么神奇的事情发生，简简单单的触发 softlock 而已
 */
static void dead_loop_work(struct work_struct *w);
static DECLARE_DELAYED_WORK(dead_loop_work_struct, dead_loop_work);
static void dead_loop_work(struct work_struct *w)
{
	for (;;)
		cpu_relax();
}
static void test_dead_loop_in_workqueue(void)
{
	queue_delayed_work(wq, &dead_loop_work_struct, 1 * HZ);
}

static DECLARE_WORK(sleep_work_struct_nodelay, sleep_work);

int test_workqueue_init(void)
{
	wq = alloc_workqueue("my_workqueue", WQ_UNBOUND | WQ_SYSFS, 1);
	if (wq == NULL)
		return -ENOMEM;
	return 0;
}

int test_workqueue_exit(void)
{
	cancel_delayed_work_sync(&wq_struct);
	cancel_work_sync(&sleep_work_struct_nodelay);
	destroy_workqueue(wq);
	return 0;
}

struct work_struct common_work;
static void atomic_leaked(struct work_struct *work)
{
	unsigned long jiffies_at_begin = jiffies;
	while (time_after(jiffies_at_begin + HZ * 3, jiffies)) {
		int *new = (int *)kzalloc(sizeof(int), GFP_KERNEL);
		synchronize_rcu();
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		kfree(new);
	}
}

struct rcu_work destroy_work;
static void css_free_rwork_fn(struct work_struct *work)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
}
static void test_rcu_workqueue(void)
{
	INIT_RCU_WORK(&destroy_work, css_free_rwork_fn);
	queue_rcu_work(wq, &destroy_work);
}

/*
 * 测试 system_power_efficient_wq 的睡眠精度
 * 目标：在指定的 nsec (0.5s) 时间点精确唤醒
 */
static struct timespec64 sched_target_time;
static unsigned long sched_target_nsec;
static bool sched_sync_fail;

static void sched_sync_work_handler(struct work_struct *w);
static DECLARE_DELAYED_WORK(sched_sync_work, sched_sync_work_handler);

/* 将纳秒延迟转换为 jiffies */
static inline unsigned long nsec_delay_to_jiffies(long nsec)
{
	/* 使用 nsecs_to_jiffies，但确保至少延迟 1 个 jiffy */
	unsigned long j = nsecs_to_jiffies(nsec);
	return j ? j : 1;
}

/*
 * 计算下一个目标时间点和延迟
 * @now: 当前时间
 * @delay_nsec: 输出参数，返回需要延迟的纳秒数
 * @delay_jiffies: 输出参数，返回需要延迟的 jiffies
 */
static void calc_next_target(const struct timespec64 *now, long *delay_nsec,
			     unsigned long *delay_jiffies)
{
	/* 计算下一个 0.5s 目标点 */
	sched_target_time.tv_sec = now->tv_sec;
	sched_target_time.tv_nsec = sched_target_nsec;

	/* 如果目标时间已过，调整到下一秒 */
	if (sched_target_time.tv_nsec <= now->tv_nsec) {
		sched_target_time.tv_sec++;
	}

	/* 计算需要延迟的纳秒数 */
	*delay_nsec = (long)(sched_target_time.tv_nsec - now->tv_nsec);
	if (*delay_nsec < 0) {
		*delay_nsec += NSEC_PER_SEC;
	}

	/* 转换为 jiffies */
	*delay_jiffies = nsec_delay_to_jiffies(*delay_nsec);
}

static void sched_sync_work_handler(struct work_struct *w)
{
	struct timespec64 now;
	s64 delta_ns;
	unsigned long delta_jiffies;
	long delay_nsec;
	unsigned long delay_jiffies;

	ktime_get_real_ts64(&now);

	/* 计算与目标时间的偏差 */
	delta_ns = (s64)(now.tv_sec - sched_target_time.tv_sec) * NSEC_PER_SEC +
		   (s64)(now.tv_nsec - sched_target_time.tv_nsec);

	/* 将偏差转换为 jiffies */
	delta_jiffies = nsecs_to_jiffies(abs(delta_ns));

	pr_info("[sched_sync] 目标时间: %lld.%09ld, 实际时间: %lld.%09ld\n",
		sched_target_time.tv_sec, sched_target_time.tv_nsec, now.tv_sec,
		now.tv_nsec);

	/* 如果偏差在 ±5 jiffies 范围内，结束测试 */
	if (delta_jiffies <= 5) {
		pr_info("[sched_sync] 偏差在 ±5 jiffies 范围内 (%lu jiffies)，测试成功结束\n",
			delta_jiffies);
		sched_sync_fail = true;
		return;
	}

	if (!sched_sync_fail) {
		calc_next_target(&now, &delay_nsec, &delay_jiffies);

		queue_delayed_work(system_power_efficient_wq, &sched_sync_work,
				   delay_jiffies);
	} else {
		pr_info("[sched_sync] 测试结束\n");
	}
}

static void test_sched_sync_hw_clock(void)
{
	struct timespec64 now;
	long delay_nsec;
	unsigned long delay_jiffies;

	sched_target_nsec = NSEC_PER_SEC / 2; /* 0.5s = 500ms */
	sched_sync_fail = false;

	/* 取消之前的 work（如果存在） */
	cancel_delayed_work_sync(&sched_sync_work);

	ktime_get_real_ts64(&now);

	calc_next_target(&now, &delay_nsec, &delay_jiffies);

	pr_info("[sched_sync] 开始测试，当前时间: %lld.%09ld, 目标: %lld.%09ld, 延迟: %ld ns (%lu jiffies)\n",
		now.tv_sec, now.tv_nsec, sched_target_time.tv_sec,
		sched_target_time.tv_nsec, delay_nsec, delay_jiffies);

	queue_delayed_work(system_power_efficient_wq, &sched_sync_work,
			   delay_jiffies);
}

static void stop_sched_sync_test(void)
{
	sched_sync_fail = true;
	cancel_delayed_work_sync(&sched_sync_work);
	pr_info("[sched_sync] 测试已停止\n");
}

/*
 * 1. INIT_DELAYED_WORK 是 DECLARE_DELAYED_WORK 的动态执行版本
 *	- 同样的，INIT_WORK 只是 DECLARE_WORK 的
 * 2. queue_delayed_work 和 queue_work 实际上区别不大，只是加入队列前会睡眠一小会
 * 3. max_active : 如果等于 1 ，那么即使第一个 work 睡眠了，第二个 work 也无法执行
 * 4. flush_workqueue : 等待 wq 中的工作做完，在 flush_workqueue 开始之后的任务不等待
 *
 * FIXME
 * 1. 为什么 delay work 和 work 不能统一起来
 * 2. 有办法测试 worker pool 的概念吗?
 */
int test_workqueue(long action)
{
	switch (action) {
	/*
	 * action 1 : 将任务挂到 system wq 上
	 * action 2 : 将任务挂到自己构建的 wq 上
	 */
	case 1:
		schedule_delayed_work(&sys_work, HZ);
		break;
	case 2:
		queue_delayed_work(wq, &wq_struct, HZ);
		break;
	/*
	 * 测试 workqueue 的 max active 参数，需要调整 test_workqueue_init 来分析
	 */
	case 3:
		test_max_active_in_workqueue();
		break;
	/*
	 * loop dead
	 */
	case 4:
		test_dead_loop_in_workqueue();
		break;
	/*
	 * 测试 flush work 的效果
	 */
	case 7:
		queue_delayed_work(wq, &sleep_work_struct, HZ);
		break;
	case 8:
		flush_delayed_work(&sleep_work_struct);
		break;

	case 10:
		queue_work(wq, &sleep_work_struct_nodelay);
		break;
	case 11:
		flush_work(&sleep_work_struct_nodelay);
		break;
	case 12:
		/*
		 * flush_workqueue 需要所有的认为做完
		 */
		flush_workqueue(wq);
		break;

	/*
	 * 测试反复 queue 同一个 work 。
	 * 1. 如果已经 queue 上了，那么后续的 queue 不会有效果
	 * 2. arch/x86/kvm/x86.c:kvm_gen_kvmclock_update 利用 workqueue 的这个机制来实现 ratelimit 的。
	 */
	case 13:
		queue_work(wq, &sleep_work_struct_nodelay);
		queue_work(wq, &sleep_work_struct_nodelay);
		queue_work(wq, &sleep_work_struct_nodelay);
		break;
	case 14:
		queue_delayed_work(wq, &sleep_work_struct, HZ);
		queue_delayed_work(wq, &sleep_work_struct, HZ * 2);
		queue_delayed_work(wq, &sleep_work_struct, HZ * 4);
		break;

	case 15:
		INIT_WORK(&common_work, atomic_leaked);
		schedule_work(&common_work);
		break;
	case 16:
		/*
		 * 测试 rcu workqueue ，就是需要让 work 在 a full RCU grace period
		 * 之后被 queue 上去，其余用起来都是一样的。
		 */
		test_rcu_workqueue();
		break;
	case 17:
		/*
		 * TODO 测试一个东西
		 * system_unbound_wq 的并发是 2048 ，如果一共只有 8 个CPU ，那么
		 * 是会有 2048 还是会轮流执行吗? 还是最多有 8 个可以同时执行，
		 * 但是需要其中的有任务彻底结束之后，才可以有新的任务来。
		 */
		break;

	/*
	 * 测试 system_power_efficient_wq 的睡眠精度
	 * action 20 : 启动测试，目标时间点为每 0.5s (NSEC_PER_SEC/2)
	 * action 21 : 停止测试
	 */
	case 20:
		test_sched_sync_hw_clock();
		break;
	case 21:
		stop_sched_sync_test();
		break;
	}
	return 0;
}
