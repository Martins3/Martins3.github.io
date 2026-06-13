/*
 * 示例 06: Volatile 关键字详解
 *
 * 知识点:
 * - volatile 防止编译器删除或优化汇编代码
 * - 何时必须使用 volatile
 * - volatile 与优化的关系
 * - 常见陷阱
 */

#include <stdio.h>
#include <assert.h>

/* 用于测试的计数器 */
static volatile int counter = 0;

int main(void)
{
	int result;

	/* ===== 非 volatile 的问题 ===== */
	/* 如果不用 volatile，编译器可能只执行一次 */
	int x = 10;

	/* 非 volatile 版本 (仅演示，实际不这样写) */
	__asm__(
		"movl %1, %0"
		: "=r" (result)
		: "r" (x)
		);

	printf("Non-volatile result: %d\n", result);

	/* ===== 必须使用 volatile 的场景 ===== */

	/* 1. 硬件寄存器访问 */
	/* 模拟读取硬件寄存器: 每次读取都必须发生 */
	unsigned int hw_reg_value;

	__asm__ __volatile__(
		"movl $0xABCD1234, %0"  /* 模拟硬件寄存器读取 */
		: "=r" (hw_reg_value)
		);

	printf("Hardware register: 0x%08x\n", hw_reg_value);
	assert(hw_reg_value == 0xABCD1234);

	/* 2. 副作用操作 */
	/* 即使输出不被使用，操作也必须执行 */
	counter = 0;

	__asm__ __volatile__(
		"incl %0"
		: "+m" (counter)
		);

	printf("Counter after inc: %d\n", counter);
	assert(counter == 1);

	/* 3. 循环中的汇编 */
	int sum = 0;
	int i;

	for (i = 0; i < 5; i++) {
		__asm__ __volatile__(
			"addl %1, %0"
			: "+r" (sum)
			: "r" (i)
			);
	}

	printf("Sum = %d (expected: 0+1+2+3+4 = 10)\n", sum);
	assert(sum == 10);

	/* ===== 没有 volatile 可能被优化掉的情况 ===== */

	/* 假设我们有一个延迟循环 */
	unsigned int delay_count = 0;

	/* 这个空循环可能被编译器完全优化掉 */
	for (i = 0; i < 1000; i++) {
		__asm__ __volatile__("nop");  /* 使用 volatile 防止优化 */
		delay_count++;
	}

	printf("Delay loop executed %u times\n", delay_count);
	assert(delay_count == 1000);

	/* ===== 空汇编的用途 ===== */
	/* 空汇编配合 volatile 作为编译器屏障 */
	int a = 1, b = 2;

	a = 10;
	__asm__ __volatile__("" ::: "memory");  /* 编译器屏障 */
	b = 20;

	printf("a=%d, b=%d\n", a, b);
	assert(a == 10 && b == 20);

	/* ===== 性能计数器读取 (必须使用 volatile) ===== */
	unsigned long tsc_before, tsc_after;

	/* 读取时间戳计数器 (RDTSC) */
	__asm__ __volatile__(
		"rdtsc\n\t"             /* 读取 tsc 到 edx:eax */
		"shlq $32, %%rdx\n\t"   /* rdx <<= 32 */
		"orq %%rdx, %0"         /* rax |= rdx */
		: "=a" (tsc_before)
		:
		: "rdx"
		);

	/* 一些工作 */
	volatile int work = 0;
	for (i = 0; i < 100; i++) {
		work += i;
	}

	__asm__ __volatile__(
		"rdtsc\n\t"
		"shlq $32, %%rdx\n\t"
		"orq %%rdx, %0"
		: "=a" (tsc_after)
		:
		: "rdx"
		);

	unsigned long cycles = tsc_after - tsc_before;
	printf("Work = %d, cycles = %lu\n", work, cycles);

	/* ===== 输入输出相同但没有副作用 ===== */
	/* 这种情况可能不需要 volatile，但通常建议加 */
	int input_output = 5;

	__asm__ __volatile__(
		"addl $10, %0"
		: "+r" (input_output)
		);

	printf("input_output = %d (expected: 15)\n", input_output);
	assert(input_output == 15);

	/* ===== 字符串输出 (副作用) ===== */
	/* 使用 syscall 输出，必须有 volatile */
	const char msg[] = "Direct syscall output\n";

	__asm__ __volatile__(
		"movq $1, %%rax\n\t"    /* syscall: write */
		"movq $1, %%rdi\n\t"    /* fd: stdout */
		"movq %0, %%rsi\n\t"    /* buf */
		"movq %1, %%rdx\n\t"    /* count */
		"syscall"
		:
		: "r" (msg), "r" (sizeof(msg) - 1)
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
		);

	printf("\n=== All volatile tests passed! ===\n");
	return 0;
}
