# svm

## 如何 dump 下 vmcb

- vmx : dump_vmcs
- svm : dump_vmcb

dump_vmcb() 中有这个，看来嵌套虚拟化的确是需要硬件支持:
```c
	pr_err("%-20s%lld\n", "nested_ctl:", control->nested_ctl);
```

## 一共只有四个退出的原因

svm 一共四个指令:
```c
	[SVM_EXIT_VMRUN]			= vmrun_interception,
	[SVM_EXIT_VMMCALL]			= kvm_emulate_hypercall,
	[SVM_EXIT_VMLOAD]			= vmload_interception,
	[SVM_EXIT_VMSAVE]			= vmsave_interception,
```

而 intel 的指令更多
```c
	[EXIT_REASON_VMCALL]                  = kvm_emulate_hypercall,
	[EXIT_REASON_VMCLEAR]		      = handle_vmx_instruction,
	[EXIT_REASON_VMLAUNCH]		      = handle_vmx_instruction,
	[EXIT_REASON_VMPTRLD]		      = handle_vmx_instruction,
	[EXIT_REASON_VMPTRST]		      = handle_vmx_instruction,
	[EXIT_REASON_VMREAD]		      = handle_vmx_instruction,
	[EXIT_REASON_VMRESUME]		      = handle_vmx_instruction,
	[EXIT_REASON_VMWRITE]		      = handle_vmx_instruction,
	[EXIT_REASON_VMOFF]		      = handle_vmx_instruction,
	[EXIT_REASON_VMON]		      = handle_vmx_instruction,
```

## 首先

首先参考 amd64/v2.md 中 15.33 Nested Virtualization

看上去，当虚拟机执行 vmsave 和 vmload 的时候，会被硬件优化一下。

## 才意识到，kvm 是可以无限嵌套的

## 看上去嵌套环境 mmu npt 存在自己的想法

- nested_svm_init_mmu_context
  - kvm_init_shadow_npt_mmu

## TODO
道理我都懂，为什么 svm 的 nested 源码比 vmx 少那么多

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
