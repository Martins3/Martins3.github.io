/*
 * 示例 08: 输入输出修饰符详解 (ARM64/AArch64)
 *
 * 知识点:
 * - = : 只写 (Write-only)
 * - + : 读写 (Read-write)
 * - & : 早期修改 (Early clobber)
 * - 数字匹配: 0, 1, 2... 表示与第n个操作数同一位置
 */

#include <stdio.h>
#include <assert.h>

int main(void)
{
	/* ===== = : 只写修饰符 ===== */
	int output_only;
	int input_val = 42;

	/* "=r" 表示这是一个纯输出，汇编代码会覆盖它 */
	__asm__ __volatile__(
		"mov %0, %1\n\t"
		"add %0, %0, #8"
		: "=r" (output_only)
		: "r" (input_val)
		);

	printf("output_only = %d (expected: 50)\n", output_only);
	assert(output_only == 50);

	/* ===== + : 读写修饰符 ===== */
	int rw_val = 10;

	/* "+r" 表示这个变量既是输入又是输出 */
	__asm__ __volatile__(
		"lsl %0, %0, #2"        /* rw_val <<= 2 (乘以 4) */
		: "+r" (rw_val)
		);

	printf("rw_val = %d (expected: 40)\n", rw_val);
	assert(rw_val == 40);

	/* ===== & : 早期修改修饰符 ===== */
	int a = 5, b = 3;
	int result_a, result_b;

	/* "=&r" 表示这个输出在输入使用完之前就被修改
	 * 两个输出都需要 & 以确保它们不与输入共享寄存器
	 */
	__asm__ __volatile__(
		"add %0, %2, %3\n\t"    /* result_a = a + b = 8 */
		"sub %1, %2, %3"        /* result_b = a - b = 2 */
		: "=&r" (result_a), "=&r" (result_b)
		: "r" (a), "r" (b)
		);

	printf("result_a=%d, result_b=%d (expected: 8, 2)\n", result_a, result_b);
	assert(result_a == 8 && result_b == 2);

	/* ===== 数字匹配约束 ===== */
	int x = 100;
	int y = 0;

	/* "0" 表示与 %0 使用同一个位置/寄存器 */
	__asm__ __volatile__(
		"mov %0, %1\n\t"
		"add %0, %0, %0"        /* y = x + x = 2*x */
		: "=r" (y)
		: "r" (x), "0" (y)
		);

	/* ===== 多操作数匹配 ===== */
	int op1 = 10, op2 = 20;
	int out1, out2;

	__asm__ __volatile__(
		"mov %0, %2\n\t"        /* out1 = op1 */
		"mov %1, %3\n\t"        /* out2 = op2 */
		"add %0, %0, %1"        /* out1 += out2 */
		: "=r" (out1), "=r" (out2)
		: "r" (op1), "r" (op2)
		);

	printf("out1=%d, out2=%d (expected: 30, 20)\n", out1, out2);
	assert(out1 == 30 && out2 == 20);

	/* ===== 组合修饰符 ===== */
	int complex_val = 5;
	int complex_result = 0;

	/* +& : 读写且早期修改 */
	__asm__ __volatile__(
		"mov %0, %1\n\t"
		"add %0, %0, #10\n\t"
		"add %0, %0, #100"
		: "+&r" (complex_result)
		: "r" (complex_val)
		);

	printf("complex_result = %d (expected: 115)\n", complex_result);
	assert(complex_result == 115);

	/* ===== 内存操作数修饰符 ===== */
	int mem_val = 50;

	/* "+m" 表示内存操作数，读写 */
	__asm__ __volatile__(
		"ldr %w0, %1\n\t"
		"add %w0, %w0, #25\n\t"
		"str %w0, %1"
		: "=r" (mem_val)
		: "m" (mem_val)
		);

	printf("mem_val = %d (expected: 75)\n", mem_val);
	assert(mem_val == 75);

	/* ===== 立即数与寄存器组合 ===== */
	int reg_result;

	__asm__ __volatile__(
		"mov %0, %1\n\t"
		"add %0, %0, %2"
		: "=r" (reg_result)
		: "r" (100), "i" (200)   /* i 表示立即数 */
		);

	printf("reg_result = %d (expected: 300)\n", reg_result);
	assert(reg_result == 300);

	/* ===== 实际案例: 原子交换 ===== */
	int old_val = 0;
	int new_val = 42;
	int *ptr = &old_val;

	/* swp: 交换寄存器和内存的值 (ARM64 已弃用，用 ld/st 替代演示) */
	__asm__ __volatile__(
		"ldxr %0, [%2]\n\t"     /* 加载旧值 */
		"stxr w9, %1, [%2]\n\t" /* 存储新值 */
		: "=r" (new_val)
		: "r" (new_val), "r" (ptr)
		: "w9", "memory"
		);

	printf("After swap: old_val=%d, new_val=%d\n", old_val, new_val);

	printf("\n=== All modifier tests passed! ===\n");
	return 0;
}
