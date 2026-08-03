#define pr_fmt(fmt) "sched_debug: " fmt
#include "internal.h"
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>

static DEFINE_SPINLOCK(sl_static);

// 那么
// 1. CONFIG_DEBUG_ATOMIC_SLEEP 到底在检测什么东西?
//
//		spin_lock_irqsave(&sl_static, flags);
//		msleep(1000); // 会被检测出来，无论是否打开 CONFIG_DEBUG_ATOMIC_SLEEP
//		spin_unlock_irqrestore(&sl_static, flags);

static void trigger_softlockup(void)
{
	/*
	 * 测试 softlock 的效果。虽然存在时钟中断，但是此内核是 voluntery preempt 的，
	 * 所以即使没有屏蔽时钟中断，该进程还是不能被调度走。
	 *
	 * 当 echo full | sudo tee /sys/kernel/debug/sched/preempt 之后，
	 * 无论等待多长时间都不会出现 softlock up
	 * 这个时候持续观察  /proc/$pid/sched 可以发现 nr_switches 持续增加
	 *
	 * 之后 echo none | sudo tee /sys/kernel/debug/sched/preempt
	 *
	 * 稍微等一段事件，然后可以发现:
	 * [65334.889685] Dynamic Preempt: none
         * [65356.013149] watchdog: BUG: soft lockup - CPU#1 stuck for 22s! [sched_debug-use:195255]
	 */
	pr_info("触发 softlockup 的效果取决于 preempt 是否为 full \n");
	for (;;) {
		cpu_relax();
		if (signal_pending(current))
			break;
	}
}

static void run_rcu_stall_busy_loop(void)
{
	/*
	 * 即便是设置 echo full > /sys/kernel/debug/sched/preempt
	 * 在 rcu critical 中循环，很快就出发了 rcu stall direction
	 *
	 * rcu stall ditection 的报错如下:
	 *
	 * [  164.986217] rcu: INFO: rcu_preempt self-detected stall on CPU
	 * [  164.986770] rcu:     7-....: (20959 ticks this GP) idle=200c/1/0x4000000000000000 softirq=3134/3134 fqs=4792
	 * [  164.987566] rcu:     (t=21002 jiffies g=2573 q=100 ncpus=8)
	 *
	 * 这显然是非常合理的，因为无法离开 rcu ，导致系统被阻塞。
	 */
	pr_info("触发 rcu stall：在 rcu read-side critical section 中持续占用 CPU\n");
	rcu_read_lock();
	for (;;)
		cpu_relax();
	rcu_read_unlock();
}
static void run_spinlock_preempt(void)
{
	unsigned long nvcsw_before;
	unsigned long nivcsw_before;
	unsigned long nvcsw_after;
	unsigned long nivcsw_after;

	// # spinlock 中为什么需要关中断
	// <!-- 49d9af22-7e3a-44f8-99a2-304bdb0b11ca -->
	//
	// 1. spinlock, irq-handler,  preempt disable 这些地方都不可以睡眠
	// 2. spinlock 看似需要关中断:
	//    1. 如果不屏蔽，中断处理函数中重新持有这个锁会导致死锁。目前看，中断处理函数中可以用的语句只有 spinlock 了
	//    但是也不是可以站住脚的，如果知道这个 lock 会被 irq handler 使用，那么就特殊标记，而不是在锁机制上配置。
	//    2. spinlock 关闭抢占，然后被调度组，会导致其他的 CPU 被阻塞
	// 3. 不管怎么讲，spinlock 在 CONFIG_PREEMPT_RT 下是可以睡眠的，那么为什么需要关闭抢占
	// 4. 如果屏蔽中断，关闭抢占的确没有意义
	//
	//
	// 1. spinlock 为什么自动关抢占
	//  https://mp.weixin.qq.com/s/fokaVEkST6ycG3hRrCbM1g
	//
	//  具体的实现可以看 : include/linux/spinlock_api_smp.h
	// 我比较认可 codex 的说法，首先，这是 spinlock 的语义的一部分，可以防止 spinlock 调度。
	// 其次，在 UP 配置下，spinlock 甚至可以退化成“几乎只有 preempt_disable()”。你可以在 include/linux/spinlock_api_up.h
	//   - __LOCK(lock) 本质就是 preempt_disable()
	// - 因为单 CPU 上进程上下文之间的并发，本来就主要靠“别被抢占”来解决
	//
	// 2. spin_lock_irq 已关中断，为何还要关抢占？
	//  https://mp.weixin.qq.com/s/Dc3OJYlDV1I-USGPqo76KQ
	// 作者认为是必要多此一举，但是继续看看 9749b2b6-17ab-40b2-8657-e3d19c77c55b 关联的文档吧

	// 在这里会触发警告，再次说明，按道理，如果中断屏蔽了，是不应该调度的
	local_irq_disable();
	// might_sleep();
	local_irq_enable();

	// 但是我们做实验可以发现，其实中断屏蔽后，可以调度的，但是这不合规矩
	nvcsw_before = READ_ONCE(current->nvcsw);
	nivcsw_before = READ_ONCE(current->nivcsw);
	local_irq_disable();
	pr_info("before schedule: in_atomic=%d irqs_disabled=%d preemptible=%d nvcsw=%lu nivcsw=%lu cpu=%d\n",
		in_atomic(), irqs_disabled(), preemptible(), nvcsw_before,
		nivcsw_before, raw_smp_processor_id());
	schedule();
	nvcsw_after = READ_ONCE(current->nvcsw);
	nivcsw_after = READ_ONCE(current->nivcsw);
	pr_info("after schedule: in_atomic=%d irqs_disabled=%d preemptible=%d nvcsw=%lu nivcsw=%lu delta=%lu cpu=%d\n",
		in_atomic(), irqs_disabled(), preemptible(), nvcsw_after,
		nivcsw_after,
		(nvcsw_after - nvcsw_before) + (nivcsw_after - nivcsw_before),
		raw_smp_processor_id());
	local_irq_enable();

	/*
	 * 不要搞这个操作，会直接 panic 的
	 *
	 * preempt_disable();
	 * schedule();
	 * preempt_enable();
	 */



}

static void run_atomic_might_sleep_check(void)
{
	unsigned long flags;
	pr_info("spin_lock_irqsave 后调用 might_sleep，观察 atomic sleep warning\n");
	spin_lock_irqsave(&sl_static, flags);
	might_sleep();
	spin_unlock_irqrestore(&sl_static, flags);

	/*
	 * 如果打开 CONFIG_DEBUG_ATOMIC_SLEEP 可以得到如下的结果:
	 *
	 * [  977.664875] BUG: sleeping function called from invalid context at /home/martins3/core/vn/code/module/preempt.c:27
	 * [  977.665595] in_atomic(): 1, irqs_disabled(): 1, non_block: 0, pid: 1647, name: bash
	 * [  977.665857] preempt_count: 1, expected: 0
	 * [  977.665997] RCU nest depth: 0, expected: 0
	 */
}

static void run_atomic_msleep_check(void)
{
	unsigned long flags;

	pr_info("spin_lock_irqsave 后调用 msleep，观察禁止睡眠上下文中的报错\n");
	spin_lock_irqsave(&sl_static, flags);
	msleep(1000); // 会被检测出来，无论是否打开 CONFIG_DEBUG_ATOMIC_SLEEP
	spin_unlock_irqrestore(&sl_static, flags);
}

static void run_msleep_with_preempt_disabled(void)
{
	pr_info("preempt_disable 后调用 msleep，观察禁止睡眠上下文中的报错\n");
	preempt_disable();
	msleep(1000); // 也是会被检测出来，无论是否打开 CONFIG_DEBUG_ATOMIC_SLEEP
	preempt_enable();
}

static void run_kmalloc_under_spinlock(void)
{
	unsigned long flags;
	int *f;

	pr_info("在 spinlock 中做 kmalloc/kfree，观察它不会像 schedule 那样被直接检测出来\n");
	// 无论是 preempt_disable 还是 spinlock ，kmalloc 和 kfree 不会被检测出来
	// 即便是打开 CONFIG_PROVE_LOCKING 所以说还是看是不是调用到检测点，
	spin_lock_irqsave(&sl_static, flags);
	f = kmalloc(sizeof(int), GFP_KERNEL);
	kfree(f);
	spin_unlock_irqrestore(&sl_static, flags);
}

/*
 * 无论是 preempt disable 还是 rcu critical region 中，绝对不可以 sleep 的
 */
static void run_schedule_with_preempt_disabled(void)
{
	preempt_disable();
	schedule();
	preempt_enable();
}

static void run_schedule_in_rcu_read_lock(void)
{
	/*
	 * 在 rcu_note_context_switch 触发警告
	 */
	rcu_read_lock();
	cond_resched();
	rcu_read_unlock();
}

/*
 * cond_resched() 对比函数是 schedule() ，当调用 schedule()
 * cond_resched() 的关键是 TIF_NEED_RESCHED 配置上
 *
 * 具体细节在 : docs/kernel/sched/sched-preempt.md 中
 */
static void run_cond_resched(void)
{
	/*
	 * 这三个都是会触发 CONFIG_DEBUG_ATOMIC_SLEEP 的
	 *
	 * might_sleep 也是如此
	 */
	preempt_disable();
	cond_resched();
	preempt_enable();

	rcu_read_lock();
	cond_resched();
	rcu_read_unlock();

	local_irq_disable();
	cond_resched();
	local_irq_enable();
}

/*
 * schedule() 与 cond_resched() 的核心区别:
 * - schedule() 无条件触发上下文切换，即使当前进程仍然拥有时间片
 *   - 但是，依旧注意，schedule() 未必表示一定会调度走，具体看 __schedule() 中存在如下逻辑:
 *   	is_switch = prev != next;
 *	if (likely(is_switch)) {
 *
 * - cond_resched() 仅在 TIF_NEED_RESCHED 标志置位时才会调用 schedule() ，所以其功能相当清晰，
 *   当在一个长循环中运行的时候，而且当内核没有打开抢占，所以需要手动执行 cond_resched 来释放
 *   但是这个释放是当前的 process 的确用完了时间片
 */
static void run_schedule_must_switch(void)
{
	for (size_t i = 0; i < 1000; i++) {
		schedule();
	}
}

static void run_cond_schedule_may_switch(void)
{
	for (size_t i = 0; i < 1000; i++)
		cond_resched();
}

static void run_interruptible_cond_resched_loop(void)
{
	pr_info("带 cond_resched 的循环，通过 signal_pending 退出，验证让出 CPU 与信号检查的关系\n");
	for (;;) {
		// 1. 无论是否屏蔽中断，signal_pending 都是可以接受到的，原因 bash 父进程传递的一个标志位
		// 2. 和用户态不同的，一个用户态程序调用到此函数，ctrl-c 没必要办法退出
		// 除非有 signal_pending 的检查，然后显示的退出
		if (signal_pending(current))
			break;

		cpu_relax();
	}
	pr_info("收到信号，结束 cond_resched 循环\n");
}

/*
 * #define preemptible()   (preempt_count() == 0 && !irqs_disabled())
 */
static void basic_api(void)
{
	pr_info("打印 in_atomic/lockdep/rcu depth，作为基础状态样本\n");
	// 总是输出 0
	pr_info("in_atomic : %d\n", in_atomic());
	pr_info("preemptible : %d\n", preemptible());

	preempt_disable();
	// 总是输出 1
	pr_info("in_atomic=%d preemptible=%d with preempt_disable\n",
		in_atomic(), preemptible());
	preempt_enable();

	/* lockdep_depth 描述了当前持有的锁的数量
	 * 如果打开 CONFIG_PROVE_LOCKING ，那么输出为 4
	 * 否则为 0
	 */
	spin_lock(&sl_static);
	pr_info("lockdep_depth : %d\n", lockdep_depth(current));
	spin_unlock(&sl_static);

	/*  rcu_read_lock 当然可以嵌套，
	 *  如果打开 CONFIG_PREEMPT_RCU ，输出为 2 ，
	 *  如果不打开，输出为 0
	 */
	rcu_read_lock();
	rcu_read_lock();
	pr_info("rcu_preempt_depth : %d\n", rcu_preempt_depth());
	rcu_read_unlock();
	rcu_read_unlock();
}

int test_sched_debug(long action)
{
	switch (action) {
	case 0:
		basic_api();
		break;
	case 1:
		run_atomic_might_sleep_check();
		break;
	case 13:
		run_spinlock_preempt();
		break;
	case 2:
		run_atomic_msleep_check();
		break;
	case 3:
		run_msleep_with_preempt_disabled();
		break;
	case 4:
		run_kmalloc_under_spinlock();
		break;
	case 5:
		run_schedule_with_preempt_disabled();
		break;
	case 6:
		run_schedule_in_rcu_read_lock();
		break;
	case 7:
		run_cond_resched();
		break;
	case 8:
		run_schedule_must_switch();
		break;
	case 9:
		run_cond_schedule_may_switch();
		break;
	case 10:
		run_interruptible_cond_resched_loop();
		break;
	case 11:
		trigger_softlockup();
		break;
	case 12:
		run_rcu_stall_busy_loop();
		break;
	default:
		pr_info("unknown action %ld\n", action);
		return -EINVAL;
	}

	return 0;
}
