# 到底什么时候需要 emulate

- 难道只有嵌套虚拟化的时候？

```txt
@[
    x86_emulate_insn+5
    x86_emulate_instruction+824
    handle_ud+86
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 11
@[
    x86_emulate_insn+5
    x86_emulate_instruction+824
    vmx_handle_exit+2041
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 103575
```

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

## complete_emulated_io

- kvm_arch_vcpu_ioctl_run
  - vcpu::arch.complete_userspace_io : 当时注册为 complete_emulated_mmio

```txt
@[
    complete_emulated_mmio+5
    kvm_arch_vcpu_ioctl_run+4080
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 29501
```

```c
// emulation_type == 0 的时候 654
#define EMULTYPE_NO_DECODE	    (1 << 0)  // 22889
#define EMULTYPE_TRAP_UD	    (1 << 1) // 3649
#define EMULTYPE_SKIP		    (1 << 2) // 4809
#define EMULTYPE_ALLOW_RETRY_PF	    (1 << 3)
#define EMULTYPE_TRAP_UD_FORCED	    (1 << 4)
#define EMULTYPE_VMWARE_GP	    (1 << 5)
#define EMULTYPE_PF		    (1 << 6)
#define EMULTYPE_COMPLETE_USER_EXIT (1 << 7)
```

## 所以，其实大多数的 emulation 都是因为 io 导致的，为什么 io 需要 emulation
对于普通指令，发生了 page fault，重新执行该指令就可以了，但是各种 io 的指令，那么需要

## EMULTYPE_TRAP_UD

## 问题
- [ ] 为什么正常的机器运行也会调用到 handle_exception_nmi 中啊
  - guest 中所有的 NMI 都需要通知一下 host 吗？ watchdog nmi 之类的


```c
/*
 * EMULTYPE_NO_DECODE - Set when re-emulating an instruction (after completing
 *			userspace I/O) to indicate that the emulation context
 *			should be resued as is, i.e. skip initialization of
 *			emulation context, instruction fetch and decode.
 *
 * EMULTYPE_TRAP_UD - Set when emulating an intercepted #UD from hardware.
 *		      Indicates that only select instructions (tagged with
 *		      EmulateOnUD) should be emulated (to minimize the emulator
 *		      attack surface).  See also EMULTYPE_TRAP_UD_FORCED.
 *
 * EMULTYPE_SKIP - Set when emulating solely to skip an instruction, i.e. to
 *		   decode the instruction length.  For use *only* by
 *		   kvm_x86_ops.skip_emulated_instruction() implementations.
 *
 * EMULTYPE_ALLOW_RETRY_PF - Set when the emulator should resume the guest to
 *			     retry native execution under certain conditions,
 *			     Can only be set in conjunction with EMULTYPE_PF.
 *
 * EMULTYPE_TRAP_UD_FORCED - Set when emulating an intercepted #UD that was
 *			     triggered by KVM's magic "force emulation" prefix,
 *			     which is opt in via module param (off by default).
 *			     Bypasses EmulateOnUD restriction despite emulating
 *			     due to an intercepted #UD (see EMULTYPE_TRAP_UD).
 *			     Used to test the full emulator from userspace.
 *
 * EMULTYPE_VMWARE_GP - Set when emulating an intercepted #GP for VMware
 *			backdoor emulation, which is opt in via module param.
 *			VMware backoor emulation handles select instructions
 *			and reinjects the #GP for all other cases.
 *
 * EMULTYPE_PF - Set when emulating MMIO by way of an intercepted #PF, in which
 *		 case the CR2/GPA value pass on the stack is valid.
 */
#define EMULTYPE_NO_DECODE	    (1 << 0)
#define EMULTYPE_TRAP_UD	    (1 << 1)
#define EMULTYPE_SKIP		    (1 << 2)
#define EMULTYPE_ALLOW_RETRY_PF	    (1 << 3)
#define EMULTYPE_TRAP_UD_FORCED	    (1 << 4)
#define EMULTYPE_VMWARE_GP	    (1 << 5)
#define EMULTYPE_PF		    (1 << 6)
```

1. 什么时候需要 emulate ?

- [ ] alloc_emulate_ctxt, 被 kvm_arch_vcpu_create 唯一调用一次
  - [ ] emulate_ops
    - [ ] 里面有一堆 read / write
        - [ ] read_std : 用于普通的内存访问 , 在 emulator_io_port_access_allowed 中，用于 check seg
        - [ ] write_gpr : 寄存器, 在 vmx_vcpu_run 的时候，检查  kvm_register_is_dirty ，如果在 software 的更新过，那么使用
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

#### opcode_table 的使用位置

```c
static const struct opcode opcode_table[256] = {

static const struct opcode twobyte_table[256] = {
```

指令编码:
```c
struct opcode {
    u64 flags : 56;
    u64 intercept : 8;
    union {
        int (*execute)(struct x86_emulate_ctxt *ctxt);
        const struct opcode *group;
        const struct group_dual *gdual;
        const struct gprefix *gprefix;
        const struct escape *esc;
        const struct instr_dual *idual;
        const struct mode_dual *mdual;
        void (*fastop)(struct fastop *fake);
    } u;
    int (*check_perm)(struct x86_emulate_ctxt *ctxt);
};
```

## emulate_ops 和 vmx_x86_ops 的操作对比
- vmx_x86_ops 提供了各种操作的硬件支持.
- vmx 的 kvm_vmx_exit_handlers 需要 emulate 的，但是 emulator 的工作需要从 emulator 中间得到数据
