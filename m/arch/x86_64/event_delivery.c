#include "internal.h"
#include <asm/desc.h>
#include <linux/mm.h>
#include <linux/smpboot.h>
#include <linux/delay.h>

/* TODO 并没有办法触发 event delivery ，虚拟机直接重启l
 */
#define IDT_TABLE_SIZE (IDT_ENTRIES * sizeof(gate_desc))
static int test_happening(void)
{
	struct desc_ptr *m = kmalloc(sizeof(struct desc_ptr), GFP_KERNEL);
	if (!m)
		return -ENOMEM;

	struct desc_ptr idt_descr = {
		.size = IDT_TABLE_SIZE - 1,
		.address = 0x10000000000ul,
	};

	native_load_idt(&idt_descr);

	return 0;
}

static int test1(void)
{
	// 参考这里的写法，显然不可以
	// https://lore.kernel.org/kvm/20241015195227.GA18617@dev-dsk-iorlov-1b-d2eae488.eu-west-1.amazon.com/T/#m56e76a7c726d2468765bb1dc06776456273a9bd4
#define MEM_REGION_GPA 0x10000000
	static const struct desc_ptr faulty_idt_desc = {
		.address = MEM_REGION_GPA,
		.size = 0xFFF,
	};

	__asm__ __volatile__("lidt %0" ::"m"(faulty_idt_desc));

	/* Generate a #GP by dereferencing a non-canonical address */
	/* *((uint8_t *)0xDEADBEEFDEADBEEFULL) = 0x1; */
	return 0;
}

// 参考 pci_conf1_write
// TODO 我这里写了很多 pages 之后，但是依旧没有触发 handle_ept_violation 啊
DEFINE_RAW_SPINLOCK(pci_config_lock);
static int test2(void)
{
	unsigned long flags;
	int nr = 100000;
	ktime_t time;
	// TODO kmalloc_array 的大小不可以无限大
	struct folio **folios =
		kmalloc_array(nr, sizeof(struct folio *), GFP_KERNEL);
	if (!folios)
		return -ENOMEM;

	for (size_t i = 0; i < nr; i++) {
		folios[i] = folio_alloc(GFP_USER, 1);
		// TODO 显然，需要先释放掉之前分配的
		if (!folios[i])
			return -ENOMEM;
	}

	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	time = ktime_get();
	raw_spin_lock_irqsave(&pci_config_lock, flags);
	for (size_t i = 0; i < nr; i++) {
		for (size_t i = 0; i < 100; i++) {
			int *m = (int *)folio_address(folios[i]);
			for (size_t j = 0; j < PAGE_SIZE / sizeof(int); j++) {
				m[j] = i + j;
			}
		}
	}
	raw_spin_unlock_irqrestore(&pci_config_lock, flags);
	time = ktime_get() - time;
	pr_info("[martins3:%s:%d] %lldms\n", __FUNCTION__, __LINE__,
		time / USEC_PER_SEC);
	return 0;
}

static DEFINE_PER_CPU(struct task_struct *, ordering_tasks);
static int should_run(unsigned int cpu)
{
	return true;
}

static void smp_thread(unsigned int cpu)
{
	unsigned long flags;
	for (size_t i = 0; i < 100000000000000ul; i++) {
		raw_spin_lock_irqsave(&pci_config_lock, flags);
#define HV_X64_MSR_EOI (0x40000070 + 4)
		wrmsr(HV_X64_MSR_EOI, APIC_EOI_ACK, 124);
		raw_spin_unlock_irqrestore(&pci_config_lock, flags);
		if (i % 1000 == 0) {
			msleep(54);
		}
	}
}

static struct smp_hotplug_thread ordering_smp_thread = {
	.store = &ordering_tasks,
	.thread_should_run = should_run,
	.thread_fn = smp_thread,
	.thread_comm = "test/%u",
};

int test_event_delivery(long action)
{
	switch (action) {
	case 0:
		test_happening();
		break;
	case 1:
		test1();
		break;
	case 2:
		test2();
		break;
	case 3:
		BUG_ON(smpboot_register_percpu_thread(&ordering_smp_thread));
		break;
	}
	return 0;
}
