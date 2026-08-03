#include "internal.h"
#include <linux/eventfd.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/fdtable.h>
#include <linux/rcupdate.h>
#include <linux/eventfd.h>

static struct eventfd_ctx *efd_ctx = NULL;
static int register_fd(u32 fd)
{
	/* 这个 fd 是用户态程序传递过来的
	 */
	efd_ctx = eventfd_ctx_fdget(fd);
	if (!efd_ctx) {
		pr_info("fdget failed");
		return -1;
	}
	return 0;
}

static int tigger_fd(void)
{
	if (!efd_ctx)
		return -1;
	eventfd_signal_mask(efd_ctx, 0x1234);
	eventfd_ctx_put(efd_ctx); /* TODO 每次 trigger 都需要 put 吗？ */
	return 0;
}

int test_eventfd(long action)
{
	if (!action)
		return tigger_fd();
	return register_fd(action);
}
