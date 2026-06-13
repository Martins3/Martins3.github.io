/*
 * count_end_rcu.c - RCU and per-thread statistical counters.
 * Corresponds to perfbook Section 13.5.1.
 *
 * Problem: per-thread __thread counters provide fast increments,
 * but summing them requires locking because threads may exit.
 * Solution: use RCU to protect the array of per-thread pointers.
 * When a thread exits, allocate a new countarray, move its count
 * to ->total, and publish the new array.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "urcu_simple.h"

#define MAX_THREADS 16

struct countarray {
	unsigned long total;
	unsigned long *counterp[MAX_THREADS];
};

static struct countarray *countarrayp = NULL;
static pthread_mutex_t final_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Per-thread counters (simulated with an array indexed by thread id) */
static __thread unsigned long __counter = 0;
static unsigned long *thread_counters[MAX_THREADS];
static int thread_ids[MAX_THREADS];
static int next_thread_id = 0;

static int get_thread_index(void)
{
	static pthread_mutex_t id_lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_lock(&id_lock);
	int id = next_thread_id++;
	thread_ids[id] = id;
	pthread_mutex_unlock(&id_lock);
	return id;
}

static inline void inc_count(void)
{
	__counter++;
}

static unsigned long read_count(void)
{
	struct countarray *cap;
	unsigned long sum;

	rcu_read_lock();
	cap = rcu_dereference(countarrayp);
	sum = cap->total;
	for (int t = 0; t < MAX_THREADS; t++) {
		unsigned long *ctrp = READ_ONCE(cap->counterp[t]);
		if (ctrp != NULL)
			sum += READ_ONCE(*ctrp);
	}
	rcu_read_unlock();
	return sum;
}

static void count_init(void)
{
	countarrayp = calloc(1, sizeof(*countarrayp));
}

static void count_register_thread(unsigned long *p, int idx)
{
	pthread_mutex_lock(&final_mutex);
	countarrayp->counterp[idx] = p;
	pthread_mutex_unlock(&final_mutex);
}

static void count_unregister_thread(int idx)
{
	struct countarray *cap = malloc(sizeof(*cap));
	if (!cap) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	pthread_mutex_lock(&final_mutex);
	*cap = *countarrayp;
	cap->total += __counter;
	cap->counterp[idx] = NULL;
	struct countarray *capold = countarrayp;
	rcu_assign_pointer(countarrayp, cap);
	pthread_mutex_unlock(&final_mutex);

	synchronize_rcu();
	free(capold);
}

/* --- test --- */
static int thread_idx;

static void *worker_thread(void *arg)
{
	(void)arg;
	int idx = get_thread_index();
	thread_idx = idx;
	thread_counters[idx] = &__counter;
	count_register_thread(&__counter, idx);

	for (int i = 0; i < 100000; i++)
		inc_count();

	count_unregister_thread(idx);
	return NULL;
}

int main(void)
{
	printf("[count_end_rcu] RCU-protected per-thread statistical counters\n");
	count_init();

	pthread_t t1, t2;
	pthread_create(&t1, NULL, worker_thread, NULL);
	pthread_create(&t2, NULL, worker_thread, NULL);

	/* Let threads run a bit, then read */
	for (int i = 0; i < 10; i++) {
		unsigned long sum = read_count();
		printf("  read_count() = %lu\n", sum);
	}

	pthread_join(t1, NULL);
	pthread_join(t2, NULL);

	unsigned long final = read_count();
	printf("  final read_count() = %lu\n", final);

	if (final == 200000)
		printf("[OK] Exact count achieved despite thread exits\n");
	else
		printf("[FAIL] Expected 200000, got %lu\n", final);

	free(countarrayp);
	return (final == 200000) ? 0 : 1;
}
