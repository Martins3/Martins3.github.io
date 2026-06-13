/*
 * atomic.c: Demonstrate GCC classic atomics, C11 atomics,
 * and modern GCC __atomic intrinsics.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>

#define NTHREADS 4
#define NINCREMENTS 100000

/* Classic GCC __sync atomic */
static long counter_sync = 0;

/* C11 atomic */
static _Atomic long counter_c11 = 0;

/* Modern GCC __atomic */
static long counter_modern = 0;

void *worker_sync(void *arg)
{
	int i;
	(void)arg;
	for (i = 0; i < NINCREMENTS; i++)
		__sync_fetch_and_add(&counter_sync, 1);
	return NULL;
}

void *worker_c11(void *arg)
{
	int i;
	(void)arg;
	for (i = 0; i < NINCREMENTS; i++)
		atomic_fetch_add_explicit(&counter_c11, 1, memory_order_relaxed);
	return NULL;
}

void *worker_modern(void *arg)
{
	int i;
	(void)arg;
	for (i = 0; i < NINCREMENTS; i++)
		__atomic_fetch_add(&counter_modern, 1, __ATOMIC_RELAXED);
	return NULL;
}

static void run_test_long(const char *name, void *(*worker)(void *), long *counter)
{
	pthread_t tid[NTHREADS];
	int i;

	*counter = 0;
	for (i = 0; i < NTHREADS; i++) {
		if (pthread_create(&tid[i], NULL, worker, NULL) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < NTHREADS; i++)
		pthread_join(tid[i], NULL);

	printf("%s: expected %ld, got %ld\n",
	       name, (long)NTHREADS * NINCREMENTS, *counter);
}

static void run_test_atomic(const char *name, void *(*worker)(void *), _Atomic long *counter)
{
	pthread_t tid[NTHREADS];
	int i;

	*counter = 0;
	for (i = 0; i < NTHREADS; i++) {
		if (pthread_create(&tid[i], NULL, worker, NULL) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < NTHREADS; i++)
		pthread_join(tid[i], NULL);

	printf("%s: expected %ld, got %ld\n",
	       name, (long)NTHREADS * NINCREMENTS, (long)*counter);
}

int main(void)
{
	printf("=== Atomic Operations Demo ===\n\n");

	run_test_long("__sync_fetch_and_add", worker_sync, &counter_sync);
	run_test_atomic("atomic_fetch_add (C11)", worker_c11, &counter_c11);
	run_test_long("__atomic_fetch_add (modern GCC)", worker_modern, &counter_modern);

	/* CAS demo */
	long val = 42;
	long old = __sync_val_compare_and_swap(&val, 42, 100);
	printf("\nCAS demo: old=%ld, new=%ld (expected old=42, new=100)\n", old, val);

	/* C11 CAS */
	_Atomic long c11_val = 42;
	long expected = 42;
	atomic_compare_exchange_strong(&c11_val, &expected, 100);
	printf("C11 CAS: result=%ld (expected 100)\n", (long)c11_val);

	return 0;
}
