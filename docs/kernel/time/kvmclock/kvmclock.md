## 常用 backtrace

```txt
@[
    kvm_vcpu_write_tsc_offset+1
    __kvm_synchronize_tsc+78
    kvm_synchronize_tsc+232
    kvm_arch_vcpu_postcreate+62
    kvm_vm_ioctl+3903
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 4
@[
    kvm_vcpu_write_tsc_offset+1
    __kvm_synchronize_tsc+78
    kvm_synchronize_tsc+232
    kvm_set_msr_common+2202
    vmx_set_msr+1262
    __kvm_set_msr+145
    kvm_arch_vcpu_ioctl+3121
    kvm_vcpu_ioctl+896
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 8
```

### kvm_synchronize_tsc 需要配合 kvmclock 使用吗?

- 那么这个是?
```txt
@[
    kvm_synchronize_tsc+1
    kvm_arch_vcpu_postcreate+62
    kvm_vm_ioctl+3903
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 4
@[
    kvm_synchronize_tsc+1
    kvm_set_msr_common+2202
    vmx_set_msr+1262
    __kvm_set_msr+145
    kvm_arch_vcpu_ioctl+3121
    kvm_vcpu_ioctl+896
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 8
```


## pvti

```txt
	u64   tsc_timestamp;//参考时间对应的tsc值
	u64   system_time;//参考时间对应的ns值
	u32   tsc_to_system_mul;//由TSC时钟频率计算而来，用于和tsc_shift一起将tsc相对值转为了时间ns
	s8    tsc_shift;
```

- kvm_guest_time_update

kvm_write_wall_clock

## AMD 机器上相比 intel 机器上少了 tsc 作为 clocksource
```txt
🧀  cat /sys/devices/system/clocksource/clocksource0/available_clocksource
hpet acpi_pm
```
对比 cpuid ，可以更加清楚的知道。

#### [Timekeeping int he Linux Kernel](http://events17.linuxfoundation.org/sites/events/files/slides/Timekeeping%20in%20the%20Linux%20Kernel_0.pdf)


## [ ] kvm_arch 字段分析
```c
struct kvm_arch {

	s64 kvmclock_offset;

	/*
	 * This also protects nr_vcpus_matched_tsc which is read from a
	 * preemption-disabled region, so it must be a raw spinlock.
	 */
	raw_spinlock_t tsc_write_lock;
	u64 last_tsc_nsec;
	u64 last_tsc_write;
	u32 last_tsc_khz;
	u64 last_tsc_offset;
	u64 cur_tsc_nsec;
	u64 cur_tsc_write;
	u64 cur_tsc_offset;
	u64 cur_tsc_generation;
	int nr_vcpus_matched_tsc;

	u32 default_tsc_khz;
	bool user_set_tsc;

	seqcount_raw_spinlock_t pvclock_sc;
	bool use_master_clock;
	u64 master_kernel_ns;
	u64 master_cycle_now;
	struct delayed_work kvmclock_update_work;
	struct delayed_work kvmclock_sync_work;
```

### last + cur

搞这么复杂的机制，难道不能让所有的 cpu 直接共用一个 tsc 吗?

## kvm_vcpu_arch
```c
struct kvm_vcpu_arch {

	u64 l1_tsc_offset;
	u64 tsc_offset; /* current tsc offset */
	u64 l1_tsc_scaling_ratio;
	u64 tsc_scaling_ratio; /* current scaling ratio */

	u64 last_guest_tsc;
	u64 last_host_tsc;

	u64 tsc_offset_adjustment;

	u64 this_tsc_nsec;
	u64 this_tsc_write;
	u64 this_tsc_generation;

	bool tsc_catchup;
	bool tsc_always_catchup;

	s8 virtual_tsc_shift;
	u32 virtual_tsc_mult;
	u32 virtual_tsc_khz;

	s64 ia32_tsc_adjust_msr;
	u64 msr_ia32_power_ctl;
```
### virtual_tsc_shift virtual_tsc_mult virtual_tsc_khz
记录下 guest os 的 tsc 频率

### l1_tsc_offset tsc_offset l1_tsc_scaling_ratio tsc_scaling_ratio
填入到 vmcs 中内容，tsc_offset 可能是 l1 的，可能是 l2 的。

最后决定了 guest 读取 rdtsc 的时候，获取到的数值是什么

### last_guest_tsc 和 last_host_tsc
```diff
History:        #0
Commit:         1d5f066e0b63271b67eac6d3752f8aa96adcbddb
Author:         Zachary Amsden <zamsden@redhat.com>
Committer:      Avi Kivity <avi@redhat.com>
Author Date:    2010年08月20日 星期五 16时07分30秒
Committer Date: 2010年10月24日 星期日 16时51分24秒

KVM: x86: Fix a possible backwards warp of kvmclock

Kernel time, which advances in discrete steps may progress much slower
than TSC.  As a result, when kvmclock is adjusted to a new base, the
apparent time to the guest, which runs at a much higher, nsec scaled
rate based on the current TSC, may have already been observed to have
a larger value (kernel_ns + scaled tsc) than the value to which we are
setting it (kernel_ns + 0).

We must instead compute the clock as potentially observed by the guest
for kernel_ns to make sure it does not go backwards.

Signed-off-by: Zachary Amsden <zamsden@redhat.com>
Signed-off-by: Marcelo Tosatti <mtosatti@redhat.com>

diff --git a/arch/x86/include/asm/kvm_host.h b/arch/x86/include/asm/kvm_host.h
index 5ab1c3fb34ef..789e9462668f 100644
--- a/arch/x86/include/asm/kvm_host.h
+++ b/arch/x86/include/asm/kvm_host.h
@@ -339,6 +339,8 @@ struct kvm_vcpu_arch {
 	unsigned int time_offset;
 	struct page *time_page;
 	u64 last_host_tsc;
+	u64 last_guest_tsc;
+	u64 last_kernel_ns;

 	bool nmi_pending;
 	bool nmi_injected;
```

## 硬件基础
### wallclock

应该是需要从 cpuid 中观察到 frequency 才对


kvmclock_init 中 ，因为 "clocksource=tsc" 没有禁用 kvmclock ，所以还是要初始化 tsc ，
所以 boottime 和 tsc 的校准还是调用的 kvm_get_tsc_khz 和 kvm_get_wallclock 。

```c
	x86_platform.calibrate_cpu = kvm_get_tsc_khz;
	x86_platform.calibrate_tsc = kvm_get_tsc_khz;
	x86_platform.get_wallclock = kvm_get_wallclock;
	x86_platform.set_wallclock = kvm_set_wallclock;
```

```c
struct x86_platform_ops x86_platform __ro_after_init = {
	.calibrate_cpu			= native_calibrate_cpu_early,
	.calibrate_tsc			= native_calibrate_tsc,
	.get_wallclock			= mach_get_cmos_time,
	.set_wallclock			= mach_set_cmos_time,
```

kvm_get_tsc_khz 和 kvm_get_wallclock 就是通过两个 msr 寄存器来确定的:
1. `MSR_KVM_SYSTEM_TIME_NEW`
2. `MSR_KVM_WALL_CLOCK_NEW`
```c
static void kvm_get_wallclock(struct timespec64 *now)
{
	wrmsrl(msr_kvm_wall_clock, slow_virt_to_phys(&wall_clock));
	preempt_disable();
	pvclock_read_wallclock(&wall_clock, this_cpu_pvti(), now);
	preempt_enable();
}
```


### vmcs
Volume 3 : 26.3

```c
static void vmx_write_tsc_offset(struct kvm_vcpu *vcpu)
{
	vmcs_write64(TSC_OFFSET, vcpu->arch.tsc_offset);
}

static void vmx_write_tsc_multiplier(struct kvm_vcpu *vcpu)
{
	vmcs_write64(TSC_MULTIPLIER, vcpu->arch.tsc_scaling_ratio);
}
```

在硬件进行的变换为:
EAX:EDX = (tsc * vcpu->arch.tsc_scaling_ratio) >> 48 + vcpu->arch.tsc_offset

`kvm_get_tsc_khz` 和 `kvm_get_wallclock` 会通过 pvti 过去了。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
