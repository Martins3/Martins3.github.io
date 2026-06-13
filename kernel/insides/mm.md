# memblock
`start_kernel`: This function initializes all the kernel features (including architecture-dependent features) before the kernel runs the first init process.
> main.c , call all kinds of 

You may remember as we built ***early page tables***, ***identity page tables*** and ***fixmap page tables*** in the boot time.

> memblock.c 开始位置的注释比这一个讲解的很好

### 关键的结构体
下面三个结构体逐个嵌套起来。
```c
/**
 * struct memblock_region - represents a memory region
 * @base: physical address of the region
 * @size: size of the region
 * @flags: memory region attributes
 * @nid: NUMA node id
 */
struct memblock_region {
	phys_addr_t base;
	phys_addr_t size;
	enum memblock_flags flags;
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
	int nid;
#endif
};


/**
 * struct memblock_type - collection of memory regions of certain type
 * @cnt: number of regions
 * @max: size of the allocated array
 * @total_size: size of all regions
 * @regions: array of regions
 * @name: the memory type symbolic name
 */
struct memblock_type {
	unsigned long cnt;
	unsigned long max;
	phys_addr_t total_size;
	struct memblock_region *regions;
	char *name;
};


/**
 * struct memblock - memblock allocator metadata
 * @bottom_up: is bottom up direction?
 * @current_limit: physical address of the current allocation limit
 * @memory: usabe memory regions
 * @reserved: reserved memory regions
 * @physmem: all physical memory
 */
struct memblock {
	bool bottom_up;  /* is bottom up direction? */
	phys_addr_t current_limit;
	struct memblock_type memory;
	struct memblock_type reserved;
#ifdef CONFIG_HAVE_MEMBLOCK_PHYS_MAP
	struct memblock_type physmem;
#endif
};
```

### 核心函数
To understand how memblock works and how it is implemented, let's look at its usage first. 
There are a couple of places in the linux kernel where memblock is used. For example let's take `memblock_x86_fill`
function from the `arch/x86/kernel/e820.c`.
This function goes through the memory map provided by the e820 and
adds memory regions reserved by the kernel to the memblock with the `memblock_add` function.
Since we have met the `memblock_add` function first,

```c
void __init e820__memblock_setup(void)
{
	int i;
	u64 end;

	/*
	 * The bootstrap memblock region count maximum is 128 entries
	 * (INIT_MEMBLOCK_REGIONS), but EFI might pass us more E820 entries
	 * than that - so allow memblock resizing.
	 *
	 * This is safe, because this call happens pretty late during x86 setup,
	 * so we know about reserved memory regions already. (This is important
	 * so that memblock resizing does no stomp over reserved areas.)
	 */
	memblock_allow_resize();

	for (i = 0; i < e820_table->nr_entries; i++) {
		struct e820_entry *entry = &e820_table->entries[i];

		end = entry->addr + entry->size;
		if (end != (resource_size_t)end)
			continue;

		if (entry->type != E820_TYPE_RAM && entry->type != E820_TYPE_RESERVED_KERN)
			continue;

		memblock_add(entry->addr, entry->size);
	}

	/* Throw away partial pages: */
	memblock_trim_memory(PAGE_SIZE);

	memblock_dump_all();
}
```

> 本文接下重点分析了 memory_add_node 中间的内容。



### 内存控制权的转移
首先是通过e820的机制将所有的内存给memblock的机制管理起来，当内核中间的内存系统启动之后，
然后将这些东西全部释放给buddy system

```c
/**
 * memblock_free_all - release free pages to the buddy allocator
 *
 * Return: the number of pages actually released.
 */
unsigned long __init memblock_free_all(void)
{
	unsigned long pages;

	reset_all_zones_managed_pages();

	pages = free_low_memory_core_early();
	totalram_pages_add(pages);

	return pages;
}

void __init mem_init(void)
{
	pci_iommu_alloc();

	/* clear_bss() already clear the empty_zero_page */

	/* this will put all memory onto the freelists */
	memblock_free_all();
	after_bootmem = 1;
	x86_init.hyper.init_after_bootmem();

	/*
	 * Must be done after boot memory is put on freelist, because here we
	 * might set fields in deferred struct pages that have not yet been
	 * initialized, and memblock_free_all() initializes all the reserved
	 * deferred pages for us.
	 */
	register_page_bootmem_info();

	/* Register memory areas for /proc/kcore */
	if (get_gate_vma(&init_mm))
		kclist_add(&kcore_vsyscall, (void *)VSYSCALL_ADDR, PAGE_SIZE, KCORE_USER);

	mem_init_print_info(NULL);
}


/*
 * Set up kernel memory allocators
 */
static void __init mm_init(void)
{
	/*
	 * page_ext requires contiguous pages,
	 * bigger than MAX_ORDER unless SPARSEMEM.
	 */
	page_ext_init_flatmem();
	mem_init();
	kmem_cache_init();
	pgtable_init();
	debug_objects_mem_init();
	vmalloc_init();
	ioremap_huge_init();
	/* Should be run before the first non-init thread is created */
	init_espfix_bsp();
	/* Should be run after espfix64 is set up. */
	pti_init();
}


asmlinkage __visible void __init start_kernel(void)
{

}
```

# Fix-Mapped Addresses and ioremap 
`Fix-Mapped` addresses are a set of special compile-time addresses whose corresponding physical addresses do not have to be a linear address minus `__START_KERNEL_map`. 

 Each fix-mapped address maps one page frame and the kernel uses them as pointers that never change their address

 > Fix map中的fix指的是固定的意思，那么固定什么东西呢？其实就是虚拟地址是固定的，也就是说，有些虚拟地址在编译（compile-time）的时候就固定下来了，而这些虚拟地址对应的物理地址不是固定的，是在kernel启动过程中被确定的。
 > 说实话，内核中间为什么虚拟地址不是确定的，凭什么?

# Introduction to the kmemcheck in the Linux kernel
All memory-mapped I/O addresses are not used by the kernel directly.
So, before the Linux kernel can use such memory, it must map it to the virtual memory space which is the main purpose of the ioremap mechanism.
