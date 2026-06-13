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

## 分析
```c
static struct kvm_x86_init_ops vmx_init_ops __initdata = {
	.hardware_setup = hardware_setup,
	.handle_intel_pt_intr = NULL,

	.runtime_ops = &vmx_x86_ops,
	.pmu_ops = &intel_pmu_ops,
};
```

- hardware_setup
  - vmx_set_cpu_caps
    - kvm_set_cpu_caps : vmx 和 svm 的公共路径
    - vmx 特有的一些，最终设置到 `kvm_cpu_caps` 中间

- vmx_init
  - kvm_x86_vendor_init
    - `__kvm_x86_vendor_init`

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
