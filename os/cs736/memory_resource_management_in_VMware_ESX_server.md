# Memory Resource Management in VMware ESX Server

1. A ballooning technique
reclaims the pages considered least valuable by the operating system running in a virtual machine.
2. An idle memory tax
achieves efficient memory utilization while maintaining performance isolation guarantees. 
3. Content-based page sharing
and hot I/O page remapping exploit transparent page remapping to eliminate redundancy and reduce copying overheads.

These techniques are combined to efficiently support virtual
machine workloads that overcommit memory.

## 1 Introduction
The design of ESX Server differs significantly from VMware Workstation, which uses
a hosted virtual machine architecture [23] that takes advantage of a pre-existing operating system for portable
I/O device support.
> 所以，这是什么 architecture ?

1. Section 2 describes low-level memory virtualization.
2. Section 3 discusses mechanisms for reclaiming memory to support dynamic resizing of virtual machines. 
3. A general technique for conserving memory by sharing identical pages between VMs is presented in Section 4.
4. Section 5 discusses the integration of working-set estimates into a proportional-share allocation algorithm.
5. Section 6 describes the high-level allocation policy that coordinates these techniques. 
6. Section 7 presents a remapping optimization that reduces I/O copying overheads in large-memory systems.
7. Section 8 examines related work. Finally, we summarize our conclusions and highlight opportunities for future work in Section 9.

## 2 Memory Virtualization
> machine address
> virtual to mechine directly !

## 3 Reclamation Mechanisms
ESX Server supports overcommitment of memory to facilitate a higher degree of server consolidation than would be possible with simple static partitioning.
> server consolidation

#### 3.1 Page Replacement Issues
> 以前的方法，采用 swap 机制实现 overcommitment

> an extra level of paging requires a meta-level page replacement policy

> **double paging** problem

#### 3.2 Ballooning
Ideally, a VM from which memory has been reclaimed should perform as if it had been configured with less memory.

> 试验内容想要说明什么东西 ?

#### 3.3 Demand Paging
When ballooning is not possible or insufficient, the system falls back to a paging mechanism.
Memory is reclaimed by paging out to an ESX Server swap area on disk, without any guest involvement

The ESX Server swap daemon receives information
about target swap levels for each VM from a higher level policy module.

A randomized page replacement policy is used to prevent the types of pathological interference with native
guest OS memory management algorithms described in
Section 3.1.

## 4 Sharing Memory

#### 4.1 Transparent Page Sharing


#### 4.2 Content-Based Page Sharing
As an optimization, an unshared page is not marked COW, but
instead tagged as a special hint entry.

#### 4.3 Implementati
> skipped

## 5 Shares vs. Working Sets
ESX Server employs a new allocation algorithm that
is able to achieve efficient memory utilization while
maintaining memory performance isolation guarantees.

#### 5.1 Share-Based Allocation
In proportional-share frameworks, resource rights are
encapsulated by shares, which are owned by clients that
consume resources.

Previous attempts to cross-apply
techniques from proportional-share CPU resource management to compensate for idleness have not been successful

ESX Server resolves this problem by introducing an
idle memory tax. The basic idea is to charge a client
more for an idle page than for one it is actively using.

> skip some part

#### 5.3 Measuring Idle Memory
For the idle memory tax to be effective, the server
needs an efficient mechanism to estimate the fraction of
memory in active use by each virtual machine. However, specific active and idle pages need not be identified
individually.

## 6 Allocation Policies
> skip another many parts, something intersting and fun to handle.


