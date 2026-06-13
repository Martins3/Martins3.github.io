/*
 * Release-Acquire Chain Litmus Test
 *
 * Demonstrates that a release-acquire pair can propagate
 * visibility across threads without full memory barriers.
 *
 * P0 writes data then release-stores to A.
 * P1 acquire-loads A, writes more data, then release-stores to B.
 * P2 acquire-loads B, then reads P0's original data.
 *
 * The release-acquire chain ensures P2 sees P0's writes.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define WRITE_ONCE(var, val) \
	atomic_store_explicit(&(var), (val), memory_order_relaxed)
#define READ_ONCE(var) \
	atomic_load_explicit(&(var), memory_order_relaxed)

static atomic_int data = 0;
static atomic_int flag_a = 0;
static atomic_int flag_b = 0;

static void *p0(void *arg)
{
	(void)arg;
	WRITE_ONCE(data, 42);
	atomic_store_explicit(&flag_a, 1, memory_order_release);
	return NULL;
}

static void *p1(void *arg)
{
	(void)arg;
	int r = 0;
	while (r == 0)
		r = atomic_load_explicit(&flag_a, memory_order_acquire);
	/* At this point, P1 is guaranteed to see data == 42 */
	atomic_store_explicit(&flag_b, 1, memory_order_release);
	return NULL;
}

static void *p2(void *arg)
{
	(void)arg;
	int r = 0;
	while (r == 0)
		r = atomic_load_explicit(&flag_b, memory_order_acquire);
	/* The chain P0->P1->P2 ensures we see data == 42 */
	int val = READ_ONCE(data);
	return (void *)(unsigned long)val;
}

int main(int argc, char **argv)
{
	int iterations = (argc > 1) ? atoi(argv[1]) : 1000000;
	int trigger = 0;

	printf("Release-Acquire Chain Litmus Test\n");
	printf("Iterations: %d\n", iterations);

	for (int i = 0; i < iterations; i++) {
		atomic_store(&data, 0);
		atomic_store(&flag_a, 0);
		atomic_store(&flag_b, 0);

		pthread_t t0, t1, t2;
		pthread_create(&t0, NULL, p0, NULL);
		pthread_create(&t1, NULL, p1, NULL);
		pthread_create(&t2, NULL, p2, NULL);

		void *ret;
		pthread_join(t0, NULL);
		pthread_join(t1, NULL);
		pthread_join(t2, &ret);

		int val = (int)(unsigned long)ret;
		if (val != 42) {
			trigger++;
		}
	}

	printf("Triggered %d times (data not propagated through chain)\n", trigger);
	printf("Rate: %.6f%%\n", 100.0 * trigger / iterations);
	return 0;
}
