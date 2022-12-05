# loader script

## https://sourceware.org/binutils/docs/ld/Scripts.html
The main purpose of the linker script is to describe how the sections in the input files should be mapped into the output file, and to control the memory layout of the output file.
Most linker scripts do nothing more than this. However, when necessary, the linker script can also direct the linker to perform many other operations, using the commands described below.

You may supply your own linker script by using the ‘-T’ command line option. When you do this, your linker script will replace the *default linker script*.
> the content of *default linker script*

Every loadable or allocatable output section has two addresses. The first is the VMA, or virtual memory address. This is the address the section will have when the output file is run. The second is the LMA, or load memory address. This is the address at which the section will be loaded. In most cases the two addresses will be the same. An example of when they might be different is when a data section is loaded into ROM, and then copied into RAM when the program starts up (this technique is often used to initialize global variables in a ROM based system). In this case the ROM address would be the LMA, and the RAM address would be the VMA.
> loaded into ROM ?

The simplest possible linker script has just one command: ‘SECTIONS’. You use the ‘SECTIONS’ command to describe the memory layout of the output file.

The first line inside the ‘SECTIONS’ command of the above example sets the value of the special symbol ‘.’, which is the location counter.

The second line defines an output section, ‘.text’. The colon is required syntax which may be ignored for now. Within the curly braces after the output section name, you list the names of the input sections which should be placed into this output section. The ‘*’ is a wildcard which matches any file name. The expression ‘*(.text)’ means all ‘.text’ input sections in all input files.

Simple Linker Script Commands
1. The first instruction to execute in a program is called the entry point. You can use the ENTRY linker script command to set the entry point. The argument is a symbol name: `ENTRY(symbol)`
> 似乎可以通过linker 设置入口位置
2. https://sourceware.org/binutils/docs/ld/File-Commands.html#File-Commands
3. https://en.wikipedia.org/wiki/Binary_File_Descriptor_library

> I need some examples

# 资源
https://github.com/bravegnu/gnu-eprog

https://github.com/davepfeiffer/embedded-makefile-flow

https://twilco.github.io/riscv-from-scratch/2019/03/10/riscv-from-scratch-1.html riscv 顺便找到的东西

https://access.redhat.com/documentation/en-US/Red_Hat_Enterprise_Linux/4/html/Using_ld_the_GNU_Linker/index.html redhat 详细

https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_chapter/ld_3.html 官方的文档 其实很短的
## 问题
1. https://0xax.github.io/categories/assembler/
    - Call C from assembly
        - ld   -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc casm.o c.o -o casm

## PIC 和 PIE 的问题
- https://stackoverflow.com/questions/36968287/why-doesnt-gcc-reference-the-plt-for-function-calls
