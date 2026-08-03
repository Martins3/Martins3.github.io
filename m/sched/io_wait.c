#include "internal.h"
#include <linux/delay.h>

/* 使用这个测试可以得到一个相当有趣的结果:
 * 1. guest 中 htop 上 32 CPU 显示都是 0%
 * 2. /proc/load_avg 持续升高，一直到 32
 *
 * 这个需要改造一下:
 */
static int io_wait_sleep(void *arg)
{
	io_schedule();
	msleep(1000);
	return 0;
}

static struct task_struct *io_wait_threads[32];
int test_io_wait(long action)
{
	if (action && io_wait_threads[0])
		return -EINVAL;

	if (action == 0 && io_wait_threads[0] == NULL)
		return -EINVAL;

	if (action)
		for (int i = 0; i < 32; i++) {
			io_wait_threads[i] =
				create_thread("wait", io_wait_sleep, NULL);
			if (!io_wait_threads[i])
				BUG();
		}
	else
		for (int i = 0; i < 32; i++) {
			stop_thread(io_wait_threads[i]);
			io_wait_threads[i] = NULL;
		}

	return 0;
}
