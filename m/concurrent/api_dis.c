#include "internal.h"
#include <asm-generic/barrier.h>

/*
 * atomic 的 api 和 c++ 的定义很像
 * include/linux/atomic/atomic-instrumented.h
 *
 * 和 core/vn/code/module/concurrent/cpp/mm-dis.cpp 对比分析
 *
 * 其实还是，需要仔细看看:
 * https://www.kernel.org/doc/html/latest/core-api/wrappers/atomic_t.html
 */

// 1. atomic_cmpxchg 和 cmpxchg 是什么关系?
static int m;
static void test_cmp(int n)
{
	try_cmpxchg(&m, &n, 0);
	try_cmpxchg_relaxed(&m, &n, 0);
	try_cmpxchg_release(&m, &n, 0);
	try_cmpxchg_acquire(&m, &n, 0);

	cmpxchg(&m, n, 0);
	cmpxchg_relaxed(&m, n, 0);
	cmpxchg_release(&m, n, 0);
	cmpxchg_acquire(&m, n, 0);

	xchg(&m, n);
	xchg_relaxed(&m, n);
	xchg_acquire(&m, n);
	xchg_relaxed(&m, n);
	/*
	 * TODO 我靠，这个 x86 也需要分析一下:
	 * <+0>:     call   0x55 <test_cmp+5>
   	 * <+5>:     xor    %ecx,%ecx
   	 * <+7>:     mov    %edi,%eax
   	 * <+9>:     lock cmpxchg %ecx,0x0(%rip)        # 0x61 <test_cmp+17>
   	 * <+17>:    jne    0xd7 <test_cmp+135>
   	 * <+19>:    mov    %edi,%eax
   	 * <+21>:    lock cmpxchg %ecx,0x0(%rip)        # 0x6d <test_cmp+29>
   	 * <+29>:    jne    0xdb <test_cmp+139>
   	 * <+31>:    xor    %ecx,%ecx
   	 * <+33>:    mov    %edi,%eax
   	 * <+35>:    lock cmpxchg %ecx,0x0(%rip)        # 0x7b <test_cmp+43>
   	 * <+43>:    jne    0xdf <test_cmp+143>
   	 * <+45>:    mov    %edi,%eax
   	 * <+47>:    lock cmpxchg %ecx,0x0(%rip)        # 0x87 <test_cmp+55>
   	 * <+55>:    jne    0xe3 <test_cmp+147>
   	 * <+57>:    xor    %ecx,%ecx
   	 * <+59>:    mov    %edi,%eax
	 *
   	 * <+61>:    lock cmpxchg %ecx,0x0(%rip)        # 0x95 <test_cmp+69>
   	 * <+69>:    mov    %edi,%eax
   	 * <+71>:    lock cmpxchg %ecx,0x0(%rip)        # 0x9f <test_cmp+79>
   	 * <+79>:    mov    %edi,%eax
   	 * <+81>:    lock cmpxchg %ecx,0x0(%rip)        # 0xa9 <test_cmp+89>
   	 * <+89>:    mov    %edi,%eax
   	 * <+91>:    lock cmpxchg %ecx,0x0(%rip)        # 0xb3 <test_cmp+99>
   	 * <+99>:    mov    %edi,%eax
	 *
   	 * <+101>:   xchg   %eax,0x0(%rip)        # 0xbb <test_cmp+107>
   	 * <+107>:   mov    %edi,%eax
   	 * <+109>:   xchg   %eax,0x0(%rip)        # 0xc3 <test_cmp+115>
   	 * <+115>:   mov    %edi,%eax
   	 * <+117>:   xchg   %eax,0x0(%rip)        # 0xcb <test_cmp+123>
   	 * <+123>:   xchg   %edi,0x0(%rip)        # 0xd1 <test_cmp+129>
   	 * <+129>:   cs jmp 0xd7 <test_cmp+135>
   	 * <+135>:   mov    %eax,%edi
   	 * <+137>:   jmp    0x63 <test_cmp+19>
   	 * <+139>:   mov    %eax,%edi
   	 * <+141>:   jmp    0x6f <test_cmp+31>
   	 * <+143>:   mov    %eax,%edi
   	 * <+145>:   jmp    0x7d <test_cmp+45>
   	 * <+147>:   mov    %eax,%edi
   	 * <+149>:   jmp    0x89 <test_cmp+57>
	 */
}

/*
 * 主要是分析: include/asm-generic/barrier.h
 * 其实这里的 API 并不多，就是 memory 有点奇葩
 */
noinline static void test_barrier(int n)
{
	int m;
	smp_store_mb(m, n);
	/*
	 * xchg   %eax,0x0(%rip)        # 0xdd <test_barrier+13>
	*/
}

noinline static void test_barrier2(void)
{
	smp_rmb();
	smp_wmb();
	smp_mb();
	/*
	 * x86 :
	 * 如果是 smp_mb() ，那么翻译为:
	 * lock addl $0x0,-0x4(%rsp)
	 * smp_rmb() 和 smp_wmb() 没有任何内容。
	 *
	 * 这个测试和 dma_mb() dma_wmb() dma_rmb() 类似
	 *
	 * arm :
	 * dmb     ishld
	 * dmb     ishst
	 * dmb     ish
	 */
}

noinline static void test_barrier3(void)
{
	rmb();
	wmb();
	mb();

	/*
	 * x86 结果:
	 *
	 * 但是连续的 smp_rmb 和 rmb 会合并到一起
	 * mfence
	 * lfence
	 * sfence
	 *
	 * arm 结果:
	 *
	 * dsb     ld
	 * dsb     st
   	 * dsb     sy
	 */
}

noinline static void test_barrier4(void)
{
	smp_mb__before_atomic();
	smp_mb__after_atomic();
	/*
	 * x86 什么都没有，直接被优化掉了
	 *
	 * 
	 * arm :
	 * 0x0000000000000078 <+8>:     dmb     ish
	 * 0x000000000000007c <+12>:    dmb     ish
	 */
}

noinline static void test_barrier5(void)
{
	dma_wmb();
	dma_rmb();
	dma_mb();

	/*
	 * x86: 
	 * dma_mb 被翻译为 mfence ，
	 * dma_wmb 和 dma_rmb 不会有任何内容
	 * 但是需要注意的，如果只有 dma_wmb ，test_barrier5 函数不会消失哦
	 * test_barrier4 真的会消失
	 *
	 * arm :
	 * 
	 * 可以看到 dma_wmb() 和 wmb() 不是相同的:
	 *    dmb     oshst
	 *    dmb     oshld
   	 *    dmb     osh
	 *
	 * ish  : Inner Shareable Domain
	 * osh 	: Outer Shareable
	 * SY   : Full System
	 * NSH  : Non-shareable (仅当前 CPU 核心)
	 */
}

noinline static int test_barrier7(int n)
{
	int m;
	smp_store_release(&m, n);
	return smp_load_acquire(&m);
	/*
	 *
	 * x86 中:
	 * 0x00000000000000e9 <+9>:     mov    %edi,0x4(%rsp)
	 * 0x00000000000000ed <+13>:    mov    0x4(%rsp),%eax
	 *
	 *
	 * */
}

noinline static void test_barrier8(void)
{
	/*
	 * 只有有 barrier ，那么就不会彻底消失掉
	 * 
	 * 0x00000000000000e0 <+0>:     call   0xe5 <test_barrier8+5>
	 * 0x00000000000000e5 <+5>:     jmp    0xea
	 *
	 * arm :
	 * 0x00000000000000b0 <+0>:     nop
	 * 0x00000000000000b4 <+4>:     nop
	 * 0x00000000000000b8 <+8>:     ret
	 */
	barrier();
}

noinline static void test_barrier9(void)
{
	/*
	 *
	 * arm:
	 *
	 * 似乎和想象的不完全一样
	 * 1. 为什么会有 msr 啊
	 *	0x00000000000000c8 <+0>:     nop
	 * 	0x00000000000000cc <+4>:     nop
	 * 	0x00000000000000d0 <+8>:     mrs     x1, sp_el0
	 * 	0x00000000000000d4 <+12>:    ldr     w0, [x1, #8]
	 * 	0x00000000000000d8 <+16>:    add     w0, w0, #0x1
	 * 	0x00000000000000dc <+20>:    str     w0, [x1, #8]
	 * 	0x00000000000000e0 <+24>:    ret
	 */
	preempt_disable();
}

// 测试方法:
// m && gdb -quiet concurrent/api_dis.o -ex "disass test_barrier" -ex "q"
//
// 1. 什么情况必须使用 smp_mb 的，为什么需要 lock ?
// 2. smp_store_md 但是为什么没有 smp_load_mb
int test_api_dis(long action)
{
	test_cmp(action);
	test_barrier(action);
	test_barrier2();
	test_barrier3();
	test_barrier4();
	test_barrier5();
	test_barrier7(action);
	test_barrier8();
	test_barrier9();
	return 0;
}
