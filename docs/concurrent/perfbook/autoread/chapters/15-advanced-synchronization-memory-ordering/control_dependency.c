/*
 * Control-Dependency Demonstration
 *
 * A control dependency occurs when a load's value determines whether
 * a later store executes. Control dependencies provide ordering from
 * loads to stores, but NOT from loads to loads.
 *
 * This program demonstrates the load->store ordering that control
 * dependencies do provide.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define WRITE_ONCE(var, val) \
	atomic_store_explicit(&(var), (val), memory_order_relaxed)
#define READ_ONCE(var) \
	atomic_load_explicit(&(var), memory_order_relaxed)

static atomic_int x = 0;
static atomic_int y = 0;

static void *p0(void *arg)
{
	(void)arg;
	WRITE_ONCE(x, 1);
	return NULL;
}

static void *p1(void *arg)
{
	(void)arg;
	int r1 = READ_ONCE(x);
	if (r1 == 1) {
		/* Control dependency: this store is ordered after the load */
		WRITE_ONCE(y, 1);
	}
	return NULL;
}

static void *p2(void *arg)
{
	(void)arg;
	int r2 = READ_ONCE(y);
	if (r2 == 1) {
		/* Can we rely on seeing x == 1? Not with control dependency alone! */
		int r3 = READ_ONCE(x);
		return (void *)(unsigned long)r3;
	}
	return (void *)0;
}

int main(int argc, char **argv)
{
	int iterations = (argc > 1) ? atoi(argv[1]) : 1000000;
	printf("Control-Dependency Demonstration\n");
	printf("Iterations: %d\n", iterations);
	printf("Note: On x86, control dependency load->store is naturally respected.\n");
	printf("But control dependency load->load is NOT guaranteed on weakly ordered CPUs.\n\n");

	for (int i = 0; i < iterations; i++) {
		atomic_store(&x, 0);
		atomic_store(&y, 0);

		pthread_t t0, t1, t2;
		pthread_create(&t0, NULL, p0, NULL);
		pthread_create(&t1, NULL, p1, NULL);
		pthread_create(&t2, NULL, p2, NULL);

		void *ret;
		pthread_join(t0, NULL);
		pthread_join(t1, NULL);
		pthread_join(t2, &ret);

		int r3 = (int)(unsigned long)ret;
		/* This is not a standard litmus test, just a demonstration */
		(void)r3;
	}

	printf("This test is for educational purposes.\n");
	printf("Control dependencies are fragile; prefer smp_load_acquire().\n");
	return 0;
}
