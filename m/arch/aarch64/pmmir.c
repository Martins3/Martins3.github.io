#include "asm/arm_pmuv3.h"
#include "internal.h"
#include <linux/version.h>

/*
 * arch/arm64/kernel/perf_event.c:__armv8pmu_probe_pmu 的简化测试
 */

static void test_armv8pmu_probe_pmu(void)
{
	u64 pmmir;
	int pmuver;
	int is;
	pr_info("1\n");
	pmuver = read_pmuver();
	pr_info("2\n");
	/*
	 * 只是在虚拟机中触发了这个错误:
	 *  Internal error: Oops - Undefined instruction: 0000000002000000 [#1]  SMP
	 * 但是 kvm 中没有日志，物理机内核 4.19 2112
	 */
	pmmir = 0;
	/*
	 * read_pmmir()
	 */
	pr_info("3\n");
	is = is_pmuv3p4(pmuver);
	pr_info("4\n");
	pr_info("pmuver=%d v3p4=%d pmmir=%llx\n", pmuver, is, pmmir);
}

int test_pmmir(long action)
{
	switch (action) {
	case 0:
		test_armv8pmu_probe_pmu();
		break;
	}
	return 0;
}
