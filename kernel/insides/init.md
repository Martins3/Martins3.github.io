
# First steps in the kernel code

You will find here a couple of posts which describe the full cycle of kernel initialization from its first step after the kernel has been decompressed to the start of the first process run by the kernel itself.

> kernel compression  如何设计 ?
> 1. when enter the compression, what has been prepared ?



```
jmp    *%rax
```

At this moment the rax register contains address of the Linux kernel entry point which was obtained as a result of the call of the `decompress_kernel` function from the `arch/x86/boot/compressed/misc.c` source code file.
> @todo misc.c 中的 `#include "../voffset.h"`  我不知道为什么这一个位置不报错 ?
> @todo 而且 decompress_kernel is replaced


As we already know the *entry point of the decompressed kernel image starts in the arch/x86/kernel/head_64.S assembly source code file* and at the beginning of it, we can see following definitions:
> @todo 这是第多少次回到 head_64.S 中间了

```
    .text
    __HEAD
    .code64
    .globl startup_64
startup_64:
    ...
    ...
    ...
```
1. `.code64` https://stackoverflow.com/questions/57618695/doesnt-64-bit-kernel-support-ia-32-application-with-code64-assembler
2. `__HEAD` https://sourceware.org/binutils/docs/as/Section.html

include/linux/init.h
```
#define __HEAD		.section	".head.text","ax"
```
@todo 似乎nasm 中间如何定义的 section 的，section 的处理难道不是需要linker 协助吗 ?
@todo .text 和　`__HEAD` 既然都是表示 section 的含义，难道不是发生了重复吗 ?


arch/x86/kernel/vmlinux.lds.S
@todo 这一个链接脚本include一堆C头文件并且包含了众多的macro
```
	.text :  AT(ADDR(.text) - LOAD_OFFSET) {
		_text = .;
		_stext = .;
		/* bootstrapping code */
		HEAD_TEXT
		TEXT_TEXT
		SCHED_TEXT
		CPUIDLE_TEXT
		LOCK_TEXT
		KPROBES_TEXT
		ALIGN_ENTRY_TEXT_BEGIN
		ENTRY_TEXT
		IRQENTRY_TEXT
		ALIGN_ENTRY_TEXT_END
		SOFTIRQENTRY_TEXT
		*(.fixup)
		*(.gnu.warning)

#ifdef CONFIG_X86_64
		. = ALIGN(PAGE_SIZE);
		__entry_trampoline_start = .;
		_entry_trampoline = .;
		*(.entry_trampoline)
		. = ALIGN(PAGE_SIZE);
		__entry_trampoline_end = .;
		ASSERT(. - _entry_trampoline == PAGE_SIZE, "entry trampoline is too big");
#endif

#ifdef CONFIG_RETPOLINE
		__indirect_thunk_start = .;
		*(.text.__x86.indirect_thunk)
		__indirect_thunk_end = .;
#endif

		/* End of text section */
		_etext = .;
	} :text = 0x9090
```
> @todo .text 后面紧跟地址的操作似乎在ld 的文档中间看到过，而且就算是地址，那么前面定义的 `. = __START_KERNEL;` 也是无法理解的
> @todo :text = 0x9090 最后一行的操作的含义需要查询文档
> @todo 居然不同的启动阶段含有不同的链接脚本

After we sanitized CPU configuration, we call `__startup_64` function which is defined in arch/x86/kernel/head64.c:

```c
	load_delta = physaddr - (unsigned long)(_text - __START_KERNEL_map);
```
> @todo physaddr 是 `__startup_64` 的参数，但是参数的含义不知道是什么意思.

> 猜测，本节处理由于random load kernel image 导致的复杂性，此外设置地址映射空间

let's look at the definition of fixup_pointer function which returns physical address of the passed argument:
```c
static void __head *fixup_pointer(void *ptr, unsigned long physaddr)
{
    return ptr - (void *)_text + (void *)physaddr;
}
```
> 这样看，load_delta 数值就是 `__START_KERNEL_map` 的物理地址
> @todo `__START_KERNEL_map` 和 `_text` 是什么关系 ?  `__START_KERNEL_map` is a base virtual address of the kernel text

> @todo 在head_64.S 中间，定义了 NEXT_PAGE 的 macro

```c
early_top_pgt[511] -> level3_kernel_pgt[0]
level3_kernel_pgt[510] -> level2_kernel_pgt[0]
level3_kernel_pgt[511] -> level2_fixmap_pgt[0]
level2_kernel_pgt[0]   -> 512 MB kernel mapping
level2_fixmap_pgt[506] -> level1_fixmap_pgt
```
> @todo 到底是多少级别的映射，为什么要建立这种复杂的蛇皮联系，硬件限制吗 ?


Now we can see the set up of identity mapping of early page tables. In Identity Mapped Paging, virtual addresses are mapped to physical addresses identically. Let's look at it in detail.
First of all we replace `pud` and `pmd` with the pointer to first and second entry of `early_dynamic_pgts`:

```c
	/*
	 * Set up the identity mapping for the switchover.  These
	 * entries should *NOT* have the global bit set!  This also
	 * creates a bunch of nonsense entries but that is fine --
	 * it avoids problems around wraparound.
	 */

	next_pgt_ptr = fixup_pointer(&next_early_pgt, physaddr);
	pud = fixup_pointer(early_dynamic_pgts[(*next_pgt_ptr)++], physaddr);
	pmd = fixup_pointer(early_dynamic_pgts[(*next_pgt_ptr)++], physaddr);
```

> 首先找不到几个结构体的勾连关系，以及目的吧!


> 设置cr3 cr0
> boottime stack
> lgdt
> gs
> 大概说明设置的内容为 : secondary_startup_64


最后跳转到 : arch/x86/kernel/head64.c 中间开始分析 x86_64_start_kernel 函数
```c
	cr4_init_shadow();

	/* Kill off the identity-map trampoline */
	reset_early_page_tables();

	clear_bss();

	clear_page(init_top_pgt);

	/*
	 * SME support may update early_pmd_flags to include the memory
	 * encryption mask, so it needs to be called before anything
	 * that may generate a page fault.
	 */
	sme_early_init();

	kasan_early_init();
```
> @todo 没有仔细看，但是为什么要清空 early_top_pgt 啊 ? 以及他到底是什么东西 ?
> @todo amd64 手册 paging 看一下，到底是多少级别，PDE 扩展的含义是什么 ?

# Early interrupt and exception handling
And the last Type field describes type of the IDT entry. There are three different kinds of gates for interrupts:
1. Task gate
2. Interrupt gate
3. Trap gate

Interrupt and trap gates contain a far pointer to the entry point of the interrupt handler. Only one difference between these types is how CPU handles IF flag. If interrupt handler was accessed through interrupt gate, CPU clear the IF flag to prevent other interrupts while current interrupt handler executes. After that current interrupt handler executes, CPU sets the IF flag again with iret instruction.
> @todo 三个 gate 和 exception interrupt syscall 对应吗 ? 阅读amd64 手册吧!
> @todo 所以为什么我们需要不停的进入状态，然后不停的设置idt呢 ? 为什么不去一次设置好，非要螺旋上升啊!

向idt 中间注册的handler 很简单，主要处理 page fault 和 一般的exception , 处理的态度都是非常被动的!

# Last preparations before the kernel entry point
In the previous part we stopped at setting Interrupt Descriptor Table and loading it in the IDTR register. At the next step after this we can see a call of the `copy_bootdata` function:


```c
	copy_bootdata(__va(real_mode_data)); // @todo 又遇到了我们熟悉的__va()
```

In the next step, as we have copied `boot_params` structure, we need to move from the `early page tables` to the page tables for initialization process.
> 关于pages 完全不知道再说什么啊!

> 顺便还讲解了 memblock 相关的内容

最后从 x86_64_start_reservations 跳转到 start_kernel 中间:

# Kernel entry point
--被整理了--

# Architecture-specific initialization, again...
```c
early_param("gbpages", parse_direct_gbpages_on);
```

After this we can see call of the:
```c
    memblock_x86_reserve_range_setup_data();
```

In the next step we make a dump of the PCI devices with the following code:
```c
/*
 * Here we put platform-specific memory range workarounds, i.e.
 * memory known to be corrupt or otherwise in need to be reserved on
 * specific platforms.
 *
 * If this gets used more widely it could use a real dispatch mechanism.
 */
static void __init trim_platform_memory_ranges(void)
{
	trim_snb_memory();
}
```
> 关于PCI的描述，部分不一致


After the `early_dump_pci_devices`, there are a couple of function related with available memory and e820 which we collected in the `First steps in the kernel setup` part:
> 的确是两个地方都处理过 e820 的，@todo 重点分析

We will get all information with the Desktop Management Interface and following functions:
1. dmi_scan_machine();
2. dmi_memdev_walk();
> @todo 先跳过，让我们首先搞清楚 dmi 是什么也不错

The next step is parsing of the `SMP configuration`.
> @todo 解析SMP configuration, 所以SMP 可以包含的configuration 是什么?

In the next step of the `setup_arch`
we can see the call of the `early_alloc_pgt_buf` function which allocates the page table buffer for early stage.
The page table buffer will be placed in the brk area.

# The End of the architecture-specific initialization, almost...
After we relocated initrd ramdisk image, the next function is `vsmp_init` from the `arch/x86/kernel/vsmp_64.c`. This function initializes support of the `ScaleMP` `vSMP`

The next function is `io_delay_init` from the arch/x86/kernel/io_delay.c.
This function allows to override default I/O delay 0x80 port.
We already saw I/O delay in the Last preparation before transition into protected mode, now let's look on the `io_delay_init` implementation:

In the next step we need to allocate area for the Direct memory access with the `dma_contiguous_reserve` function which is defined in the drivers/base/dma-contiguous.c.

The next step is the call of the function - `x86_init.paging.pagetable_init`

The next step after `SparseMem` initialization is setting of the trampoline_cr4_features

You may remember how we made a search of the SMP configuration in the previous part.
Now we need to get the SMP configuration if we found it. For this we check `smp_found_config` variable which we set in the `smp_scan_config` function
(read about it the previous part) and call the `get_smp_config` function:
> @todo 这就非常的奇怪了，为什么smp_config 的事情不是一次完成啊!

Here we are getting to the end of the `setup_arch` function.
The rest of function of course is important, but details about these stuff will not will not be included in this part.

# 说明
1. 直接阅读，试图记忆，体验非常之差，没有必要
