#define pr_fmt(fmt) "srcu : " fmt
#include "internal.h"
#include <linux/delay.h>

/*
 * 测试的效果非常的简单，就是在 srcu_read_lock 中是可以睡眠的，
 * 而 writer 真的需要等待到结束。
 * TODO 先看文档，把 API 全部都补齐再说，不要看实现
 *
 * call_rcu 也是可以在 srcu 中使用的
 */
DEFINE_STATIC_SRCU(srcu);

static int srcu_reader_thread(void)
{
	int srcu_idx;
	srcu_idx = srcu_read_lock(&srcu);
	msleep(5000);
	pr_info("reader finished\n");
	srcu_read_unlock(&srcu, srcu_idx);
	return 0;
}

static int srcu_writer_thread(void)
{
	synchronize_srcu(&srcu);
	pr_info("write synchronize finished\n");
	return 0;
}

static void test_failed_logic(struct work_struct *work)
{
	struct work *test = (struct work *)work;
	pr_info("run %d\n", test->id);
	switch (test->id) {
	case 0:
		srcu_writer_thread();
		break;
	case 1:
		srcu_reader_thread();
		break;
	default:
		break;
	}
}

int test_srcu(long action)
{
	return batch_queue_works(test_failed_logic, 2, sizeof(struct work));
}
