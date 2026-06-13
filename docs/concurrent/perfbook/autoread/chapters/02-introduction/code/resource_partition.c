/*
 * resource_partition.c
 * 演示资源划分与复制(Resource Partitioning and Replication)的效果
 *
 * 对应 perfbook 第 2 章 sec:Resource Partitioning and Replication
 * 核心观点：粗粒度划分减少同步开销；per-thread 数据避免缓存行竞争
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
#define OPERATIONS 5000000

static uint64_t timespec_diff_ns(struct timespec *a, struct timespec *b)
{
	return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ULL +
	       (uint64_t)(b->tv_nsec - a->tv_nsec);
}

/*
 * 场景 1: 单一全局计数器（所有线程竞争同一个缓存行）
 * 这会导致严重的缓存行弹跳 (cache-line bouncing)
 */
static _Alignas(64) _Atomic uint64_t global_counter = 0;

static void *thread_global_counter(void *arg)
{
	(void)arg;
	for (int i = 0; i < OPERATIONS; i++)
		atomic_fetch_add_explicit(&global_counter, 1, memory_order_relaxed);
	return NULL;
}

/*
 * 场景 2: 按锁划分（多个锁，每个锁保护一部分数据）
 * 减少锁竞争，但仍有同步开销
 */
#define N_PARTITIONS 4

struct partitioned_counter {
	pthread_mutex_t lock;
	uint64_t count;
	char pad[64 - sizeof(pthread_mutex_t) - sizeof(uint64_t)];
};

static _Alignas(64) struct partitioned_counter part_counters[N_PARTITIONS];

static void *thread_partitioned(void *arg)
{
	int tid = *(int *)arg;
	int part = tid % N_PARTITIONS;

	for (int i = 0; i < OPERATIONS; i++) {
		pthread_mutex_lock(&part_counters[part].lock);
		part_counters[part].count++;
		pthread_mutex_unlock(&part_counters[part].lock);
	}
	return NULL;
}

/*
 * 场景 3: per-thread 计数器（完全消除同步）
 * 每个线程修改自己独立的缓存行，最后汇总
 * 对应 Linux 内核中的 percpu_counter 思想
 */
static _Alignas(64) uint64_t per_thread_counters[N_THREADS];

static void *thread_per_thread(void *arg)
{
	int tid = *(int *)arg;
	for (int i = 0; i < OPERATIONS; i++)
		per_thread_counters[tid]++;
	return NULL;
}

/*
 * 场景 4: 读多写少场景的复制（每个线程读本地副本，写时广播）
 * 演示复制 read-mostly 数据的思想
 */
static _Alignas(64) uint64_t config_value = 42;
static _Alignas(64) uint64_t local_config[N_THREADS];

static void *thread_replicated_read(void *arg)
{
	int tid = *(int *)arg;
	uint64_t sum = 0;

	/* 每个线程读取自己的本地副本 */
	for (int i = 0; i < OPERATIONS; i++)
		sum += local_config[tid];
	(void)sum;
	return NULL;
}

static double run_test(void *(*fn)(void *), void **args)
{
	pthread_t threads[N_THREADS];
	struct timespec t0, t1;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N_THREADS; i++)
		pthread_create(&threads[i], NULL, fn, args[i]);
	for (int i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	return (double)timespec_diff_ns(&t0, &t1) / 1000000.0;
}

int main(void)
{
	double t;
	int tids[N_THREADS];
	void *args[N_THREADS];

	for (int i = 0; i < N_THREADS; i++) {
		tids[i] = i;
		args[i] = &tids[i];
	}

	printf("Resource Partitioning & Replication Demo\n");
	printf("========================================\n");
	printf("threads=%d, operations=%d per thread\n\n", N_THREADS, OPERATIONS);

	/* 初始化分区锁 */
	for (int i = 0; i < N_PARTITIONS; i++) {
		pthread_mutex_init(&part_counters[i].lock, NULL);
		part_counters[i].count = 0;
	}

	/* 初始化本地配置副本 */
	for (int i = 0; i < N_THREADS; i++)
		local_config[i] = config_value;

	/* 场景 1 */
	global_counter = 0;
	t = run_test(thread_global_counter, args);
	printf("Global atomic counter:   %.3f ms (result=%lu)\n",
	       t, atomic_load(&global_counter));

	/* 场景 2 */
	t = run_test(thread_partitioned, args);
	uint64_t total_part = 0;
	for (int i = 0; i < N_PARTITIONS; i++)
		total_part += part_counters[i].count;
	printf("Partitioned (4 locks):   %.3f ms (result=%lu)\n",
	       t, total_part);

	/* 场景 3 */
	memset(per_thread_counters, 0, sizeof(per_thread_counters));
	t = run_test(thread_per_thread, args);
	uint64_t total_perthread = 0;
	for (int i = 0; i < N_THREADS; i++)
		total_perthread += per_thread_counters[i];
	printf("Per-thread counters:     %.3f ms (result=%lu)\n",
	       t, total_perthread);

	/* 场景 4 */
	t = run_test(thread_replicated_read, args);
	printf("Replicated reads:        %.3f ms\n", t);

	printf("\nKey insight: per-thread data eliminates both lock contention\n");
	printf("and cache-line bouncing, yielding the best scalability.\n");

	for (int i = 0; i < N_PARTITIONS; i++)
		pthread_mutex_destroy(&part_counters[i].lock);

	return 0;
}
