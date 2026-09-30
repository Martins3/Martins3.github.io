# kvmclock ioctl

| ioctl               | 类型                  | capability                                       | 作用                                                | kernel 实现                                                                        |
| ------------------- | --------------------- | ------------------------------------------------ | --------------------------------------------------- | ---------------------------------------------------------------------------------- |
| `KVM_GET_CLOCK`     | vm ioctl              | `KVM_CAP_ADJUST_CLOCK`                           | 读当前 kvmclock（可选带 realtime / host_tsc）       | `kvm_vm_ioctl_get_clock()` → `get_kvmclock()`                                      |
| `KVM_SET_CLOCK`     | vm ioctl              | `KVM_CAP_ADJUST_CLOCK`                           | 通过 `kvmclock_offset` 把 kvmclock 拨到指定值       | `kvm_vm_ioctl_set_clock()`                                                         |
| `KVM_SET_TSC_KHZ`   | vcpu ioctl / vm ioctl | `KVM_CAP_TSC_CONTROL` / `KVM_CAP_VM_TSC_CONTROL` | 设 guest 看到的 TSC 频率                            | `kvm_arch_vcpu_ioctl()` → `kvm_set_tsc_khz()`；vm 级写 `kvm->arch.default_tsc_khz` |
| `KVM_GET_TSC_KHZ`   | vcpu ioctl / vm ioctl | `KVM_CAP_GET_TSC_KHZ` / `KVM_CAP_VM_TSC_CONTROL` | 读 `vcpu->arch.virtual_tsc_khz` / `default_tsc_khz` | 同上                                                                               |
| `KVM_KVMCLOCK_CTRL` | vcpu ioctl            | `KVM_CAP_KVMCLOCK_CTRL`                          | 往 pvti 里塞 `PVCLOCK_GUEST_STOPPED`                | `kvm_set_guest_paused()`                                                           |

1. KVM_GET_CLOCK : `get_kvmclock()` 只是套了一层 `kvm->arch.pvclock_sc` 的 seqcount，真正的分叉在 `__get_kvmclock()`：
    1. master clock
       - `clock` 由 `__pvclock_read_cycles()` 用 `master_cycle_now`/`master_kernel_ns + kvmclock_offset` 算出来，
         所以语义是“所有 vCPU 在调用这一刻看到的同一个 kvmclock 值”。
    2. 否则：
       - `clock = get_kvmclock_base_ns() + ka->kvmclock_offset`，`flags` 全 0，
         语义退化成 `CLOCK_MONOTONIC` 加一个常数，各个 vCPU 读到的值可能不同。

## KVM_KVMCLOCK_CTRL

```txt
4.70 KVM_KVMCLOCK_CTRL
----------------------

:Capability: KVM_CAP_KVMCLOCK_CTRL
:Architectures: Any that implement pvclocks (currently x86 only)
:Type: vcpu ioctl
:Parameters: None
:Returns: 0 on success, -1 on error

This ioctl sets a flag accessible to the guest indicating that the specified
vCPU has been paused by the host userspace.

The host will set a flag in the pvclock structure that is checked from the
soft lockup watchdog.  The flag is part of the pvclock structure that is
shared between guest and host, specifically the second bit of the flags
field of the pvclock_vcpu_time_info structure.  It will be set exclusively by
the host and read/cleared exclusively by the guest.  The guest operation of
checking and clearing the flag must be an atomic operation so
load-link/store-conditional, or equivalent must be used.  There are two cases
where the guest will clear the flag: when the soft lockup watchdog timer resets
itself or when a soft lockup is detected.  This ioctl can be called any time
after pausing the vcpu, but before it is resumed.
```
实际上就是传递一个 PVCLOCK_GUEST_STOPPED 到 guest os ，guest 用这个来告诉 watchdog 不要激动。

## KVM_GET_CLOCK
```txt
4.29 KVM_GET_CLOCK
------------------

:Capability: KVM_CAP_ADJUST_CLOCK
:Architectures: x86
:Type: vm ioctl
:Parameters: struct kvm_clock_data (out)
:Returns: 0 on success, -1 on error

Gets the current timestamp of kvmclock as seen by the current guest. In
conjunction with KVM_SET_CLOCK, it is used to ensure monotonicity on scenarios
such as migration.

When KVM_CAP_ADJUST_CLOCK is passed to KVM_CHECK_EXTENSION, it returns the
set of bits that KVM can return in struct kvm_clock_data's flag member.

The following flags are defined:

KVM_CLOCK_TSC_STABLE
  If set, the returned value is the exact kvmclock
  value seen by all VCPUs at the instant when KVM_GET_CLOCK was called.
  If clear, the returned value is simply CLOCK_MONOTONIC plus a constant
  offset; the offset can be modified with KVM_SET_CLOCK.  KVM will try
  to make all VCPUs follow this clock, but the exact value read by each
  VCPU could differ, because the host TSC is not stable.

KVM_CLOCK_REALTIME
  If set, the `realtime` field in the kvm_clock_data
  structure is populated with the value of the host's real time
  clocksource at the instant when KVM_GET_CLOCK was called. If clear,
  the `realtime` field does not contain a value.

KVM_CLOCK_HOST_TSC
  If set, the `host_tsc` field in the kvm_clock_data
  structure is populated with the value of the host's timestamp counter (TSC)
  at the instant when KVM_GET_CLOCK was called. If clear, the `host_tsc` field
  does not contain a value.

::

  struct kvm_clock_data {
	__u64 clock;  /* kvmclock current value */
	__u32 flags;
	__u32 pad0;
	__u64 realtime;
	__u64 host_tsc;
	__u32 pad[4];
  };
```

## kvmclock 在 qemu 热迁移时候

1. source
  1. 把 `MSR_IA32_TSC` 中内容保存到 `uint64_t CPUArchState::tsc` 中
  2. ret = kvm_vm_ioctl(kvm_state, KVM_GET_CLOCK, &data); 从我的角度理解，第一个问题就可以了，为什么还需要第二个?

热迁移可以保证虚拟机中 rdtsc() 连续的。

由于修改的是 : kvm->arch.kvmclock_offset ，所以 ... ，不是的，并不会连续

### KVM_SET_CLOCK 的实现原理
KVM_SET_CLOCK 中，使用 kvmclock_offset 来自动修正，如果之后更新 master clock ，不会有问题吗?

kvmclock_offset 在热迁移之后，物理意义发生了变化，这有影响吗?

我们的理解是，去修改 pvti 是不是更加简单一点?

### 应该总是根据 guest os 的数值来计算 masterclock ，不然总是有变化

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
