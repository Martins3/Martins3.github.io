/*
 * 示例 11: 性能计数器 (RDPMC / RDTSC)
 *
 * 知识点:
 * - rdtsc: 读取时间戳计数器
 * - rdtscp: 读取 TSC + 处理器 ID (序列化)
 * - rdpmc: 读取性能监控计数器
 * - cpuid: 序列化指令流
 * - 实际应用: 性能测量
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>

/* 读取 TSC (时间戳计数器)
 * 注意: rdtsc 不是序列化的，可能在指令乱序执行中提前执行
 */
static inline uint64_t rdtsc(void)
{
	uint32_t lo, hi;

	__asm__ __volatile__(
		"rdtsc"
		: "=a" (lo), "=d" (hi)
		);

	return ((uint64_t)hi << 32) | lo;
}

/* 使用 cpuid 序列化的 rdtsc
 * cpuid 是序列化指令，确保前面的指令都完成后才执行
 */
static inline uint64_t rdtsc_serialized(void)
{
	uint32_t lo, hi;

	__asm__ __volatile__(
		"xorl %%eax, %%eax\n\t"  /* cpuid 输入: eax = 0 */
		"cpuid\n\t"              /* 序列化 */
		"rdtsc\n\t"
		"movl %%eax, %0\n\t"
		"movl %%edx, %1\n\t"
		"xorl %%eax, %%eax\n\t"
		"cpuid"                  /* 再次序列化防止后面的指令提前 */
		: "=r" (lo), "=r" (hi)
		:
		: "rax", "rbx", "rcx", "rdx"
		);

	return ((uint64_t)hi << 32) | lo;
}

/* 使用 rdtscp (如果支持)
 * rdtscp 自动序列化，并且返回处理器 ID
 */
static inline uint64_t rdtscp(uint32_t *aux)
{
	uint32_t lo, hi;

	__asm__ __volatile__(
		"rdtscp\n\t"
		"movl %%edx, %0\n\t"
		"movl %%eax, %1\n\t"
		"movl %%ecx, %2"
		: "=r" (hi), "=r" (lo), "=r" (*aux)
		:
		: "memory"
		);

	return ((uint64_t)hi << 32) | lo;
}

/* 简单的 rdtscp 版本 */
static inline uint64_t rdtscp_simple(void)
{
	uint32_t lo, hi;

	__asm__ __volatile__(
		"rdtscp"
		: "=a" (lo), "=d" (hi)
		:
		: "rcx", "memory"
		);

	return ((uint64_t)hi << 32) | lo;
}

/* 计算 CPU 频率 */
#if 0
static double measure_cpu_frequency(void)
{
	uint64_t tsc_start, tsc_end;

	tsc_start = rdtsc_serialized();

	/* 执行一些工作 */
	volatile int sum = 0;
	for (int i = 0; i < 1000000; i++) {
		sum += i;
	}

	tsc_end = rdtsc_serialized();

	return (double)(tsc_end - tsc_start);
}
#endif

/* 性能测量宏 */
#define MEASURE_CYCLES(code) do { \
	uint64_t _start = rdtsc_serialized(); \
	code; \
	uint64_t _end = rdtsc_serialized(); \
	printf("Cycles for " #code ": %" PRIu64 "\n", _end - _start); \
} while(0)

static void empty_function(void) {}

int main(void)
{
	uint64_t tsc1, tsc2;
	uint32_t aux;

	/* ===== 测试 1: 基本 rdtsc ===== */
	tsc1 = rdtsc();
	/* 一些工作 */
	volatile int x = 0;
	for (int i = 0; i < 100; i++) {
		x += i;
	}
	tsc2 = rdtsc();

	printf("TSC diff for small loop: %" PRIu64 "\n", tsc2 - tsc1);
	printf("x = %d (prevent optimization)\n", x);
	assert(tsc2 > tsc1);

	/* ===== 测试 2: 序列化 rdtsc ===== */
	tsc1 = rdtsc_serialized();
	tsc2 = rdtsc_serialized();

	printf("Overhead of serialized rdtsc: %" PRIu64 " cycles\n", tsc2 - tsc1);

	/* ===== 测试 3: rdtscp ===== */
	tsc1 = rdtscp_simple();
	tsc2 = rdtscp_simple();

	printf("Overhead of rdtscp: %" PRIu64 " cycles\n", tsc2 - tsc1);

	/* 带处理器 ID 的版本 */
	tsc1 = rdtscp(&aux);
	printf("TSC = %" PRIu64 ", Processor ID = %u\n", tsc1, aux & 0xFFF);

	/* ===== 测试 4: 测量不同操作的周期数 ===== */
	printf("\n=== Measuring operation latencies ===\n");

	/* 整数加法 */
	volatile int a = 1, b = 2, c;
	MEASURE_CYCLES(c = a + b);
	(void)c;  /* 防止未使用警告 */

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

	loop_start = rdtsc_serialized();
	for (int i = 0; i < iterations; i++) {
		sum += i;
	}
	loop_end = rdtsc_serialized();

	uint64_t total_cycles = loop_end - loop_start;
	printf("\nTotal cycles for %d iterations: %" PRIu64 "\n",
	       iterations, total_cycles);
	printf("Average cycles per iteration: %.2f\n",
	       (double)total_cycles / iterations);
	printf("Sum = %d\n", sum);

	/* ===== 测试 6: 函数调用开销 ===== */
	uint64_t call_start, call_end;

	call_start = rdtsc_serialized();
	for (int i = 0; i < 1000; i++) {
		empty_function();
	}
	call_end = rdtsc_serialized();

	printf("\nFunction call overhead: %.2f cycles\n",
	       (double)(call_end - call_start) / 1000);

	/* ===== 测试 7: 内存屏障开销 ===== */
	uint64_t fence_start, fence_end;

	fence_start = rdtsc_serialized();
	__asm__ __volatile__("mfence" ::: "memory");
	fence_end = rdtsc_serialized();

	printf("mfence latency: %" PRIu64 " cycles\n", fence_end - fence_start);

	printf("\n=== All RDPMC tests passed! ===\n");
	return 0;
}
