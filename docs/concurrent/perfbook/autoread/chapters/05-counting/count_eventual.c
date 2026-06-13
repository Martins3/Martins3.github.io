/*
 * count_eventual.c: 最终一致性计数器
 *
 * 对应 perfbook 第 5 章 "Eventually Consistent Implementation"
 * 使用单独的线程定期将 per-thread 计数汇总到全局计数器。
 * 读操作直接返回全局值，写操作只更新 per-thread 值。
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

static _Thread_local unsigned long thread_counter = 0;
static unsigned long *counterp[MAX_THREADS];
static unsigned long global_count = 0;
static int stopflag = 0;

static inline void inc_count(void)
{
	thread_counter++;
}

static unsigned long read_count(void)
{
	return __atomic_load_n(&global_count, __ATOMIC_RELAXED);
}

static void *eventual_thread(void *arg)
{
	int t;
	unsigned long sum;
	(void)arg;

	while (__atomic_load_n(&stopflag, __ATOMIC_ACQUIRE) < 3) {
		sum = 0;
		for (t = 0; t < MAX_THREADS; t++) {
			if (counterp[t] != NULL)
				sum += __atomic_load_n(counterp[t], __ATOMIC_RELAXED);
		}
		__atomic_store_n(&global_count, sum, __ATOMIC_RELEASE);
		usleep(1000); /* 1ms */
		if (__atomic_load_n(&stopflag, __ATOMIC_RELAXED)) {
			int old = __atomic_load_n(&stopflag, __ATOMIC_RELAXED);
			__atomic_store_n(&stopflag, old + 1, __ATOMIC_RELEASE);
		}
	}
	return NULL;
}

static void count_register_thread(int idx)
{
	counterp[idx] = &thread_counter;
}

static int thread_idx[MAX_THREADS];

static void *worker_thread(void *arg)
{
	int idx = *(int *)arg;
	long i;
	unsigned long mycount;

	count_register_thread(idx);
	for (i = 0; i < NR_INCREMENTS; i++)
		inc_count();
	/* before exit, save our count to global and unregister */
	mycount = thread_counter;
	counterp[idx] = NULL;
	__atomic_add_fetch(&global_count, mycount, __ATOMIC_RELAXED);
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
	pthread_t ev_thread;
	double t0, t1;
	int i;

	if (argc > 1)
		nr_threads = atoi(argv[1]);
	if (nr_threads > MAX_THREADS)
		nr_threads = MAX_THREADS;

	memset(counterp, 0, sizeof(counterp));

	printf("Test: eventually consistent counter, %d threads, %d increments each\n",
		nr_threads, NR_INCREMENTS);

	if (pthread_create(&ev_thread, NULL, eventual_thread, NULL) != 0) {
		perror("pthread_create eventual");
		exit(EXIT_FAILURE);
	}

	t0 = gettime();
	for (i = 0; i < nr_threads; i++) {
		thread_idx[i] = i;
		if (pthread_create(&threads[i], NULL, worker_thread, &thread_idx[i]) != 0) {
			perror("pthread_create worker");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < nr_threads; i++)
		pthread_join(threads[i], NULL);
	t1 = gettime();

	/* signal eventual thread to stop and wait for it to finish */
	__atomic_store_n(&stopflag, 3, __ATOMIC_RELEASE);
	pthread_join(ev_thread, NULL);

	unsigned long expected = (unsigned long)nr_threads * NR_INCREMENTS;
	unsigned long actual = read_count();
	printf("Expected: %lu, Actual(eventual): %lu, Time: %.3fs\n",
		expected, actual, t1 - t0);

	return 0;
}
