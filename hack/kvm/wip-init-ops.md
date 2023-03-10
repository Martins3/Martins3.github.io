# 这里，我们分析下 kvm 启动

```c
static struct kvm_x86_init_ops vmx_init_ops __initdata = {
	.hardware_setup = hardware_setup,
	.handle_intel_pt_intr = NULL,

	.runtime_ops = &vmx_x86_ops,
	.pmu_ops = &intel_pmu_ops,
};

// 在 KVM init 的时候，确定使用何种硬件设置，但是 emulate 还是存在的
int kvm_arch_hardware_setup(void *opaque)
{
  // ...
    memcpy(&kvm_x86_ops, ops->runtime_ops, sizeof(kvm_x86_ops));
  // ...
```

- vmx_init
  - kvm_x86_vendor_init
    - `__kvm_x86_vendor_init`
      - vmx_init_ops::hardware_setup -> hardware_setup()
        - vmx_set_cpu_caps
          - `u32 kvm_cpu_caps[NR_KVM_CPU_CAPS] __read_mostly;` : 最后都是初始化这变量，这应该是 kvm 可以对外提供的最多的能力
