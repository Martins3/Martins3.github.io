#include "internal.h"
#include <asm/apic.h>
#include <linux/interrupt.h>

// 重复定义一下，解决兼容性问题
// commit bf348f667ed3 ("KVM: x86: lapic: Rename [GET/SET]_APIC_DEST_FIELD to [GET/SET]_XAPIC_DEST_FIELD")
#define		GET_XAPIC_DEST_FIELD(x)	(((x) >> 24) & 0xFF)
#define		SET_XAPIC_DEST_FIELD(x)	((x) << 24)
#define		GET_APIC_DEST_FIELD(x)	(((x) >> 24) & 0xFF)
#define		SET_APIC_DEST_FIELD(x)	((x) << 24)

static inline int __prepare_ICR2(unsigned int mask)
{
	return SET_XAPIC_DEST_FIELD(mask);
}

static inline unsigned int __prepare_ICR(unsigned int shortcut, int vector,
					 unsigned int dest)
{
	unsigned int icr = shortcut | dest;

	switch (vector) {
	default:
		icr |= APIC_DM_FIXED | vector;
		break;
	case NMI_VECTOR:
		icr |= APIC_DM_NMI;
		break;
	case -1:
		icr |= APIC_DM_SMI;
		break;
	}
	return icr;
}

static void apic_mem_wait_icr_idle(void)
{
	while (native_apic_mem_read(APIC_ICR) & APIC_ICR_BUSY)
		cpu_relax();
}

static void __default_send_IPI_dest_field(unsigned int dest_mask, int vector,
					  unsigned int dest_mode)
{
	apic_mem_wait_icr_idle();
	/* Set the IPI destination field in the ICR */
	native_apic_mem_write(APIC_ICR2, __prepare_ICR2(dest_mask));
	/* Send it with the proper destination mode */
	native_apic_mem_write(APIC_ICR, __prepare_ICR(0, vector, dest_mode));
}

static void __default_send_SMI_dest_field(unsigned int dest_mask,
					  unsigned int dest_mode)
{
	apic_mem_wait_icr_idle();
	/* Set the IPI destination field in the ICR */
	native_apic_mem_write(APIC_ICR2, __prepare_ICR2(dest_mask));
	/* Send it with the proper destination mode */
	native_apic_mem_write(APIC_ICR, __prepare_ICR(0, -1, dest_mode));
}

static void default_send_IPI_single_phys(int cpu, int vector)
{
	unsigned long flags;

	local_irq_save(flags);
	__default_send_IPI_dest_field(per_cpu(x86_cpu_to_apicid, cpu), vector,
				      APIC_DEST_PHYSICAL);
	local_irq_restore(flags);
}

static void default_send_SMI_single_phys(int cpu)
{
	unsigned long flags;

	local_irq_save(flags);
	// TODO x86_cpu_to_apicid 的含义理解一下
	__default_send_SMI_dest_field(per_cpu(x86_cpu_to_apicid, cpu),
				      APIC_DEST_PHYSICAL);
	local_irq_restore(flags);
}

static int id = 1234;
static irqreturn_t i8042_interrupt(int irq, void *dev_id)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return IRQ_HANDLED;
}

// TODO 继续调查的
// 1. thread irq
// 2. 通往软中断
static void test_directly_irq(void)
{
	// 参考 kernel/irq/irqdesc.c:
	//  然后检查 /proc/interrupts ，发现 0 - 24 中没用的就是 irq 的取值
	int irq = 13;

	// TODO /sys/kernel/debug/irq/irqs/13 里面的 Vector 和 Target 都不对
	// 看看 timer 都是如何初始化的吧
	// 奇怪啊
	int error =
		request_irq(irq, i8042_interrupt, IRQF_SHARED, "martins3", &id);
	if (error)
		pr_err("request_irq failed with %d\n", error);
	// TODO 通过这个找到具体在哪一个 CPU 上，就是理解 irq domain 之类入口了
	/* struct irq_desc *desc = irq_to_desc(irq); */

	// TODO 看看有无办法测试这个函数
	/* irq_set_chip_and_handler_name() */
}

// 只有关闭 CONFIG_X86_X2APIC=y 的时候才有用
// TODO 这里也需要确认一下，这种模式，ICR 的写是不是无法被虚拟化，只有 x2apic 的那种 wrmsr 的才可以
int test_apic(long action)
{
	switch (action) {
	case 1:
		// 比想象的还好用，会得到这个，
		// 如果通过 ipi 可以实现对于 CPU 发送任何 vector 的中断，那么是不是
		// 其实就不需要 dummy virtio 来测试了。
		//
		// 而且register 一个 handler 就可以了，然后这个 handler 正好
		// 在一个 cpu 的 vector ，然后就可以测试了。
		//
		// [11168.627474] call_irq_handler: 0.129 No irq handler for vector
		default_send_IPI_single_phys(1, 49);
		break;
	case 2:
		test_directly_irq();
		break;
	case 3:
		default_send_SMI_single_phys(0);
		break;
	}
	return 0;
}
