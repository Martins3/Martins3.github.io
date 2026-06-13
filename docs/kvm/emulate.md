## 到底什么时候需要 emulate


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

#### opcode_table 的使用位置

```c
static const struct opcode opcode_table[256];

static const struct opcode twobyte_table[256];
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

```txt
  0)               |  x86_emulate_instruction [kvm]() {
  0)   0.069 us    |    vmx_can_emulate_instruction [kvm_intel]();
  0)               |    x86_decode_emulated_instruction [kvm]() {
  0)               |      init_emulate_ctxt [kvm]() {
  0)               |        vmx_get_cs_db_l_bits [kvm_intel]() {
  0)   0.066 us    |          vmx_read_guest_seg_ar [kvm_intel]();
  0)   0.181 us    |        }
  0)   0.063 us    |        vmx_get_rflags [kvm_intel]();
  0)   0.060 us    |        vmx_cache_reg [kvm_intel]();
  0)   0.060 us    |        init_decode_cache [kvm]();
  0)   0.638 us    |      }
  0)               |      x86_decode_insn [kvm]() {
  0)               |        __do_insn_fetch_bytes [kvm]() {
  0)               |          emulator_get_cr [kvm]() {
  0)   0.060 us    |            vmx_cache_reg [kvm_intel]();
  0)   0.169 us    |          }
  0)               |          kvm_fetch_guest_virt [kvm]() {
  0)               |            vmx_get_cpl [kvm_intel]() {
  0)   0.058 us    |              vmx_read_guest_seg_ar [kvm_intel]();
  0)   0.163 us    |            }
  0)               |            paging64_gva_to_gpa [kvm]() {
  0)               |              paging64_walk_addr_generic [kvm]() {
  0)   0.059 us    |                vmx_cache_reg [kvm_intel]();
  0)   0.076 us    |                kvm_vcpu_gfn_to_memslot [kvm]();
  0)   0.058 us    |                gfn_to_hva_memslot_prot [kvm]();
  0)   0.061 us    |                kvm_vcpu_gfn_to_memslot [kvm]();
  0)   0.064 us    |                gfn_to_hva_memslot_prot [kvm]();
  0)   0.059 us    |                kvm_vcpu_gfn_to_memslot [kvm]();
  0)   0.058 us    |                gfn_to_hva_memslot_prot [kvm]();
  0)   0.058 us    |                vmx_get_rflags [kvm_intel]();
  0)               |                __kvm_mmu_refresh_passthrough_bits [kvm]() {
  0)   0.059 us    |                  vmx_cache_reg [kvm_intel]();
  0)   0.166 us    |                }
  0)   1.553 us    |              }
  0)   1.662 us    |            }
  0)               |            kvm_vcpu_read_guest_page [kvm]() {
  0)   0.058 us    |              kvm_vcpu_gfn_to_memslot [kvm]();
  0)               |              __kvm_read_guest_page [kvm]() {
  0)               |                __check_object_size() {
  0)   0.058 us    |                  check_stack_object();
  0)   0.057 us    |                  is_vmalloc_addr();
  0)   0.068 us    |                  __virt_addr_valid();
  0)   0.060 us    |                  __check_heap_object();
  0)   0.504 us    |                }
  0)   0.625 us    |              }
  0)   0.835 us    |            }
  0)   2.876 us    |          }
  0)   3.223 us    |        }
  0)   0.060 us    |        emulator_read_gpr [kvm]();
  0)               |        decode_operand [kvm]() {
  0)               |          decode_register [kvm]() {
  0)   0.059 us    |            emulator_read_gpr [kvm]();
  0)   0.173 us    |          }
  0)   0.058 us    |          fetch_register_operand [kvm]();
  0)   0.393 us    |        }
  0)   0.059 us    |        decode_operand [kvm]();
  0)   0.063 us    |        decode_operand [kvm]();
  0)   4.140 us    |      }
  0)   4.937 us    |    }
  0)               |    x86_emulate_insn [kvm]() {
  0)   0.059 us    |      emulator_is_guest_mode [kvm]();
  0)   0.058 us    |      em_mov [kvm]();
  0)               |      writeback [kvm]() {
  0)               |        segmented_write.isra.0 [kvm]() {
  0)               |          linearize.isra.0 [kvm]() {
  0)   0.058 us    |            emulator_get_cr [kvm]();
  0)   0.173 us    |          }
  0)               |          emulator_write_emulated [kvm]() {
  0)               |            emulator_read_write [kvm]() {
  0)               |              emulator_read_write_onepage [kvm]() {
  0)   0.059 us    |                emulator_can_use_gpa [kvm]();
  0)   0.064 us    |                vcpu_is_mmio_gpa [kvm]();
  0)               |                write_mmio [kvm]() {
  0)   0.059 us    |                  apic_mmio_write [kvm]();
  0)               |                  kvm_io_bus_write [kvm]() {
  0)               |                    __kvm_io_bus_write [kvm]() {
  0)               |                      kvm_io_bus_get_first_dev [kvm]() {
  0)   0.066 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.062 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.059 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.062 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.057 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.059 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.057 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.060 us    |                        kvm_io_bus_sort_cmp [kvm]();
  0)   0.963 us    |                      }
  0)   1.069 us    |                    }
  0)   1.176 us    |                  }
  0)   1.402 us    |                }
  0)   1.736 us    |              }
  0)   0.066 us    |              write_exit_mmio [kvm]();
  0)   1.960 us    |            }
  0)   2.061 us    |          }
  0)   2.395 us    |        }
  0)   2.494 us    |      }
  0)               |      writeback_registers [kvm]() {
  0)   0.062 us    |        emulator_write_gpr [kvm]();
  0)   0.176 us    |      }
  0)   3.066 us    |    }
  0)   0.064 us    |    vmx_get_rflags [kvm_intel]();
  0)   0.058 us    |    vmx_get_interrupt_shadow [kvm_intel]();
  0)   0.057 us    |    kvm_pmu_trigger_event [kvm]();
  0)   0.059 us    |    vmx_update_emulated_instruction [kvm_intel]();
  0)   0.059 us    |    vmx_set_rflags [kvm_intel]();
  0)   8.873 us    |  }
```


## 为什么会有如此复杂的模拟
```c
static const struct read_write_emulator_ops write_emultor = {
	.read_write_emulate = write_emulate,
	.read_write_mmio = write_mmio,
	.read_write_exit_mmio = write_exit_mmio,
	.write = true,
};
```

```c
static const struct read_write_emulator_ops read_emultor = {
	.read_write_prepare = read_prepare,
	.read_write_emulate = read_emulate,
	.read_write_mmio = vcpu_mmio_read,
	.read_write_exit_mmio = read_exit_mmio,
};

static const struct read_write_emulator_ops write_emultor = {
	.read_write_emulate = write_emulate,
	.read_write_mmio = write_mmio,
	.read_write_exit_mmio = write_exit_mmio,
	.write = true,
};
```

发现调用链特别深:
```txt
@[
    read_prepare+5
    emulator_read_write+59
    read_emulated+86
    x86_emulate_insn+553
    x86_emulate_instruction+740
    kvm_mmu_page_fault+705
    vmx_handle_exit+1988
    vcpu_enter_guest.constprop.0+1613
    kvm_arch_vcpu_ioctl_run+855
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 12072
```

就是为了模拟这个东西?

## 有办法从 x86_emulate_instruction dump 出来当时模拟的是什么指令，
如果可以，就比较直观了。到底是什么指令导致的。

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
