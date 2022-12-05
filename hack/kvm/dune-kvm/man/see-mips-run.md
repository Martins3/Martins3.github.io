# RISCs and MIPS Architectures
1. pipeline
The MIPS architecture was planned with separate instruction and data
caches, so it can fetch an instruction and read or write a memory variable simultaneously

2. five stage


IF (instruction fetch) Gets the next instruction from the instruction cache
(I-cache).

RD (read registers) Fetches the contents of the CPU registers whose numbers are in the two possible source register fields of the instruction.

ALU (arithmetic/logic unit) Performs an arithmetical or logical operation in
one clock cycle (floating-point math and integer multiply/divide can’t
be done in one clock cycle and are done differently, but that comes
later).

MEM Is the stage where the instruction can read/write memory variables in
the data cache (D-cache). On average, about three out of four instructions do nothing in this stage, but allocating the stage for each instruction ensures that you never get two instructions wanting the data cache
at the same time. (It’s the same as the mushy peas served by
Dionysus.)

WB (write back) Stores the value obtained from an operation back to the
register file

3. The differences


# MIPS Architecture

## Registers
`$31`Is always used by the normal subroutine-calling instruction (jal) for the
return address. Note that the call-by-register version (jalr) can use any
register for the return address, though use of anything except $31 would
be eccentric.

## Integer Multiply

## Basic Address Space
A MIPS CPU runs at one of two privilege levels: user and kernel.

# Coprocessor 0 : MIPS Processor Control
1. CPU configuration
2. Cache Control
3. Exception/interrupt control
4. Memory management unit control
5. Miscellaneous: There’s always more: timers, event counters, parity error detection

> 为什么需要单独设置C0 出来，Intel对应的策略是什么 ?
> 哪一个策略更加好

- CP3 has been invaded by floating-point instructions from MIPS32/64 and is now only usable where you are sure you’ll *never want to implement floating point*.
- CP2 is available and occasionally used for custom ISA extensions or to provide application-specific registers in a few SoC applications.
- CP1 is the floating-point unit itself.

Special MIPS Use of the *Word Coprocessor*. The word coprocessor is normally used to mean an
optional part of a processor that takes responsibility for some extension to the instruction set.

## CPU Control Instructions
```
mtc0 s, <n> # Move to coprocessor 0
mfc0 d,$n # Move from coprocessor 0

mtc0 s,$12,1 # 64 位向下兼容指令
```

## Which Registers Are Relevant When?
>  为什么都是和中断有关的， cache等就是没有关系的

1. After power-up
  SR config
2. Handling any exception
  k0 k1 Cause
3. Returning from exception
  SR(EXL)
4. Interrupts
  SR
5. Instructions just there to cause exceptions

## CPU Control Registers and Their Encoding

### SR

| Name     | Specification                                                 |
| :------: | :------:                                                      |
| CU3-0    | CU0 user mode run Co0 instruction  CU1(FPU) CU2(Co2) CU3(Co3) |
| RP       | reduce power                                                  |
| FR       |                                                               |
| RE       | reverse endian                                                |
| MX       | enable instruction set extension                              |
| PX       |                                                               |
| BEV      | boot exception vector                                         |
| FR       | tlb shutdown                                                  |
| SR, NMI  |                                                               |
| FR       |                                                               |
| FR       |                                                               |
| FR       |                                                               |
| FR       |                                                               |
| FR       |                                                               |

### Cause Register
> 64/85


### Exception Restart Address (EPC) Register
**EPC** holds the address of the return point for this exception.

The instruction causing (or suffering) the exception is at EPC, unless Cause (BD),
in which case EPC points to the previous (branch) instruction.
>  Cause(BD) 是什么东西，为什么会有两种情况出来的

EPC is 64 bits wide if the CPU is.

### Bad Virtual Address (BadVAddr) Register
This register holds the address whose use led to an exception;

it is set on any MMU-related exception, on an attempt by a user program to access addresses
outside kuseg, or if an address is wrongly aligned. After any other exception it is undefined.

Note in particular that it is not set after a bus error. BadVAddr is 64 bits wide if the CPU is.

### Count/Compare Registers: The On-CPU Timer
These registers provide a simple general-purpose interval timer that runs continuously and that can be programmed to interrupt

The interrupt usually comes out of the CPU and is wired back to an interrupt by some
system-dependent mechanism—but see the IntCtl register for how to find out
> wired back to 是什么意思
> IntCtl 中间的含义是什么意思

Compare is a 32-bit read/write register. When Count increments to a value
equal to Compare, the interrupt is raised. The interrupt remains asserted until
cleared by a subsequent write to Compare

To produce a periodic interrupt, the interrupt handler should always increment Compare by a fixed amount (not an increment to Count, because the
period would then get stretched slightly by interrupt latency
> 为什么中间含有时间的延迟啊

### Processor ID (PRId) Register

1. a read-only register to be consulted to identify your CPU
2. composed of **Company Options**    **Company ID**  **CPU ID**  **Revision**
3. Revision is strictly manufacturer dependent and wholly unreliable for any purpose other than helping a CPU vendor to keep track of silicon revisions

### Config Registers: CPU Resource Information and Configuration
> 难道这些消息都是非常常用的内容吗，为什么需要写入到寄存器中间，对应的数据存储在那些地方，　是在什么时候加载到内存中间的

The MIPS32/64 standard defines four configuration registers for initialization software to use: **Config** and **Config1-3**

| Name | Specification |
| :------:| :------: |
|     M   |Continuation bit—reads 1 if there’s at least one more configuration register (i.e. Config1) available.|
| impl    | Implementation-dependent configuration flags |
| BE      | Reads 1 for big-endian, 0 for little-endian. |
| AT      | MIPS32 or MIPS64 |
| AR      | Architecture revision level |
| MT      | MMU type |
| VI      | Set 1 if the L1 I-cache is indexed and tagged with virtual (program) addresses. A virtual I-cache requires special care by operating systems |
| and so on | configuration of the hardware |


### EBase and IntCtl: Interrupt and Exception Setup

These registers (new in Release 2 of MIPS32/64) give you control over the new interrupt capabilities added there.
> new interrupt capabilities 难道是新的中断能力吗？

EBase was added to allow you to relocate all the exception entry points for a CPU;
> exception entry point 有点难以理解其中的道理

it’s primarily there for multiprocessor systems that share memory, so that different CPUs are not obliged to use the same exception handlers.
> 两者含有逻辑关系的吗?


### SRSCtl and SRSMap: Shadow Register Setup
CPUs are equipped with one or more extra sets of general-purpose registers and switch
to a different set on an exception and in particular, on an interrupt

###  Load-Linked Address (LLAddr) Register
This register holds the physical address of the last-run **load-linked** operation,
which is kept to monitor accesses that may cause a future store conditional to fail;

Software access to LLAddr is for diagnostic use only

## CP0 Hazards—A Trap for the Unwary
> 分析了一下Coprocessor 和　流水线的关系，但是感觉说的不是非常的清楚

MIPS32/64 distinguishes two flavors of CP0 hazard, depending on which
stage of a dependent instruction’s operation may be affected.
**Execution hazards** are those where the dependent instruction will not be affected until it reads a CP0 register value;
**instruction hazards** are those where the dependent instruction is affected in its very earliest stages—at worst, at the time it is fetched from
cache or memory
> 从来没有见到过的hazard 的处理


> !!!! 下面的三个全部没有看，但是很短的
### Hazard Barrier Instructions

### Instruction Hazards and User Hazards

### Hazards between CP0 Instructions























# How Caches Work on MIPS Processors
> 这一章的核心问题解决了什么东西
> 对于Cache实现那些操作，如何实现，　使用那些指令
> 需要注意的问题是什么
>

0. the way MIPS caches work and what the software has to do to make caches useful and reliable.
1. bootstrap software must be careful to initialize the caches correctly before relying on them
2. how to test the cache memory and probe for particular entries.
3. control exactly what will get cached at run time

## Caches and Cache Management
MIPS CPUs always have separate L1 caches for instructions and data
(I-cache and D-cache, respectively) so that an instruction can be read and a
load or store done simultaneously.

## How Caches Work
略

##  Write-Through Caches in Early MIPS CPUs

## Write-Back Caches in MIPS CPUs

## Other Choices in Cache Design
1. physically addressed/virtually addressed
  我认为没有讲清楚
2. Choice of line size
The line size is the number of words of data stored with each tag
3. Split/unified

## Managing Caches
A MIPS CPU has two fixed 512-MB windows onto physical memory, one cached (“kseg0”) and one uncached (“kseg1”).
Typically, OS code runs in kseg0 and uses kseg1 to build uncached references.

**?????????????**

## L2 and L3 Caches

## Cache Configurations for MIPS CPUs
CPUs that add a second level of cache reduce the miss penalty for the L1
cache—which may then be able to be smaller or simpler. CPUs with on-chip
L2 caches typically have smaller L1s, with dual 16-KB L1 caches a favored “sweet spot.”

## Programming MIPS32/64 Caches

## The Cache Instruction
1. Invalidate range of locations
2. Write-back range of locations
3. Invalidate entire cache
4. Initialize caches

### The Cache Instruction

```
cache OP,addr
```

Of the 5-bit field, the upper 2 bits select which cache to work on

There are three ways differ in how they select the cache entry (the “cache line”) they will work on:
1. **Hit-type** cache operation: Presents an address (just like a load/store),
which is looked up in the cache. If this location is in the cache (if it
“hits”), the cache operation is carried out on the enclosing line. If this
location is not in the cache, nothing happens.
> enclosing line 是什么意思的 ?

2. **Address-type** cache operation: Presents an address of some memory data,
which is processed just like a cached access. That is, if the line you
addressed was not previously in cache, the data is fetched from memory
before the cache operation.

3. **Index-type** cache operation: Uses as many low bits of the virtual address
as are needed to select the byte within the cache line, then the cache
line address inside one of the cache ways, and then the way. You have
to know the size of your cache to know exactly where the field boundaries
are, but your address is used something like this:

\[Unused\] \[Way1-0\] \[Index\] \[byte-within-line\]

The synci instruction (new to the MIPS32 Release 2 update) provides a
clean mechanism for ensuring that instructions you’ve just written are correctly
presented for execution (it combines a D-cache write-back with an
I-cache invalidate)

###  Cache Initialization and Tag/Data Registers
For diagnostic and maintenance purposes it’s good to be able to read and write the cache tags;

MIPS32/64 defines a pair of 32-bit registers **TagLo** and **TagHi** to stage data between the tag part
of the cache and management software.
> 如何理解stage data的含义

The fields in the two registers(**TagLo** and **TagHi**) reflect the tag field in the cache and are CPU
dependent.
> 如何理解CPU dependent 如何解释


Only one thing is guaranteed: An all-zero value in the tag register(s)
corresponds to a legal, properly formed tag for a line that contains no valid data.
The CPU implements a cache IndexStoreTag instruction, which copies the
contents of the tag registers into the cache line. So by setting the registers to zero,
and storing the tag value, you can start to initialize a cache from an unknown
starting state

### Cache Sizing and Figuring Out Configuration
> 似乎什么都没有说

### Initialization Routines

1. Set up some memory you can fill the cache from—it doesn’t matter what
data you store in it, but it needs to have correct check bits if your system
uses parity or ECC. A good trick is to reserve the bottom 32 K of system
memory for this purpose, at least until after the cache has been initialized. If you fill it with data using uncached writes, it will contain correct
parity.

A buffer that size is not going to be big enough to initialize a large L2
cache, and you may need to do something more complicated.
> 更加复杂的事情，比如说什么东西

2. Set TagLo to zero, which makes sure that the valid bit is unset and the
tag parity is consistent (we’d set TagHi on a CPU that needed it).
The TagLo register will be used by the cache IndexStoreTag cache
instructions to forcibly invalidate a line and clear the tag parity.
3. Disable interrupts if they might otherwise happen.
4. Initialize the I-cache first, then the D-cache.
5. D-cache initialization is slightly more awkward, because there is no D-side equivalent of the “fill” operation;
> 两个初始化的过程　不是非常的清楚

### Invalidating or Writing Back a Region of Memory in the Cache

## Cache Efficiency

## Reorganizing Software to Influence Cache Efficiency

## Cache Aliases




















# Exceptions, Interrupt, and Initialization
> 嵌套中断的处理真的有什么不同的位置吗 ？


In the MIPS architecture interrupts, traps, system calls, and everything else that can disrupt the normal flow of execution are called exceptions and are handled by a single mechanism
> 这一个定义老子是服了

Category:
1. External events(interrupts)：
2. Memory translation exceptions: page fault, segment fault and so on.
3. Other unusual program conditions for the kernel to fix :this category is fuzzy, maybe caused by some difficult and rare combination of operation and operands
4. Program or hardware-detected errors
5. Data integrity problems
6. System calls and traps

Some things do not cause exceptions, though you’d expect them to.

The way that a MIPS CPU starts up after system reset is implemented as a kind of exception and borrows functions from exceptions

## Precise Exception
>　分支延迟槽　在中断中间，　如何分析和处理


We’ll explain why MIPS exceptions are called “precise,” discuss **exception entry points**, and discuss some **software conventions**.

The MIPS implementation of precise exceptions is quite costly, because it
limits the scope for pipelining. That’s particularly painful in the FPU, because
floating-point operations often take many pipeline stages to run. The instruction following a MIPS FP instruction can’t be allowed to commit state (or reach
its own exception-determination point) until the hardware can be sure that the
FP instruction won’t produce an exception

> 为什么流水线长就precise exception会导致消耗很大

## Exception Vector

## Exception Handling: Basics
Any MIPS exception handler routine has to go through the same stages:
1. Bootstrapping
  **k0** and **k1** registers  to reference a piece of memory that can be used for other register saves
2. Dispatching different exceptions

3. Constructing the exception processing environment
  save CPU registers
4. Processing the exception
5. Preparing to return
  The high-level function is usually called as a subroutine and therefore returns into the low-level dispatch code
> **low level dispatch code** 是什么东西，为什么preparing to return 和 return 需要拆分成为两个部分出来的

6. Returning from an exception


## Returning from an Exception
1. The return of control to the exception victim and the change (if required) back from kernel to a lower-privilege level must be done at the same time
> 为什么需要是原子性的，没有会发生什么，如何实现保证的

2. MIPS CPUs have an instruction, **eret**, that does the whole job; it both
clears the SR(EXL) bit and returns control to the address stored in EPC

## Nesting Exceptions
1. vital state from the interrupted program is held in EPC and SR

3. An exception handler that is going to survive a nested exception must use
some memory locations to save register values. The data structure used is often
called an **exception frame**; multiple exception frames from nested exceptions are
usually arranged on a stack.

4.You can avoid all exceptions;
  1. interrupts can be individually masked by software to conform to your priority rules, masked all at once with the **SR(IE)** bit, or implicitly masked (for later CPUs) by the exception-level bit.
  2. Other kinds of exceptions can be avoided by appropriate software discipline.
For example, privilege violations can’t happen in kernel mode (used by most exception processing software), and programs can avoid the possibility of addressing
errors and TLB misses. It’s essential to do so when processing higher-priority exceptions
> 到底和如何区分exception 和 interrupt的

## An Exception Routine
将counter ++ 的中断
```
.set noreorder
.set noat
xcptgen:
la k0,xcptcount # get address of counter
lw k1,0(k0) # load counter
addu k1,1 # increment counter
sw k1,0(k0) # store counter
eret # return to program
.set at
.set reorder
```


## Interrupts
The MIPS exception mechanism is general purpose, following are two main
  1. One is the TLB miss when an application running under a
  memory-mapped OS like UNIX steps outside the (limited) boundaries of the
  on-chip translation table; we mentioned that before and will come back to it in
  2. the other popular exceptions are interrupts, occurring when a device
  outside the CPU wants attention.

we’re dealing with an outside world that won’t wait for us, interrupt service time is often critical

Three important aspects of interrupt：
1. Interrupt resources in MIPS CPUs
2. Implementing interrupt priority
3. Critical regions, disabling interrupts, and semaphores


### Interrupt Resources in MIPS CPUs
> 似乎是Cause 接受中断， SR 来处理中断
0. **Cause** register has seven eight independent interrupt bits, five or six of these are signals from external logic into the CPU,
while two of them are purely software accessible
1. An active level on any input signal is sensed in each cycle and will cause an
exception if enabled

The CPU’s willingness to respond to an interrupt is affected by bits in **SR**. There are three relevant fields
1. The global interrupt enable bit SR(IE) must be set to 1, or no interrupt will be serviced
2. The **SR(EXL)** (exception level) and **SR(ERL)** (error level) bits will inhibit interrupts if set (as one of them will be immediately after any exception)
> 括号中间的话什么意思， 这两个bit会被设置为1当发生exception 的时候，为什么只是其中的一个，**SR(ERL)** 是为什么?
3. The status register also has eight individual interrupt mask bits **SR(IM)**, one for each interrupt bit in Cause. Each SR(IM) bit should be set to 1 to enable the corresponding interrupt so that programs can determine exactly which interrupts can happen and which cannot


> Cause(ExeCode) 到底是用来做什么的东西


What Are the Software Interrupt Bits For?

The software interrupts are at the lowest positions, and the hardware interrupts are arranged in increasing order

Interrupt processing proper begins after you have received an exception and discovered from **Cause(ExcCode)** that it was a hardware interrupt. Consulting Cause(IP), we can find which interrupt is active and thus which device is signaling us. Here is the usual sequence:
1. Consult the Cause register IP field and logically “and” it with the current interrupt masks in SR(IM) to obtain a bit map of active, enabled interrupt requests. There may be more than one, any of which would have caused the interrupt.
2. Select one active, enabled interrupt for attention. Most OSs assign the different inputs to fixed priorities and deal with the highest priority first, but it is all decided by the software.
3. You need to save the old interrupt mask bits in SR(IM), but you probably already saved the whole SR register in the main exception routine.
4. Change SR(IM) to ensure that the current interrupt and all interrupts your software regards as being of equal or lesser priority are inhibited
5. If you haven’t already done it in the main exception routine, save the state (user registers, etc.) required for nested exception processing.
6. Now change your CPU state to that appropriate to the higher-level part of the interrupt handler, where typically some nested interrupts and exceptions are permitted.
  In all cases, set the global interrupt enable bit SR(IE) to allow higherpriority interrupts to be processed. You’ll also need to change the CPU
privilege-level field SR(KSU) to keep the CPU in kernel mode as you
clear exception level and, of course, clear SR(EXL) itself to leave exception mode and expose the changes made in the status register.
7. Call your interrupt routine.
8. On return you’ll need to disable interrupts again so you can restore the
preinterrupt values of registers and resume execution of the interrupted
task. To do that you’ll set SR(EXL). But in practice you’re likely to do this
implicitly when you restore the just-after-exception value of the whole
SR register, before getting into your end-of-exception sequence


### Implementing Interrupt Priority in Software
> 当真讲了关于任何　和　优先级　实现有关的东西的吗 ?

interrupt priority level(IPL)

The MIPS CPU (until you use the new vectored interrupt facilities) has a simpleminded approach to interrupt priority: all interrupts are equal.

Not only are interrupt handlers run with the **IPL** set to the level appropriate to their particular interrupt cause, but there’s provision for programmers to raise and lower the IPL

Not only are interrupt handlers run with the IPL set to the level appropriate
to their particular interrupt cause, but there’s provision for programmers to
raise and lower the IPL


While there are other ways of doing it, the simplest schemes have the following characteristics
1. Fixed priorities
> 似乎表达的是高优先级可以打断低优先级
2. IPL relates to code being run
> 问题是piece of code 说的是什么东西
3. Simple nested scheduling
> Except at the lowest level, any interrupted code will be returned to as soon as there are no more active interrupts at a higher level. At the lowest level there’s quite likely a scheduler that shares the CPU out among various tasks, and it’s common to take the opportunity to reschedule after a period of interrupt activity
> 表示不知道在说什么JB玩意儿啊，难受啊

On a MIPS CPU a transition between interrupt levels must (at least) be
accompanied by a change in the status register SR, since that register contains
all the interrupt control bits.
> **cause** 寄存器是做什么的， 既然SR 包含所有的中断控制bits


Any change in the IPL, therefore,
requires a piece of code that reads, modifies, and writes back the SR in
separate operations:
```
mfc0 t0, SR
1:
or t0, things_to_set
and t0, ˜(things_to_clear)
2:
mtc0 t0, SR
```

### Atomicity and Atomic Changes to SR
The code implementing the atomic change is sometimes called a critical region

So in a uniprocessor system, any critical region can be simply
protected by disabling all interrupts around it; this is crude but effective
> 难道multiprocessor 还有更加麻烦的问题吗 ？

The interrupt-disabling sequence (requiring a read-modify-write sequence on SR) is itself not
guaranteed to be atomic. I know of four ways of fixing this impasse and one way to avoid it.


In MIPS32 Release 2, **di** instruction atomically clears the SR(IE) bit, returning the original value of SR in a general-purpose register.
> 当关中断需要三个步骤， 所以关中断的过程中间也是可能含有中断， 使用di指令实现原子操作


A more general fix is to insist that no interrupt may change the value of
**SR** held by any interruptible code

This requires that interrupt routines always restore **SR** before returning, just as they’re expected to restore the state of all the user-level registers

But sometimes this restriction is too much. For example, when you’ve sent
the last waiting byte on a byte-at-a-time output port, you’d like to disable the
ready-to-send interrupt (to avoid eternal interrupts) until you have some more
data to send. And again, some systems like to rotate priorities between different
interrupts to ensure a fair distribution of interrupt service attention.
> 受不了， this restriction 在上下文中间完全没有办法检测到是什么东西啊




Another solution is to use a system call to disable interrupts (probably you’d
define the system call as taking separate bit-set and bit-clear parameters and get
it to update the status register accordingly). Since a syscall instruction works
by causing an exception, it disables interrupts atomically. Under this protection
your bit-set and bit-clear can proceed cheerfully. When the system call exception handler returns, the global interrupt enable status is restored (once again
atomically).
> syscall 是 exception, 而exception 是无法多级中断的， 为什么需要采用如此曲折的方法



A system call sounds pretty heavyweight, but it actually doesn’t need to take
long to run; however, you will have to untangle this system call from the rest of
the system’s exception-dispatching code.
> 不知道exception-dispatching code是什么， untangle 指的是什么东西

The third solution—which all substantial systems should use for at least
some critical regions—is to use the load-linked and store-conditional instructions to build critical regions without disabling interrupts at all, as described
below. Unlike anything described above, that mechanism extends correctly to
multiprocessor or hardware-multithreading systems
> 第四种是什么东西
> load-linked store-conditional 是什么操作的啊


### Critical Regions with Interrupts Enabled: Semaphores the MIPS Way
> 不知道为什么， 原子性和中断就挂上钩了

High-level atomicity (for threads calling wait()) is
dependent on being able to build low-level atomicity,
where a test-and-set operation can operate correctly in the face of interrupts (or, on a multiprocessor, in
the face of access by other CPUs).

using the ll (load-linked) and sc (store-conditional)
instructions in sequence. sc will only write the addressed location if the hardware confirms that there has been no competing access since the last ll and will
leave a 1/0 value in a register to indicate success or failure.
> 但是**sc** **ll** 的实现还是不清楚的

In most implementations this information is pessimistic: Sometimes sc
will fail even though the location has not been touched; CPUs will fail
the sc when there’s been any exception serviced since the ll,16 and most
multiprocessors will fail on any write to the same “cache line”-sized block
of memory. It’s only important that the sc should usually succeed when
there’s been no competing access and that it always fails when there has
been one such.
> 有点看不懂

Here’s wait() for the binary semaphore sem:
```
wait:
  la t0, sem
TryAgain:
  ll t1, 0(t0)
  bne t1, zero, WaitForSem
  li t1, 1
  sc t1, 0(t0)
  beq t1, zero, TryAgain
  /* got the semaphore... */
  jr ra
```

### Vectored and EIC Interrupts in MIPS32/64 CPUs
> 真的讲了EIC mode 吗 ？


### Shadow Register
Release 2 of the MIPS32/64 specification permits CPUs to provide one or
more distinct sets of general-purpose registers: The extra register sets are called
shadow registers. Shadow registers can be used for any kind of exception but are
most useful for interrupts.



## Starting Up
> 启动过程一堆问题

### Probing and Recognizing Your CPU

### Bootstrap Sequences

### Bootstrap Sequences



## Emulating Instructions
> 有点麻烦的呀

1. Sometimes an exception is used to invoke a software handler that will stand in
for the exception victim instruction, as when you are using software to implement a floating-point operation on a CPU that doesn’t support FP in hardware.
Debuggers and other system tools may sometimes want to do this too.

Finding the exception-causing instruction is easy; it’s usually pointed to by
EPC, unless it was in a branch delay slot, in which case Cause(BD) is set and
the exception victim is at the address EPC + 4
> 找到EPC 和 Cause(BD) 来解释这两个东西














































# Low-level Memory Management and the TLB
> 查看14.4

> what's page table

> 整个地址空间的划分是如何进行的

> 搞清楚两个结构， TLB entry registers  memory space

> TLB refill exception

[tlb](https://www.embedded.com/print/4007605)




## The TLB/MMU Hardware and What It Does
virtual page number(VPN)
page frame number(PFN)

A set of flag bits is stored and returned with each PFN and allows the OS to designate a page as read-only or to specify how data from that page might be cached.

Most modern MIPS CPUs (and all MIPS32/64 CPUs) double up, with each TLB entry holding two independent physical addresses corresponding to an adjacent pair of virtual pages

**EntryHi** holds the **VPN** and **ASID**

> 什么 **refill** **trap**





## TLB/MMU Registers Described
> 图6.1是真的迷糊，G是什么，　位的长度是什么,　ASID是什么
> TLB refill trap是什么东西

All TLB entries being written or read are staged through the registers **EntryHi**, **EntryLo0-1**, and **PageMask**
> 但是实际上，文档的表格还有补充的内容

Address Space Identifier(ASID)  is normally left holding the operating system’s idea of the current address space

**Random** 配合 **tlbwr** 使用，用于处理refill trap

**Context**和**XContext**也是辅助实现**refill trap** 更快实现的寄存器。

> 但是不知道是如何实现节省时间的


所以**Context** 和 **XContext** 到底是干嘛的 ?



## TLB/MMU Control Instructions


## TLB Key Fields—EntryHi and PageMask



## Programming the TLB
> TLB的programming的处理到底是谁来做的事情， 据说TLB 和 Cache 非常的相似，那么两者具体是如何使用的

TLB entries are set up by writing the required fields into EntryHi and EntryLo,
then using a **tlbwr** or **tlbwi** instruction to copy that entry into the TLB proper

> 这两个指令的specification是什么

If the TLB contains duplicate entries, then maybe **SR(TS)** bit being set
TLB entry matching some particular program address using **tlbp** to set up the Index register
Use a **tlbr** to read the TLB entry into EntryHi and EntryLo0-1

### How Refill Happens
When a program makes an access in any of the translated address regions (normally kuseg for application programs under a protected OS and kseg2 for
kernel-privilege mappings), and no translation record is present, the CPU takes a TLB refill exception.

**kseg0** untranslated

**kuseg** for application programs under a protected OS

**kseg2** for kernel-privilege mappings

## Hardware-Friendly Page Tables and Refill Mechanism


## Everyday Use of the MIPS TLB
because the MIPS TLB provides a generalpurpose address translation service, there are a number of ways you might take advantage of it
> 地址变幻还可以玩出花来


There’s no need to support a TLB **refill exception** or a **separate memory-held page table**
if your mapping requirements are modest enough that you can
accommodate all the translations you need in the TLB.
> separate memmory-held page table 是什么机制


> 未完待续




## Memory Management in a Simpler OS
[VxWork](https://en.wikipedia.org/wiki/VxWorks) is a real-time operating system.

no task-to-task protection

But operating systems targeted at
embedded applications do not usually have their roots in hardware with
memory management, and the process memory map often has the fossils
of unmapped memory maps hidden inside it.



# Floating porint

>





















# Reading MIPS Assembly Language

## A Simple Example
1. The biggest change is to unroll the loop so that it performs two comparisons per iteration; we can also move one of the loads down to the tail of the loop. With these changes, the delay slot for every load and branch can be filled with useful work
> 显然对于其中的循环展开和指令的调整是很不清楚的

2. `#inlcude`和`macro`
  1. LEAF is used to define a simple subroutine (one that calls no other subroutine and hence is a “leaf” on the calling tree
  2. Nonleaf functions have to do much more work saving variables, returning addresses, and so on
> 显然这两个关于LEAF的定义含有互相冲突的地方

## Syntax Overview

### Layout, Delimiters, and Identifiers
1. Assembly code is line oriented and `;` makes multiple instructions same line possible
2. comments

3.


###



## General Rules for Instructions

### Immediates: Computational Instructions with Constants

1. Many of the MIPS arithmetical and logical operations have an alternative form that uses a 16-bit immediate in place of **t**
> t 是什么意思

2. If an immediate value is too large to fit into the 16-bit field in the machine instruction,
then the assembler helps out again. It automatically loads the constant into the **assembler temporary register at/$1** and then uses it to perform the operation

3. assembler automatically chooses the best way to code the operation **li**, according to the properties of the integer value.

Assembly instructions that expand into multiple machine instructions are
troublesome if you’re using **.set noreorder** directives to take control of
managing branch delay slots
> 既然noreorder了， 为什么还是会导致

### Regarding 64-Bit and 32-Bit Instructions
  1. the execution of MIPS32 instructions always leaves the upper 32 bits of any GP register set either to all ones or all zeros (reflecting the value of bit 31)
  2. arithmetic functions can not carry over directly to 64-bit systems. Adds, subtracts, shifts, multiplies, and divides all need new versions. The new instructions
are named by prefixing the old mnemonic with d (double)
> 为什么arithmetic function 就是这么特殊了啊。


## Addressing Modes
1. As noted previously, the hardware supports only one addressing mode, base **reg+offset**
2. the assembler will synthesize code to access data at addresses specified in various other ways
  1. Direct: A data label or external variable name supplied by you
  1. Direct+index: An offset from a labeled location specified with a register
  1. Constant: Just a large number, interpreted as an absolute 32-bit address
  1. Register indirect: Just register+offset with an offset of zero

3. %hi() and %lo() represent the high and low 16 bits of the address. This is not quite the straightforward division into low and high halfwords that it looks



3. **lui** The immediate value is shifted left 16 bits and stored in the register. The lower 16 bits are zeroes.
  ```
  lui at, %hi(addr) /* 将addr中的高16bit放置到 */
  ```

4. `addiu` Adds a register and a sign-extended immediate value and stores the result in a register
  The only difference between them is **addi** generates a trap when overflow while **addiu** doesn't. So addi and its overflow family (add, sub...) is often useless

5. In principle, la could avoid messing around with apparently negative
lo( ) values by using an ori instruction.
But load/store instructions have
a signed 16-bit address offset, and as a result the linker is already equipped
with the ability to fix up addresses into two parts that can be added correctly.
So la uses the add instruction to avoid the linker having to understand two
different fix-up types
> 我曹，什么东西

### Gp-Relative Addressing
1. programs that make a lot of use of global or static data, mips addressing strategy can make the compiled code significantly fatter and slower

2. This technique requires
    1. the cooperation of the **compiler**, **assembler**, **linker**, and **start-up code** to pool all of the “small” variables and constants into a single memory region;
    2. then it sets register $28 (known as the **global pointer** or **gp register**) to point to the
middle of this region. (The linker creates a special symbol, gp, whose address is the middle of this region. The address of gp must then be loaded into the gp
register by the start-up code, before any load or store instructions are used.)

3.


# 14 How Hardware and Software Work Together

## The Life and Times of an Interrupt

## What happens on a system call

## How addresses get translated in Linux/MIPS


# MIPS Specific Issues in the Linux Kernel

## Explicit Cache Management
I/O-cache coherent
> DMA 直接对于内存写入数据导致内存比cache ...

# manual

## MIPS® Architecture Reference Manual Vol. III: MIPS64® microMIPS64™ Privileged Resource Architecture

1. xkseg
2. xkphys
3. xsseg
4. xuseg
