---
title: "Meltdown and Spectre"
date: 2019-11-03T15:19:47+08:00
draft: false
categories: [system]
---

## Meltdown: Reading Kernel Memory from User Space 总结
操作系统的安全基础之一: memory isolation 由于 Meltdown 而失效，

Meltdown 的原理 : 乱序执行会影响 cache , 并且可以通过 cache side channel 和 microarchitectural convert channel 将数据导出

操作系统需要通过 KAISER 来防范。

Contributions. The contributions of this work are:
1. We describe out-of-order execution as a new, extremely powerful, software-based side channel.
2. We show how out-of-order execution can be combined with a microarchitectural covert channel to
transfer the data from an elusive state to a receiver
on the outside.
3. We present an end-to-end attack combining out-oforder execution with exception handlers or TSX, to
read arbitrary physical memory without any permissions or privileges, on laptops, desktop machines,
mobile phones and on public cloud machines.
4. We evaluate the performance of Meltdown and the
effects of KAISER on it.

Additionally, other optimizations like move elimination or the recognition of zeroing idioms are directly handled by the reorder buffer.


However, side-channel attacks allow to
detect the exact location of kernel data structures [21, 29, 37] or derandomize ASLR in JavaScript [16].
A combination of a software bug and the knowledge of these addresses can lead to privileged code execution.
> 就算是确定了这些代码的执行位置，也应该是没有权限执行的啊!

Flush+Reload attacks work on a single cache line granularity.
利用 clflush 将 targeted memory location ，然后测量时间，看别的进程是否是否加载该位置的


> Invalidates from every level of the cache hierarchy in the cache coherence domain the cache line that contains the **linear address** specified with the memory operand. 
> If that cache line contains modified data at any level of the cache hierarchy, that data is written back to memory. The source operand is a byte memory location.

A special use case of a **side-channel** attack is a **covert
channel**. Here the attacker controls both, the part that **induces the side effect**, and the part that **measures the side
effect**. This can be used to leak information from one
security domain to another, while bypassing any boundaries existing on the architectural level or above.
> 什么 side 啊 ? Other side channels can also detect whether a specific memory location is cached, including Prime+Probe [55, 48, 52], Evict+Reload [47], or Flush+ Flush [22]. As Flush+Reload is the most accurate known cache side channel and is simple to implement, we do not consider any other side channel for this example.
> 应该就是这几种方法吧 !

If the attacker targets a secret at a userinaccessible address, the attacker has to cope with this
exception. We propose two approaches:
With **exception handling**, we catch the exception effectively occurring after executing the transient instruction sequence,
and with **exception suppression**, we prevent the exception from occurring at all and instead redirect the control
flow after executing the transient instruction sequence.
1. 基于 exception handling : fork ，让 child 去死，parent 来收集信息
2. .... 



## Spectre Attacks: Exploiting Speculative Execution
Note that Kocher et al. [40] pursue an orthogonal approach, called
Spectre Attacks, which trick speculatively executed instructions into leaking information that the victim process is authorized to access. As a result, Spectre Attacks
lack the privilege escalation aspect of Meltdown and require tailoring to the victim process’s software environment, but apply more broadly to CPUs that support speculative execution and are not prevented by KAISER.

## Linux Kernel 的处理
> 找到类似的 lwn 的文章
