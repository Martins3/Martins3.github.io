#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

int a;
int main(int argc, char *argv[])
{
	a = 1;
	a = 2;
	a = 3;
	asm volatile("" : : : "memory");
	a = 4;
	a = 5;
	putchar(a);
	/*
	 * 如果将 a 声明为局部变量， 无论是否有 asm volatile ，都是一样的
	 * 而如果将 a 声明为全局变量，效果会不同的。
	 *
	 * stp     x29, x30, [sp, #-16]!
         * mov     x29, sp
         * adrp    x8, 0x440000 <__libc_start_main@got.plt>
         * mov     w0, #0x5                        // #5
         * ldr     x1, [x8, #40]
         * bl      0x410050 <putc@plt>
         * mov     w0, wzr
         * ldp     x29, x30, [sp], #16
         * ret
	 */
	return EXIT_SUCCESS;
}
