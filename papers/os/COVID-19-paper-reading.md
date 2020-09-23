# 由于 COVID-19 疫情在家阅读的文章
https://github.com/TheShadow29/research-advice-list

https://github.com/shining1984/PL-Compiler-Resource

## 来源
1. https://github.com/papers-we-love/papers-we-love
2. zhang : 9 (where is web ?)
3. linux kernel architecture : 2
    1. https://www.cs.kent.ac.uk/people/staff/srk21//research/papers/kell13operating.pdf

4. minix
    1. http://sigops.org/s/conferences/sosp/2013/papers/p133-elphinstone.pdf
    2. http://www.minix3.org/ => https://www.cs.vu.nl/~ast/reliable-os/ => https://www.cs.vu.nl/~ast/Publications/Papers/computer-2006a.pdf 争论2

http://pages.cs.wisc.edu/~bart/736/f2019/reading_list.html

# 官方要求
课程编号：081201M05008H-3    课时：30    学分：1.00    课程属性：专业普及课    主讲教师：李国荣
课程名称：文献阅读-计算机系统结构专业 3班（计算机系统结构专业同学 选修）19-20春季

教学目的、要求
科技文献是科研人员交流学术思想和创新成果的重要途径之一。作为科学研究的必要环节，科技文献阅读是研究生独立开展科学研究应具备的基本技能。本课程是一门培养学生掌握科技文献检索、分析理解、归纳总结等科学方法的课程，通过该门课程的学习，学生可以了解科技文献检索、选择、整理的基本原理和方法技巧，掌握科技论文的基本框架、结构特点、写作规范，锻炼科学思维的能力，拓展专业知识领域，激发学生的科研热情，培养学生的创新能力，为学生在将来的科研工作中精准高效的了解和把握专业领域前沿和最新成果打下坚实的基础。
预修课程
无
教材
无
主要内容
教学内容
1.2020年2月13日前，根据选课学生的专业领域分班，并指定授课教师。
2.2020年2月26日前，导师提供文献检索范围由学生检索收集阅读文献或指定15~20篇文献，学生根据导师的具体要求以及导师提供的清单确定阅读文献清单（参见附件1）。
3.2020年3月26日前，学生向导师和授课教师分别提交文献阅读书面报告（参见附件2），导师对文献阅读报告进行评价。
4.2020年4月9日前，学生将导师给出的评语及考核成绩反馈给授课教师（参见附件3）。
5.2019-2020年春季学期结束前1周，授课教师将根据选课人数组织课堂分组答辩，给出课堂答辩的成绩，并综合书面报告成绩和课堂答辩成绩出文献阅读课程的最终成绩。

教学手段与方法：
6.导师指定文献检索范围或文献资料，并指导学生开展文献检索及文献阅读，授课教师协助未指定导师的学生确定文献阅读资料。
7.在开课期间授课教师负责检查学生的文献阅读进度安排，并为学生提供必要的文献阅读指导建议。
授课教师组织学生开展课堂PPT报告答辩。
参考用书
导师提供
教师简介
略


## F2FS: A New File System for Flash Storage
The file system builds on
**append-only logging** and its key design decisions were
made with the characteristics of flash storage in mind.

Despite its broad use,
flash memory has several limitations, like erase-beforewrite requirement, the need to write on erased blocks sequentially and limited write cycles per erase block.

Unless handled carefully, frequent random
writes and flush operations in modern workloads can seriously increase a flash device’s I/O latency and reduce
the device lifetime.

Listed in the following are the main considerations for the design of F2FS:
> really cool, but too concise


## The Design and Implementation of a Log-Structured File System
> http://www.cs.cornell.edu/courses/cs4410/2015su/lectures/lec20-lfs.html
> 这个笔记总结的到位


## Journaling the Linux ext2fs Filesystem
> 首先理解一下 file system consistency 吧!

> 1. ext3 在 ext2 的基础上增加似乎就是 log
> 2. ext2 增加 log 的核心作用是不是只是因为用于实现 fs consistency ? 存在其他的好处吗 ?
> 3. 类似数据库的操作，文件系统也是需要使用将操作原子化的，实现的方法是 commit 


Log-Structured Filesystems achieve the same end
by writing all filesystem data−**both file contents and
metadata**−to the disk in a continuous stream (the ‘‘log’’).


A central concept when considering a journaled filesystem is the transaction, corresponding to a single
update of the filesystem. 

The decision about when to commit the current
compound transaction and start a new one is a
policy decision which should be under user control,
since it involves a trade-off which affects system
performance.

We use one of these reserved inodes to store the
filesystem journal, and in all other respects the filesystem will be compatible with existing Linux kernels.
> inode 从哪里来的，以及为什么需要使用 inode 存储 journal ?

A journal metadata block contains the entire contents of a single block of filesystem metadata as updated by a transaction. This means that however
small a change we make to a filesystem metadata
block, we have to write an entire journal block out
to log the change. However, this turns out to be relatively cheap for two reasons:

To completely commit and finish checkpointing a
transaction, we go through the following stages:
1. Close the transaction. **At this point we make a
new transaction in which we will record any
filesystem operations which begin in the future. Any existing, incomplete operations will
still use the existing transaction: we cannot
split a single filesystem operation over multiple transactions!**
> 似乎 transaction 的定义不是 : 一个文件操作，而是一个时间段。
2. Start flushing the transaction to disk. In the
context of a separate log-writer kernel thread,
we begin writing out to the journal all metadata buffers which have been modified by the
transaction. *We also have to write out any dependent data at this stage (see the section
above, Anatomy of a transaction).*
> depenent data 为什么依赖这个就可以 ?


Collisions between transactions : 
> 只有 write 冲突会造成，但是不理解啊 !


## A fork() in the road
> 1. 其参考文献非常的有意思啊，似乎有人试图
> 2. 请回答一下问题 : 作者到底列举出来了多少个 fork 的问题 ?

> 3. 所以，到底如何才可以消除 fork 啊，消除 fork 的难度在于
> 4. 虽然说 clone 和 fork 在内核中间的实现最终的到达相同的位置，但是在 syscall 层次，两者的区别在于何处 ?

> 消除 fork 的关键在于，到底能不能不要使用 hierarchy 的机制，

> 作者似乎连续的说明了，各种替代方法，甚至连 vfork 都说过一遍，为什么需要举出 vfork 的例子 ?

> 使用 K42 的例子，只是想要说明 K42 使用 fork 的错误经验。


> 第 4 和 5 : 都是说明 fork 的问题!
> : 4 说明的是，当时 fork 设计很好，但是随着硬件和操作系统的发展，曾经的优点已经消失了。
> : 5 应该说明无法应对新的要求，


> 第 6 和 7 小节的关系是什么 : 为什么都是阐述如何实现去掉 fork.
> 核心在 6 小节：high level 使用 spawn 替代，low-level 使用clean-slate designs 的原因，感觉就是


Rather, it was
the choice to allow any process at any point to be forkable,
and to do so efficiently in the quest of competitive performance, that proved our undoing—the attendant complexity may very well have been key to us abandoning all but our
support for Unix and our native personalities.
> the attendant complexity : 随之而来的复杂性


In Unix today, the only fallback for advanced use-cases remains code executed after fork, but clean-slate designs have demonstrated an alternative model where system calls that modify per-process state are not constrained to merely the current process, but rather can manipulate any process to which the caller has access.
> 在当今的Unix中，高层用例的唯一回退仍然是fork之后执行的代码，但是clean slate设计展示了一个替代模型，其中修改每个进程状态的系统调用不限于当前进程，而是可以操作调用者可以访问的任何进程。
> fallback 和 clean slate 是什么意思呀 ?

This yields the flexibility and orthogonality of the fork/exec model, without most of its drawbacks: a new process starts as an empty address space, and an advanced user may manipulate it in a piecemeal fashion, populating its address-space and kernel context prior to execution, without needing to clone the parent nor run code in the context of the child. ExOS [43] implemented fork in user-mode atop such a primitive. Retrofitting cross-process APIs into Unix seems at first glance challenging, but may also be productive for future research.

In its place, we propose a combination of a high-level spawn API and a lower-level
microkernel-like API to setup a new process prior to execution.
> 提出替代 fork 的想法，分为 high level 和 low level 两个部分。

*Low-level: Cross-process operations.*
> 什么叫做 cross-process operation 呀!

Fork-only use-cases. There exist special cases where fork is
not followed by exec, that rely on duplicating the parent.
1. Multi-process servers. 如今没有这种操作了。
2. Copy-on-write memory. cow 是个好东西，但是没有必要和 fork 一起使用
POSIX would benefit from an API for using copy-on-write
memory independently of forking a new process. 

## Practical, transparent operating system support for superpages
> 回答问题 : 
> 1. 这个 superpages 到底是如何设计的


**Promotion**: Once a certain number of base pages
within a potential superpage have been allocated, assuming that the set of pages satisfy the aforementioned constraints on size, contiguity, alignment and protection, the
OS may decide to promote them into a superpage. 
> 是不是说，首先进行预约 superpage，只有当利用率还不错的时候，正式变为 superpage
> 那么 superpage 和 一般的 page 的区别在于 ?

The next section describes previous approaches to the problem, and Section 4 describes how our design effectively tackles all these issues.

> 是不是说: superpage 仅限于给 application memory，内核中间的内存需要使用这种内存吗 ?

Many operating systems use superpages for kernel segments and frame buffers. This section discusses existing superpage solutions for application memory, which
is the focus of this work. These approaches can be classified by how they manage the contiguity required for
superpages: reservation-based schemes try to preserve
contiguity; relocation-based approaches create contiguity; and hardware-based mechanisms reduce or eliminate
the contiguity requirement for superpages.

Reservations and page relocation can complement
each other in a hybrid approach. One way would be
to use relocation whenever reservations fail to provide
enough contiguity and a large number of TLB misses
is observed. Alternatively, page relocation can be performed as a background task to do off-line memory compaction. The goal is to merge fragmented chunks and
gradually restore contiguity in the system. The IRIX coalescing daemon does this and is described in [6], but no
evaluation is presented.

Our approach generalizes Talluri and Hill’s reservation mechanism to multiple superpage sizes. To regain
contiguity on fragmented physical memory without relocating pages, it biases the page replacement policy to
select those pages that contribute the most to contiguity.
It also tackles the *issues of demotion and eviction* (described in Section 2.3) not addressed by previous work,
and does not require special hardware support.
> 终于开始讲点正经事情了

A high-level sketch of the design contains the following components. Available physical memory is classified
into contiguous regions of different sizes, and is managed using a buddy allocator [14].
A multi-list reservation scheme is used to track partially used memory
reservations, and to help in **choosing reservations for preemption**, as described in Section 4.8. A population map
keeps track of memory allocations in each memory object, as described in Section 4.9. The system uses these
data structures to implement allocation, preemption, promotion and demotion policies. *Finally, it controls external memory fragmentation by performing page replacements in a contiguity-aware manner, as described in Section 4.4*.
The following subsections elaborate on these concepts.
> preemption 指的是什么东西 ?


> 物理页面实现会分配

Our policy is that, whenever possible, the system preempts existing reservations rather than refusing an allocation of the desired size.
When more than one reservation can yield an extent of the desired size, the reservation is preempted whose most recent page allocation occurred least recently, among all candidate reservations.
This policy is based on the observation that useful reservations are often populated quickly, and that reservations
that have not experienced any recent allocations are less
likely to be fully allocated in the near future.
> preempt 应该指的是处理 : 不同的 reservation 可能会重叠 ?

However, *coalescing by itself is only effective if the system periodically reaches a state where all
or most of main memory is available.* To control fragmentation under persistent memory pressure, the page
replacement daemon is modified to perform contiguityaware page replacement. 
> 为什么在这种情况下才是 ?

Each reservation
appears in the list corresponding to the size of the largest
free extent that can be obtained if the reservation is preempted.

It extends the basic design along several dimensions, such as support for
multiple superpage sizes, scalability to very large superpages, demotion of sparsely referenced superpages, effective preservation of contiguity without the need for
compaction, and efficient disk I/O for partially modified
superpages.
> 设计目标

A multi-list reservation scheme is used to track partially used memory
1. reservations, and to help in choosing reservations for preemption, as described in Section 4.8.
2. A **population map** keeps track of memory allocations in each memory object, as described in Section 4.9.
3. The system uses these data structures to implement allocation, preemption, promotion and demotion policies. Finally, it controls external memory fragmentation by performing page replacements in a *contiguity-aware manner*, as described in Section 4.4. 


5 Implementation notes : 
> Under memory pressure, the daemon moves clean
> inactive pages to the cache, pages out dirty inactive
> pages, and also deactivates some unreferenced pages
> from the active list. We made the following changes to
> factor contiguity restoration into the page replacement
> policy.


> 后面的测试内容显然可以加深理解深度
## An Analysis of Linux Scalability to Many Cores
> 1. 知道为什么选取这些程序为测试程序吗 ?
> 2. 内核的措施是什么 ?
> 3. 测试结果说明什么 ?

Here are a few types of serializing interactions that
the MOSBENCH applications encountered. 
- The tasks may compete for space in a limited-size
shared hardware cache, so that increasing the number
of cores increases the cache miss rate. This problem
can occur even if tasks never share memory.

- The tasks may compete for other shared hardware
resources such as inter-core interconnect or DRAM
interfaces, so that additional cores spend their time
waiting for those resources rather than computing.

- There may be too few tasks to keep all cores busy,
so that increasing the number of cores leads to more
idle cores.

so sharing mutable data can have a disproportionate effect
on performance.

Exercising the cache coherence machinery by modifying shared data can produce two kinds of scaling problems.
1. First, the cache coherence protocol serializes modifications to the same cache line, which can prevent parallel
speedup.
2. Second, in extreme cases the protocol may saturate the inter-core interconnect, again preventing additional cores from providing additional performance.
**Thus good performance and scalability often demand that data be structured so that each item of mutable data is used by only one core.**

Performance is often the enemy of scaling. 

In all cases we were able to eliminate
scaling bottlenecks with only local changes to the kernel
code. The following subsections explain our techniques.
> 对于内核做出的修改是什么?

> 选了几个程序作为benchmark，然后分析内核对于多核的改进方法，最后进行分析测试。

section 4:
1. Multicore packet processing : 处理Tcp package，sample ,hash, long term and short term
2. sloppy counter 
3. Lock-free comparison
  1. The lock-free protocol uses a *generation counter*, which the *PK* kernel increments after every modification to a directory entry (e.g., mv foo bar).
  2. 这个 lock free 和我理解的 lock free 不是一个东西呀! 感觉是一个 optimistic lock 吧!
4. Per-core data structures
  1. sloppy counter 不就是这个的特例，为什么需要单独说明 ?
5. Eliminating false sharing
6. Avoiding unnecessary locking

> Evaluation 的部分就跳过吧!

Different applications or more cores are certain to reveal
more bottlenecks, just as we encountered bottlenecks at
48 cores that were not important at 24 cores. *For example, the costs of thread and process creation seem likely
to grow with more cores in the case where parent and
child are on different cores.*
> 为什么，core 的数量增多，然后创建 thread 和 process 的难度就会增加呀。


## A Hardware Architecture for Implementing Protection Rings

The mechanisms allow
cross-ring calls and subsequent returns to occur
without trapping to the supervisor.
Automatic
hardware validation of references across ring
boundaries is also performed.
> 回答一下问题 :
> 1. 如何实现保护 ?(hardware 自动报错)
> 2. 如何实现 cross ring ?
> 3. 当访问的位置错误，segment fault 的触发过程是什么 ?

In Multics the strategy was adopted of
limiting the number of domains which may be associated
with a process, and of forcing certain relationships to
exist among the sets of access capabilities included in
the domains. The result is protection rings.
> 限制 process domain 相关的，

Restricting the start of execution in a particular domain to certain program locations, called **gates**, provides this ability, for it gives the program sections that begin at those locations complete control over the use made of the access capabilities included in the domain. Thus, changing the domain of execution must be restricted to occur only as the result of a transfer of control to one of these gate locations of another domain.
> syscall 的实现原型

To provide control of this downward ring switching capability which is consistent with the subset property of rings, a gate extension to the execute bracket of a segment is defined. The gate extension specifies the consecutively numbered rings above the execute bracket of the segment that include the "transfer to a gate and change ring" capability for the segment. The gate list and the gate extension to the execute bracket can both be specified with additional fields in each SDW.
> gate 也是存在入口的规定的

The abstract description of rings is now one step
from completion. The last step comes from the observation that for each procedure segment in the virtual memory of each process there is a lowest-numbered ring in
which that procedure is intended to execute.
> 实际上，左侧只是其中只是针对于 exe 而已，read 和 write 的左侧都是从 0 开始的
In order
to provide the means for preventing the accidental
transfer to and execution of a procedure in a ring lower
than intended, the requirement that execute brackets
have a lower limit at ring 0 is relaxed and instead an
arbitrary lower limit is allowed.
> 内核可以执行用户态的程序吗 ?

The stack of a process is implemented with a separate segment for each ring being used. 

The next two sections describe in more detail how
**downward calls**, **argument referencing and validation**, and **upward returns** are implemented.


## The Duality of Memory and Communication in the Implementation of a Multiprocessor Operating System 
1. Accent 是什么东西呀 ?
2. memory 的实现存在什么特殊之处吗 ? 和 communication 的关系是什么 ?
3. data manager 是啥 ?

http://web.mit.edu/darwin/src/modules/xnu/osfmk/man/ : API 手册
https://www.os-book.com/OS9/appendices-dir/b.pdf : Mach 操作系统的发展历史

An important component of the Mach design is the use of memory objects which can be managed either by the kernel or by user programs through a message interface.
> 通过 message interface 进行操作 memory objects

A key and unusual element in the design of Much is the
notion that communication (in the form of **message passing**)
and **virtual memory** can play complementary roles, not only in
the organization of distributed and parallel applications, but in
the implementation of the operating system kernel itself.
> message 和 memory 是互补的存在的

Mach uses memory-mapping techniques to make the passing
of large messages on a tightly coupled multiprocessor or
uniprocessor more efficient. In addition, Mach implements
virtual memory by mapping process addresses onto memory
objects which are represented as communication channels and
accessed via messages. 

The design of Mach owes a great deal to a previous system
developed at CMU called **Accent** [15]. A central feature of
Accent was the integration of virtual memory and communication. Large amounts of data could be transmitted
between processes in Accent with extremely high performance through its use of memory-mapping techniques. This
allowed client and server processes to exchange potentially
huge data objects, such as large files, without concern for the
traditional data copying costs of message passing
> Accent 只是之前提到过的类似于 Android 中间的 Binder 的内容

It supported a single level store in
which primary memory acted as a cache of secondary storage.
Filesystem data and runtime allocated storage were both implemented as disk-based data objects.
> 这不就是 mmap 吗 ?

Large amounts of data could be transmitted
between processes in **Accent** with extremely high performance through its use of memory-mapping techniques.
> 通过 IO 映射直接共享信息, 但是 shmem 难道不就是使用这一个方法实现的吗 ?

Copies of large messages were managed using **shadow paging techniques.**

There are **four** basic abstractions that Mach inherited
(although substantially changed) from Accent: task, thread,
port and message. Their primary, purpose is to provide control over program execution, internalprogram virtual memory
management and interprocess communication. In addition,
Mach provides a fifth abstraction called the **memory object**
around which secondary storage management is structured. It
is the Math memory object abstraction that most sets it apart
from Accent and that gives Mach the ability to efficiently
manage system services such as network paging and filesystern support outside the kernel.

Messages sent to such a port result in operations being performed on the object it represents.

A task's address space consists of an ordered collection of valid memory regions.

The Mach external memory management interface is based
on the the Math memory objects . Like other abstract objects in
the Math environment, **a memory object is represented by a
port**. Unlike other Mach objects, the memory object is not
provided solely by the Math kernel, but can be created and
serviced by a user-level data manager task.
> 有点 interesting !

Section 3: 分析 Mach 的实现，主要四个部分:
3.1. Execution Control Primitives : task 和 thread 两个粒度
3.2. Inter-Process Communication : ports 和 messages 是实现IPC的基础，实际上
3.3. Virtual Memory Management : 
3.4. External Memory Management : 

The Mach external memory management interface is based
on the the Math memory objects . Like other abstract objects in
the Math environment, a memory object is represented by a
port.

> memory management 基于 memory objects，而 memory objects 基于 port 

A memory object is an abstract object representing a collection of data bytes on which several operations (e.g., read,
write) are defined. 

when no references to a memory object remain, and all
modifications have been written back to the memory object,
the kernel deallocatesits rights to the three ports associated
with that memory object. The data manager receives notification of the destruction of the request and name ports, at which
time it can perform appropriate shutdown.
> 三个端口为 : name port，request port 和 ....@todo


Section 4: 利用 Section 3 中间已经讲解的API 实现两个小demo

Section 5: 

Four basic data structures are used within the Mach kernel to
implement the external memory management interface:
**address maps**, **memory object structures**, **resident page structures**, and a set of **pageout queues**.

A list of resident page structures is attached to the object in order to expediently release the pages associated with an object when it is destroyed.

Section 6 : The Problems of External Memory Management
1. Data manager fails to free flushed data : A data manager may wreak havok with the pageout process by failing to promptly release memory following pageout of dirty pages.

## Memory Resource Management in VMware ESX Server
A **ballooning** technique reclaims the pages considered least valuable by the operating system running in a virtual machine.
An **idle memory tax** achieves efficient memory utilization while maintaining performance isolation guarantees.
**Content-based page sharing** and **hot I/O page remapping** exploit transparent page remapping to eliminate redundancy and reduce copying overheads.
These techniques are combined to efficiently support virtual machine workloads that overcommit memory.

The balloon driver communicates the physical page number for each allocated
page to ESX Server, which may then reclaim the corresponding machine page.
Deflating the balloon frees up
memory for general use within the guest OS.

3.1 Page Replacement Issues : The standard approach used by
earlier virtual machine systems is to introduce another
level of paging
> 我猜测 : 是因为当 一个 page 被换出的时候，在 guest os 哪里看不到变化的，


Future guest OS support for hot-pluggable memory
cards would enable an additional form of coarse-grained
ballooning. Virtual memory cards could be inserted into
or removed from a VM in order to rapidly adjust its
physical memory size.
> 我真的是万万没有想到，hot-pluggable 居然还有这种效果 !


**When ballooning is not possible or insufficient,
the system falls back to a paging mechanism.**
Memory is reclaimed by paging out to an ESX Server swap area on
disk, without any guest involvement.

> 3.3 Demand Paging
> 感觉内容 和 demand paging 没有什么关系啊!

6 Allocation Policies

ESX Server computes a target memory allocation for
each VM based on both its share-based entitlement and
an estimate of its **working set**, using the algorithm presented in Section 5. These targets are achieved via the
ballooning and paging mechanisms presented in Section 3. Page sharing runs as an additional background
activity that reduces overall memory pressure on the system. This section describes how these various mechanisms are coordinated in response to specified allocation
parameters and system load.

## An Updated Performance Comparison of Virtual Machines and Linux Containers
> https://blog.jessfraz.com/post/containers-zones-jails-vms/

> https://serverfault.com/questions/208693/difference-between-kvm-and-qemu
> https://www.packetflow.co.uk/what-is-the-difference-between-qemu-and-kvm/ : 这个总结的也不错
> 这么说，KVM 其实就是 QEMU 的内核态的支持，如果可以实现跨架构，那还是非常不错的。

> 问题 : 为什么作者不介绍 QEMU ?

> 应该认真理解前面的介绍，总结其中的各种结论
> introduction + background + evaluation 

We use KVM as a representative hypervisor and Docker as a container manager.
Our results show that containers result in equal or better
performance than VMs in almost all cases. Both VMs and
containers require tuning to support I/O-intensive applications.
We also discuss the implications of our performance results for
future cloud architectures.

**We expect other hypervisors such as Xen, VMware ESX, and
Microsoft Hyper-V to provide similar performance to KVM
given that they use the same hardware acceleration features.**

We make the following contributions:
• We provide an up-to-date comparison of native, container, and virtual machine environments using recent
hardware and software across a cross-section of interesting benchmarks and workloads that are relevant to
the cloud.
• We identify the primary performance impact of current
virtualization options for HPC and server workloads.
• *We elaborate on a number of non-obvious practical
issues that affect virtualization performance.*
• We show that containers are viable even at the scale
of an entire server with minimal performance impact.
> 应该在测试中间说明，但是并没有清晰的指出来，找到是如何回答以上问题的 ?


A problem caused by Unix’s shared global filesystem is
the lack of **configuration isolation**. 

KVM supports both
emulated I/O devices through QEMU [16] and paravirtual
I/O devices using **virtio** [40].

The primary type of
security vulnerability in containers is system calls that are not
namespace-aware and thus can introduce accidental leakage
between containers. Because the Linux system call API set is
huge, the process of auditing every system call for namespacerelated bugs is still ongoing. Such bugs can be mitigated (at the
cost of potential application incompatibility) by whitelisting
system calls using seccomp [5].


By default, KVM does not expose
topology information to VMs, so the guest OS believes it
is running on a uniform 32-socket system with one core per
socket.
> 在本文中间，socket 的含义到底是什么 ?

Table I shows the performance of Linpack on Linux,
Docker, and KVM. Performance is almost identical on both
Linux and Docker–this is not surprising given how little OS
involvement there is during the execution. However, untuned
KVM performance is markedly worse, showing the costs of
abstracting/hiding hardware details from a workload that can
take advantage of it. By being unable to detect the exact nature
of the system, the execution employs a more general algorithm
with consequent performance penalties. Tuning KVM to pin
vCPUs to their corresponding CPUs and expose the underlying
cache topology increases performance nearly to par with
native.
> HPC 中间 KVM 的问题的处理办法!

Containers that do not use NAT have identical performance to native Linux.

Unlike as in the throughput test, virtualization overhead cannot be amortized in this case.

As we would expect, Docker introduces no overhead
but KVM delivers only half as many IOPS
because each I/O operation must go through QEMU. 

> 第三部分的测试内容包括 : CPU HPC IO 等等。

## Section 2

#### Obsevation on the Development of the Operating System
In this section, we will recount a few of the problems we encountered and lessons we learned during the development.

> 作者总结发开发其的教训

> 分析了几类操作系统

> 总结，为什么存在5~7年的开发周期

#### Fine-Grained Dynamic Instrumentation of Commodity Operating System Kernels
The contents of the inserted code—whether performance profiling annotations, optimized versions of functions, or process-specific kernel extensions—are orthogonal to the issue of how to splice it into a commodity kernel.

**Dynamic kernel instrumentation is the process of splicing dynamically generated code sequences into specified points in the kernel code space.**
> 这么NB，为什么不开发一个用于应用层的工具。

To instrument a running kernel, kerninstd needs to allocate the patch area heap, parse the kernel’s runtime symbol table, and obtain permission to write to any portion
of the kernel’s address space.
> 这个要求不简单，@todo 就算是做一个用户态的动态插入代码那都是很难的事情呀!

The code generation and splicing phases of dynamic instrumentation are decoupled;
> 实现的两个主要难点，code generation 和 splice 

kerninstd’s dynamic instrumentation
steps. This section describes the first three: live register
analysis, allocating a patch to hold the generated code,
and code generation.

Fast fine-grained code splicing is KernInst’s major technology contribution. Splicing is the action of inserting
runtime generated code before a desired kernel code
location (the instrumentation point). 

splice 的三个要点:
1. 生成的代码的布局如何 ?
2. 将指令替换为跳转指令的难度是什么 ?
3. 似乎是跳转的范围有限，所以进行特殊处理一下。

#### Threads and Input/Output in the Synthesis Kernel

#### Information and Control in Gray-Box Systems
In this paper, we develop and investigate three
gray-box Information and Control Layers (ICLs) for determining the contents of the file-cache, controlling the layout of
files across local disk, and limiting process execution based on available memory.

In Figure 1, we demonstrate this relationship by plotting the correlation between the **presence** of a random page
in a prediction unit (i.e., a contiguous region of the file)
and the percentage of the unit within the cache.
> prediction unit : 表示预测的范围，而 access size 表示其中的 size 的大小而已。


From the graph, we can see that when the prediction unit
is less than or equal to the access size, the presence of the
probed page is highly correlated with the presence of the
entire prediction unit.
> access size : 在文件的某一个位置开始进行连续访问的大小

If the prediction unit is too large
relative to the access unit of the application, then the cor-

> 关于 prediction size 以及 access size 以及表格理解有点问题，所以无法理解后面 5M 以及 20M 在说明什么


We now perform experiments to demonstrate the utility
and efficacy of our gray-box FCCD. We begin by showing
that our software obtains good performance when reordering
accesses from within a single large file and when reordering
accesses across several files. We then examine the benefits
to two applications modified to use our interfaces: fastsort
and grep. Finally, we demonstrate that our techniques work
well across three different Unix-based operating systems.

instead, this scan sequentially accesses segments of the file in the
size directly determined by the access unit.

> 关于 FCCD，探测的过程中间，不会就造成 file cache 的预取吗 ?


#### Plan 9 from Bell Labs
For
Plan 9, we adopted this idea by designing a network-level protocol, called 9P, to enable
machines to access files on remote systems.

This paper serves as an overview of the system. It discusses the architecture from
the lowest building blocks to the computing environment seen by users. It also serves
as an introduction to the rest of the Plan 9 Programmers Manual, which it accompanies.
More detail about topics in this paper can be found elsewhere in the manual.
> 本文的定位，介绍 Plan 9 的构成

The view of the system is built upon **three principles**. First, resources are named
and accessed like files in a hierarchical file system. Second, there is a standard protocol,
called 9P, for accessing these resources. Third, the disjoint hierarchies provided by
different services are joined together into a single private hierarchical file name space.

**Parallel programming** : Plan 9 support for parallel programming has two aspects. First, the kernel pro›
vides a simple process model and a few carefully designed system calls for synchroniza›
tion and sharing. Second, a new parallel programming language called Alef supports
concurrent programming. 
**File Caching** : 

**The IL Protocol** : The 9P protocol must run above a reliable transport protocol with delimited messages.

> 终于到达总结性的内容 :

1. Plan 9 has a relatively conventional kernel; the system's novelty lies in the pieces
outside the kernel and the way they interact. When building Plan 9, we considered all
aspects of the system together, solving problems where the solution fit best. Some›
times the solution spanned many components. 
2. The best example is 9P, which centralizes naming, access,
and authentication. 9P is really the core of the system; it is fair to say that the Plan 9
kernel is primarily a 9P multiplexer.
3. Plan 9's focus on files and naming is central to its expressiveness.
4. 同时: 其存在一些问题:

Converting every resource in the system into a file system is a kind of metaphor, and metaphors can be abused.

> 这里将Plan 9 作为一个 research platform，之前也看到过 K42 作为 research 的平台，这种脱离实践或者说大规模使用的，其作用在哪里呀。

## 问题
1. 为什么 log 的 fs 实现纠错非常简单 ?
    1. 纠错的过程是什么 ? 定义一个 transaction ?
    2. 当不是基于 log 的为什么进行全盘扫描就可以实现 ?
