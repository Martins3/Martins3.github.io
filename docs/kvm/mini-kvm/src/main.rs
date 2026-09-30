#[cfg(not(all(target_os = "linux", target_arch = "x86_64")))]
compile_error!("mini-kvm requires Linux x86_64");

mod memory;
mod x86;

use std::error::Error;
use std::io::{self, Read, Write};
use std::path::Path;
use std::process::ExitCode;

use kvm_bindings::{KVM_API_VERSION, kvm_userspace_memory_region};
use kvm_ioctls::{Cap, Kvm, VcpuExit, VcpuFd};

use memory::GuestMemory;

type Result<T> = std::result::Result<T, Box<dyn Error>>;

fn context<T, E: Error>(operation: &str, result: std::result::Result<T, E>) -> Result<T> {
    result.map_err(|error| io::Error::other(format!("{operation}: {error}")).into())
}

fn load_image(path: &Path) -> Result<Vec<u8>> {
    let max_size = x86::STACK_BOTTOM - x86::ENTRY;
    let file = context(
        &format!("open guest image {}", path.display()),
        std::fs::File::open(path),
    )?;
    // Read at most one byte beyond the image limit, even for a very large input.
    let mut image = Vec::new();
    context(
        "read guest image",
        file.take(max_size as u64 + 1).read_to_end(&mut image),
    )?;
    if image.is_empty() || image.len() > max_size {
        return Err(format!("guest image must contain 1..={max_size} bytes").into());
    }
    if image.starts_with(b"\x7fELF") {
        return Err("expected a flat image (guest.out), not an ELF file (guest.elf.out)".into());
    }
    Ok(image)
}

fn run(vcpu: &mut VcpuFd) -> Result<()> {
    const DEBUG_PORT: u16 = 0xe9;
    let mut output = io::stdout().lock();
    loop {
        match vcpu.run() {
            Ok(VcpuExit::IoOut(DEBUG_PORT, data)) => {
                output.write_all(data)?;
                output.flush()?;
                // Re-entering KVM_RUN completes the pending I/O instruction.
            }
            Ok(VcpuExit::Hlt) => {
                // guest_main returns an int in EAX; _start preserves it across HLT.
                let status = context("KVM_GET_REGS", vcpu.get_regs())?.rax as u32 as i32;
                if status != 0 {
                    return Err(format!("guest_main returned {status}").into());
                }
                eprintln!("mini-kvm: guest halted, guest_main returned 0");
                return Ok(());
            }
            Ok(exit) => return Err(format!("unexpected KVM exit: {exit:?}").into()),
            Err(error) if error.errno() == libc::EINTR => continue,
            Err(error) => return context("KVM_RUN", Err(error)),
        }
    }
}

fn execute() -> Result<()> {
    let mut args = std::env::args_os().skip(1);
    let path = args.next().ok_or("usage: mini-kvm.out <guest.out>")?;
    if args.next().is_some() {
        return Err("usage: mini-kvm.out <guest.out>".into());
    }
    let image = load_image(Path::new(&path))?;
    let kvm = context("open /dev/kvm", Kvm::new())?;
    if kvm.get_api_version() != KVM_API_VERSION as i32 {
        return Err("unsupported KVM API version".into());
    }
    if !kvm.check_extension(Cap::UserMemory) {
        return Err("KVM_CAP_USER_MEMORY is required".into());
    }

    // Declaration order matters: Rust drops vCPU, VM, then guest RAM on every exit.
    let mut memory = context("allocate guest RAM", GuestMemory::new(x86::RAM_SIZE))?;
    memory.write(x86::ENTRY, &image)?;
    let vm = context("KVM_CREATE_VM", kvm.create_vm())?;
    // Intel KVM needs a reserved three-page GPA range for its real-mode TSS.
    // This range is outside our RAM and is not accessed by the guest.
    context("KVM_SET_TSS_ADDR", vm.set_tss_address(0xfffb_d000))?;
    let region = kvm_userspace_memory_region {
        slot: 0,
        guest_phys_addr: 0,
        memory_size: x86::RAM_SIZE as u64,
        userspace_addr: memory.host_address(),
        flags: 0,
    };
    // SAFETY: memory is page-aligned, fixed in place, and outlives VM and vCPU.
    context("KVM_SET_USER_MEMORY_REGION", unsafe {
        vm.set_user_memory_region(region)
    })?;
    let mut vcpu = context("KVM_CREATE_VCPU", vm.create_vcpu(0))?;
    x86::setup(&kvm, &vcpu, &mut memory)?;
    run(&mut vcpu)
}

fn main() -> ExitCode {
    match execute() {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("mini-kvm: {error}");
            ExitCode::FAILURE
        }
    }
}
