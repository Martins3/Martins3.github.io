use std::io;
use std::ptr::NonNull;

/// Owns the page-aligned host mapping backing guest physical memory.
/// It must outlive the VM and all of its vCPUs.
pub struct GuestMemory {
    address: NonNull<u8>,
    size: usize,
}

impl GuestMemory {
    pub fn new(size: usize) -> io::Result<Self> {
        // SAFETY: an anonymous mapping needs no fd; size is the fixed guest RAM size.
        // MAP_ANONYMOUS also provides the initial zeroes for page tables and guest BSS.
        let address = unsafe {
            libc::mmap(
                std::ptr::null_mut(),
                size,
                libc::PROT_READ | libc::PROT_WRITE,
                libc::MAP_SHARED | libc::MAP_ANONYMOUS,
                -1,
                0,
            )
        };
        if address == libc::MAP_FAILED {
            return Err(io::Error::last_os_error());
        }
        let Some(address) = NonNull::new(address.cast()) else {
            // SAFETY: mmap succeeded, but Rust references cannot address a null mapping.
            unsafe { libc::munmap(address, size) };
            return Err(io::Error::other("mmap returned a null address"));
        };
        Ok(Self { address, size })
    }

    pub fn host_address(&self) -> u64 {
        self.address.as_ptr() as u64
    }

    /// Called only during setup, before any KVM_RUN can access this mapping.
    pub fn write(&mut self, offset: usize, bytes: &[u8]) -> io::Result<()> {
        if offset > self.size || bytes.len() > self.size - offset {
            return Err(io::Error::new(
                io::ErrorKind::InvalidInput,
                "write exceeds guest RAM",
            ));
        }
        // SAFETY: the range was checked, the source does not alias the mapping,
        // and the sole vCPU is not running while we initialize RAM.
        unsafe {
            std::ptr::copy_nonoverlapping(
                bytes.as_ptr(),
                self.address.as_ptr().add(offset),
                bytes.len(),
            );
        }
        Ok(())
    }

    pub fn write_u64(&mut self, offset: usize, value: u64) -> io::Result<()> {
        self.write(offset, &value.to_le_bytes())
    }
}

impl Drop for GuestMemory {
    fn drop(&mut self) {
        // SAFETY: this object owns the full mapping; the VM/vCPU have been dropped first.
        unsafe { libc::munmap(self.address.as_ptr().cast(), self.size) };
    }
}
