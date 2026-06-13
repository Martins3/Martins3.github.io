# Fixmap

从 arch/x86/include/asm/apicdef.h 看:
```c
#define APIC_BASE (fix_to_virt(FIX_APIC_BASE))
#define APIC_BASE_MSR		0x800
#define APIC_X2APIC_ID_MSR	0x802
#define XAPIC_ENABLE		BIT(11)
#define X2APIC_ENABLE		BIT(10)
```

```c
void __init register_lapic_address(unsigned long address)
{
	/* This should only happen once */
	WARN_ON_ONCE(mp_lapic_addr);
	mp_lapic_addr = address;

	if (!x2apic_mode)
		apic_set_fixmap(true);
}
```

```c
static __init void apic_set_fixmap(bool read_apic)
{
	set_fixmap_nocache(FIX_APIC_BASE, mp_lapic_addr);
	apic_mmio_base = APIC_BASE;
	apic_printk(APIC_VERBOSE, "mapped APIC to %16lx (%16lx)\n",
		    apic_mmio_base, mp_lapic_addr);
	if (read_apic)
		apic_read_boot_cpu_id(false);
}
```

```c
static inline u32 native_apic_mem_read(u32 reg)
{
	return readl((void __iomem *)(APIC_BASE + reg));
}
```

有趣，做了这么复杂的变换，到底是为了什么?

#### 5.6 Fix-Mapped Linear Addresses

Basically, a fix-mapped linear address is a constant linear address like `0xffffc000`
whose corresponding physical address does not have to be the linear address minus
`0xc000000`, but rather a physical address set in an arbitrary way.
Thus, each fixmapped linear address maps one page frame of the physical memory. As we’ll see in later chapters,
**the kernel uses fix-mapped linear addresses instead of pointer variables that never change their value.**


Each fix-mapped linear address is represented by a small integer index defined in the
`enum fixed_addresses` data structure:
```c
/*
 * Here we define all the compile-time 'special' virtual
 * addresses. The point is to have a constant address at
 * compile time, but to set the physical address only
 * in the boot process.
 * for x86_32: We allocate these special addresses
 * from the end of virtual memory (0xfffff000) backwards.
 * Also this lets us do fail-safe vmalloc(), we
 * can guarantee that these special addresses and
 * vmalloc()-ed addresses never overlap.
 *
 * These 'compile-time allocated' memory buffers are
 * fixed-size 4k pages (or larger if used with an increment
 * higher than 1). Use set_fixmap(idx,phys) to associate
 * physical memory with fixmap indices.
 *
 * TLB entries of such buffers will not be flushed across
 * task switches.
 */
enum fixed_addresses {
#ifdef CONFIG_X86_32
	FIX_HOLE,
#else
#ifdef CONFIG_X86_VSYSCALL_EMULATION
	VSYSCALL_PAGE = (FIXADDR_TOP - VSYSCALL_ADDR) >> PAGE_SHIFT,
#endif
#endif
	FIX_DBGP_BASE,
	FIX_EARLYCON_MEM_BASE,
#ifdef CONFIG_PROVIDE_OHCI1394_DMA_INIT
	FIX_OHCI1394_BASE,
#endif
#ifdef CONFIG_X86_LOCAL_APIC
	FIX_APIC_BASE,	/* local (CPU) APIC) -- required for SMP or not */
#endif
#ifdef CONFIG_X86_IO_APIC
	FIX_IO_APIC_BASE_0,
	FIX_IO_APIC_BASE_END = FIX_IO_APIC_BASE_0 + MAX_IO_APICS - 1,
#endif
#ifdef CONFIG_X86_32
	FIX_KMAP_BEGIN,	/* reserved pte's for temporary kernel mappings */
	FIX_KMAP_END = FIX_KMAP_BEGIN+(KM_TYPE_NR*NR_CPUS)-1,
#ifdef CONFIG_PCI_MMCONFIG
	FIX_PCIE_MCFG,
#endif
#endif
#ifdef CONFIG_PARAVIRT
	FIX_PARAVIRT_BOOTMAP,
#endif
#ifdef	CONFIG_X86_INTEL_MID
	FIX_LNW_VRTC,
#endif

#ifdef CONFIG_ACPI_APEI_GHES
	/* Used for GHES mapping from assorted contexts */
	FIX_APEI_GHES_IRQ,
	FIX_APEI_GHES_NMI,
#endif

	__end_of_permanent_fixed_addresses,

	/*
	 * 512 temporary boot-time mappings, used by early_ioremap(),
	 * before ioremap() is functional.
	 *
	 * If necessary we round it up to the next 512 pages boundary so
	 * that we can have a single pmd entry and a single pte table:
	 */
#define NR_FIX_BTMAPS		64
#define FIX_BTMAPS_SLOTS	8
#define TOTAL_FIX_BTMAPS	(NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS)
	FIX_BTMAP_END =
	 (__end_of_permanent_fixed_addresses ^
	  (__end_of_permanent_fixed_addresses + TOTAL_FIX_BTMAPS - 1)) &
	 -PTRS_PER_PTE
	 ? __end_of_permanent_fixed_addresses + TOTAL_FIX_BTMAPS -
	   (__end_of_permanent_fixed_addresses & (TOTAL_FIX_BTMAPS - 1))
	 : __end_of_permanent_fixed_addresses,
	FIX_BTMAP_BEGIN = FIX_BTMAP_END + TOTAL_FIX_BTMAPS - 1,
#ifdef CONFIG_X86_32
	FIX_WP_TEST,
#endif
#ifdef CONFIG_INTEL_TXT
	FIX_TBOOT_BASE,
#endif
	__end_of_fixed_addresses
};


/*
 * 'index to address' translation. If anyone tries to use the idx
 * directly without translation, we catch the bug with a NULL-deference
 * kernel oops. Illegal ranges of incoming indices are caught too.
 */
static __always_inline unsigned long fix_to_virt(const unsigned int idx)
{
	BUILD_BUG_ON(idx >= __end_of_fixed_addresses);
	return __fix_to_virt(idx);
}

static inline unsigned long virt_to_fix(const unsigned long vaddr)
{
	BUG_ON(vaddr >= FIXADDR_TOP || vaddr < FIXADDR_START);
	return __virt_to_fix(vaddr);
}
```

To associate a physical address with a fix-mapped linear address, the kernel uses the
`set_fixmap(idx,phys)` and `set_fixmap_nocache(idx,phys)` macros. Both of them initialize the Page Table entry corresponding to the fix_to_virt(idx) linear address
with the physical address phys; however, the second function also sets the PCD flag of
the Page Table entry, thus disabling the hardware cache when accessing the data in
the page frame (see the section “Hardware Cache” earlier in this chapter). Conversely, clear_fixmap(idx) removes the linking between a fix-mapped linear address
idx and the physical address.

```c
void native_set_fixmap(unsigned /* enum fixed_addresses */ idx,
		       phys_addr_t phys, pgprot_t flags)
{
	/* Sanitize 'prot' against any unsupported bits: */
	pgprot_val(flags) &= __default_kernel_pte_mask;

	__native_set_fixmap(idx, pfn_pte(phys >> PAGE_SHIFT, flags));
}
```
> @todo
> 1. 为什么建立 fixmap
> 2. set_fixmap 的作用是什么
> 3. 使用 enum
> 4. 具体物理地址是什么 ?

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
