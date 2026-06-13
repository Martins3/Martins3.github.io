/*
 * 示例 01: 最基本的内联汇编
 *
 * 知识点:
 * - __asm__ 或 asm 关键字用于嵌入汇编代码
 * - __volatile__ 或 volatile 防止编译器优化掉这段汇编
 * - 这是 "Basic Asm" 形式，没有输入输出操作数
 */

#include <stdio.h>
#include <unistd.h>

int main(void)
{
	/* 最基本的内联汇编: 直接写入汇编指令 */
	printf("Before asm\n");

	/* 空操作，用于演示 */
	__asm__ __volatile__("nop");

	/* 在 x86_64 上执行一个空操作 */
	__asm__ __volatile__("nop\n\t"
				     "nop");

	printf("After asm\n");

	/* 使用 syscall 指令直接调用 write (系统调用号 1)
	 * 这是 Basic Asm 形式，完全由程序员控制寄存器
	 */
	printf("Using direct syscall:\n");
	fflush(stdout);  /* 确保 printf 先输出 */
	__asm__ __volatile__(
		"movq $1, %%rax\n\t"    /* syscall number: write = 1 */
		"movq $1, %%rdi\n\t"    /* fd: stdout = 1 */
		"lea msg(%%rip), %%rsi\n\t" /* buf: message address */
		"movq $14, %%rdx\n\t"   /* count: message length */
		"syscall\n\t"
		:
		:
		: "memory", "rax", "rdi", "rsi", "rdx", "rcx", "r11");

	return 0;
}

/* 用于 basic asm 的数据 */
__asm__(
".section .rodata\n"
"msg:\n\t"
".string \"Hello, Asm!\\n\"\n"
".previous\n"
);
