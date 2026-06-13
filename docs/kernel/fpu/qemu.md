# qemu 处理 fpu
## 基本流程

- KVM_GET_XSAVE2 : qemu 中 kvm_arch_get_registers()
- KVM_GET_XSAVE

- KVM_SET_XSAVE : qemu 中 kvm_arch_put_registers()

```c
typedef struct CPUArchState {
    // ...
    void *xsave_buf;
    uint32_t xsave_buf_len;
    // ...
    uint64_t xstate_bv;
```

问题在于，当 qemu KVM_GET_XSAVE 和 KVM_SET_XSAVE 的时候，
enable 的 bits 都是在哪里的?

基本流程，热迁移之前:
- kvm_get_xsave
  - kvm_vcpu_ioctl : KVM_GET_XSAVE2
  - x86_cpu_xrstor_all_areas
    - env->xstate_bv = header->xstate_bv;


关键的传递是记录在: header->xstate_bv 和 env->xstate_bv

仅仅简单分析下 x86_cpu_xsave_all_areas() 中的变化，结果为:

target 端:
```txt
[martins3:x86_cpu_xsave_all_areas:51] 0 // 基本的初始化
// ...
[martins3:x86_cpu_xsave_all_areas:51] 203 // 最后传递过来的
// ...
```

soruce 端
```txt
# 启动的时候
[martins3:x86_cpu_xsave_all_areas:51] 0
// 打印 0 会有多个，但是最后输出 200 只有一个
[martins3:x86_cpu_xsave_all_areas:51] 200

# 发起热迁移之后，这是由于总是自动补充上 fpu 和 sse
localhost login: [martins3:x86_cpu_xrstor_all_areas:189] 203
[martins3:x86_cpu_xrstor_all_areas:189] 203
```
也就是会有变化的

诡异的 qemu 初始化过程是
```txt
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - qemu_process_cpu_events
          - process_queued_cpu_work
            - do_kvm_cpu_synchronize_post_init
              - kvm_arch_put_registers
                - kvm_put_xsave
                  - x86_cpu_xsave_all_areas
```

也就是虚拟机中观察到的都是正确的。由于 kvm_caps.supported_xcr0 导致
vcpu->arch.guest_supported_xcr0 实际上少

## qemu 在做热迁移的时候，状态是如何转递的

应该是 cpu flags 是 cpu flags ，状态是状态吧

所以，如果出现了状态越过了 cpu flags

传递太多状态过去了，所以会有问题的

> This becomes a problem when the user decides on migrating the above guest
> to another machine that does not support PKRU: the new host restores
> guest's fpu regs to as they were before (xrstor(s)), but since the new
> host don't support PKRU, a general-protection exception ocurs in xrstor(s)
> and that crashes the guest.

非常合理，只用传递需要的状态过去:

> This can be solved by making the guest's fpstate->user_xfeatures hold
> a copy of guest_supported_xcr0. This way, on 7 the only flags copied to
> userspace will be the ones compatible to guest requirements, and thus
> there will be no issue during migration.

目前为止，太合理了。

## 问题

x86_ext_save_areas 这个数组如何理解?

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
