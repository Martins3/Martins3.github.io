#include "internal.h"
#include <linux/delay.h>
#include <linux/preempt.h>
#include <linux/version.h>
#include <linux/timer.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>

/*
 * timer 的 callback 可能 softirq 上下文中执行的
 * 可能在 ksoftirqd 中执行。
 *
 * 所以，如果在其中调用 msleep(1) ，就会触发我们熟悉的
 * 
 * BUG: scheduling while atomic: tee/1555/0x00000102
 */
static void worker_in_softirq(struct timer_list *timer)
{
	pr_info("%ld %ld %ld\n", in_irq(), in_softirq(), in_interrupt());
}

/* Data structure for RCU testing */
struct rcu_test_data {
	int value;
	struct rcu_head rcu;
};

static struct rcu_test_data *rcu_data;

static void rcu_callback(struct rcu_head *head)
{
	struct rcu_test_data *data =
		container_of(head, struct rcu_test_data, rcu);
	pr_info("RCU callback executed in softirq context: in_irq=%ld, in_softirq=%ld, in_interrupt=%ld\n",
		in_irq(), in_softirq(), in_interrupt());
	kfree(data);
}

static void rcu_in_softirq(struct timer_list *timer)
{
	pr_info("%ld %ld %ld\n", in_irq(), in_softirq(), in_interrupt());
	pr_info("call call_rcu()\n");
	/*
	 * 不过我没有想到的是，是可以调用 call_rcu 的，不过也合理，只是
	 * 将工作挂到 list 上而已。
	 */
	call_rcu(&rcu_data->rcu, rcu_callback);

	pr_info("call synchronize_rcu()\n");
	/*
	 * 显然是不可以调用 synchronize_rcu 的，softirq 中不可以睡眠
	 */ 
	// synchronize_rcu();
}

static inline int clean_up_timer(struct timer_list *tl)
{
#if (LINUX_VERSION_CODE > KERNEL_VERSION(6, 15, 0))
	return timer_delete_sync(tl);
#else
	return del_timer_sync(&tl);
#endif
}

int test_softirq(long action)
{
	struct timer_list tl;
	switch (action) {
	case 0:
		/*
		 * 最简单的测试，只是说明 timer 的确是执行在 softirq 下的
		 */
		timer_setup(&tl, worker_in_softirq, 0);
		tl.expires = jiffies + usecs_to_jiffies(1000);
		add_timer(&tl);
		/*
		 * 这个等待是必须的，不然 clean_up_timer 返回错误，而且 timer 会不触发
		 * 具体原因以后再看吧
		 */
		msleep(100);
		return clean_up_timer(&tl);
	case 1:
		/*
		 * 测试 softirq 的 callback 中可不可以执行 call_rcu
		 * 在尝试理解 docs/concurrent/rcu/bh.md 中的内容
		 */
		rcu_data = kmalloc(sizeof(struct rcu_test_data), GFP_KERNEL);
		if (!rcu_data)
			return -ENOMEM;
		timer_setup(&tl, rcu_in_softirq, 0);
		add_timer(&tl);
		msleep(100);
		return clean_up_timer(&tl);
	}
	return 0;
}
