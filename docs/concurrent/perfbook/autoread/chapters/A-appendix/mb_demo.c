/*
 * mb_demo.c: Demonstrate why memory barriers are needed.
 *
 * Based on perfbook appendix/whymb/whymemorybarriers.tex.
 *
 * This program demonstrates the store-buffering problem:
 *   CPU 0:  a = 1; b = 1;
 *   CPU 1:  while (b == 0); assert(a == 1);
 *
 * Without memory barriers, the assertion can fail on weakly-ordered
 * hardware because CPU 0's store to 'a' may sit in its store buffer
 * while the store to 'b' becomes visible to CPU 1.
 *
 * Build: make mb_demo.out
 * Run:   ./mb_demo.out [iterations]
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>

static atomic_int a = 0;
static atomic_int b = 0;
static atomic_int ready = 0;

/*
 * Without memory barriers: the compiler and CPU are free to reorder
 * the stores to 'a' and 'b'.  On x86 TSO, store-store reordering is
 * not allowed, but this is still a good illustration of the concept.
 * We use memory_order_relaxed to allow the compiler maximum freedom.
 */
static void *writer_no_mb(void *arg)
{
	(void)arg;
	atomic_store_explicit(&a, 1, memory_order_relaxed);
	atomic_store_explicit(&b, 1, memory_order_relaxed);
	return NULL;
}

static void *reader_no_mb(void *arg)
{
	(void)arg;
	while (atomic_load_explicit(&b, memory_order_relaxed) == 0)
		;
	int av = atomic_load_explicit(&a, memory_order_relaxed);
	if (av != 1) {
		/* This indicates we observed b=1 before a=1 */
		return (void *)(uintptr_t)1;
	}
	return NULL;
}

/*
 * With proper memory ordering:
 *   writer: release store to b ensures all prior stores are visible
 *   reader: acquire load from b ensures all subsequent loads see prior stores
 */
static void *writer_with_mb(void *arg)
{
	(void)arg;
	atomic_store_explicit(&a, 1, memory_order_relaxed);
	atomic_store_explicit(&b, 1, memory_order_release);
	return NULL;
}

static void *reader_with_mb(void *arg)
{
	(void)arg;
	while (atomic_load_explicit(&b, memory_order_acquire) == 0)
		;
	int av = atomic_load_explicit(&a, memory_order_relaxed);
	if (av != 1)
		return (void *)(uintptr_t)1;
	return NULL;
}

static int run_test(void *(*wr)(void *), void *(*rd)(void *), int iters)
{
	int failures = 0;
	for (int i = 0; i < iters; i++) {
		pthread_t tw, tr;
		atomic_store(&a, 0);
		atomic_store(&b, 0);
		pthread_create(&tw, NULL, wr, NULL);
		pthread_create(&tr, NULL, rd, NULL);
		void *res;
		pthread_join(tw, NULL);
		pthread_join(tr, &res);
		if (res)
			failures++;
	}
	return failures;
}

int main(int argc, char *argv[])
{
	int iters = 1000000;
	if (argc > 1)
		iters = atoi(argv[1]);

	printf("Memory barrier demo (%d iterations)\n", iters);
	printf("Note: on x86 TSO, store-store reordering does not happen,\n");
	printf("so the 'no barrier' case rarely fails.  The test still\n");
	printf("demonstrates the required programming discipline.\n\n");

	printf("Running without explicit memory ordering...\n");
	int fail1 = run_test(writer_no_mb, reader_no_mb, iters);
	printf("  failures: %d (%.4f%%)\n", fail1,
	       100.0 * (double)fail1 / (double)iters);

	printf("Running with release/acquire ordering...\n");
	int fail2 = run_test(writer_with_mb, reader_with_mb, iters);
	printf("  failures: %d (%.4f%%)\n", fail2,
	       100.0 * (double)fail2 / (double)iters);

	return 0;
}
