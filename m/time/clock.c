#include "internal.h"
#include <linux/delay.h>
#include <linux/sched/clock.h>
#include <linux/sched/signal.h>
#include <asm/pvclock.h>
#include <uapi/asm/kvm_para.h>

static void test_ktime_get(void)
{
	/**
	* 1. ktime_get_real_ts64 等价于 gettimeofday
	* 2. ktime_get 是 boottime
	*/
	pr_info("ktime_get %lld\n", ktime_get());
	struct timespec64 ts;
	ktime_get_real_ts64(&ts);
	pr_info("ktime_get_real_ts64 sec:%lld tv_nsec:%ld\n", ts.tv_sec,
		ts.tv_nsec);

	/**
	 * [  446.946024] ktime_get 446939106446
	 * [  446.946247] ktime_get_real_ts64 sec:1716358333 tv_sec:8920170
	 * [  448.189074] kvmclock_store : action = 1 current=bash
	 * [  448.189297] ktime_get 448182371620
	 * [  448.189418] ktime_get_real_ts64 sec:1716358334 tv_sec:252083024
	 * [  449.685429] kvmclock_store : action = 1 current=bash
	 * [  449.685632] ktime_get 449678697097
	 * [  449.685752] ktime_get_real_ts64 sec:1716358335 tv_sec:748408298
	 *
	 * 1716358335 转换的时间为 2024-05-22 14:12:15
	 * 所以 ktime_get_real_ts64 等价于 gettimeofday
	 *
	 * 对比 demsg 的时间戳，可以很清楚的知道 ktime_get 就是 boottime
	 */
}

// kernel/sched/clock.c 中定义的，用于 watchdog ，但是目前看只有 powerpc 实现过
// arch/powerpc/kernel/time.c ，这里无法直接测试 running_clock ，只能测试其调用的
// local_clock
/*
 * Running clock - returns the time that has elapsed while a guest has been
 * running.
 * On a guest this value should be local_clock minus the time the guest was
 * suspended by the hypervisor (for any reason).
 * On bare metal this function should return the same as local_clock.
 * Architectures and sub-architectures can override this.
 */

/*
 * 但是即便如此，虚拟机暂停也是不会导致虚拟机的时间跳变的，
 * 更不用说导致 watchdog 被触发
 * [ 6230.495495] [test_running_clock:47] local_clock=1856
 * [ 6230.711462] clocksource: Long readout interval, skipping watchdog check: cs_nsec: 45768544254 wd_nsec: 496009415
 * [ 6231.519507] [test_running_clock:47] local_clock=1857
 *
 * 但是 RCU 会吗?
 */
static void test_running_clock(void)
{
	for (size_t i = 0; i < 1000; i++) {
		pr_info("[%s:%d] local_clock=%lld\n", __FUNCTION__, __LINE__,
			local_clock() / NSEC_PER_SEC);
		if (schedule_timeout_interruptible(HZ))
			break;
	}
}

// TODO
// 1. sched_clock_cpu : Similar to cpu_clock(), but requires local IRQs to be disabled.
static int test_sched_clock(void)
{
	return 0;
}

static void handler(void *data)
{
	u64 pa;
	int msr_kvm_system_time = MSR_KVM_SYSTEM_TIME;
	rdmsrl(msr_kvm_system_time, pa);
	pr_info("[martins3:%s:%d] %d 0x%llx\n", __FUNCTION__, __LINE__,
		smp_processor_id(), pa);
	wrmsrl(msr_kvm_system_time, pa);
}

// 测试 : 如果在 CPU 0 写 MSR_KVM_SYSTEM_TIME ，那么会从 master clock 切换到非 master clock 的
static void test_use_kvmclock(void)
{
	smp_call_function_single(0, handler, NULL, 1);
}

// TODO ktime_get_real_ts64 是什么?
static void test_kernel_clock_jump(void)
{
	u64 mono;
	u64 last_mono = 0;
	u64 last_tsc = 0;
	u64 tsc;
	for (;;) {
		pr_info("raw      : %lld\n", ktime_get_raw_ns());
		// real 和 clocktai 都是 wallclock ，不过区别是什么?
		pr_info("real     : %lld\n", ktime_get_real_ns());
		pr_info("clocktai : %lld\n", ktime_get_clocktai_ns());
		// 似乎这两个是一样的，不过他们的区别是什么呢?
		pr_info("boottime : %lld\n", ktime_get_boottime_ns());
		mono = ktime_get_ns();
		pr_info("mono     : %lld\n", mono);
		if (last_mono > mono) {
			pr_info("mono : last=%lld now=%lld diff=%lld",
				last_mono, mono, last_mono - mono);
			break;
		}
		last_mono = mono;

#ifdef CONFIG_X86_64
		tsc = rdtsc();
		pr_info("rdtsc    : %lld\n", tsc);
		if (tsc < last_tsc) {
			pr_info("tsc : last=%lld now=%lld diff=%lld", last_tsc,
				tsc, last_tsc - tsc);
			break;
		}
		last_tsc = tsc;
#endif
		if (schedule_timeout_interruptible(100))
			break;
	}
}

int test_clock_init(void)
{
	return 0;
}
int test_clock_exit(void)
{
	return 0;
}

int test_clock(long action)
{
	switch (action) {
	case 0:
		test_ktime_get();
		break;
	case 1:
		test_running_clock();
		break;
	case 2:
		test_sched_clock();
		break;
	case 3:
		// 这个在 arm 环境可以构建？TODO
		test_use_kvmclock();
		break;
	case 4:
		test_kernel_clock_jump();
		break;
	}
	return 0;
}
