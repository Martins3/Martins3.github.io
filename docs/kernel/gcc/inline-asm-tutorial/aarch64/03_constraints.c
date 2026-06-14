/*
 * 示例 03: 约束条件 (Constraints) 详解 (ARM64/AArch64)
 *
 * 知识点:
 * - 通用寄存器约束: "r" (任何通用寄存器 x0-x30)
 * - 特定寄存器约束: 通过 register asm 语法
 * - 内存约束: "m" (内存操作数)
 * - 立即数约束: "i" (立即数), "n" (已知数值的立即数)
 * - 匹配约束: "0", "1" 等表示与前面某个操作数使用同一位置
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

int main(void)
{
	/* ===== 特定寄存器 (通过 register asm) ===== */
	unsigned long x0_val = 0;
	unsigned int w0_val = 42;

	/* 强制使用特定寄存器 (ARM64 系统调用号在 x8) */
	register long x8_val __asm__("x8") = 64;  /* write syscall */

	(void)x8_val;  /* 抑制警告 */

	/* 使用通用约束 (AArch64 mov 立即数编码有限，用 ldr = 伪指令加载任意常量) */
	__asm__ __volatile__(
		"ldr %0, =%c1"
		: "=r" (x0_val)
		: "i" (123456)
		);

	printf("x0 = %lu (expected: 123456)\n", x0_val);
	assert(x0_val == 123456);

	/* 使用 w0 作为输入 (32-bit) */
	unsigned int result;
	__asm__ __volatile__(
		"add %w0, %w1, #8"
		: "=r" (result)
		: "r" (w0_val)       /* 输入 */
		);

	printf("%d + 8 = %d (expected: 50)\n", w0_val, result);
	assert(result == 50);

	/* ===== 内存约束 ===== */
	int mem_val = 100;

	/* "m" 约束表示操作数在内存中 */
	__asm__ __volatile__(
		"ldr %w0, %1\n\t"
		"add %w0, %w0, #1\n\t"
		"str %w0, %1"
		: "=r" (mem_val)
		: "m" (mem_val)
		);

	printf("After inc: %d (expected: 101)\n", mem_val);
	assert(mem_val == 101);

	/* ===== 立即数约束 ===== */
	int imm_result;

	/* "i" 约束表示立即数 (AArch64 mov 立即数编码有限，用 ldr = 伪指令加载任意常量) */
	__asm__ __volatile__(
		"ldr %0, =%c1"
		: "=r" (imm_result)
		: "i" (0xDEADBEEF)    /* 编译时常数 */
		);

	printf("imm_result = 0x%x (expected: 0xdeadbeef)\n", imm_result);
	assert(imm_result == (int)0xDEADBEEF);

	/* ===== 匹配约束 ===== */
	int in_out = 10;

	__asm__ __volatile__(
		"add %0, %0, %1"      /* in_out += 20 */
		: "=r" (in_out)
		: "r" (20), "0" (in_out) /* "0" 表示与 %0 同一位置 */
		);

	printf("in_out = %d (expected: 30)\n", in_out);
	assert(in_out == 30);

	/* 更好的方式: 使用 + 修饰符表示读写 */
	int rw_val = 5;
	__asm__ __volatile__(
		"lsl %0, %0, #2"      /* rw_val <<= 2 (乘以 4) */
		: "+r" (rw_val)       /* + 表示读写 */
		);

	printf("rw_val = %d (expected: 20)\n", rw_val);
	assert(rw_val == 20);

	/* ===== 地址约束 ===== */
	int arr[4] = {10, 20, 30, 40};
	int *ptr = &arr[1];
	int loaded;

	__asm__ __volatile__(
		"ldr %w0, [%1]"
		: "=r" (loaded)
		: "r" (ptr)           /* 指针在寄存器中 */
		);

	printf("loaded = %d (expected: 20)\n", loaded);
	assert(loaded == 20);

	printf("\n=== All constraint tests passed! ===\n");
	return 0;
}
