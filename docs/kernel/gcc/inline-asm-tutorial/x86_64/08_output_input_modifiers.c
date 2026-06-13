/*
 * 示例 08: 输入输出修饰符详解
 *
 * 知识点:
 * - = : 只写 (Write-only)
 * - + : 读写 (Read-write)
 * - & : 早期修改 (Early clobber)
 * - % : 可交换操作数 (Commutative)
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
		"movl %1, %0\n\t"
		"addl $8, %0"
		: "=r" (output_only)
		: "r" (input_val)
		);

	printf("output_only = %d (expected: 50)\n", output_only);
	assert(output_only == 50);

	/* ===== + : 读写修饰符 ===== */
	int rw_val = 10;

	/* "+r" 表示这个变量既是输入又是输出 */
	__asm__ __volatile__(
		"shll $2, %0"          /* rw_val *= 4 */
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
		"movl %2, %0\n\t"      /* result_a = a */
		"addl %3, %0\n\t"      /* result_a += b (result_a = a + b = 8) */
		"movl %2, %1\n\t"      /* result_b = a */
		"subl %3, %1"          /* result_b -= b (result_b = a - b = 2) */
		: "=&r" (result_a), "=&r" (result_b)
		: "r" (a), "r" (b)
		);

	printf("result_a=%d, result_b=%d (expected: 8, 2)\n", result_a, result_b);
	assert(result_a == 8 && result_b == 2);

	/* ===== 数字匹配约束 ===== */
	int x = 100;
	int y = 0;  /* 初始化避免警告，实际不会被使用 */

	/* "0" 表示与 %0 使用同一个位置/寄存器
	 * 这里 y 既是输入也是输出，使用同一寄存器
	 */
	__asm__ __volatile__(
		"movl %1, %0\n\t"
		"addl %0, %0"          /* y = x + x = 2*x */
		: "=r" (y)
		: "r" (x), "0" (y)      /* "0" 表示与 %0 使用同一寄存器 */
		);

	/* ===== 多操作数匹配 ===== */
	int op1 = 10, op2 = 20;
	int out1, out2;

	/* 更复杂的匹配场景 */
	__asm__ __volatile__(
		"movl %2, %0\n\t"      /* out1 = op1 */
		"movl %3, %1\n\t"      /* out2 = op2 */
		"addl %1, %0"          /* out1 += out2 */
		: "=r" (out1), "=r" (out2)
		: "r" (op1), "r" (op2)
		);

	printf("out1=%d, out2=%d (expected: 30, 20)\n", out1, out2);
	assert(out1 == 30 && out2 == 20);

	/* ===== % : 可交换操作数修饰符 ===== */
	/* 在某些架构上，% 告诉编译器操作数可以交换位置
	 * 主要用于优化，x86 上不太常见
	 */

	/* ===== 组合修饰符 ===== */
	int complex_val = 5;
	int complex_result = 0;  /* 初始化避免警告 */

	/* +& : 读写且早期修改 */
	__asm__ __volatile__(
		"movl %1, %0\n\t"
		"addl $10, %0\n\t"
		"addl %2, %0"
		: "+&r" (complex_result)
		: "r" (complex_val), "r" (100)
		);

	printf("complex_result = %d (expected: 115)\n", complex_result);
	assert(complex_result == 115);

	/* ===== 内存操作数修饰符 ===== */
	int mem_val = 50;

	/* "+m" 表示内存操作数，读写 */
	__asm__ __volatile__(
		"addl $25, %0"
		: "+m" (mem_val)
		);

	printf("mem_val = %d (expected: 75)\n", mem_val);
	assert(mem_val == 75);

	/* ===== 立即数与寄存器组合 ===== */
	int reg_result;

	__asm__ __volatile__(
		"movl %1, %0\n\t"
		"addl %2, %0"
		: "=r" (reg_result)
		: "r" (100), "i" (200)   /* i 表示立即数 */
		);

	printf("reg_result = %d (expected: 300)\n", reg_result);
	assert(reg_result == 300);

	/* ===== 实际案例: 原子交换 ===== */
	int old_val = 0;
	int new_val = 42;
	int *ptr = &old_val;

	/* xchg: 交换寄存器和内存的值
	 * +m 用于内存操作数 (读写)
	 * =r 用于获取旧值
	 */
	__asm__ __volatile__(
		"xchgl %0, %1"
		: "=r" (new_val), "+m" (*ptr)
		: "0" (new_val)          /* 0 表示与 %0 同一位置 */
		: "memory"
		);

	printf("After xchg: old_val=%d, new_val=%d (expected: 42, 0)\n",
	       old_val, new_val);
	assert(old_val == 42 && new_val == 0);

	printf("\n=== All modifier tests passed! ===\n");
	return 0;
}
