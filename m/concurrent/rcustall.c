#include "internal.h"

#include <linux/delay.h>

/*
 * --- 1. 如果虚拟机中使用 kvm-clock 作为时钟源 -----
 *
 * 只有 5s 的时间，正常来说，不会触发任何问题，但是执行的时候
 * 在 qemu hmp 执行的 stop ，然后检查结果，输出如下:
 *
 * tick 0 : 4294777
 * tick 1 : 4294778
 * tick 2 : 4294779
 * tick 3 : 4294780
 * tick 4 : 4294781
 *
 * 看来虚拟机暂停，kvm-clock 会正确的维护时间。而且不会导致 rcustall 出现
 *
 * --- 2. 如果虚拟机中使用 tsc 作为时钟源 -----
 *
 * begin
 * tick 0 : 4294730
 * tick 1 : 4294731
 * BUG: workqueue lockup - pool cpus=0 node=0 flags=0x0 nice=0 stuck for 60s!
 * Showing busy workqueues and worker pools:
 * workqueue events: flags=0x0
 *   pwq 2: cpus=0 node=0 flags=0x0 nice=0 active=6 refcnt=7
 *     pending: psi_avgs_work, vmstat_shepherd, 4*psi_avgs_work
 * workqueue events_power_efficient: flags=0x80
 *   pwq 2: cpus=0 node=0 flags=0x0 nice=0 active=2 refcnt=3
 *     pending: neigh_managed_work, neigh_periodic_work
 * workqueue writeback: flags=0x4a
 *   pwq 128: cpus=0-31 flags=0x4 nice=0 active=2 refcnt=3
 *     in-flight: 44:wb_workfn ,308:wb_workfn
 * pool 128: cpus=0-31 flags=0x4 nice=0 hung=0s workers=5 idle: 538 42 12
 * Showing backtraces of running workers in stalled CPU-bound worker pools:
 * tick 2 : 4294790
 * tick 3 : 4294791
 * tick 4 : 4294792
 * end
 *
 * 但是这里不会导致 rcu stall 被检查到。
 *
 * --- 3. 在 qemu 中，让时间恢复的时候，出现时间跳变
 *  kvmclock_vm_state_change 中
 *          -        data.clock = s->clock;
 *	    +        data.clock = s->clock + 60 *  NANOSECONDS_PER_SECOND;
 * 时钟使用 kvmclock ，并不会出现问题，无论是主线内核还是 4.19 内核。
 *
 * --- 4. 使用 gdb attach 到 qemu 上，让 qemu 暂停，经过一段时间，重新启动虚拟机
 *  那么将可以触发 rcustall ，是必现的
 *
 * 为什么 tsc 和 kvmclock 作为时钟的都可以避开，
 * 因为似乎是因为内核在 rcustall 的检测中，有检测 kvm_check_and_clear_guest_paused
 * 而 kvmclock 打开之后，并不会由于 clocksource 是 tsc 还是 kvmclock 
 * 虚拟机暂停恢复，总是回去调用 kvm_make_request(KVM_REQ_CLOCK_UPDATE, vcpu);
 * 然其跳过。
 *
 * tsc 作为时钟源有时候会导致这个，但是还是不会导致 rcu stall 出现:
 *
 * [   45.843801] systemd[1]: systemd-logind.service: Watchdog timeout (limit 3min)!
 * [   45.844475] systemd[1]: systemd-logind.service: Killing process 663 (systemd-logind) with signal SIGABRT.
 * [   45.852416] systemd[1]: Created slice Slice /system/systemd-coredump.
 * [   45.857346] systemd-coredump[2003]: elfutils disabled, parsing ELF objects not supported
 * [   45.857607] systemd-coredump[2003]: Process 504 (systemd-journal) of user 0 dumped core.
 * [   45.857857] systemd-coredump[2003]: Coredump diverted to /var/lib/systemd/coredump/core.systemd-journal.0.72711187964c4f108c24dc46450895b0.504.1746949153000000.lz4
 * [   45.860985] systemd[1]: Started Process Core Dump (PID 2005/UID 0).
 * [   45.862790] systemd[1]: systemd-journald.service: Main process exited, code=dumped, status=6/ABRT
 * [   45.863087] systemd[1]: systemd-journald.service: Failed with result 'watchdog'.
 * [   45.863619] systemd[1]: systemd-journald.service: Scheduled restart job, restart counter is at 2.
 * [   45.864639] systemd[1]: Starting Journal Service...
 * [   45.871046] systemd-journald[2008]: Collecting audit messages is disabled.
 * [   45.871399] systemd-journald[2008]: File /run/log/journal/6759b8f500df4380987a172f7ee24c8c/system.journal corrupted or uncleanly shut down, renaming and replacing.
 * [   45.873167] systemd[1]: Started Journal Service.
 *
 */
static void rcustall(void)
{
	pr_info("begin\n");
	rcu_read_lock();
	for (size_t i = 0; i < 9; i++) {
		unsigned long jiffies_at_begin = jiffies;
		while (time_after(jiffies_at_begin + HZ, jiffies))
			cpu_relax();
		pr_info("tick %ld : %ld\n", i, jiffies / CONFIG_HZ);
	}
	rcu_read_unlock();
	pr_info("end\n");
}

static void show_jiffies(void)
{
	size_t i = 0;
	// 如果通过修改 qemu 注入时间跳变，那么 jiffies 会出现跳变
	// jiffies 不是中断数量来增加的。似乎是通过 mono 来增加的
	while (!schedule_timeout_interruptible(HZ)) {
		pr_info("tick %ld : %ld\n", i, jiffies / CONFIG_HZ);
		i++;
	}
}

int test_rcustall(long action)
{
	switch (action) {
	case 0:
		rcustall();
		break;
	case 1:
		show_jiffies();
		break;
	}
	return 0;
}
