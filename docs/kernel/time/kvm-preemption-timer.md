## preemempt timer exit
```c
/* VMCS Encodings */
enum vmcs_field {
  // ...
	VMX_PREEMPTION_TIMER_VALUE      = 0x0000482E,
```

EXIT_REASON_PREEMPTION_TIMER

```txt
   0.44%  reason PREEMPTION_TIMER rip 0x433bb9 info 0 0
   0.24%  reason PREEMPTION_TIMER rip 0x7f496d8c2ffe info 0 0
   0.16%  reason PREEMPTION_TIMER rip 0x625722 info 0 0
   0.14%  reason PREEMPTION_TIMER rip 0x4339a6 info 0 0
   0.14%  reason PREEMPTION_TIMER rip 0x416448 info 0 0
   0.11%  reason PREEMPTION_TIMER rip 0x7f496d8c2eb0 info 0 0
   0.11%  reason PREEMPTION_TIMER rip 0x7f496d8c2ea4 info 0 0
   0.10%  reason PREEMPTION_TIMER rip 0x7f496d8c2c4a info 0 0
```

1. l1 虚拟机走 : handle_fastpath_preemption_timer
2. l2 虚拟机退出的话走 : handle_preemption_timer

## steal time

- kvm_arch_vcpu_put
  - kvm_steal_time_set_preempted
    - vcpu->stat.preemption_reported++;


- /sys/kernel/debug/kvm/preemption_other
- /sys/kernel/debug/kvm/preemption_reported
```txt
[root@bogon 93070-19]# cat preemption_other
316
[root@bogon 93070-19]# cat preemption_reported
116
```

两个字段都是在 kvm_steal_time_set_preempted 中的，所以这个的含义是什么:
```c
	/*
	 * The vCPU can be marked preempted if and only if the VM-Exit was on
	 * an instruction boundary and will not trigger guest emulation of any
	 * kind (see vcpu_run).  Vendor specific code controls (conservatively)
	 * when this is true, for example allowing the vCPU to be marked
	 * preempted if and only if the VM-Exit was due to a host interrupt.
	 */
	if (!vcpu->arch.at_instruction_boundary) {
		vcpu->stat.preemption_other++;
		return;
	}
  vcpu->stat.preemption_reported++;
```

## 其他
1. kvm_intel 通过 preemption_timer
```c
/* Guest_tsc -> host_tsc conversion requires 64-bit division.  */
static int __read_mostly cpu_preemption_timer_multi;
static bool __read_mostly enable_preemption_timer = 1;
#ifdef CONFIG_X86_64
module_param_named(preemption_timer, enable_preemption_timer, bool, S_IRUGO);
#endif
```
2. 如果系统不忙的时候，可以做到没有任何 vmexit ，preemempt timer 完全不启用

## 问题
1. 嵌套虚拟化 vmx_start_preemption_timer
2. arm 是如何实现的，arm 环境中有 preemption_timer 吗?

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
