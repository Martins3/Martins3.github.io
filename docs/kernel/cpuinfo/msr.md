## msr
- `kvm_emulate_cpuid` 可以给 guest 提供 msr 吗?

- [ ] kvm_init_msrs : 这个是做啥的

`x86_cpu_get_supported_feature_word`


```c
FeatureWordInfo feature_word_info[FEATURE_WORDS] = {
    // ...
    /*Below are MSR exposed features*/
    [FEAT_ARCH_CAPABILITIES] = {
        .type = MSR_FEATURE_WORD,
        .feat_names = {
            "rdctl-no", "ibrs-all", "rsba", "skip-l1dfl-vmentry",
            "ssb-no", "mds-no", "pschange-mc-no", "tsx-ctrl",
            "taa-no", NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
            NULL, NULL, NULL, NULL,
        },
        .msr = {
            .index = MSR_IA32_ARCH_CAPABILITIES,
        },
    },
    // ...
```

- `x86_cpu_get_supported_feature_word` 分别调用
  - `kvm_arch_get_supported_cpuid` : 获取 cpuid 的方法
  - `kvm_arch_get_supported_msr_feature` : 获取 cpufeature 的支持
    - 使用 KVM_GET_MSRS 来获取

实际上，可以访问的 MSR 寄存器只有这几个，实际上:

```c
static int kvm_get_msr_feature(struct kvm_msr_entry *msr)
{
	switch (msr->index) {
	case MSR_IA32_ARCH_CAPABILITIES:
		msr->data = kvm_get_arch_capabilities();
		break;
	case MSR_IA32_PERF_CAPABILITIES:
		msr->data = kvm_caps.supported_perf_cap;
		break;
	case MSR_IA32_UCODE_REV:
		rdmsrl_safe(msr->index, &msr->data);
		break;
	default:
		return static_call(kvm_x86_get_msr_feature)(msr); // 最终在 vmx_get_msr_feature，
	}
	return 0;
}
```


- kvm_arch_init
  - kvm_get_supported_feature_msrs
    - ret = kvm_ioctl(s, KVM_GET_MSR_FEATURE_INDEX_LIST, &msr_list);

- kvm_init_msr_list 中初始化使用 kvm_probe_msr_to_save 来初始化:
  - msrs_to_save_base
  - msrs_to_save_pmu
  - emulated_msrs_all
  - num_msr_based_features

拷贝出去只有这个:
```c
static u32 msr_based_features[ARRAY_SIZE(msr_based_features_all)];
static unsigned int num_msr_based_features;
```

看内核的注释，真的就是这几个了:
```c
/*
 * List of msr numbers which are used to expose MSR-based features that
 * can be used by a hypervisor to validate requested CPU features.
 */
static const u32 msr_based_features_all[] = {
	MSR_IA32_VMX_BASIC,
	MSR_IA32_VMX_TRUE_PINBASED_CTLS,
	MSR_IA32_VMX_PINBASED_CTLS,
	MSR_IA32_VMX_TRUE_PROCBASED_CTLS,
	MSR_IA32_VMX_PROCBASED_CTLS,
	MSR_IA32_VMX_TRUE_EXIT_CTLS,
	MSR_IA32_VMX_EXIT_CTLS,
	MSR_IA32_VMX_TRUE_ENTRY_CTLS,
	MSR_IA32_VMX_ENTRY_CTLS,
	MSR_IA32_VMX_MISC,
	MSR_IA32_VMX_CR0_FIXED0,
	MSR_IA32_VMX_CR0_FIXED1,
	MSR_IA32_VMX_CR4_FIXED0,
	MSR_IA32_VMX_CR4_FIXED1,
	MSR_IA32_VMX_VMCS_ENUM,
	MSR_IA32_VMX_PROCBASED_CTLS2,
	MSR_IA32_VMX_EPT_VPID_CAP,
	MSR_IA32_VMX_VMFUNC,

	MSR_AMD64_DE_CFG,
	MSR_IA32_UCODE_REV,
	MSR_IA32_ARCH_CAPABILITIES,
	MSR_IA32_PERF_CAPABILITIES,
};
```

因为这里不提供，所以


### guest 中如何检测 MSR 寄存器

- tsx_init : 大致是这个代码吧！

```c
static bool __init tsx_ctrl_is_supported(void)
{
	u64 ia32_cap = x86_read_arch_cap_msr();

	return !!(ia32_cap & ARCH_CAP_TSX_CTRL_MSR);
}

u64 x86_read_arch_cap_msr(void)
{
	u64 ia32_cap = 0;

	if (boot_cpu_has(X86_FEATURE_ARCH_CAPABILITIES))
		rdmsrl(MSR_IA32_ARCH_CAPABILITIES, ia32_cap);

	return ia32_cap;
}
```

kvm 设置的位置: `kvm_set_msr_common`

```txt
@[
    kvm_set_msr_common+5
    vmx_set_msr+1401
    __kvm_set_msr+127
    kvm_emulate_wrmsr+86
    vmx_handle_exit+1971
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 2269
@[
    kvm_set_msr_common+5
    vmx_set_msr+1401
    __kvm_set_msr+127
    kvm_arch_vcpu_ioctl+3144
    kvm_vcpu_ioctl+1208
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 6216
```

### guest 设置 msr 最后是什么效果

### host 中设置 msr 的目的是什么

## mark_unavailable_features -> 其中也是会列举 MSR 的


### [ ] 从 KVM 的角度来说，好复杂啊，但是从 QEMU 的角度来说，还可以接受

只有这两个 MSR 吗?

1. MSR_IA32_CORE_CAPS
2. MSR_IA32_ARCH_CAPABILITIES


### [ ] 各种 model 是如何提供 msr 的

## 根本看不懂各种 msr 的含义

### kvm_synchronize_tsc

```txt
@[
    kvm_synchronize_tsc+1
    kvm_set_msr_common+2755
    vmx_set_msr+1401
    __kvm_set_msr+127
    kvm_arch_vcpu_ioctl+3144
    kvm_vcpu_ioctl+1208
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 62
```

### 分析一下 MSR_IA32_CORE_CAPS : QEMU 是 MSR，但是 kernel 是 CPUID

MSR_IA32_CORE_CAPS 中 split_lock_setup

```c
static void __init split_lock_setup(struct cpuinfo_x86 *c)
{
	const struct x86_cpu_id *m;
	u64 ia32_core_caps;

	if (boot_cpu_has(X86_FEATURE_HYPERVISOR))
		return;

	/* Check for CPUs that have support but do not enumerate it: */
	m = x86_match_cpu(split_lock_cpu_ids);
	if (m)
		goto supported;

	if (!cpu_has(c, X86_FEATURE_CORE_CAPABILITIES))
		return;

	/*
	 * Not all bits in MSR_IA32_CORE_CAPS are architectural, but
	 * MSR_IA32_CORE_CAPS_SPLIT_LOCK_DETECT is.  All CPUs that set
	 * it have split lock detection.
	 */
	rdmsrl(MSR_IA32_CORE_CAPS, ia32_core_caps);
	if (ia32_core_caps & MSR_IA32_CORE_CAPS_SPLIT_LOCK_DETECT)
		goto supported;

	/* CPU is not in the model list and does not have the MSR bit: */
	return;

supported:
	cpu_model_supports_sld = true;
	__split_lock_setup();
}
```

- kvm_get_supported_msrs
- kvm_init_msrs 构建 X86CPU::kvm_msr_buf

## 最终的问题
- MSR_IA32_CORE_CAPS : kvm 和 qemu 是如何协商
- libvirt 并不会对比 msr
