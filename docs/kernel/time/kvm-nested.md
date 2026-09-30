## 还是两个数值
```diff
commit 805d705ff8f3a05e63ce350ac0c37a3290ed9bb7
Author: Ilias Stamatis <ilstam@amazon.com>
Date:   Wed May 26 19:44:09 2021 +0100

    KVM: X86: Store L1's TSC scaling ratio in 'struct kvm_vcpu_arch'

    Store L1's scaling ratio in the kvm_vcpu_arch struct like we already do
    for L1's TSC offset. This allows for easy save/restore when we enter and
    then exit the nested guest.

    Signed-off-by: Ilias Stamatis <ilstam@amazon.com>
    Reviewed-by: Maxim Levitsky <mlevitsk@redhat.com>
    Message-Id: <20210526184418.28881-3-ilstam@amazon.com>
    Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```
## 其实也不复杂

```c
struct kvm_vcpu_arch {
	u64 l1_tsc_offset;
	u64 tsc_offset; /* current tsc offset */
```
- l1_tsc_offset : 如果当前的 vcpu 是 nested 的时候，那么 l1_tsc_offset 记录 l1 的 offset

所以，当需要 l2 和 l1 之间切换的时候至少需要转换一下:

1. 进入之前加上
prepare_vmcs02
```c
	vcpu->arch.tsc_offset = kvm_calc_nested_tsc_offset(
			vcpu->arch.l1_tsc_offset,
			vmx_get_l2_tsc_offset(vcpu),
			vmx_get_l2_tsc_multiplier(vcpu));
```
2. 离开的时候还原

nested_vmx_vmexit
```c
	if (nested_cpu_has(vmcs12, CPU_BASED_USE_TSC_OFFSETTING)) {
		vcpu->arch.tsc_offset = vcpu->arch.l1_tsc_offset;
		if (nested_cpu_has2(vmcs12, SECONDARY_EXEC_TSC_SCALING))
			vcpu->arch.tsc_scaling_ratio = vcpu->arch.l1_tsc_scaling_ratio;
	}
```
## TODO
看看 CPU_BASED_USE_TSC_OFFSETTING 的影响是什么

## 如果继续调查
find_merge_commit 805d705ff8f3a05e63ce350ac0c37a3

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
