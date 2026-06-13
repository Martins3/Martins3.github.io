## qemu 如何支持 kvm 的

4.2 中 cpus.c::qemu_init_vcpu 中间分析了多种 kvm 和 tcg 的引擎, 由于 tcg 和 kvm 被放到 accel 下，但是
whpx, hvf[^1] 和 hax 的支持都是 intel 特有的，所以在 qemu/target/i386 下面, 说着是加速，
其实这应该是 exec engine, 在 latest 的版本，这一些内容都被移动到这里了

对于 kvm 支持的位置:
hw/i386/kvm : 主要是设备模拟
accel/kvm


## accel/kvm/kvm-all.c:do_kvm_cpu_synchronize_state 到底是什么东西?
bool CPUState::vcpu_dirty

似乎只有进入到特殊状态之后，才会执行这些奇怪的操作。
kvm_cpu_synchronize_state

除了一个情况，
```c
static void kvm_accel_ops_class_init(ObjectClass *oc, const void *data)
{
    AccelOpsClass *ops = ACCEL_OPS_CLASS(oc);

    ops->create_vcpu_thread = kvm_start_vcpu_thread;
    ops->cpu_thread_is_idle = kvm_vcpu_thread_is_idle;
    ops->cpus_are_resettable = kvm_cpus_are_resettable;
    ops->synchronize_post_reset = kvm_cpu_synchronize_post_reset;
    ops->synchronize_post_init = kvm_cpu_synchronize_post_init;
    ops->synchronize_state = kvm_cpu_synchronize_state;
    ops->synchronize_pre_loadvm = kvm_cpu_synchronize_pre_loadvm;

#ifdef TARGET_KVM_HAVE_GUEST_DEBUG
    ops->update_guest_debug = kvm_update_guest_debug_ops;
    ops->supports_guest_debug = kvm_supports_guest_debug;
    ops->insert_breakpoint = kvm_insert_breakpoint;
    ops->remove_breakpoint = kvm_remove_breakpoint;
    ops->remove_all_breakpoints = kvm_remove_all_breakpoints;
#endif
}
```
这个导致，当 qhp 执行的时候，这个函数可以被调用。

### 如何理解 kvm_arch_process_async_events

kvm_cpu_exec 调用的开始会现执行一下 kvm_arch_process_async_events

但是在正常的配置中，这里的正常配置值得是 kvm_irqchip_in_kernel() ，只有这两个:
- CPU_INTERRUPT_MCE
- CPU_INTERRUPT_INIT


## 看看 kvm_put_msrs 的实现

1. 如何实现一次性提交多个 msr 的。

2. 这个是如何知道的 has_msr_tsc_aux 或者 has_msr_tsc_adjust ，需要取决于 guest 的 id 吗?
```c
    if (has_msr_tsc_aux) {
        kvm_msr_entry_add(cpu, MSR_TSC_AUX, env->tsc_aux);
    }
    if (has_msr_tsc_adjust) {
        kvm_msr_entry_add(cpu, MSR_TSC_ADJUST, env->tsc_adjust);
    }
```

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
