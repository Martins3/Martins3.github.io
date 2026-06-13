Domain U HVM Guests 利用 Qemu-DM[^1]

A new feature in Xen designed to improve overall performance and reduce the load on
the Dom 0 Guest is PCI Passthru which allows the Domain U Guest to have direct access
to local hardware without using the Domain 0 for hardware access.[^1]

Domain 0 has responsibility for all devices on the system.
Normally, as it discovers PCI devices, it passes those to drivers within the Linux kernel.
In order for a device to be accessed by a guest, the device must instead be assigned to a special domain 0 driver.
This driver is called xen-pciback in pvops kernels, and called pciback in classic kernels.
PV guests access the device via a kernel driver in the guest called xen-pcifront (pcifront in classic xen kernels),
which connects to pciback. HVM guests see the device on the emulated PCI bus presented by QEMU.[^2]

[^1]: http://www-archive.xenproject.org/files/Marketing/HowDoesXenWork.pdf
[^2]: https://wiki.xenproject.org/wiki/Xen_PCI_Passthrough

## linux 中 read_cr2 在 xen 中将会不同

```txt
Symbol: PARAVIRT_XXL [=n]
Type  : bool
Defined at arch/x86/Kconfig:794
  Depends on: HYPERVISOR_GUEST [=y]
Selected by [n]:
  - XEN_PV [=n] && HYPERVISOR_GUEST [=y] && XEN [=n] && X86_64 [=y]
```

当 PARAVIRT_XXL 打开的时候发
```c
static inline unsigned long read_cr2(void)
{
  return PVOP_CALL0(unsigned long, pv_mmu_ops.read_cr2);
}
```

更加有趣的定义在: arch/x86/kernel/paravirt.c 中的

```c
struct paravirt_patch_template pv_ops = {
	/* Cpu ops. */
	.cpu.io_delay		= native_io_delay,

#ifdef CONFIG_PARAVIRT_XXL
	.cpu.cpuid		= native_cpuid,
	.cpu.get_debugreg	= pv_native_get_debugreg,
	.cpu.set_debugreg	= pv_native_set_debugreg,
	.cpu.read_cr0		= native_read_cr0,
	.cpu.write_cr0		= native_write_cr0,
	.cpu.write_cr4		= native_write_cr4,
	.cpu.wbinvd		= pv_native_wbinvd,
	.cpu.read_msr		= native_read_msr,
	.cpu.write_msr		= native_write_msr,
	.cpu.read_msr_safe	= native_read_msr_safe,
	.cpu.write_msr_safe	= native_write_msr_safe,
	.cpu.read_pmc		= native_read_pmc,
	.cpu.load_tr_desc	= native_load_tr_desc,
	.cpu.set_ldt		= native_set_ldt,
	.cpu.load_gdt		= native_load_gdt,
	.cpu.load_idt		= native_load_idt,
	.cpu.store_tr		= native_store_tr,
	.cpu.load_tls		= native_load_tls,
	.cpu.load_gs_index	= native_load_gs_index,
	.cpu.write_ldt_entry	= native_write_ldt_entry,
	.cpu.write_gdt_entry	= native_write_gdt_entry,
	.cpu.write_idt_entry	= native_write_idt_entry,

	.cpu.alloc_ldt		= paravirt_nop,
	.cpu.free_ldt		= paravirt_nop,

	.cpu.load_sp0		= native_load_sp0,

#ifdef CONFIG_X86_IOPL_IOPERM
	.cpu.invalidate_io_bitmap	= native_tss_invalidate_io_bitmap,
	.cpu.update_io_bitmap		= native_tss_update_io_bitmap,
#endif

	.cpu.start_context_switch	= paravirt_nop,
	.cpu.end_context_switch		= paravirt_nop,

	/* Irq ops. */
	.irq.save_fl		= __PV_IS_CALLEE_SAVE(native_save_fl),
	.irq.irq_disable	= __PV_IS_CALLEE_SAVE(pv_native_irq_disable),
	.irq.irq_enable		= __PV_IS_CALLEE_SAVE(pv_native_irq_enable),
	.irq.safe_halt		= pv_native_safe_halt,
	.irq.halt		= native_halt,
#endif /* CONFIG_PARAVIRT_XXL */

	/* Mmu ops. */
	.mmu.flush_tlb_user	= native_flush_tlb_local,
	.mmu.flush_tlb_kernel	= native_flush_tlb_global,
	.mmu.flush_tlb_one_user	= native_flush_tlb_one_user,
	.mmu.flush_tlb_multi	= native_flush_tlb_multi,
	.mmu.tlb_remove_table	=
			(void (*)(struct mmu_gather *, void *))tlb_remove_page,

	.mmu.exit_mmap		= paravirt_nop,
	.mmu.notify_page_enc_status_changed	= paravirt_nop,

#ifdef CONFIG_PARAVIRT_XXL
	.mmu.read_cr2		= __PV_IS_CALLEE_SAVE(pv_native_read_cr2),
	.mmu.write_cr2		= pv_native_write_cr2,
	.mmu.read_cr3		= __native_read_cr3,
	.mmu.write_cr3		= native_write_cr3,

	.mmu.pgd_alloc		= __paravirt_pgd_alloc,
	.mmu.pgd_free		= paravirt_nop,

	.mmu.alloc_pte		= paravirt_nop,
	.mmu.alloc_pmd		= paravirt_nop,
	.mmu.alloc_pud		= paravirt_nop,
	.mmu.alloc_p4d		= paravirt_nop,
	.mmu.release_pte	= paravirt_nop,
	.mmu.release_pmd	= paravirt_nop,
	.mmu.release_pud	= paravirt_nop,
	.mmu.release_p4d	= paravirt_nop,

	.mmu.set_pte		= native_set_pte,
	.mmu.set_pmd		= native_set_pmd,

	.mmu.ptep_modify_prot_start	= __ptep_modify_prot_start,
	.mmu.ptep_modify_prot_commit	= __ptep_modify_prot_commit,

	.mmu.set_pud		= native_set_pud,

	.mmu.pmd_val		= PTE_IDENT,
	.mmu.make_pmd		= PTE_IDENT,

	.mmu.pud_val		= PTE_IDENT,
	.mmu.make_pud		= PTE_IDENT,

	.mmu.set_p4d		= native_set_p4d,

#if CONFIG_PGTABLE_LEVELS >= 5
	.mmu.p4d_val		= PTE_IDENT,
	.mmu.make_p4d		= PTE_IDENT,

	.mmu.set_pgd		= native_set_pgd,
#endif /* CONFIG_PGTABLE_LEVELS >= 5 */

	.mmu.pte_val		= PTE_IDENT,
	.mmu.pgd_val		= PTE_IDENT,

	.mmu.make_pte		= PTE_IDENT,
	.mmu.make_pgd		= PTE_IDENT,

	.mmu.dup_mmap		= paravirt_nop,
	.mmu.activate_mm	= paravirt_nop,

	.mmu.lazy_mode = {
		.enter		= paravirt_nop,
		.leave		= paravirt_nop,
		.flush		= paravirt_nop,
	},

	.mmu.set_fixmap		= native_set_fixmap,
#endif /* CONFIG_PARAVIRT_XXL */

#if defined(CONFIG_PARAVIRT_SPINLOCKS)
	/* Lock ops. */
#ifdef CONFIG_SMP
	.lock.queued_spin_lock_slowpath	= native_queued_spin_lock_slowpath,
	.lock.queued_spin_unlock	=
				PV_CALLEE_SAVE(__native_queued_spin_unlock),
	.lock.wait			= paravirt_nop,
	.lock.kick			= paravirt_nop,
	.lock.vcpu_is_preempted		=
				PV_CALLEE_SAVE(__native_vcpu_is_preempted),
#endif /* SMP */
#endif
};
```
