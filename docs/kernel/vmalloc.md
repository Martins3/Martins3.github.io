## mm/vmalloc.c

1. 分配物理页面和连续虚拟地址空间，然后勾连起来
2. 物理页面的最小的大小为一个 page


## VMALLOC_START

1. arch/x86/Kconfig
```plain
config DYNAMIC_MEMORY_LAYOUT
	bool
	---help---
	  This option makes base addresses of vmalloc and vmemmap as well as
	  __PAGE_OFFSET movable during boot.

config RANDOMIZE_MEMORY
	bool "Randomize the kernel memory sections"
	depends on X86_64
	depends on RANDOMIZE_BASE
	select DYNAMIC_MEMORY_LAYOUT
	default RANDOMIZE_BASE
	---help---
	   Randomizes the base virtual address of kernel memory sections
	   (physical memory mapping, vmalloc & vmemmap). This security feature
	   makes exploits relying on predictable memory locations less reliable.

	   The order of allocations remains unchanged. Entropy is generated in
	   the same way as RANDOMIZE_BASE. Current implementation in the optimal
	   configuration have in average 30,000 different possible virtual
	   addresses for each memory section.

	   If unsure, say Y.

config RANDOMIZE_MEMORY_PHYSICAL_PADDING
	hex "Physical memory mapping padding" if EXPERT
	depends on RANDOMIZE_MEMORY
	default "0xa" if MEMORY_HOTPLUG
	default "0x0"
	range 0x1 0x40 if MEMORY_HOTPLUG
	range 0x0 0x40
	---help---
	   Define the padding in terabytes added to the existing physical
	   memory size during kernel memory randomization. It is useful
	   for memory hotplug support but reduces the entropy available for
	   address randomization.

	   If unsure, leave at the default value.
```
2. @todo 我是万万没有想到，其实 x86 的内存空间是可以配置的，需要细品。



## remove_vm_area 和 get_vm_area

1. 处理虚拟地址空间上的分配工作
2. 参考书上的描述，非常清晰了
3. 新引入的内容 : vmap_area (可能是为了更加高效的查询，使用了 rbtree 机制)

```c
void __init vmalloc_init(void)
{
	struct vmap_area *va;
	struct vm_struct *tmp;
	int i;

	for_each_possible_cpu(i) {
		struct vmap_block_queue *vbq;
		struct vfree_deferred *p;

		vbq = &per_cpu(vmap_block_queue, i);
		spin_lock_init(&vbq->lock);
		INIT_LIST_HEAD(&vbq->free);
		p = &per_cpu(vfree_deferred, i);
		init_llist_head(&p->list);
		INIT_WORK(&p->wq, free_work);
	}

	/* Import existing vmlist entries. */
	for (tmp = vmlist; tmp; tmp = tmp->next) {
		va = kzalloc(sizeof(struct vmap_area), GFP_NOWAIT);
		va->flags = VM_VM_AREA;
		va->va_start = (unsigned long)tmp->addr;
		va->va_end = va->va_start + tmp->size;
		va->vm = tmp;
		__insert_vmap_area(va);
	}

	vmap_area_pcpu_hole = VMALLOC_END;

	vmap_initialized = true;
}
```

## vmalloc 实现机制
1. 参考书上的内容


```c
/**
 *	vmalloc  -  allocate virtually contiguous memory
 *	@size:		allocation size
 *	Allocate enough pages to cover @size from the page level
 *	allocator and map them into contiguous kernel virtual space.
 *
 *	For tight control over page level allocator and protection flags
 *	use __vmalloc() instead.
 */
void *vmalloc(unsigned long size) { return __vmalloc_node_flags(size, NUMA_NO_NODE, GFP_KERNEL); }

static inline void *__vmalloc_node_flags(unsigned long size, int node, gfp_t flags) { return __vmalloc_node(size, 1, flags, PAGE_KERNEL, node, __builtin_return_address(0)); }

/**
 *	__vmalloc_node  -  allocate virtually contiguous memory
 *	@size:		allocation size
 *	@align:		desired alignment
 *	@gfp_mask:	flags for the page level allocator
 *	@prot:		protection mask for the allocated pages
 *	@node:		node to use for allocation or NUMA_NO_NODE
 *	@caller:	caller's return address
 *
 *	Allocate enough pages to cover @size from the page level
 *	allocator with @gfp_mask flags.  Map them into contiguous
 *	kernel virtual space, using a pagetable protection of @prot.
 *
 *	Reclaim modifiers in @gfp_mask - __GFP_NORETRY, __GFP_RETRY_MAYFAIL
 *	and __GFP_NOFAIL are not supported
 *
 *	Any use of gfp flags outside of GFP_KERNEL should be consulted
 *	with mm people.
 *
 */
static void *__vmalloc_node(unsigned long size, unsigned long align, gfp_t gfp_mask, pgprot_t prot, int node, const void *caller) { return __vmalloc_node_range(size, align, VMALLOC_START, VMALLOC_END, gfp_mask, prot, 0, node, caller); }

/**
 *	__vmalloc_node_range  -  allocate virtually contiguous memory
 *	@size:		allocation size
 *	@align:		desired alignment
 *	@start:		vm area range start
 *	@end:		vm area range end
 *	@gfp_mask:	flags for the page level allocator
 *	@prot:		protection mask for the allocated pages
 *	@vm_flags:	additional vm area flags (e.g. %VM_NO_GUARD)
 *	@node:		node to use for allocation or NUMA_NO_NODE
 *	@caller:	caller's return address
 *
 *	Allocate enough pages to cover @size from the page level
 *	allocator with @gfp_mask flags.  Map them into contiguous
 *	kernel virtual space, using a pagetable protection of @prot.
 */
void *__vmalloc_node_range(unsigned long size, unsigned long align,
			unsigned long start, unsigned long end, gfp_t gfp_mask,
			pgprot_t prot, unsigned long vm_flags, int node,
			const void *caller)
{
	struct vm_struct *area;
	void *addr;
	unsigned long real_size = size;

	size = PAGE_ALIGN(size);
	if (!size || (size >> PAGE_SHIFT) > totalram_pages)
		goto fail;

	area = __get_vm_area_node(size, align, VM_ALLOC | VM_UNINITIALIZED | // 首先获取 vm_struct 也就是虚拟空间上分配
				vm_flags, start, end, node, gfp_mask, caller);
	if (!area)
		goto fail;

	addr = __vmalloc_area_node(area, gfp_mask, prot, node); // 将虚拟空间和物理空间勾连起来。
	if (!addr)
		return NULL;

	/*
	 * In this function, newly allocated vm_struct has VM_UNINITIALIZED
	 * flag. It means that vm_struct is not fully initialized.
	 * Now, it is fully initialized, so remove this flag here.
	 */
	clear_vm_uninitialized_flag(area);

	kmemleak_vmalloc(area, size, gfp_mask);

	return addr;

fail:
	warn_alloc(gfp_mask, NULL,
			  "vmalloc: allocation failure: %lu bytes", real_size);
	return NULL;
}

static void *__vmalloc_area_node(struct vm_struct *area, gfp_t gfp_mask,
				 pgprot_t prot, int node) // 和书上描述完全相同，此函数将调用 alloc_page 分配物理空间，然后将 vm_struct 和 pages 勾连起来
         // todo 如果想知道 page table 的修改过程可以找到此处 !
```


## doc & ref
1. 为什么需要使用 vmalloc ? https://stackoverflow.com/questions/116343/what-is-the-difference-between-vmalloc-and-kmalloc
2. 如果没有超过 page size 使用 kmalloc 机制，向 buddy system 询问 ?

https://gcc.gnu.org/onlinedocs/gcc/Return-Address.html
https://stackoverflow.com/questions/1693011/how-can-i-determine-the-return-address-on-stack


## vmap

给定了 pages, 只需要分配 vm_struct ，然后利用 map_vm_area 将两者勾连起来即可。

```c
/**
 *	vmap  -  map an array of pages into virtually contiguous space
 *	@pages:		array of page pointers
 *	@count:		number of pages to map
 *	@flags:		vm_area->flags
 *	@prot:		page protection for the mapping
 *
 *	Maps @count pages from @pages into contiguous kernel virtual
 *	space.
 */
void *vmap(struct page **pages, unsigned int count,
		unsigned long flags, pgprot_t prot)
{
	struct vm_struct *area;
	unsigned long size;		/* In bytes */

	might_sleep();

	if (count > totalram_pages)
		return NULL;

	size = (unsigned long)count << PAGE_SHIFT;
	area = get_vm_area_caller(size, flags, __builtin_return_address(0));
	if (!area)
		return NULL;

	if (map_vm_area(area, prot, pages)) {
		vunmap(area->addr);
		return NULL;
	}

	return area->addr;
}
EXPORT_SYMBOL(vmap);
```
