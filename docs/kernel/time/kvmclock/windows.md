# kvmclock windows
<!-- f57b954b-43a4-431a-ae29-dd42a7da4c7a -->

windows 开启 hv_time 后不会主动写 tsc，但是会依赖pvti
开启后，windows guest 会去写这个 msr HV_X64_MSR_REFERENCE_TSC，然后建立一个共享页把 tsc 的counter 传递过去
之后会不定期的刷这个共享页

刷的地方就在这里 kvm_guest_time_update 最下面
	if (v == kvm_get_vcpu(v->kvm, 0))
		kvm_hv_setup_tsc_page(v->kvm, &vcpu->hv_clock);

关于 windows 更多的问题:
- https://gitlab.redox-os.org/enygmator/qemu/-/blob/main/docs/hyperv.txt
- https://stackoverflow.com/questions/20286207/windows-time-drifting-with-qemu-kvm
- https://documentation.suse.com/sles/15-SP6/html/SLES-all/sec-kvm-managing-clock.html

## codex
严格来说：主流 Windows Guest 不直接使用 KVM 原生的 kvmclock ABI。

Windows 在 KVM/QEMU 上通常走两类路径：

- 未启用 Hyper-V enlightenments：使用虚拟 TSC、HPET、PIT/RTC 等传统时钟源。
- 启用 hv-time：Windows 使用 Hyper-V 的时间接口：
    - HV_X64_MSR_TIME_REF_COUNT（0x40000020）
    - HV_X64_MSR_REFERENCE_TSC（0x40000021）
    - 通常还配合 hv-stimer 使用 Hyper-V synthetic timer。

而真正的 kvmclock ABI 是：

- MSR_KVM_SYSTEM_TIME_NEW：0x4b564d01
- MSR_KVM_WALL_CLOCK_NEW：0x4b564d00
- 通过 KVM_FEATURE_CLOCKSOURCE{,2} 宣告

它主要由 Linux 等理解 KVM PV ABI 的 guest 使用，定义见 Documentation/virt/kvm/x86/msr.rst:20 和 Documentation/virt/kvm/x86/cpuid.rst:39。

容易混淆的是：**Windows 虽不直接读 kvmclock，KVM 实现 Windows 的 Hyper-V 时钟时，内部会以 kvmclock/pvclock 为基础。**例如：

- Hyper-V reference counter 在某些情况下回退到 get_kvmclock_ns()：arch/x86/kvm/hyperv.c:572
- KVM 会把 pvclock_vcpu_time_info 转换成 Hyper-V Reference TSC page 参数：arch/x86/kvm/hyperv.c:compute_tsc_page_parameters

所以可以概括为：

> Windows 通常没有直接使用 kvmclock；它使用的是 Hyper-V hv-time 接口，但 KVM 后端可能用 kvmclock/pvclock 数据来实现这个接口。

这也是 QEMU 为 Windows 推荐开启 hv-time，而不是期待 Windows 识别 KVM 原生 kvmclock 的原因。QEMU Hyper-V enlightenments 文档
(https://www.qemu.org/docs/master/system/i386/hyperv.html)

```txt
@[
        kvm_hv_setup_tsc_page+5
        kvm_guest_time_update+499
        vcpu_enter_guest.constprop.0+925
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+371
        kvm_vcpu_ioctl+876
        __x64_sys_ioctl+185
        do_syscall_64+226
        entry_SYSCALL_64_after_hwframe+118
]: 50
```

## 寄存器的使用

使用 collei 的 Win11 24H2（build 26200.9445，16 vCPU）冷启动测试：

 配置           guest WRMSR                                                  QPC frequency
━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━
 hv_time=on     CPU0 写 HV_X64_MSR_REFERENCE_TSC(0x40000021)=0xd001                 10 MHz
─────────────  ───────────────────────────────────────────────────────────  ───────────────
 hv_time=off    不写 Hyper-V/KVM clock MSR；各 vCPU 写 MSR_IA32_TSC(0x10)          100 MHz

两种配置下，都没有观察到 guest 写：

- MSR_KVM_SYSTEM_TIME（0x12）
- MSR_KVM_SYSTEM_TIME_NEW（0x4b564d01）
- KVM wall-clock MSR


## 什么时候更新 tsc_reference

其实就是在 kvm_guest_time_update 中，更新每一个 vCPU 的 pvti 的时候，调用

kvm_hv_setup_tsc_page 中:

```c
	if (!compute_tsc_page_parameters(hv_clock, &hv->tsc_ref))
		goto out_err;

	/* Ensure sequence is zero before writing the rest of the struct.  */
	smp_wmb();
	if (kvm_write_guest(kvm, gfn_to_gpa(gfn), &hv->tsc_ref, sizeof(hv->tsc_ref)))
		goto out_err;
```
## 问题

1. 那么 windows 有 master clock 机制吗?

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
