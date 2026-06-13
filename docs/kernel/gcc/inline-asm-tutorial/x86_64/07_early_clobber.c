/*
 * 示例 07: Early Clobber (早期修改)
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
	/* 假设我们想交换两个变量 */
	int a = 10, b = 20;

	/* 危险: 编译器可能将 a 和 b 分配到同一个寄存器
	 * 当它们值相同时，这种优化是合法的
	 */
	__asm__ __volatile__(
		"xchgl %0, %1"
		: "=r" (a), "=r" (b)   /* 输出 */
		: "r" (a), "r" (b)     /* 输入 */
		);

	/* 注意: 这个版本有潜在问题，下面的正确版本演示 & */
	printf("After swap (unsafe): a=%d, b=%d\n", a, b);

	/* ===== 使用 Early Clobber & ===== */
	a = 10; b = 20;

	/* & 表示这个操作数在指令结束前就被修改
	 * 强制编译器不要与输入使用同一寄存器
	 */
	__asm__ __volatile__(
		"movl %2, %0\n\t"      /* temp = a */
		"movl %3, %1\n\t"      /* b = temp_b (使用输入值) */
		"movl %0, %2"          /* a = temp */
		: "=&r" (a), "=r" (b)  /* & 表示早期修改 */
		: "r" (a), "r" (b)
		);

	printf("After swap (safe): a=%d, b=%d (expected: 20, 10)\n", a, b);
	/* 注意: 上面的实现有问题，只是为了演示 & 语法 */

	/* ===== 正确的交换实现 ===== */
	a = 10; b = 20;

	/* 使用 + 修饰符表示读写操作数，避免 & 的复杂性 */
	__asm__ __volatile__(
		"xchgl %0, %1"
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
		"movb (%1), %%al\n\t"   /* al = *s */
		"movb %%al, (%0)\n\t"   /* *d = al */
		"incq %0\n\t"          /* d++ */
		"incq %1"              /* s++ */
		: "=&r" (d), "=&r" (s)   /* & 表示在输入使用前修改 */
		: "0" (d), "1" (s)       /* 匹配前面的输出位置 */
		: "al", "memory"
		);

	printf("First char copied: dst[0]=%c (expected: %c)\n", dst[0], src[0]);
	assert(dst[0] == 'H');

	/* ===== 复杂例子: 除法运算 ===== */
	long dividend = 100;
	long divisor = 7;
	long quotient, remainder;

	/* idiv 使用 rdx:rax 作为被除数
	 * rax 存放结果，rdx 存放余数
	 * 这两个都在指令结束前被修改
	 */
	__asm__ __volatile__(
		"movq %2, %%rax\n\t"
		"cqto\n\t"              /* 符号扩展到 rdx */
		"idivq %3"              /* rax = rdx:rax / divisor */
		: "=&a" (quotient), "=&d" (remainder)  /* & 因为 rdx 在 idiv 前被修改 */
		: "r" (dividend), "r" (divisor)
		: "cc"
		);

	printf("%ld / %ld = %ld, remainder = %ld\n",
	       dividend, divisor, quotient, remainder);
	assert(quotient == 14 && remainder == 2);

	/* ===== 多个输出的早期修改 ===== */
	int x = 5, y = 3;
	int sum, diff;

	/* 同时计算和与差，结果在输入使用前产生 */
	__asm__ __volatile__(
		"movl %2, %0\n\t"      /* sum = x */
		"addl %3, %0\n\t"      /* sum += y */
		"movl %2, %1\n\t"      /* diff = x */
		"subl %3, %1"          /* diff -= y */
		: "=&r" (sum), "=&r" (diff)
		: "r" (x), "r" (y)
		);

	printf("sum=%d, diff=%d (expected: 8, 2)\n", sum, diff);
	assert(sum == 8 && diff == 2);

	/* ===== 匹配约束与 & 的组合使用 ===== */
	int val = 100;
	int result;

	/* 0 表示与 %0 使用同一位置，但 & 确保它在输入使用前修改 */
	__asm__ __volatile__(
		"leal (%0, %0, 4), %1" /* result = val + val*4 = 5*val */
		: "=&r" (val), "=r" (result)
		: "0" (val)
		);

	printf("val=%d, result=%d (expected: 100, 500)\n", val, result);
	assert(result == 500);

	printf("\n=== All early clobber tests passed! ===\n");
	return 0;
}
