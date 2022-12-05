# 搞一个经典的路径

```txt
@[
    x86_emulate_insn+1
    x86_emulate_instruction+997
    handle_ud+81
    vmx_handle_exit+373
    kvm_arch_vcpu_ioctl_run+3604
    kvm_vcpu_ioctl+625
    __x64_sys_ioctl+138
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+99
]: 11
```

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
