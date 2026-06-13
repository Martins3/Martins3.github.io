/*
 * litmus_sb.c
 *
 * Demonstrates the Store Buffering (SB) litmus test pattern discussed
 * in perfbook Chapter 12 Section 12.2 (PPCMEM).
 *
 * Two threads write to two different variables and then read the other
 * variable. On weakly ordered hardware (or without proper barriers),
 * both threads may read the old value (0), which is the outcome:
 *   r1 = 0, r2 = 0
 *
 * This program uses plain loads/stores to show that this outcome is
 * possible on modern CPUs due to store buffering. Adding smp_mb()
 * (or memory_order_seq_cst) removes the possibility.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

static atomic_int x = 0;
static atomic_int y = 0;

/* Result variables (non-atomic for observation) */
static int r1 = 0;
static int r2 = 0;

static void *p0(void *arg)
{
	(void)arg;
	atomic_store_explicit(&x, 1, memory_order_relaxed);
	r1 = atomic_load_explicit(&y, memory_order_relaxed);
	return NULL;
}

static void *p1(void *arg)
{
	(void)arg;
	atomic_store_explicit(&y, 1, memory_order_relaxed);
	r2 = atomic_load_explicit(&x, memory_order_relaxed);
	return NULL;
}

int main(void)
{
	int sb_count = 0;
	int trials = 100000;

	printf("Running SB litmus test (%d trials, relaxed MO)...\n", trials);

	for (int t = 0; t < trials; t++) {
		atomic_store(&x, 0);
		atomic_store(&y, 0);
		r1 = 0;
		r2 = 0;

		pthread_t t0, t1;
		pthread_create(&t0, NULL, p0, NULL);
		pthread_create(&t1, NULL, p1, NULL);
		pthread_join(t0, NULL);
		pthread_join(t1, NULL);

		if (r1 == 0 && r2 == 0)
			sb_count++;
	}

	printf("Outcome r1=0 && r2=0 observed: %d times (%.4f%%)\n",
	       sb_count, 100.0 * sb_count / trials);
	printf("This outcome is allowed on weakly ordered hardware.\n");

	return 0;
}
