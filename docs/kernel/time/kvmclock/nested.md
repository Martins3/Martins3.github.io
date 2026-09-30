## 基本逻辑
从这里运行的，是为什么?
```c
static void kvm_vcpu_write_tsc_offset(struct kvm_vcpu *vcpu, u64 l1_offset) // 这个参数
{
	trace_kvm_write_tsc_offset(vcpu->vcpu_id,
				   vcpu->arch.l1_tsc_offset,
				   l1_offset);

	vcpu->arch.l1_tsc_offset = l1_offset;

	/*
	 * If we are here because L1 chose not to trap WRMSR to TSC then
	 * according to the spec this should set L1's TSC (as opposed to
	 * setting L1's offset for L2).
	 */
	if (is_guest_mode(vcpu))
		vcpu->arch.tsc_offset = kvm_calc_nested_tsc_offset(
			l1_offset,
			kvm_x86_call(get_l2_tsc_offset)(vcpu),
			kvm_x86_call(get_l2_tsc_multiplier)(vcpu));
	else
		vcpu->arch.tsc_offset = l1_offset;

	kvm_x86_call(write_tsc_offset)(vcpu); // 替换为 vmcs_write64(TSC_OFFSET, vcpu->arch.tsc_offset);
}
```

```c
u64 vmx_get_l2_tsc_offset(struct kvm_vcpu *vcpu)
{
	struct vmcs12 *vmcs12 = get_vmcs12(vcpu);

	if (nested_cpu_has(vmcs12, CPU_BASED_USE_TSC_OFFSETTING))
		return vmcs12->tsc_offset;

	return 0;
}

u64 vmx_get_l2_tsc_multiplier(struct kvm_vcpu *vcpu)
{
	struct vmcs12 *vmcs12 = get_vmcs12(vcpu);

	if (nested_cpu_has(vmcs12, CPU_BASED_USE_TSC_OFFSETTING) &&
	    nested_cpu_has2(vmcs12, SECONDARY_EXEC_TSC_SCALING))
		return vmcs12->tsc_multiplier;

	return kvm_caps.default_tsc_scaling_ratio;
}
```

从虚拟机中通过的路线为:

- kvm_set_msr_common
  - case : MSR_IA32_TSC
    - u64 adj = kvm_compute_tsc_offset(vcpu, data) - vcpu->arch.l1_tsc_offset;
		- adjust_tsc_offset_guest(vcpu, adj);

- kvm_compute_tsc_offset(vcpu, data) 为 : target tsc - rdtsc()
- adjust_tsc_offset_guest 中

	u64 tsc_offset = vcpu->arch.l1_tsc_offset;
	kvm_vcpu_write_tsc_offset(vcpu, tsc_offset + adjustment);

kvm_vcpu_write_tsc_offset 的参数为 target tsc - rdtsc()

在 kvm_vcpu_write_tsc_offset 中计算:

```txt
  // 由于是 l1_tsc_offset ，所以可以正好让 l1 获取 tsc 的时候自动为 target tsc 了
	vcpu->arch.l1_tsc_offset = target tsc - rdtsc();
	vcpu->arch.tsc_offset = kvm_x86_ops->write_l1_tsc_offset(vcpu, offset);
```

## 问题

### vmcs_write64(TSC_OFFSET, vcpu->arch.tsc_offset) 的刷新位置

每次 vcpu 的 load 切换都是需要切换的吧

是的，在 prepare_vmcs02 中回去计算出来 l2 的，然后写入到其中:
```c
	vcpu->arch.tsc_offset = kvm_calc_nested_tsc_offset(
			vcpu->arch.l1_tsc_offset,
			vmx_get_l2_tsc_offset(vcpu),
			vmx_get_l2_tsc_multiplier(vcpu));

	vcpu->arch.tsc_scaling_ratio = kvm_calc_nested_tsc_multiplier(
			vcpu->arch.l1_tsc_scaling_ratio,
			vmx_get_l2_tsc_multiplier(vcpu));

	vmcs_write64(TSC_OFFSET, vcpu->arch.tsc_offset);
```


### 为什么感觉 l2 设置 tsc 会修改 l1 的?
因为修改了 l1_tsc_offset 了。

不过测试发现，在 l1 中启动 l2 ，并不会调用到
kvm_vcpu_write_tsc_offset 中，正如其中注释
所说的。

不过，我们什么时候会出现，可以让 guest 直接写 tsc ，直接穿透到
host 呢?

### l1_tsc_offset 只是一个缓存，无论 l1 l2 谁都是用 tsc_offset
但是在缓存什么呢？ 缓存的就是 l1 的 tsc ，而且

通过这个路径，来实现来确定如何给虚拟机配置中断的:
```txt
@[
    kvm_read_l1_tsc+5
    start_sw_timer+393
    kvm_lapic_switch_to_sw_timer+75
    kvm_arch_vcpu_ioctl_run+1253
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+153
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 159
@[
    kvm_read_l1_tsc+5
    vmx_set_hv_timer+53
    restart_apic_timer+227
    handle_fastpath_set_msr_irqoff+345
    vcpu_enter_guest.constprop.0+1191
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+153
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 527
```

默认情况下，传递的数值给 guest os 的 tsc 是完全不会经过变换的吧

```sh
sudo bpftrace -e 'kfunc:kvm:kvm_read_l1_tsc { printf("%llx\n", args->vcpu->arch.l1_tsc_scaling_ratio); }'
```

### 为什么 l1 中启动 l2 不需要写 vmx_write_tsc_offset 了?
应该是转发到给 l1 ，然后 l1 写入到 vmcs 中，也就是 vmcs12 中，然后通过 vmcs12 构建 prepare_vmcs02 来实现的虚拟化。

### 在什么地方写入的 vmcs12::tsc_offset 的

```c
u64 vmx_get_l2_tsc_offset(struct kvm_vcpu *vcpu)
{
	struct vmcs12 *vmcs12 = get_vmcs12(vcpu);

	if (nested_cpu_has(vmcs12, CPU_BASED_USE_TSC_OFFSETTING))
		return vmcs12->tsc_offset;

	return 0;
}
```
应该不是通过 reference ，而是通过 vmcs 的整体同步实现的。

## 重大嫌疑点 1

- kvm_get_msr_common 中， 应该是 l1_tsc_offset

```c
	case MSR_IA32_TSC:
		msr_info->data = kvm_scale_tsc(vcpu, rdtsc()) + vcpu->arch.tsc_offset;
		break;
```

显然缺少了这个 patch :
cc5b54dd58d0420c1b1f1e9b29f53327076e9355

## 导致负数的原因

这里计算的结果是负数:
```c
u64 kvm_read_l1_tsc(struct kvm_vcpu *vcpu, u64 host_tsc)
{
	return vcpu->arch.l1_tsc_offset +
		kvm_scale_tsc(host_tsc, vcpu->arch.l1_tsc_scaling_ratio);
}
```

kvm_vcpu_write_tsc_offset 三个地方:
- __kvm_synchronize_tsc
- adjust_tsc_offset_guest : 没有影响，被自动消除了
- kvm_arch_vcpu_load : 只有 unstable 的时候


### 开机为什么会有 kvm_synchronize_tsc 的调用？


### 是 qemu 还是 guest os  写 MSR_IA32_TSC 吗?
会的，

### 原来两个数值都是有问题的

```c
	vcpu->hv_clock.tsc_timestamp = tsc_timestamp;
	vcpu->hv_clock.system_time = kernel_ns + v->kvm->arch.kvmclock_offset;
	vcpu->last_guest_tsc = tsc_timestamp;
```

 MSR_IA32_TSC


## elapsed 使用 mono ?
 ```txt
 static void kvm_synchronize_tsc(struct kvm_vcpu *vcpu, u64 *user_value)
{
	u64 data = user_value ? *user_value : 0;
	struct kvm *kvm = vcpu->kvm;
	u64 offset, ns, elapsed;
	unsigned long flags;
	bool matched = false;
	bool synchronizing = false;

	raw_spin_lock_irqsave(&kvm->arch.tsc_write_lock, flags);
	offset = kvm_compute_l1_tsc_offset(vcpu, data);
	ns = get_kvmclock_base_ns();
	elapsed = ns - kvm->arch.last_tsc_nsec;
```

## 关键

- kvm_guest_time_update
- sudo perf record -e kvm:kvm_pvclock_update

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
