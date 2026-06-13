The Embedded Shadow Page Table (ESPT) approach
has been proposed to effectively decrease this address translation cost. ESPT directly maps GVA to HPA, thus avoid the
lengthy guest virtual to guest physical, guest physical to host
virtual, and host virtual to host physical address translation.

> original work use LKM, not a problem for us ?
> what's LKM

Instead of relying on using LKMs,
our approach *adopts a shared memory mapping scheme* to
maintain the shadow page table (SPT) using only “mmap”
system call.

Furthermore, this work studies the support of
SPT for *multi-processing* in greater details. It devices *three*
different SPT organizations and evaluates their strength and
weakness with standard and real Android applications on the
system virtual machine which emulates the Android/ARM platform on x86-64 systems.

*Dynamic Binary Translation (DBT) is often used to speed up emulation*

Hardware-assisted memory virtualizations, such as *Intel
Extended Page Tables* [19] and *AMD Nested Paging* [9], are
effective ways to reduce this overhead for same-ISA virtual machines.

We divided the operations on SPT into three types: creating
SPT, synchronizing with guest page table (GPT) and switching SPT for different processes. 

Proposed and evaluated *three* SPT organizations, including Shared, Private and Group Shared SPT to handle
multi-processing in guest OSes. A guideline on how to
select each variation is provided.


**A virtual machine usually allocates a large
chunk of virtual space to simulate the guest physical memory, we call this space “Simulated Guest Physical Space”
(SGPS) (as indicated in the figure).**
> If this is implementation method, maybe transparent huge page is useful for it ?

*When running the guest code, the page table base pointer, such as the CR3 register
in x86, will be changed to the SPT and then it can directly
use hardware MMU to accomplish the address translation.*
> so no more other hardware is needed ?
> what's the interface of mmu/tlb ?

**QEMU [5] uses software MMU to
translate a guest virtual address (GVA) to the host virtual
address (HVA), and let the host machine to handle the HVA
translation afterwards**

Each memory access instruction of the guest is translated into several
host instructions by the internal dynamic binary translator
[5] of QEMU.

A virtual machine usually allocates a large
chunk of virtual space to simulate the guest physical memory, we call this space “Simulated Guest Physical Space”
(SGPS) (as indicated in the figure). 

**For same-ISA system virtual machines, a Shadow Page
Table (SPT) [10] is often created, which maps the guest VA
to the machine physical address.**
> Maybe this is definition of shadow page
> for same ISA, shadow page can record all the HVA to GPA, and mmu is the cache of it

For cross-ISA system virtual machines, such memory translation process is simulated in software, so it is often called “software MMU”. 
QEMU [5] uses software MMU to translate a guest virtual address (GVA) to the host virtual address (HVA), and let the host machine to handle the HVA translation afterwards. 
> This is the second method, really stupid
> software for host translation and hardware for host mapping

For example a guest register is emulated as a host memory location, so when running the guest code,
**it needs to access both the guest addresses and the host addresses in the same translated code block,
each requires a different page table. So frequently switching back and forth between SPT and HPT is needed.**
> switching page table is not desirable
> so ? ESPT

*The consistency between GPT and SPT is also maintained by using LKMs.*
> so what is the consistency problem ?

ESPT first sets all the 4G memory space dedicated for the shadow page entries as protected, when
certain pages are accessed, SIGSEGV will be triggered to invoke a registered signal handler.
In the signal handler, ESPT will use the fault GVA to go through the aforementioned
three-level of address translation to find HPA and then create the mapping from GVA to HPA into the SPT. Finally, it
resumes from the fault instruction. *ESPT maintains a SPT for each guest process*, when the guest process switches, ESPT will use LKMs to set the host directory page table base
pointer for the lower 4G space to the targeted SPT. For example, Figure 1(c) shows that “Embedded Shadow Page Entry”
points to a SPT, when the guest process switches, ESPT will
set “Embedded Shadow Page Entry” to point to the guest’s
new SPT.
> 1. reserved 4G page is virtual or physical ? (virtual)
> 2. for every process, we need a differenet virtual address region for it, what if process need more than 4G, what if there are so many process ?
> 3. so can we enable protection in host, now that all the process share the same addresss space ?

Taking ARM-32 as guest and x86-64 as host, as shown in Figure 2(b),
we can see that ESPT will translate one guest access memory instruction to a ‘mov’, 
a ‘jmp’ and several Software MMU instructions.
There are two types of page faults in this approach: 
one is shadow page fault and another is guest page fault. 
1. Shadow page fault occurs when the requested GVA is not in the SPT. It is handled by LKMs. 
2. A guest page fault is handled by Software MMU. 
Those instructions work as follow: the ‘mov’ instruction will first try to access the GVA by using SPT. 
If hit in SPT, the obtained HVA will go through the hardware MMU to execute and then jump to execute the next guest instruction. 
If miss in SPT, LKMs will be invoked to fill the SPT and resume execution, and if guest page fault occurs, ESPT will replace the ‘jmp’ with ‘nop’ and execute Software MMU instructions to fill the GPT.
> why do ESPT and HSPT share Figure(2) ? anything in common ?
> so why author created HSPT ?
> Software MMU : translation is handled in software : from GVA to HVA
> Software MMU instruction : address translation is alway handled by hardware, but tlb refilled is handle by software

It is non-trivial to avoid using LKMs for two reasons: 
1. Our new approach must be able to create and maintain SPT in kernel space without using LKMs; 
2. *Multi-processing* emulation with the same address space must be supported by our method. 
> why multi processing is critical, difficult, tricky, what should be done to coporate it ?


Similar to ESPT, our HSPT focuses on the widespread scenario where the virtual space of the host is larger than the guest, such as ARM 32bit to x86 64bit.

> 是不是说，如果想要使用 mmu 进行加速，那么就需要必须在相同的地址空间中间 ?
> 应该是的，不然同一个虚拟地址会指向不同的物理地址
> so how software mmu worked ? no tlb, so there is no need for different addresss space

We also believe our approach could be applied to the scenario where the guest is an ARM 64bit architecture. 
The user address space of x86-64 is 256TB (48-bit). Although ARM 64bit also supports 64-bit virtual address, the current AArch64 OS only uses 39-bit of virtual address (512GB), 
so there are room left for embedding SPTs.

> it seems : 将 host 的地址空间放到host 的虚拟地址空间中间，如此，guest 的虚拟地址转换就不需要进行操作了，所以 shadow page table 还有意义吗 ?
> How many level is SPT ?
> be aware, we dynamic change what instruction to emit !

So far, we have introduced the framework of HSPT, but there are still several issues to address:
1. We had described the memory accesses to guest virtual page P1 can be transformed to accessing the host virtual page G1. Section 3.1 will discuss how to create the mapping from G1 to P4.
2. SPT keeps the direct maps from guest virtual address to host physical address, so it must keep pace with the guest page table. Section 3.2 will discuss a mechanism to maintain the consistency between SPT and GPT.
3. When a guest process switch happens, both the tradition- al SPT and ESPT must also change the SPT. Section 3.3 will discuss three new SPT variations in our HSPT to sup- port the guest multi-process.

**As shown in Figure 3, creating shadow page mapping means creating the mapping from G1 to P4 into the SPT.**
After basic translation process, we know that P3 is mapped to P4. 
**What we need to do is to make G1 to *share* P4 mapped from P3.**
> 所以，其实 shadow page table 就是放到 host page table 中间的，而且就是 host va 到 pa 的映射，必须说，很好!
>
> 注意，对于一个操作系统而言，总是会维护自己的 page table 的啊
>
> host 的 page walk 一定需要使用软件的方式
> 如果 guest 和 host virtual memory 的直接映射，可以让qumu翻译的代码多 emit 出来的代码很少，但是，操作系统还是会维护自己的 page table
> 
> 思考一下，如下操作应该如何完成:
> 1. cow : fork ?
> 2. 一个进程修改 region 的指向，导致映射到另一个进程的相同的物理页面上
>
> 因为 host virtual memory 和 host virtual memory 总是线性对应的关系，那么当 guest 的映射发生修改之后，真正需要进行修改的是，host 在哪个区间的映射
> 1. 操作系统提供了修改虚拟地址映射的接口吗 ?
> 2. qemu 怎么检查 guest 系统正在发生 page table 的修改，从上下文中间根本无法判断啊 ?
>     1. 如果，qemu 和 内核是"融合"在一起的，这种自然映射自然是简单的
> 
> 似乎想法是 :
> 1. guest 的物理地址被映射到一个文件，这个文件的具体位置 根据 MMT 规定，SGPS (Simulated Guest Physical Address)
> 2. 同时，保证两个虚拟地址总是 offset 对应的，这就是 GDVAS
> 3. 

In the virtual machine initialization phase, the virtual machine allocates a host virtual space which is the so-called “SGPS” space (as indicated in the figure) and used as the guest physical space. 
When doing this, we use the “mmap” system call with ‘MAP SHARED’ flag to map this space to a file. 
**Then when the virtual machine starts to run and needs to create the SPT entry for G1, 
what we need to do is to map G1 with proper protection to the same host physical page P4.** 
This is done by using the “mmap” system call and map G1 to the same offset F1 of the target file with P3. After this, the host OS will automatically map G1 and P3 to the same host physical page P4 with isolated page protection.
> 当需要创建 G1 的 SPT 的时候，将 G1 map 到 P4，那么此时（似乎没有 shadow page 的使用必要啊)，
> SPT 是不是包含了所有从 guest va 到 hsot pa 的位置

shadow table 的更新策略:
1. 创建process 的时候，lazy + signal
2. 


1. 难道，SIGSEGV 是什么，难道不应该是 page fault 吗 ? 为 GDVAS 这个区域而设计的
2. inconsistency 如何检查出来 ?

If it is caused by the guest page fault, the handler will jump to execute the guest exception related handler. 
Otherwise, this exception is caused by the inconsistency of SPT and **we should use the synchronization method mentioned** in Section 3.1 to map G1 (Step 4) 
and the host OS will update the SPT entry automatically (Step 5).
> 此时的信号捕获，同时是 guest 的 page fault，也可能是 lazy，也可能是 inconsistency
> 
> 1. 其实，我非常担心，这里要截获 guest 的 page fault，同时插入 自己的 segv ，那么怎么可能仅仅使用 mmap 就可以实现这里的代码啊

When we intercept these instructions, we do not modify the shadow page entry to the newest mapping, instead, we just clear the outdated mapping in SPT by using
“mmap” system call with ‘PROT NONE’ flag. When such pages are accessed, a SIGSEGV signal will be raised and we could synchronize at that time.
> 1. 操作系统更新 page table 的时候，居然会进行 tlb invalid，只能说，很神奇呀
> 2. mmap 的功能很强啊 !
>
> 但是，是不是操作系统一定可以保证，必然存在TLB更新当更新 page table 的? 
> 

It works as follows: When the emulator detects a guest process is switching out (*this can be detected by monitoring the page table pointer*), 
we should clear the SPT by using “mmap” system call with ‘PROT NONE’ flag to get them ready for the next switched in process. 
> 1. 通过检查 page table 地址改动的方法检测，不太聪明的样子啊
> 2. 岂不是，每次换回来，全部GG

Because the page table entries could be changed during the time of switched out,
we only record which entries were accessed but not with full detailed information. 
Moreover, the number of filled entries will accumulate as the process keeps executing and only a small part of the entries filled may be accessed in this timeslot,
so if we prefill all the past entries at the resumption of each process,
the overhead incurred may exceed the benefit. 
Based on this observation, we set a window for each guest process. Each time a new SPT entry is synchronized, 
we’ll record the index of this entry in the window. When the window is full, it is flushed to restart recording. 
After done this, when a certain process is switched in, we’ll first synchronize all the entries recorded in this processs window. The impact of the window size to performance will be evaluated in the experiment section.
> prefill 可以批量化操作吗 ?
> 
>
> mmap 可以通过让当前的地址指向已经映射的地址，从而实现规定映射位置，那么岂不是要创建出来大量的 vma 出来
>

*Setting write-protection to the switched-out GPTs is a common but expensive method to monitor the modification of GPT [24].*
To reduce this overhead, we again intercept the TLB-invalidation instructions to identify modified page entries. 
Consider x86 and ARM, as an example, they use PCID (Process Context Identifier) [4] and ASID (Address Space Identifier) [1, 20] respectively to identify TLB entries for each process. 
We call this kind of identifier as “Context Identifier” (CID). Same virtual address of different processes can be distinguished in the TLB due to CID. 
*Based on this, when the process switching happens, there is no need to flush the whole TLB.*
> 1. 不是，已经说过，使用 TLB 作为检测标准，为什么又要讨论使用 write-protection 来
> 2. 因为 guest 在同一个地址空间，所以，没有必要进行 tlb 的切换啊

- Principles of TLB structure with CID Under this structure
OS must obey the following principles: 
1. At the time of process switching, **CID register** which contains the CID of the current active process should be modified. 
2. When the OS modifies the page table, TLB must be informed with the CID and the address. 
3. There are a *limited* number of available CIDs.

Management of Private SPTs Based on the principles above, we can tell which entry of a process, and the process CID is modified from the TLB-invalidation instructions. 
Therefore, when setting up each private SPT,
*we choose to bind each SPT with a CID (as shown in Figure 6) rather than the process ID so that we can use the TLB-invalidation instructions to maintain consistency between SPT and the corresponding guest page table.*
> 有讲过 CID 和 process ID 有什么区别啊 ？

- Switching SPT
> 等等，Figure 2(c) 的问题是什么 ？


## 问题
1. 但是，对于 espt 的提升是什么 ？
2. 有没有代码跑一跑啊 ?
