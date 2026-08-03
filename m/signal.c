#include "internal.h"
#include <linux/sched/signal.h>
int test_signal(long action)
{
	switch (action) {
	case 0:
		/*
		 * 配合 signal.cpp 来测试 ERESTARTNOINTR 的功能
		 * 这个机制真的非常邪乎，直接让 syscall 在内核中重新调用
		 */
		return restart_syscall();
		break;
	}
	return 0;
}
