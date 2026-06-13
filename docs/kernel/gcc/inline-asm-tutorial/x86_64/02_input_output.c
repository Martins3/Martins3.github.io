/*
 * 示例 02: 带输入输出的内联汇编 (Extended Asm)
 *
 * 知识点:
 * - Extended Asm 语法: asm [volatile] (asm template : outputs : inputs : clobbers)
 * - "=r" 表示输出操作数，只写，使用寄存器
 * - "r" 表示输入操作数，只读，使用寄存器
 * - %% 在模板中转义为单个 %
 * %0, %1 等表示第 n 个操作数
 */

#include <stdio.h>
#include <assert.h>

int main(void)
{
	int a = 10, b = 20;
	int result;

	/* 简单的加法: result = a + b
	 *
	 * 约束解释:
	 * - "=r" (result) : 输出操作数 %0，使用寄存器，= 表示只写
	 * - "r" (a)       : 输入操作数 %1，使用寄存器
	 * - "r" (b)       : 输入操作数 %2，使用寄存器
	 */
	__asm__ __volatile__(
		"movl %1, %0\n\t"    /* result = a */
		"addl %2, %0"        /* result += b */
		: "=r" (result)       /* %0: 输出 */
		: "r" (a), "r" (b)    /* %1: a, %2: b */
		);

	printf("%d + %d = %d (expected: 30)\n", a, b, result);
	assert(result == 30);

	/* 乘法示例 */
	long x = 100, y = 25;
	long product;

	/* 使用 imul 指令: product = x * y */
	__asm__ __volatile__(
		"movq %1, %%rax\n\t"
		"imulq %2, %%rax\n\t"
		"movq %%rax, %0"
		: "=r" (product)
		: "r" (x), "r" (y)
		: "rax"               /* clobber: rax 被修改 */
		);

	printf("%ld * %ld = %ld (expected: 2500)\n", x, y, product);
	assert(product == 2500);

	/* 位操作示例: 计算前导零个数 */
	unsigned int value = 0x00100000;  /* 只有第 20 位被设置 */
	unsigned int lzcnt;

	/* lzcnt: 计算前导零个数 (需要 BMI1 指令集支持) */
	/* 如果不支持，可以用 bsr (bit scan reverse) 模拟 */
	__asm__ __volatile__(
		"bsrl %1, %0\n\t"     /* 找到最高位 1 的位置 */
		"xorl $31, %0"        /* 转换为前导零个数 */
		: "=r" (lzcnt)
		: "r" (value)
		);

	printf("Leading zeros of 0x%08x = %u (expected: 11)\n", value, lzcnt);
	assert(lzcnt == 11);  /* 32 - 21 = 11 */

	/* 交换两个变量的值 */
	int m = 100, n = 200;

	__asm__ __volatile__(
		"xchgl %0, %1"
		: "+r" (m), "+r" (n)   /* + 表示读写操作数 */
		);

	printf("After swap: m=%d, n=%d (expected: 200, 100)\n", m, n);
	assert(m == 200 && n == 100);

	printf("\n=== All tests passed! ===\n");
	return 0;
}
