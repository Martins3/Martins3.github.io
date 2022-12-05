# Volume 1 : Basic Architecture

# Volume 2 : Instruction Set Reference

# Volume 3 : System Programming Guide

## 10 ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (APIC)
- [ ] When a local APIC has sent an interrupt to its processor core for handling, the processor uses the interrupt and
exception handling mechanism described in Chapter 6, “Interrupt and Exception Handling.” See Section 6.1, “Interrupt and Exception Overview,” for an introduction to interrupt and exception handling.

Local APICs can receive interrupts from the following sources: 1. ...
2. ...

- [ ]the local APIC delivers the interrupt to the processor core using an interrupt delivery protocol that has been set up through a group of APIC registers called the local vector table or LVT

- [ ] A processor can generate IPIs by programming the interrupt command register (ICR) in its local APIC (see Section 10.6.1, “Interrupt Command Register (ICR)”).


- [ ] Descriptions of how to program the local APIC are given in Section 10.5.1, “Local Vector Table,” and Section 10.6.1, “Interrupt Command Register (ICR).”


- [ ] Figure 10-4. Local APIC Structure

### 10.12 EXTENDED XAPIC (X2APIC)

## VMX NON-ROOT OPERATION
分析了导致 vmexit 的指令和其他的原因，有些执行的行为在 non-root 中是存在变化的，最后，25.5 不知道在干什么?

#### 25.1.2 Instructions That Cause VM Exits Unconditionally
- The following instructions cause VM exits when they are executed in VMX non-root operation: CPUID, GETSEC, INVD, and XSETBV.
- This is also true of instructions introduced with VMX, which include: INVEPT, INVVPID, VMCALL, VMCLEAR, VMLAUNCH, VMPTRLD, VMPTRST, VMRESUME, VMXOFF, and VMXON.

- [ ] invd : invalid cache, but why invalid tlb isn't unconditionally

#### 25.1.3 Instructions That Cause VM Exits Conditionally
- RDTSC. The RDTSC instruction causes a VM exit if the “RDTSC exiting” VM-execution control is 1.

非常神奇，RDTSC 是否导致 vmexit 是可选的。

## 29 APIC VIRTUALIZATION AND VIRTUAL INTERRUPTS
When these controls are used, the processor will emulate many accesses to the APIC, track the state of the virtual
APIC, and deliver virtual interrupts — all in VMX non-root operation **without** a VM exit.

The processor tracks the state of the virtual APIC using a **virtual-APIC page** identified by the virtual-machine monitor (VMM).

The following are the VM-execution controls relevant to APIC virtualization and virtual interrupts

- **Virtual-interrupt delivery**. This controls enables the evaluation and delivery of pending virtual interrupts
(Section 29.2). It also enables the emulation of writes (memory-mapped or MSR-based, as enabled) to the
APIC registers that control interrupt prioritization.
- **Use TPR shadow**. This control enables emulation of accesses to the APIC’s task-priority register (TPR) via CR8
(Section 29.3) and, if enabled, via the memory-mapped or MSR-based interfaces.
- **Virtualize APIC accesses**. This control enables virtualization of memory-mapped accesses to the APIC
(Section 29.4) by causing VM exits on accesses to a VMM-specified APIC-access page. Some of the other
controls, if set, may cause some of these accesses to be emulated rather than causing VM exits.
- **Virtualize x2APIC mode**. This control enables virtualization of MSR-based accesses to the APIC (Section
29.5).
- **APIC-register virtualization**. This control allows memory-mapped and MSR-based reads of most APIC
registers (as enabled) by satisfying them from the virtual-APIC page. It directs memory-mapped writes to the
APIC-access page to the virtual-APIC page, following them by VM exits for VMM emulation.
- **Process posted interrupts**. This control allows software to post virtual interrupts in a data structure and send
a notification to another logical processor; upon receipt of the notification, the target processor will process the
posted interrupts by copying them into the virtual-APIC page (Section 29.6)

- [ ] TPR : task priority, and interrupt priority
- [ ] Upon receipt of the notification, the target processor will process the posted interrupts by copying them into the virtual-APIC page
- [ ] APIC-access page : It directs memory-mapped writes to the APIC-access page to the virtual-APIC page.


### 29.1 VIRTUAL APIC STATE
Depending on the settings of certain VM-execution controls, the processor may virtualize certain fields on the virtual-APIC page with functionality analogous to that performed by the local APIC.

#### 29.1.1 Virtualized APIC Registers
Depending on the setting of certain VM-execution controls, a logical processor may virtualize certain accesses to
APIC registers using the following fields on the virtual-APIC page:
1. Virtual task-priority register (VTPR)
2. Virtual processor-priority register (VPPR)
3. Virtual end-of-interrupt register (VEOI)
4. Virtual interrupt-service register (VISR)
5. Virtual interrupt-request register (VIRR)
6. Virtual interrupt-command register (VICR_LO)
7. Virtual interrupt-command register (VICR_HI)

- [ ] All this kind of register.
### 29.2 EVALUATION AND DELIVERY OF VIRTUAL INTERRUPTS
If the “virtual-interrupt delivery” VM-execution control is 1, certain actions cause a logical processor to evaluate pending virtual interrupts.
**The following actions cause the evaluation of pending virtual interrupts: VM entry; TPR virtualization; EOI virtualization; self-IPI virtualization; and posted-interrupt processing.**
No other operations do so, even if they modify RVI or VPPR.

- [x] Amazing, not just VM entry cause the evaluation
- [ ] what's TPR virtualization, EOI virtualization, self-IPI virtualization, and posted-interrupt processing

### 29.2.2 Virtual-Interrupt Delivery
If a virtual interrupt has been recognized (see Section 29.2.1), it is delivered at an instruction boundary when the
following conditions all hold:
(1) RFLAGS.IF = 1;
(2) there is no blocking by STI;
*(3) there is no blocking by MOV SS or by POP SS; and*
*(4) the “interrupt-window exiting” VM-execution control is 0.*

The following pseudocode details the behavior of virtual interrupt delivery (see Section 29.1.1 for definition of VISR, VIRR, and VPPR):
```
Vector ← RVI;
VISR[Vector] ← 1;
SVI ← Vector;
VPPR ← Vector & F0H;
VIRR[Vector] ← 0;
IF any bits set in VIRR
THEN RVI ← highest index of bit set in VIRR
ELSE RVI ← 0;
FI;
deliver interrupt with Vector through IDT;
cease recognition of any pending virtual interrupt;
```

### 29.6 POSTED-INTERRUPT PROCESSING
Posted-interrupt processing is a feature by which a processor processes the virtual interrupts by recording them as pending on the virtual-APIC page.

The processing is performed in response to the arrival of an interrupt with the posted-interrupt notification vector.
In response to such an interrupt, the processor processes virtual interrupts recorded in a data structure called a
posted-interrupt descriptor. The **posted-interrupt notification vector** and the address of the **posted-interrupt descriptor** are fields in the VMCS;

Use of the posted-interrupt descriptor **differs from** that of other data structures that are referenced by pointers in
a VMCS. There is a general requirement that software ensure that each such data structure **is modified only when
no logical processor with a current VMCS that references it is in VMX non-root operation.** That requirement does
not apply to the posted-interrupt descriptor. There is a requirement, however, that such modifications be done
using locked read-modify-write instructions.

## CHAPTER 34 SYSTEM MANAGEMENT MODE
SMM is a special-purpose operating mode provided for handling system-wide functions like power management, system hardware control, or proprietary OEM-designed code.
It is intended for use only by system firmware, not by applications software or general-purpose systems software.
The main benefit of SMM is that it offers a distinct and easily isolated processor environment that operates transparently to the operating system or executive and software applications.

The execution environment after entering SMM is in real address mode with paging disabled (CR0.PE = CR0.PG = 0). In this initial execution environment, the SMI handler
can address up to 4 GBytes of memory and can execute all I/O and system instructions.
