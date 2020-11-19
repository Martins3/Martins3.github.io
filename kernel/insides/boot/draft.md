# Booting
https://en.wikipedia.org/wiki/Reset_vector

The reset vector is the default location a central processing unit will go to find the first instruction it will execute after a reset. The reset vector is a pointer or address, where the CPU should always begin as soon as it is able to execute instructions. The address is in a section of non-volatile memory initialized to contain instructions to start the operation of the CPU, as the first step in the process of booting the system containing the CPU.
> reset_vector 实际地址在ROM 中间，NB

It contains a jump (jmp) instruction that usually points to the BIOS entry point.
> 然后立即跳转到 BIOS 中间，所以为什么不直接将reset_vector 设置到 bios 的entry point 中间啊 ?

CPU 上电，跳转到BIOS, BIOS 的源代码，参考coreboot, BIOS 检查硬件，查询启动盘，

(the origin is set to 0x7c00 and we end it with the magic sequence)
> 应该是读取sector 通过magic sequence 发现是 boot sector 之后，然后将sector 加载内存中间的 0x7c00 中间

A real-world boot sector has code for continuing the boot process and **a partition table** instead of a bunch of 0's and an exclamation mark :)

In the beginning of this post, I wrote that the first instruction executed by the CPU is located at address 0xFFFFFFF0, which is much larger than 0xFFFFF (1MB). How can the CPU access this address in real mode? The answer is in the coreboot documentation:
```
0xFFFE_0000 - 0xFFFF_FFFF: 128 kilobyte ROM mapped into address space
```
At the start of execution, the BIOS is not in RAM, but in ROM.
> @todo 又是一个map ，但是不知道是如何实现的

In general, real mode's memory map is as follows:
```
0x00000000 - 0x000003FF - Real Mode Interrupt Vector Table
0x00000400 - 0x000004FF - BIOS Data Area
0x00000500 - 0x00007BFF - Unused
0x00007C00 - 0x00007DFF - Our Bootloader
0x00007E00 - 0x0009FFFF - Unused
0x000A0000 - 0x000BFFFF - Video RAM (VRAM) Memory
0x000B0000 - 0x000B7777 - Monochrome Video Memory
0x000B8000 - 0x000BFFFF - Color Video Memory
0x000C0000 - 0x000C7FFF - Video ROM BIOS
0x000C8000 - 0x000EFFFF - BIOS Shadow Area
0x000F0000 - 0x000FFFFF - System BIOS
```
> 但是我们并不知道如何将这一个map 建立起来

Continuing from before, now that the **BIOS has chosen a boot device** and transferred control to the **boot sector code**, execution starts from **boot.img**.

This code is very simple, due to the limited amount of space available, and contains a pointer which is used to **jump to the location of GRUB 2's core image**

The core image **begins with diskboot.img**, which is usually stored immediately after the first sector in the unused space before the first partition. 
The above code **loads the rest of the core image**, which contains GRUB 2's kernel and drivers for handling filesystems, into memory. 
After loading the rest of the core image, it executes the **grub_main** function.

The grub_main function initializes the console, gets the base address for modules, sets the root device, loads/parses the grub configuration file, loads modules, etc. At the end of execution, the grub_main function moves grub to normal mode. The grub_normal_execute function (from the grub-core/normal/main.c source code file) completes the final preparations and shows a menu to select an operating system.




As we can read in the kernel boot protocol, the bootloader must read and fill some fields of the kernel setup header, which starts at the 0x01f1 offset from the kernel setup code.


As we can see in the kernel boot protocol, the memory will be mapped as follows after loading the kernel:

```
         | Protected-mode kernel  |
100000   +------------------------+
         | I/O memory hole        |
0A0000   +------------------------+
         | Reserved for BIOS      | Leave as much as possible unused
         ~                        ~
         | Command line           | (Can also be below the X+10000 mark)
X+10000  +------------------------+
         | Stack/heap             | For use by the kernel real-mode code.
X+08000  +------------------------+
         | Kernel setup           | The kernel real-mode code.
         | Kernel boot sector     | The kernel legacy boot sector.
       X +------------------------+
         | Boot loader            | <- Boot sector entry point 0x7C00
001000   +------------------------+
         | Reserved for MBR/BIOS  |
000800   +------------------------+
         | Typically used by MBR  |
000600   +------------------------+
         | BIOS use only          |
000000   +------------------------+
So, when the bootloader transfers control to the kernel, it sta
```
The bootloader has now loaded the Linux kernel into memory, filled the header fields, and then jumped to the corresponding memory address. We can now move directly to the kernel setup code.
> bios 选择正确的sector 并且从其中执行， bootload(grub) 将image 加载内存中间

 After all these things are done, the kernel setup part will decompress the actual kernel and jump to it. Execution of the setup part starts from arch/x86/boot/header.S at the `_start` symbol.

 ```c
     .globl _start
_start:
    .byte  0xeb
    .byte  start_of_setup-1f // @todo 这到底是跳转到哪里去啊 ?
1:
 ```
After the jump to `start_of_setup`, the kernel needs to do the following:

- Make sure that all segment register values are equal
- Set up a correct stack, if needed
- Set up `bss`
- Jump to the C code in arch/x86/boot/main.c

> 文章算是比较详细的分析了从 start_of_setup 到 main 之间的大约30行汇编代码，其大致内容如上。
> 完成的内容就是real mode 最基本的初始化功能

# First steps in the kernel setup
In this part, we will continue to research the kernel setup code and go over
- what protected mode is,
- the transition into it,
- the initialization of the heap and the console,
- memory detection, CPU validation and keyboard initialization
- and much much more.
> @todo 如果是阅读amd64 的手册，一定想不到，初始化的过程为何要初始化 console  和 keyboard initialization , 现在初始化的必定不是最终版本的，而且感觉现在用不到啊

**Segment registers contain segment selectors as in real mode**. However, in protected mode, a segment selector is handled differently. Each Segment Descriptor has an associated Segment Selector which is a 16-bit structure.

Every segment register has a visible and a hidden part.
- Visible - The Segment Selector is stored here.
- Hidden - The Segment Descriptor (which contains the base, limit, attributes & flags) is stored here

`copy_boot_params();`
It copies the kernel setup header into the corresponding field of the `boot_params` structure which is defined in the arch/x86/include/uapi/asm/bootparam.h header file.
> 1. boot_params 是什么 ? 干什么的 ?
> 2. zeropage ?
> 3. 据说hdr是bootloader 进行填充的

```c
/*
 * Copy the header into the boot parameter block.  Since this
 * screws up the old-style command line protocol, adjust by
 * filling in the new-style command line pointer instead.
 */

static void copy_boot_params(void)
{
	struct old_cmdline {
		u16 cl_magic;
		u16 cl_offset;
	};
	const struct old_cmdline * const oldcmd =
		(const struct old_cmdline *)OLD_CL_ADDRESS;

	BUILD_BUG_ON(sizeof boot_params != 4096);
	memcpy(&boot_params.hdr, &hdr, sizeof hdr);

	if (!boot_params.hdr.cmd_line_ptr &&                  // 主要处理事情是兼容性的问题，并不关心
	    oldcmd->cl_magic == OLD_CL_MAGIC) {
		/* Old-style command line protocol. */
		u16 cmdline_seg;

		/* Figure out if the command line falls in the region
		   of memory that an old kernel would have copied up
		   to 0x90000... */
		if (oldcmd->cl_offset < boot_params.hdr.setup_move_size)
			cmdline_seg = ds();
		else
			cmdline_seg = 0x9000;

		boot_params.hdr.cmd_line_ptr =
			(cmdline_seg << 4) + oldcmd->cl_offset;
	}
}
```

After serial port initialization we can see the first output.
> 其实，并没有分析初始化，是讲解了当初始化工作完成之后，puts 是如何实现的

```c

		puts("early console in setup code\n");

void __attribute__((section(".inittext"))) puts(const char *str)
{
	while (*str)
		putchar(*str++);
}


void __attribute__((section(".inittext"))) putchar(int ch)
{
	if (ch == '\n')
		putchar('\r');	/* \n -> \r\n */

	bios_putchar(ch);

	if (early_serial_base != 0)
		serial_putchar(ch);
}

static void __attribute__((section(".inittext"))) bios_putchar(int ch)
{
	struct biosregs ireg;

	initregs(&ireg);
	ireg.bx = 0x0007;
	ireg.cx = 0x0001;
	ireg.ah = 0x0e;
	ireg.al = ch;
	intcall(0x10, &ireg, NULL); // 汇编程序
}
```

`init_heap();`
> 不知道为什么只是设置了 heap_end 没有设置 heap 开始的位置

The `set_bios_mode` function executes the 0x15 BIOS interrupt to tell the BIOS that long mode (if bx == 2) will be used.


`detect_memory();`
We will see only the implementation of the 0xE820 interface here.
Ultimately, this function collects data from the address allocation table and writes this data into the e820_entry array:
- start of memory segment
- size of memory segment
- type of memory segment (whether the particular segment is usable or reserved)
> 似乎，在init 部分，还有更加复杂的e820 的操作


`keyboard_init();`
> 并没有什么特殊的，又是int 就完事了

```c
	/* Query Intel SpeedStep (IST) information */
	query_ist();

	/* Query APM information */
#if defined(CONFIG_APM) || defined(CONFIG_APM_MODULE)
	query_apm_bios();
#endif

	/* Query EDD information */
#if defined(CONFIG_EDD) || defined(CONFIG_EDD_MODULE)
	query_edd();
#endif
```
> 三个query只有只有 query_ist 含有作用，而且实现和 query_ist 差不多，
> 感觉这些 arch specific 的东西意义不大

# Video mode initialization and transition to protected mode
```c
	RESET_HEAP(); // 在init video mode 中间初始化的heap


#define RESET_HEAP() ((void *)( HEAP = _end ))
```
> @todo video 初始化过程没有兴趣，也许看一看ucore 中间的内容吧




```c
/*
 * Actual invocation sequence
 */
void go_to_protected_mode(void)
{
	/* Hook before leaving real mode, also disables interrupts */
	realmode_switch_hook();

	/* Enable the A20 gate */
	if (enable_a20()) {
		puts("A20 gate not responding, unable to boot...\n");
		die();
	}

	/* Reset coprocessor (IGNNE#) */
	reset_coprocessor();

	/* Mask all interrupts in the PIC */
	mask_all_interrupts();

	/* Actual transition to protected mode... */
	setup_idt(); // 这是空值
	setup_gdt(); // CS DS 的base 和 limit 设置其实就是糊弄segment 而已
	protected_mode_jump(boot_params.hdr.code32_start,
			    (u32)&boot_params + (ds() << 4)); // @todo 这就是因为 ds << 4 + offset 吗 ? 但是难道 &boot_params 获取都不是effective address， 而只是偏移而已
}
```

After this, we shift bx by 4 bits and add it to the memory location labeled 2 (which is (cs << 4) + in_pm32, the physical address to jump after transitioned to 32-bit mode).
> @todo ebx 之后会成为 esp , 所以stack 会成为 32-bit mode 的stack


# The Transition to 64-bit mode
当执行完成之后，此时cs = 0x10 ip = 0x100000  此时cs 的数值表示 rpl = 0 选择gdt 第二个，而前面初始化gdt 的第二项为 GDT_ENTRY_BOOT_CS

`/home/shen/linux/arch/x86/boot/compressed/head_64.S` 中入口地址:
First, why is the directory named compressed? The answer to that is that bzimage is a gzipped package consisting of vmlinux, header and kernel setup code. We looked at kernel setup code in all of the previous parts.
The main goal of the code in `head_64.S` is to prepare to enter long mode, enter it and then decompress the kernel

```
	__HEAD
	.code32
ENTRY(startup_32)
...
ENDPROC(startup_32)
```

```c
#define __HEAD        .section    ".head.text","ax"
```

> **Reload the segments if needed** @todo 此小节没有阅读，等我TM学一下linker脚本之后再来

```c
/* setup a stack and make sure cpu supports long mode. */
	movl	$boot_stack_end, %eax
	addl	%ebp, %eax
	movl	%eax, %esp
```
First of all, we put the address of `boot_stack_end` into the eax register,
so the eax register contains the address of `boot_stack_end` as it was linked, which is 0x0 + `boot_stack_end`.
To get the real address of `boot_stack_end`, we need to add the real address of the `startup_32` function.
We've already found this address and put it into the ebp register. In the end, the eax register will contain the real address of `boot_stack_end` and we just need to set the stack pointer to it.

> **Calculate the relocation address** 和 **Reload the segments if needed** @todo

After we *get the address to relocate the compressed kernel image to*, we need to do one last step before we can transition to 64-bit mode.


> @todo page table build 过程没有仔细分析, boot 后面关于compress的和kernel load address randomization 等下一波的时候在分析吧! 等到ucore 看完 ?



# 猜测内容
1. 主要内容: 当执行init 或者 reset 之后，bios 已经把什么读取到FFFF_0000h中间了
2. 如何一步步切换到long mode 中间 ?
3. 将镜像读入到内核中间并且加以解压
    1. 那么压缩程序如何读入 ?　读入而已，又不是压缩程序被需要解压
    2. 解压缩中间同时确定符号的位置的确定，静态链接的，应该很容易实现
 
# 补充材料
https://manybutfinite.com/post/how-computers-boot-up/ 一篇文章

Boot and Init are two separate part, so what is the boundary ?
> 很显然，在x86文件夹下面，含有boot 和 init 两个文件夹，找到从boot 跳转到 init 中间即可

