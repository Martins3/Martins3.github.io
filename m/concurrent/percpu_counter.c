#include "internal.h"
// TODO 思考一个问题，含有上界的 counter 如何实现?

// TODO 似乎只有两个 worker 的时候，这个结果不是很符合预期
//
// 不用锁:
//	cost 310 ns
//  	cost 230 ns
// atomic:
//	cost 157958812 ns
//	cost 155045076 ns
// counter:
//	cost 185853822 ns
//	cost 185853822 ns

static struct percpu_counter s_dirs_counter;
static atomic_t atoc_counter;
static int none;

enum sum_way {
	ATOMIC,
	COUNTER,
	NONE,
};
struct test {
	enum sum_way use_counter;
	struct work_struct work;
};

static void do_inc(struct work_struct *work)
{
	struct test *test = container_of(work, struct test, work);
	int i;
	ktime_t start = ktime_get();
	switch (test->use_counter) {
	case ATOMIC:
		for (i = 0; i < 100000000; i++)
			atomic_inc(&atoc_counter);
		break;
	case COUNTER:
		for (i = 0; i < 100000000; i++)
			percpu_counter_inc(&s_dirs_counter);
		break;
	case NONE:
		for (i = 0; i < 100000000; i++)
			none++;
		break;
	}
	pr_info("cost %lld ns\n", ktime_get() - start);
}

int test_percpu_counter_init(void)
{
	return percpu_counter_init(&s_dirs_counter, 10, GFP_KERNEL);
}
int test_percpu_counter_exit(void)
{
	percpu_counter_destroy(&s_dirs_counter);
	return 0;
}

static struct test worker1;
static struct test worker2;

static void test(enum sum_way way)
{
	worker1.use_counter = way;
	worker2.use_counter = way;
	INIT_WORK(&worker1.work, do_inc);
	INIT_WORK(&worker2.work, do_inc);
	schedule_work(&worker1.work);
	schedule_work(&worker2.work);
	flush_work(&worker1.work);
	flush_work(&worker2.work);

	switch (way) {
	case ATOMIC:
		pr_info("atomic_read : %d\n", atomic_read(&atoc_counter));
		break;
	case COUNTER:
		pr_info("percpu_counter_sum  : %lld\n",
			percpu_counter_sum(&s_dirs_counter));
		pr_info("percpu_counter_read : %lld\n",
			percpu_counter_read(&s_dirs_counter));
		break;
	case NONE:
		pr_info("%d\n", none);
		break;
	}

	none = 0;
	atomic_set(&atoc_counter, 0);
	percpu_counter_set(&s_dirs_counter, 0);
}

int test_percpu_counter(long action)
{
	// TODO 这个写的不够通用，参考 concurrent/atomic.c 实现任意多的并发
	test(action);
	return 0;
}
