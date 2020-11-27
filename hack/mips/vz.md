# MIPS VZ

<!-- vim-markdown-toc GitLab -->

- [TODO](#todo)
- [irqchip](#irqchip)
- [what's happending in the](#whats-happending-in-the)
- [how to handle fpu](#how-to-handle-fpu)
- [async ioctl](#async-ioctl)
- [functions](#functions)
- [mmu](#mmu)
- [emulate.c](#emulatec)
- [entry.c](#entryc)
- [exception](#exception)
- [coproc](#coproc)
- [kseg0](#kseg0)
- [tlb](#tlb)
- [cpu load / put](#cpu-load-put)
- [kvm_trap_vz_handle_guest_exit and kvm_mips_handle_exit](#kvm_trap_vz_handle_guest_exit-and-kvm_mips_handle_exit)
- [entry.c](#entryc-1)
- [GuestCtl0 GuestCtl1](#guestctl0-guestctl1)
- [code overview](#code-overview)

<!-- vim-markdown-toc -->

## TODO
- [ ] what's relation with loongson vz and MIPS vz ?

- [ ] so, why we should emulate ?

- [x] exc ?
- [x] c0
  - arch/mips/include/asm/mipsregs.h defines macros for access **system control register**

- [ ] Makefile : without KVM_MIPS_VZ, `dyntrans.c` and `trap_emul.c` are needed
  - [ ] check code flow if `KVM_MIPS_VZ` is disabled
  - [ ] why *emulate* and *vmx* coexist in x86

- [ ] dyntrans.c

kvm_mips_check_privilege : if `EMULATE_PRIV_FAIL`, then `kvm_mips_emulate_exc`

- [ ] `kvm_trap_vz_handle_cop_unusable` : use unusable cpu ?

- [ ] `kvm_trap_vz_no_handler` : kvm_mips_callbacks::handle_break : break ?

- [ ] `kvm_trap_vz_handle_msa_disabled` : msa ?

- [ ] `kvm_trap_vz_handle_guest_exit` ?

- [ ] What does this mean ?	`if (!IS_ENABLED(CONFIG_KVM_MIPS_VZ))`

Why so little exit handler ? `kvm_mips_handle_exit` need more 

kvm_arch_init_vm :

- [ ] vz used mmu_notifier in kvm_main.c

- [ ] kvm_set_memory_region =>  kvm_arch_commit_memory_region => kvm_mips_mkclean_gpa_pt : we can track ad bit here too !

- [ ] also, we should read the documentations about MIPS KVM, the explanation of ioctls


```c
static unsigned long kvm_mips_num_regs(struct kvm_vcpu *vcpu)
{
	unsigned long ret;

	ret = ARRAY_SIZE(kvm_mips_get_one_regs);
	if (kvm_mips_guest_can_have_fpu(&vcpu->arch)) {
		ret += ARRAY_SIZE(kvm_mips_get_one_regs_fpu) + 48;
		/* odd doubles */
		if (boot_cpu_data.fpu_id & MIPS_FPIR_F64)
			ret += 16;
	}
	if (kvm_mips_guest_can_have_msa(&vcpu->arch))
		ret += ARRAY_SIZE(kvm_mips_get_one_regs_msa) + 32;
	if (kvm_mips_guest_has_lasx(vcpu))
		ret += 32;
	ret += kvm_mips_callbacks->num_regs(vcpu);

	return ret;
}
```
- [ ] kvm_mips_guest_can_have_fpu
- [x] kvm_mips_guest_can_have_msa : MSA2 是MIPS科技公司继 MSA 后推出的新向量指令集，在龙芯3A/B4000中首次实现。龙芯中科以此拓展出256位的 LASX 向量指令集。
- [x] kvm_mips_guest_has_lasx : SIMD

So, we can understand mips arch now:
- [x] kvm_mips_get_reg
- [x] kvm_mips_set_reg
- [ ] kvm_vcpu_arch
  - [ ] mips_coproc
  - [ ] mips_fpu_struct
  - [ ] kvm_mips_tlb
  - [ ] kseg0_commpage
  - [ ] hrtimer
  - [ ] MIPS R6
    - [ ] lo
    - [ ] hi
    - [ ] pc
  - [ ] `kvm_mips_callbacks->get_one_reg` : read_gc0_context
    - [ ] gc0 is coproc ?

## irqchip
Loongson add ...

## what's happending in the 

## how to handle fpu

## async ioctl

## functions
- kvm_read_c0_guest_status


## mmu
- [ ] flush ?
  - [ ] difference between kvm_mips_flush_gpa_pte && kvm_mips_flush_gva_pte
  - [ ] why we need flush ?

- [ ] mkold
- [ ] mkclean

kvm_mips_map_page : is **core** function ?

- [ ] kseg0

- kvm_mips_handle_vz_root_tlb_fault
  - [ ] why surrounded by CONFIG_KVM_MIPS_VZ
  - x: kvm_trap_vz_handle_tlb_ld_miss + kvm_trap_vz_handle_tlb_st_miss
    - x: kvm_mips_handle_exit
- kvm_mips_handle_kseg0_tlb_fault
  - x: kvm_trap_emul_gva_fault
    - x: kvm_get_inst
- kvm_mips_handle_mapped_seg_tlb_fault : caller is emulate



KO : **kvm_mips_handle_vz_root_tlb_fault** is the key

- [ ] others functions is used by emulate ?
  - [ ] what does emulate mean ? shadow page ? what shold you emulate ?


## emulate.c
- [ ] kvm_mips_emulate_CP0 : so many, but not used by kvm
- [ ] kvm_mips_emulate_load : called by vz.c, with kvm_trap_vz_handle_tlb_ld_miss, used for *MMIO*

## entry.c
just like entry.S

## exception
- [ ] what special attention does it need  ?

- [ ] ebase reg ?
  - [ ] cpu_has_ebase_wg
- [ ] gebase

- [ ] kvm_arch_vcpu_create : +2000 +180 +200

## coproc
```c
#define N_MIPS_COPROC_REGS	32
#define N_MIPS_COPROC_SEL	8

struct mips_coproc {
	unsigned long reg[N_MIPS_COPROC_REGS][N_MIPS_COPROC_SEL];
#ifdef CONFIG_KVM_MIPS_DEBUG_COP0_COUNTERS
	unsigned long stat[N_MIPS_COPROC_REGS][N_MIPS_COPROC_SEL];
#endif
};
```
## kseg0

## tlb
kvm_vz_vcpu_load_tlb

## cpu load / put
**kvm_vz_vcpu_put** && **kvm_vz_vcpu_load** register here :
```c
static struct kvm_mips_callbacks kvm_vz_callbacks = {
```
and used by:
- [x] kvm_arch_vcpu_put
- [x] kvm_arch_vcpu_load : they are used by kvm_main.c and sched/core.c

- [ ] Understand how vcpu_get , vcpu_put worked in dune ?
  - when vcpu bind to a physical cpu, if vcpu doesn't match the cpu, then ...
  - don't go to cpu until vcpu_put
  - dune: 
    - vcpu points to vmcs, load vmcs to the cpu before run vmcs_read

- [ ] ASID ?

- [ ] gc0
```c
static int kvm_vz_vcpu_load(struct kvm_vcpu *vcpu, int cpu)
{
	kvm_restore_gc0_wired(cop0);
```

## kvm_trap_vz_handle_guest_exit and kvm_mips_handle_exit

```c
	case EXCCODE_GE:
		/* defer exit accounting to handler */
		ret = kvm_mips_callbacks->handle_guest_exit(vcpu);

/* GuestCtl0.GExcCode Hypervisor exception cause codes */
#define MIPS_GCTL0_GEXC_GPSI	0  /* Guest Privileged Sensitive Instruction */
#define MIPS_GCTL0_GEXC_GSFC	1  /* Guest Software Field Change */
#define MIPS_GCTL0_GEXC_HC	2  /* Hypercall */
#define MIPS_GCTL0_GEXC_GRR	3  /* Guest Reserved Instruction Redirect */
#define MIPS_GCTL0_GEXC_GVA	8  /* Guest Virtual Address available */
#define MIPS_GCTL0_GEXC_GHFC	9  /* Guest Hardware Field Change */
#define MIPS_GCTL0_GEXC_GPA	10 /* Guest Physical Address available */
```
----> is it nested virtualization ?

```c
/*
 * Cause.ExcCode trap codes.
 */
#define EXCCODE_INT		0	/* Interrupt pending */
#define EXCCODE_MOD		1	/* TLB modified fault */
#define EXCCODE_TLBL		2	/* TLB miss on load or ifetch */
#define EXCCODE_TLBS		3	/* TLB miss on a store */
#define EXCCODE_ADEL		4	/* Address error on a load or ifetch */
#define EXCCODE_ADES		5	/* Address error on a store */
#define EXCCODE_IBE		6	/* Bus error on an ifetch */
#define EXCCODE_DBE		7	/* Bus error on a load or store */
#define EXCCODE_SYS		8	/* System call */
#define EXCCODE_BP		9	/* Breakpoint */
#define EXCCODE_RI		10	/* Reserved instruction exception */
#define EXCCODE_CPU		11	/* Coprocessor unusable */
#define EXCCODE_OV		12	/* Arithmetic overflow */
#define EXCCODE_TR		13	/* Trap instruction */
#define EXCCODE_MSAFPE		14	/* MSA floating point exception */
#define EXCCODE_FPE		15	/* Floating point exception */
#define EXCCODE_TLBRI		19	/* TLB Read-Inhibit exception */
#define EXCCODE_TLBXI		20	/* TLB Execution-Inhibit exception */
#define EXCCODE_MSADIS		21	/* MSA disabled exception */
#define EXCCODE_MDMX		22	/* MDMX unusable exception */
#define EXCCODE_WATCH		23	/* Watch address reference */
#define EXCCODE_MCHECK		24	/* Machine check */
#define EXCCODE_THREAD		25	/* Thread exceptions (MT) */
#define EXCCODE_DSPDIS		26	/* DSP disabled exception */
#define EXCCODE_GE		27	/* Virtualized guest exception (VZ) */
```

## entry.c

kvm_arch_vcpu_create ==> kvm_mips_build_exit : 
- [x] kvm_mips_build_ret_from_exit : Assemble the code to handle the return from kvm_mips_handle_exit(), either resuming the guest or returning to the host depending on the return value.
  - [ ] kvm_mips_build_ret_to_guest
  - [ ] kvm_mips_build_ret_to_host

- [ ] kvm_mips_build_vcpu_run
  - [ ] kvm_mips_build_enter_guest

- [ ] kvm_mips_build_tlb_refill_exception : *Why a special entry*


- [ ] scratch 
  - [ ] kvm_mips_build_save_scratch
  - [ ] c0_kscratch
  - [ ] kvm_mips_build_restore_scratch



## GuestCtl0 GuestCtl1


## code overview
| name              | blank | commet | code |
|-------------------|-------|--------|------|
| ls3acomp-vz.c     | 360   | 598    | 2691 |
| vz.c              | 337   | 596    | 2315 |
| emulate.c         | 441   | 656    | 2275 |
| mips.c            | 340   | 247    | 1795 |
| mmu.c             | 200   | 369    | 1010 |
| trap_emul.c       | 135   | 205    | 989  |
| entry.c           | 149   | 307    | 524  |
| tlb.c             | 115   | 142    | 464  |
| ls7a_irq.c        | 51    | 16     | 393  |
| ls3a_ext_irq.c    | 35    | 57     | 391  |
| trace.h           | 43    | 19     | 288  |
| ls3a_ht_irq.c     | 46    | 12     | 282  |
| ls3a_router_irq.c | 48    | 15     | 238  |
| ls3a_ipi.c        | 41    | 14     | 206  |
| stable_timer.c    | 54    | 243    | 203  |
| interrupt.c       | 45    | 26     | 171  |
| lasx.S            | 8     | 14     | 166  |
| msa.S             | 7     | 23     | 131  |
| ls7a_irq.h        | 27    | 36     | 122  |
| fpu.S             | 6     | 22     | 97   |
| dyntrans.c        | 20    | 33     | 90   |
| ls3a_ext_irq.h    | 11    | 11     | 79   |
| ls3a_ipi.h        | 17    | 10     | 78   |
| ls3a_router_irq.h | 9     | 17     | 71   |
| ls7a_irqfd.c      | 11    | 32     | 61   |
| callback.c        | 2     | 8      | 56   |
| ls3a_ht_irq.h     | 10    | 17     | 50   |
| stats.c           | 4     | 10     | 49   |
| hypcall.c         | 10    | 10     | 46   |
| interrupt.h       | 6     | 14     | 38   |
| ls3a.h            | 3     | 11     | 26   |
| Makefile          | 4     | 3      | 16   |
| commpage.c        | 5     | 12     | 15   |
| commpage.h        | 5     | 11     | 8    |

[^1]: https://zh.wikipedia.org/wiki/MIPS_SIMD_Architecture
