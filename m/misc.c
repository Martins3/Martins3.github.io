#include "internal.h"
#include <linux/ratelimit.h>
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/cpuidle.h>
#include <linux/sched/debug.h>
#include <asm/irq_regs.h>

/*
 * TODO 测试下 net ratelimit ，为什么 net 需要自己的 ratelimit
 */
static void test_ratelimit(void)
{
	static DEFINE_RATELIMIT_STATE(ratelimit, 5 * HZ, 5);
	if (__ratelimit(&ratelimit)) {
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	}
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#else /* >= 5.15.0 */
#endif /* 5.15.0 */

// 一些失败的测试
#ifdef TYR_AND_FAILED
extern struct list_head cpuidle_detected_devices;
static void test2(void)
{
	struct cpuidle_device *dev;
	list_for_each_entry(dev, &cpuidle_detected_devices, device_list)
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
}

static void test_show_regs(void)
{
	struct pt_regs *regs = get_irq_regs();
	if (regs)
		show_regs(regs);
}
#endif

int test_misc_init(void)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

int test_misc_exit(void)
{
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static void test_trigger_oops(void)
{
	// 如果 kernel_args+=" oops=panic panic=8"
	// kernel 不会重启，只有 oops
	char *m = NULL;
	*m = 'a';
}

int test_misc(long action)
{
	switch (action) {
	case 1:
		test_ratelimit();
		break;
#ifdef TYR_AND_FAILED
	case 2:
		test2();
		break;
	case 3:
		test_show_regs();
		break;
#endif

	case 3:
		test_trigger_oops();
		break;

	case 4:
		WARN_ONCE(true, "show_state is trigger by vt keyboard\n");

		break;
	case 6:
		// 这个 arm 还是 x86 都是支持的
		// trace_kmem_cache_alloc(_RET_IP_, ret, s, gfpflags, NUMA_NO_NODE);
		pr_info("%lx", _RET_IP_);
		break;
	}
	return 0;
}
