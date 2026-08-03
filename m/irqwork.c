#include "internal.h"
#include <linux/irq_work.h>

static struct irq_work test_work;

static void wakeup_readers(struct irq_work *work)
{
	/*
	 * 整个 stacktrace 就是这个效果，queue 完之后立刻开始触发中断
         * [   65.358767] Call Trace:
         * [   65.358767]  <IRQ>
         * [   65.358767]  dump_stack_lvl+0x66/0xa0
         * [   65.358767]  wakeup_readers+0xe/0x30 [martins3]
         * [   65.358767]  irq_work_single+0x6b/0x90
         * [   65.358767]  irq_work_run_list+0x26/0x40
         * [   65.358767]  irq_work_run+0x18/0x30
         * [   65.358767]  __sysvec_irq_work+0x1c/0xc0
         * [   65.358767]  sysvec_irq_work+0x6e/0x80
         * [   65.358767]  </IRQ>
         *
         * [   65.358767]  <TASK>
         * [   65.358767]  asm_sysvec_irq_work+0x1a/0x20
         * [   65.358767]  arch_irq_work_raise+0x24/0x30
         * [   65.358767]  irq_work_queue+0x2f/0x50
         * [   65.358767]  test_irqwork+0x71/0x80 [martins3]
         * [   65.358767]  irqwork_store+0x6e/0xa0 [martins3]
         * [   65.358767]  kernfs_fop_write_iter+0x10c/0x1f0
         * [   65.358767]  vfs_write+0x2a3/0x480
         * [   65.358767]  ksys_write+0x6f/0xf0
         * [   65.358767]  do_syscall_64+0xc1/0x210
         * [   65.358767]  entry_SYSCALL_64_after_hwframe+0x6d/0x75
         * [   65.358767]  </TASK>
	 */
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
}

static int init;
int test_irqwork(long action)
{
	if (!init) {
		init = 1;
		init_irq_work(&test_work, wakeup_readers);
	}
	switch (action) {
	case 0:
		irq_work_queue(&test_work);
		break;
	case 1:
		irq_work_sync(&test_work);
	}

	return 0;
}
