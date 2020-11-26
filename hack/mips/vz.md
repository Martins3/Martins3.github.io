# MIPS VZ

<!-- vim-markdown-toc GitLab -->

- [TODO](#todo)
- [irqchip](#irqchip)
- [what's happending in the](#whats-happending-in-the)
- [how to handle fpu](#how-to-handle-fpu)
- [async ioctl](#async-ioctl)
- [functions](#functions)
- [mmu](#mmu)
- [code overview](#code-overview)

<!-- vim-markdown-toc -->

## TODO
- [ ] what's relation with loongson vz and MIPS vz ?

- [ ] so, why we should emulate ?

- [ ] exc ?
- [ ] c0

- [ ] Makefile : without KVM_MIPS_VZ, `dyntrans.c` and `trap_emul.c` are needed
  - [ ] check code flow if `KVM_MIPS_VZ` is disabled
  - [ ] why *emulate* and *vmx* coexist in x86

- [ ] dyntrans.c

kvm_mips_check_privilege : if `EMULATE_PRIV_FAIL`, then `kvm_mips_emulate_exc`

- [ ] `kvm_trap_vz_handle_cop_unusable` : use unusable cpu ?

- [ ] `kvm_trap_vz_no_handler` : kvm_mips_callbacks::handle_break : break ?

- [ ] `kvm_trap_vz_handle_msa_disabled` : msa ?


- [ ] `kvm_trap_vz_handle_guest_exit` ?

Why so little exit handler ? `kvm_mips_handle_exit` need more 

kvm_arch_init_vm :

- [ ] vz used mu_notifier in kvm_main.c

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
- [ ] kvm_mips_guest_can_have_msa
- [ ] kvm_mips_guest_has_lasx

So, we can understand mips arch now:
- [ ] kvm_mips_get_reg
- [ ] kvm_mips_set_reg

## irqchip
Loongson add ...

## what's happending in the 

## how to handle fpu

## async ioctl

## functions
- kvm_read_c0_guest_status


## mmu
- [ ] All kinds of flush ?
  - [ ] kvm_mips_flush_gpa_pte
  - [ ] kvm_mips_flush_gva_pte


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
