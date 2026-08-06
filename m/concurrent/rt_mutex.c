#include "internal.h"
#include <linux/rtmutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/completion.h>

/*
 * rt_mutex 和普通 mutex 的区别只有一个: 优先级继承 (priority inheritance)。
 *
 * 优先级反转问题:
 *   低优先级线程 L 持锁，高优先级线程 H 等锁，此时一个中优先级线程 M
 *   抢占了 L 的 CPU 。虽然 H 优先级最高，但是 H 在等 L 释放锁，
 *   而 L 抢不过 M ，最终 H 的实际等待时间取决于 M ，优先级形同虚设。
 *
 * rt_mutex 的解法: H 开始等锁的时候，会临时把自己的优先级"借"给 L ，
 * L 被提升之后自然抢得过 M ，尽快放锁，H 的等待时间重新只取决于
 * L 的临界区长度。
 *
 * 下面的 demo 把三个线程绑到同一个 CPU 上:
 *   low  : SCHED_FIFO 10 ，持锁忙等 3s
 *   mid  : SCHED_FIFO 20 ，不碰锁，纯烧 CPU 5s
 *   high : SCHED_FIFO 30 ，0.5s 后来抢锁，测量等了多久
 *
 * 预期结果:
 *   rt_mutex : low 被提升到 30 ，mid 抢不走 CPU ，high 约等 2.5s(取决于 low 的临界区)
 *   mutex    : low 被 mid 抢走 CPU ，high 要等 mid 烧完 5s 再多等 low 收尾，约 7.5s
 *
 * 注意: sched_setscheduler_nocheck 没有 EXPORT ，模块里无法自己设置
 * SCHED_FIFO ，所以测试分两步:
 *   echo 2 > rt_mutex   # 创建三个线程，线程打印自己的 pid 之后等待发令枪
 *   chrt -f -p 10 <low_pid> ; chrt -f -p 20 <mid_pid> ; chrt -f -p 30 <high_pid>
 *   echo 4 > rt_mutex   # 发令枪响，开始测试，等待结束后返回
 * 如果 120s 内没有发令枪，线程会以普通优先级继续跑，保证不会卡死。
 */

static DEFINE_RT_MUTEX(rt_mutex_demo);
static DEFINE_MUTEX(mutex_demo);

static struct completion pi_start;
static struct completion pi_done[3];
static bool pi_use_rt_mutex;
static bool pi_running;

/* 纯烧 CPU ，不切出，模拟临界区内的计算 */
static void burn_ms(int ms)
{
	ktime_t end = ktime_add_ms(ktime_get(), ms);

	while (ktime_before(ktime_get(), end))
		cpu_relax();
}

static void pi_wait_start(const char *name)
{
	pr_info("%s : pid=%d , set fifo priority from userspace now\n", name,
		current->pid);
	if (!wait_for_completion_timeout(&pi_start, 120 * HZ))
		pr_err("%s : start timeout, run with normal priority\n", name);
}

static int pi_low(void *unused)
{
	pi_wait_start("low ");
	pr_info("low  : start, hold lock 3000 ms\n");
	if (pi_use_rt_mutex)
		rt_mutex_lock(&rt_mutex_demo);
	else
		mutex_lock(&mutex_demo);

	burn_ms(3000);

	if (pi_use_rt_mutex)
		rt_mutex_unlock(&rt_mutex_demo);
	else
		mutex_unlock(&mutex_demo);
	pr_info("low  : lock released\n");
	complete(&pi_done[0]);
	return 0;
}

static int pi_mid(void *unused)
{
	pi_wait_start("mid ");
	/* 让 low 先拿到锁 */
	msleep(200);
	pr_info("mid  : start, burn cpu 5000 ms (never touch the lock)\n");
	burn_ms(5000);
	pr_info("mid  : finished\n");
	complete(&pi_done[1]);
	return 0;
}

static int pi_high(void *unused)
{
	ktime_t start;

	pi_wait_start("high");
	/* 等 low 持锁、mid 开始烧 CPU 之后再来抢锁 */
	msleep(500);

	pr_info("high : start waiting for lock\n");
	start = ktime_get();
	if (pi_use_rt_mutex)
		rt_mutex_lock(&rt_mutex_demo);
	else
		mutex_lock(&mutex_demo);
	pr_info("high : waited %lld ms\n",
		ktime_to_ms(ktime_sub(ktime_get(), start)));

	if (pi_use_rt_mutex)
		rt_mutex_unlock(&rt_mutex_demo);
	else
		mutex_unlock(&mutex_demo);
	complete(&pi_done[2]);
	return 0;
}

/*
 * 三个线程必须绑到同一个 CPU 上，优先级反转才有意义:
 * 如果 mid 和 low 可以各自跑在不同的 CPU 上，low 根本不会被饿死。
 *
 * 只创建线程并发令枪前的等待，真正的计时在 test_rt_mutex 的 case 4 里。
 */
static void priority_inversion_test(bool use_rt_mutex)
{
	struct task_struct *tasks[3];
	int cpu = num_online_cpus() > 1 ? 1 : 0;
	int i, j;

	if (pi_running) {
		pr_err("previous test still running, echo 4 first\n");
		return;
	}

	pi_use_rt_mutex = use_rt_mutex;
	init_completion(&pi_start);
	for (i = 0; i < 3; i++)
		init_completion(&pi_done[i]);

	pr_info("=== %s ===\n",
		use_rt_mutex ? "rt_mutex (priority inheritance)"
			     : "mutex (no priority inheritance)");

	tasks[0] = kthread_create(pi_low, NULL, "pi_low");
	tasks[1] = kthread_create(pi_mid, NULL, "pi_mid");
	tasks[2] = kthread_create(pi_high, NULL, "pi_high");

	for (i = 0; i < 3; i++) {
		if (IS_ERR(tasks[i])) {
			pr_err("kthread_create failed\n");
			/* 已经创建的线程放出去跑完，不要留下孤儿线程 */
			complete_all(&pi_start);
			for (j = 0; j < i; j++)
				wake_up_process(tasks[j]);
			for (j = 0; j < i; j++)
				wait_for_completion(&pi_done[j]);
			return;
		}
		kthread_bind(tasks[i], cpu);
	}

	pi_running = true;
	for (i = 0; i < 3; i++)
		wake_up_process(tasks[i]);
}

/*
 * 基础竞争 demo: worker 0 持锁 2s ，worker 1 测量自己排队等了多久。
 * 只展示 rt_mutex 的互斥语义，和优先级继承无关。
 */
static void rt_mutex_worker(struct work_struct *work)
{
	struct work *test = (struct work *)work;
	ktime_t start;

	if (test->id == 0) {
		rt_mutex_lock(&rt_mutex_demo);
		msleep(2000);
		rt_mutex_unlock(&rt_mutex_demo);
		pr_info("holder released\n");
	} else {
		/* 保证 worker 1 在 worker 0 持锁之后才来竞争 */
		msleep(100);
		start = ktime_get();
		rt_mutex_lock(&rt_mutex_demo);
		pr_info("waiter waited %lld ns\n", ktime_get() - start);
		rt_mutex_unlock(&rt_mutex_demo);
	}
}

int test_rt_mutex(long action)
{
	int i;

	switch (action) {
	case 0:
		/* 基础 API: 持锁时 trylock 失败，空锁时 trylock 成功 */
		rt_mutex_lock(&rt_mutex_demo);
		pr_info("trylock while holding : %d (expect 0)\n",
			rt_mutex_trylock(&rt_mutex_demo));
		rt_mutex_unlock(&rt_mutex_demo);
		pr_info("trylock while free    : %d (expect 1)\n",
			rt_mutex_trylock(&rt_mutex_demo));
		rt_mutex_unlock(&rt_mutex_demo);
		break;
	case 1:
		batch_queue_works(rt_mutex_worker, 2, sizeof(struct work));
		break;
	case 2:
		priority_inversion_test(true);
		break;
	case 3:
		priority_inversion_test(false);
		break;
	case 4:
		/* 发令枪，然后等所有线程结束，保证 rmmod 时没有遗留线程 */
		if (!pi_running) {
			pr_err("echo 2 or 3 first\n");
			break;
		}
		complete_all(&pi_start);
		for (i = 0; i < 3; i++)
			wait_for_completion(&pi_done[i]);
		pi_running = false;
		pr_info("priority inversion test finished\n");
		break;
	}
	return 0;
}
