/*
 * livelock_demo.c: 演示活锁、饥饿和指数退避
 *
 * 对应 perfbook 第7章:
 *  - 7.1.2 Livelock and Starvation
 *  - 7.1.3 Unfairness
 *  - 7.1.4 Inefficiency
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdatomic.h>
#include <string.h>

/* ========== 1. 活锁演示 (对称条件锁) ========== */

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
atomic_int livelock_count = 0;
volatile int stop_livelock = 0;

void *thread_livelock_1(void *arg)
{
	(void)arg;
	while (!stop_livelock) {
		pthread_mutex_lock(&lock1);
		if (pthread_mutex_trylock(&lock2) == 0) {
			/* 成功获取两把锁 */
			pthread_mutex_unlock(&lock2);
			pthread_mutex_unlock(&lock1);
			break;
		}
		/* 失败：释放 lock1 并重试 */
		pthread_mutex_unlock(&lock1);
		atomic_fetch_add(&livelock_count, 1);
		usleep(1000); /* 微小延迟 */
	}
	return NULL;
}

void *thread_livelock_2(void *arg)
{
	(void)arg;
	while (!stop_livelock) {
		pthread_mutex_lock(&lock2);
		if (pthread_mutex_trylock(&lock1) == 0) {
			pthread_mutex_unlock(&lock1);
			pthread_mutex_unlock(&lock2);
			break;
		}
		pthread_mutex_unlock(&lock2);
		atomic_fetch_add(&livelock_count, 1);
		usleep(1000);
	}
	return NULL;
}

/* ========== 2. 指数退避解决活锁 ========== */

atomic_int backoff_count = 0;
volatile int stop_backoff = 0;

void *thread_backoff_1(void *arg)
{
	(void)arg;
	unsigned int wait = 1;
	while (!stop_backoff) {
		pthread_mutex_lock(&lock1);
		if (pthread_mutex_trylock(&lock2) == 0) {
			pthread_mutex_unlock(&lock2);
			pthread_mutex_unlock(&lock1);
			break;
		}
		pthread_mutex_unlock(&lock1);
		atomic_fetch_add(&backoff_count, 1);
		/* 指数退避: 等待时间翻倍 */
		usleep(wait * 1000);
		if (wait < 128)
			wait <<= 1;
	}
	return NULL;
}

void *thread_backoff_2(void *arg)
{
	(void)arg;
	unsigned int wait = 1;
	while (!stop_backoff) {
		pthread_mutex_lock(&lock2);
		if (pthread_mutex_trylock(&lock1) == 0) {
			pthread_mutex_unlock(&lock1);
			pthread_mutex_unlock(&lock2);
			break;
		}
		pthread_mutex_unlock(&lock2);
		atomic_fetch_add(&backoff_count, 1);
		usleep(wait * 1000);
		if (wait < 128)
			wait <<= 1;
	}
	return NULL;
}

/* ========== 3. 公平性演示: 一个线程饥饿 ========== */

pthread_mutex_t fair_lock = PTHREAD_MUTEX_INITIALIZER;
atomic_int hungry_counter[3] = {0, 0, 0};
volatile int stop_fairness = 0;

void *hungry_thread(void *arg)
{
	int id = *(int *)arg;
	while (!stop_fairness) {
		pthread_mutex_lock(&fair_lock);
		atomic_fetch_add(&hungry_counter[id], 1);
		pthread_mutex_unlock(&fair_lock);
		/* 线程0不休眠，线程1和2稍微休眠，观察争抢 */
		if (id != 0)
			usleep(100);
	}
	return NULL;
}

/* ========== 4. 粗粒度 vs 细粒度锁效率 ========== */

#define ARRAY_SIZE 1000000
#define NTHREADS 4

int global_array[ARRAY_SIZE];
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t per_elem_lock[ARRAY_SIZE]; /* 实际中不会这样，仅演示 */

/* 粗粒度: 一个锁保护整个数组 */
void *coarse_grain_work(void *arg)
{
	int id = *(int *)arg;
	int start = id * (ARRAY_SIZE / NTHREADS);
	int end = start + (ARRAY_SIZE / NTHREADS);
	unsigned long sum = 0;

	pthread_mutex_lock(&global_lock);
	for (int i = start; i < end; i++) {
		sum += global_array[i];
	}
	pthread_mutex_unlock(&global_lock);
	return (void *)sum;
}

/* 无锁: 每个线程访问独立分区 */
void *no_lock_work(void *arg)
{
	int id = *(int *)arg;
	int start = id * (ARRAY_SIZE / NTHREADS);
	int end = start + (ARRAY_SIZE / NTHREADS);
	unsigned long sum = 0;

	for (int i = start; i < end; i++) {
		sum += global_array[i];
	}
	return (void *)sum;
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
	pthread_t t1, t2;
	double start, end;

	printf("=== Livelock, Starvation, and Inefficiency Demo ===\n\n");

	/* 1. 活锁演示 */
	printf("--- 1. Livelock (symmetric conditional locking) ---\n");
	livelock_count = 0;
	stop_livelock = 0;
	pthread_create(&t1, NULL, thread_livelock_1, NULL);
	pthread_create(&t2, NULL, thread_livelock_2, NULL);
	usleep(500000); /* 观察0.5秒 */
	stop_livelock = 1;
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	printf("[Livelock] Retry count in 0.5s: %d (high = livelock)\n\n",
	       atomic_load(&livelock_count));

	/* 2. 指数退避解决活锁 */
	printf("--- 2. Exponential Backoff ---\n");
	backoff_count = 0;
	stop_backoff = 0;
	pthread_create(&t1, NULL, thread_backoff_1, NULL);
	pthread_create(&t2, NULL, thread_backoff_2, NULL);
	usleep(500000);
	stop_backoff = 1;
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	printf("[Backoff] Retry count in 0.5s: %d (should be much lower)\n\n",
	       atomic_load(&backoff_count));

	/* 3. 公平性/饥饿演示 */
	printf("--- 3. Unfairness / Starvation ---\n");
	stop_fairness = 0;
	memset((void *)hungry_counter, 0, sizeof(hungry_counter));
	pthread_t threads[3];
	int ids[3] = {0, 1, 2};
	for (int i = 0; i < 3; i++)
		pthread_create(&threads[i], NULL, hungry_thread, &ids[i]);
	usleep(500000);
	stop_fairness = 1;
	for (int i = 0; i < 3; i++)
		pthread_join(threads[i], NULL);
	printf("[Fairness] Thread acquire counts: [%d, %d, %d]\n",
	       atomic_load(&hungry_counter[0]),
	       atomic_load(&hungry_counter[1]),
	       atomic_load(&hungry_counter[2]));
	printf("           Thread 0 (no sleep) may dominate.\n\n");

	/* 4. 锁粒度效率对比 */
	printf("--- 4. Lock Granularity Efficiency ---\n");
	for (int i = 0; i < ARRAY_SIZE; i++)
		global_array[i] = i % 100;

	pthread_t work_threads[NTHREADS];
	int work_ids[NTHREADS];

	/* 粗粒度锁 */
	start = get_time_ms();
	for (int i = 0; i < NTHREADS; i++) {
		work_ids[i] = i;
		pthread_create(&work_threads[i], NULL, coarse_grain_work, &work_ids[i]);
	}
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(work_threads[i], NULL);
	end = get_time_ms();
	printf("[Efficiency] Coarse-grain lock: %.3f ms\n", end - start);

	/* 无锁分区 */
	start = get_time_ms();
	for (int i = 0; i < NTHREADS; i++)
		pthread_create(&work_threads[i], NULL, no_lock_work, &work_ids[i]);
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(work_threads[i], NULL);
	end = get_time_ms();
	printf("[Efficiency] No lock (partitioned): %.3f ms\n", end - start);
	printf("             Note: coarse-grain serializes all threads!\n\n");

	return 0;
}
