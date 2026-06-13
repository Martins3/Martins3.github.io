/*
 * count_limit.c: 简单的近似限制计数器
 *
 * 对应 perfbook 第 5 章 "Approximate Limit Counters"
 * 使用 per-thread counter + countermax 来减少全局锁竞争。
 * 这是 Parallel Fastpath 设计模式的示例。
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_THREADS 64
#define NR_OPS 1000000

static unsigned long globalcountmax = 100000;
static unsigned long globalcount = 0;
static unsigned long globalreserve = 0;

static _Thread_local unsigned long thread_counter = 0;
static _Thread_local unsigned long thread_countermax = 0;
static unsigned long *counterp[MAX_THREADS];

static pthread_mutex_t gblcnt_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void globalize_count(void)
{
	globalcount += thread_counter;
	thread_counter = 0;
	globalreserve -= thread_countermax;
	thread_countermax = 0;
}

static inline void balance_count(void)
{
	unsigned long delta;

	delta = globalcountmax - globalcount - globalreserve;
	delta /= MAX_THREADS; /* simplified */
	if (delta == 0)
		delta = 1;
	thread_countermax = delta;
	globalreserve += thread_countermax;
	thread_counter = thread_countermax / 2;
	if (thread_counter > globalcount)
		thread_counter = globalcount;
	globalcount -= thread_counter;
}

static int add_count(unsigned long delta)
{
	if (thread_countermax - thread_counter >= delta) {
		thread_counter += delta;
		return 1;
	}

	pthread_mutex_lock(&gblcnt_mutex);
	globalize_count();
	if (globalcountmax - globalcount - globalreserve < delta) {
		pthread_mutex_unlock(&gblcnt_mutex);
		return 0;
	}
	globalcount += delta;
	balance_count();
	pthread_mutex_unlock(&gblcnt_mutex);
	return 1;
}

static int sub_count(unsigned long delta)
{
	if (thread_counter >= delta) {
		thread_counter -= delta;
		return 1;
	}

	pthread_mutex_lock(&gblcnt_mutex);
	globalize_count();
	if (globalcount < delta) {
		pthread_mutex_unlock(&gblcnt_mutex);
		return 0;
	}
	globalcount -= delta;
	balance_count();
	pthread_mutex_unlock(&gblcnt_mutex);
	return 1;
}

static unsigned long read_count(void)
{
	int t;
	unsigned long sum;

	pthread_mutex_lock(&gblcnt_mutex);
	sum = globalcount;
	for (t = 0; t < MAX_THREADS; t++) {
		if (counterp[t] != NULL)
			sum += *counterp[t];
	}
	pthread_mutex_unlock(&gblcnt_mutex);
	return sum;
}

static void count_register_thread(int idx)
{
	pthread_mutex_lock(&gblcnt_mutex);
	counterp[idx] = &thread_counter;
	pthread_mutex_unlock(&gblcnt_mutex);
}

static void count_unregister_thread(int idx)
{
	pthread_mutex_lock(&gblcnt_mutex);
	globalize_count();
	counterp[idx] = NULL;
	pthread_mutex_unlock(&gblcnt_mutex);
}

static int thread_idx[MAX_THREADS];

static void *worker_thread(void *arg)
{
	int idx = *(int *)arg;
	long i;
	int success = 0;
	int fail = 0;

	count_register_thread(idx);
	for (i = 0; i < NR_OPS; i++) {
		if (add_count(1))
			success++;
		else
			fail++;
		/* alternate add/sub to keep count near limit */
		if (i % 2 == 0) {
			if (sub_count(1))
				success++;
		}
	}
	count_unregister_thread(idx);
	return NULL;
}

int main(int argc, char **argv)
{
	int nr_threads = 4;
	pthread_t threads[MAX_THREADS];
	int i;

	if (argc > 1)
		nr_threads = atoi(argv[1]);
	if (nr_threads > MAX_THREADS)
		nr_threads = MAX_THREADS;

	memset(counterp, 0, sizeof(counterp));

	printf("Test: approximate limit counter, limit=%lu, threads=%d\n",
		globalcountmax, nr_threads);

	for (i = 0; i < nr_threads; i++) {
		thread_idx[i] = i;
		if (pthread_create(&threads[i], NULL, worker_thread, &thread_idx[i]) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < nr_threads; i++)
		pthread_join(threads[i], NULL);

	unsigned long total = read_count();
	printf("Final count: %lu (global=%lu)\n", total, globalcount);

	return 0;
}
