#include "internal.h"
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/semaphore.h>

unsigned long counter;
static DEFINE_MUTEX(test_mutex_hi);
static DEFINE_MUTEX(test_mutex_hi2);
#define LOOP_NUM 10000000

static struct task_struct *holder;

static int mutex_lock_it(void *idx)
{
	mutex_lock(&test_mutex_hi);
	for (int i = 0; i < 1000; i++) {
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		msleep(1000);
	}
	mutex_unlock(&test_mutex_hi);
	return 1;
}

static void test_mutex_lock(void)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	mutex_lock(&test_mutex_hi);
	mutex_unlock(&test_mutex_hi);
}

static void test_mutex_lock_killable(void)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	// mutex_lock_interruptible 类似的，当在等待的时候，ctrl-c 都可以杀掉
	if (mutex_lock_killable(&test_mutex_hi))
		pr_info("lock killed");
}

/*
 * use case 参考 virtballoon_migratepage
 */
static void test_mutex_trylock(void)
{
	if (mutex_trylock(&test_mutex_hi)) {
		schedule_timeout_interruptible(HZ * 10);
		mutex_unlock(&test_mutex_hi);
	} else
		pr_info("mutex_trylock : 😀");
}
/*
 * 这个 API 也测试一下:
 * There are two ways to define new mutexes:
 * 1. Static mutexes can be generated at compile time by using `DEFINE_MUTEX` (be sure not to confuse this with DECLARE_MUTEX from the semaphore-based mutexes!).
 * 2. `mutex_init` dynamically initializes a new mutex at run time.
 * */

int test_mutex(long action)
{
	switch (action) {
	/*
	 * 测试基本的 api 使用
	 */
	case 0:
		// TODO 修改为 workqueue 吧
		if (!holder)
			holder = create_thread("holder", mutex_lock_it, NULL);
		break;
	case 1:
		test_mutex_lock();
		break;
	case 2:
		test_mutex_lock_killable();
		break;
	case 3:
		test_mutex_trylock();
		break;
	}
	return 0;
}
