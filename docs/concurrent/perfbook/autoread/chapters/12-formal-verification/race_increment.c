/*
 * race_increment.c
 *
 * Demonstrates the textbook race condition resulting from non-atomic
 * increment, corresponding to the Promela increment.spin model in
 * perfbook Chapter 12 (Formal Verification).
 *
 * Two threads fetch the counter, increment it, and store it back.
 * Because the fetch-increment-store is not atomic, one update can be lost.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUMPROCS 2
#define ITERATIONS 100000

static int counter = 0;

static void *incrementer(void *arg)
{
	(void)arg;
	for (int i = 0; i < ITERATIONS; i++) {
		int temp = counter;
		counter = temp + 1;
	}
	return NULL;
}

int main(void)
{
	pthread_t threads[NUMPROCS];

	for (int i = 0; i < NUMPROCS; i++)
		pthread_create(&threads[i], NULL, incrementer, NULL);

	for (int i = 0; i < NUMPROCS; i++)
		pthread_join(threads[i], NULL);

	printf("Expected counter: %d\n", NUMPROCS * ITERATIONS);
	printf("Actual counter:   %d\n", counter);

	if (counter != NUMPROCS * ITERATIONS) {
		printf("RACE DETECTED: non-atomic increment lost updates!\n");
		return 1;
	}

	printf("OK (unlikely with enough iterations)\n");
	return 0;
}
