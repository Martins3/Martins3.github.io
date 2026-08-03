#include "internal.h"
#include <linux/irq_work.h>
#include <linux/smpboot.h>
#include <linux/delay.h>
/**

 1. CALL_FUNCTION_VECTOR : native_send_call_func_ipi 发送一个
 2. CALL_FUNCTION_SINGLE_VECTOR : native_send_call_func_single_ipi 发送给一群

经典 backtrace :
@[
    native_send_call_func_single_ipi+5
    ttwu_queue_wakelist+239
    try_to_wake_up+686
    wake_up_q+78
    futex_wake+345
    do_futex+239
    __x64_sys_futex+146
    do_syscall_64+67
    entry_SYSCALL_64_after_hwframe+111
]: 1139

外部接口: send_call_function_single_ipi

经典 backtrace :
@[
    native_send_call_func_ipi+5
    smp_call_function_many_cond+835
    on_each_cpu_cond_mask+64
    flush_tlb_mm_range+348
    tlb_finish_mmu+231
    zap_page_range_single+319
    do_madvise+3328
    __x64_sys_madvise+44
    do_syscall_64+67
    entry_SYSCALL_64_after_hwframe+111
]: 44

外部接口: smp_call_function_many_cond 等

 */

static void handler(void *data)
{
	// 原来这个 handler 是一个硬中断
	/*
	 * [82573.850444] Call Trace:
	 * [82573.850519]  <IRQ>
	 * [82573.850581]  dump_stack_lvl+0x86/0xc0
	 * [82573.850688]  ? __pfx_handler+0x10/0x10 [martins3]
	 * [82573.850826]  handler+0x19/0x30 [martins3]
	 * [82573.850944]  __flush_smp_call_function_queue+0xb7/0x400
	 * [82573.851108]  __sysvec_call_function_single+0x1c/0xc0
	 * [82573.851251]  sysvec_call_function_single+0x6e/0x80
	 * [82573.851396]  </IRQ>
	 * [82573.851459]  <TASK>
	 * [82573.851522]  asm_sysvec_call_function_single+0x1a/0x20
	 */
	if (smp_processor_id() == 0) {
		dump_stack();
		pr_info("in_softirq=%ld\n", in_softirq());
		pr_info("in_interrupt=%ld\n", in_interrupt());
	}
	int *sender = (int *)data;
	pr_info("%d get message from %d\n", smp_processor_id(), *sender);
}

int test_ipi(long action)
{
	unsigned cpu = action;
	// XXX : 获取到了 num_online_cpus 获取了数值之后，结果 CPU 被拔掉了，怎么办
	if (cpu >= num_online_cpus())
		return -EINVAL;

	int sender = smp_processor_id();
	smp_call_function_single(cpu, handler, &sender, 1);
	return 0;
}
