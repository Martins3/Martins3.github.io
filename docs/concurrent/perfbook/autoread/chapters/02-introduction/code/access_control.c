/*
 * access_control.c
 * 演示并行访问控制(Parallel Access Control)的不同策略
 *
 * 对应 perfbook 第 2 章 sec:Parallel Access Control
 * 核心观点：不同协调机制（锁、原子变量、数据所有权）的权衡
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <stdatomic.h>
#include <string.h>

#define N_THREADS 4
#define INCREMENTS 10000000

static uint64_t timespec_diff_ns(struct timespec *a, struct timespec *b)
{
	return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ULL +
	       (uint64_t)(b->tv_nsec - a->tv_nsec);
}

/* 1. 无保护（数据竞争，仅用于演示，实际不应使用） */
static uint64_t counter_no_lock = 0;

static void *thread_no_lock(void *arg)
{
	(void)arg;
	for (int i = 0; i < INCREMENTS; i++)
		counter_no_lock++;
	return NULL;
}

/* 2. 互斥锁 (pthread_mutex) */
static uint64_t counter_mutex = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *thread_mutex(void *arg)
{
	(void)arg;
	for (int i = 0; i < INCREMENTS; i++) {
		pthread_mutex_lock(&mutex);
		counter_mutex++;
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

/* 3. 原子变量 (C11 _Atomic) */
static _Atomic uint64_t counter_atomic = 0;

static void *thread_atomic(void *arg)
{
	(void)arg;
	for (int i = 0; i < INCREMENTS; i++)
		atomic_fetch_add_explicit(&counter_atomic, 1, memory_order_relaxed);
	return NULL;
}

/* 4. 数据所有权（每个线程修改自己的计数器，最后汇总） */
static _Alignas(64) uint64_t counter_owned[N_THREADS];

static void *thread_owned(void *arg)
{
	int idx = *(int *)arg;
	for (int i = 0; i < INCREMENTS; i++)
		counter_owned[idx]++;
	return NULL;
}

static double run_test_args(void *(*thread_fn)(void *), void **args)
{
	pthread_t threads[N_THREADS];
	struct timespec t0, t1;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N_THREADS; i++)
		pthread_create(&threads[i], NULL, thread_fn, args[i]);
	for (int i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	return (double)timespec_diff_ns(&t0, &t1) / 1000000.0; /* ms */
}

static double run_test(void *(*thread_fn)(void *))
{
	pthread_t threads[N_THREADS];
	struct timespec t0, t1;
	int default_args[N_THREADS];

	for (int i = 0; i < N_THREADS; i++)
		default_args[i] = i;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N_THREADS; i++)
		pthread_create(&threads[i], NULL, thread_fn, &default_args[i]);
	for (int i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	return (double)timespec_diff_ns(&t0, &t1) / 1000000.0; /* ms */
}

int main(void)
{
	double t;
	uint64_t total;

	printf("Parallel Access Control Demo (threads=%d, increments=%d)\n",
	       N_THREADS, INCREMENTS);
	printf("-----------------------------------------------------------\n");

	/* 无保护 - 错误结果 */
	counter_no_lock = 0;
	t = run_test(thread_no_lock);
	printf("No protection:      %.3f ms, result=%lu (WRONG!)\n",
	       t, counter_no_lock);

	/* 互斥锁 - 正确但慢 */
	counter_mutex = 0;
	t = run_test(thread_mutex);
	printf("Mutex lock:         %.3f ms, result=%lu\n",
	       t, counter_mutex);

	/* 原子变量 - 正确且较快 */
	counter_atomic = 0;
	t = run_test(thread_atomic);
	printf("Atomic fetch_add:   %.3f ms, result=%lu\n",
	       t, counter_atomic);

	/* 数据所有权 - 最快 */
	memset(counter_owned, 0, sizeof(counter_owned));
	t = run_test(thread_owned);
	total = 0;
	for (int i = 0; i < N_THREADS; i++)
		total += counter_owned[i];
	printf("Data ownership:     %.3f ms, result=%lu\n",
	       t, total);

	printf("\nNote: Data ownership eliminates inter-thread communication,\n");
	printf("      achieving the best performance.\n");

	return 0;
}
