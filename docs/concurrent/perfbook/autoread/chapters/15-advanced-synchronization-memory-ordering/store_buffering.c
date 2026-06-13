/*
 * Store-Buffering Litmus Test (C-SB+o-o+o-o)
 *
 * Demonstrates that without memory barriers, both threads can
 * simultaneously read the old value (0) from the other thread's variable.
 *
 * On x86, this requires using C11 relaxed atomics to avoid compiler
 * ordering and x86's implicit strong ordering on plain accesses.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

static atomic_int x0 = 0;
static atomic_int x1 = 0;

/*
 * The above loop-based test is flawed for observing the race because
 * the threads run many iterations without resetting between them.
 * A proper per-iteration test:
 */
static void *p0_once(void *arg)
{
	(void)arg;
	atomic_store_explicit(&x0, 1, memory_order_relaxed);
	int r2 = atomic_load_explicit(&x1, memory_order_relaxed);
	return (void *)(unsigned long)r2;
}

static void *p1_once(void *arg)
{
	(void)arg;
	atomic_store_explicit(&x1, 1, memory_order_relaxed);
	int r2 = atomic_load_explicit(&x0, memory_order_relaxed);
	return (void *)(unsigned long)r2;
}

int main(int argc, char **argv)
{
	int iterations = (argc > 1) ? atoi(argv[1]) : 10000000;
	int trigger = 0;

	printf("Store-Buffering Litmus Test\n");
	printf("Iterations: %d\n", iterations);

	for (int i = 0; i < iterations; i++) {
		atomic_store(&x0, 0);
		atomic_store(&x1, 0);

		pthread_t t0, t1;
		pthread_create(&t0, NULL, p0_once, NULL);
		pthread_create(&t1, NULL, p1_once, NULL);

		void *ret0, *ret1;
		pthread_join(t0, &ret0);
		pthread_join(t1, &ret1);

		int r0 = (int)(unsigned long)ret0;
		int r1 = (int)(unsigned long)ret1;

		if (r0 == 0 && r1 == 0) {
			trigger++;
		}
	}

	printf("Triggered %d times (both loads saw 0)\n", trigger);
	printf("Rate: %.6f%%\n", 100.0 * trigger / iterations);
	return 0;
}
