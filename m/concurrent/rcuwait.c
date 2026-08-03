#include <linux/rcuwait.h>
#include "internal.h"
static struct rcuwait manager_wait = __RCUWAIT_INITIALIZER(manager_wait);

static int condition;

/*
 * 这个 API 相当简单了，但是需要记住，rcuwait_wait_event 是在等待 condition
 *
 * 1. 如果 condition 已经满足了，rcuwait_wait_event 会立刻返回
 * 2. 如果 condition 没有满足，那么 rcuwait_wake_up 并不会带来什么效果
 *
 * 通常的时候，使用模式是
 *
 * condition = 1
 * rcuwait_wake_up
 */
int test_rcuwait(long action)
{
	switch (action) {
	case 0:
		rcuwait_wait_event(&manager_wait, condition,
				   TASK_UNINTERRUPTIBLE);
		break;
	case 1:
		rcuwait_wake_up(&manager_wait);
		break;
	case 2:
		condition = 1;
		break;
	case 3:
		condition = 0;
		break;
	}

	return 0;
}
