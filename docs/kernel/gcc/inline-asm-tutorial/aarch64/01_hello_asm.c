/*
 * 示例 01: 最基本的内联汇编 (ARM64/AArch64)
 *
 * 知识点:
 * - __asm__ 或 asm 关键字用于嵌入汇编代码
 * - __volatile__ 或 volatile 防止编译器优化掉这段汇编
 * - 这是 "Basic Asm" 形式，没有输入输出操作数
 * - ARM64 使用 x0-x30 寄存器 (64-bit) 或 w0-w30 (32-bit)
 */

#include <stdio.h>
#include <unistd.h>

int main(void)
{
	/* 最基本的内联汇编: 直接写入汇编指令 */
	printf("Before asm\n");

	/* 空操作，用于演示 (ARM64 使用 nop) */
	__asm__ __volatile__("nop");

	/* 在 ARM64 上执行两个空操作 */
	__asm__ __volatile__("nop\n\t"
				     "nop");

	printf("After asm\n");

	/* 使用 svc 指令直接调用 write (系统调用号 64)
	 * ARM64 Linux syscall 约定:
	 *   x8 = syscall number
	 *   x0-x5 = arguments
	 *   svc #0 = 触发系统调用
	 */
	printf("Using direct syscall:\n");
	fflush(stdout);

	const char msg[] = "Hello, ARM64 Asm!\n";

	__asm__ __volatile__(
		"mov x8, #64\n\t"       /* syscall number: write = 64 */
		"mov x0, #1\n\t"        /* fd: stdout = 1 */
		"mov x1, %0\n\t"        /* buf: message address */
		"mov x2, %1\n\t"        /* count: message length */
		"svc #0\n\t"
		:
		: "r" (msg), "r" ((long)(sizeof(msg) - 1))
		: "x0", "x1", "x2", "x8", "memory"
		);

	return 0;
}
