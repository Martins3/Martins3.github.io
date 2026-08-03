#include "internal.h"
#include <asm/tsc.h>
#include <linux/log2.h>
#include <linux/delay.h>

static void write_tsc_adjust_handler(void *data)
{
	pr_info("cpu : %d\n", smp_processor_id());
	wrmsrl(MSR_IA32_TSC_ADJUST, -10000);
}

/*
 * 如果 single = true ，那么结果为:
 * [Firmware Bug]: TSC ADJUST differs: CPU2 0 --> -10000000000. Restoring
 *
 * 如果 single = false ，那么:
 *
 * wrmsrl(MSR_IA32_TSC_ADJUST, - NSEC_PER_SEC * 10);
 * 或者
 * wrmsrl(MSR_IA32_TSC_ADJUST, NSEC_PER_SEC * 10);
 * [Firmware Bug]: TSC ADJUST differs: CPU3 0 --> -10000000000. Restoring
 * [Firmware Bug]: TSC ADJUST differs: CPU1 0 --> -10000000000. Restoring
 * [Firmware Bug]: TSC ADJUST differs: CPU0 0 --> -10000000000. Restoring
 * [Firmware Bug]: TSC ADJUST differs: CPU2 0 --> -10000000000. Restoring
 *
 * 也就是说，无论向前还是向后，都可以被检查出来，似乎只有 tsc_sync.c 自己调整才可以
 * 或者说，如果我们想要测试 kvm 中关于这个的代码，似乎需要利用 kernel kvm selftest 类似的机制
 */
static void read_write_tsc_adjust(bool single)
{
	s64 bootval;
	s64 tsc_orig;
	s64 tsc_new;
	int cpu;
	ktime_t ktime_orig;
	ktime_t ktime_new;

	rdmsrl(MSR_IA32_TSC_ADJUST, bootval);
	pr_info("MSR_IA32_TSC_ADJUST before : %lld\n", bootval);

	ktime_orig = ktime_get();
	rdmsrl(MSR_IA32_TSC, tsc_orig);

	if (single)
		wrmsrl(MSR_IA32_TSC_ADJUST, -NSEC_PER_SEC * 10);
	else
		for_each_possible_cpu(cpu) {
			smp_call_function_single(cpu, write_tsc_adjust_handler,
						 NULL, true);
		}

	rdmsrl(MSR_IA32_TSC, tsc_new);
	ktime_new = ktime_get();

	rdmsrl(MSR_IA32_TSC_ADJUST, bootval);
	pr_info("MSR_IA32_TSC %lld -> %lld\n", tsc_orig, tsc_new);
	pr_info("ktime_get %lld -> %lld\n", ktime_orig, ktime_new);
	pr_info("MSR_IA32_TSC_ADJUST after : %lld\n", bootval);
}

static void write_tsc_handler(void *data)
{
	pr_info("cpu : %d\n", smp_processor_id());
	wrmsrl(MSR_IA32_TSC, NSEC_PER_SEC * 10);
}

static void read_write_tsc(bool single)
{
	s64 tsc_orig;
	s64 tsc_new;
	ktime_t ktime_orig;
	ktime_t ktime_new;
	int cpu;

	// TODO rdmsrl 和 rdtsc 有区别吗?
	ktime_orig = ktime_get();
	rdmsrl(MSR_IA32_TSC, tsc_orig);

	/*
	 *
	 * [Firmware Bug]: TSC ADJUST differs: CPU2 0 --> -45928000242. Restoring
	* clocksource: Long readout interval, skipping watchdog check: cs_nsec: 30772305759 wd_nsec: 30772303145
	  * [Firmware Bug]: TSC ADJUST differs: CPU0 0 --> -45927679889. Restoring
	  * [Firmware Bug]: TSC ADJUST differs: CPU1 0 --> -91892983082. Restoring
	  * [Firmware Bug]: TSC ADJUST differs: CPU3 0 --> -45928111568. Restoring
	 */
	if (single)
		wrmsrl(MSR_IA32_TSC, NSEC_PER_SEC * 10);
	else
		// 不会立刻有错误，但是过一会有错误，似乎需要让时间重新走到相同的时间点
		/*
		 * [  185.776243] martins3: loading out-of-tree module taints kernel.
		[  185.778407] [martins3:greeter_init:311]
		[  185.788492] action = 4 current=tee
		[  185.788617] cpu : 0
		[  185.788671] cpu : 1
		[  185.788722] cpu : 2
		[  185.788773] cpu : 3
		[    3.189402] tsc 556921436308 -> 10000279930
		[    3.189323] systemd-journald[507]: Time jumped backwards, rotating.
		[    3.189516] ktime_get 185772923433 -> 185772026454
		[  324.106239] rcu: INFO: rcu_preempt detected stalls on CPUs/tasks:
		[  324.106258] [Firmware Bug]: TSC ADJUST differs: CPU1 0 --> -546921763214. Restoring
		[  324.106560] [Firmware Bug]: TSC ADJUST differs: CPU2 0 --> -546921909990. Restoring
		[  324.106709] rcu:     (detected by 3, t=138432 jiffies, g=7757, q=6014 ncpus=4)
		[  324.107547] rcu: INFO: Stall ended before state dump start
		[  324.107569] [Firmware Bug]: TSC ADJUST differs: CPU0 0 --> -546921601146. Restoring
		[  324.107630] systemd-journald[507]: Time jumped backwards, rotating.
		[  324.107775] clocksource: Long readout interval, skipping watchdog check: cs_nsec: 0 wd_nsec: 138436896371
		[  324.109253] systemd-journald[507]: Time jumped backwards, rotating.
		[  324.109790] [Firmware Bug]: TSC ADJUST differs: CPU3 0 --> -546922186965. Restoring
		 */
		for_each_possible_cpu(cpu) {
			smp_call_function_single(cpu, write_tsc_handler, NULL,
						 1);
		}

	ktime_new = ktime_get();
	rdmsrl(MSR_IA32_TSC, tsc_new);

	pr_info("tsc %lld -> %lld\n", tsc_orig, tsc_new);
	pr_info("ktime_get %lld -> %lld\n", ktime_orig, ktime_new);
}

static int test_rdtsc(void)
{
	unsigned long long start;
	unsigned long long end;
	start = rdtsc();
	mdelay(1000);
	end = rdtsc();
	pr_info("mdelay begin : %lld\n", start);
	pr_info("mdelay end   : %lld\n", end);
	pr_info("mdelay       : %lld\n", end - start);

	start = rdtsc();
	schedule_timeout_interruptible(1000);
	end = rdtsc();
	pr_info("msleep begin : %lld\n", start);
	pr_info("msleep end   : %lld\n", end);
	pr_info("msleep       : %lld\n", end - start);
	/*
	 * [ 1596.356748] mdelay       : 2995539838
	 * [ 1597.378754] msleep       : 3058399680
	 *
	 * 虚拟机中的频率是:   299_5200_000
	 */
	return 0;
}

int test_tsc(long action)
{
	switch (action) {
	case 0:
		// 虚拟机中的 tsc 时间点是从 0 开始的
		// [   16.907032] rdtsc : 51437004066
		for (size_t i = 0; i < 10; i++) {
			pr_info("rdtsc : %lld\n", rdtsc());
			if (schedule_timeout_killable(HZ))
				break;
		}
		break;
	case 1:
		pr_info("tsc_khz %d\n", tsc_khz);
		break;
	case 2:
		read_write_tsc_adjust(true);
		break;
	case 3:
		read_write_tsc_adjust(false);
		break;
	case 4:
		read_write_tsc(true);
		break;
	case 5:
		read_write_tsc(false);
		break;
	case 6:
		// 如果手动的将 tsc 配置为 unstable ，那么 clocksource 还会是 tsc 吗?
		// 需要在 firecracker 或者物理机中测试:
		//
		// 答案: 不会，
		// 可以触发如下日志:
		//
		// tsc: Marking TSC unstable due to martins3
		// clocksource: Checking clocksource tsc synchronization from CPU 1 to CPUs 0,3.
		// clocksource: Switched to clocksource kvm-clock
		//
		// 而且:
		// cat /sys/devices/system/clocksource/clocksource0/current_clocksource
		// cat /sys/devices/system/clocksource/clocksource0/available_clocksource
		// kvm-clock
		// kvm-clock
		mark_tsc_unstable("martins3");
		break;
	case 7:

		test_rdtsc();
		break;
	}
	return 0;
}
