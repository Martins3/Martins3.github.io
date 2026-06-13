/*
 * Message-Passing Litmus Test (C-MP+o-wmb-o+o-rmb-o)
 *
 * P0 writes a message (x0) then sets a flag (x1).
 * P1 reads the flag (x1) then reads the message (x0).
 *
 * Without proper barriers, P1 might see the flag set but read stale
 * message data.
 *
 * This test uses kernel-style primitives:
 *   smp_wmb() to order P0's stores
 *   smp_rmb() to order P1's loads
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

/* Simulate kernel primitives using C11 atomics */
#define WRITE_ONCE(var, val) \
	atomic_store_explicit(&(var), (val), memory_order_relaxed)
#define READ_ONCE(var) \
	atomic_load_explicit(&(var), memory_order_relaxed)

#define smp_wmb() atomic_thread_fence(memory_order_release)
#define smp_rmb() atomic_thread_fence(memory_order_acquire)

static atomic_int x0 = 0; /* message */
static atomic_int x1 = 0; /* flag  */

static void *p0(void *arg)
{
	(void)arg;
	WRITE_ONCE(x0, 42);
	smp_wmb();
	WRITE_ONCE(x1, 1);
	return NULL;
}

static void *p1(void *arg)
{
	(void)arg;
	int r2 = 0;
	while (r2 == 0)
		r2 = READ_ONCE(x1);
	smp_rmb();
	int r3 = READ_ONCE(x0);
	return (void *)(unsigned long)r3;
}

int main(int argc, char **argv)
{
	int iterations = (argc > 1) ? atoi(argv[1]) : 1000000;
	int trigger = 0;

	printf("Message-Passing Litmus Test\n");
	printf("Iterations: %d\n", iterations);

	for (int i = 0; i < iterations; i++) {
		atomic_store(&x0, 0);
		atomic_store(&x1, 0);

		pthread_t t0, t1;
		pthread_create(&t0, NULL, p0, NULL);
		pthread_create(&t1, NULL, p1, NULL);

		void *ret;
		pthread_join(t0, NULL);
		pthread_join(t1, &ret);

		int r3 = (int)(unsigned long)ret;
		if (r3 != 42) {
			trigger++;
		}
	}

	printf("Triggered %d times (flag set but message stale)\n", trigger);
	printf("Rate: %.6f%%\n", 100.0 * trigger / iterations);
	return 0;
}
