# kvmclock 基础

## pvti 字段的含义
<!-- 986a7051-e6e2-4528-89f7-34cdf396a547 -->
```c
/*
 * These structs MUST NOT be changed.
 * They are the ABI between hypervisor and guest OS.
 * Both Xen and KVM are using this.
 *
 * pvclock_vcpu_time_info holds the system time and the tsc timestamp
 * of the last update. So the guest can use the tsc delta to get a
 * more precise system time.  There is one per virtual cpu.
 *
 * pvclock_wall_clock references the point in time when the system
 * time was zero (usually boot time), thus the guest calculates the
 * current wall clock by adding the system time.
 *
 * Protocol for the "version" fields is: hypervisor raises it (making
 * it uneven) before it starts updating the fields and raises it again
 * (making it even) when it is done.  Thus the guest can make sure the
 * time values it got are consistent by checking the version before
 * and after reading them.
 */

struct pvclock_vcpu_time_info {
	u32   version;
	u32   pad0;
	u64   tsc_timestamp;
	u64   system_time;
	u32   tsc_to_system_mul;
	s8    tsc_shift;
	u8    flags;
	u8    pad[2];
} __attribute__((__packed__)); /* 32 bytes */

struct pvclock_wall_clock {
	u32   version;
	u32   sec;
	u32   nsec;
} __attribute__((__packed__));
```
pvti 的作用就是获取到 offset 和计算的标准。


tsc_timestamp 和 system_time 这两个都是如何计算的?
认为 tsc_timestamp 和 system_time 都是相对于 guest 而言的
为什么这么设计，这是为了让虚拟机观察到的 tsc 是被虚拟过的

而且计算公式为:
multi_shift(tsc - tsc_timestamp) + system_time

利用 master clock 来更新 pvti ，但是 pvti 大概率不会修改的。

所以在 kvm_guest_time_update 中计算机出来这个
CPU 的 tsc 和 system_time 的

## pvti 到底有多少份
显然是一个 vCPU 一个的:

```c
struct kvm_vcpu_arch {
  // ...
	struct pvclock_vcpu_time_info hv_clock;
  // ...
```

MSR_KVM_SYSTEM_TIME_NEW `kvm_register_clock` 中，每一个 CPU 都是来注册的。



## guest 如何使用 kvm-clock
<!-- d5cd3184-5453-4447-a34f-109569c27be8 -->

```c
struct clocksource kvm_clock = {
	.name	= "kvm-clock",
	.read	= kvm_clock_get_cycles,
	.rating	= 400,
	.mask	= CLOCKSOURCE_MASK(64),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
	.enable	= kvm_cs_enable,
};
```

- ktime_get
  - kvm_clock_get_cycles
    - timekeeping_get_delta
    - timekeeping_delta_to_ns

```c
/* Timekeeper helper functions. */

static inline u64 timekeeping_delta_to_ns(const struct tk_read_base *tkr, u64 delta)
{
	u64 nsec;

	nsec = delta * tkr->mult + tkr->xtime_nsec;
	nsec >>= tkr->shift;

	return nsec;
}
```

- timekeeping_get_ns
    - tk_clock_read
        - kvm_clock_get_cycles
          - kvm_clock_read
            - pvclock_clocksource_read_nowd
              - __pvclock_clocksource_read
                - __pvclock_read_cycles

在 __pvclock_read_cycles 中，通过 pvti 和 tsc 获取时间
```c
static __always_inline
u64 __pvclock_read_cycles(const struct pvclock_vcpu_time_info *src, u64 tsc)
{
	u64 delta = tsc - src->tsc_timestamp;
	u64 offset = pvclock_scale_delta(delta, src->tsc_to_system_mul,
					     src->tsc_shift);
	return src->system_time + offset; // 这里得到正好是 ns 数值
}
```

注意，这里在做一个非常的变换: 一般 tsc 是小于 src->tsc_timestamp 的，所以最后加加减减让
__pvclock_read_cycles 最后返回的是 boottime 经过的 ns 数值!

- 为什么不能直接是 pvclock_scale_delta(tsc, src->tsc_to_system_mul, src->tsc_shift)
- 显然两者不是等价的，src->tsc_timestamp + src->system_time 是 masterclock 刷新的时候修改的，
刷新完成之后，__pvclock_read_cycles 返回值实际上是 host 的
get_kvmclock_base_ns() - v->kvm->arch.kvmclock_offset


2. 再次确认 __pvclock_read_cycles() 读到的 cycle 就是 ns
```c
static __always_inline u64 timekeeping_get_ns(const struct tk_read_base *tkr)
{
	return timekeeping_cycles_to_ns(tkr, tk_clock_read(tkr));
}
```

需要说明的是，实际上 kvm_clock_get_cycles 获取到的
```c
	clocksource_register_khz(&clocksource_tsc, tsc_khz);
	clocksource_register_hz(&kvm_clock, NSEC_PER_SEC);
```

## [ ] kvm 时间相关的 request
<!-- 85ec5100-11db-4271-b647-d7d194df07de -->

效果:
```c
		if (kvm_check_request(KVM_REQ_MIGRATE_TIMER, vcpu))
			__kvm_migrate_timers(vcpu); // timer 也需要迁移
		if (kvm_check_request(KVM_REQ_MASTERCLOCK_UPDATE, vcpu))
			kvm_update_masterclock(vcpu->kvm); // 更新当前 vcpu 的 masterclock
		if (kvm_check_request(KVM_REQ_GLOBAL_CLOCK_UPDATE, vcpu))
			kvm_gen_kvmclock_update(vcpu); // 用于触发让所有的 vcpu 都去刷新 master clock 的
		if (kvm_check_request(KVM_REQ_CLOCK_UPDATE, vcpu)) {
			r = kvm_guest_time_update(vcpu); // 更新 pvti
			if (unlikely(r))
				goto out;
		}
```

一共三个对象，也就是 master clock ，pvti 和 timer

- master clock 是基准 ?
	- master clock 和 pvti 什么关系? 如果使用 masterclock ，那么就所有的 pvti 使用一个 lock 来刷新
	，如果不使用，那么每一个 vCPU 都使用自己的。
- pvti 是分析的基础
- [ ] timer 热迁移的确调查一下了

### x86 kvm 更新 pvti 的位置
<!-- fcb64a6b-9aff-4d2f-88c0-89fa0484303a -->

在 kvm_guest_time_update 中，这是唯一的地方
```c
	vcpu->hv_clock.tsc_timestamp = tsc_timestamp;
	vcpu->hv_clock.system_time = kernel_ns + v->kvm->arch.kvmclock_offset;
```

kvm_guest_time_update 每 5 分钟刷新一次。


### 在哪里更新 master clock
- pvclock_update_vm_gtod_copy

其调用点为如下四个地方:
- kvm_hyperv_tsc_notifier
- kvm_vm_ioctl_set_clock
- kvm_update_masterclock
- kvm_arch_init_vm

## kvm_synchronize_tsc 的作用
<!-- ca21327f-ac6c-490f-b4e4-77cee23ccb16 -->

就是设置 tsc ，但是由于采集的 tsc 各不相同，所以需要让 target 的 vCPU 设置的数值相同。

## 热迁移后，可以保证虚拟机中观察到的 tsc 是连续的，但是

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
