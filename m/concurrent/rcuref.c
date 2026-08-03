#include "internal.h"
// 测试 include/linux/rcuref.h
rcuref_t rcuref;
static void basic_test(void)
{
	rcuref_init(&rcuref, 2);
	rcu_read_lock();
	pr_info("%d\n", rcuref_get(&rcuref));
	pr_info("%d\n", rcuref_read(&rcuref));
	pr_info("%d\n", rcuref_put(&rcuref));
	pr_info("%d\n", rcuref_put(&rcuref));
	pr_info("%d\n", rcuref_put(&rcuref));
	// [29398.765198] 1
	// [29398.765254] 3
	// [29398.765261] 0
	// [29398.765267] 0
	// [29398.765273] 1
	rcu_read_unlock();
}

int test_rcuref(long action)
{
	switch (action) {
	case 1:
		basic_test();
		break;
	}
	return 0;
}
