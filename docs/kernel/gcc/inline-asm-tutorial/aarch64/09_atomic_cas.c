/*
 * 示例 09: 原子操作 - Compare And Swap (CAS) (ARM64/AArch64)
 *
 * 知识点:
 * - ARM64 使用 LDXR/STXR 实现原子操作
 * - LDXR: 加载独占 (Load Exclusive)
 * - STXR: 存储独占 (Store Exclusive), 成功返回 0
 * - WFE/SEV: 等待事件/发送事件 (用于自旋锁优化)
 * - 实际应用: 实现自旋锁、无锁数据结构
 */

#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

/* 原子比较交换 (Compare And Swap)
 * 如果 *ptr == expected，则 *ptr = desired，返回 true
 * 否则，*expected = *ptr，返回 false
 *
 * ARM64 使用 LDXR/STXR 循环实现
 */
static inline bool atomic_cas_int(volatile int *ptr, int *expected, int desired)
{
	int old_val;
	int success;

	__asm__ __volatile__(
		"1: ldxr %w0, [%3]\n\t"       /* 独占加载当前值 */
		"cmp %w0, %w4\n\t"            /* 比较 */
		"b.ne 2f\n\t"                 /* 不相等则跳转到失败 */
		"stxr %w1, %w5, [%3]\n\t"     /* 独占存储新值 */
		"cbnz %w1, 1b\n\t"            /* 失败则重试 */
		"mov %w1, #1\n\t"             /* 成功 */
		"b 3f\n\t"
		"2: mov %w1, #0\n\t"          /* 失败 */
		"3:"
		: "=&r" (old_val), "=&r" (success)
		: "r" (ptr), "r" (ptr), "r" (*expected), "r" (desired)
		: "memory", "cc"
		);

	if (!success)
		*expected = old_val;
	return success;
}

/* 简化的原子递增 (使用 CAS 循环) */
static inline int atomic_inc(volatile int *ptr)
{
	int old_val, new_val;

	do {
		old_val = *ptr;
		new_val = old_val + 1;
	} while (!atomic_cas_int(ptr, &old_val, new_val));

	return new_val;
}

/* 简单的自旋锁实现 */
typedef struct {
	volatile int lock;
} spinlock_t;

#define SPINLOCK_INITIALIZER {0}

static inline void spinlock_init(spinlock_t *sl)
{
	sl->lock = 0;
}

static inline void spinlock_lock(spinlock_t *sl)
{
	int expected = 0;

	while (!atomic_cas_int(&sl->lock, &expected, 1)) {
		expected = 0;
		/* ARM64 使用 wfe/isb 优化自旋 */
		__asm__ __volatile__("isb" ::: "memory");
	}

	/* 获取锁后加入 acquire 屏障，防止临界区内的内存访问被重排序到锁获取之前 */
	__asm__ __volatile__("dmb ish" ::: "memory");
}

static inline void spinlock_unlock(spinlock_t *sl)
{
	__asm__ __volatile__(
		"stlr %w0, [%1]"          /* STLR: Store-Release */
		:
		: "r" (0), "r" (&sl->lock)
		: "memory"
		);
}

/* 测试数据 */
static volatile int counter = 0;
static spinlock_t counter_lock = SPINLOCK_INITIALIZER;

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 100000

void *increment_worker(void *arg)
{
	(void)arg;

	for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
		spinlock_lock(&counter_lock);
		counter++;
		spinlock_unlock(&counter_lock);
	}

	return NULL;
}

int main(void)
{
	/* ===== 测试 1: 基本 CAS 操作 ===== */
	int value = 100;
	int expected = 100;
	bool success;

	success = atomic_cas_int(&value, &expected, 200);
	printf("CAS(value=100, expected=100, desired=200): %s\n",
	       success ? "success" : "failure");
	printf("value = %d (expected: 200)\n", value);
	assert(success && value == 200);

	/* 测试失败情况 */
	expected = 100;  /* 错误的期望值 */
	success = atomic_cas_int(&value, &expected, 300);
	printf("CAS(value=200, expected=100, desired=300): %s\n",
	       success ? "success" : "failure");
	printf("value = %d, expected updated to %d (expected: 200, 200)\n",
	       value, expected);
	assert(!success && value == 200 && expected == 200);

	/* ===== 测试 2: CAS 循环 (实现原子递增) ===== */
	volatile int atomic_counter = 0;

	for (int i = 0; i < 1000; i++) {
		atomic_inc(&atomic_counter);
	}

	printf("Atomic counter = %d (expected: 1000)\n", atomic_counter);
	assert(atomic_counter == 1000);

	/* ===== 测试 3: 自旋锁多线程测试 ===== */
	pthread_t threads[NUM_THREADS];

	printf("Starting %d threads, each incrementing %d times...\n",
	       NUM_THREADS, INCREMENTS_PER_THREAD);

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, increment_worker, NULL);
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("Final counter = %d (expected: %d)\n",
	       counter, NUM_THREADS * INCREMENTS_PER_THREAD);
	assert(counter == NUM_THREADS * INCREMENTS_PER_THREAD);

	/* ===== 测试 4: 独占加载/存储 ===== */
	volatile int excl_val = 42;
	int loaded, new_val, status;

	do {
		__asm__ __volatile__(
			"ldxr %w0, [%3]\n\t"      /* 独占加载 */
			"add %w2, %w0, #1\n\t"    /* 修改 */
			"stxr %w1, %w2, [%3]"     /* 独占存储 */
			: "=&r" (loaded), "=&r" (status), "=&r" (new_val)
			: "r" (&excl_val)
			: "memory"
			);
	} while (status != 0);

	printf("Exclusive load/store: loaded=%d, status=%d (expected: 42, 0)\n",
	       loaded, status);
	assert(loaded == 42 && status == 0);  /* status=0 表示成功 */
	assert(excl_val == 43);

	printf("\n=== All atomic CAS tests passed! ===\n");
	return 0;
}
