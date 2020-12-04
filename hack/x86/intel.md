# Intel

<!-- vim-markdown-toc GitLab -->

- [Volume 1 : Basic Architecture](#volume-1-basic-architecture)
- [Volume 2 : Instruction Set Reference](#volume-2-instruction-set-reference)
    - [1.2 OVERVIEW OF VOLUME 2A, 2B, 2C AND 2D: INSTRUCTION SET REFERENCE](#12-overview-of-volume-2a-2b-2c-and-2d-instruction-set-reference)
- [Volume 3 : System Programming Guide](#volume-3-system-programming-guide)
  - [10 ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (APIC)](#10-advanced-programmable-interrupt-controller-apic)
    - [10.12 EXTENDED XAPIC (X2APIC)](#1012-extended-xapic-x2apic)
      - [25.1.2 Instructions That Cause VM Exits Unconditionally](#2512-instructions-that-cause-vm-exits-unconditionally)
- [questions](#questions)

<!-- vim-markdown-toc -->
# Volume 1 : Basic Architecture

# Volume 2 : Instruction Set Reference
### 1.2 OVERVIEW OF VOLUME 2A, 2B, 2C AND 2D: INSTRUCTION SET REFERENCE

# Volume 3 : System Programming Guide

## 10 ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (APIC)
- [ ] When a local APIC has sent an interrupt to its processor core for handling, the processor uses the interrupt and 
exception handling mechanism described in Chapter 6, “Interrupt and Exception Handling.” See Section 6.1, “Interrupt and Exception Overview,” for an introduction to interrupt and exception handling.

Local APICs can receive interrupts from the following sources:
1. ...
2. ...

- [ ]the local APIC delivers the interrupt to the processor core using an interrupt delivery protocol that has been set up through a group of APIC registers called the local vector table or LVT

- [ ] A processor can generate IPIs by programming the interrupt command register (ICR) in its local APIC (see Section 10.6.1, “Interrupt Command Register (ICR)”). 


- [ ] Descriptions of how to program the local APIC are given in Section 10.5.1, “Local Vector Table,” and Section 10.6.1, “Interrupt Command Register (ICR).”


- [ ] Figure 10-4. Local APIC Structure

### 10.12 EXTENDED XAPIC (X2APIC)

#### 25.1.2 Instructions That Cause VM Exits Unconditionally
- The following instructions cause VM exits when they are executed in VMX non-root operation: CPUID, GETSEC, INVD, and XSETBV.
- This is also true of instructions introduced with VMX, which include: INVEPT, INVVPID, VMCALL, VMCLEAR, VMLAUNCH, VMPTRLD, VMPTRST, VMRESUME, VMXOFF, and VMXON.

- [ ] invd : invalid cache, but why invalid tlb isn't unconditionally 

# questions
flat and cluster 
> huxueshi : my comments for the question, please provide enough background about the question, make it clear for latter reference.
