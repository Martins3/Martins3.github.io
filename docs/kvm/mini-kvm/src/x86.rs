use kvm_bindings::{KVM_MAX_CPUID_ENTRIES, kvm_regs, kvm_segment};
use kvm_ioctls::{Kvm, VcpuFd};

use crate::memory::GuestMemory;
use crate::{Result, context};

pub const RAM_SIZE: usize = 2 * 1024 * 1024;
// Keep these in sync with guest/linker.ld.
pub const ENTRY: usize = 0x1_0000;
pub const STACK_BOTTOM: usize = 0x1f_0000;
const STACK_TOP: usize = RAM_SIZE;
const PML4: usize = 0x1000;
const PDPT: usize = 0x2000;
const PD: usize = 0x3000;
const GDT: usize = 0x5000;

pub fn setup(kvm: &Kvm, vcpu: &VcpuFd, memory: &mut GuestMemory) -> Result<()> {
    let cpuid = context(
        "KVM_GET_SUPPORTED_CPUID",
        kvm.get_supported_cpuid(KVM_MAX_CPUID_ENTRIES),
    )?;
    context("KVM_SET_CPUID2", vcpu.set_cpuid2(&cpuid))?;

    // A single 2 MiB leaf maps GVA [0, 2 MiB) to the same GPA range.
    const PRESENT_WRITE: u64 = 0x3;
    const HUGE_PAGE: u64 = 1 << 7;
    memory.write_u64(PML4, PDPT as u64 | PRESENT_WRITE)?;
    memory.write_u64(PDPT, PD as u64 | PRESENT_WRITE)?;
    memory.write_u64(PD, PRESENT_WRITE | HUGE_PAGE)?;

    // Null, ring-0 64-bit code, and ring-0 data descriptors.
    memory.write_u64(GDT, 0)?;
    memory.write_u64(GDT + 8, 0x00af_9b00_0000_ffff)?;
    memory.write_u64(GDT + 16, 0x00cf_9300_0000_ffff)?;

    let mut sregs = context("KVM_GET_SREGS", vcpu.get_sregs())?;
    sregs.gdt.base = GDT as u64;
    sregs.gdt.limit = 3 * 8 - 1;
    sregs.cs = kvm_segment {
        base: 0,
        limit: u32::MAX,
        selector: 8,
        type_: 0xb, // Executable, readable, accessed.
        present: 1,
        s: 1,
        l: 1, // Long-mode code: L=1, DB=0.
        g: 1,
        ..Default::default()
    };
    let data = kvm_segment {
        selector: 16,
        type_: 0x3, // Writable, accessed.
        l: 0,
        db: 1,
        ..sregs.cs
    };
    sregs.ds = data;
    sregs.es = data;
    sregs.fs = data;
    sregs.gs = data;
    sregs.ss = data;

    const CR0_PE: u64 = 1;
    const CR0_MP: u64 = 1 << 1;
    const CR0_ET: u64 = 1 << 4;
    const CR0_NE: u64 = 1 << 5;
    const CR0_WP: u64 = 1 << 16;
    const CR0_PG: u64 = 1 << 31;
    const CR4_PAE: u64 = 1 << 5;
    const EFER_LME: u64 = 1 << 8;
    const EFER_LMA: u64 = 1 << 10;
    sregs.cr0 = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_PG;
    sregs.cr3 = PML4 as u64;
    sregs.cr4 = CR4_PAE;
    sregs.efer = EFER_LME | EFER_LMA;
    context("KVM_SET_SREGS (64-bit mode)", vcpu.set_sregs(&sregs))?;

    // _start issues CALL with a 16-byte aligned stack, satisfying the SysV C ABI.
    // RFLAGS bit 1 must be set; IF stays clear because there is no interrupt setup.
    let regs = kvm_regs {
        rip: ENTRY as u64,
        rsp: STACK_TOP as u64,
        rflags: 2,
        ..Default::default()
    };
    context("KVM_SET_REGS", vcpu.set_regs(&regs))
}
