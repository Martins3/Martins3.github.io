#include "internal.h"
#include <linux/semaphore.h>

static int ll_x, ll_y;
static int ll_r1, ll_r2;

static DEFINE_SEMAPHORE(sem_x, 0);
static DEFINE_SEMAPHORE(sem_y, 0);
static DEFINE_SEMAPHORE(sem_end, 0);

static bool should_stop;

static void ll_thread0(struct work_struct *work)
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

static void ll_thread1(struct work_struct *work)
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
static void ll_thread2(struct work_struct *work)
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
		}

		counter++;
	}
}
