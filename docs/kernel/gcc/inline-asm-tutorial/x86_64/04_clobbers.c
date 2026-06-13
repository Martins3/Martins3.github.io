/*
 * 示例 04: Clobbers (修改列表)
 *
 * 知识点:
 * - Clobbers 告诉编译器哪些寄存器或内存被汇编代码修改了
 * - "memory": 表示汇编代码可能读写任意内存位置
 * - "cc": 表示条件码/标志位被修改
 * - 特定寄存器: "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "r8"-"r15"
 * - "r10", "r11" 在 x86_64 syscall 中会被内核修改
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(void)
{
	/* ===== 寄存器 Clobber ===== */
	int input = 10;
	int output;

	/* 汇编代码隐式修改了 rax 和 rdx，但没有将它们列为输出
	 * 必须在 clobber 列表中声明
	 */
	__asm__ __volatile__(
		"movl %1, %%eax\n\t"
		"xorl %%edx, %%edx\n\t"  /* rdx = 0 */
		"leal (%%rax,%%rax,4), %0" /* output = rax + rax*4 = 5*input */
		: "=r" (output)
		: "r" (input)
		: "rax", "rdx"           /* 声明我们修改了 rax 和 rdx */
		);

	printf("5 * %d = %d (expected: 50)\n", input, output);
	assert(output == 50);

	/* ===== 条件码 Clobber ===== */
	int a = 5, b = 3;
	int max_val;

	/* 比较操作会修改条件码 (eflags) */
	__asm__ __volatile__(
		"cmpl %2, %1\n\t"       /* 比较 a 和 b */
		"cmovgl %1, %0\n\t"     /* 如果 a > b，output = a */
		"cmovlel %2, %0"        /* 否则 output = b */
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

	/* 在这个例子中，memory clobber 确保前面的赋值完成
	 * 后才继续执行后面的代码
	 */
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
	long divisor = 3;
	long quotient, remainder;

	/* idiv 指令: rax = rdx:rax / divisor
	 * 商在 rax，余数在 rdx
	 */
	__asm__ __volatile__(
		"movq %2, %%rax\n\t"
		"cqto\n\t"              /* 符号扩展到 rdx:rax */
		"idivq %3"              /* rax = rdx:rax / divisor */
		: "=a" (quotient), "=d" (remainder)
		: "r" (dividend), "r" (divisor)
		: "cc"
		);

	printf("%ld / %ld = %ld, remainder = %ld\n",
	       dividend, divisor, quotient, remainder);
	assert(quotient == 333 && remainder == 1);

	/* ===== x86_64 系统调用 clobber 示例 ===== */
	/* x86_64 syscall 约定:
	 * 输入: rax=syscall_no, rdi,rsi,rdx,r10,r8,r9=arg1-6
	 * 输出: rax=return value
	 * Clobbers: rcx, r11 (内核使用它们保存 rip 和 rflags)
	 */
	long sys_result;
	const char msg[] = "Hello from syscall\n";
	size_t len = strlen(msg);

	__asm__ __volatile__(
		"syscall"
		: "=a" (sys_result)
		: "a" (1),              /* syscall: write = 1 */
		  "D" ((long)1),        /* fd: stdout = 1 (rdi) */
		  "S" (msg),            /* buf (rsi) */
		  "d" (len)             /* count (rdx) */
		: "rcx", "r11", "memory" /* syscall 修改这些 */
		);

	printf("syscall returned: %ld\n", sys_result);
	assert(sys_result == (long)len);

	printf("\n=== All clobber tests passed! ===\n");
	return 0;
}
