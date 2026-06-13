# Efficient Memory Virtualization for Cross-ISA System Mode Emulation

> 所以，corss-ISA 就不可以使用 shadow page table 了 ?
> 1. 因为没有办法模拟寄存器 ?
> 2. 因为需要硬件支持，比如 AMD 和 Intel 的支持，所以，那么这些硬件支持提供的接口是怎么样子的啊!

EPST embeds a shadow page table into the address space of a cross-ISA dynamic bina- ry translation (DBT) and uses hardware memory man- agement unit in the CPU to translate memory addresses, instead of software translation in a current DBT emulator like QEMU. We also use the larger address space on modern 64-bit CPUs to accommodate our DBT emulator so that it will not interfere with the guest operating system.
