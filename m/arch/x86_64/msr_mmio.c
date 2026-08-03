#include "internal.h"
#include <hyperv/hvgdk_mini.h>

/*
 * 似乎 msr 不可以随便读，随便写。似乎 msr 的访问有一大堆 api 啊
 *
 * mmio 的空间也测试下吧，哪些地方一定是 mmio ，virtio driver 是如何访问的， 访问一下非对齐的 mmio 是不是有特殊的效果
 */

raw_spinlock_t lock;
int test_msr_mmio(long action)
{
	unsigned long flags;
	ktime_t time;
	switch (action) {
	case 1:
		/*
		 * QEMU 需要 -cpu host,hv_apic 才可以
		 *
		 * 循环一千次，这样在
		 * 1. perf top -e kvm:msr
		 * 2. sudo perf top -e msr:write_msr
		 * 中可以看到
		 */
		for (size_t i = 0; i < 1000; i++)
			wrmsr(HV_X64_MSR_EOI, 1, 0);
		break;
	case 2:
		/*
		 * HV_X64_MSR_VP_ASSIST_PAGE 是不存在的，可以触发 exception 的。
		 *
		 * [ 9392.420062] unchecked MSR access error: WRMSR to 0x40000074 (tried to write 0x0000000000000001) at rIP: 0xffffffffc020b7a5 (test_msr_mmio+0x55/0x80 [martins3])
		 * [ 9392.420778] Call Trace:
		 * [ 9392.420948]  <TASK>
		 * [ 9392.421091]  ? fixup_exception+0x27d/0x5a0
		 * [ 9392.421313]  ? irqentry_enter+0x31/0x60
		 * [ 9392.421527]  ? irqentry_exit+0x3b/0x50
		 * [ 9392.421734]  ? exc_general_protection+0x1bb/0x5d0
		 * [ 9392.421999]  ? _prb_read_valid+0x1e6/0x520
		 * [ 9392.422223]  ? console_flush_all+0x37b/0x3e0
		 * [ 9392.422454]  ? vprintk_emit+0x188/0x2e0
		 * [ 9392.422664]  ? asm_exc_general_protection+0x26/0x30
		 * [ 9392.422931]  ? vprintk_emit+0x188/0x2e0
		 * [ 9392.423148]  ? test_msr_mmio+0x55/0x80 [martins3]
		 * [ 9392.423398]  msr_mmio_store+0xa0/0xd0 [martins3]
		 * [ 9392.423645]  kernfs_fop_write_iter+0xf0/0x170
		 * [ 9392.423897]  vfs_write+0x37c/0x460
		 * [ 9392.424096]  ksys_write+0x72/0xe0
		 * [ 9392.424286]  do_syscall_64+0xed/0x210
		 * [ 9392.424487]  ? exc_page_fault+0xb2/0x1f0
		 * [ 9392.424701]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
		 * [ 9392.424981] RIP: 0033:0x7f13e9f989b4
		 */
		wrmsr(HV_X64_MSR_VP_ASSIST_PAGE + 1, 1, 0);
		break;
	case 3:
		raw_spin_lock_init(&lock);
		time = ktime_get();
		pr_info("[martins3:%s:%d] %lld\n", __FUNCTION__, __LINE__, time);
		raw_spin_lock_irqsave(&lock, flags);
		for (size_t i = 0; i < 10000000; i++)
			wrmsr(HV_X64_MSR_EOI, APIC_EOI_ACK, 0);
		raw_spin_unlock_irqrestore(&lock, flags);
		time = ktime_get();
		pr_info("[martins3:%s:%d] %lld\n", __FUNCTION__, __LINE__, time);
		break;
	}

	return 0;
}
