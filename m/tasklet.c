#include "linux/interrupt.h"
#include "internal.h"

static DEFINE_PER_CPU(struct tasklet_struct, tsq_tasklet);

static void tcp_tasklet_func(struct tasklet_struct *t)
{
	/*
	 * mod tasklet 0 可以得到:
	 *
	 * [   57.589468] Call trace:
	 * [   57.590240]  dump_backtrace+0xfc/0x17c
	 * [   57.591725]  show_stack+0x18/0x24
	 * [   57.593540]  dump_stack_lvl+0x40/0x84
	 * [   57.595437]  dump_stack+0x18/0x24
	 * [   57.597017]  tcp_tasklet_func+0x24/0x30 [martins3]
	 * [   57.599218]  tasklet_action_common+0x190/0x1f0
	 * [   57.601222]  tasklet_action+0x2c/0x38
	 * [   57.602789]  handle_softirqs+0xd4/0x1fc
	 * [   57.604193]  run_ksoftirqd+0x2c/0xc8
	 * [   57.605298]  smpboot_thread_fn+0x1e4/0x24c
	 * [   57.606541]  kthread+0x104/0x18c
	 * [   57.607533]  ret_from_fork+0x10/0x20
	 *
	 */
	pr_info("%s\n", __FUNCTION__);
	dump_stack();
}

int test_tasklet_init(void)
{
	int i;
	for_each_possible_cpu(i) {
		struct tasklet_struct *tsq = &per_cpu(tsq_tasklet, i);
		tsq->data += 0x1000;
	}
	tasklet_setup(NULL, tcp_tasklet_func);
	return 0;
}

int test_tasklet_exit(void)
{
	return 0;
}

int test_tasklet(long action)
{
	switch (action) {
	case 0:
		tasklet_schedule(this_cpu_ptr(&tsq_tasklet));
		break;
	case 1:
		/*
		 * TODO 没有细究 hi ，测试发现 tasklet 和 hi 是取决于用哪个函数来调用的
		 */
		tasklet_hi_schedule(this_cpu_ptr(&tsq_tasklet));
		break;
	}
	return 0;
}
