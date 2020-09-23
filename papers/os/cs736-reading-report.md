# WISCONSIN CS763 论文阅读

http://pages.cs.wisc.edu/~dusseau/Classes/CS736/CS736-F13/questions.html

首先分析了几个经典操作系统的特点，Multics的分析段式寻址权限管理，Synthesis 的线程和IO实现，Mach 的将通信和内存融合，了解 Bell 实验室的Plan 9 的整体设计，以及通过 Pilot 分析一个操作系统的开发遇到的问题和发展历程。

关于文件系统，基于日志(log)的文件系统 Spirit LFS 是这方面的开山之作，ext3 的journal机制采用了类似的思想，但是主要目的是保持文件系统一致性，f2fs 是也是基于日志的文件系统，但是是为固态存储和闪存而设计的。

内存管理方面，分析了 hugetlb 的实现，多核方面，对于特定的程序进行，作者提出优化方法并且得出结论现在的linux内核足以应对多核的挑战。

虚拟化上，分析了VMware ESX Server 如何管理内存资源，让多个虚拟机的使用的物理内存大于实际物理内存，比较了以KVM为例子的虚拟机，和以Docker为例的容器的性能。

进程管理上，有人提出 fork 已经不适合现代的操作系统和硬件，将 fork 从操作系统中间移除才是正确的抉择。

<!-- vim-markdown-toc GitLab -->

- [canonical](#canonical)
    - [A Hardware Architecture for Implementing Protection Rings](#a-hardware-architecture-for-implementing-protection-rings)
    - [Threads and Input/Output in the Synthesis Kernel](#threads-and-inputoutput-in-the-synthesis-kernel)
    - [The Duality of Memory and Communication in the Implementation of a Multiprocessor Operating System](#the-duality-of-memory-and-communication-in-the-implementation-of-a-multiprocessor-operating-system)
    - [Plan 9 from Bell Labs](#plan-9-from-bell-labs)
    - [Obsevation on the Development of the Operating System](#obsevation-on-the-development-of-the-operating-system)
- [File System](#file-system)
    - [The Design and Implementation of a Log-Structured File System](#the-design-and-implementation-of-a-log-structured-file-system)
    - [Journaling the Linux ext2fs Filesystem](#journaling-the-linux-ext2fs-filesystem)
    - [F2FS: A New File System for Flash Storage](#f2fs-a-new-file-system-for-flash-storage)
- [Memory Management](#memory-management)
    - [Practical, transparent operating system support for superpages](#practical-transparent-operating-system-support-for-superpages)
- [Multicore](#multicore)
    - [An Analysis of Linux Scalability to Many Cores](#an-analysis-of-linux-scalability-to-many-cores)
- [Techniques](#techniques)
    - [Information and Control in Gray-Box Systems](#information-and-control-in-gray-box-systems)
    - [Fine-Grained Dynamic Instrumentation of Commodity Operating System Kernels](#fine-grained-dynamic-instrumentation-of-commodity-operating-system-kernels)
- [Virtual](#virtual)
    - [Memory Resource Management in VMware ESX Server](#memory-resource-management-in-vmware-esx-server)
    - [An Updated Performance Comparison of Virtual Machines and Linux Containers](#an-updated-performance-comparison-of-virtual-machines-and-linux-containers)
- [Process](#process)
    - [A fork in the road](#a-fork-in-the-road)

<!-- vim-markdown-toc -->

## canonical

#### A Hardware Architecture for Implementing Protection Rings
本文分析 Multics 操作的 protection Ring 的实现，分析为什么权限需要设置为 ring 的形式，在实现 cross ring 的调用。
虽然描述是基于 segment 的内存模型，而且相对复杂，但是核心思想并没有变化: 一个 segment 的权限分为读，写，执行，gate 四种，其中 gate 可以从该 segment 到达的 ring 的范围。
通过 gates 可以实现从外层的ring 进入到内层的 ring。

protection 机制的基本要求和设计标准
1. 基本要求: 控制用户之间的交互。保证用户相互隔离，同时提供交互。
2. 设计标准:
  - functional capability : 能够完成软件需要的功能
  - economy : 不会添加过多的性能损失。
  - simplicity : 容易理解和验证正确性
  - programming generality : 易于编程。

protection ring 要求硬件的功能:
1. 验证访问的地址合法
2. 实现切换所在的 ring

在 Multics 的 gates 的含义:
在一个 domain 开始执行的程序都被限制到特定的位置，这些位置就是 gates，从 gates 开始执行的程序具有访问该 domain 的权限，因此切换当前执行的程序的域，只能在通过gates 实现。
虽然，形式和 syscall 差别很大，但是功能是完全相同的。 同 ring 的 inter segment 的调用，可以使用普通的 transfer 指令，从而绕过 call 对于 gate 的限制。

实现call 和 return 需要解决的三个基本问题:
  1. stack : 当进程切换到low ring 的时候，原来的ring 中间的stack 是不可以访问的，如何将确立新的ring。将 ring number 和 该 ring 的 stack segment ring number 自动关联起来。
  2. 参数 ：进入到low ring 的时候，可以访问 high ring 的参数。利用PR 和 IND，以及硬件的配合实现在自动 validate 参数访问。
  3. 返回到正确的ring :  在 Multics 的这个模型中间， 一个进程ring 从 low ring 到 high ring 返回之后，虽然进程可以返回到正确的segment，但是无法返回到正确的ring。
为什么现在不用处理这一个问题: 内核态和用户态的管理的虚拟地址空间不同，那么从内核的虚拟地址空间切换到用户的虚拟地址空间，自然知道回到的位置所在的 ring 是什么。
但是 Multics 提出的模型，这些问题显然是存在的，返回之后，进程没有办法从 segment 的属性得到其该处于何种 ring 的状态。
这个问题相当于，当内核和用户的虚拟地址空间都是0~4G的时候，对于中断返回，没有特殊的分析，程序是没有办法知道自己现在是在内核态还是用户态的。
解决方法 : 将调用者的 ring 放到一个程序可以访问的寄存器中间，也就是PR 中间。实际上，返回的时候 ring 可能比调用的时候更加大。

为什么处理 upward call 是非常复杂的:
1. called 访问 calling procedure 的参数会出现问题，在处理 downward call的时候，只会存在 validate indirect reference 的问题。(validate 的问题是 : 连calling procedure 也是没有办法访问的) 作者提出了三种解决方法，
但是前面两种方法，让编程变得麻烦(programming generality)，第三种无法共享。
    1. 让参数放到 higher ring 可以访问的位置。
    2. 动态的让 called procedure 所在的ring具有访问 segment 的能力，但是这表示该 segment 只能含有参数。
    3. 将参数拷贝。
2. 第二是，需要提供一个向下返回的机制: 因为upward return 的过程从low ring 到high ring 的过程，没有什么问题，但是从high 到 low 需要借助 gate 实现。
可以动态创建gate 来实现upward 返回，但是这种动态创建，难以处理函数是递归的情况。最终 Multics 对于 upward call 的处理是，触发 trap，交给软件完成。

构建访存的基本含义:
1. DBR : address + length 描述一个 descriptor segment 的偏移和长度，descriptor segment 是一组 SDW
2. SDW : address + length 描述该 segment 范围，R1 R2 R3 R W E 描述 read write exe 以及 gate extension 的范围，GATE 描述该 gates 的数量。描述的范围如下:
  - write 从 0 到 R1
  - exe R1 到 R2
  - gate R2 +1 到 R3 
  - read 0 到 R2
3. IPR && TPR : 当前进程所在的 ring。segno 和 wordno 共同描述下一条指令的地址。TPR 表示是临时IPR，在硬件进行地址翻译的过程中间使用。
4. program accessible pointer register(简称PR) 和 inst 格式: PR 指向一个特定的地址，inst 利用PRNUM 表示基准的PR，offset 表示在 PR 指向的地址的偏移量来说明操作数的位置。 PR 的作用有两个: 一个作用是访问在 higher-numbered ring 的指令，其次可以用作 stack pointer
5. IND : 用于间接访问，这是解决call and return 的参数访问的关键。

专题分析1: 在这个上下文中间，trap 和 gate 到底指的是什么东西 ?
[参考这个](https://stackoverflow.com/questions/3425085/the-difference-between-call-gate-interrupt-gate-trap-gate)
> A gate (call, interrupt, task or trap) is used to transfer control of execution across segments. Privilege level checking is done differently depending on the type of destination and instruction used.

*The hardware mechanisms detailed in the next section eliminate the need to trap in these cases.*
> trap 的含义 : 陷入到 kernel 中间，无法回去

*Although the same object code sequences that perform all calls and returns are used in these cases as well, the hardware responds to each attempted upward call or downward return by generating a trap to a supervisor procedure which performs the necessary environment adjustments.*
> trap 只是表示进一步向下调用

The access violations and other conditions requiring software intervention shown in this and following figures generate traps, derailing the instruction
cycle. A traps action is described later in this section.
> 并不是表示调用，应该更多的是表示如何才可以进行 需要 supervisor 的服务

Two items remain to be considered to complete the
description of the processor hardware for implementing
rings. One is the action of a trap. Traps are generated by
a variety of conditions in Figures 4-9, as well as by
missing segments and pages, I/O completions, etc. *When
the processor detects such a condition, it changes the
ring of execution to zero and transfers control to a fixed
location in the supervisor.* A special instruction allows
the state of the processor at the time of the trap to be
restored later if appropriate, resuming the disrupted
instruction .

temp 记录:
1. gate 总是和各种指令放在一起的 ? call jmp syscall 以及 int32 指令，找到这些指令的使用方法是什么 !


> Call gates are (or have been, probably) gradually abandoned in favour of the SYSENTER/SYSEXIT mechanism, which is faster.

难道不是 int gate 被放弃吗 ?

Interrupt & trap gates, together with task gates, are known as the Interrupt Descriptor Table.

专题分析2: 如何处理 privileged 的指令 ?
只是不许执行即可

*Implicit invocation of certain ring 0 supervisor
procedures occurs as a result of a trap.
Explicit invocation of selected ring 0 and ring 1 supervisor procedures
by procedures executing in rings 2-5 of a process is by
standard subroutine calls to gates. Procedures executing
in rings 6 and 7 are not given access to supervisor gates.*
> 分为各种层次使用

专题分析3: 现代操作系统如何实现 syscall 的参数传递的



启发和想法:
1. Multics 操作系统对于后来的影响巨大，可以找一些对于该系统综述的文章阅读一下。
2. 是不是描述 segment 是一个 bracket，而 process 的权限只能是一个数值，是否存在 process 也是 bracket 的范围。

#### Threads and Input/Output in the Synthesis Kernel
本文描述了 Synthesis 这个系统的实现，本文重点分析了 synchronization 的优化，以及基于此的 threads 和 IO 的实现。
Synthesis 通过 kernel code synthesis，粒度适中的调度(fine-grained scheduling)，以及 乐观同步(optimistic synchronization) 来实现高性能。

fine-grain scheduling : Synthesis 认为一个thread是否应该被执行取决于所在的 quaspace 的 IO 频率。

实现 synthesize code ，Synthesis 提出三种方法:
1. Factoring Invariants : 类似于常数折叠(constant folding)，如果函数调用的结果是一个常数，那么可以去除掉该函数调用。
2. Collapsing Layers : 消除不必要的函数调用，第二次函数调用复用第一次的结果。
3. Executable Data Structures : 数据结构中间持有可执行的代码，经常遍历的数据会插入一些数据，让其可以自己遍历。

Synthesis 使用三种方法来实现减少同步的:
1. code isolation : 是如今 percpu 的雏形
2. procedure changing : 当Synthesis 在处理中断的时候，如果此时信号到达，不会立刻处理信号，而是将信号处理放到中断处理结束的时候，这样让可以简化 synchronization.
3. optimistic synchronization : 也就是乐观锁。

Synthesis context-switch 更加短是是因为:
1. 只保存，替换会发生修改的部分，例如大多数程序都是不会访问浮点寄存器(linux 内核态用于不会访问到浮点寄存器，这意味内核态之间的context switch 绝对不会涉及到浮点寄存器)
2. 使用可执行的数据结构来缩短关键路径。在数据结构中间保存可执行的代码，消除掉 dispatcher 的干预，让停止执行的线程直接执行就绪线程。这种方法似乎引入不安全(将数据和代码放在一起，而且代码可以动态修改)，所以现代处理器并没有使用了。

quabject 是一组程序，数据和硬件资源的集合，大多数 quabject 通过少量的 building block 实现，例如显示器，队列，调度器。quabject 通过 quabject creator 生成，其生命周期为 : 分配，factorization ，优化。
分配阶段分配内存和代码，factorization 使用常数将替换其中"模板代码"。quabject 用于启动创建好了的 quabject，启动分为: 资源连接，优化，程序接口链接

在 Synthesis 中间，物理IO设备被封装在quajects 中间，叫做 device server.
每个 device server 可以含有自己的thread。一个 device server 可以由多个基础 device server 组成。
在系统启动的时候，内核为裸物理设备创建出来对应的 device server.

启发和想法:
1. Synthesis 所谓的 fine-grain scheduling 的依据是IO，这非常的不科学，除非其中存在另外的深层次的原因，或者描述过于简略。

专题分析1:
本文核心分析的内容是，thread 和 IO，
1. thread 部分讲解了什么东西 ? 就像讲解 thread 是什么，如何运行，并且分析其中两个关键问题，如何进行 context switch 和 scheduling 
2. kernel code synthesize 对于其的支持是什么 ? 比如 context switch  利用的 executable data structures

专题分析2:
Kernel code systhesize 到底指的是 ?
在backgroup 的分析中间，提出三种方法，而在 introduction 部分，只有两种方法.
其实关于systhesis 是什么吸引了过多的注意力，真正重要的是 thread 和 IO，以及 sync 的设计方法。

专题分析3:
Physical I/O devices are encapsulated in quabject called device servers.
Each device server may have its own threads or not.
A polling I/O server would run continuously on its own thread. An interrupt-driven server would block after its initialization.
High-level servers may be composed from more basicservers. 
The implementation of the stream model of I/O in Synthesis can be summarized using the well-known producer/consumer paradigm.
Synthesis interrupt handling differs from some traditional OS’s (such as UNIX) in that each thread in Synthesis synthesizes it's own interrupt handling routine, as well as system calls. 

专题分析4:
interrupt 是如何分析的 ?
1. Synthesis interrupt handling differs from some tra.ditional OS’s (such as UNIX) in that each threa.d in Synthesis synthesizes it,s own interrupt ha.ndling routine, as
well as system calls. 

#### The Duality of Memory and Communication in the Implementation of a Multiprocessor Operating System 
Mach 从 Accent 继承了四种抽象: task, thread, port and message. 
其中taks 和 thread 表示进程和线程，port 是消息队列，message 表示消息，也就是 task 或者 thread 用 port 进行传递 message
在这些基础上，Mach 提供用于处理二级存储的缓存的 memory object 新的抽象，这也是 Mach 的核心设计。

主要内容为: 
1. Mach 设计，也就是介绍API
    0. Message Operatiation
    1. Port Operatiation
    2. Virtual Memory Operatiation
    3. External Memory Management
2. 使用minial File System 和 consistent network shared memory service 两个例子作为说明
3. 解释 external memory management 的实现，其依赖于四个基本的数据结构:
    1. address maps : 虚拟地址和 memory object 的关系表，通过
    2. memory object structures :
    3. resident page structures : 描述一个物理页面的信息
    4. a set of pageout queues : 用于实现页面换出的队列
4. 简要说明 external memory management 实现问题和解决方案
5. 简要论述处理 UMA NUMA 和 NORMA 三种情况

关键问题说明:
1. single level store 是什么? external memory mamagement? memory object? data manage task? pager?
> - page cache
> - second storage management
> - memory object 是Mach 设计核心概念，其定位类似于linux的virtual memory area，不同的地方放在和Mach的 communication 机制结合之后，可以关联一个port, 并且使用message操控。
> - 其实就是管理 secondary disk 的线程
> - pager is frequently used to describe the data manager task that implements a memory object.

2. 相比 Accent，Mach 的优势是什么 ?
> Accent 存在硬件适配以及 Unix 兼容性问题。

启发和想法:
1. Mach 算是最早的微内核之一，感受了一下微内核设计理念和宏内核的区别

#### Plan 9 from Bell Labs
本文介绍Plan 9 的整体设计思路。

主要内容:
- 图形界面
- 文件服务器
- 内核配置
- 权限管理
- 文件缓存
- 对于C原因的支持以及并行编程语言Alef
- 命名空间的实现
- 文件缓存
- 网络
- 访问设备
- 身份认证
- 文件权限

1. 为什么Plan 9 被开发出来 ?
> 作者认为Unix过于古老，并没有很好的将网络和图形界面集成到操作系统中间，而且觉得Unix让设备驱动的书写变的比较麻烦。我个人感觉最重要的原因是Bell lab的人有能力写，所以写了一个。

2. 核心设计理念 ?
Plan 9 的设计思想基于三个方面:
1. 资源被抽象为进行访问
2. 使用标准协议，9P，来实现访问这些资源
3. 每一个服务都是划分为层级，然后将这些服务整合在一起形成用户的私有资源命名空间

3. 分布式操作系统体现在 ?
Plan 9 是各种功能集合，比如文件server，多核server 以及 terminal，这些设备通过高速网络连接起来，并且通过 Ethernet 来远程访问。

启发和想法:
1. 无

#### Obsevation on the Development of the Operating System
Pilot 是单用户，多任务的Mesa语言编写的操作系统，Pilot的作者回顾开发的教训和经验。

教训总结:
1. 内核镜像过大
2. 内存消耗多
3. 在一些基本问题上过多的争论，消耗过多的时间和精力。比如进程管理到底是基于message 还是 procedure and monitor。读写文件是使用直接传输物理页面的方法还是内存映射的方法发。
4. 文件的 transaction 设计没有集成到内核中间，在用户态的 transaction 机制性能和稳定性都有问题。
5. 虚拟内存将三种不同的接口，allocation mapping以及 swapping 融合在一起，结果难以实现，而且不容易使用。
6. 自行设计的IO工具没有被用户广泛使用。


作者提出的对于操作系统的5种分类:
1. 简单小众
2. 商用
3. 兼容性操作系统
4. 教学操作系统
5. 没见过的操作系统

为什么作者说一般操作系统需要5~7年的开发周期 ?
> 需要经过设计，验证，完善和需求改变。

启发和想法:
1. 可以阅读 Pilot: An Operating System for a Personal Computer 获取更加深刻的理解

## File System 

#### The Design and Implementation of a Log-Structured File System
log-structures fs 的开山之作，由于操作系统使用内存作为二级存储的缓存，大多数的读都会被缓存拦截，这让读操作的性能变得不在重要，此外大多数的读写都是小文件，而LFS的写的顺序性让写变得很快。
除此之外，LFS的写顺序性让文件系统一致性非常容易实现。对于Flash存储的特性，寿命短，需要整块更新，LFS也是十分契合。

文件系统一致性: 很多操作是原子性质的，例如添加一个新的文件时候，同时需要修改其所在的文件夹的内容，当第一步操作完成了系统崩溃掉了，为了消除这种 inconsistency，必须扫描所有的文件进行比对，因为无法知道在崩溃前进行了那些操作。

LFS需要解决的两个关键问题，如何读取log和压缩log.

文件删除或者 trunction 处理办法 : 并不需要特殊的标注，block 是否live 可以通过segment summary 中间保存的block uid 确定。为了加快live data 的确定，在inode map 中间保存了inode的版本号，当文件被删除或者截断为０的时候，版本号加1，通过比较block 的版本号和其所在的inode 版本号。

LFS中间的基本元素是什么:
1. inode, indirect block, data block, superblock: 和一般的文件系统没有区别
2. segment summary, segment usage block : 一个segment 的统计信息，分别保存其中block的身份信息和 live block 数量。segment summary 用于确定一个block 是否live 的，而 segment usage block 用于支持 segment clean 的判断。
3. inode map : 由于更新inode 是以日志方法，所以以前的通过inode number 就可以直接访问到inode 机制不存在了，所以需要。所以，当增长一个文件，不仅仅会产生data block 日志，而且会产生 inode 日志，进一步产生 inode map 日志。
4. Checkpoint Region : 保存inode map 和 segment usage table 的地址，并且确定日志中间最后的一个checkpoint
5. Directory change log : 用于解决 directory entries and inodes 不一致的日志。
6. 说明: segment summary 是 per segment 的，而inode map 和 segment usage block 都是描述全局信息，也是散布在任何segment 中间的

如何管理空闲磁盘的: 将磁盘划分为segment，

压缩log的方法: 也就是清理掉一个segment，也就是将该segment中间的live block 清理掉，具体的操作办法是将一些需要清理的segment 读入到内存中间，标记出来live data ,将live data 写入到更早的log 中间。

实现segment clean 策略的四个基本问题:
1. 什么时候启动 segment cleaner ?
2. 每次清理多少个 segment ?
3. 那些 segment 需要被清理 ?
4. 当 live block 被 written out 的时候应该如何组织。
对于前两个问题并没有很好的解决办法。


文件系统一致性的两个基本操作:checkpoint 和 roll-forward。使用roll-forward的原因是，尽可能回收在crash 的时候 所在segment 中间的数据，而不是直接启用checkpoint，将数据抛弃。

恢复的过程中间一共存在哪几种 consistency 需要考虑 ? 解决的办法又分别是什么？
1. 文件数据写入磁盘，但是inode中间的信息没有更新。解决方法: segment summary block 保存了一个 segment 其中的所有block 所在inode以及偏移量，当该block保存inode，则会更新inode map，当block保存普通数据，而且其对应的inode没有更新，那么直接放弃数据。
2. segment usage table 和 segment 中间的实际数据保存的数据量不一致。解决办法: 扫描整个segment 中间的live block。
3. directory entries and inodes 直接的不一致。解决办法: 在进行 directory entries 以及 inodes 之前，首先写入 directory operation log，当进行 roll forward 的操作的时候，根据 directory operation log 进行操作。

启发和想法:
1. segment usage table 中间保存的live block数量并没有很好的确定方法。
2. 本文定义write cost，并且以此为基础比较各种 segment clean 策略的好坏。是否存在其他的评价标准。

专题分析1：
segment 是什么 ?
为什么我们需要 segment usage table ?

> We are now left with two problems, however. The first is mechanism: how can LFS tell which blocks within a segment are live, and which are dead? The second is policy: how often should the cleaner run, and which segments should it pick to clean?
> 随着LFS的运行，其中有些部分的内容属于过时，
> 需要清理垃圾，所以采取的办法是按照 segment 为单位划分清理的
> 1. 知道哪一个是 out-of-dated 的 ?
> 在 segment summary data 被放到 segment 的开头位置，记录每一个 data block 的 inode 以及在该 inode 中间的偏移量
> 为了提升查找速度，struncated or deleted 的时候，inode map 中间更新 version number


专题分析2：
级联反映的过程，
inode map : 在日志中间，如果 inode 固定，那么修改 inode map 就需要做一个远距离的移动
总是需要一个，固定的内容，比如 checkpoint region，但是 checkpoint region 的固定，和 inode map 固定存在什么区别 ?

专题分析3:
Consistency 的维护过程是什么 ?
1. 如何理解 roll forward 机制 : 根据 CR 是完整，那么总是获取该 CR 时刻的内容，但是损失过大，其实根据日志内容，尽可能恢复。
2. 为什么需要两个 CR，两个 CR 的存放位置是什么 ? CR 的存放位置并不重要，之所以采用两个，是因为，当只有一个的时候，在写入的过程中间，突然发生 crash，那么CR就不是一致性的，
那么总是存在一个是 consistent 的

每一个CR都是存在两个 timestamp 的，在从内存写 timestamp 到磁盘的时候，开始写入一个 timestamp，结束的时候，也写入一个 timestamp，之后恢复的检查两个 timestamp 是不是一致的。
crash 发生的时机
1. 第一种是普通操作，写入各种数据
2. 第二种是CR 的写

专题分析4:
To restore consistency between directories and inodes, Sprite LFS outputs a special record in the log for each directory change. 

directory change log 中间包含的内容是什么 ?
1. The record includes an operation code (create, link, rename, or unlink), the location of the directory entry (i-number for the directory and the position within the directory), the contents of the directory entry
(name and i-number), and the new reference count for the inode named in the entry

首先写入日志，然后进行操作，roll-forward 利用 directory change log 实现 inode 和 log 之间的关系。


#### Journaling the Linux ext2fs Filesystem
ext2fs 采用 Journaling 的主要原因是为了处理系统崩溃之后，快速的恢复到一致性的状态，需要和LFS区分的在于，journal 的加入没有提升性能，甚至存在性能损耗，因为相同的数据首先需要从内存写入到磁盘的journal，当 transaction 结束之后，再从内存写入到主数据区。
但是，一旦出现crash，导致fs不一致出现的地方只能是在journal 中间，而主要数据区用于都是维持一致性的。

crash之后恢复fs一致性的方法:
As a result, filesystem recovery can be achieved by scanning the journal and copying back all committed data into the main filesystem area. 

文件系统的 transaction 和数据库的 transaction 的区别是什么，这种区别带来的好处是什么 ?
1. 没有 transaction abort，在数据库的 transaction 中间支持操作进行了一半撤销，并且将之前的产生的影响消除掉。但是文件系统的 transaction 在进行之前就已经进行了必要的检查，因此不会出现 transaction abort，由此不用考虑撤销操作，简化了 transaction commit
2. transaction 生命周期都很短，没有长期未能提交的 transaction 以及对于其的依赖，所以即使是串行提交，也不会操作太大的性能影响。
以上两个观察，可以将多个 transaction 进行合并起来。

利用ext2设计时保留的inode来描述journal.

journal中间包含什么 ?
1. metadata
2. descriptor : 描述 metadata 的数量和磁盘号
3. header blocks : 记录journal的首尾以及序列号

完成 commit 需要的步骤是什么 ?
1. 关闭 transaction，处于性能考虑，将多个 transaction 合并起来了，所以关闭 transaction 之后，还需要接收未完成的操作，新的操作放入到下一个 transaction。
2. 使用log-writer将内存中间修改的metadata 以及 dependent data 以journal的形式写入磁盘，并且将内存中间保存该 transaction 的 buffer 设置为 pinning 状态。
3. 等待该 transaction 中间的未完成的操作完成。
4. 等待所有的 transaction 的修改写入到磁盘的journal 中间。
5. 更新header block 中间的记录的日志的起始结束位置
6. 当journal 中间的数据被写入到主数据区之后，可以将内存中间的buffer的状态更新为 unpinning 的状态，当 buffer 中间的所有的数据都是处于 unpinning 的状态，那么可以释放对应 journal 中间的数据。
值的注意的是，由于linux的buffer head 的设计，将数据从journal 到 主数据区的拷贝只需要对于 buffer head 中的磁盘号进行调整即可。

> 1. pin 的作用是什么 ?
> 2. dependent data


之前解决fs consistency 的方法是什么 ? 遇到的问题是什么 ?
1. 在进行一次crash之后，失败的模式需要是可预测的，一般情况下，这要求当一个更新操作修改多个block 的时候，写入磁盘顺序也是可预测的。最简单的实现方法是等待上一个操作完成，然后进行下一个操作。
2. 第二种方法保证原子性的更新

深入理解 transaction :
1. 读操作也是transaction 的一部分
2. 顺序要求不仅仅出现在 metadata 的更新上，而且也出现在普通数据更新上。
3. 由于为了提高性能，在一个 compound transaction 没有完成的情况下，可以开始下一个 transaction。当新的 transaction 需要修改老的 transaction 的buffer，是不能直接修改的，而是需要创建一个备份出来。

启发和想法:
1. 作者指出文件系统的设计空间(不考虑nfs之类的)包括:数据在磁盘中间的布局，内部缓存以及IO 调度，难道后面两者的工作不都是交给 block layer 完成的吗 ?

专题分析1:
1. transaction 的概念

A central concept when considering a journaled filesystem is the transaction, corresponding to a single
update of the filesystem. Exactly one transaction
results from any single filesystem request made by
an application, and contains all of the changed metadata resulting from that request.

在 Anatomy of a transaction 这一节，其中说明了上面描述的定义，同时，说明了 transaction 两个:


专题分析2:
1. 如何从错误中间恢复

如果在journal 拷贝的过程中间 crash ?


专题分析3:
journal 中间到底存在什么东西 ?

在 Format of the filesystem journal 中间描述的非常清晰，但是 metadata 指的是 ?

Log-Structured Filesystems achieve the same end by writing all filesystem data−both file contents and
metadata−to the disk in a continuous stream (the ‘‘log’’)
> metadata 和 file contents 是区分开来的，所以

merged transaction 是不是总是收集完毕之后，才会进行拷贝 ?

专题分析4: 为什么说 transaction 是可以实现操作的 aotmic 的
Transactions are atomic because we can
always either undo a transaction (throw away the
new data in the journal) or redo it (copy the journal
copy back to the original copy) after a crash, according to whether or not the journal contains a
commit record for the transaction. Many modern
filesystems have adopted variations on this design.

#### F2FS: A New File System for Flash Storage
f2fs 是专门针对SSD、eMMC、UFS等闪存设备设计的文件系统。 F2FS虽然基于通用块设备层接口实现，但并不像通用文件系统一样无差别的对待机械磁盘和闪存盘，在设计上是”flash-awared”。
根据闪存内部结构和闪存管理机制(FTL)，F2FS可通过多个参数配置磁盘布局、选择分配和回收单元大小从而适配不同实现的闪存存储设备。本文的介绍f2fs核心设计思想，包括空间布局，文件结构，目录文件结构，清理日志，恢复等基本。

f2fs 的文件的结构和目录结构和ext4等没有特别大的区别。目录文件，其使用 bitmap 描述 dentry 是否有效，使用hash来实现快速查询。

f2fs 的空间布局分为一下几个部分:
1. 超级块(Superblock) : 基本分区信息以及格式化分区的时候不可修改的信息。
3. 主区域(MainArea) : 由4KB大小的数据块组成，每个块被分配用于存储数据（普通文件的内容或目录文件的内容）和索引（inode或数据块索引）。一定数量的连续块组成Segment，进而组成Section和Zone（如前所述）。一个Segment要么存储数据，要么存储索引，据此可将Segment划分为数据段和索引段。
4. 段摘要区(Segment Summary Area)(SSA) : 保存MainArea 数据块(数据和索引)的归属信息，用于确认该数据块的数据是否有效。
5. 段信息表(Segment Information Table)SIT）: 每个段的有效块数和*标记块是否有效的位图*。
6. 索引节点地址表(Node Address Table)（NAT）：用于定位所有主区域的索引节点块(node blocks)（包括：inode节点、直接索引节点、间接索引节点）地址。即NAT中存放的是inode或各类索引node的实际存放地址。
2. 检查点(Checkpoint) : 有效 SIT 和 NAT 的bitmap，文件系统的状态等

使用NAT，保存索引节点块的地址的原因是: 访问一个大型的文件的数据，可能首先需要访问经过多级的间接索引，
那么修改该数据，首先需要写入该数据到日志头，然后逐级反向更新间接索引节点，直到inode 节点，就像是"滚雪球"一样。
由于NAT的存在，数据和各级索引节点之间的“滚雪球效应”被打破。

Cleaning 是基于日志的文件系统的关键一环，f2fs 的GC 分为 foregrounp 和 backgrounp 两个部分。foregrounp 触发的原因是没有足够的磁盘空间了，所以其要求尽可能快速的完成，
Cost-benefit算法主要用于后台GC，综合了有效块数和Section中段的年龄（由SIT中Segment的更新时间计算)。对于冷数据进行搬移，热数据由于可能在接下来的时间中间更新，所以无需搬移。
确定需要被移动的segment之后，将相关数据块读入页缓存并标记为脏页交由后台回写进程处理。当一个 segment 的有效块被清理之后，那么其处于 pre-free 的状态，而当一个CheckPoint被建立之后，
那么该segment就可以被真正释放。

Multi-head logging : 按照将数据使用频率划分为hot warm 和 cold 三种情况，按照数据类型每种划分为data block  和 node block，一共存在六种。对于每一种情况，单独建立日志写入的位置，从而实现负载均衡。

threaded logging : normal logging 是选择干净的 segment，并且按照顺序的写入，但是当空闲segment的数量减少到一定的程度的时候，这种写入方法引入很高的Cleaning的overhead。而threaded logging 将数据写入到segment 的空洞之中，
也就是invalid 或者 outdated 的block中间。

建立检查点的过程:
1. 将所有的dirty数据从page cache中间flush出去
2. 暂停通常的写入操作
3. 将文件系统的元数据，NAT，SIT，和SSA写入到对应的位置。
4. 将checkpoint pack 写入到 Checkpoint 区域(回顾空间布局的内容)
其中 checkpoint pack 的包括的内容为:
1. Header 和 footer : 位于pack 的首尾，保存版本号
2. NAT and SIT bitmaps : 记录和现有pack相比，NAT 和 SIT 的 block 的修改。
3. NAT and SIT journals : 为了防止对于NAT 和 SIT的频繁更新，用于保存对于NAT 和 SIT 的少量修改。
4. 即将写入到SSA 区域的 SSA block。
5. 孤儿inode : 当一个inode 在关闭之前就被删除了，那么该节点就是孤儿节点，那么该节点。

f2fs 存在两种恢复方式:
1. 后滚恢复(roll-back recovery) : 当发生断电之类的错误的时候，f2fs 会采用恢复到最近的一致的
2. 前滚恢复(roll-forward recovery) : 部分应用经常使用fsync，但是fsync 会触发checkpoint 的产生，为了消除产生的overhead，采用的办法是进行在日志中间写入数据以及直接关联的索引。

启发和想法:
1. FTL 的提供的模拟传统磁盘的虚拟层次，对于log structured fs，这种模拟似乎是一种浪费，会不会出现越过FTL的文件系统 ?

专题分析1: 按照 introduction 部分一共讲解了什么内容 ?


专题分析2: 空间布局是什么样子的 ?
A section is comprised of consecutive segments, and a zone consists of a series of sections
> 1. segment 一直都是分析的基本单位，zone 和 section 的作用是什么 ? 据说，一般三者都是一样大的

Main area 是什么 ? 放置数据，分为 node 和 data 区间，
Note that a section does not store data and node blocks simultaneously.

为什么需要 NAT ? Checkpoint 保存的是有效的 NAT 的数据，在什么时候 NAT 的数据会被无效掉 ?
在描述查找过程的时候，为什么总是通过 NAT，当获取 inode number 就可以找到 inode，是不是 NAT 就是 inode map 强化版本，同时持有其他的 node 类型，如果混合有其他的 node 类型，如何找到数据的来源 ?

The original LFS introduced *inode map* to translate an inode number to an on-disk location.
In comparison, F2FS utilizes the “node” structure that extends the inode map to locate more indexing blocks.

A direct node block contains block addresses of data and an indirect node block has *node IDs* locating another node blocks.
> 为什么可以解决 tree wandering 的问题， 因为 indirect 中间存放的是 node id 的内容
> 当其中 apppend 一个数据: direct node 首先需要更新，因为其中持有的 data block 编号变化了，但是 indirect 不需要修改，因为其持有的数据 node number，而更新 direct node 只是更新 node number

好的，现在解决了 NAT 和 SSA 的问题。

CP 中间的 checkpoint 的 summary entry for currently active segment 是什么东西啊 ?
> 在2.7中间，分析 CP 的内容，其实就是暂存一下以后会写入到 SSA 的内容，


SIT 的作用是什么: 如果其持有了 segment 中间的有效的 block 的　bitmap，那么为什么还需要 SSA 啊 ?
只能说，SIT 的作用没有详细分析。


专题分析3: 如何体现了其中的 flash 的属性 ?
没有想到吧，居然是在 multiple head logger 中间分析的，

FTL algorithms are largely classified into three groups (block-associative,
set-associative and fully-associative) according to the associativity between data and “log flash blocks”

虽然不是很懂，具体的含义，大概是想要表达，其中的数据总是，其中，


专题分析4: Recovery
请问 Roll-Forward Recovery 和 fsync 存在什么关系，不就是减少了写入的内容吗 ?

In order to find the data blocks selectively after rolling back
to the stable checkpoint, F2FS remains a special flag inside direct node blocks.

fsync 的 naive 操作是，建立 checkpoint，然后 roll-back 的，而 roll-forward 的操作指的是，fsycn 的时候，仅仅写入 data 和 direct 到磁盘，其余的在内存中间。

special flag 是什么东西 ? 似乎找到那些在 roll-back 依旧被选中的数据

1. F2FS collects the *direct node blocks* having the　special flag located in N+n, 

专题分析5: 为什么需要冷热分离 ?


## Memory Management
#### Practical, transparent operating system support for superpages
讲解superpages 的设计方法。

设计中间主要处理的问题:
1. 分配 superpage : 处于性能的考虑，倾向于预先分配特定大小的物理页面。
2. promotion 和 demotion : 动态升级和降级 superpage 的大小。
3. fragmentation 控制 : 使用不同的大小的page size 会造成内存碎片化

本文的四个贡献:
1. 将基于保留的方法拓展到不同大小的superpage，并且证明这样做的好处。
2. 分析 superpage 对于 fragmentation 的影响
3. contiguity aware page replacement algorithm 来控制 fragmentation
4. 分析superpage demotion 和 dirty page 写回的问题

Pages in the cache list are clean and unmapped and hence can be easily freed under memory pressure.
Inactive pages are those mapped into the address space of some process, and have not been referenced for a long time. 
Active pages are those that have been accessed recently, but may or may not have their reference bit set.

主要设计思路:
1. Reservation-based allocation : 如果想要使用 superpage，需要预留出来对应大小superpages，而不是进行想要升级为更大的superpage，在其他位置寻找何时的大小，然后进行拷贝。
2. Preferred superpage size policy : 对于大小确定的memory object，比如代码段，在不超出范围情况下的，分配最大的 superpage，对于大小不固定的memory object，例如 heap 或者 stack，尽可能大的 superpage，但是大小不超过 memory object 的大小。
3. Preempting reservations : 当内存资源不足的时候，操作系统可以抢占superpages的reservation
4. Fragmentation control : 使用contiguity-aware page deamon来实现，所谓contiguity-aware就是修改原来的page daemon，让其显示的处理连续性的问题。
5. Incremental promotions : promotion是逐步进行的，并且只有当superpage中间的全部base page被使用之后，才可以进行promotion
6. Speculative demotions: demotion和promotion类似，不是一下子将superpage全部拆分为base page，而是逐级拆分。superpage虽然持有多个base page，但是硬件只是提供了一个reference bit，所以没有办法知道base page的各自的活跃状态，采用的办法是speculative的将superpage拆分开
7. Paging out dirty superpages : 如果superpage中间仅仅是一个base page是dirty的，就需要全部写回，一种解决办法是，要写入的时候，将superpage拆分，当所有的base page都是dirty的时候，才可以合并。一种计算每一个base page的160bit SHA-1，对于SHA-1没有变化的不要写回。
8. Multi-list reservation scheme : 当从buddy system中间分配内存失败，那么从superpage的reservation中间获取。相同大小的reservation被放到同一个链表中间，并且按照其所在的superpage被访问的时间排序。
9. Population map : 在一个superpage中，利用population map来维护一个memory object中的base page是否被分配的信息，利用population map辅助page fault，promotion, demotion以及overlap detection.

添加两个数据结构 : reservation list 和 population map，分别用于preemption和记录memory object中间的base page使用情况.

superpage 对于 FreeBSD's A-LRU 的修改:
1. Contiguity-aware page daemon : 一共划分为三种 page，做出的修改为 : 将cached page 用于 reservation，page daemon 的启动时间，获取更多的 inactive page。

2. Wired page clustering : 将 non-pageable 的 page 合并在一起，然后处理
3. Multiple mappings : 不就是让虚拟地址对齐吗，和multiple mappings 有什么关系吗？


启发和想法:
2. superpage 的使用需要显示的指出吗，还是操作系统默认的?
3. 使用不同大小的superpage 会导致了内存碎片化，为什么 ? 
1. fragmentation 控制 : 所谓的 contiguity-aware page daemon 应该没有说的那么神奇，
4. dirty page 写回被作者不了了之了

专题分析1: superpage 到底是需要在 fragmentation 维持性能，还是 superpage 自身会导致 fragmentation 问题加剧 ?

Contiguity-aware page daemon

page daemon 的作用，类似于linux kernel 的 vmscan 的作用:
We made the following changes to factor contiguity restoration into the page replacement policy.
1. We consider cache pages as available for reservations



## Multicore

#### An Analysis of Linux Scalability to Many Cores
使用7个程序进行测试，然后分析性能瓶颈，最后从用户态和内核态做出修改。

选择测试程序的原因是什么?
选择的标准是测试程序尽可能利用各种内核服务，一类程序在内核上并发度出现瓶颈，一类程序是天生并行的，并且kernel intensive 的

对于内核进行了那些优化 ?
1. 多核网络包处理 : 在内核中间，一个网络包会经过多个队列，最后到达每一个socket专用的队列中间，为了减少cache miss 和 lock contension，让package，queue，socket都是一个core处理的，而不要切换到core.
2. slobby counter : 在多数情况下都是进行本地更新计数器，当本地更新到达一个特定的阈值的时候才会访问共享计数器
3. Lock-free comparison : 文件访问需要经常比较dentry，一种简单的方法是首先访问之前首先上锁，而 lock-free comparison 的方法是，当修改的dentry的时候，将generation number设置为0，而将dentry拷贝到本地，进行比较的时候，总是首先比较 generation number，如果为0，使用原先的锁机制。
4. percpu 数据结构 : 其实就是 slobby counter 更加一般的，将一个数据结构，拆分为每一个core一个，由于局部性，可以保证大多数时候，只是访问local的。
5. 消除假共享 : 需要被频繁修改的数据位于同一个cache line，可以调整数据分布，将数据分开。
6. 消除不必要的锁


分析的内容划分三个部分:内核，用户，用户使用内核调用的方法。

产生并行瓶颈的集中情况:
1. 多个进程争用同一个锁
2. 多个进程写同一个内存区间，由于 cache coherence 协议而性能下降
3. 进程争用有限的cache，导致cache miss rate 增加.
4. 进程争用核间的互联网络带宽以及内存访问带宽
5. 无法让所有的core 处于运转

得出的结论: 只需要使用常规的并行化技术就可以消除大多数并发瓶颈，现有的内核架构没有必要进行修改。

启发和想法:
1. 目前对于内核网络缺乏理解，可以进一步分析一下网卡和内核是如何配合使用的。



## Techniques
#### Information and Control in Gray-Box Systems
通过在内核和用户之间插入一个ICL(Information and Control)层次，在不修改内核的情况下，提升用户程序的性能。

Gray Box 的核心在于插入ICL，也就是 获取Information 和 施加Control.
1. Information Techniques 包括的手段:
    1. 获取内核使用的算法。
    2. 使用 microbench 获取系统的参数。
    3. 当用户层和内核的交互较少，可以让ICL 主动添加探测
    4. 监控输出。之所以不监控输入的原因是，监控输入需要监控所有进程的输入。对于输出还可以使用统计的方法。
2. Control Techniques 包括的手段:
    1. 将系统迁移到一个可知的状态 : 例如通过写一个虚拟地址，在进行该操作前，其对应的物理页面有可能处于被换出，有可能还没有被page fault 过，不同的状态，进行访存延迟测试的结果不同。
    2. 通过反馈强化行为的 : 例如读写文件的顺序会决定文件缓存，当文件IO 通过ICL，ICL 可以让缓存的内容更加可预测，而可预测的缓存让进一步的访问文件更加可控。

比如: microbench 来获取系统基本参数。TCP利用message 是否drop来确定网络的拥塞程度。当一个进程处于运行状态，会立刻回复消息，通过回复的时间判断远程节点是否在运行。

案例分析: 操作系统为了给进程提供一个内存无限大的假象，会利用swap，但是过度超额使用物理内存，其实会降低性能，可以在分配物理内存的时候，首先探测操作系统中间是否存在重组的内存，探测的方法首先对于需要使用的内存进行两次访问，第一次访问是初始化状态，第二次访问通过访存时间来确定需要的内存是否存在。

根据三个案例，一个 gray box ICL 需要具备的基本功能是:
1. 利用 microbench 获取基本参数
2. 输出响应时间测量 : 测量时间需要足够精准，并且最好引入的overhead 可以忽略不计。
3. 输出内容分析工具

启发和想法:
1. 文章建立在无法对于内核无法施加修改的基础上，但是其中分析的三个案例，确定file cache 的内容，文件在磁盘的结构，以及测量物理内存的数量，使用gray-box添加大量的overhead，并且是一种推测的方式获取的，结果并不精装，正确的操作方法是应该添加内核驱动直接访问所需要的信息。

专题分析1:
为什么说是gray-box ?
When treating the operating system as a gray-box, one recognizes that not changing the OS restricts, but does not completely obviate, both the information one can acquire about the internal state of the OS and the control one can impose on the OS. 
The thesis of this paper is that a surprisingly large class of "OS-like" services can be provided to applications without any modification to the OS itself.
Experienced programmers tend to exploit their knowledge of the behavior of the underlying system; we believe that this knowledge should be encapsulated in ICLs, so that these techniques can be used by all programmers.

专题分析2: 
gray-box knowledge : 前面详细的说明为什么
We begin by assuming only the coarsest level of algorithmic knowledge:
when the buffer cache for files is full, some page must be replaced in order to fit a new page.

Our hypothesis is that we can predict the presence of a file (or
part of a file) within the file cache by timing a few carefully
selected file-cache probes, where a probe is a read() of a single byte of a page within a file.
> 通过返回的快慢来实现分析数据是否及时到达

probe 必须是稀疏的，cost 和 state

In Figure 1, we demonstrate this **relationship** by plotting the correlation between the presence of a random page
in a prediction unit (i.e., a contiguous region of the file) and the percentage of the unit within the cache.
> 一个 page 在 unit 的概率和 unit 在 cache 中间的比率
> 似乎后面根本没有提到 relationship 是如何被利用的

the access unit is the amount of data that the application reads sequentially after randomly picking an offset in the file.
> 为什么需要强调顺序访问

**区分 predict size 和 access size 是什么 ?**
一个预测的，一个是访问的

图的含义 : 一个物理页面在 prediction unit的时候，prediction unit 在 page cahce 的概率

From the graph, we can see that when the prediction unit is less than or equal to the access size,
the presence of the probed page is highly correlated with the presence of the entire prediction unit.
> 如果 prediction size < access size，
> 是否可以认为 prediction size 是需要的
> presence of the probed page : 可以认为是，当探测的页面存在，那么 prediction unit 必然存在，
> 虽然两个 unit 的开始位置都是随机的



实现细节提出的三个问题:
In our implementation section, we discuss how with use of the FCCD,
the prediction unit can be made smaller than the access unit, as desired.
> 不是说好的，当 prediction unut 的大小比 access unit 的时候，难道不就是会产生，不就是产生100%的覆盖了吗 ?

一般的使用过程:
1. 文件访问
2. FCCD 返回可能在 file cache 的内容
3. 用户程序改变访问的优先级


gbp : 在文件没有访问之前，首先提交

Our implementation must
address three problems: 
1. how to differentiate between probe times that are in cache and out of cache, (获取访问 cache 和时间差别)
2. the amount of data the application should access as a unit. (**为什么application 需要按照 unit 的访问数据**)
3. and the number of pages whose state is predicted from a single probe. (**通过一个 probe 就可以推测出来状态的页面的数量**)

在分析第三个问题，prediction unit，其含义应该指的是，对于一次 probe，那么可以预测的范围是什么 :
We have already shown that picking a prediction unit that is smaller than the access unit of the
application is sufficient for high prediction accuracy.
所以，该问题变为如何才可以确定，预测的大小!
之前的图说明过，只要是 access size 内，那么就可以在可以直接访问。

**However, we have found that performing a few probes within each access
unit is slightly more robust**, and therefore currently use a
prediction unit of 5 MB. Thus, our gray-box layer probes
four points within each default access unit, measures the
time of each probe, and sorts the access units by the total
time for its four probes. The overhead of the probes is negligible; measurements reveal probe time for in-cache data in
the realm of a few microseconds, and a few milliseconds per
probe for out-of-cache data, which will likely be amortized
by the entire file access time. Files smaller than 5 MB in
size are probed exactly once.
> 如果对于 access unit 多进行几次 probe，结果会更加稳定 （robust 指的是
> 多次指的是 4次，对于 20M 的(**什么东西，对于其中的四个位置进行探测**)
> 如果文件小于 5M，只是进行一次探测
> 做了这么多分析，其实只是想要表示，如果一个大块的内存被访问过，只要预测的范围不超过访问范围，
> 那么就表示其中的范围和探测的结果的范围相同


We have also found that the method for choosing a probe
point within a prediction unit is important.
> 不仅仅是大小，而且探测的位置也非常重要
One approach is to select bytes at predetermined offsets; however, if a process
*terminates after the probe phase but before the access phase*,
or if two processes probe the file-cache for the same file at
nearly the same time, then the second set of probes will
return bad information, indicating that all pages are likely in
the file cache. Our solution is to probe a random byte within
the prediction unit. This method is robust across runs and
has the added benefit that an application can probe the file
cache repeatedly for increased confidence.
> 一种方法是固定大小位置，
> probe 的位置
> 探测的位置 : 在 prediction 中间随机探测



single-file scan:

*instead, this scan sequentially accesses segments of the file in the
size directly determined by the access unit*. The effect of
running the application multiple times is an example of the
control technique of positive feedback; by accessing the file
in access-unit sized chunks, it is likely that access-unit sized
chunks will be present in the cache.
> 不是顺序的访问文件，而是按照 access unit 进行访问的
> 如果按照 access-unit 的大小访问，那么 access-unit 大小的文件应该会出现在 cache 中间

single file 的访问，如果是按照传统的访问，对于一个文件顺序访问，由于其访问是LRU机制
> 虽然原理上，其总是选择在 page cache 中间的访问，但是有什么区别啊!
> 而且这个图，LRU的方式访问的，为什么瞬间拔高到
> 超过了 file system 的内容之后，所有的文件就是都需要从文件系统中间获取

Figure 2 的图的注释说明一下:
1. 这个图描述的是在 warm cache 的情况下，对于其中的

From the figure, we see that the traditional scan suffers a large
performance decrease when the size of the file exceeds the
size of the file cache.
> file cache 的大小是什么时候确定的 ?

multiple file scan:

The basic premise is that file blocks and the meta-data from files in the same directory are likely
to be accessed together and thus FFS tries to place them
together in the same cylinder group (i.e., a few consecutive
cylinders on the disk). Based on this algorithmic knowledge
of FFS, a simple heuristic to reduce seek time is to group
each set of files by directory name and then access them in
this order [37]
> 同一个文件夹中间的文件可能被连续访问
> ICL希望将其放在一起
> 在同一个文件中间，



#### Fine-Grained Dynamic Instrumentation of Commodity Operating System Kernels
将想要执行的代码段动态插入制定的位置从而实现debug之类的功能，核心技术在于将插入的代码和原代码连接起来。

大致内容: 描述动态代码插入的技术KernInst，其中关键在于如何连接原代码和插入的新代码，说明为何只能复写一条指令以及处理跳转指令范围的问题。
然后利用KernInst分析内核性能瓶颈以及web代理服务器的功能。最后分析这个技术带来的安全问题以及解决办法。

1. 存在那些应用场景 ?
  1. 性能测试
  2. 代码分析: code coverage, kernel tracing 和 kernel debugger
  3. 自动动态代码优化
  4. 安全检测
  5. 动态修改代码功能
2. 实现动态插入需要具备那些条件 ?
    1. 存储需要插入的代码的heap
    2. 内核符号表
    3. 修改内核代码段的权限
4. 进行 instrumentation 的步骤
    1. 找到在该指令位置的live register
    2. 生成插入代码
    3. 将被重写指令进行重定位，并且添加跳转回来的指令
    4. 插入代码段，并且利用 springboard 解决跳转长度不够的问题

3. 为什么需要对于内核代码进行结构分析 ?
> 分析的内容为: 利用内核符号表确定所有的函数，将函数中间的basic block 找到，然后进行过程间live register分析，确定每一个basic block 前面的live register
> 原因: 无法安全的使用 save 和 restore 指令。 On the SPARC, this involves emitting save and restore instructions. Because these instructions cannot safely be executed within the trap handlers for register window overflow and underflow, kerninstd cannot instrument these routines

5. fine-grained 体现在何处 ?
> 因为其几乎可以在任何指令的位置插入代码

6. 存在那些安全问题，解决方法是什么 ?
> 主要的安全问题不在于插入过程的影响，而是在于插入的

专题分析1: 如何利用 kprobe 实现 webserver 的性能提升的 ?

专题分析2: 到底需要那些条件 ?
To instrument a running kernel, kerninstd needs to allocate the patch area heap, parse the kernel’s runtime symbol table, and obtain permission to write to any portion
of the kernel’s address space.

Kerninstd cannot allocate kernel memory, so it has /dev/kerninst perform the necessary kmem_alloc via an ioctl.

1. 启动 : 初始化 /dev/kerinst 和 kerninst，说明是如何解决 patch code 如何传入到内核，再拥有权限写入内核的代码段
2. 结构分析 : 确定活跃寄存器，让生成的指令不要使用在插入的上下文中间的活跃寄存器
3. 代码生成
4. 代码插入 :


Splicing at the delay slot of a control transfer instruction is difficult because the branch to the code patch will occur before the control transfer instruction has changed the PC. 
> delay slot 会非常的麻烦，前面分析过，如果替换掉的跳转指令，那么需要将delay slot 中的指令带着。
> 这里分析是如果一不小心替换的指令是 slot，在 code patch 的可能先执行，而当 code patch 返回的时候，其不能跳转到下一条指令中间，而是跳转到 branch instruction 指向的位置。
> 但是如果 branch instruction 是条件跳转，patch code 返回的位置有两个。
> 措施就是，让将 control transfer instruction 放到代码结尾，然后跳转到 slot 的下一个指令。
> 但是，对于Solaris kernel，其跳转位置，

A delay slot instruction is the target of a branch, and thus is not always executed as the delay slot of the preceding
control transfer instruction. Kerninstd does not instrument these cases since a code patch would have to choose from two different instruction sequences for
returning. This case is detected by noticing a delay slot instruction at the start of a basic block.


Note that instructions whose semantics are PC-dependent, such as branches, cannot
be relocated verbatim to the code patch. In these cases, kerninstd emits a sequence of instructions with combined semantics equivalent to the original instruction.
> 替换的指令如果是 PC-dependent 的，那么需要小心处理
Patch code ends with a jump back to the instruction following the instrumentation point. 
> 需要考虑的第二个问题，如果替换掉正好是
If a single branch instruction
does not have sufficient range, a scratch register is written with the destination address and then used for the
jump. Since this jump executes after the relocated
instruction, an available scratch register must be found
from the set of registers free after the instrumentation
point. This contrasts with instrumentation code, which
executes in a context of free registers before the instrumentation point. If no integer registers are available,
Kerninstd makes one available by spilling it to the
stack4. Kerninstd generates the relocated instrumentation point instruction and the returning jump in 36 µs.
> **第三个问题，利用指令跳转的范围不够，那么利用寄存器**

专题分析3: 使用单条指令的好处到底是什么 ?
如果第一条指令天魂，

专题分析4: 为什么 kprobe 一劳永逸的解决问题 ?


## Virtual
#### Memory Resource Management in VMware ESX Server
当一个host中间同时运行多个虚拟机的时候，给每一个虚拟机分配的内存之和可以超过实际的物理内存。为了实现内存资源利用率的最大化，
本文介绍了ballooning，idle memory tax，Content-based page sharing，hot I/O page remapping等技术，从分析了虚拟内存，页面回收，共享内存方面加以分析。

虚拟内存的实现:  
1. ESX Server 利用pmap 数据结构实现从guest 的物理地址到host 的物理地址的装换。
2. guest 用于操作page table 和 TLB 的指令会被截断，防止直接更新host 的 MMU 状态。
3. 使用 shadow page tables 来实现从 guest 的虚拟地址直接到 host 的物理地址，减少访存的 overhead.

Vmware ESX Server 的四项技术的基本含义:
1. ballooning : 用于回收在虚拟机中操作系统不常用的内存。
2. idle memory tax : 不同host对于内存的使用情况不同，如果一个host 的idle memory 的比率更高，那么其就更加应该将内存共享出来。
3. Content-based page sharing : 将不同guest操作系统中含有相同的内容的物理页面进行共享。
4. hot I/O page remapping : 将I/O 关联的 page 动态地重新映射DMA zone 上。

ballooning: 将物理内存写回到二级存储上，可以让host决定，但是host 掌控的信息并补充分发，而且可能出现host 和 guest 将同一个页面两次写入到相同的位置。ballooning 是添加一个内核驱动，当需要从guest 回收内存的时候，那么
让 balloon 膨胀，让 guest 产生内存不足的"错觉"，利用guest 原生的page reclaim 机制进行回收内存。

Content-based page sharing: 在同一个host 上运行的不同的guest 往往会拥有相同的程序以及数据，对于这些内容共享可以减少对于host memory 的使用，采用的方法有两种:
1. 透明页面共享，将多个guest内容相同的"物理页面"映射到host 同一个物理页面，这种方法需要对于操作系统进行多出修改。
2. 基于内容的共享，将物理页面的内容hash，当两个页面的hash数值相同的时候，再进行内容比对，然后利用cow实现页面共享。

idle memory tax : 首先定义 shares-per-page ratio，其可以理解为价格，内存会从价格低的 guest 移动到价格高的 guest 中间去。
```
shares-per-page ratio = client's share / [allocation pages * (active fraction + idle page cost * (1- active fraction))]
idle page cost = 1 / (1 - tax rate)
```
从上面的公式，当 tax rate = 0 的时候，idle page cost 最小, shares-per-page 就是简单的 client 的 share 平分给其所分配的page.
当 tax rate 接近 1 的时候，idle page cost 趋于无穷大，如果分配的页面中间的idle page 比率会迅速降低其 shares-per-page，表示该client的 page 重要性不高，可以被换出。
tax rate 一般被设置成为0.75，那么如何测量 idle memory 的数量? 之所以不采用操作系统的接口来获取 idle memory ，idle memory 在不同的操作系统中间的标准不同，其次无法处理DMA内存，
所以采用统计的方法，并且结合三种数据，这样可以让需要内存的 guest 迅速补充，而持有大量 idle memory 的guest 的内存数量缓慢下降。

hot I/O page remapping : 由于PCI总线只有32bit等原因，DMA只能访问低4G的物理地址，所以，在high memory 中间反复涉及到IO操作，那么可以将该地址映射到低地址区间。

启发和想法:
1. 本文只是着重分析了内存资源管理，但是关于如何减少由于虚拟化带来的访存延迟以及访存带宽下降被没有仔细分析。
2. 可以了解一下KVM的技术以及Virtual Box的实现，对比在内存管理上使用技术的差别是什么。
3. 尝试分析一下VMware在其他方便虚拟化的技术，理解实现虚拟化的基本要点是什么。



#### An Updated Performance Comparison of Virtual Machines and Linux Containers

内容简介：
本文通过对比无虚拟化，虚拟机(主要是KVM)，以及容器在各种测试下的性能指标，分析虚拟机和容器带来的性能损耗。

必要的背景介绍
1. 为什么需要 cloud virtualization : 配置冲突和资源隔离
1. KVM 是 type-1 hypervisors，也就是直接和硬件交互的
3. KVM 同样的可以利用硬件进行加速。
4. VM 和外面交流的唯一方法是通过有限的hypercall 和 模拟设备
5. Containers 之间很容易共享资源。


2. 测试项目以及测试软件是什么 ?
  - compute-PXZ : docker 无性能下降，KVM 无论是否提供cache topology，都存在 20% 作用的性能下降。
  - HPC-Linpack : docker 无性能下降，KVM 在提供cache topology 后，几乎无性能下降，否则 17% 性能下降。
  - memory bandwidth-STREAM : 两者都无性能波动。
  - memory latency-RandomAccess : 无差别
  - Network bandwidth—nuttcp : 就传输速度而言，都没有差别，区别在于在进行全速传输的时候，对于CPU的消耗的多少。Docker在没有添加NAT的时候，和native的性能几乎没有区别，否则存在明显的CPU消耗量增加。KVM利用 vhost-net，其在发送时性能损失小，但是接受性能损失很大。
  - Network latency—netperf : 添加了
  - Block I/O—fio : 访问顺序差别不打，但是随机KVM下降明显。
  - Redis && MySQL : 

3. 基本结论是什么 ?
对于CPU和memory，KVM 和Docker几乎不会增加overhead，但是对于IO，网络等会增加开销，粒度越小，开销越大，开销体现的形式为CPU使用率和延迟。
KVM需要小心的配置large pages, CPU model, vCPU pinning, and cache topology 等才可以发挥其全部的性能。
Docker的配置，例如是否增加NAT，会对于性能影响很大。
通过上述的结果，传统观点认为IaaS部署在VM上，而PaaS部署在容器中间，单单从性能角度来说，IaaS应该都是部署在容器上，而且容器还有其他的好处。
虽然KVM的性能一直在提升，但是overhead依旧不可忽视。容器在性能上没有问题，而应该在convenience, faster deployment, elasticity 上下功夫。

疑问:
We expect other hypervisors such as Xen, VMware ESX, and Microsoft Hyper-V to provide similar performance to KVM given that they use the same hardware acceleration features.
> 关于这一个论述，并不相信。

补充知识:
1. KVM 和 QEMU 的关系是什么 ?
> KVM 是内核模块，负责控制CPU和内存的访问，QEMU 可以提供完整虚拟化环境来执行各种程序，QEMU可以借助
2. type-1 和 type-2 hypervisors 的区别
> 是否直接和硬件交互，还是借助操作系统。

启发和想法:
1. 部分测试出现性能损失，但是作者并没有分析，可以深入分析一下。
2. 采用其他常用的类似于 Redis 和 MySQL 的程序进行测试分析。
3. 猜测本文中间KVM是基于相同架构，可以进行跨架构的测试。
4. 原来发文章这么简单，搞几个程序跑一下就可以了。

## Process

#### A fork in the road
fork 曾经是一个天才设计，但是对于现在的操作系统以及硬件，其弊端显现，如何才可以替换掉 fork 机制。

为什么说 fork 曾经是天才设计 ?
1. 早期的程序和内存较小，访存相对于CPU频率差别不太大
2. fork 提供了容易理解的抽象，而且易于使用(fork 的调用无需参数)
3. fork 让 concurrency 易于实现，但是还不存在 async IO 以及 thread
4. fork 和 exec 之间存在操控空间

如今fork 存在那些缺点 ?
1. fork 如今变的非常复杂(POSIX 提供了25种拷贝控制)
2. fork 不是线程安全的。当parent是一个多线程，fork只会对于所在的 thread 进行。
3. fork 不安全。fork可能会继承不该继承的东西，除非显示的设置。
4. fork 性能不好。在如今的程序中间，单单是建立 copy-on-write mappings 就需要 100m
5. fork 可扩展性(scale)不好。具体含义参考:The scalable commutativity rule: Designing scalable software for multicore processors. 
6. fork 鼓励 memory overcommit。copy-on-write page mappings 会消耗内存，同时保守的策略要求首先具备备用的内存才可以。

fork 的限制 ?
1. 由于 fork 是拷贝 parent 的地址空间，在一个地址空间中间，fork 无法让多个进程运行。
2. fork 无法和异构的硬件良好的交互。
3. 任何子模块都需要和考虑 fork，其将整个系统牵连在一起。

fork 之所以难以清除的原因是什么 ?
fork 不仅仅是一个系统调用，而是一个贯穿于内核到用户层的设计思路，替换掉fork 需要导致大量的应用程序重新设计。

替换掉 fork 的方法是什么 ?
1. 上层，使用 spawn 机制，但是对于一些特殊的情况，spawn 无法处理.
2. 下层，让一个process 来初始化另一个 process 的方法创建进程。

启发和想法:
1. posix_spawn() 还是依赖于 fork 实现，为什么可以比 fork 性能高。
2. 可以在现有操作系统上添加 Cross-process operations 的功能吗 ?
3. 现在的其他的操作系统都是如何创建进程的 ?
