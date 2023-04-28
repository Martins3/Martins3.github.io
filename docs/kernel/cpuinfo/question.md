- ArchCPU::ucode_rev 是做什么的?

## 初始化的流程最终如何决定 cpuid 的样子的
- kvm_cpu_instance_init
  - kvm_cpu_max_instance_init
    - host_cpu_max_instance_init : 使用 host 中的
    - lmce_supported
    - kvm_arch_get_supported_cpuid

## x86_cpu_expand_features

- [ ] 为什么叫做 expand ？
- [ ] filter 是什么含义?

## KVM_CPUID_FLAG_SIGNIFCANT_INDEX : 这个是什么含义?

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

## mark_unavailable_features -> 其中也是会列举 MSR 的


### [ ] 从 KVM 的角度来说，好复杂啊，但是从 QEMU 的角度来说，还可以接受

只有这两个 MSR 吗?

1. MSR_IA32_CORE_CAPS
2. MSR_IA32_ARCH_CAPABILITIES


### [ ] 各种 model 是如何提供 msr 的
