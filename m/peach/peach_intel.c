// SPDX-License-Identifier: GPL-2.0
#include <linux/capability.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <asm/desc.h>
#include <asm/debugreg.h>
#include <asm/msr.h>
#include <asm/special_insns.h>
#include <asm/vmx.h>

#include "peach.h"
#include "guest.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ScratchLab");
MODULE_DESCRIPTION("Peach nested VMX entry/exit demo for KVM guests");

static dev_t peach_dev = MKDEV(PEACH_MAJOR, PEACH_MINOR);
static struct cdev peach_cdev;
static DEFINE_MUTEX(peach_lock);

/* 与 vmexit_handler.S 的偏移一致；RSP 单独存放在 VMCS 中。 */
struct guest_regs {
	u64 rax, rcx, rdx, rbx, rbp, rsi, rdi;
	u64 r8, r9, r10, r11, r12, r13, r14, r15;
};
static_assert(sizeof(struct guest_regs) == 15 * sizeof(u64));
static_assert(offsetof(struct guest_regs, rdi) == 48);
static_assert(offsetof(struct guest_regs, r15) == 112);
extern int peach_enter(struct guest_regs *regs, unsigned long resume);

struct peach_vm {
	unsigned long vmxon, vmcs, memory, ept;
	struct guest_regs regs;
	u64 eptp;
	bool replay_nmi;
	unsigned int nr_exits;
	struct {
		unsigned long reason, rip;
		u64 ax, bx, cx;
	} exits[8];
};

static int vmwrite(unsigned long field, unsigned long value)
{
	u8 failed;

	asm volatile("vmwrite %2, %1; setna %0"
		     : "=qm"(failed)
		     : "r"(field), "r"(value)
		     : "cc", "memory");
	if (failed)
		pr_err("peach: VMWRITE field=%#lx failed\n", field);
	return failed ? -EIO : 0;
}

static unsigned long vmread(unsigned long field)
{
	unsigned long value;

	asm volatile("vmread %1, %0" : "=r"(value) : "r"(field) : "cc");
	return value;
}

#define W(field, value)                    \
	do {                               \
		if (vmwrite(field, value)) \
			return -EIO;       \
	} while (0)

static int write_control(unsigned long field, u32 msr, u32 wanted)
{
	u64 cap = native_read_msr(msr);
	u32 value = (wanted | (u32)cap) & (cap >> 32);

	if ((value & wanted) != wanted)
		return -EOPNOTSUPP;
	return vmwrite(field, value);
}

static int setup_controls(struct peach_vm *vm, u64 basic)
{
	int ret;
	bool true_ctls = basic & VMX_BASIC_TRUE_CTLS;

	/* 外部中断不在 VM-exit 时确认，回到 Linux 后由正常 IRQ 路径处理。 */
	ret = write_control(PIN_BASED_VM_EXEC_CONTROL,
			    true_ctls ? MSR_IA32_VMX_TRUE_PINBASED_CTLS :
					MSR_IA32_VMX_PINBASED_CTLS,
			    PIN_BASED_EXT_INTR_MASK | PIN_BASED_NMI_EXITING);
	if (ret)
		return ret;
	ret = write_control(CPU_BASED_VM_EXEC_CONTROL,
			    true_ctls ? MSR_IA32_VMX_TRUE_PROCBASED_CTLS :
					MSR_IA32_VMX_PROCBASED_CTLS,
			    CPU_BASED_HLT_EXITING |
				    CPU_BASED_ACTIVATE_SECONDARY_CONTROLS);
	if (ret)
		return ret;
	ret = write_control(
		SECONDARY_VM_EXEC_CONTROL, MSR_IA32_VMX_PROCBASED_CTLS2,
		SECONDARY_EXEC_ENABLE_EPT | SECONDARY_EXEC_UNRESTRICTED_GUEST);
	if (ret)
		return ret;
	ret = write_control(VM_EXIT_CONTROLS,
			    true_ctls ? MSR_IA32_VMX_TRUE_EXIT_CTLS :
					MSR_IA32_VMX_EXIT_CTLS,
			    VM_EXIT_HOST_ADDR_SPACE_SIZE |
				    VM_EXIT_LOAD_IA32_EFER);
	if (ret)
		return ret;
	ret = write_control(VM_ENTRY_CONTROLS,
			    true_ctls ? MSR_IA32_VMX_TRUE_ENTRY_CTLS :
					MSR_IA32_VMX_ENTRY_CTLS,
			    VM_ENTRY_LOAD_IA32_EFER);
	if (ret)
		return ret;
	W(VMCS_LINK_POINTER, ~0ULL);
	W(EPT_POINTER, vm->eptp);
	W(EXCEPTION_BITMAP, ~0U);
	return 0;
}

static int setup_guest(void)
{
	unsigned int i;
	u64 cr0, cr4;

	/* 16 位实模式 guest，EPT 将 GPA 0 起的 64 KiB 映射到私有内存。 */
	for (i = 0; i < 6; i++) {
		W(GUEST_ES_SELECTOR + i * 2, 0);
		W(GUEST_ES_BASE + i * 2, 0);
		W(GUEST_ES_LIMIT + i * 2, 0xffff);
		W(GUEST_ES_AR_BYTES + i * 2, i == 1 ? 0x9b : 0x93);
	}
	W(GUEST_LDTR_AR_BYTES, 0x10000);
	W(GUEST_TR_AR_BYTES, 0x8b);
	W(GUEST_TR_LIMIT, 0x67);
	W(GUEST_GDTR_LIMIT, 0xffff);
	W(GUEST_IDTR_LIMIT, 0xffff);
	cr0 = native_read_msr(MSR_IA32_VMX_CR0_FIXED0);
	cr0 &= ~(X86_CR0_PE | X86_CR0_PG);
	cr4 = native_read_msr(MSR_IA32_VMX_CR4_FIXED0);
	W(GUEST_CR0, cr0);
	W(GUEST_CR4, cr4);
	W(GUEST_IA32_EFER, 0);
	W(GUEST_DR7, 0x400);
	W(GUEST_RSP, 0xfff0);
	W(GUEST_RIP, 0);
	W(GUEST_RFLAGS, 2);
	return 0;
}

static int setup_host(const struct desc_ptr *gdt, const struct desc_ptr *idt)
{
	unsigned short sel;
	struct ldttss_desc *tr;
	u64 tr_base;

#define HOST_SEG(name, reg)            \
	do {                           \
		savesegment(reg, sel); \
		W(name, sel & ~7);     \
	} while (0)
	HOST_SEG(HOST_ES_SELECTOR, es);
	HOST_SEG(HOST_CS_SELECTOR, cs);
	HOST_SEG(HOST_SS_SELECTOR, ss);
	HOST_SEG(HOST_DS_SELECTOR, ds);
	HOST_SEG(HOST_FS_SELECTOR, fs);
	HOST_SEG(HOST_GS_SELECTOR, gs);
#undef HOST_SEG
	store_tr(sel);
	W(HOST_TR_SELECTOR, sel & ~7);
	tr = (struct ldttss_desc *)(gdt->address + (sel & ~7));
	tr_base = tr->base0 | ((u64)tr->base1 << 16) | ((u64)tr->base2 << 24) |
		  ((u64)tr->base3 << 32);
	W(HOST_TR_BASE, tr_base);
	W(HOST_GDTR_BASE, gdt->address);
	W(HOST_IDTR_BASE, idt->address);
	W(HOST_CR0, read_cr0());
	W(HOST_CR3, __read_cr3());
	W(HOST_CR4, __read_cr4());
	W(HOST_FS_BASE, native_read_msr(MSR_FS_BASE));
	W(HOST_GS_BASE, native_read_msr(MSR_GS_BASE));
	W(HOST_IA32_EFER, native_read_msr(MSR_EFER));
	W(HOST_IA32_SYSENTER_CS, native_read_msr(MSR_IA32_SYSENTER_CS));
	W(HOST_IA32_SYSENTER_ESP, native_read_msr(MSR_IA32_SYSENTER_ESP));
	W(HOST_IA32_SYSENTER_EIP, native_read_msr(MSR_IA32_SYSENTER_EIP));
	return 0;
}
#undef W

static void init_ept(struct peach_vm *vm)
{
	u64 *p = (u64 *)vm->ept;
	unsigned int i;

	p[0] = __pa(vm->ept + PAGE_SIZE) | 7;
	p[512] = __pa(vm->ept + 2 * PAGE_SIZE) | 7;
	p[1024] = __pa(vm->ept + 3 * PAGE_SIZE) | 7;
	for (i = 0; i < 16; i++)
		p[1536 + i] = __pa(vm->memory + i * PAGE_SIZE) | (6 << 3) | 7;
	/* 不启用可选的 EPT A/D 和 VPID，减少对嵌套 VMX 的能力要求。 */
	vm->eptp = __pa(vm->ept) | VMX_EPTP_PWL_4 | 6;
}

static int invalidate_ept(struct peach_vm *vm)
{
	struct {
		u64 eptp, reserved;
	} desc = { vm->eptp, 0 };
	u8 failed;

	/* VMXOFF 不保证清除 EPT 缓存，重复运行前必须失效旧映射。 */
	asm volatile("invept %1, %2; setna %0"
		     : "=qm"(failed)
		     : "m"(desc), "r"(1UL)
		     : "cc", "memory");
	return failed ? -EIO : 0;
}

static int run_guest(struct peach_vm *vm)
{
	unsigned long reason, rip;
	int i;

	for (i = 0; i < 8; i++) {
		if (peach_enter(&vm->regs, i != 0)) {
			pr_err("peach: VM-entry failed, instruction_error=%lu\n",
			       vmread(VM_INSTRUCTION_ERROR));
			return -EIO;
		}
		reason = vmread(VM_EXIT_REASON);
		rip = vmread(GUEST_RIP);
		/* 先保存证据，VMXOFF 并恢复 IRQ 后再打印，避免串口放大临界区。 */
		vm->exits[i].reason = reason;
		vm->exits[i].rip = rip;
		vm->exits[i].ax = vm->regs.rax;
		vm->exits[i].bx = vm->regs.rbx;
		vm->exits[i].cx = vm->regs.rcx;
		vm->nr_exits++;
		switch (reason) {
		case EXIT_REASON_CPUID:
			vm->regs.rax = 0x6368;
			vm->regs.rbx = 0x6561;
			vm->regs.rcx = 0x70;
			vm->regs.rdx = 0;
			if (vmwrite(GUEST_RIP,
				    rip + vmread(VM_EXIT_INSTRUCTION_LEN)))
				return -EIO;
			break;
		case EXIT_REASON_HLT:
			return i == 1 && rip == 16 && vm->regs.rax == 0x4348 &&
					       vm->regs.rbx == 0x4541 &&
					       vm->regs.rcx == 0xe050 ?
				       0 :
				       -EIO;
		case EXIT_REASON_EXTERNAL_INTERRUPT:
			return -EAGAIN;
		case EXIT_REASON_EXCEPTION_NMI:
			if ((vmread(VM_EXIT_INTR_INFO) &
			     (INTR_INFO_VALID_MASK | INTR_INFO_INTR_TYPE_MASK |
			      INTR_INFO_VECTOR_MASK)) ==
			    (INTR_INFO_VALID_MASK | INTR_TYPE_NMI_INTR | 2)) {
				vm->replay_nmi = true;
				return -EAGAIN;
			}
			fallthrough;
		default:
			pr_err("peach: unexpected exit, qualification=%#lx intr_info=%#lx\n",
			       vmread(EXIT_QUALIFICATION),
			       vmread(VM_EXIT_INTR_INFO));
			return -EIO;
		}
	}
	return -ELOOP;
}

static int peach_run(void)
{
	struct peach_vm vm = {};
	struct desc_ptr gdt, idt;
	unsigned long flags, cr4, dr7;
	unsigned short sel;
	u64 debugctl;
	u64 basic, cap, vmxon_pa, vmcs_pa, fixed0, fixed1;
	u8 failed;
	unsigned int i;
	int ret = -ENOMEM;

	vm.vmxon = get_zeroed_page(GFP_KERNEL);
	vm.vmcs = get_zeroed_page(GFP_KERNEL);
	vm.memory = __get_free_pages(GFP_KERNEL | __GFP_ZERO, 4);
	vm.ept = __get_free_pages(GFP_KERNEL | __GFP_ZERO, 2);
	if (!vm.vmxon || !vm.vmcs || !vm.memory || !vm.ept)
		goto free;
	memcpy((void *)vm.memory, guest_bin, sizeof(guest_bin));
	init_ept(&vm);

	/* VMXON、VMCS 和 host 状态属于当前 CPU；整个区间禁止调度和 IRQ。 */
	preempt_disable();
	local_irq_save(flags);
	cr4 = __read_cr4();
	ret = -EBUSY;
	if (cr4 & X86_CR4_VMXE)
		goto unlock;
	ret = -EOPNOTSUPP;
	/* VM-exit 不恢复 LDTR；此演示只处理普通 64 位调用者。 */
	store_ldt(sel);
	if (sel)
		goto unlock;
	savesegment(fs, sel);
	if (sel)
		goto unlock;
	savesegment(gs, sel);
	if (sel)
		goto unlock;
	get_debugreg(dr7, 7);
	debugctl = native_read_msr(MSR_IA32_DEBUGCTLMSR);
	if (rdmsrq_safe(MSR_IA32_VMX_BASIC, &basic))
		goto unlock;
	cap = native_read_msr(MSR_IA32_FEAT_CTL);
	if (!(cap & FEAT_CTL_VMX_ENABLED_OUTSIDE_SMX) ||
	    !(cap & FEAT_CTL_LOCKED))
		goto unlock;
	if (((basic >> 32) & 0x1fff) > PAGE_SIZE || ((basic >> 50) & 15) != 6)
		goto unlock;
	cap = native_read_msr(MSR_IA32_VMX_EPT_VPID_CAP);
	if ((cap & (VMX_EPT_PAGE_WALK_4_BIT | VMX_EPTP_WB_BIT |
		    VMX_EPT_INVEPT_BIT | VMX_EPT_EXTENT_CONTEXT_BIT)) !=
	    (VMX_EPT_PAGE_WALK_4_BIT | VMX_EPTP_WB_BIT | VMX_EPT_INVEPT_BIT |
	     VMX_EPT_EXTENT_CONTEXT_BIT))
		goto unlock;
	fixed0 = native_read_msr(MSR_IA32_VMX_CR0_FIXED0);
	fixed1 = native_read_msr(MSR_IA32_VMX_CR0_FIXED1);
	if ((read_cr0() & fixed0) != fixed0 || (read_cr0() & ~fixed1))
		goto unlock;
	fixed0 = native_read_msr(MSR_IA32_VMX_CR4_FIXED0);
	fixed1 = native_read_msr(MSR_IA32_VMX_CR4_FIXED1);
	if (((cr4 | X86_CR4_VMXE) & fixed0) != fixed0 ||
	    ((cr4 | X86_CR4_VMXE) & ~fixed1))
		goto unlock;
	*(u32 *)vm.vmxon = (u32)basic & 0x7fffffff;
	*(u32 *)vm.vmcs = (u32)basic & 0x7fffffff;
	vmxon_pa = __pa(vm.vmxon);
	vmcs_pa = __pa(vm.vmcs);
	if ((basic & VMX_BASIC_32BIT_PHYS_ADDR_ONLY) &&
	    ((vmxon_pa | vmcs_pa) >> 32))
		goto unlock;
	store_gdt(&gdt);
	store_idt(&idt);
	/* 仅临时改变 VMXE，在恢复 IRQ/抢占前恢复原值，不改变 Linux CR4 shadow。 */
	asm volatile("mov %0, %%cr4" : : "r"(cr4 | X86_CR4_VMXE) : "memory");
	asm volatile("vmxon %1; setna %0"
		     : "=qm"(failed)
		     : "m"(vmxon_pa)
		     : "cc", "memory");
	ret = -EIO;
	if (failed) {
		pr_err("peach: VMXON failed, revision=%#x\n", *(u32 *)vm.vmxon);
		goto restore;
	}
	asm volatile("vmclear %1; setna %0"
		     : "=qm"(failed)
		     : "m"(vmcs_pa)
		     : "cc", "memory");
	if (failed)
		goto off;
	asm volatile("vmptrld %1; setna %0"
		     : "=qm"(failed)
		     : "m"(vmcs_pa)
		     : "cc", "memory");
	if (failed)
		goto off;
	ret = setup_controls(&vm, basic);
	if (!ret)
		ret = setup_guest();
	if (!ret)
		ret = setup_host(&gdt, &idt);
	if (!ret)
		ret = invalidate_ept(&vm);
	if (!ret)
		ret = run_guest(&vm);
	asm volatile("vmclear %0" : : "m"(vmcs_pa) : "cc", "memory");
off:
	asm volatile("vmxoff" : : : "cc", "memory");
restore:
	asm volatile("mov %0, %%cr4" : : "r"(cr4) : "memory");
	/* VM-exit 将 descriptor table limit 设为 0xffff，恢复 Linux 原值。 */
	load_gdt(&gdt);
	load_idt(&idt);
	set_debugreg(dr7, 7);
	wrmsrq(MSR_IA32_DEBUGCTLMSR, debugctl);
	/* NMI-exit 已消费 NMI，恢复 host 状态后通过 IDT 交还 Linux。 */
	if (vm.replay_nmi)
		asm volatile("int $2" : : : "memory");
unlock:
	local_irq_restore(flags);
	preempt_enable();
free:
	free_pages(vm.ept, 2);
	free_pages(vm.memory, 4);
	free_page(vm.vmcs);
	free_page(vm.vmxon);
	for (i = 0; i < vm.nr_exits; i++)
		pr_info("peach: %s -> VM-exit reason=%#lx rip=%#lx ax=%#llx bx=%#llx cx=%#llx\n",
			i ? "VMRESUME" : "VMLAUNCH", vm.exits[i].reason,
			vm.exits[i].rip, vm.exits[i].ax, vm.exits[i].bx,
			vm.exits[i].cx);
	pr_info("peach: run result=%d\n", ret);
	return ret;
}

static long peach_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	u64 basic;
	int ret;

	if (!capable(CAP_SYS_RAWIO))
		return -EPERM;
	switch (cmd) {
	case PEACH_PROBE:
		if (rdmsrq_safe(MSR_IA32_VMX_BASIC, &basic))
			return -EOPNOTSUPP;
		pr_info("peach: IA32_VMX_BASIC=%#llx revision=%#x\n", basic,
			(u32)basic & 0x7fffffff);
		return 0;
	case PEACH_RUN:
		if (mutex_lock_interruptible(&peach_lock))
			return -ERESTARTSYS;
		ret = peach_run();
		mutex_unlock(&peach_lock);
		return ret;
	default:
		return -ENOTTY;
	}
}

static const struct file_operations peach_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = peach_ioctl,
};

static int __init peach_init(void)
{
	u32 eax, ebx, ecx, edx;
	int ret;

	/* 本实验只允许在暴露 VMX 的 KVM guest 中加载，避免误操作物理机。 */
	if (!boot_cpu_has(X86_FEATURE_HYPERVISOR) ||
	    !boot_cpu_has(X86_FEATURE_VMX))
		return -ENODEV;
	asm volatile("cpuid"
		     : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
		     : "0"(0x40000000), "2"(0));
	if (ebx != 0x4b4d564b || ecx != 0x564b4d56 || edx != 0x4d)
		return -ENODEV;
	ret = register_chrdev_region(peach_dev, PEACH_COUNT, "peach");
	if (ret)
		return ret;
	cdev_init(&peach_cdev, &peach_fops);
	ret = cdev_add(&peach_cdev, peach_dev, PEACH_COUNT);
	if (ret)
		unregister_chrdev_region(peach_dev, PEACH_COUNT);
	return ret;
}

static void __exit peach_exit(void)
{
	cdev_del(&peach_cdev);
	unregister_chrdev_region(peach_dev, PEACH_COUNT);
}

module_init(peach_init);
module_exit(peach_exit);
