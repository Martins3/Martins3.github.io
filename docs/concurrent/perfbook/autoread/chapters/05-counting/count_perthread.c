/*
 * count_perthread.c: 基于 per-thread 变量的统计计数器
 *
 * 对应 perfbook 第 5 章 "Statistical Counters"
 * 每个线程更新自己的计数器，读取时汇总所有线程的值。
 * 这是 Data Ownership 模式的典型示例。
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define MAX_THREADS 64
#define NR_INCREMENTS 10000000

/* per-thread counter using _Thread_local */
static _Thread_local unsigned long thread_counter = 0;
static unsigned long *counterp[MAX_THREADS];
static unsigned long finalcount = 0;
static pthread_mutex_t final_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void inc_count(void)
{
	/* fastpath: only touch per-thread variable */
	thread_counter++;
}

static unsigned long read_count(void)
{
	int t;
	unsigned long sum;

	pthread_mutex_lock(&final_mutex);
	sum = finalcount;
	for (t = 0; t < MAX_THREADS; t++) {
		if (counterp[t] != NULL)
			sum += __atomic_load_n(counterp[t], __ATOMIC_RELAXED);
	}
	pthread_mutex_unlock(&final_mutex);
	return sum;
}

static void count_register_thread(int idx)
{
	pthread_mutex_lock(&final_mutex);
	counterp[idx] = &thread_counter;
	pthread_mutex_unlock(&final_mutex);
}

static void count_unregister_thread(int idx)
{
	pthread_mutex_lock(&final_mutex);
	finalcount += thread_counter;
	counterp[idx] = NULL;
	pthread_mutex_unlock(&final_mutex);
}

static int thread_idx[MAX_THREADS];

static void *thread_func(void *arg)
{
	int idx = *(int *)arg;
	long i;

	count_register_thread(idx);
	for (i = 0; i < NR_INCREMENTS; i++)
		inc_count();
	count_unregister_thread(idx);
	return NULL;
}

static double gettime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec / 1e6;
}

int main(int argc, char **argv)
{
	int nr_threads = 4;
	pthread_t threads[MAX_THREADS];
	double t0, t1;
	int i;

	if (argc > 1)
		nr_threads = atoi(argv[1]);
	if (nr_threads > MAX_THREADS)
		nr_threads = MAX_THREADS;

	memset(counterp, 0, sizeof(counterp));

	printf("Test: per-thread statistical counter, %d threads, %d increments each\n",
		nr_threads, NR_INCREMENTS);

	t0 = gettime();
	for (i = 0; i < nr_threads; i++) {
		thread_idx[i] = i;
		if (pthread_create(&threads[i], NULL, thread_func, &thread_idx[i]) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < nr_threads; i++)
		pthread_join(threads[i], NULL);
	t1 = gettime();

	unsigned long expected = (unsigned long)nr_threads * NR_INCREMENTS;
	unsigned long actual = read_count();
	printf("Expected: %lu, Actual: %lu, Time: %.3fs\n",
		expected, actual, t1 - t0);

	return 0;
}
