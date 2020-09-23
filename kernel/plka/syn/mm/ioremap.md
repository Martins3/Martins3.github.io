# x86/mm/ioremap.c

## TODO
1. kernel/iomem.c : cacheable memory 的作用是什么 ?
2. ldd3 之类的东西上如何 ioremap 的 ?
3. plka 一定讲过这一个内容的。
4. https://www.cnblogs.com/nufangrensheng/p/3713331.html
5. /home/shen/Core/linux/lib/iomap.c 中间有好东西啊




## doc

However, reservation of a memory region is not the only action needed when using I/O memory.
Depending on bus system and processor type, it may be necessary to map the address space of an expansion device into kernel address space before it can be accessed (this is known as software I/O mapping).
This is achieved by setting up the system page tables appropriately using the `ioremap` kernel function,
which is available at various points in the kernel sources and whose definition is architecture-specific.
The likewise architecture-specific iounmap function is provided to unmap mappings.
> 1. 实现 ioremap 的操作的方法就是使用修改 page table 的
> 2. @todo 很奇怪，为什么 resource  不是 architecture 相关的(这个不是需要和地址管理器)，而实现映射却是架构相关的(不就是修改一下page table 吗 ? 按照 vmalloc 的方法实现)

## question
1. cache mode 指的是 ?

## 物理地址空间


## ioremap
1. ioremap : 如果不同的 driver 申请相同地址的 phys_addr，会失败吗 ?
2. readb/readw/readl/writeb/writew/writel 等各种函数分析。

```c
/**
 * ioremap     -   map bus memory into CPU space
 * @phys_addr:    bus address of the memory
 * @size:      size of the resource to map
 *
 * ioremap performs a platform specific sequence of operations to
 * make bus memory CPU accessible via the readb/readw/readl/writeb/
 * writew/writel functions and the other mmio helpers. The returned
 * address is not guaranteed to be usable directly as a virtual
 * address.
 *
 * This version of ioremap ensures that the memory is marked uncachable
 * on the CPU as well as honouring existing caching rules from things like
 * the PCI bus. Note that there are other caches and buffers on many
 * busses. In particular driver authors should read up on PCI writes
 *
 * It's useful if some control registers are in such an area and
 * write combining or read caching is not desirable:
 *
 * Must be freed with iounmap.
 */
// 这个返回值，不是 void , 是 void *，其实就是驱动提供物理地址，然后返回分配的虚拟地址
// todo 所以，如何保证这些物理地址不会重复 ? 地址和大小是如何分配的
// XXX 其实我猜测，这是架构确定的，所以导致访问某些地址被访问到设备或者 RAM 上
// 如果真的如此，那么为什么还需要 resource.c 机制
// XXX 还是说，首先调用 ioremap 分配物理地址空间，然后使用 ioremap 获取空间
void __iomem *ioremap(resource_size_t phys_addr, unsigned long size) 
{
	/*
	 * Ideally, this should be:
	 *	pat_enabled() ? _PAGE_CACHE_MODE_UC : _PAGE_CACHE_MODE_UC_MINUS;
	 *
	 * Till we fix all X drivers to use ioremap_wc(), we will use
	 * UC MINUS. Drivers that are certain they need or can already
	 * be converted over to strong UC can use ioremap_uc().
	 */
	enum page_cache_mode pcm = _PAGE_CACHE_MODE_UC_MINUS;

	return __ioremap_caller(phys_addr, size, pcm,
				__builtin_return_address(0), false);
}

/*
 * Remap an arbitrary physical address space into the kernel virtual
 * address space. It transparently creates kernel huge I/O mapping when
 * the physical address is aligned by a huge page size (1GB or 2MB) and
 * the requested size is at least the huge page size.
 *
 * NOTE: MTRRs can override PAT memory types with a 4KB granularity.
 * Therefore, the mapping code falls back to use a smaller page toward 4KB
 * when a mapping range is covered by non-WB type of MTRRs.
 *
 * NOTE! We need to allow non-page-aligned mappings too: we will obviously
 * have to convert them into an offset in a page-aligned mapping, but the
 * caller shouldn't need to know that small detail.
 */
static void __iomem *
__ioremap_caller(resource_size_t phys_addr, unsigned long size,
		 enum page_cache_mode pcm, void *caller, bool encrypted)
{
	unsigned long offset, vaddr;
	resource_size_t last_addr;
	const resource_size_t unaligned_phys_addr = phys_addr;
	const unsigned long unaligned_size = size;
	struct ioremap_desc io_desc; // todo 分析之
	struct vm_struct *area; // todo 最后这个 vma 被放到什么位置了
	enum page_cache_mode new_pcm;
	pgprot_t prot;
	int retval;
	void __iomem *ret_addr;

	/* Don't allow wraparound or zero size */
	last_addr = phys_addr + size - 1;
	if (!size || last_addr < phys_addr)
		return NULL;

	if (!phys_addr_valid(phys_addr)) {
		printk(KERN_WARNING "ioremap: invalid physical address %llx\n",
		       (unsigned long long)phys_addr);
		WARN_ON_ONCE(1);
		return NULL;
	}

	__ioremap_check_mem(phys_addr, size, &io_desc); // 利用 walk_mem_res 确认该地址是否被使用过，而且可以确认物理地址就是资源

	/*
	 * Don't allow anybody to remap normal RAM that we're using..
	 */
	if (io_desc.flags & IORES_MAP_SYSTEM_RAM) { // todo 什么? IORES_MAP_SYSTEM_RAM 也是内存的一部分
		WARN_ONCE(1, "ioremap on RAM at %pa - %pa\n",
			  &phys_addr, &last_addr);
		return NULL;
	}

	/*
	 * Mappings have to be page-aligned
	 */
	offset = phys_addr & ~PAGE_MASK;
	phys_addr &= PHYSICAL_PAGE_MASK;
	size = PAGE_ALIGN(last_addr+1) - phys_addr;

	retval = memtype_reserve(phys_addr, (u64)phys_addr + size, // todo 我不知道这是什么意思
						pcm, &new_pcm);
	if (retval) {
		printk(KERN_ERR "ioremap memtype_reserve failed %d\n", retval);
		return NULL;
	}

	if (pcm != new_pcm) {
		if (!is_new_memtype_allowed(phys_addr, size, pcm, new_pcm)) {
			printk(KERN_ERR
		"ioremap error for 0x%llx-0x%llx, requested 0x%x, got 0x%x\n",
				(unsigned long long)phys_addr,
				(unsigned long long)(phys_addr + size),
				pcm, new_pcm);
			goto err_free_memtype;
		}
		pcm = new_pcm;
	}

	/*
	 * If the page being mapped is in memory and SEV is active then
	 * make sure the memory encryption attribute is enabled in the
	 * resulting mapping.
	 */
	prot = PAGE_KERNEL_IO;
	if ((io_desc.flags & IORES_MAP_ENCRYPTED) || encrypted)
		prot = pgprot_encrypted(prot);

	switch (pcm) {
	case _PAGE_CACHE_MODE_UC:
	default:
		prot = __pgprot(pgprot_val(prot) |
				cachemode2protval(_PAGE_CACHE_MODE_UC));
		break;
	case _PAGE_CACHE_MODE_UC_MINUS:
		prot = __pgprot(pgprot_val(prot) |
				cachemode2protval(_PAGE_CACHE_MODE_UC_MINUS));
		break;
	case _PAGE_CACHE_MODE_WC:
		prot = __pgprot(pgprot_val(prot) |
				cachemode2protval(_PAGE_CACHE_MODE_WC));
		break;
	case _PAGE_CACHE_MODE_WT:
		prot = __pgprot(pgprot_val(prot) |
				cachemode2protval(_PAGE_CACHE_MODE_WT));
		break;
	case _PAGE_CACHE_MODE_WB:
		break;
	}

	/*
	 * Ok, go for it.. // 上面各种检查，
	 */
	area = get_vm_area_caller(size, VM_IOREMAP, caller);
  // XXX 猜测，整个地址空间都是被 resource 管理的，RAM 只是其中的一部分
  // XXX 猜测，正如本函数的注释所说的，这是映射内核虚拟地址空间，所以，vma 是自动挂载到内核的 mm 上的，实际上，并没有, XXX 所以进步猜测，那就是内核的虚拟地址空间特地部分空间划分出来用于 vmalloc 以及 ioremap
  // todo 物理地址在 physical 中间获取，此处获取等大小的虚拟地址
  // todo 比想象多出来了一层映射 : 从 kernel virtual memory 到 physical address 的映射
	if (!area)
		goto err_free_memtype;
	area->phys_addr = phys_addr;
	vaddr = (unsigned long) area->addr; // 这就非常的奇怪啊，

	if (memtype_kernel_map_sync(phys_addr, size, pcm))
		goto err_free_area;

	if (ioremap_page_range(vaddr, vaddr + size, phys_addr, prot)) // 两个申请工作完成，在此处映射. 所谓的映射就是修改 page table entry 之类的操作
		goto err_free_area;

	ret_addr = (void __iomem *) (vaddr + offset);
	mmiotrace_ioremap(unaligned_phys_addr, unaligned_size, ret_addr);

	/*
	 * Check if the request spans more than any BAR in the iomem resource
	 * tree.
	 */
	if (iomem_map_sanity_check(unaligned_phys_addr, unaligned_size))
		pr_warn("caller %pS mapping multiple BARs\n", caller);

	return ret_addr;
err_free_area:
	free_vm_area(area);
err_free_memtype:
	memtype_free(phys_addr, phys_addr + size);
	return NULL;
}
```


## 分析一下 /proc/iomem
```
00000000-00000fff : Reserved
00001000-00057fff : System RAM
00058000-00058fff : Reserved
00059000-0009dfff : System RAM
0009e000-000fffff : Reserved
  000a0000-000bffff : PCI Bus 0000:00
  000c0000-000c3fff : PCI Bus 0000:00
  000c4000-000c7fff : PCI Bus 0000:00
  000c8000-000cbfff : PCI Bus 0000:00
  000cc000-000cffff : PCI Bus 0000:00
  000d0000-000d3fff : PCI Bus 0000:00
  000d4000-000d7fff : PCI Bus 0000:00
  000d8000-000dbfff : PCI Bus 0000:00
  000dc000-000dffff : PCI Bus 0000:00
  000e0000-000e3fff : PCI Bus 0000:00
  000e4000-000e7fff : PCI Bus 0000:00
  000e8000-000ebfff : PCI Bus 0000:00
  000ec000-000effff : PCI Bus 0000:00
  000f0000-000fffff : PCI Bus 0000:00
    000f0000-000fffff : System ROM
00100000-6c2cefff : System RAM
6c2cf000-6c2cffff : ACPI Non-volatile Storage
6c2d0000-6c2d0fff : Reserved
6c2d1000-6e4b7fff : System RAM
6e4b8000-6edb7fff : Reserved
6edb8000-7a3a8fff : System RAM
7a3a9000-7a3aefff : Reserved
7a3af000-8a57dfff : System RAM
8a57e000-8a77dfff : Unknown E820 type
8a77e000-8af7dfff : Reserved
8af7e000-8cf7dfff : ACPI Non-volatile Storage
  8cf54000-8cf54fff : USBC000:00
8cf7e000-8cffdfff : ACPI Tables
8cffe000-8cffefff : System RAM
8cfff000-8fffffff : Reserved
  8e000000-8fffffff : Graphics Stolen Memory
90000000-dfffffff : PCI Bus 0000:00
  90000000-9001ffff : pnp 00:08
  a0000000-b1ffffff : PCI Bus 0000:01
    a0000000-afffffff : 0000:01:00.0
    b0000000-b1ffffff : 0000:01:00.0
  b2000000-b2ffffff : 0000:00:02.0
  b3000000-b3ffffff : PCI Bus 0000:01
    b3000000-b3ffffff : 0000:01:00.0
  b4000000-b40fffff : PCI Bus 0000:03
    b4000000-b4003fff : 0000:03:00.0
      b4000000-b4003fff : nvme
  b4100000-b41fffff : PCI Bus 0000:02
    b4100000-b4101fff : 0000:02:00.0
      b4100000-b4101fff : iwlwifi
  b4200000-b420ffff : 0000:00:1f.3
    b4200000-b420ffff : ICH HD audio
  b4210000-b421ffff : 0000:00:14.0
    b4210000-b421ffff : xhci-hcd
      b4218070-b421846f : intel_xhci_usb_sw
  b4220000-b4223fff : 0000:00:1f.3
    b4220000-b4223fff : ICH HD audio
  b4224000-b4227fff : 0000:00:1f.2
  b4228000-b4228fff : 0000:00:08.0
  b4229000-b4229fff : 0000:00:14.2
    b4229000-b4229fff : Intel PCH thermal driver
  b422a000-b422afff : 0000:00:15.0
    b422a000-b422a1ff : lpss_dev
      b422a000-b422a1ff : i2c_designware.0
    b422a200-b422a2ff : lpss_priv
    b422a800-b422afff : idma64.0
      b422a800-b422afff : idma64.0
  b422b000-b422bfff : 0000:00:15.1
    b422b000-b422b1ff : lpss_dev
      b422b000-b422b1ff : i2c_designware.1
    b422b200-b422b2ff : lpss_priv
    b422b800-b422bfff : idma64.1
      b422b800-b422bfff : idma64.1
  b422c000-b422cfff : 0000:00:16.0
    b422c000-b422cfff : mei_me
  b422d000-b422d0ff : 0000:00:1f.4
  c0000000-cfffffff : 0000:00:02.0
e0000000-efffffff : PCI MMCONFIG 0000 [bus 00-ff]
  e0000000-efffffff : Reserved
    e0000000-efffffff : pnp 00:08
fd000000-fe7fffff : Reserved
  fd000000-fe7fffff : PCI Bus 0000:00
    fd000000-fdabffff : pnp 00:00
    fdac0000-fdacffff : INT344B:00
      fdac0000-fdacffff : INT344B:00
    fdad0000-fdadffff : pnp 00:00
    fdae0000-fdaeffff : INT344B:00
      fdae0000-fdaeffff : INT344B:00
    fdaf0000-fdafffff : INT344B:00
      fdaf0000-fdafffff : INT344B:00
    fdb00000-fdffffff : pnp 00:00
      fdc6000c-fdc6000f : iTCO_wdt
        fdc6000c-fdc6000f : iTCO_wdt
    fe000000-fe01ffff : pnp 00:00
    fe036000-fe03bfff : pnp 00:00
    fe03d000-fe3fffff : pnp 00:00
    fe410000-fe7fffff : pnp 00:00
fec00000-fec00fff : Reserved
  fec00000-fec003ff : IOAPIC 0
fed00000-fed00fff : Reserved
  fed00000-fed003ff : HPET 0
    fed00000-fed003ff : PNP0103:00
fed10000-fed19fff : Reserved
  fed10000-fed17fff : pnp 00:08
  fed18000-fed18fff : pnp 00:08
  fed19000-fed19fff : pnp 00:08
fed20000-fed3ffff : pnp 00:08
fed40000-fed40fff : MSFT0101:00
  fed40000-fed40fff : MSFT0101:00
fed84000-fed84fff : Reserved
fed90000-fed90fff : dmar0
fed91000-fed91fff : dmar1
fee00000-fee00fff : Local APIC
  fee00000-fee00fff : Reserved
ff000000-ffffffff : INT0800:00
  ffa00000-ffffffff : Reserved
100000000-46effffff : System RAM // 4G ~ 17G
  11ba00000-11c600e50 : Kernel code
  11c600e51-11cf4c53f : Kernel data
  11d4a7000-11d7fffff : Kernel bss
46f000000-46fffffff : RAM buffer // 17G ~ 17G


```

```
0000-0cf7 : PCI Bus 0000:00
  0000-001f : dma1
  0020-0021 : pic1
  0040-0043 : timer0
  0050-0053 : timer1
  0060-0060 : keyboard
  0064-0064 : keyboard
  0070-0077 : rtc0
  0080-008f : dma page reg
  00a0-00a1 : pic2
  00c0-00df : dma2
  00f0-00ff : fpu
  0400-041f : iTCO_wdt
    0400-041f : iTCO_wdt
  0680-069f : pnp 00:02
0cf8-0cff : PCI conf1
0d00-ffff : PCI Bus 0000:00
  164e-164f : pnp 00:02
  1800-18fe : pnp 00:02
    1800-1803 : ACPI PM1a_EVT_BLK
    1804-1805 : ACPI PM1a_CNT_BLK
    1808-180b : ACPI PM_TMR
    1810-1815 : ACPI CPU throttle
    1830-1833 : iTCO_wdt
      1830-1833 : iTCO_wdt
    1850-1850 : ACPI PM2_CNT_BLK
    1854-1857 : pnp 00:04
    1880-189f : ACPI GPE0_BLK
  2000-20fe : pnp 00:01
  3000-3fff : PCI Bus 0000:01
    3000-307f : 0000:01:00.0
  4000-403f : 0000:00:02.0
  4040-405f : 0000:00:1f.4
    4040-405f : i801_smbus
  ffff-ffff : pnp 00:02
    ffff-ffff : pnp 00:02
      ffff-ffff : pnp 00:02
```
