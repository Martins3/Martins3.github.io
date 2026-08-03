#include <asm-generic/rwonce.h>
#include <linux/printk.h>
#include <linux/delay.h>
#include "internal.h"

/**
 * 就 while 循环判断这种，编译器非常的智能，只要认为存在其他的地方修改代码，
 * 都不会优化掉的
 */
void test1(void);
void test2(void);
void test3(void);
void test4(void);
void test5(void);
void test6(void);

/**
 * 指令合并
 */
void test7(void);
void test8(void);

/**
 * 指令重拍
 */
void test9(void);
void test10(void);
void test11(void);

/*
 *   直接被优化为死循环
 *
 *   0x0000000000000010 <+0>:     endbr64
 *   0x0000000000000014 <+4>:     call   0x19 <test1+9>
 *   0x0000000000000019 <+9>:     mov    $0x3e8,%edi
 *   0x000000000000001e <+14>:    call   0x23 <test1+19>
 *   0x0000000000000023 <+19>:    jmp    0x19 <test1+9>
 */
void test1(void)
{
	int test1 = 1;
	while (test1) {
		msleep(1000);
	}
}

/*
 * 没有优化
 *
 * 0x0000000000000250 <+0>:     endbr64
 * 0x0000000000000254 <+4>:     call   0x259 <test2+9>
 * 0x0000000000000259 <+9>:     sub    $0x8,%rsp
 * 0x000000000000025d <+13>:    movl   $0x1,0x4(%rsp)
 * 0x0000000000000265 <+21>:    mov    0x4(%rsp),%eax
 * 0x0000000000000269 <+25>:    test   %eax,%eax
 * 0x000000000000026b <+27>:    je     0x27f <test2+47>
 * 0x000000000000026d <+29>:    mov    $0x3e8,%edi
 * 0x0000000000000272 <+34>:    call   0x277 <test2+39>
 * 0x0000000000000277 <+39>:    mov    0x4(%rsp),%eax
 * 0x000000000000027b <+43>:    test   %eax,%eax
 * 0x000000000000027d <+45>:    jne    0x26d <test2+29>
 * 0x000000000000027f <+47>:    add    $0x8,%rsp
 * 0x0000000000000283 <+51>:    jmp    0x288
 */
void test2(void)
{
	int test2 = 1;
	while (READ_ONCE(test2)) {
		msleep(1000);
	}
}

// 注意，
// 1. 第一个 call 应该是 tracepoint 函数
// 2. 第二个 call 函数没有链接，其实回去调用 msleep
/*   0x0000000000000090 <+0>:     endbr64
 *   0x0000000000000094 <+4>:     call   0x99 <test3+9>
 *   0x0000000000000099 <+9>:     jmp    0xa5 <test3+21>
 *   0x000000000000009b <+11>:    mov    $0x3e8,%edi
 *   0x00000000000000a0 <+16>:    call   0xa5 <test3+21>
 *   0x00000000000000a5 <+21>:    mov    0x0(%rip),%eax        # 0xab <test3+27>
 *   0x00000000000000ab <+27>:    test   %eax,%eax
 *   0x00000000000000ad <+29>:    jne    0x9b <test3+11>
 *   0x00000000000000af <+31>:    jmp    0xb4
 */
int access_once_test3 = 1;
void test3(void)
{
	while (READ_ONCE(access_once_test3)) {
		msleep(1000);
	}
}

// ok
static int access_once_test4 = 1;
void test4(void)
{
	while (READ_ONCE(access_once_test4)) {
		msleep(1000);
	}
}

/*
 * 并没有优化，但是注意，这个这个不是必然的。
 * 这个是 volatile 使用的经典原因了。
 *
 * 0x0000000000000320 <+0>:     endbr64
 * 0x0000000000000324 <+4>:     call   0x329 <test5+9>
 * 0x0000000000000329 <+9>:     jmp    0x335 <test5+21>
 * 0x000000000000032b <+11>:    mov    $0x3e8,%edi
 * 0x0000000000000330 <+16>:    call   0x335 <test5+21>
 * 0x0000000000000335 <+21>:    mov    0x0(%rip),%eax        # 0x33b <test5+27>
 * 0x000000000000033b <+27>:    test   %eax,%eax
 * 0x000000000000033d <+29>:    jne    0x32b <test5+11>
 * 0x000000000000033f <+31>:    jmp    0x344
 *
 */
static int access_once_test5 = 1;
void test5(void)
{
	while (access_once_test5) {
		msleep(1000);
	}
}

// ok
int access_once_test6 = 1;
void test6(void)
{
	while (access_once_test6) {
		msleep(1000);
	}
}

// 会合并
int access_once_test7;
void test7(void)
{
	access_once_test7 = 1;
	access_once_test7 = 2;
	access_once_test7 = 3;
}

// 不会合并
void test8(void)
{
	WRITE_ONCE(access_once_test7, 1);
	WRITE_ONCE(access_once_test7, 2);
	WRITE_ONCE(access_once_test7, 3);
}

int access_once_test9 = 1;
int access_once_test10 = 1;

/**
 *
 * Dump of assembler code for function test9:
 *  0x00000000000001f0 <+0>:     endbr64
 *  0x00000000000001f4 <+4>:     call   0x1f9 <test9+9>
 *  0x00000000000001f9 <+9>:     movl   $0x2,0x0(%rip)        # 0x203 <test9+19>
 *  0x0000000000000203 <+19>:    movl   $0x3,0x0(%rip)        # 0x20d <test9+29>
 *  0x000000000000020d <+29>:    jmp    0x212
 **/

void test9(void)
{
	access_once_test9 = 0x2222;
	access_once_test10 = 0x2222;

	/* test_access_once(1); */
	access_once_test9 = 0x3333;
	access_once_test10 = 0x2222;
}

void test10(void)
{
	access_once_test9 = 0x2222;
	access_once_test10 = 0x2222;
	// XXX 如果想要输出 0x3333 和 0x2222 ，这两个 WRITE_ONCE 都不可以省去，
	// 因为另外一个不会省去
	WRITE_ONCE(access_once_test9, 0x3333);
	/* access_once_test10 = 0x2222; */
	WRITE_ONCE(access_once_test10, 0x2222);
}

/**
 * mb 的确可以防止指令重拍，但是 mb 和 READ_ONCE / WRITE_ONCE 是两个
 * 维度的问题，只是有时候效果非常类似。
 *
 * 0x0000000000000270 <+0>:     endbr64
 * 0x0000000000000274 <+4>:     call   0x279 <test11+9>
 * 0x0000000000000279 <+9>:     movl   $0x2222,0x0(%rip)        # 0x283 <test11+19>
 * 0x0000000000000283 <+19>:    movl   $0x2222,0x0(%rip)        # 0x28d <test11+29>
 * 0x000000000000028d <+29>:    lock addl $0x0,-0x4(%rsp)
 * 0x0000000000000293 <+35>:    movl   $0x3333,0x0(%rip)        # 0x29d <test11+45>
 * 0x000000000000029d <+45>:    movl   $0x2222,0x0(%rip)        # 0x2a7 <test11+55>
 * 0x00000000000002a7 <+55>:    jmp    0x2ac
 *
 * arm 中反汇编的结果:
 *
 * bti     c
 * adrp    x0, 0x0 <test1>
 * add     x0, x0, #0x0
 * mov     w1, #0x2222                     // #8738
 * stp     w1, w1, [x0, #12]
 * dmb     ish
 * mov     w2, #0x3333                     // #13107
 * stp     w1, w2, [x0, #12]
 * ret
 */
void test11(void)
{
	access_once_test9 = 0x2222;
	access_once_test10 = 0x2222;
	smp_mb();
	access_once_test9 = 0x3333;
	access_once_test10 = 0x2222;
}

/*
 * compiletime_assert_rwonce_type 的触发条件:
 *   __native_word(t) 为假 且 sizeof(t) != sizeof(long long)
 * __native_word 只认 char/short/int/long 的大小, x86_64 上即 1/2/4/8 字节。
 * 下面这个 24 字节结构体两头都不沾, 编译直接报错:
 *   error: call to '__compiletime_assert_174' declared with attribute error:
 *          Unsupported access size for {READ,WRITE}_ONCE().
 * 原理: _compiletime_assert 声明了一个带 __attribute__((error)) 的函数,
 * 只有条件为假时这个调用才会留在代码里, 于是编译期爆炸。
 */
struct access_once_big {
	char data[24];
};

#if 0 /* 故意触发编译错误, 平时保持关闭 */
static struct access_once_big access_once_test12;

void test12(void)
{
	READ_ONCE(access_once_test12);
}
#endif

int test_access_once(long action)
{
	switch (action) {
	case 0:
		// 增加 access_once_test5 会导致 test5 结果不同
		access_once_test5 = 0;
		break;
	case 1:
		test5();
		break;
	}
	return 0;
}
