# khz

关键字段:
```c
struct kvm_vcpu_arch {
  // ...
	unsigned int hw_tsc_khz;
u64 l1_tsc_scaling_ratio;
	u64 tsc_scaling_ratio; /* current scaling ratio */

```


kvm_guest_time_update

定期刷新:
```c
	/* With all the info we got, fill in the values */

	if (kvm_caps.has_tsc_control)
		tgt_tsc_khz = kvm_scale_tsc(tgt_tsc_khz,
					    v->arch.l1_tsc_scaling_ratio);

	if (unlikely(vcpu->hw_tsc_khz != tgt_tsc_khz)) {
		kvm_get_time_scale(NSEC_PER_SEC, tgt_tsc_khz * 1000LL,
				   &vcpu->hv_clock.tsc_shift,
				   &vcpu->hv_clock.tsc_to_system_mul);
		vcpu->hw_tsc_khz = tgt_tsc_khz;
		kvm_xen_update_tsc_info(v);
	}
```

## 这些字段的作用是什么?
```c
struct kvm_vcpu_arch {
  // ...
  u64 l1_tsc_offset;
	u64 tsc_offset; /* current tsc offset */
	u64 last_guest_tsc;
	u64 last_host_tsc;
	u64 tsc_offset_adjustment; // 主要考虑时间
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
	u64 l1_tsc_scaling_ratio;
	u64 tsc_scaling_ratio; /* current scaling ratio */
  // ...
```

## kvm_caps 的相关内容
```c
struct kvm_caps {
	/* control of guest tsc rate supported? */
	bool has_tsc_control;
	/* maximum supported tsc_khz for guests */
	u32  max_guest_tsc_khz;
	/* number of bits of the fractional part of the TSC scaling ratio */
	u8   tsc_scaling_ratio_frac_bits;
	/* maximum allowed value of TSC scaling ratio */
	u64  max_tsc_scaling_ratio;
	/* 1ull << kvm_caps.tsc_scaling_ratio_frac_bits */
	u64  default_tsc_scaling_ratio;

  // ...
};
```

- KVM_SET_TSC_KHZ
KVM_SET_TSC_KHZ

1. has_tsc_control

```c
	if (cpu_has_vmx_tsc_scaling())
		kvm_caps.has_tsc_control = true;
```

2. tsc_scaling_ratio_frac_bits 是架构定义的常量
vmx :
```c
	kvm_caps.max_tsc_scaling_ratio = SVM_TSC_RATIO_MAX;
	kvm_caps.tsc_scaling_ratio_frac_bits = 32;
```

在 kvm_x86_vendor_init 中出现:
```c
	kvm_caps.default_tsc_scaling_ratio = 1ULL << kvm_caps.tsc_scaling_ratio_frac_bits;
```

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

之所以采用 TSC_MULTIPLIER ，应该是为了处理 guest os 的 频率变化，但是默认设置的是相同频率，
所以不会出现精度损失。

svm :
```c
	kvm_caps.max_tsc_scaling_ratio = KVM_VMX_TSC_MULTIPLIER_MAX;
	kvm_caps.tsc_scaling_ratio_frac_bits = 48;
```

### vcpu->arch.l1_tsc_scaling_ratio
```c
u64 kvm_read_l1_tsc(struct kvm_vcpu *vcpu, u64 host_tsc)
{
	return vcpu->arch.l1_tsc_offset +
		kvm_scale_tsc(host_tsc, vcpu->arch.l1_tsc_scaling_ratio);
}
```
如果不去设置频率，那么直接等价于 vcpu->arch.l1_tsc_offset + host_tsc


## kvm
```c
static u64 compute_guest_tsc(struct kvm_vcpu *vcpu, s64 kernel_ns)
{
	u64 tsc = pvclock_scale_delta(kernel_ns-vcpu->arch.this_tsc_nsec,
				      vcpu->arch.virtual_tsc_mult,
				      vcpu->arch.virtual_tsc_shift);
	tsc += vcpu->arch.this_tsc_write;
	return tsc;
}
```

__get_kvmclock 中什么不去使用 compute_guest_tsc 来计算 guest 的时间?

## kvm_arch_tsc_set_attr 该如何使用?

```txt
History:        #0
Commit:         828ca89628bfcb1b8f27535025f69dd00eb55207
Author:         Oliver Upton <oliver.upton@linux.dev>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Fri 17 Sep 2021 02:15:38 AM CST
Committer Date: Tue 19 Oct 2021 02:43:45 AM CST

KVM: x86: Expose TSC offset controls to userspace

To date, VMM-directed TSC synchronization and migration has been a bit
messy. KVM has some baked-in heuristics around TSC writes to infer if
the VMM is attempting to synchronize. This is problematic, as it depends
on host userspace writing to the guest's TSC within 1 second of the last
write.

A much cleaner approach to configuring the guest's views of the TSC is to
simply migrate the TSC offset for every vCPU. Offsets are idempotent,
and thus not subject to change depending on when the VMM actually
reads/writes values from/to KVM. The VMM can then read the TSC once with
KVM_GET_CLOCK to capture a (realtime, host_tsc) pair at the instant when
the guest is paused.

Cc: David Matlack <dmatlack@google.com>
Cc: Sean Christopherson <seanjc@google.com>
Signed-off-by: Oliver Upton <oupton@google.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
Message-Id: <20210916181538.968978-8-oupton@google.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```
优化 __kvm_synchronize_tsc 逻辑的行为的

可以理解这里的一堆判断吗?
```txt
		matched = (vcpu->arch.virtual_tsc_khz &&
			   kvm->arch.last_tsc_khz == vcpu->arch.virtual_tsc_khz &&
			   kvm->arch.last_tsc_offset == offset);
```
似乎关键的逻辑还是在 __kvm_synchronize_tsc 中:

这个 offset 不要通过

## 不同的 hyper
qemu ，当时我是用 tcg 启动的?
```txt
[    0.000000] DMI: QEMU Standard PC (i440FX + PIIX, 1996), BIOS rel-1.14.0-14-g748d619-dirty-20210524_151541-maritns3-pc 04/01/2014
[    0.000000] tsc: Fast TSC calibration using PIT
[    0.000000] tsc: Detected 1992.598 MHz processor
```

qemu  2025-04-24 nix-2043 看到的
```txt
[    0.000002] tsc: Detected 2995.200 MHz processor
[    0.095683] TSC deadline timer available
[    0.199224] clocksource: tsc-early: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
[    0.315566] clocksource: tsc: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
```

firecracker
```txt
[    0.000007] tsc: Detected 2995.200 MHz processor
[    0.050012] TSC deadline timer available
[    0.138469] clocksource: tsc-early: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
[    0.180371] clocksource: tsc: mask: 0xffffffffffffffff max_cycles: 0x2b2c8ec87c7, max_idle_ns: 440795278598 ns
[    0.180934] clocksource: Switched to clocksource tsc
```

## 虚拟机的频率是默认继承的吗?

就去使用 n100 来观测一下吧

## KVM_SET_TSC_KHZ 可以在 vm 基本设置，也可以在

但是 qemu 在 kvm_arch_set_tsc_khz 中居然使用的 vCPU 的


## 这里的问题

- kvm_set_tsc_khz

```c
	/* Compute a scale to convert nanoseconds in TSC cycles */
	kvm_get_time_scale(user_tsc_khz * 1000LL, NSEC_PER_SEC,
			   &vcpu->arch.virtual_tsc_shift,
			   &vcpu->arch.virtual_tsc_mult);
	vcpu->arch.virtual_tsc_khz = user_tsc_khz;
```


那么这里就是有问题的:
```c
static inline u64 nsec_to_cycles(struct kvm_vcpu *vcpu, u64 nsec)
{
	return pvclock_scale_delta(nsec, vcpu->arch.virtual_tsc_mult,
				   vcpu->arch.virtual_tsc_shift);
}
```
真的有必要使用这个吗?

pvti 中的 multi shift 必须正确，

```c
	tgt_tsc_khz = get_cpu_tsc_khz(); // 物理机的 tsc
	if (unlikely(tgt_tsc_khz == 0)) {
		local_irq_restore(flags);
		kvm_make_request(KVM_REQ_CLOCK_UPDATE, v);
		return 1;
	}

  // ... 省去一部分

	if (kvm_caps.has_tsc_control)
		tgt_tsc_khz = kvm_scale_tsc(tgt_tsc_khz,
					    v->arch.l1_tsc_scaling_ratio);

	if (unlikely(vcpu->hw_tsc_khz != tgt_tsc_khz)) { // 使用 hw_tsc_khz 防止重复计算
		kvm_get_time_scale(NSEC_PER_SEC, tgt_tsc_khz * 1000LL, // 这里的计算说明，pvti 是考虑过
				   &vcpu->hv_clock.tsc_shift,
				   &vcpu->hv_clock.tsc_to_system_mul);
		vcpu->hw_tsc_khz = tgt_tsc_khz;
		kvm_xen_update_tsc_info(v);
	}
```

如何和物理机差不多，那么直接使用物理机的
- set_tsc_khz
  - kvm_vcpu_write_tsc_multiplier

## 这里是没有问题的

这里的计算一定是正确的，就是使用单纯的 tsc 到物理机的频率的变化:

__get_kvmclock 中:
```c
		data->host_tsc = rdtsc();

		data->flags |= KVM_CLOCK_TSC_STABLE;
		hv_clock.tsc_timestamp = ka->master_cycle_now;
		hv_clock.system_time = ka->master_kernel_ns + ka->kvmclock_offset;
		kvm_get_time_scale(NSEC_PER_SEC, get_cpu_tsc_khz() * 1000LL,
				   &hv_clock.tsc_shift,
				   &hv_clock.tsc_to_system_mul);
		data->clock = __pvclock_read_cycles(&hv_clock, data->host_tsc);
```

## kvm_caps.tsc_scaling_ratio_frac_bits 右移 48 bit ，这个数值是固定的
这会导致精度异常吗?

对于 hz 进行变换的内容，可能性是在是不大

## 没有感觉很奇怪吗?
既然 tsc 的频率都是可以保证，为什么还需要 kvm-clock 啊?

## 难道问题在 __kvm_synchronize_tsc 中

## 4.19 有 mono 的问题，热迁移出现问题算太奇怪吧






## guest 的 tsc 频率是可以设置的

```c
typedef struct CPUArchState {

    bool tsc_valid;
    int64_t tsc_khz;
    int64_t user_tsc_khz; /* for sanity check only */ // 和 tsc_khz 是一个东西，在 cpu_post_load 进行校验
    uint64_t apic_bus_freq; // 另外的计数系统，暂时不考虑了
    uint64_t tsc;  //  kvm_get_one_msr(cpu, MSR_IA32_TSC, &value);
```

```c
    object_class_property_add(oc, "tsc-frequency", "int",
                              x86_cpuid_get_tsc_freq,
                              x86_cpuid_set_tsc_freq, NULL, NULL);
```

```c
    object_property_add_alias(obj, "tsc_scale", obj, "tsc-scale");
```

virtual_tsc_shift + virtual_tsc_mult = virtual_tsc_khz

- kvm_arch_vcpu_create
  - kvm_set_tsc_khz(vcpu, vcpu->kvm->arch.default_tsc_khz); : vcpu->kvm->arch.default_tsc_khz 来自于 kvm_arch_init_vm 中的 tsc_khz 的设置， 所谓 tsc_khz 就是物理机的 tsc 的 khz 。
    - set_tsc_khz
      - kvm_vcpu_write_tsc_multiplier : 因为默认不设置，最后导致

## KVM_SET_TSC_KHZ

从 QEMU 看:
```c
static int kvm_arch_set_tsc_khz(CPUState *cs)
```
```c
    /* vcpu's TSC frequency is either specified by user, or following
     * the value used by KVM if the former is not present. In the
     * latter case, we query it from KVM and record in env->tsc_khz,
     * so that vcpu's TSC frequency can be migrated later via this field.
     */
    if (!env->tsc_khz) {
        r = kvm_check_extension(cs->kvm_state, KVM_CAP_GET_TSC_KHZ) ?
            kvm_vcpu_ioctl(cs, KVM_GET_TSC_KHZ) :
            -ENOTSUP;
        if (r > 0) {
            env->tsc_khz = r;
        }
    }
```

从 kernel 中看:
```c
	case KVM_SET_TSC_KHZ: {
		u32 user_tsc_khz;

		r = -EINVAL;
		user_tsc_khz = (u32)arg;

		if (kvm_caps.has_tsc_control &&
		    user_tsc_khz >= kvm_caps.max_guest_tsc_khz)
			goto out;

		if (user_tsc_khz == 0)
			user_tsc_khz = tsc_khz;

		WRITE_ONCE(kvm->arch.default_tsc_khz, user_tsc_khz);
		r = 0;

		goto out;
	}
```

```c
	case KVM_SET_TSC_KHZ: {
		u32 user_tsc_khz;

		r = -EINVAL;
		user_tsc_khz = (u32)arg;

		if (kvm_caps.has_tsc_control &&
		    user_tsc_khz >= kvm_caps.max_guest_tsc_khz)
			goto out;

		if (user_tsc_khz == 0)
			user_tsc_khz = tsc_khz;

		if (!kvm_set_tsc_khz(vcpu, user_tsc_khz))
			r = 0;

		goto out;
	}
```
- kvm_set_tsc_khz
  - set_tsc_khz
    - kvm_vcpu_write_tsc_multiplier : 然后修改 vmcs 中的 offset


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
