#include "internal.h"
#include <asm/cpu.h>
#include <asm/tsc.h>
#include <linux/kernel.h>
#include <linux/module.h>

static void tsx_disable(void)
{
	u64 tsx;

	rdmsrl(MSR_IA32_TSX_CTRL, tsx);

	/* Force all transactions to immediately abort */
	tsx |= TSX_CTRL_RTM_DISABLE;

	/*
	 * Ensure TSX support is not enumerated in CPUID.
	 * This is visible to userspace and will ensure they
	 * do not waste resources trying TSX transactions that
	 * will always abort.
	 */
	tsx |= TSX_CTRL_CPUID_CLEAR;

	wrmsrl(MSR_IA32_TSX_CTRL, tsx);
}

static int vermagic_init(void)
{
	pr_info("[martins3:%s:%d] disbale tsx\n", __FUNCTION__, __LINE__);
	tsx_disable();
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static void aperfmperf_get_khz(int cpu)
{
	// @todo 测试一下这个代码
	/* smp_call_function_single(cpu, aperfmperf_snapshot_khz, NULL, wait); */
}

int test_tsx(long action)
{
	switch (action) {
	case 0:
		aperfmperf_get_khz(0);
		vermagic_init();
		break;
	}
	return 0;
}
