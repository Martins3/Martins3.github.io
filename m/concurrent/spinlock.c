#include "internal.h"
#include <linux/spinlock.h>

// TODO spinlock 的实现似乎有很多变种
// 1. raw_spin_lock_init
// 2. 测试下 pv spinlock 吧

int test_spinlock(long action)
{
	switch (action) {
	case 0:
		break;
	}
	return 0;
}
