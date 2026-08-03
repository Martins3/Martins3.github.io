#include "internal.h"
#include <linux/delay.h>
#include <linux/semaphore.h>

// 参考: https://github.com/smcdef/memory-reordering
static bool should_stop;
struct work {
	struct work_struct work;
	int id;
};

static long x;
static long y;
static bool run = true;
noinline static void writer(void)
{
	while (run && !should_stop) {
		x = 1;
		barrier();
		y = 1;

		barrier();
		y = 0;
		barrier();
		x = 0;
		smp_mb();
	}
}

noinline static void reader(void)
{
	while (should_stop) {
		if (y == 1 && x == 0) {
			pr_info("found 🐭\n");
			break;
		}
	}
	run = false;
}

static void test_failed_logic(struct work_struct *work)
{
	struct work *test = container_of(work, struct work, work);
	switch (test->id) {
	case 0:
		writer();
		break;
	case 1:
		reader();
		break;
	default:
		msleep(8000);
		should_stop = true;
	}
}

static atomic_t count = ATOMIC_INIT(0);
static unsigned int a, b;
static void ss_thread0(void)
{
	while (!should_stop) {
		atomic_inc(&count);
	}
}

static void ss_thread1(void)
{
	while (!should_stop) {
		int temp = atomic_read(&count);

		a = temp;
#ifdef CONFIG_USE_CPU_BARRIER
		smp_wmb();
#else
		/* Prevent compiler reordering. */
		barrier();
#endif
		b = temp;
	}
}

static void ss_thread2(void)
{
	int counter = 0;
	while (!should_stop) {
		unsigned int c, d;

		d = b;
#ifdef CONFIG_USE_CPU_BARRIER
		smp_rmb();
#else
		/* Prevent compiler reordering. */
		barrier();
#endif
		c = a;

		if ((int)(d - c) > 0) {
			pr_info("reorders detected, a = %d, b = %d , counter=%d\n",
				c, d, counter);
			should_stop = true;
		}

		counter++;
	}
}

static void test_ss_logic(struct work_struct *work)
{
	struct work *test = container_of(work, struct work, work);
	switch (test->id) {
	case 0:
		ss_thread0();
		break;
	case 1:
		ss_thread1();
		break;
	case 2:
		ss_thread2();
		break;
	default:
		msleep(8000);
		should_stop = true;
	}
}

static int ll_x, ll_y;
static int ll_r1, ll_r2;

static DEFINE_SEMAPHORE(sem_x, 0);
static DEFINE_SEMAPHORE(sem_y, 0);
static DEFINE_SEMAPHORE(sem_end, 0);

static void ll_thread0(void)
{
	while (!should_stop) {
		down(&sem_x);
		ll_x = 1;
#ifdef CONFIG_USE_CPU_BARRIER
		smp_mb();
#else
		/* Prevent compiler reordering. */
		barrier();
#endif
		ll_y = 1;
		up(&sem_end);
	}
}

static void ll_thread1(void)
{
	while (!should_stop) {
		down(&sem_y);
		ll_r1 = ll_x;
#ifdef CONFIG_USE_CPU_BARRIER
		smp_mb();
#else
		/* Prevent compiler reordering. */
		barrier();
#endif
		ll_r2 = ll_y;
		up(&sem_end);
	}
}

/* The Watcher */
static void ll_thread2(void)
{
	int counter = 0;
	while (!should_stop) {
		/* Reset x and y. */
		ll_x = 0;
		ll_y = 0;

		up(&sem_x);
		up(&sem_y);

		down(&sem_end);
		down(&sem_end);

		if (ll_r1 == 1 && ll_r2 == 0) {
			pr_info("reorders detected, counter=%d\n", counter);
			should_stop = true;
			up(&sem_x);
			up(&sem_y);
		}

		counter++;
	}
}

static void test_ll_logic(struct work_struct *work)
{
	struct work *test = container_of(work, struct work, work);
	switch (test->id) {
	case 0:
		ll_thread0();
		break;
	case 1:
		ll_thread1();
		break;
	case 2:
		ll_thread2();
		break;
	default:
		msleep(8000);
		should_stop = true;
	}
}

static void init_func(void *work, int id)
{
	struct work *test = (struct work *)work;
	test->id = id;
}

// TODO 这里的 API 到时候再去修改吧
int test_memory_model(long action)
{
	should_stop = false;
	switch (action) {
	case 0:
		return batch_queue_works(test_failed_logic, init_func, 3,
					 sizeof(struct work));
	case 1:
		return batch_queue_works(test_ss_logic, init_func, 4,
					 sizeof(struct work));
	case 2:
		return batch_queue_works(test_ll_logic, init_func, 4,
					 sizeof(struct work));
	}
	return 0;
}
