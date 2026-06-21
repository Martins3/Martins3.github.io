#include <stdio.h>

void print_number(long a)
{
	printf("print_number %lx\n", a);
}

void print_double(double a)
{
	printf("print_double %lf\n", a);
}

void print_hex(long a)
{
	printf("print_hex 0x%lx\n", a);
}

/*
 * sub     sp, sp, #0x10
 * str     w0, [sp, #12]
 * str     w1, [sp, #8]
 * ldr     w1, [sp, #12]
 * ldr     w0, [sp, #8]
 * add     w0, w1, w0
 * add     sp, sp, #0x10
 * ret
 */
int add(int a, int b)
{
	return a + b;
}

/*
 * stp     x29, x30, [sp, #-32]!
 * mov     x29, sp
 * str     w0, [sp, #28]
 * str     w1, [sp, #24]
 * ldr     w1, [sp, #24]
 * ldr     w0, [sp, #28]
 * bl      0x410214 <add>
 * ldp     x29, x30, [sp], #32
 * ret
 */
int non_leaf_add(int a, int b)
{
	return add(a, b);
}
