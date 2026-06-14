/*
 * 示例 06: Volatile 关键字详解 (ARM64/AArch64)
 *
 * 知识点:
 * - volatile 防止编译器删除或优化汇编代码
 * - 何时必须使用 volatile
 * - volatile 与优化的关系
 * - ARM64 性能计数器读取 (CNTPCT)
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

/* 读取 ARM64 通用计时器 (CNTVCT_EL0 - Virtual Counter)
 * 类似于 x86 的 TSC
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

int main(void)
{
	int result;

	/* ===== 非 volatile 的问题 ===== */
	int x = 10;

	__asm__(
		"mov %0, %1"
		: "=r" (result)
		: "r" (x)
		);

	printf("Non-volatile result: %d\n", result);

	/* ===== 必须使用 volatile 的场景 ===== */

	/* 1. 硬件寄存器访问 */
	unsigned int hw_reg_value;

	__asm__ __volatile__(
		"ldr %0, =0xABCD1234"   /* 模拟硬件寄存器读取 */
		: "=r" (hw_reg_value)
		);

	printf("Hardware register: 0x%08x\n", hw_reg_value);
	assert(hw_reg_value == 0xABCD1234);

	/* 2. 副作用操作 */
	static volatile int counter = 0;

	__asm__ __volatile__(
		"add %0, %0, #1"
		: "+r" (counter)
		);

	printf("Counter after inc: %d\n", counter);
	assert(counter == 1);

	/* 3. 循环中的汇编 */
	int sum = 0;
	int i;

	for (i = 0; i < 5; i++) {
		__asm__ __volatile__(
			"add %0, %0, %1"
			: "+r" (sum)
			: "r" (i)
			);
	}

	printf("Sum = %d (expected: 0+1+2+3+4 = 10)\n", sum);
	assert(sum == 10);

	/* ===== 没有 volatile 可能被优化掉的情况 ===== */
	unsigned int delay_count = 0;

	for (i = 0; i < 1000; i++) {
		__asm__ __volatile__("nop");
		delay_count++;
	}

	printf("Delay loop executed %u times\n", delay_count);
	assert(delay_count == 1000);

	/* ===== 空汇编的用途 ===== */
	int a = 1, b = 2;

	a = 10;
	__asm__ __volatile__("" ::: "memory");  /* 编译器屏障 */
	b = 20;

	printf("a=%d, b=%d\n", a, b);
	assert(a == 10 && b == 20);

	/* ===== 性能计数器读取 ===== */
	uint64_t cnt_before, cnt_after;
	uint64_t freq = read_cntfrq();

	printf("\nTimer frequency: %" PRIu64 " Hz\n", freq);

	cnt_before = read_cntvct();

	/* 一些工作 */
	volatile int work = 0;
	for (i = 0; i < 100; i++) {
		work += i;
	}

	cnt_after = read_cntvct();

	uint64_t ticks = cnt_after - cnt_before;
	printf("Work = %d, timer ticks = %" PRIu64 "\n", work, ticks);

	/* ===== 输入输出相同但没有副作用 ===== */
	int input_output = 5;

	__asm__ __volatile__(
		"add %0, %0, #10"
		: "+r" (input_output)
		);

	printf("input_output = %d (expected: 15)\n", input_output);
	assert(input_output == 15);

	/* ===== 字符串输出 (副作用) ===== */
	const char msg[] = "Direct syscall output\n";

	__asm__ __volatile__(
		"mov x8, #64\n\t"       /* syscall: write */
		"mov x0, #1\n\t"        /* fd: stdout */
		"mov x1, %0\n\t"        /* buf */
		"mov x2, %1\n\t"        /* count */
		"svc #0"
		:
		: "r" (msg), "r" ((long)(sizeof(msg) - 1))
		: "x0", "x1", "x2", "x8", "memory"
		);

	printf("\n=== All volatile tests passed! ===\n");
	return 0;
}
