# x86 emulate

## 是不是因为需要处理变长指令 ?

- kvm_emulate_instruction ，进行完成模拟之后，就需要移动一下 ip ?

- 难道只有嵌套虚拟化的时候？
  - 不是，系统启动的时候也是存在的，但是之后，就无法
- 嵌套虚拟化需要额外

```c
	/* If guest state is invalid, start emulating.  L2 is handled above. */
	if (vmx->emulation_required)
		return handle_invalid_guest_state(vcpu);
```

并不是原因，因为 vmx_emulation_required 总是返回 0

```c
bool vmx_emulation_required(struct kvm_vcpu *vcpu)
{
	return emulate_invalid_guest_state && !vmx_guest_state_valid(vcpu);
}
```

主要的调用源头为:

- skip_emulated_instruction
- handle_io
- complete_emulated_io

## 问题

- [ ] 为什么正常的机器运行也会调用到 handle_exception_nmi 中啊
  - guest 中所有的 NMI 都需要通知一下 host 吗？ watchdog nmi 之类的

1. 什么时候需要 emulate ?

- [ ] alloc_emulate_ctxt, 被 kvm_arch_vcpu_create 唯一调用一次

  - [ ] emulate_ops
    - [ ] 里面有一堆 read / write
      - [ ] read_std : 用于普通的内存访问 , 在 emulator_io_port_access_allowed 中，用于 check seg
      - [ ] write_gpr : 寄存器, 在 vmx_vcpu_run 的时候，检查 kvm_register_is_dirty ，如果在 software 的更新过，那么使用
    - [ ] cr, idt, gdt 之类的，为什么不通过 vmx exit 处理

- vmx_x86_ops::run

  - [ ] vmx_vcpu_run
    - [ ] vmx_vcpu_enter_exit
      - [ ] `__vmx_vcpu_run` : 设置汇编实现，会将 arch.regs 更新到 vcpu 中

- [ ] 如何保持原子性 ?

- [ ] emulate 的所有的内容，应该都是为了处理 vm exit 才对啊, 因为部分指令需要模拟，但是进而牵涉到其他的模拟

- [ ] kvm_vmx_exit_handlers
  - [ ] handle_io : 应该可以找到和 eventfd 的关系
  - [ ] handle_vmx_instruction
    - [ ] kvm_queue_exception : 使用 UD_VECTOR, 表示 #UD 的 exception，因为 vmx 在指令是不应该支持的
      - [ ] kvm_multiple_exception
- [ ] emulate_ops :
- [ ] vmx_x86_ops

```c
static void emulator_get_idt(struct x86_emulate_ctxt *ctxt, struct desc_ptr *dt)
{
	kvm_x86_ops.get_idt(emul_to_vcpu(ctxt), dt);
}
```


## 一些 backtrace

### x86_emulate_instruction 每次重启都是走的这种路径，到底是什么调用路径形成的

```txt
@[
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+163
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+163
    bpf_trampoline_354334906189+87
    x86_emulate_instruction+9
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1701
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 91
@[
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+163
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+163
    bpf_trampoline_354334906189+87
    x86_emulate_instruction+9
    kvm_arch_vcpu_ioctl_run+3265
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 78833
@[
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+163
    bpf_prog_815d6551cd4d7b0b_sd_fw_ingress+163
    bpf_trampoline_354334906189+87
    x86_emulate_instruction+9
    vmx_handle_exit+2034
    kvm_arch_vcpu_ioctl_run+1701
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 122776
```
好吧，这里的确很难分析啊

## 哦，这样啊
已整理到 [emulate-2.md](emulate-2.md)，保留了原始采样，并补上当前源码调用链和 ARM64 对比。

  核心原因是：某些 VM exit 后，KVM 必须用软件完成整条指令的效果。MMIO 是最典型的场景。

  例如 guest 执行 mov dword ptr [rbx], eax：

  • 如果访问 RAM，只是 EPT 映射缺失，修好映射后重试即可。
  • 如果访问 QEMU 模拟的设备，那里没有可直接访问的 RAM；重试仍会退出。
  • EPT exit 提供地址和访问属性，却没有给出完成任意指令所需的全部信息。KVM 必须解码，确定宽度、写入值、读结果去向，以及其他状态变化。

  主要使用场景如下：

   场景                               为什么需要模拟
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   被模拟的 MMIO                      完成访问设备的普通访存指令
  ─────────────────────────────────  ──────────────────────────────────────────
   INS/OUTS、REP 字符串 I/O           同时处理端口、内存、重复次数和寄存器更新
  ─────────────────────────────────  ──────────────────────────────────────────
   写受保护的 guest 页表              完成写入，同时维护 shadow 页表
  ─────────────────────────────────  ──────────────────────────────────────────
   旧 VMX 硬件的状态限制              软件执行到能够重新进入硬件运行的状态
  ─────────────────────────────────  ──────────────────────────────────────────
   特定 #UD、UMIP、VMware 兼容路径    补充特定指令行为或异常语义

  复杂性来自 16/32/64 位模式、分段、权限与异常、flags、原子操作、REP 部分完成，以及跨用户态继续执行。变长指令只是其中一部分。

  尤其是 MMIO 读：KVM 需要先退出到 QEMU 取得数据，再通过 EMULTYPE_NO_DECODE 继续完成同一条指令。这也是你看到大量 complete_emulated_io()
  调用的原因；它们不能直接算作新的被模拟指令。KVM API 说明 (https://docs.kernel.org/virt/kvm/api.html#the-kvm-run-structure)

  另外，普通 IN/OUT、CPUID、常见 CR/MSR 操作通常走专用 handler，不需要完整模拟器。你的 function graph 已能确认一个具体样本：em_mov() 的 MMIO 写；要进一步
  看到具体指令字节，已有 kvm:kvm_emulate_insn tracepoint。

### 几个问题?

1. 那就是为什么 arm 没有那么复杂?
    - arm 只能用特殊指令处理 mmio
2. 总是从 kvm_mmu_page_fault() 开始的吗?

3. 难道，之后写 mmio 通知 virtio ，都是需要走一个这么慢的流程吗?

## svm_check_emulate_instruction

看看是不是真的有

```txt
* Detect and workaround Errata 1096 Fam_17h_00_0Fh.
```

复现方法，在 qemu 中，virtio-blk 给 dpdk 使用

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
