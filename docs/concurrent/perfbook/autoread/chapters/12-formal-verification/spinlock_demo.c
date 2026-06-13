/*
 * spinlock_demo.c
 *
 * Demonstrates a simple test-and-set spinlock protecting a shared counter.
 * Corresponds to the Promela spinlock model in perfbook Chapter 12.
 *
 * Multiple locker threads repeatedly acquire the lock, do work, and unlock.
 * An atomic counter tracks how many threads are in the critical section.
 * If two threads ever enter simultaneously, the counter exceeds 1 and we
 * record the event. Because the spinlock is correct, this should never happen.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdatomic.h>

#define N_LOCKERS 3
#define ITERATIONS 50000

/* Simple test-and-set spinlock */
static atomic_int mutex = 0;
static atomic_int in_critical = 0;
static atomic_int max_concurrent = 0;

static void spin_lock(void)
{
	while (atomic_exchange(&mutex, 1) == 1)
		; /* spin */
}

static void spin_unlock(void)
{
	atomic_store(&mutex, 0);
}

static void *locker(void *arg)
{
	int me = (intptr_t)arg;
	(void)me;

	for (int i = 0; i < ITERATIONS; i++) {
		spin_lock();
		int prev = atomic_fetch_add(&in_critical, 1);
		int current = prev + 1;
		if (current > atomic_load(&max_concurrent))
			atomic_store(&max_concurrent, current);
		/* critical section: simulate some work */
		atomic_fetch_sub(&in_critical, 1);
		spin_unlock();
	}
	return NULL;
}

int main(void)
{
	pthread_t threads[N_LOCKERS];

	for (int i = 0; i < N_LOCKERS; i++)
		pthread_create(&threads[i], NULL, locker, (void *)(intptr_t)i);

	for (int i = 0; i < N_LOCKERS; i++)
		pthread_join(threads[i], NULL);

	int max = atomic_load(&max_concurrent);
	printf("Max concurrent critical sections observed: %d\n", max);

	if (max > 1) {
		printf("ASSERTION FAILED: lock did not prevent concurrent entry!\n");
		return 1;
	}

	printf("Spinlock test passed: mutual exclusion holds.\n");
	return 0;
}
