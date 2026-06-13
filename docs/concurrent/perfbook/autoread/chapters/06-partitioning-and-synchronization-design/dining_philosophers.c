/*
 * dining_philosophers.c: Partitioned solution to Dining Philosophers Problem.
 *
 * Classic problem: 5 philosophers, 5 forks, each needs two forks to eat.
 * Textbook solution (numbering forks) allows only 1 philosopher at a time
 * in worst case. Partitioned solution pairs forks so that at least 2 can
 * eat concurrently (with 4 philosophers) or all 5 (fully partitioned).
 *
 * This demo uses the fully partitioned approach: provide 5 independent
 * pairs of forks (one pair per philosopher). Zero contention.
 *
 * Build: make dining_philosophers.out
 * Run:   ./dining_philosophers.out
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define N_PHILOSOPHERS 5
#define EAT_ROUNDS 100000

/* Each philosopher owns a pair of forks (fully partitioned) */
struct fork_pair {
	pthread_mutex_t lock;
};

static struct fork_pair fork_pairs[N_PHILOSOPHERS];

static void init_forks(void)
{
	for (int i = 0; i < N_PHILOSOPHERS; i++)
		pthread_mutex_init(&fork_pairs[i].lock, NULL);
}

static unsigned long eat_count[N_PHILOSOPHERS];
static volatile int start_flag = 0;

static void *philosopher(void *arg)
{
	int id = (intptr_t)arg;
	struct fork_pair *my_forks = &fork_pairs[id];

	while (!start_flag)
		;

	for (int i = 0; i < EAT_ROUNDS; i++) {
		/* Pick up both forks from my own pair */
		pthread_mutex_lock(&my_forks->lock);
		/* Eat */
		eat_count[id]++;
		/* Put down both forks */
		pthread_mutex_unlock(&my_forks->lock);
	}
	return NULL;
}

/* Textbook solution for comparison: number forks, pick lower first */
static pthread_mutex_t textbook_forks[N_PHILOSOPHERS];

static void *philosopher_textbook(void *arg)
{
	int id = (intptr_t)arg;
	int first = id;
	int second = (id + 1) % N_PHILOSOPHERS;
	/* Numbering: pick lower-numbered fork first */
	if (first > second) {
		int tmp = first; first = second; second = tmp;
	}

	while (!start_flag)
		;

	for (int i = 0; i < EAT_ROUNDS; i++) {
		pthread_mutex_lock(&textbook_forks[first]);
		pthread_mutex_lock(&textbook_forks[second]);
		eat_count[id]++;
		pthread_mutex_unlock(&textbook_forks[second]);
		pthread_mutex_unlock(&textbook_forks[first]);
	}
	return NULL;
}

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static double run_test(const char *name,
		       void *(*routine)(void *),
		       int use_fork_pairs)
{
	pthread_t tids[N_PHILOSOPHERS];
	memset(eat_count, 0, sizeof(eat_count));
	start_flag = 0;

	if (use_fork_pairs) {
		init_forks();
	} else {
		for (int i = 0; i < N_PHILOSOPHERS; i++)
			pthread_mutex_init(&textbook_forks[i], NULL);
	}

	for (int i = 0; i < N_PHILOSOPHERS; i++)
		pthread_create(&tids[i], NULL, routine, (void *)(intptr_t)i);

	double t0 = now_sec();
	start_flag = 1;
	for (int i = 0; i < N_PHILOSOPHERS; i++)
		pthread_join(tids[i], NULL);
	double t1 = now_sec();

	unsigned long total = 0;
	for (int i = 0; i < N_PHILOSOPHERS; i++)
		total += eat_count[i];

	printf("%-25s time=%.4fs total_eats=%lu throughput=%.2f eats/sec\n",
	       name, t1 - t0, total, total / (t1 - t0));
	return t1 - t0;
}

int main(void)
{
	printf("=== Dining Philosophers Benchmark ===\n");
	printf("philosophers=%d eat_rounds=%d\n\n",
	       N_PHILOSOPHERS, EAT_ROUNDS);

	printf("--- Textbook (numbered forks) ---\n");
	run_test("textbook", philosopher_textbook, 0);

	printf("--- Fully Partitioned (one pair per philosopher) ---\n");
	run_test("partitioned", philosopher, 1);

	return 0;
}
