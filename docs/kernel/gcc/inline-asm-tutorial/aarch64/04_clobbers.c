/*
 * 示例 04: Clobbers (修改列表) (ARM64/AArch64)
 *
 * 知识点:
 * - Clobbers 告诉编译器哪些寄存器或内存被汇编代码修改了
 * - "memory": 表示汇编代码可能读写任意内存位置
 * - "cc": 表示条件码/标志位被修改
 * - x0-x18 调用者保存 (caller-saved)
 * - x19-x28 被调用者保存 (callee-saved)
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void)
{
	/* ===== 寄存器 Clobber ===== */
	int input = 10;
	int output;

	/* 汇编代码隐式修改了 x9 和 x10，但没有将它们列为输出
	 * 必须在 clobber 列表中声明
	 */
	__asm__ __volatile__(
		"mov w9, %w1\n\t"
		"mov w10, #5\n\t"
		"madd %w0, w9, w10, w9"  /* output = input * 5 + input = 6*input */
		: "=r" (output)
		: "r" (input)
		: "x9", "x10"           /* 声明我们修改了 x9 和 x10 */
		);

	printf("6 * %d = %d (expected: 60)\n", input, output);
	assert(output == 60);

	/* ===== 条件码 Clobber ===== */
	int a = 5, b = 3;
	int max_val;

	/* 比较操作会修改条件码 */
	__asm__ __volatile__(
		"cmp %w1, %w2\n\t"      /* 比较 a 和 b */
		"csel %w0, %w1, %w2, gt" /* 如果 a > b，output = a，否则 b */
		: "=r" (max_val)
		: "r" (a), "r" (b)
		: "cc"                   /* 声明修改了条件码 */
		);

	printf("max(%d, %d) = %d (expected: 5)\n", a, b, max_val);
	assert(max_val == 5);

	/* ===== Memory Clobber ===== */
	int mem1 = 100;
	int mem2 = 200;

	/* 使用 "memory" clobber 确保内存一致性
	 * 这会阻止编译器乱序优化内存访问
	 */
	__asm__ __volatile__(
		""                      /* 空汇编模板 */
		:
		:
		: "memory"              /* 声明可能修改任意内存 */
		);

	printf("mem1 = %d, mem2 = %d\n", mem1, mem2);

	/* ===== 实际使用场景: 内存屏障 ===== */
	int flag = 0;
	int data = 0;

	/* 写入数据 */
	data = 42;

	/* 编译器屏障: 防止编译器重排序 */
	__asm__ __volatile__("" ::: "memory");

	/* 设置标志 */
	flag = 1;

	printf("data = %d, flag = %d\n", data, flag);
	assert(data == 42 && flag == 1);

	/* ===== 复杂 Clobber 示例: 除法 ===== */
	long dividend = 1000;
	long divisor = 7;
	long quotient, remainder;

	/* ARM64 使用 sdiv 和 msub 实现除法和取余 */
	__asm__ __volatile__(
		"sdiv %0, %2, %3\n\t"   /* quotient = dividend / divisor */
		"msub %1, %0, %3, %2"   /* remainder = dividend - quotient * divisor */
		: "=&r" (quotient), "=r" (remainder)
		: "r" (dividend), "r" (divisor)
		);

	printf("%ld / %ld = %ld, remainder = %ld\n",
	       dividend, divisor, quotient, remainder);
	assert(quotient == 142 && remainder == 6);

	/* ===== ARM64 系统调用 clobber 示例 ===== */
	/* ARM64 syscall 约定:
	 * 输入: x8=syscall_no, x0-x5=arg1-6
	 * 输出: x0=return value
	 * Clobbers: x8 (系统调用号), 条件码
	 */
	long sys_result;
	const char msg[] = "Hello from ARM64 syscall\n";
	size_t len = strlen(msg);

	__asm__ __volatile__(
		"mov x8, #64\n\t"       /* syscall: write = 64 */
		"svc #0"
		: "=r" (sys_result)
		: "r" ((long)1),        /* fd: stdout = 1 (x0) */
		  "r" (msg),            /* buf (x1) */
		  "r" (len)             /* count (x2) */
		: "x8", "memory"
		);

	printf("syscall returned: %ld\n", sys_result);
	assert(sys_result == (long)len);

	printf("\n=== All clobber tests passed! ===\n");
	return 0;
}
