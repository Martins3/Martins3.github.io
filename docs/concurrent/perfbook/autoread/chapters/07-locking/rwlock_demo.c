/*
 * rwlock_demo.c: 读者-写者锁的简单实现
 *
 * 对应 perfbook 第7章:
 *  - 7.2.2 Reader-Writer Locks
 *
 * 使用 per-thread 独占锁的简化方案:
 *   - 读: 只获取自己的锁
 *   - 写: 获取所有锁
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>

#define NREADERS 4
#define NWRITERS 1

/* ========== 简单的读者-写者锁实现 ========== */

/* 使用 pthread_rwlock_t 演示概念，然后展示简单的 per-thread 锁方案 */
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
int shared_data = 0;

/* per-thread 锁方案 (Wilson C. Hsieh 方案简化版) */
#define MAX_THREADS 8
pthread_mutex_t reader_locks[MAX_THREADS];
atomic_int reader_count = 0;

void init_per_thread_locks(void)
{
	for (int i = 0; i < MAX_THREADS; i++)
		pthread_mutex_init(&reader_locks[i], NULL);
}

/* 简化版：仅演示思想，不处理动态线程 */
void per_thread_read_lock(int tid)
{
	pthread_mutex_lock(&reader_locks[tid]);
}

void per_thread_read_unlock(int tid)
{
	pthread_mutex_unlock(&reader_locks[tid]);
}

void per_thread_write_lock(void)
{
	for (int i = 0; i < MAX_THREADS; i++)
		pthread_mutex_lock(&reader_locks[i]);
}

void per_thread_write_unlock(void)
{
	for (int i = 0; i < MAX_THREADS; i++)
		pthread_mutex_unlock(&reader_locks[i]);
}

/* ========== 工作线程 ========== */

volatile int stop = 0;

void *reader_pthread(void *arg)
{
	int id = *(int *)arg;
	int local_reads = 0;
	(void)id;

	while (!stop) {
		pthread_rwlock_rdlock(&rwlock);
		/* 读操作 */
		int val = shared_data;
		(void)val;
		local_reads++;
		pthread_rwlock_unlock(&rwlock);
		usleep(1000); /* 模拟读间隙 */
	}
	return (void *)(long)local_reads;
}

void *writer_pthread(void *arg)
{
	(void)arg;
	int local_writes = 0;

	while (!stop) {
		pthread_rwlock_wrlock(&rwlock);
		shared_data++;
		local_writes++;
		pthread_rwlock_unlock(&rwlock);
		usleep(5000); /* 写频率较低 */
	}
	return (void *)(long)local_writes;
}

void *reader_per_thread(void *arg)
{
	int id = *(int *)arg;
	int local_reads = 0;

	while (!stop) {
		per_thread_read_lock(id);
		int val = shared_data;
		(void)val;
		local_reads++;
		per_thread_read_unlock(id);
		usleep(1000);
	}
	return (void *)(long)local_reads;
}

void *writer_per_thread(void *arg)
{
	(void)arg;
	int local_writes = 0;

	while (!stop) {
		per_thread_write_lock();
		shared_data++;
		local_writes++;
		per_thread_write_unlock();
		usleep(5000);
	}
	return (void *)(long)local_writes;
}

/* ========== 主函数 ========== */

static double get_time_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void)
{
	pthread_t readers[NREADERS];
	pthread_t writers[NWRITERS];
	int ids[NREADERS];
	void *ret;
	double start, end;
	unsigned long total_reads = 0;
	unsigned long total_writes = 0;

	printf("=== Reader-Writer Lock Demo ===\n\n");

	/* 1. 使用 pthread_rwlock_t */
	printf("--- 1. pthread_rwlock_t ---\n");
	shared_data = 0;
	stop = 0;
	for (int i = 0; i < NREADERS; i++) {
		ids[i] = i;
		pthread_create(&readers[i], NULL, reader_pthread, &ids[i]);
	}
	for (int i = 0; i < NWRITERS; i++)
		pthread_create(&writers[i], NULL, writer_pthread, NULL);

	usleep(500000); /* 运行 0.5 秒 */
	stop = 1;

	for (int i = 0; i < NREADERS; i++) {
		pthread_join(readers[i], &ret);
		total_reads += (unsigned long)ret;
	}
	for (int i = 0; i < NWRITERS; i++) {
		pthread_join(writers[i], &ret);
		total_writes += (unsigned long)ret;
	}
	printf("[pthread rwlock] Reads: %lu, Writes: %lu, Final data: %d\n\n",
	       total_reads, total_writes, shared_data);

	/* 2. 使用 per-thread 锁方案 */
	printf("--- 2. Per-thread exclusive locks (simplified) ---\n");
	init_per_thread_locks();
	shared_data = 0;
	stop = 0;
	total_reads = 0;
	total_writes = 0;
	for (int i = 0; i < NREADERS; i++)
		pthread_create(&readers[i], NULL, reader_per_thread, &ids[i]);
	for (int i = 0; i < NWRITERS; i++)
		pthread_create(&writers[i], NULL, writer_per_thread, NULL);

	usleep(500000);
	stop = 1;

	for (int i = 0; i < NREADERS; i++) {
		pthread_join(readers[i], &ret);
		total_reads += (unsigned long)ret;
	}
	for (int i = 0; i < NWRITERS; i++) {
		pthread_join(writers[i], &ret);
		total_writes += (unsigned long)ret;
	}
	printf("[per-thread]     Reads: %lu, Writes: %lu, Final data: %d\n\n",
	       total_reads, total_writes, shared_data);

	printf("Notes:\n");
	printf("  - pthread_rwlock_t: single lock, writer may starve readers or vice versa\n");
	printf("  - per-thread locks: readers scale well (no cache misses),\n");
	printf("    but writer must acquire all locks (expensive)\n");
	printf("  - Linux kernel has qrwlock (queued rwlock) for better fairness\n");

	return 0;
}
