# fixmap

PLKA :

Basically, a fix-mapped linear address is a constant linear address like
`0xffffc000` whose corresponding physical address does not have to be the linear
address minus `0xc000000`, but rather a physical address set in an arbitrary
way. Thus, each fixmapped linear address maps one page frame of the physical
memory. As we’ll see in later chapters, **the kernel uses fix-mapped linear
addresses instead of pointer variables that never change their value.**

Each fix-mapped linear address is represented by a small integer index defined
in the `enum fixed_addresses` data structure:

To associate a physical address with a fix-mapped linear address, the kernel
uses the `set_fixmap(idx,phys)` and `set_fixmap_nocache(idx,phys)` macros. Both
of them initialize the Page Table entry corresponding to the fix_to_virt(idx)
linear address with the physical address phys; however, the second function also
sets the PCD flag of the Page Table entry, thus disabling the hardware cache
when accessing the data in the page frame (see the section “Hardware Cache”
earlier in this chapter). Conversely, clear_fixmap(idx) removes the linking
between a fix-mapped linear address idx and the physical address.

就是这个意思了，将固定的虚拟地址空间划分出来，来映射物理地址

- **物理地址不固定**。LAPIC 的 MMIO 物理地址由固件（ACPI MADT / MP
  table）在启动时上报（register_lapic_address 的 mp_lapic_addr，典型默认值是
  0xFEE00000），编译期无法知道。
- **但内核代码想要一个编译期常量地址来访问它**。APIC_BASE 展开为
  fix_to_virt(FIX_APIC_BASE)，是个常数，于是 native_apic_mem_read()
  就是对常量地址做 readl：
  - 不需要任何全局指针变量，也就没有"变量必须先初始化才能用"的时序问题——这在启动早期（ioremap/vmalloc
    还没就绪、甚至汇编阶段）尤其重要；
  - 生成的代码更直接，访存不需要先 load 一个指针；
  - 这些页是全局映射，TLB 项不会因进程切换被刷掉（enum fixed_addresses
    的注释里也写了这一点）。

调用 set_fixmap 来构建映射。

## 关键数据结构

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
#ifdef CONFIG_KMAP_LOCAL
	FIX_KMAP_BEGIN,	/* reserved pte's for temporary kernel mappings */
	FIX_KMAP_END = FIX_KMAP_BEGIN + (KM_MAX_IDX * NR_CPUS) - 1,
#ifdef CONFIG_PCI_MMCONFIG
	FIX_PCIE_MCFG,
#endif
#endif
#ifdef CONFIG_PARAVIRT_XXL
	FIX_PARAVIRT_BOOTMAP,
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

## 经典案例

### APIC

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
// arch/x86/kernel/apic/apic.c:2084，register_lapic_address 和 native_apic_mem_read 至今未变，
// 只是打印宏从 apic_printk(APIC_VERBOSE, ...) 换成了 apic_pr_verbose(...)
static __init void apic_set_fixmap(bool read_apic)
{
	set_fixmap_nocache(FIX_APIC_BASE, mp_lapic_addr);
	apic_mmio_base = APIC_BASE;
	apic_pr_verbose("Mapped APIC to %16lx (%16lx)\n",
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

```c
// arch/x86/mm/pgtable.c:585（旧版在 arch/x86/mm/ioremap.c，已搬家，实现不变）
void native_set_fixmap(unsigned /* enum fixed_addresses */ idx,
		       phys_addr_t phys, pgprot_t flags)
{
	/* Sanitize 'prot' against any unsupported bits: */
	pgprot_val(flags) &= __default_kernel_pte_mask;

	__native_set_fixmap(idx, pfn_pte(phys >> PAGE_SHIFT, flags));
}
```

###

kmap 需要一个"预留的虚拟地址池"，而 fixmap 恰好就是内核提供的这种池子。拆开说：

kmap 要解决的问题

要访问一个 struct page，必须有一个虚拟地址，其 PTE
指向该页的物理页帧。问题是：用哪个虚拟地址？

- 32 位 highmem 机器上，高端内存页没有线性映射，天然需要临时建映射；\
- 但这个"临时映射的虚拟地址"不能随便挑——必须是编译期就预留好的，否则会和
vmalloc、直接映射区等撞车。

fixmap 正好就是这个预留机制

回顾一下 fixmap
的定义：一段编译期保留的虚拟地址区域，每个槽位的虚拟地址是常量，物理地址运行时才绑定。
kmap 只是 fixmap 的一个"客户"——它从 fixmap 里划走一整段槽位（FIX_KMAP_BEGIN ~
FIX_KMAP_END）当作自己的临时映射地址池。所以二者的关系是：

```txt
fixmap          = 机制：编译期保留的常量虚拟地址槽位 + 运行时绑定物理页
kmap_local_page = 客户：借用其中 FIX_KMAP_* 一段槽位，加上自己的槽位分配纪律
```

当前内核的实际流程（mm/highmem.c 的 __kmap_local_pfn_prot()，mm-fixmap.md
里引用的就是它的槽位）：

1. migrate_disable(); preempt_disable(); —— 槽位是 per-CPU 的，不能跑到别的 CPU
   上；
2. idx = arch_kmap_local_map_idx(kmap_local_idx_push(), pfn); ——
   像栈一样压入一个嵌套编号（per-task 的 kmap_ctrl.idx），这样中断里嵌套 kmap 也
   不会踩到别人的槽；
3. vaddr = __fix_to_virt(FIX_KMAP_BEGIN + idx); —— 虚拟地址就是 fixmap
   槽位号算出来的常量；
4. kmap_get_pte(vaddr, idx) 找到覆盖该 fixmap 地址的 PTE，写入 pfn_pte(pfn,
   prot)，刷 TLB，返回 vaddr。

kunmap_local 反过来：清掉那个 PTE、弹出 idx。

## 32f5b6995f790ac 是一个 arm64 fixmap 边界修正提交

它解决的问题是：arm64 的 fixmap 实际包含三类区域：

- 永久运行时映射，例如 FDT、早期控制台、KPTI trampoline 等；
- 启动阶段临时映射，例如 early_ioremap() 使用的 FIX_BTMAP；
- 用于创建页表的临时槽位。

但原来的 FIXADDR_START/FIXADDR_SIZE 只覆盖第一类永久槽位，而 map_kernel()
和页表 dump 逻辑却把它们当成整个 fixmap 的边界使用。这会导致：

- ptdump 输出的 fixmap 起始地址不准确，漏掉启动期区域；
- map_kernel() 判断 PGD/P4D/PUD 是否覆盖 fixmap 时可能使用错误边界；
- 如果启动期 fixmap 将来跨入不同的 PMD/PGD，理论上可能导致页表建立错误。

提交新增：

#define FIXADDR_TOT_SIZE  (__end_of_fixed_addresses << PAGE_SHIFT)
#define FIXADDR_TOT_START (FIXADDR_TOP - FIXADDR_TOT_SIZE)

其中：

- FIXADDR_START/SIZE 继续表示永久运行时 fixmap；
- FIXADDR_TOT_START/SIZE 表示包含所有槽位的完整 fixmap。

随后：

- arch/arm64/mm/mmu.c 的 map_kernel() 和 early_fixmap_init() 改用 FIXADDR_TOT_START；
- 启动期 FIX_BTMAP 地址转换改用底层的 __fix_to_virt()；
- arch/arm64/mm/ptdump.c 从完整 fixmap 起点开始打印。

因此，这个提交基本不改变 fixmap 的实际 VA 布局，也不改变常规运行时 API，
主要是修正边界定义和页表初始化/调试代码对边界的使用，
消除一个当前配置下通常不会触发、但理论上可能导致问题的潜在错误。


## fixmap
fixmap 不是一种特殊的物理内存，而是一段预先保留好的虚拟地址区间。
内核可以把某个物理页临时映射到固定的虚拟地址槽位，例如：

虚拟地址 FIX_FDT       -> 物理地址 FDT
虚拟地址 FIX_BTMAP_x   -> 物理地址某个 MMIO/固件区域

这样内核不需要动态申请虚拟地址，也能在启动早期或特殊场景访问物理内存。

### 1. 永久运行时槽位

这些槽位在 fixmap.h 中位于 __end_of_permanent_fixed_addresses 之前。

典型包括：

- FIX_FDT
    - FDT 是 Flattened Device Tree，即 bootloader 传给内核的硬件描述信息。
    - 内核启动时需要读取 CPU、内存、串口、设备树节点等信息。
    - fixmap_remap_fdt() 把 FDT 所在的物理内存映射到固定 VA。
    - FDT 可能跨多个页，因此会占用一组连续槽位。

- FIX_EARLYCON_MEM_BASE
    - early console 使用的槽位。
    - 正常串口驱动和完整的 ioremap() 尚未初始化时，内核也需要打印启动日志。
    - 这个槽位把 UART 等 MMIO 寄存器映射进来，让 earlycon 可以访问串口。

- FIX_TEXT_POKE0
    - 用于运行时修改内核代码，例如打补丁、ftrace、替换指令。
    - 内核代码通常是只读/可执行的，patch_map() 临时把目标物理页映射到一个可写的固定槽位，修改完成后再清除映射。
    - 因此它属于“永久预留的槽位”，但映射本身通常是按需建立的。

- FIX_ENTRY_TRAMP_TEXT1 等
    - KPTI 使用的入口 trampoline 代码。
    - 开启 KPTI 后，用户态和内核态使用不同的页表；发生系统调用、中断或异常时，需要先执行一小段位于安全映射中的入口代码，再切换到完整内核页表。
    - 这些代码需要在特殊页表中保持可访问，所以使用固定槽位。

这里“永久”主要表示槽位在地址布局中永久保留，不表示每个槽位始终都有有效映射。

### 2. 启动阶段临时槽位：FIX_BTMAP

FIX_BTMAP 是 early_ioremap() 的后端。

启动早期，完整的虚拟内存和 ioremap() 子系统还没有准备好，但内核已经可能需要访问：

- ACPI 表；
- PCI 或固件提供的 MMIO 区域；
- EFI 数据；
- 其他设备寄存器或启动参数。

early_ioremap(phys_addr, size) 的流程大致是：

1. 从预留的 FIX_BTMAP 槽位中找一个空闲 slot；
2. 把目标物理页逐页映射进去；
3. 返回一个内核虚拟地址；
4. 使用完毕后通过 early_iounmap() 清除映射并复用 slot。

当前 arm64 大致预留了 7 个 slot，每个 slot 最多覆盖约 256 KiB，因此它是一个很小的启动期临时映射池，而不是通用的长期 I/O 映射机制。

### 3. 页表创建用的临时槽位：FIX_PTE、FIX_PMD 等

这些槽位用于“访问和修改页表本身”。

例如：

pte_set_fixmap(addr)
pmd_set_fixmap(addr)
pud_set_fixmap(addr)
pgd_set_fixmap(addr)

当内核需要修改某个虚拟地址对应的页表项时，它可能只有该页表页的物理地址，而当前阶段没有方便的永久虚拟地址可以直接访问它。于是内核：

1. 找到目标页表页的物理地址；
2. 把该物理页映射到 FIX_PTE 或 FIX_PMD 等固定槽位；
3. 通过这个固定 VA 修改页表；
4. 清除该 fixmap 映射。

这类槽位本质上是“访问页表页的临时工作台”，主要用于早期页表建立、内核页表修改和某些低级内存管理操作。

### 与这个 commit 的关系

原来的 FIXADDR_START 只覆盖第一类“永久预留槽位”，但完整 fixmap 还包括后两类槽位。这个 commit 新增的：

FIXADDR_TOT_START
FIXADDR_TOT_SIZE

才覆盖从 FIX_FDT 一直到 FIX_PGD 的整个区域。

因此它修正的是“fixmap 整体边界”的计算和使用，并没有改变这些槽位各自的用途。

## enum fixed_addresses 的内容

arm 也是存在类似的结果:
```c
/*
 * Here we define all the compile-time 'special' virtual
 * addresses. The point is to have a constant address at
 * compile time, but to set the physical address only
 * in the boot process.
 *
 * Each enum increment in these 'compile-time allocated'
 * memory buffers is page-sized. Use set_fixmap(idx,phys)
 * to associate physical memory with a fixmap index.
 */
enum fixed_addresses {
	FIX_HOLE,

	/*
	 * Reserve a virtual window for the FDT that is a page bigger than the
	 * maximum supported size. The additional space ensures that any FDT
	 * that does not exceed MAX_FDT_SIZE can be mapped regardless of
	 * whether it crosses any page boundary.
	 */
	FIX_FDT_END,
	FIX_FDT = FIX_FDT_END + DIV_ROUND_UP(MAX_FDT_SIZE, PAGE_SIZE) + 1,

	FIX_EARLYCON_MEM_BASE,
	FIX_TEXT_POKE0,

#ifdef CONFIG_KVM
	/* One slot per CPU, mapping the guest's VNCR page at EL2. */
	FIX_VNCR_END,
	FIX_VNCR = FIX_VNCR_END + NR_CPUS,
#endif

#ifdef CONFIG_ACPI_APEI_GHES
	/* Used for GHES mapping from assorted contexts */
	FIX_APEI_GHES_IRQ,
	FIX_APEI_GHES_SEA,
#ifdef CONFIG_ARM_SDE_INTERFACE
	FIX_APEI_GHES_SDEI_NORMAL,
	FIX_APEI_GHES_SDEI_CRITICAL,
#endif
#endif /* CONFIG_ACPI_APEI_GHES */

#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
#ifdef CONFIG_RELOCATABLE
	FIX_ENTRY_TRAMP_TEXT4,	/* one extra slot for the data page */
#endif
	FIX_ENTRY_TRAMP_TEXT3,
	FIX_ENTRY_TRAMP_TEXT2,
	FIX_ENTRY_TRAMP_TEXT1,
#define TRAMP_VALIAS		(__fix_to_virt(FIX_ENTRY_TRAMP_TEXT1))
#endif /* CONFIG_UNMAP_KERNEL_AT_EL0 */
	__end_of_permanent_fixed_addresses,

	/*
	 * Temporary boot-time mappings, used by early_ioremap(),
	 * before ioremap() is functional.
	 *
	 * Reserve one extra page so a 256K mapping may start at any
	 * offset within a page. early_ioremap() maps the page-aligned
	 * physical range, so the initial offset can consume an extra page.
	 */
#define NR_FIX_BTMAPS		((SZ_256K / PAGE_SIZE) + 1)
#define FIX_BTMAPS_SLOTS	7
#define TOTAL_FIX_BTMAPS	(NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS)

	FIX_BTMAP_END = __end_of_permanent_fixed_addresses,
	FIX_BTMAP_BEGIN = FIX_BTMAP_END + TOTAL_FIX_BTMAPS - 1,

	/*
	 * Used for kernel page table creation, so unmapped memory may be used
	 * for tables.
	 */
	FIX_PTE,
	FIX_PMD,
	FIX_PUD,
	FIX_P4D,
	FIX_PGD,

	__end_of_fixed_addresses
};
```

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
