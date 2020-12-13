# Port dune to mips
- [x] 如何交叉编译从而可以让我在 x86 的电脑上看内核 ?



# We will finishes it 10 days
|8 days | lindune 隐患探测|
|7 days | 完成 kernel space 的代码|


## question before write
- hypercall 
  - [ ] cause BD : something instruction emulation

- [x] syscall
  - We have to recompile glibc
  - [x] enter the guest mode in the usermode, maybe we have to adjust some register value to indicate the usermode.
    - x86 : 如果没有配置 MSR_LSTAR 的话，而是使用 int 80, 那么会执行 idt 中间的地址
    - [x] mips : syscall 的手册 : 
  - [x] MIPS virtualization manual :  4.7.4 Exception Priority
    - [x] A guest enabled interrupt occurred. / A root enabled interrupt occurred. : how to confict ? junru wang's paper says it's dangerous

- tdp
  - [x] pte_mkclean / pte_mkold
    - pte_mkold : used by notifier
    - pte_mkclean : used by map page fast 
  - [x] mmu notifier
    - [x] invalidate_range_start
    - [x] invalidate_range_end
    - [x] clear_flush_young  
    - [x] clear_young
    - [x] test_young
    - [x] change_pte : **I think dune will never call this function**
    - [x] release
  - [x] invalid TLB
  - [ ] asid

-  kvm_mips_handle_exit
  - kvm_trap_vz_handle_tlb_ld_miss + kvm_trap_vz_handle_tlb_st_miss
    - *kvm_mips_handle_vz_root_tlb_fault*
      - kvm_mips_map_page
        - [ ] kvm_mips_set_pmd_huge
      - kvm_vz_host_tlb_inv

- relation between kvm_mips_flush_gpa_pt && kvm_vz_host_tlb_inv && kvm_mips_flush_gva_pt
  - kvm_mips_flush_gva_pt : used by emulate.c at which we're not interested
  - kvm_mips_flush_gpa_pt : mmu notifier, used for invalid page table translation from `gpa -> gpa`
  - kvm_vz_host_tlb_inv : load miss / store miss exception

- entry
  - [x] why need assembly code to return to host ? because we have something more to restore, such as host register
    - [x] RESUME_HOST to userspace.
  - [ ] **TLB exception handler is managed by software, so we have to setup the TLB entry before enter to guest software.**
    - Now that we have already setup entry for the guset software, of course, change the syscall entry is possible too.
      - [x] is supporting kernel mode syscall and user mode syscall possible ?
        - YES, x86 remap the page,  MIPS rewrite the code


- memslot : substituted by our wired **dune_vm_map_pages**

## clean and old
- [x] how mips handle clean and old page in the host ?

## user mode

#### entry.c
- [x] setup_safe_stack
  - [ ] mips switch to kernel stack
- [ ] fs / gs register
  - [ ] fs keep is consistent with kernel space, maybe risk something
  - [x] gs is reserved for percpu access
- [ ] percpu
  - bind to vcpu ?
  - thread / percpu / vcpu / physical cpu 的关系
    - percpu is thread local area
    - vcpu is create under current process
    - so, thread / percpu / vcpu can migrate different physical cpu
    - [ ] gdt / idt / tss 是全局的还是局部的(必然是局部的，否则 lidt 的指令无法解释)
      - 好吧，gdt / idt / tss 都是保存在 vmcs 中间的
      - 没有 preempt 的时候，即使是在内核任何位置都是可以出现代码被 preempt 的情况
      - 在 vcpu 正在运行的时候，不会直接切换到另一个 cpu 上，但是一旦进入到 host 中间，随时都是可能切换到不同的 cpu 上，切换之后，保证再次进入的时候，vcpu 内容 和 cpu 加载的保持一致。
      - [ ] 加载到 CPU 中间的内容是 :
        - vmcs 的地址 ： vmcs_load
        - [ ]  `__vmx_setup_cpu`
      - vmcs 保存了 vcpu 的 general purpose register 的数值，除此之外还有什么 ?
        - [ ] host_rsp
        - [ ] cr2
        - [ ] msr_autoload
        - [x] guest_kernel_gs_base
        - [x] idt_base : related with posted interrupt
      - cpu 通过持有 `__this_cpu_read` 来确定自己执行的 vcpu 结构体


vcpu::guest_kernel_gs_base
```c
#define MSR_FS_BASE		0xc0000100
#define MSR_GS_BASE		0xc0000101
#define MSR_KERNEL_GS_BASE	0xc0000102
```



- [x] do_dune_enter / on_dune_exit
  - save regs in the host mode, but restore it in the guest mode
  - dune_conf

```c
	conf->vcpu = 0; // used for specify which conf is bind to vcpu
  conf->rip = (__u64) &__dune_ret;
	conf->rsp = 0; // 在 __dune_enter 的代码中间，movq	%rsp, DUNE_CFG_RSP(%rsi) 实现的赋值
	conf->cr3 = (physaddr_t) pgroot;
	conf->rflags = 0x2; // although written into the vmcs, but why 0x2 is save TODO
```
- dune_enter
  - vmx_launch
    - vmx_create_vcpu
      - vmx_setup_registers
    - vmx_copy_registers_to_conf

- [x] `__dune_go_dune` : mainly used for signal

- [x] TSS
  - 如果 syscall 全部都是被截断的，为什么一个内核态的 Stack ? 实际上，部分内容还是需要切换到内核中间, 首先，syscall 的代码的部分被覆盖，被覆盖的代码实际上在内核中间执行。
  - 其次，由于掌控内核态 和 用户态程序，在切换到内核态的时候，需要 stack 的支持
  - [ ] 有些问题没有调查清楚
    - [ ] 默认情况下，进入内核，使用的 sp 还是用户态配置的，所以并不是很清楚，在自由切换用户态和内核 的时候， stack 到底是哪一个
      - [ ] 似乎只要保持始终在内核态，那么就可以了，首先测试一下简单的程序吧 !

#### vm.c
- [ ] ept and host page table has different form, but it seems we handle ept violation and paging setup in the same way ?

## horrible
1. page table format
  - [ ] how can I verify it ?

- [x] how signal handled ?

- [ ] page 

- [ ] check code in entry line by line
  - [ ] how to register idt in the code

- [ ] thread local variable

- [ ] vmx_run_vcpu 
  - [ ] `Lkvm_vmx_return` : we are relying on some wired symbol ?
## Not Now
- [ ] `__dune_go_linux` : related with debug, but currently, debug is not used by far.
