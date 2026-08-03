#include "internal.h"
#include <linux/delay.h>
#include <linux/slab.h>

static struct nic __rcu *g_pfoo = NULL;
static bool dump = true;

struct nic {
	int a;
	int b;
	int c;
	/*
	 * 为什么 call rcu 需要和 rcu_head 配合使用
	 * 应该用来串联所有需要执行的任务
	 */
	struct rcu_head rcu;
};

static bool rcu_protect;
static bool use_synchronize_rcu;
struct rcu_test {
	struct work work;
	struct mutex write_lock;
};

int test_rcupdate_init(void)
{
	g_pfoo = (struct nic *)kzalloc(sizeof(struct nic), GFP_KERNEL);
	if (!g_pfoo)
		return -ENOMEM;
	return 0;
}

int test_rcupdate_exit(void)
{
	kfree(g_pfoo);
	return 0;
}

#define LOOP_COUNT 100
static void rcu_basic_reader(void)
{
	int cnt = LOOP_COUNT;

	struct nic *p = NULL;
	pr_info("[%s] start\n", __FUNCTION__);

	while (cnt--) {
		/* 即便是没有 guard(rcu) ，也不会立刻出现问题，guard 这是保证内存
		 * 在该范围内一定没被释放。
		 */
		if (rcu_protect)
			rcu_read_lock();
		/* rcu_dereference 如果不考虑 compiler / cpu optimization ，其实就是一个
		 * 简单的赋值。TODO 有办法测试出来不用 rcu_dereference 的效果吧
		 */
		p = rcu_dereference(g_pfoo);
		unsigned long jiffies_at_begin = jiffies;
		/*
		 * 如果 loop 的时间太久，因为 rcu critical period 的时间一直没有
		 * 结束，最后 oom 。只有所有的 rcu period 都结束，才可以进入到
		 * grace period ，开始回收。 即便是大多数 object 是没有
		 * reference count 的。
		 *
		 * 1. 为什么采用 refcount 的方式来自动回收? 太重了，reader 使用
		 *	之前需要 refcount inc ，这个就用了 atomic 了
		 * 2. call_rcu 之后，如何知道所命中的 rcu critical period 都结束了？
		 *	- 如何知道命中了那些 rcu ?
		 *	- 如何知道结束了?
		 * TODO
		 */
		while (time_after(jiffies_at_begin + HZ / 50, jiffies)) {
			cpu_relax();
		}
		if (p->a == -1 || p->b == -1 || p->c == -1) {
			pr_err("reference a wrong value !\n");
			break;
		}
		if (rcu_protect)
			rcu_read_unlock();
		cond_resched();
	}
	pr_info("[%s] finished \n", __FUNCTION__);
}

static void rcu_reclaimer(struct rcu_head *rh)
{
	struct nic *p = container_of(rh, struct nic, rcu);
	p->a = -1;
	p->b = -1;
	p->c = -1;
	kfree(p);
	if (dump) {
		dump = false;
		/*
		 * 如果是使用 synchronize_rcu ，那么当前环境中执行的:
		 * [69728.103212]  rcu_reclaimer+0x44/0x50 [martins3]
		 * [69728.103217]  rcu_basic_writer+0xa8/0x100 [martins3]
		 * [69728.103223]  two_reader_logic+0x50/0x68 [martins3]
		 * [69728.103227]  process_one_work+0x214/0x618
		 * [69728.103236]  worker_thread+0x1c4/0x370
		 * [69728.103239]  kthread+0x13c/0x210
		 * [69728.103246]  ret_from_fork+0x10/0x20
		 *
		 * 如果是使用 call_rcu ，其实可以将工作延迟
		 *
		 * [69616.616171]  rcu_reclaimer+0x44/0x50 [martins3]
		 * [69616.616177]  rcu_core+0x300/0xd20
		 * [69616.616184]  rcu_core_si+0x18/0x30
		 * [69616.616187]  handle_softirqs+0x130/0x488
		 * [69616.616192]  __do_softirq+0x1c/0x28
		 * [69616.616195]  ____do_softirq+0x18/0x30
		 * [69616.616199]  call_on_irq_stack+0x24/0x58
		 * [69616.616202]  do_softirq_own_stack+0x24/0x50
		 * [69616.616206]  __irq_exit_rcu+0x148/0x188
		 * [69616.616208]  irq_exit_rcu+0x18/0x48
		 */
		dump_stack();
	}
}

static void rcu_basic_writer(void)
{
	int value = 1;

	unsigned long jiffies_at_begin = jiffies;
	pr_info("[%s] start\n", __FUNCTION__);
	while (time_after(jiffies_at_begin + HZ * 3, jiffies)) {
		struct nic *old;
		struct nic *new =
			(struct nic *)kzalloc(sizeof(struct nic), GFP_KERNEL);
		/* TODO 难道这里访问不应该使用 rcu_dereference 吗?
		 */
		old = g_pfoo;
		new->a = value;
		new->b = value;
		new->c = value;
		/* TODO 如果这里使用 g_pfoo = new 可以被检测到吗?
		 */
		rcu_assign_pointer(g_pfoo, new);
		if (use_synchronize_rcu) {
			synchronize_rcu();
			rcu_reclaimer(&old->rcu);
		} else {
			call_rcu(&old->rcu, rcu_reclaimer);
		}

		value++;
	}
	pr_info("[%s] finished with value=%d \n", __FUNCTION__, value);
}

static void rcu_deadlock_writer(void)
{
	pr_info("[%s] start\n", __FUNCTION__);
	rcu_read_lock();
	synchronize_rcu();
	rcu_read_unlock();
}

static void rcu_lock_writer(struct rcu_test *test)
{
	struct nic *nic;
	mutex_lock(&test->write_lock);
	/*
	 * TODO
	 *
	 * 本来想要测试下如下四个函数的区别:
	 * 1. rcu_dereference_protected  : writer lock protected
	 * 2. rcu_dereference : read cirtical section protected 就可以了
	 * 3. rcu_access_pointer  : 测试 pointer 是否为 NULL
	 * 4. rcu_dereference_raw : lockdep
	 * 但是这里都是 memory barrier 和 READ_ONCE 的对比，还是先搞一个 memory model 的
	 * 测试框架吧。
	 */
	nic = rcu_dereference_protected(g_pfoo,
					lockdep_is_held(&test->write_lock));
	mutex_unlock(&test->write_lock);
}

static void two_reader_logic(struct work_struct *work)
{
	struct rcu_test *test = (struct rcu_test *)work;
	pr_info("run %d\n", test->work.id);
	switch (test->work.id) {
	case 0:
		rcu_basic_reader();
		break;
	case 1:
		rcu_basic_reader();
		break;
	case 2:
		rcu_basic_writer();
		break;
	}
}

/*
 * 显然，是需要 mutex 才可以保证 writer 正常，例如
 * rcu_basic_writer 中，如果无法保证 old = g_pfoo; 的执行，
 * 那么后续的发释放可能释放的是同一个结构体。
 */
static void two_writer_logic(struct work_struct *work)
{
	struct rcu_test *test = (struct rcu_test *)work;
	pr_info("run %d\n", test->work.id);
	switch (test->work.id) {
	case 0:
		rcu_basic_reader();
		break;
	case 1:
		rcu_basic_writer();
		break;
	case 2:
		rcu_lock_writer(test);
		break;
	}
}

/*
 * TODO
 fd_install 中使用的 rcu_read_lock_sched 如何测试理解 ?
- synchronize_rcu_expedited() ： 强制结束 rcu grace 时间，感觉一般都是在 subsystem 关闭的时候
- kfree_rcu
 */

static void rcu_sched_logic(struct work_struct *work)
{
	struct rcu_test *test = (struct rcu_test *)work;
	switch (test->work.id) {
	case 0: {
		int a;
		rcu_read_lock_sched();
		rcu_read_unlock_sched();
		pr_info("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__, a);

	} break;
	case 1: {
	} break;
	}
}

int test_rcupdate(long action)
{
	dump = true;
	switch (action) {
	case 1:
		/* RCU 保护，不会访问到已经被释放的 object
		 * 但是，如果在 rcu_basic_reader 中的 cirtical region 的时间跨度
		 * 太大，rcu_basic_writer 分配的内存可以导致 oom
		 */

		rcu_protect = true;
		use_synchronize_rcu = false;
		return batch_queue_works(two_reader_logic, 3,
					 sizeof(struct rcu_test));
	case 2:
		/* 受保护，导致 synchronize_rcu 受阻于 rcu_basic_reader 的
		 * rcu_read_unlock 执行结束
		 */
		rcu_protect = true;
		use_synchronize_rcu = true;
		return batch_queue_works(two_reader_logic, 3,
					 sizeof(struct rcu_test));
	case 3:
		/* 不受 RCU 保护，可以访问到已经被释放的 object
		 */
		rcu_protect = false;
		use_synchronize_rcu = false;
		return batch_queue_works(two_reader_logic, 3,
					 sizeof(struct rcu_test));
		break;
	case 4:
		/*
		 * 测试在 rcu_read_lock 中使用 synchronize_rcu
		 *
		 * 立刻触发
		 *
		 * Voluntary context switch within RCU read-side critical section!
		 * [ 2165.798397] WARNING: CPU: 4 PID: 10579 at kernel/rcu/tree_plugin.h:332 rcu_note_context_switch+0x4f8/0x560
		 *
		 * 接下来，系统开始不正常，并且触发这个错误
		 *
		 * [ 2207.271331] rcu: INFO: rcu_preempt detected expedited stalls on CPUs/tasks: { P10579 } 21209 jiffies s: 861 root: 0x1/.
		 * [ 2207.273192] rcu: blocking rcu_node structures (internal RCU debug): l=1:0-15:0x0/T
		 * [ 2249.821015] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
		 * [ 2249.822021] rcu:     Tasks blocked on level-1 rcu_node (CPUs 0-15): P10579/1:b..l
		 * [ 2249.823057] rcu:     (detected by 1, t=84025 jiffies, g=88925, q=52514 ncpus=8)
		 *
		 */
		rcu_deadlock_writer();
		break;
	case 5:
		rcu_protect = true;
		use_synchronize_rcu = false;
		return batch_queue_works(two_writer_logic, 3,
					 sizeof(struct rcu_test));
	case 6:
		return batch_queue_works(rcu_sched_logic, 2,
					 sizeof(struct rcu_test));
	}
	return 0;
}
