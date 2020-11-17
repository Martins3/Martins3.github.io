# 10 ADVANCED PROGRAMMABLE INTERRUPT CONTROLLER (APIC)
- [ ] When a local APIC has sent an interrupt to its processor core for handling, the processor uses the interrupt and 
exception handling mechanism described in Chapter 6, “Interrupt and Exception Handling.” See Section 6.1, “Interrupt and Exception Overview,” for an introduction to interrupt and exception handling.

Local APICs can receive interrupts from the following sources:
1. ...
2. ...

- [ ]the local APIC delivers the interrupt to the processor core using an interrupt delivery protocol that has been set up through a group of APIC registers called the local vector table or LVT

- [ ] A processor can generate IPIs by programming the interrupt command register (ICR) in its local APIC (see Section 10.6.1, “Interrupt Command Register (ICR)”). 


- [ ] Descriptions of how to program the local APIC are given in Section 10.5.1, “Local Vector Table,” and Section 10.6.1, “Interrupt Command Register (ICR).”


- [ ] Figure 10-4. Local APIC Structure

#### 10.12 EXTENDED XAPIC (X2APIC)


## questions
flat and cluster 

## 命名规则
1. x86 echnically x86 simply refers to a family of processors and the instruction set they all use. It doesn't actually say anything specific about data sizes. 
2. IA-32 (short for "**Intel Architecture, 32-bit**", sometijmes also called **i386**)
3. IA-64 (also called Intel Itanium architecture) ，安腾架构，已经死了
4. x86-64 (also known as **x64**, **x86_64**, **AMD64** and **Intel 64**)

实际上，软件命名规则是x86 指32位版本，x64指64位版本，比如这一个
[例子](https://www.oracle.com/technetwork/java/javase/downloads/jdk8-downloads-2133151.html)

偶尔会出现amd64, 就是x86-64，之所以含有这一个名字是因为64位是AMD最早提出的。
