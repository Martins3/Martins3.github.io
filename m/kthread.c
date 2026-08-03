#include "internal.h"
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kthread.h>

typedef int thread_function(void *);
struct kthread_func {
	thread_function *func;
	void *parameter;
	struct task_struct *kth;
};

static int kthread_runner(void *idx)
{
	struct kthread_func *func = idx;
	pr_info("[%s] %s\n", __FUNCTION__, current->comm);
	while (!kthread_should_stop()) {
		int ret = func->func(func->parameter);
		if (ret)
			return ret;
	}
	return 123;
}

static void stop_thread(struct task_struct *task)
{
	struct kthread_func *kf;
	int ret;
	if (task == NULL)
		return;
	ret = kthread_stop(task);
	/* of course, it's 123
	 */
	pr_info("kthread return : %d", ret);
	kf = container_of(&task, struct kthread_func, kth);
	kfree(kf);
}

static struct task_struct *create_thread(const char *name, thread_function func,
				  void *parameter)
{
	struct task_struct *kth;
	struct kthread_func *f =
		kmalloc(sizeof(struct kthread_func), GFP_KERNEL);
	if (f == NULL)
		return NULL;

	f->func = func;
	f->parameter = parameter;
	kth = kthread_create(kthread_runner, f, "martins3-%s", name);
	if (kth != NULL) {
		f->kth = kth;
		/**
		 * 创建了 kthread 之后需要立刻使用 wake_up_process 吗?
		 * 一般不是贴在一起的，但是一般是放在一个函数中的，例如 bch2_run_thread_with_file
		 */
		wake_up_process(kth);
	} else
		kfree(kth);
	return kth;
}

/*
 * kthread 默认的状态启动是 TS ，也就是 SCHED_OTHER
 * ➜  share ps -elf -c | grep sleep
 * 1 R root        1590       2 TS   19 -     0 -      17:17 ?        00:00:23 [martins3-sleep]
 *
 * 如果设置 sched_set_fifo_low(current); ，那么可以看到这个:
 *
 * [   77.164540] sched: RT throttling activated
 */
static int loop_one_second(void *arg)
{
	long rt = (long)arg;
	int slice = 10;

	if (rt == 1)
		sched_set_fifo_low(current);

	/* 这里说明，即使是中间释放了 CPU ，也会触发 RT throttling
	 * rt throttling 是统计过去一段时间中，rt thread 运行占用的时间。
	 */
	for (size_t i = 0; i < slice; i++) {
		unsigned long jiffies_at_begin = jiffies;
		while (time_after(jiffies_at_begin + HZ / slice, jiffies))
			cpu_relax();
		schedule();
	}

	return 1;
}

// 参考 arch/x86/kvm/i8254.c 来实现的
struct kthread_worker *thread_worker;
struct kthread_work basic;
static void pit_do_work(struct kthread_work *work)
{
	// 睡眠一会，来测试 flush
	//
	// 测试在用户态修改 kthread 的优先级:
	//
	// ps -e -o uid,pid,ppid,pri,ni,cmd | grep martins3
	//  0    1525       2  19   0 [martins3/12]
	//  pri 为 19 ，nice 为 0
	//
	//  sudo renice 8 1525
	//
	// 在此观测
	//     0    1525       2  11   8 [martins3/12]
	//
	// 所以，一直有一个误区，认为 kthread 用户态是没办法控制的
	// 其实不对，kthread 只是不去运行用户态代码而已。
	for (int i = 0; i < 10; i++) {
		pr_info("%s : %d\n", __FUNCTION__, i);
		if (schedule_timeout_interruptible(HZ))
			break;
	}
}

int test_kthread_init(void)
{
	pr_info("crate kthread worker\n");
	thread_worker = kthread_create_worker(0, "martins3/%d", 4396);
	kthread_init_work(&basic, pit_do_work);

	return 0;
}

int test_kthread_exit(void)
{
	// 会等到任务结束之后，rmmod 才可以结束
	kthread_destroy_worker(thread_worker);
	return 0;
}

int test_kthread(long action)
{
	switch (action) {
	// TODO
	// 1. 测试 kthread worker 相关的
	//
	// kthread worker 和 workqueue 的区别什么? 尤其是 kthread_queue_work 。
	// 当然，我们知道 workqueue 的也是 kthread 的封装。但是，workqueue 封装
	// 带来了什么好处和引入了什么限制 ?
	case 0:
		break;
	case 1:
		kthread_queue_work(thread_worker, &basic);
		break;
	case 2:
		kthread_flush_worker(thread_worker);
		break;
	// 2. 测试 rt thread 相关
	case 4:
		create_thread("sleep", loop_one_second, (void *)0);
		break;
	case 5:
		create_thread("sleep", loop_one_second, (void *)1);
		break;
	}
	return 0;
}
