#include "internal.h"
#include <asm/apic.h>


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

static void __x2apic_send_SMI_dest(unsigned int apicid, unsigned int dest)
{
	unsigned long cfg = __prepare_ICR(0, -1, dest);
	native_x2apic_icr_write(cfg, apicid);
}

static void x2apic_send_SMI(int cpu)
{
	u32 dest = per_cpu(x86_cpu_to_apicid, cpu);

	/* x2apic MSRs are special and need a special fence: */
	weak_wrmsr_fence();
	__x2apic_send_SMI_dest(dest, APIC_DEST_PHYSICAL);
}


int test_x2apic(long action)
{
	switch (action) {
	case 0:
		x2apic_send_SMI(0);
		break;
	}
	return 0;
}
