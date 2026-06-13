/*
 * privatization.c: Demonstrate privatization via Dining Philosophers.
 *
 * Classic solution: 5 philosophers, 5 forks (shared between neighbors).
 * With a simple lock ordering, deadlock can occur when all pick up left
 * fork simultaneously. We use asymmetric ordering (last philosopher picks
 * right first) to avoid deadlock.
 *
 * Privatized solution: each philosopher gets their own private pair of forks.
 * All 5 can eat concurrently, with zero synchronization overhead.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NR_PHILOSOPHERS 5
#define EAT_ITERATIONS 500000

/* Classic: shared forks */
static pthread_mutex_t shared_forks[NR_PHILOSOPHERS];

static double classic_time;
static double privatized_time;

static long long get_usec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000LL;
}

static void *classic_philosopher(void *arg)
{
	long id = (long)arg;
	long first, second;
	unsigned long i;

	/* Asymmetric ordering to avoid deadlock */
	if (id == NR_PHILOSOPHERS - 1) {
		first = (id + 1) % NR_PHILOSOPHERS;
		second = id;
	} else {
		first = id;
		second = (id + 1) % NR_PHILOSOPHERS;
	}

	for (i = 0; i < EAT_ITERATIONS; i++) {
		pthread_mutex_lock(&shared_forks[first]);
		pthread_mutex_lock(&shared_forks[second]);
		/* eat */
		pthread_mutex_unlock(&shared_forks[second]);
		pthread_mutex_unlock(&shared_forks[first]);
	}
	return NULL;
}

static void *privatized_philosopher(void *arg)
{
	unsigned long i;
	(void)arg;

	for (i = 0; i < EAT_ITERATIONS; i++) {
		/* eat with private forks: no locking needed */
	}
	return NULL;
}

int main(void)
{
	pthread_t philosophers[NR_PHILOSOPHERS];
	long t;
	int rc;
	void *res;
	long long start, end;

	/* Classic solution */
	for (t = 0; t < NR_PHILOSOPHERS; t++)
		pthread_mutex_init(&shared_forks[t], NULL);

	start = get_usec();
	for (t = 0; t < NR_PHILOSOPHERS; t++) {
		rc = pthread_create(&philosophers[t], NULL, classic_philosopher, (void *)t);
		if (rc != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
	for (t = 0; t < NR_PHILOSOPHERS; t++)
		pthread_join(philosophers[t], &res);
	end = get_usec();
	classic_time = (double)(end - start) / 1000.0;

	for (t = 0; t < NR_PHILOSOPHERS; t++)
		pthread_mutex_destroy(&shared_forks[t]);

	/* Privatized solution */
	start = get_usec();
	for (t = 0; t < NR_PHILOSOPHERS; t++) {
		rc = pthread_create(&philosophers[t], NULL, privatized_philosopher, (void *)t);
		if (rc != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
	for (t = 0; t < NR_PHILOSOPHERS; t++)
		pthread_join(philosophers[t], &res);
	end = get_usec();
	privatized_time = (double)(end - start) / 1000.0;

	printf("classic:     %.3f ms (with %d mutexes)\n", classic_time, NR_PHILOSOPHERS);
	printf("privatized:  %.3f ms (zero synchronization)\n", privatized_time);
	printf("speedup:     %.2fx\n", classic_time / privatized_time);
	return 0;
}
