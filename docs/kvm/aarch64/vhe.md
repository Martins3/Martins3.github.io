# vhe 和 non-vhe

## 当前环境主要使用 vhe 模式

[To EL2, and Beyond! Optimizing the Design and Implementation of KVM/ARM](http://events17.linuxfoundation.org/sites/events/files/slides/To%20EL2%20and%20Beyond_0.pdf)

1. Xen disables VHE

配套的文章:
[ARM Virtualization: Performance and Architectural Implications](https://par.nsf.gov/servlets/purl/10310788)

[官方文档](https://developer.arm.com/documentation/102142/0100/Virtualization-host-extensions)

ARMv8 定义了 4 个特权级（EL0 ~ EL3），与虚拟化相关的主要是：

- EL0：用户态（运行普通应用程序）。
- EL1：内核态（运行操作系统内核，如 Linux）。
- EL2：Hypervisor 层（运行虚拟机监控器，如 KVM）。
- EL3：安全监控层（如 TrustZone）。

在 Non vhe 模式:

1. 每次陷入 Hypervisor（例如虚拟机发起系统调用或触发异常）时，CPU 需要从 Guest EL1/EL0 → Host EL1 → EL2。
返回时需反向切换 EL2 → Host EL1 → Guest EL1/EL0。
2. 内存虚拟化需要两阶段地址翻译（Stage-1 由 Guest OS 管理，Stage-2 由 Hypervisor 管理），
而宿主内核的页表在 EL1 和 EL2 之间切换时需额外处理。

## aarch64 的 el2 模式的作用
<!-- 8027116c-1a1a-4107-aa5a-ccdd3d0ac730 -->


Type-1 和 Type-2 的 Hypervisor :

Type-1模式下(XEN)：
```txt
EL0: [ APP  APP ] [ APP  APP ]
EL1: [ Guest OS ] [ Guest OS ]
EL2: [        XEN Hyp        ]
```

Type-2模式下(KVM)：
```txt
EL0: [    APP  APP    ] [ APP  APP ]
EL1: [ Host OS + KVM  ] [ Guest OS ]
EL2: [           KVM Hyp           ]
```

如果是 vhe 模式:
```txt
EL0: [    APP  APP    ] [ APP  APP ]
EL1: [                ] [ Guest OS ]
EL2: [    Host OS + KVM + Hyp      ]  (VHE Mode)
```
可以看到 el2 是为了给 xen 之类的 Hypervisor 设计的，其实对于 qemu kvm 不是很友好，所以有这个。

参考 https://kernel-tour.org/archives/kvm/vhe
## 基本的代码分析

nvhe 的处理分为两种情况:
```c
static const exit_handler_fn hyp_exit_handlers[] = {
	[0 ... ESR_ELx_EC_MAX]		= NULL,
	[ESR_ELx_EC_CP15_32]		= kvm_hyp_handle_cp15_32,
	[ESR_ELx_EC_SYS64]		= kvm_hyp_handle_sysreg,
	[ESR_ELx_EC_SVE]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_FP_ASIMD]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_IABT_LOW]		= kvm_hyp_handle_iabt_low,
	[ESR_ELx_EC_DABT_LOW]		= kvm_hyp_handle_dabt_low,
	[ESR_ELx_EC_PAC]		= kvm_hyp_handle_ptrauth,
};

static const exit_handler_fn pvm_exit_handlers[] = {
	[0 ... ESR_ELx_EC_MAX]		= NULL,
	[ESR_ELx_EC_SYS64]		= kvm_handle_pvm_sys64,
	[ESR_ELx_EC_SVE]		= kvm_handle_pvm_restricted,
	[ESR_ELx_EC_FP_ASIMD]		= kvm_hyp_handle_fpsimd,
	[ESR_ELx_EC_IABT_LOW]		= kvm_hyp_handle_iabt_low,
	[ESR_ELx_EC_DABT_LOW]		= kvm_hyp_handle_dabt_low,
	[ESR_ELx_EC_PAC]		= kvm_hyp_handle_ptrauth,
};

static const exit_handler_fn *kvm_get_exit_handler_array(struct kvm_vcpu *vcpu)
{
	if (unlikely(kvm_vm_is_protected(kern_hyp_va(vcpu->kvm))))
		return pvm_exit_handlers;

	return hyp_exit_handlers;
}
```

而 vhe 中只有一种:
```c
static const exit_handler_fn *kvm_get_exit_handler_array(struct kvm_vcpu *vcpu)
{
	return hyp_exit_handlers;
}
```

## 分析下 Asahi Linux 是那种模式 vhe 还是 non-vhe 的

- kvm_arch_vcpu_ioctl_run
  - kvm_arm_vcpu_enter_exit
    - `__kvm_vcpu_run`

```c
/*
 * Actually run the vCPU, entering an RCU extended quiescent state (EQS) while
 * the vCPU is running.
 *
 * This must be noinstr as instrumentation may make use of RCU, and this is not
 * safe during the EQS.
 */
static int noinstr kvm_arm_vcpu_enter_exit(struct kvm_vcpu *vcpu)
{
	int ret;

	guest_state_enter_irqoff();
	ret = kvm_call_hyp_ret(__kvm_vcpu_run, vcpu);
	guest_state_exit_irqoff();

	return ret;
}
```

- `__kvm_vcpu_run` : nvhe 和 vhe 都存在一份代码
  - __guest_enter
  - fixup_guest_exit
    - kvm_hyp_handle_exit


## 如何检查当前是 VHE 模式
<!-- f7c2e88a-9222-4929-a3b7-7135f3c97892 -->

默认都是 VHE 模式的

mac 上
```txt
[    0.040622] kvm [1]: IPA Size Limit: 36 bits (Reduced IPA size, limited VM/VMM compatibility)
[    0.040632] kvm [1]: Non-architectural vgic, tainting kernel
[    0.040634] kvm [1]: GICv3: no GICV resource entry
[    0.040635] kvm [1]: disabling GICv2 emulation
[    0.040637] kvm [1]: GICv3 with broken locally generated SEI
[    0.040638] kvm [1]: GICv3 sysreg trapping enabled ([G0G1D], reduced performance)
[    0.040650] kvm [1]: GIC system register CPU interface enabled
[    0.040656] kvm [1]: vgic interrupt IRQ33
[    0.040675] kvm [1]: VHE mode initialized successfully
```

kunpeng 上:
```txt
[    0.326826] kvm [1]: nv: 566 coarse grained trap handlers
[    0.326937] kvm [1]: IPA Size Limit: 48 bits
[    0.326986] kvm [1]: GICv4 support disabled
[    0.326991] kvm [1]: disabling GICv2 emulation
[    0.327054] kvm [1]: GIC system register CPU interface enabled
[    0.327066] kvm [1]: vgic interrupt IRQ9
[    0.327149] kvm [1]: VHE mode initialized successfully
```

在 asahi linux 尝试强制打开: kvm-arm.mode=nvhe

```txt
[    0.000000] Machine model: Apple MacBook Pro (13-inch, M2, 2022)
[    0.000000] ------------[ cut here ]------------
[    0.000000] WARNING: CPU: 0 PID: 0 at arch/arm64/kvm/arm.c:2643 early_kvm_mode_cfg+0x15c/0x178
[    0.000000] Modules linked in:
[    0.000000] CPU: 0 PID: 0 Comm: swapper Not tainted 6.8.9-405.asahi.fc40.aarch64+16k #1
[    0.000000] Hardware name: Apple MacBook Pro (13-inch, M2, 2022) (DT)
[    0.000000] pstate: 604001c9 (nZCv dAIF +PAN -UAO -TCO -DIT -SSBS BTYPE=--)
[    0.000000] pc : early_kvm_mode_cfg+0x15c/0x178
[    0.000000] lr : early_kvm_mode_cfg+0x78/0x178
[    0.000000] sp : ffffafcf59963c60
[    0.000000] x29: ffffafcf59963c60 x28: 0000000000000000 x27: 00000000000001c0
[    0.000000] x26: 0000000000000000 x25: ffffafcf58379c48 x24: ffffafcf58379000
[    0.000000] x23: ffffafcf58e151a8 x22: ffffafcf58379c30 x21: ffffafcf58f4b9e0
[    0.000000] x20: ffffafcf58e1519b x19: ffffafcf58e151a8 x18: 000000007d0aff31
[    0.000000] x17: 000000000000000a x16: 0000000000002c01 x15: fffffefffde0e00e
[    0.000000] x14: 0000000000000074 x13: 0000000000000020 x12: 0000000000000000
[    0.000000] x11: ffffafcf57fdd8b0 x10: ffffffffff56e1d0 x9 : 0000000000000020
[    0.000000] x8 : 0101010101010101 x7 : 7f7f7f7f7f7f7f7f x6 : 0000000080808080
[    0.000000] x5 : 0000000000000000 x4 : 8080808000000000 x3 : 0000000000000000
[    0.000000] x2 : 0000000000000000 x1 : ffffafcf58383378 x0 : 0000000000000008
[    0.000000] Call trace:
[    0.000000]  early_kvm_mode_cfg+0x15c/0x178
[    0.000000]  do_early_param+0xc8/0x110
[    0.000000]  parse_one+0xcc/0x220
[    0.000000]  parse_args+0xb4/0x238
[    0.000000]  parse_early_param+0x8c/0xc0
[    0.000000]  setup_arch+0x128/0x288
[    0.000000]  start_kernel+0x7c/0x410
[    0.000000]  __primary_switched+0xb8/0xc8
[    0.000000] ---[ end trace 0000000000000000 ]---
[    0.000000] Malformed early option 'kvm-arm.mode'
```

似乎并没有什么用，观察 vhe 下的函数，还是可以被正确使用的:
```c
void kvm_vcpu_put_vhe(struct kvm_vcpu *vcpu)
{
	__vcpu_put_deactivate_traps(vcpu);
	__vcpu_put_switch_sysregs(vcpu);
}
```

## 具体代码的分析

- kvm_arch_vcpu_ioctl_run
  - `__kvm_vcpu_run` : vhe 和 nvhe 各有一份的
    - `__guest_enter` : arch/arm64/kvm/hyp/entry.S
  - handle_exit
   - handle_trap_exceptions
    - kvm_get_exit_handler
    - arm_exit_handlers : 所有的 handler 入口
      - kvm_handle_guest_abort : mmu.c 处理 tdp 的映射问题, 这些和 MIPS 中间的内容非常的对称

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
