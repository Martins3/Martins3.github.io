#include "internal.h"
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/wait.h>

static DECLARE_WAIT_QUEUE_HEAD(martins3_waitq);
static atomic_t ok = ATOMIC_INIT(0);

/**
 * DEFINE_WAIT_FUNC 的使用模式全部几乎都是类似这种模式，但是存在一些例外
 *
 * 这种模式指的是:
 * 1. woken_wake_function 和 wait_woken 配合使用
 * 2. wait_woken 在一个循环中，每次执行到都需要等待一次，直到被 wake_up 唤醒
 */
static void loop_wait(void)
{
	DEFINE_WAIT_FUNC(wait, woken_wake_function);
	add_wait_queue(&martins3_waitq, &wait);
	for (size_t i = 0; i < 3; i++) {
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		might_sleep();
		wait_woken(&wait, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);
	}
	remove_wait_queue(&martins3_waitq, &wait);
}

/*
 * 测试 init_waitqueue_func_entry 函数
 * init_waitqueue_func_entry 初始化一个 wait_queue_entry_t 结构体，指定一个唤醒函数
 * 这与 DEFINE_WAIT_FUNC 类似，将 wait_queue_entry_t 结构体的中内容初始化一下而已。
 */
static void init_waitqueue_func_entry_test(void)
{
	wait_queue_entry_t func_wait;

	// 使用 init_waitqueue_func_entry 初始化等待队列项
	init_waitqueue_func_entry(&func_wait, woken_wake_function);
	add_wait_queue(&martins3_waitq, &func_wait);

	// 使用 wait_woken 等待被唤醒，类似 loop_wait
	might_sleep();
	wait_woken(&func_wait, TASK_UNINTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT);

	remove_wait_queue(&martins3_waitq, &func_wait);
	pr_info("[martins3:%s:%d] init_waitqueue_func_entry test completed\n",
		__FUNCTION__, __LINE__);
}

/*
 * 仿照 usblp_rwait_and_lock 的实现
 *
 * 似乎和 loop_wait 的效果完全相同，难道是两种不同的写法
 *
 * 这种写法更加容易理解点，设置 TASK_INTERRUPTIBLE 之后，当前进程只会被两种原因唤醒:
 * 1. signal
 * 2. 在 wait queue 中被唤醒
 *
 * 参考 https://lwn.net/Articles/22913/ : 这种方法没有将 wait 从 waitq 中移开，反而导致性能有问题。
 * TODO 表示怀疑
 *
 */
static void loop_wait2(void)
{
	long state;
	DECLARE_WAITQUEUE(waita, current);
	add_wait_queue(&martins3_waitq, &waita);
	for (size_t i = 0; i < 3; i++) {
		set_current_state(TASK_INTERRUPTIBLE);
		pr_info("[%s:%d] %d\n", __FUNCTION__, __LINE__, current->pid);
		schedule();
	}
	state = get_current_state();
	// TODO 从这里退出的时候，本来就是 state 为 0 ，为什么还要设置为
	pr_info("[martins3:%s:%d] %lx\n", __FUNCTION__, __LINE__, state);
	set_current_state(TASK_RUNNING);
	remove_wait_queue(&martins3_waitq, &waita);
}

/* TODO 这个注释是什么意思，为什么需要 wqh -> lock 啊?
 * Called with wqh->lock held and interrupts disabled
 * static int irqfd_wakeup(wait_queue_entry_t *wait, unsigned mode, int sync, void *key)
 */

/*
 * FIXME https://lwn.net/Articles/628628/ 中描述的不推荐的写法
 *
 * 但是 参考 ioc_rqos_throttle / bd_prepare_to_claim
 * 还是用的 finish_wait ，到底不推荐什么东西了
 *
 * 应该把 finish_wait 都去掉?
 */
static void loop_wait3(void)
{
	DEFINE_WAIT(w);
	for (size_t i = 0; i < 100; i++) {
		prepare_to_wait(&martins3_waitq, &w, TASK_IDLE);
		might_sleep(); // CONFIG_DEBUG_ATOMIC_SLEEP 将会产生警告
		pr_info("[%s:%d] %d\n", __FUNCTION__, __LINE__, current->pid);
		schedule();
	}
	finish_wait(&martins3_waitq, &w);
}


int test_wait(long action)
{
	switch (action) {
	case 0:
		// wake_up 和 wake_up_all 的区别 :
		// 1. 到底是唤醒一个 thread ，还是所在的 thread group 中的所有的 thread
		// 2. 如果是不同的 thread group 等待到同一个 waitq 上，wake_up(&waitq) 会将他们都唤醒
		wake_up(&martins3_waitq);
		break;
	case 1:
		atomic_set(&ok, 1);
		break;
	case 2:
		atomic_set(&ok, 0);
		break;
	case 3:
		/**
		 * wait_event 只是 waitqueue 的一个简单封装
		 * 在 ___wait_event 中，
		*/
		wait_event_interruptible(martins3_waitq, atomic_read(&ok));
		break;
	case 4:
		wait_event_killable(martins3_waitq, atomic_read(&ok));
		break;
	case 5:
		wait_event(martins3_waitq, atomic_read(&ok));
		break;
	case 6:
		loop_wait();
		break;
	case 7:
		loop_wait2();
		break;
	case 8:
		loop_wait3();
		break;
	case 9:
		/*
		 * 如果 wait queue 上存在 waiter ，return true
		 *
		 * mod wait 9
		 * mod wait 5 &
		 * mod wait 9
		 */
		pr_info("[%d] \n", waitqueue_active(&martins3_waitq));
		break;
	case 10:
		/*
		 * 测试 init_waitqueue_func_entry
		 */
		init_waitqueue_func_entry_test();
		break;
	default:
		break;
	}

	return 0;
}
