# Kernel booting process. Part 1.
Memory segmentation is used to make use of all the address space available
Now the BIOS starts; after initializing and checking the hardware, the BIOS needs to find a bootable device.

## 问题:
1. 那些部分是描述bios  grub 以及 boot的(easy)
2. 为什么bios部分的地址空间和grub 的地址空间不同
3. 哪一个位置实现的real 和protect的装换的(都不是，是下一个阶段)
4. header.S中间为什么需要从`_start` 跳转到 `_start_of_setup`里面去

# Kernel booting process. Part 2.

Protected mode brought many changes, but the main one is the difference in memory management. The 20-bit address bus was replaced with a 32-bit address bus. It allowed access to 4 Gigabytes of memory vs the 1 Megabyte in real mode. Also, paging support was added, which you can read about in the next sections.

Memory management in Protected mode is divided into two, almost independent parts: **Segmentation** **Paging**

> 原来是在这一个位置做的内存检查工作，使用0xE820来实现， 也就是说此时内存还没有加载，所以之前的代码是放置在哪里的?


Ultimately, this function collects data from the address allocation table and writes this data into the e820_entry array:
1. start of memory segment
2. size of memory segment
3. type of memory segment (whether the particular segment is usable or reserved)

## 问题
1. 不是没有segment的吗? 为什么还是含有gdt作为segment descriptor?
2. copies the kernel setup header into the corresponding field of the boot_params structure 所以为什么需要这样的复制
3. head初始化为什么需要设置head_end=stack_end 以及为什么设置
```
		heap_end = (char *)
			((size_t)boot_params.hdr.heap_end_ptr + 0x200);
```




# Kernel booting process. Part 3.
## 问题
1. 所以，为是么ucore中间没有使用处理Video mode initialization
2. 为什么需要使用两次的关中断 mask_all_interrupts realmode_switch
3. io_delay为什么需要使用io_delay函数
4. 所以为什么跳转的时候需要设置所有的段寄存为`$__BOOT_DS`
```
If you paid attention, you can remember that we saved $__BOOT_DS in the cx register. Now we fill it with all segment registers besides cs (cs is already __BOOT_CS).
```
5. 为什么可以使用`addl	%ebx, %esp`来设置函数。
And setup a valid stack for debugging purposes

# Kernel booting process. Part 4.
## 问题
1. 所有数值的来源是什么，需要知道。

```
eax            0x100000	1048576
ecx            0x0	    0
edx            0x0	    0
ebx            0x0	    0
esp            0x1ff5c	0x1ff5c
ebp            0x0	    0x0
esi            0x14470	83056
edi            0x0	    0
eip            0x100000	0x100000 // j *eax获取的
eflags         0x46	    [ PF ZF ]　// 似乎没有位置
cs             0x10	16　// 
ss             0x18	24
ds             0x18	24
es             0x18	24
fs             0x18	24
gs             0x18	24
```




