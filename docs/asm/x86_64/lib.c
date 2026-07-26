#include <stdio.h>
#include <stdlib.h>

void print_number(long a)
{
	printf("print_number %lx\n", a);
}

void print_double(double a)
{
	printf("print_double %lf\n", a);
}

/*
 * Dump of assembler code for function add:
 *    0x00000000000011b0 <+0>:     push   %rbp
 *    0x00000000000011b1 <+1>:     mov    %rsp,%rbp
 *    0x00000000000011b4 <+4>:     mov    %edi,-0x4(%rbp)
 *    0x00000000000011b7 <+7>:     mov    %esi,-0x8(%rbp)
 *    0x00000000000011ba <+10>:    mov    -0x4(%rbp),%eax
 *    0x00000000000011bd <+13>:    add    -0x8(%rbp),%eax
 *    0x00000000000011c0 <+16>:    pop    %rbp
 *    0x00000000000011c1 <+17>:    ret
 * End of assembler dump.
 */
int add(int a, int b)
{
	return a + b;
}

/*
 * Dump of assembler code for function non_leaf_add:
 *    0x00000000000011d0 <+0>:     push   %rbp
 *    0x00000000000011d1 <+1>:     mov    %rsp,%rbp
 *    0x00000000000011d4 <+4>:     sub    $0x10,%rsp
 *    0x00000000000011d8 <+8>:     mov    %edi,-0x4(%rbp)
 *    0x00000000000011db <+11>:    mov    %esi,-0x8(%rbp)
 *    0x00000000000011de <+14>:    mov    -0x4(%rbp),%edi
 *    0x00000000000011e1 <+17>:    mov    -0x8(%rbp),%esi
 *    0x00000000000011e4 <+20>:    call   0x11b0 <add>
 *    0x00000000000011e9 <+25>:    add    $0x10,%rsp
 *    0x00000000000011ed <+29>:    pop    %rbp
 *    0x00000000000011ee <+30>:    ret
 */
int non_leaf_add(int a, int b)
{
	return add(a, b);
}

int loop(int a)
{
	for (size_t i = 0; i < a; i++) {
		printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	}
	return 0;
}

/*
 * Dump of assembler code for function write_to:
 * 0x0000000000401260 <+0>:     push   %rbp
 * 0x0000000000401261 <+1>:     mov    %rsp,%rbp
 * 0x0000000000401264 <+4>:     mov    %rdi,-0x8(%rbp)
 * 0x0000000000401268 <+8>:     mov    -0x8(%rbp),%rax
 * 0x000000000040126c <+12>:    movq   $0xc,(%rax)
 * 0x0000000000401273 <+19>:    xor    %eax,%eax
 * 0x0000000000401275 <+21>:    pop    %rbp
 * 0x0000000000401276 <+22>:    ret
 */
int write_to(long *a)
{
	*a = 12;
	return 0;
}
