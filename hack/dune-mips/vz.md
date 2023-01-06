# MIPS VZ

<!-- vim-markdown-toc GitLab -->

* [question](#question)
* [TODO](#todo)
* [irqchip](#irqchip)
* [fpu](#fpu)
* [async ioctl](#async-ioctl)
* [functions](#functions)
* [mmu](#mmu)
* [emulate.c](#emulatec)
* [exception](#exception)
* [interrupts](#interrupts)
* [hrtimer && timer](#hrtimer-timer)
* [config](#config)
* [coproc](#coproc)
* [cp0](#cp0)
* [kseg0](#kseg0)
* [tlb](#tlb)
* [asid](#asid)
* [guestid](#guestid)
* [cpu load / put](#cpu-load-put)
* [kvm_trap_vz_handle_guest_exit and kvm_mips_handle_exit](#kvm_trap_vz_handle_guest_exit-and-kvm_mips_handle_exit)
* [entry.c](#entryc)
  * [kvm_mips_build_vcpu_run](#kvm_mips_build_vcpu_run)
  * [kvm_mips_build_enter_guest](#kvm_mips_build_enter_guest)
  * [kvm_mips_build_exit](#kvm_mips_build_exit)
  * [uasm.c && uasm-mips.c](#uasmc-uasm-mipsc)
* [kscratch](#kscratch)
* [GuestCtl0 GuestCtl1](#guestctl0-guestctl1)
* [kvm_arch_vcpu_ioctl_run](#kvm_arch_vcpu_ioctl_run)
* [kmalloc](#kmalloc)
* [timer](#timer)
* [GuestCtl0 GExcCode](#guestctl0-gexccode)
* [code overview](#code-overview)

<!-- vim-markdown-toc -->


## question
- [x] when enter to guest, PWbase will be used for PGA translation
  - [x] so, the handler code is put into unmapped area. *YES*

## TODO
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



## fpu

## async ioctl

## functions
- kvm_read_c0_guest_status

## mmu
- [ ] where is the hugetlb ?

```c
/*
 * On R4000-style MMUs where a TLB entry is mapping a adjacent even / odd
 * pair of pages we only have a single global bit per pair of pages.  When
 * writing to the TLB make sure we always have the bit set for both pages
 * or none.  This macro is used to access the `buddy' of the pte we're just
 * working on.
 */
#define ptep_buddy(x)	((pte_t *)((unsigned long)(x) ^ sizeof(pte_t)))
```

- [ ] Something related to emualted:
  - [x] `mapped` and `unmapped`
    - [x] kvm_mips_gpa_pte_to_gva_unmapped, kvm_mips_gpa_pte_to_gva_mapped
    - [x] kvm_mips_handle_kseg0_tlb_fault <= kvm_trap_emul_gva_fault <= kvm_get_inst <= used by emulate.c
  - [ ] others functions is used by emulate ?
    - [ ] what does emulate mean ? shadow page ? what shold you emulate ?
  - [x] kseg0
    - kvm_mips_handle_kseg0_tlb_fault
  - x: kvm_trap_emul_gva_fault
    - x: kvm_get_inst
  - kvm_mips_handle_mapped_seg_tlb_fault : caller is emulate

## emulate.c
- [ ] kvm_mips_emulate_CP0 : so many, but not used by kvm
- [ ] kvm_mips_emulate_load : called by vz.c, with kvm_trap_vz_handle_tlb_ld_miss, used for *MMIO*

## exception
- [ ] cpu_has_ebase_wg

- [x] kvm_arch_vcpu_create :
  - SMR 372: As the CPU fetches instructions from the exception entry point, it also flips on the exception state bit SR(EXL), which will make it insensitive to further
interrupts and puts it in kernel-privilege mode. It will go to the general exception entry point, at 0x8000.0180.

## interrupts
> KVM 可以将中断配置为直通虚拟机中断， 此时将 GuestCtl0.PIP 置 1 即可
>
> 通过 KVM 使用 mtc0 指令写 GuestCtl2.VIP 而使得 Guest.Cause.IP 置位， 用 HW 表示外部硬件中断

kvm_vz_vcpu_run && kvm_mips_handle_exit => kvm_mips_deliver_interrupts => `kvm_mips_callbacks->irq_deliver` / kvm_vz_irq_deliver_cb

```c
void kvm_mips_deliver_interrupts(struct kvm_vcpu *vcpu, u32 cause)
{
	unsigned long *pending = &vcpu->arch.pending_exceptions;
	unsigned long *pending_clr = &vcpu->arch.pending_exceptions_clr;
	unsigned int priority;

	if (!(*pending) && !(*pending_clr))
		return;

	priority = __ffs(*pending_clr);
	while (priority <= MIPS_EXC_MAX) {
		if (kvm_mips_callbacks->irq_clear(vcpu, priority, cause)) {
			if (!KVM_MIPS_IRQ_CLEAR_ALL_AT_ONCE)
				break;
		}

		priority = find_next_bit(pending_clr,
					 BITS_PER_BYTE * sizeof(*pending_clr),
					 priority + 1);
	}

	priority = __ffs(*pending);
	while (priority <= MIPS_EXC_MAX) {
		if (kvm_mips_callbacks->irq_deliver(vcpu, priority, cause)) {
			if (!KVM_MIPS_IRQ_DELIVER_ALL_AT_ONCE)
				break;
		}

		priority = find_next_bit(pending,
					 BITS_PER_BYTE * sizeof(*pending),
					 priority + 1);
	}

}
```
So, it's clear, when guest exit, deliver interrupt to guest with `set_c0_guestctl2`

## hrtimer && timer
kvm_mips_callbacks::queue_timer_int

- [ ] what's happending in vz about hrtimer ?

在 KVM 初始化时， 定义了一个内核高精度定时器 hrtimer， 它的用处就是在
guest 被调度时根据目前 count 和 compare 的差值进行高精度定时， 定时器触发时
发生进程切换， 内核调度该定时器的 guest 进程立即执行并注入一个 TI。

```c
static struct kvm_timer_callbacks kvm_vz_timer_callbacks = {
	.restore_timer = kvm_vz_restore_timer,
	.acquire_htimer = kvm_vz_acquire_htimer,
	.save_timer = kvm_vz_save_timer,
	.lose_htimer = kvm_vz_lose_htimer,
	.count_timeout = kvm_mips_count_timeout,
	.write_count = kvm_mips_write_count,
};
```

## config
- [ ] kvm_vz_config1_user_wrmask
- [ ] kvm_vz_config1_guest_wrmask

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

## cp0
```c
struct kvm_vcpu_arch {

	/* Host CP0 registers used when handling exits from guest */
	unsigned long host_cp0_badvaddr;
	unsigned long host_cp0_epc;
	u32 host_cp0_cause;
	u32 host_cp0_guestctl0;
	u32 host_cp0_badinstr;
	u32 host_cp0_badinstrp;
	u32 host_cp0_gscause;
```


```c
/*
 * Bitfields and bit numbers in the coprocessor 0 cause register.
 *
 * Refer to your MIPS R4xx0 manual, chapter 5 for explanation.
 */
#define CAUSEB_EXCCODE		2
#define CAUSEF_EXCCODE		(_ULCAST_(31)  <<  2)
#define CAUSEB_IP		8
#define CAUSEF_IP		(_ULCAST_(255) <<  8)
#define	 CAUSEB_IP0		8
#define	 CAUSEF_IP0		(_ULCAST_(1)   <<  8)
#define	 CAUSEB_IP1		9
#define	 CAUSEF_IP1		(_ULCAST_(1)   <<  9)
#define	 CAUSEB_IP2		10
#define	 CAUSEF_IP2		(_ULCAST_(1)   << 10)
#define	 CAUSEB_IP3		11
#define	 CAUSEF_IP3		(_ULCAST_(1)   << 11)
#define	 CAUSEB_IP4		12
#define	 CAUSEF_IP4		(_ULCAST_(1)   << 12)
#define	 CAUSEB_IP5		13
#define	 CAUSEF_IP5		(_ULCAST_(1)   << 13)
#define	 CAUSEB_IP6		14
#define	 CAUSEF_IP6		(_ULCAST_(1)   << 14)
#define	 CAUSEB_IP7		15
#define	 CAUSEF_IP7		(_ULCAST_(1)   << 15)
#define CAUSEB_FDCI		21
#define CAUSEF_FDCI		(_ULCAST_(1)   << 21)
#define CAUSEB_WP		22
#define CAUSEF_WP		(_ULCAST_(1)   << 22)
#define CAUSEB_IV		23
#define CAUSEF_IV		(_ULCAST_(1)   << 23)
#define CAUSEB_PCI		26
#define CAUSEF_PCI		(_ULCAST_(1)   << 26)
#define CAUSEB_DC		27
#define CAUSEF_DC		(_ULCAST_(1)   << 27)
#define CAUSEB_CE		28
#define CAUSEF_CE		(_ULCAST_(3)   << 28)
#define CAUSEB_TI		30
#define CAUSEF_TI		(_ULCAST_(1)   << 30)
#define CAUSEB_BD		31
#define CAUSEF_BD		(_ULCAST_(1)   << 31)
```


## kseg0

## tlb
- [ ] kvm_vz_vcpu_load_tlb
  - everytime enter guest

Let's find what lives in tlb.c

```c
struct kvm_mips_tlb {
	long tlb_mask;
	long tlb_hi;
	long tlb_lo[2];
};

struct kvm_vcpu_arch {
  // ...

	/* S/W Based TLB for guest */
	struct kvm_mips_tlb guest_tlb[KVM_MIPS_GUEST_TLB_SIZE];
```

flush root / guest tlb, inv tlb when update `gpa->hpa`

## asid
```c
static u32 kvm_mips_get_kernel_asid(struct kvm_vcpu *vcpu)
{
	struct mm_struct *kern_mm = &vcpu->arch.guest_kernel_mm;
	int cpu = smp_processor_id();

	return cpu_asid(cpu, kern_mm);
}

static u32 kvm_mips_get_user_asid(struct kvm_vcpu *vcpu)
{
	struct mm_struct *user_mm = &vcpu->arch.guest_user_mm;
	int cpu = smp_processor_id();

	return cpu_asid(cpu, user_mm);
}
```
- [ ] How kernel maintains the asid
- [ ] How asid loaded to TLB EntryHi ?

kvm_mips_host_tlb_inv need `va` and `asid` to assemble `EntryHi`.

## guestid
- clear_root_gid
- set_root_gid_to_guest_gid


## cpu load / put
调用链条 :
1. vcpu_load
  - preempt_notifier_register : 这让只要 process 被换出，就执行 kvm_sched_out => kvm_arch_vcpu_put
  - kvm_arch_vcpu_load
    - `kvm_mips_callbacks->vcpu_load` => kvm_vz_vcpu_load

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
- [ ] **four** TLB entrys
  - [ ] EXCCODE_TLBRI, EXCCODE_TLBXI used for ad bit ?
    - [ ] cross/arch/mips/kernel/traps.c : host need it too.
    - [ ] Only loongson implement it, with same hanlder with `handle_tlb_ld_miss`

- [ ] .handle_tlb_mod and .handle_tlb_ld_miss use same handler ?

```c
	.handle_tlb_mod = kvm_trap_vz_handle_tlb_st_miss,
	.handle_tlb_ld_miss = kvm_trap_vz_handle_tlb_ld_miss, // ??? can't find the physical page ?
	.handle_tlb_st_miss = kvm_trap_vz_handle_tlb_st_miss, // ???
```



```c
// cross/arch/mips/kernel/traps.c
void __init trap_init(void)
{
  // ...
	if (cpu_has_rixiex) {
		set_except_vector(EXCCODE_TLBRI, tlb_do_page_fault_0);
		set_except_vector(EXCCODE_TLBXI, tlb_do_page_fault_0);
	}
  // ...
```

## entry.c
kvm_arch_vcpu_create
==> `kvm_mips_build_exception` will build exceptions which jumps to `kvm_mips_build_exit`
==> `kvm_mips_build_tlb_refill_exception` : special entrance for tlb refill

- [x] kvm_mips_build_ret_from_exit : Assemble the code to handle the return from kvm_mips_handle_exit(), either resuming the guest or returning to the host depending on the return value.
  - [x] kvm_mips_build_ret_to_guest
  - [x] kvm_mips_build_ret_to_host

- [x] kvm_mips_build_vcpu_run
  - [x] kvm_mips_build_enter_guest
    - [ ] tlbmiss_handler_setup_pgd : initialized in `build_setup_pgd`, used for setup a new pgd, called in `kvm_mips_build_enter_guest` and `kvm_mips_build_exit`
      - [ ]


- [x] scratch
  - [x] kvm_mips_build_save_scratch : scratch_vcpu => cp0_epc scratch_vcpu => cp0_cause
  - [x] kvm_mips_build_restore_scratch
  - [x] c0_kscratch
  - In `kvm_mips_build_exception`, `UASM_i_MFC0(&p, K1, scratch_vcpu[0], scratch_vcpu[1]);` imply the cpo UASM_i_MFC0 currently using is guest's cp0

### kvm_mips_build_vcpu_run
- [ ] C0_STATUS

### kvm_mips_build_enter_guest
- [ ] `#define MIPS_CPU_LDPTE		MBIT_ULL(41)	/* CPU has ldpte/lddir instructions */`


### kvm_mips_build_exit
- [ ] kvm_mips_entry_setup : so, we will
  - [ ] K1 : address of `kvm_vcpu_arch` : ???
    - [ ] what if we have more than one vcpu : ??
    - [ ] which convention specify this ? -> so how we launch the vcpu ?
  - [ ] S1 / S0
  - [ ] build label
  - [ ] VZ : means ?
    - [ ] vpid
    - [ ] CPU uses VZ ASE GuestID feature : MIPS_CPU_GUESTID
  - [ ] some field kvm_vcpu_arch : pure load and store without more action
```c
static unsigned int scratch_vcpu[2] = { C0_DDATA_LO };
static unsigned int scratch_tmp[2] = { C0_ERROREPC };

/*
 * The version of this function in tlbex.c uses current_cpu_type(), but for KVM
 * we assume symmetry.
 */
static int c0_kscratch(void)
{
	switch (boot_cpu_type()) {
	case CPU_XLP:
	case CPU_XLR:
		return 22;
	default:
		return 31; // DESAVE
	}
}

/**
 * kvm_mips_entry_setup() - Perform global setup for entry code.
 *
 * Perform global setup for entry code, such as choosing a scratch register.
 *
 * Returns:	0 on success.
 *		-errno on failure.
 */
int kvm_mips_entry_setup(void)
{
	/*
	 * We prefer to use KScratchN registers if they are available over the
	 * defaults above, which may not work on all cores.
	 */
	unsigned int kscratch_mask = cpu_data[0].kscratch_mask;

	if (pgd_reg != -1)
		kscratch_mask &= ~BIT(pgd_reg);

	/* Pick a scratch register for storing VCPU */
	if (kscratch_mask) {
		scratch_vcpu[0] = c0_kscratch();
		scratch_vcpu[1] = ffs(kscratch_mask) - 1;
		kscratch_mask &= ~BIT(scratch_vcpu[1]);
	}

	/* Pick a scratch register to use as a temp for saving state */
	if (kscratch_mask) {
		scratch_tmp[0] = c0_kscratch();
		scratch_tmp[1] = ffs(kscratch_mask) - 1;
		kscratch_mask &= ~BIT(scratch_tmp[1]);
	}

	return 0;
}

```



### uasm.c && uasm-mips.c
- [ ] What's the relation of uasm.c && uasm-mips.c ?

- [ ] enum fields
- [ ] mask
- [x] enum opcode : second parameters of `build_insn`
- [ ] build_rs :
- [ ] build_insn

- [x] label && relocation

- [x] Ip / I : p means prototype
- [ ] uasm / UASM

- [ ] UASM_i_LA_mostly
- [ ] UASM_i_LA

```c
#define Ip_u2u1u3(op)							\
void uasm_i##op(u32 **buf, unsigned int a, unsigned int b, unsigned int c)

Ip_u2u1u3(_srl);

#define I_u2u1s3(op)					\
Ip_u2u1s3(op)						\
{							\
	build_insn(buf, insn##op, b, a, c);		\
}							\
UASM_EXPORT_SYMBOL(uasm_i##op);
```
===> uasm_i_srl

```c
/*
 * The order of opcode arguments is implicitly left to right,
 * starting with RS and ending with FUNC or IMM.
 */
static void build_insn(u32 **buf, enum opcode opc, ...)
{
	const struct insn *ip;
	va_list ap;
	u32 op;

	if (opc < 0 || opc >= insn_invalid ||
	    (opc == insn_daddiu && r4k_daddiu_bug()) ||
	    (insn_table[opc].match == 0 && insn_table[opc].fields == 0))
		panic("Unsupported Micro-assembler instruction %d", opc);

	ip = &insn_table[opc];

	op = ip->match;
	va_start(ap, opc);
	if (ip->fields & RS)
		op |= build_rs(va_arg(ap, u32));
	if (ip->fields & SIMM9)
		op |= build_scimm9(va_arg(ap, u32));
	va_end(ap);

	**buf = op;
	(*buf)++;
}
```

```c
void uasm_il_beqzl(u32 **p, struct uasm_reloc **r, unsigned int reg,
			    int lid)
{
	uasm_r_mips_pc16(r, *p, lid);
	uasm_i_beqzl(p, reg, 0);
}
```

## kscratch


## GuestCtl0 GuestCtl1
seems only used in loongson vz ?

- kvm_sched_in && vcpu_load && vcpu_put

struct kvm_vcpu_arch::cop0 : keep all the guest cop0, even exit from guest, cop0 will not be saved automatically.

- [ ] mtgc0 mfgc0 hypcall

## kvm_arch_vcpu_ioctl_run
- kvm_arch_vcpu_ioctl_run
  - kvm_vz_vcpu_load_tlb
  - kvm_vz_vcpu_run
    - kvm_mips_build_enter_guest

## kmalloc
```c
#define CAC_BASE        _AC(0x9800000000000000, UL)
#define PAGE_OFFSET		(CAC_BASE + PHYS_OFFSET)
#define PHYS_OFFSET		_AC(0, UL)
#define __va(x)		((void *)((unsigned long)(x) + PAGE_OFFSET - PHYS_OFFSET))
```
pa + 0x9800 0000 0000 0000 = va

in the `xkphys` area



## timer
1. kvm_timer_callbacks::count_timeout
```c
/* low level hrtimer wake routine */
static enum hrtimer_restart kvm_mips_comparecount_wakeup(struct hrtimer *timer)
{
	struct kvm_vcpu *vcpu;

	vcpu = container_of(timer, struct kvm_vcpu, arch.comparecount_timer);
	kvm_mips_comparecount_func((unsigned long) vcpu);
	return kvm_timer_callbacks->count_timeout(vcpu);
}

int kvm_arch_vcpu_init(struct kvm_vcpu *vcpu)
{
	int err;

	err = kvm_mips_callbacks->vcpu_init(vcpu);
	if (err)
		return err;

	hrtimer_init(&vcpu->arch.comparecount_timer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_REL);
	vcpu->arch.comparecount_timer.function = kvm_mips_comparecount_wakeup;
	return 0;
}
```
2. kvm_timer_callbacks::lose_htimer
  - kvm_vz_gpsi_cop0
  - kvm_trap_vz_handle_gsfc


3. kvm_timer_callbacks::restore_timer
  - kvm_vz_vcpu_load

4. kvm_timer_callbacks::save_timer
  - kvm_vz_vcpu_put

5. kvm_timer_callbacks::write_count
  - only mips vz register, but loongson use it.

```c
static enum emulation_result kvm_vz_gpsi_cop0(union mips_instruction inst,
					      u32 *opc, u32 cause,
					      struct kvm_run *run,
					      struct kvm_vcpu *vcpu)
{
        // ...
				kvm_timer_callbacks->lose_htimer(vcpu);
				if(!vcpu->kvm->arch.use_stable_timer)
					kvm_timer_callbacks->write_count(vcpu, vcpu->arch.gprs[rt]);

static int kvm_vz_set_one_reg(struct kvm_vcpu *vcpu,
			      const struct kvm_one_reg *reg,
			      s64 v)
{

  // ...
	case KVM_REG_MIPS_CP0_COUNT:
		if(!vcpu->kvm->arch.use_stable_timer)
			kvm_timer_callbacks->write_count(vcpu, v);
		else
			kvm_timer_callbacks->write_stable_timer(vcpu, v);
```

6. kvm_timer_callbacks::acquire_htimer
  - kvm_vz_vcpu_run
  - kvm_mips_handle_exit : when `ret == RESUME_GUEST`

Call it everytime enter guest os.

```c
/**
 * kvm_vz_should_use_htimer() - Find whether to use the VZ hard guest timer.
 * @vcpu:	Virtual CPU.
 *
 * Returns:	true if the VZ GTOffset & real guest CP0_Count should be used
 *		instead of software emulation of guest timer.
 *		false otherwise.
 */
static bool kvm_vz_should_use_htimer(struct kvm_vcpu *vcpu)
{
	if (kvm_mips_count_disabled(vcpu))
		return false;

	/* Chosen frequency must match real frequency */
	if (mips_hpt_frequency != vcpu->arch.count_hz)
		return false;

	/* We don't support a CP0_GTOffset with fewer bits than CP0_Count */
	if (current_cpu_data.gtoffset_mask != 0xffffffff)
		return false;

	return true;
}
```

## GuestCtl0 GExcCode
virtualization manual : chapter 4.7.7, table 5.3

`Table 4.8 CP0 Registers in Guest CP0 context` :

## code overview
| name              | blank | commet | code |
|-------------------|-------|--------|------|
| vz.c              | 337   | 596    | 2315 |
| mips.c            | 340   | 247    | 1795 |
| mmu.c             | 200   | 369    | 1010 |
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
