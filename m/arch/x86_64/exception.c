#include "internal.h"

/*
 * https://os.phil-opp.com/double-fault-exceptions/
 *
 * 终于做出来了这个实验
 * 1. 内核中显然可以触发 pagefault 的
 * 2. 但是在 pagefault 后，不可以继续触发 page fault ，将会以 double fault 的形式报告。
 *
 * 或者说，当我们在内核中发现了 double fault ，基本上都是内存污染导致的
 */
static void __attribute__((noreturn)) cause_double_fault(void)
{
	unsigned long invalid_stack = 0x12345678; // 无效地址，无映射

	printk(KERN_INFO
	       "Setting RSP to invalid address and triggering divide-by-zero...\n");

	/*
	 * 关键：将 RSP 切换到一个无效地址，然后执行一个会触发异常的指令（如除零）
	 * 当除零发生时，CPU 试图压栈（RIP, RFLAGS, etc.）到 RSP 指向的位置，
	 * 但该地址无效 → 触发 #PF；
	 * 然后 CPU 尝试调用 #PF 处理程序，但压栈时再次访问无效栈 → 又 #PF，
	 * 此时无法处理 → 触发 #DF (double fault)
         */
	asm volatile("movq %0, %%rsp\n\t"
		     "xorl %%eax, %%eax\n\t"
		     "divl %%eax\n\t"
		     :
		     : "r"(invalid_stack)
		     : "rax" // <-- 移除了 "rsp"
	);
	__builtin_unreachable();
}
STACK_FRAME_NON_STANDARD(cause_double_fault);

int test_exception(long action)
{
	switch (action) {
	case 1:
		/*
		 * 导致内核 panic :
		 * Oops: invalid opcode: 0000 [#1] SMP NOPTI
		 */
		pr_info("%lx\n", 1 / (action - 7));
		break;
	case 2:
		/*
		 * traps: PANIC: double fault, error_code: 0x0
		 * Oops: double fault: 0000 [#1] SMP NOPTI
		 */
		cause_double_fault();
		break;
	case 3:
		/*
		 * hrtimer 的 callback 中 get_user_page 来访问用户态地址，从而实现中断中触发
		 */
		break;
	}
	return 0;
}
