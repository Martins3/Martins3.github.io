/*
 * atomic_increment.c
 *
 * Fixes the race condition in race_increment.c by using atomic operations.
 * Corresponds to the atomic-increment fix in perfbook Chapter 12.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define NUMPROCS 2
#define ITERATIONS 100000

static atomic_int counter = 0;

static void *incrementer(void *arg)
{
	(void)arg;
	for (int i = 0; i < ITERATIONS; i++)
		atomic_fetch_add(&counter, 1);
	return NULL;
}

int main(void)
{
	pthread_t threads[NUMPROCS];

	for (int i = 0; i < NUMPROCS; i++)
		pthread_create(&threads[i], NULL, incrementer, NULL);

	for (int i = 0; i < NUMPROCS; i++)
		pthread_join(threads[i], NULL);

	int expected = NUMPROCS * ITERATIONS;
	int actual = atomic_load(&counter);

	printf("Expected counter: %d\n", expected);
	printf("Actual counter:   %d\n", actual);

	if (actual != expected) {
		printf("FAILED: atomic increment still lost updates?\n");
		return 1;
	}

	printf("OK: atomic increment is race-free.\n");
	return 0;
}
