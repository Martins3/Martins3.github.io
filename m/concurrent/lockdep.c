#include "internal.h"
/*
 * 这种情况会被 lockdep 检测到，分析细节在
 * docs/concurrent/kernel/lockdep-usage.md
 */

int __rcu *blkg_hint;
int test_lockdep_init(void)
{
	return 0;
}

int test_lockdep_exit(void)
{
	return 0;
}

static DEFINE_MUTEX(test_mutex_hi);
static DEFINE_MUTEX(test_mutex_hi2);

/*
 *
 * 这种不会被 lockdep 检测到，但是还是会卡住，最后 hung task 。
 */
static void test_nested_lock(void)
{
	mutex_lock_nested(&test_mutex_hi, 0);
	mutex_lock_nested(&test_mutex_hi, 1);
}

static void test_nested_lock2(void)
{
	mutex_lock(&test_mutex_hi);
	mutex_lock(&test_mutex_hi);
}

/*
 * 这种情况会被 lockdep 检测到，这种情况指的是连续的 mutex lock 多个
 */
static void test_nested_lock3(void)
{
	mutex_lock(&test_mutex_hi);
	mutex_lock(&test_mutex_hi2);
}

int test_lockdep(long action)
{
	switch (action) {
	case 0:
		// 测试最简单的:
		rcu_read_lock();
		debug_show_held_locks(current);
		rcu_read_unlock();
		break;
		// 测试 lockdep_assert_held
	case 1:
		// 睡眠 100S
		schedule_timeout_interruptible(1000000);
		break;
	case 5:
		test_nested_lock2();
		break;
	case 6:
		test_nested_lock3();
		break;
	case 7:
		test_nested_lock();
		break;
	case 8:
		lockdep_assert_held(&test_mutex_hi);
		break;
	case 9:
		mutex_lock(&test_mutex_hi);
		lockdep_assert_held(&test_mutex_hi);
		mutex_unlock(&test_mutex_hi);
		break;
	case 10:
		// 测试 rcu_dereference 中自动包含了一个 rcu lock 的检查
		// 所以，这里会触发一个警告
		rcu_dereference(blkg_hint);
		break;
	case 11:
		// 测试 __rcu 的作用，似乎是可以直接访问，应该有什么工具打开吧
		blkg_hint = NULL;
		break;
	}
	return 0;
}
