/*
 * 示例 11: ARM64 通用计时器 (Generic Timer)
 *
 * 知识点:
 * - CNTVCT_EL0: 虚拟计数器 (类似于 x86 TSC)
 * - CNTPCT_EL0: 物理计数器
 * - CNTFRQ_EL0: 计数器频率
 * - 使用 ISB 序列化指令流
 * - 实际应用: 性能测量
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

/* 读取虚拟计数器 (CNTVCT_EL0)
 * 这是虚拟化的安全计数器，推荐在用户空间使用
 */
static inline uint64_t read_cntvct(void)
{
	uint64_t val;

	__asm__ __volatile__(
		"mrs %0, cntvct_el0"
		: "=r" (val)
		);

	return val;
}

/* 读取物理计数器 (CNTPCT_EL0) */
static inline uint64_t read_cntpct(void)
{
	uint64_t val;

	__asm__ __volatile__(
		"mrs %0, cntpct_el0"
		: "=r" (val)
		);

	return val;
}

/* 读取计数器频率 */
static inline uint64_t read_cntfrq(void)
{
	uint64_t val;

	__asm__ __volatile__(
		"mrs %0, cntfrq_el0"
		: "=r" (val)
		);

	return val;
}

/* 使用 ISB 序列化的计时器读取
 * ISB (Instruction Synchronization Barrier) 确保前面的指令完成
 */
static inline uint64_t read_cntvct_serialized(void)
{
	uint64_t val;

	__asm__ __volatile__(
		"isb\n\t"                /* 序列化 */
		"mrs %0, cntvct_el0\n\t"
		"isb"                   /* 再次序列化 */
		: "=r" (val)
		:
		: "memory"
		);

	return val;
}

/* 性能测量宏 */
#define MEASURE_CYCLES(code) do { \
	uint64_t _start = read_cntvct_serialized(); \
	code; \
	uint64_t _end = read_cntvct_serialized(); \
	printf("Cycles for " #code ": %" PRIu64 "\n", _end - _start); \
} while(0)

static void empty_function(void) {}

int main(void)
{
	uint64_t cnt1, cnt2;
	uint64_t freq;

	/* ===== 测试 1: 基本计数器读取 ===== */
	freq = read_cntfrq();
	printf("Counter frequency: %" PRIu64 " Hz (%.2f MHz)\n",
	       freq, (double)freq / 1000000);

	cnt1 = read_cntvct();
	/* 一些工作 */
	volatile int x = 0;
	for (int i = 0; i < 100; i++) {
		x += i;
	}
	cnt2 = read_cntvct();

	printf("Timer diff for small loop: %" PRIu64 " ticks\n", cnt2 - cnt1);
	printf("x = %d (prevent optimization)\n", x);
	assert(cnt2 > cnt1);

	/* ===== 测试 2: 序列化读取 ===== */
	cnt1 = read_cntvct_serialized();
	cnt2 = read_cntvct_serialized();

	printf("Overhead of serialized read: %" PRIu64 " ticks\n", cnt2 - cnt1);

	/* ===== 测试 3: 物理计数器 ===== */
	/* CNTPCT_EL0 在多数系统上需要 EL1 权限，用户态读取可能触发 Illegal Instruction。
	 * 这里保留函数实现作为参考，但默认不调用。
	 */
	/* cnt1 = read_cntpct(); */
	/* printf("Physical counter: %" PRIu64 "\n", cnt1); */

	/* ===== 测试 4: 测量不同操作的周期数 ===== */
	printf("\n=== Measuring operation latencies ===\n");

	/* 整数加法 */
	volatile int a = 1, b = 2, c;
	MEASURE_CYCLES(c = a + b);
	(void)c;

	/* 整数乘法 */
	MEASURE_CYCLES(c = a * b);

	/* 整数除法 */
	MEASURE_CYCLES(c = a / b);

	/* 内存访问 */
	volatile int arr[100];
	MEASURE_CYCLES(arr[0] = 42);
	MEASURE_CYCLES(c = arr[50]);

	/* ===== 测试 5: 循环测量 ===== */
	uint64_t loop_start, loop_end;
	volatile int sum = 0;
	const int iterations = 10000;

	loop_start = read_cntvct_serialized();
	for (int i = 0; i < iterations; i++) {
		sum += i;
	}
	loop_end = read_cntvct_serialized();

	uint64_t total_ticks = loop_end - loop_start;
	printf("\nTotal ticks for %d iterations: %" PRIu64 "\n",
	       iterations, total_ticks);
	printf("Average ticks per iteration: %.2f\n",
	       (double)total_ticks / iterations);
	printf("Sum = %d\n", sum);

	/* ===== 测试 6: 函数调用开销 ===== */
	uint64_t call_start, call_end;

	call_start = read_cntvct_serialized();
	for (int i = 0; i < 1000; i++) {
		empty_function();
	}
	call_end = read_cntvct_serialized();

	printf("\nFunction call overhead: %.2f ticks\n",
	       (double)(call_end - call_start) / 1000);

	/* ===== 测试 7: 内存屏障开销 ===== */
	uint64_t fence_start, fence_end;

	fence_start = read_cntvct_serialized();
	__asm__ __volatile__("dmb sy" ::: "memory");
	fence_end = read_cntvct_serialized();

	printf("dmb sy latency: %" PRIu64 " ticks\n", fence_end - fence_start);

	/* ===== 测试 8: 转换为时间 ===== */
	uint64_t tick_diff = read_cntvct_serialized() - read_cntvct_serialized();
	double time_us = (double)tick_diff / freq * 1000000;
	printf("\nTime for two serialized reads: %.3f us\n", time_us);

	printf("\n=== All timer tests passed! ===\n");
	return 0;
}
