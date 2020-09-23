---
title: Learn OS
date: 2018-04-19 18:09:37
tags: os
---

# Practice

## [cfenollosa/os-tutorial](https://github.com/cfenollosa/os-tutorial)
1. what is tty mode ?
[doc](http://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
Luckily, what we do have is the Basic Input/Output Software (BIOS), a collection of software routines that are
initially loaded from a chip into memory and initialised when the computer is switched on

Here, we are reminded, though, that BIOS cannot simply load a
file that represents your operating system from a disk, since BIOS has no notion of a filesystem

BIOS must read specific sectors of data (usually 512 bytes in size) from specific physical locations of the disk devices

The emulators translate low-level display device instructions into pixel rendering on
a desktop window, so you can see exactly what would be rendered on a real monitor.
> If in the real machine, how to debug and test the code 

we used the `-f bin` option to instruct nasm to produce raw machine code.

Registers are extended to 32 bits, with their full capacity being accessed by prefixing an e to the register name, for example: mov ebx, 0x274fe8fe
1. For convenience, there are two additional general purpose segment registers, **fs** and **gs**.
2. 32-bit memory offsets are available, so an offset can reference a whopping 4 GB of memory (0xffffffff).
3. The CPU supports a more sophisticated --- though slightly more complex ---
means of memory segmentation, which offers two big advantages:
    1. Code in one segment can be prohibited from executing code in a more privilidged segment, so you can protect your kernel code from user applications
    2. The CPU can implement virtual memory for user processes, such that pages
(i.e. fixed-sized chunks) of a process’s memory can be swapped transparently
between the disk and memory on an as-needed basis. This ensure main
memory is used efficiently, in that code or data that is rarely executed needn’t hog valuable memory.
4. Interrupt handling is also more sophisticated
[global descriptor table](https://en.wikibooks.org/wiki/X86_Assembly/Global_Descriptor_Table): which defines memory segments and their protected-mode attributes.


Compile the first kernle with C:
1. qemu -fda Use file as floppy disk 0/1 image
2. 



### question
1. make the code run on the raspiberry pif !
2. why **Loaded Boot Sector** is located at 0x7c00
3. [org 0x7c00] the line means what, why just this file has it !
4. can we make a tutorial for mips, as we know, there is no such thing as real mode and protected mode.
5. 32-bit protected mode : is there something such as 32-bit real mode 
6. what's gdt, describe it in details.
7. **ld** 


## assembly language
文档
https://software.intel.com/en-us/articles/intel-sdm
https://software.intel.com/en-us/articles/introduction-to-x64-assembly

输出一个hello world !
https://stackoverflow.com/questions/27594297/how-to-print-a-string-to-the-terminal-in-x86-64-assembly-nasm-without-syscall

## ucore
https://www.nasm.us/

# Theory
How to create an another Android ?

## Doc & Ref
1. [osdev](https://wiki.osdev.org/Main_Page) Provide almost resource about how to create a new os !
2. 

## doubly freeing memory
https://www.owasp.org/index.php/Doubly_freeing_memory

## tool
https://setupstepbystep.wordpress.com/2017/02/16/running-bochs-emulator-using-kernel-exe-grub-conf-file/
