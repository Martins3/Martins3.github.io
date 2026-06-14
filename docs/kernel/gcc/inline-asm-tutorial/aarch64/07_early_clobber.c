/*
 * 示例 07: Early Clobber (早期修改) (ARM64/AArch64)
 *
 * 知识点:
 * - Early Clobber "&": 表示操作数在汇编指令结束前就被修改
 * - 解决输入输出寄存器分配的冲突问题
 * - 何时需要使用 &
 */

#include <stdio.h>
#include <assert.h>

int main(void)
{
	/* ===== 不使用 & 的问题 ===== */
	int a = 10, b = 20;

	/* 危险: 编译器可能将 a 和 b 分配到同一个寄存器 */
	__asm__ __volatile__(
		"eor %0, %0, %1\n\t"
		"eor %1, %0, %1\n\t"
		"eor %0, %0, %1"
		: "=r" (a), "=r" (b)
		: "r" (a), "r" (b)
		);

	printf("After swap (unsafe): a=%d, b=%d\n", a, b);

	/* ===== 使用 Early Clobber & ===== */
	a = 10; b = 20;

	/* & 表示这个操作数在指令结束前就被修改
	 * 强制编译器不要与输入使用同一寄存器
	 */
	__asm__ __volatile__(
		"mov %0, %2\n\t"        /* result_a = a */
		"add %0, %0, %3\n\t"    /* result_a += b */
		"mov %1, %2\n\t"        /* result_b = a */
		"sub %1, %1, %3"        /* result_b -= b */
		: "=&r" (a), "=&r" (b)
		: "r" (10), "r" (20)     /* 使用立即数值 */
		);

	printf("After calculation: a=%d, b=%d (expected: 30, -10)\n", a, b);

	/* ===== 正确的交换实现 ===== */
	a = 10; b = 20;

	/* 使用 + 修饰符表示读写操作数，避免 & 的复杂性 */
	__asm__ __volatile__(
		"eor %0, %0, %1\n\t"
		"eor %1, %0, %1\n\t"
		"eor %0, %0, %1"
		: "+r" (a), "+r" (b)
		);

	printf("Swap with +: a=%d, b=%d (expected: 20, 10)\n", a, b);
	assert(a == 20 && b == 10);

	/* ===== Early Clobber 实际案例: 字符串复制 ===== */
	char src[] = "Hello";
	char dst[6] = {0};
	char *s = src;
	char *d = dst;

	/* 使用 & 确保指针不会冲突 */
	__asm__ __volatile__(
		"ldrb w9, [%1]\n\t"     /* w9 = *s */
		"strb w9, [%0]\n\t"     /* *d = w9 */
		"add %0, %0, #1\n\t"    /* d++ */
		"add %1, %1, #1"        /* s++ */
		: "=&r" (d), "=&r" (s)
		: "0" (d), "1" (s)
		: "w9", "memory"
		);

	printf("First char copied: dst[0]=%c (expected: %c)\n", dst[0], src[0]);
	assert(dst[0] == 'H');

	/* ===== 复杂例子: 乘加运算 ===== */
	long a_val = 100, b_val = 7;
	long result;

	__asm__ __volatile__(
		"mul %0, %1, %2\n\t"    /* result = a * b */
		"add %0, %0, %1"        /* result += a */
		: "=&r" (result)
		: "r" (a_val), "r" (b_val)
		);

	printf("%ld * %ld + %ld = %ld (expected: 800)\n",
	       a_val, b_val, a_val, result);
	assert(result == 800);

	/* ===== 多个输出的早期修改 ===== */
	int x = 5, y = 3;
	int sum, diff;

	/* 同时计算和与差，结果在输入使用前产生 */
	__asm__ __volatile__(
		"add %0, %2, %3\n\t"    /* sum = x + y */
		"sub %1, %2, %3"        /* diff = x - y */
		: "=&r" (sum), "=&r" (diff)
		: "r" (x), "r" (y)
		);

	printf("sum=%d, diff=%d (expected: 8, 2)\n", sum, diff);
	assert(sum == 8 && diff == 2);

	/* ===== 匹配约束与 & 的组合使用 ===== */
	int val = 100;
	int mul_result;

	__asm__ __volatile__(
		"mul %1, %2, %2\n\t"    /* result = val * val */
		"add %1, %1, %2"        /* result += val */
		: "=&r" (val), "=r" (mul_result)
		: "0" (val)
		);

	printf("val=%d, result=%d (expected: 100, 10100)\n", val, mul_result);
	assert(mul_result == 10100);

	printf("\n=== All early clobber tests passed! ===\n");
	return 0;
}
