# Professional Linux Kernel Architecture : System Calls

Of particular note are the different **virtual address spaces** of the two modes and the
different ways of exploiting various **processor features**. Also of interest is how control is transferred
backward and forward between applications and the kernel, and how parameters and return values
are passed. This chapter discusses such questions.

First, let’s take a brief look at system programming to distinguish clearly between library routines of
the standard library and the corresponding system calls.

We then closely examine the kernel sources
in order to describe the mechanism for **switching** from userspace to kernel space. The infrastructure
used to implement system calls is described, and special implementation features are discussed.


## 13.1 Basics of System Programming
In this situation, a pragmatic view that somehow the data will be coaxed out of
the file is sufficient
> 学习英语了。

#### 13.1.1 Tracing System Calls
> 写了一个简单的程序，然后使用strace 跟踪了一下

#### 13.1.2 Supported Standards
Linux features system calls from all three(POSIX, BSD, System V) of the above sources — in a separate implementation, of course

#### 13.1.3 Restarting System Calls
An interesting problem arises when system calls clash with signals

In the event of an interruption, a third situation arises: The application must be informed that the
system call would have terminated successfully, had it not been interrupted by a signal during execution.
In such situations, the `-EINTR` constant is used under Linux (and under other System V derivatives).
> 优秀的倒装句啊!

Linux supports the BSD variant by means of the SA_RESTART flag, which can be specified on a per-signal
basis when handler routines are installed (see Chapter 5). The mechanism proposed by System V is
used by default because the BSD mechanism also occasionally gives rise to difficulties, as the following
example taken from [ME02], page 229, shows
> 下面有一个例子，应该简单，但是没有看

## 13.2 Available System Calls
Each system call is identified by means of a symbolic constant whose platform-dependent definition is
specified in `<asm-arch/unistd.h>`.
> 那么与平台无关的内容在哪里

Since not all system calls are supported on all architectures (some
combinations are meaningless), the number of available calls varies from platform to platform

1. Process Management
2. Time Operations
3. Signal Handling
4. Scheduling
5. Modules
6. Filesystem
7. Memory Management
8. Interprocess Communication and Network Functions
9. System Information and Settings
10. System Security and Capabilities

## 13.3 Implementation of System Calls
Functions are not called in the same
way as normal C functions because the boundary between user and kernel mode is crossed. This raises
various problems that are handled by platform-specific assembly language code. This code establishes
a processor-independent state as quickly as possible to enable system calls to be implemented independently of the underlying architecture. How parameters are passed between userspace and kernel space
must also be considered.
> 1. 切换地址空间
> 2. 传递参数
> 1. @todo fread 获取的文件数据也是需要 copy_from_user 之类的操作吗 ？


##### 13.3.1 Structure of System Calls
The mechanism for calling the routine is packed with platform-specific features
and must take numerous details into consideration — so that ultimately implementation in assembly
language code is a must.

* **Implementation of Handler Functions**

The handler functions for implementing system calls share several formal features:
1. The name of each function is prefixed with `sys_` to uniquely identify the function as a system
call — or to be more accurate, as a handler function for a system call.

2. All handler functions accept a maximum of **five parameters**; these are specified in a parameter
list as in normal C functions.

3. All system calls are executed in kernel mode.

Regardless of their complexity,
all handler functions have one thing in common. Each function declaration includes the additional (`asmlinkage`) qualifier, which is not a standard element of C syntax.
asmlinkage is an assembler macro defined in `<linkage.h>`. What is its purpose? For most platforms, the
answer is very simple — it does nothing at all!
```c
// linkage.h
#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif

#ifndef asmlinkage
#define asmlinkage CPP_ASMLINKAGE
#endif
// 暂时不关心这些内容吧!
```


However, the macro is used in conjunction with the GCC enhancement (`__attribute__`) discussed
in Appendix C on `IA-32` and `IA-64` systems only in order to inform the compiler of the special **calling
conventions** for the function (examined in the next section).


* **Dispatching and Parameter Passing**

The following overview shows the methods used by a few popular architectures to make system calls:
1. On IA-32 systems, the assembly language instruction `int $0x80` raises software interrupt 128.
This is a call gate to which a specific function is assigned to continue system call processing. The
system call number is passed in register `eax`, while parameters are passed in registers `ebx`, `ecx`,
`edx`, `esi`, and `edi`.

On more **modern** processors of the IA-32 series (Pentium II and higher), two assembly language
instructions (`sysenter` and `sysexit`) are used to enter and exit kernel mode quickly. The way in
which parameters are passed and returned is the same, but switching between privilege levels is
faster.
To enable `sysenter` calls to be made faster without losing downward **compatibility** with older
processors, the kernel maps a memory page into the top end of address space (at `0xffffe000`).
Depending on processor type, the system call code on this page includes either int 0x80 or
sysenter.

Calling the code stored there (with `call 0xffffe000`) allows the standard library to automatically select the method that matches the processor used.

2. The AMD64 architecture also has its own assembly language instruction with the revealing name
of syscall to implement system calls. The system call number is held in the raw register, parameters in `rdi`, `rsi`, `rdx`, `r10`, `r8`, and `r9`.
> 如果syscall 中间最多只能包含五个参数，为什么寄存器一共有6个

A table named `sys_call_table`, which holds a set of function pointers to handler routines
The principle, however, is always the same: by reference to the system call number, the kernel
finds the appropriate position in the table at which a pointer points to the desired handler function.
> @todo 如何使用syscall_table 来方便编程的, syscall_table 是如何被编译的

The tables defined in this way have the properties of a C array and can therefore be processed using
pointer arithmetic. `sys_call_table` is the base pointer and points to the start of the array, that is, to
the zero entry in C terms. If a userspace program invokes the open system call, the number passed
is 5. The dispatcher routine adds this number to the `sys_call_table` base and arrives at the fifth entry
that holds the address of `sys_open` — this is the processor-independent handler function. **Once the
parameter values still held in registers have been copied onto the stack**, the kernel calls the handler
routine and switches to the processor-independent part of system call handling.
```c
// arch/x86/include/asm/syscall.h
extern const sys_call_ptr_t sys_call_table[];
```
> 配合syscall_tbl64 中的内容，以及链接脚本等操作，最后实现只是需要一个syscall num 就可以调用对应的函数

[syscall table 的实际位置 : syscall_tbl64](https://stackoverflow.com/questions/17652555/where-is-the-system-call-table-in-linux-kernel)

system call parameters cannot be passed on the stack as would normally
be the case. Switching between the stacks is performed either in
architecture-specific assembly language code that is called when kernel mode is
entered, or is carried out automatically by the processor when the protection level is
switched from user to kernel mode.

*Because the kernel mode and user mode use two different stacks, as described in
Chapter 3*(@todo 什么情况啊，什么时候讲过), system call parameters cannot be passed on the stack as would normally
be the case. Switching between the stacks is performed either in
architecture-specific assembly language code that is called when kernel mode is
entered, or is carried out automatically by the processor when the protection level is
switched from user to kernel mode.


* **Return to User Mode**
Generally, the following applies for system call return values. Negative values indicate an error, and
positive values (and 0) denote successful termination.

> 书上还讨论一些关于error code 的定义和含义的问题.

The C function, in which the system call handler is implemented, uses return to place the
return code on the kernel stack. This value is copied into a specific processor register (eax on IA-32
systems, a3 on Alpha systems, etc.), where it is processed by the standard library and transferred to user
applications.

#### 13.3.2 Access to Userspace
There are two reasons why the kernel has
to access the address space of user applications:
1. If a system call requires more than six different arguments, they can be passed only with the help
of C structures that reside in process memory space. A pointer to the structures is passed to the
system call by means of registers.
2. Larger amounts of data generated as a side effect of a system call cannot be passed to the user
process using the normal return mechanism. Instead, the data must be exchanged in defined
memory areas. These must, of course, be located in userspace so that the user application is able
to access them.


The kernel may not therefore simply de-reference userspace pointers, but also must employ specific
functions to ensure that the desired area resides in RAM. To make sure that the kernel complies with this
convention, userspace pointers are labeled with the `__user` attribute to support automated checking by
C check tools.


#### 13.3.3 System Call Tracing
Implementation of the `sys_ptrace` handler routine is architecture-specific and is defined in
`arch/arch/kernel/ptrace.c`

Before examining the flow of the system call in detail, it should be noted that this call is needed because
ptrace — essentially a tool for reading and modifying values in process address space — **cannot be used
directly to trace system calls**. Only by extracting the desired information at the right places can trace
processes draw conclusions on which system calls have been made. Even debuggers such as gdb are
totally reliant on ptrace for their implementation. ptrace offers more options than are really needed to
simply trace system calls.

`ptrace` requires four arguments as the definition in the kernel sources shows:

```c
// ptrace.c

SYSCALL_DEFINE4(ptrace, long, request, long, pid, unsigned long, addr,
		unsigned long, data)
```

1. `pid` identifies the target process. The process identifier is interpreted with respect to the
namespace of the caller. Even though the way in which strace is handled suggests that process
tracing must be enabled right from the start, this is not true. The tracer program must **attach**
itself to the target process by means of ptrace — and this can be done while the process is
already running (not only when the process starts).
`strace` is responsible for attaching to the process, usually immediately after the target program
is started with `fork` and `exec`.
> @todo 到底什么动作指的是attach

2. `addr` and `data` pass a memory address and additional information to the kernel. Their meanings
differ according to the operation selected.

3. With the help of symbolic constants, request selects an operation to be performed by ptrace. A
list of all possible values is given on the manual page `ptrace(2)` and in `<ptrace.h>` in the kernel
sources. The available options are as follows:

- `PTRACE_ATTACH` issues a request to attach to a process and initiates tracing. `PTRACE_DETACH`
detaches from the process and terminates tracing. **A traced process is always terminated
when a signal is pending.** *The options below enable ptrace to be stopped during a system
call or after a single assembly language instruction.*

When a traced process is stopped, the tracer program is informed by means of a `SIGCHLD`
signal that waiting can take place using the `wait` function discussed in Chapter 2.
When tracing is installed, the `SIGSTOP` signal is sent to the traced process — this causes the
tracer process to be interrupted for the first time. This is essential when system calls are
traced, as demonstrated below by means of an example.

- `PEEKTEXT`, `PEEKDATA`, and `PEEKUSR` read data from the process address space. PEEKUSR reads
the normal CPU registers and any other debug registers used11 (of course, only the contents
of a single register selected on the basis of its identifier are read — not the contents of the
entire register set). PEEKTEXT and PEEKDATA read any words from the text or data segment
of the process.

- `POKETEXT`, `POKEDATA`, and `PEEKUSR` write values to the three specified areas of the monitored process and therefore manipulate the process address space contents; this can be very
important when debugging programs interactively.

> peek 和 poke 机制 ?

Because PTRACE_POKEUSR manipulates the debug registers of the CPU, this option supports
the use of advanced debugging techniques; for example, monitoring of events that halt
program execution at a particular point when certain conditions are satisfied.

- `PTRACE_SETREGS` and `PTRACE_GETREGS` set and read values in the privileged register set of
the CPU.

- `PTRACE_SETFPREGS` and `PTRACE_GETFPREGS` set and read registers used for floating-point
computations. These operations are also very useful when testing and debugging applications interactively.

- System call tracing is based on `PTRACE_SYSCALL`. If ptrace is activated with this option, the
kernel starts process execution until a system call is invoked. Once the traced process has
been stopped, wait informs the tracer process, which then analyzes the process address
space using the above ptrace operations to gather relevant information on the system call.
The traced process is stopped for a second time after completion of the system call to allow
the tracer process to check whether the call was successful.
Because the system call mechanism differs according to platform, trace programs such
as strace must implement the reading of data separately for each architecture; this is
a tedious task that quickly renders source code for portable programs unreadable (the
strace sources are overburdened with pre-processor conditionals and are no pleasure
to read).

- `PTRACE_SINGLESTEP` places the processor in single-step mode during execution of the
traced process. In this mode, the tracer process is able to access the traced process after
each assembly language instruction. Again, this is a very popular application debugging
technique, particularly when attempting to track down compiler errors or other such
subtleties.
Implementation of the single-step function is strongly dependent on the CPU used — after
all, the kernel is operating on a machine-oriented level at this point. Nevertheless, a
uniform interface is available to the tracer process on all platforms. After execution of
the assembler function, a `SIGCHLD` signal is sent to the tracer, which gathers detailed
information on the process state using further ptrace options. This cycle is constantly
repeated — the next assembler instruction is executed after invoking ptrace with
the `PTRACE_SINGLESTEP` argument, the process is put to sleep, the tracer is informed
accordingly by means of `SIGCHLD`, and so on.

- `PTRACE_KILL` closes the traced process by sending a KILL signal.
- `PTRACE_TRACEME` starts tracing the current process. The parent of the current process automatically assumes the role of tracer and must be prepared to receive tracing information
from its child.
- `PTRACE_CONT` resumes execution of a traced process without specifying special conditions
for stopping the process — the traced application next stops when it receives a signal.

* **System Call Tracing**

[](../code/strace.c) 

> 这个代码并没有办法执行，但是易于阅读

Program flow is obvious once the ball is rolling. A system call requested by the traced process triggers
the ptrace mechanism in the kernel, which sends a `CHLD` signal to the tracer process. The handler of the
tracer process reads the required information — the number of the system call — and outputs it, again
using the ptrace mechanism. Execution of the traced process is resumed and interrupted again when a
system call is invoked.
But how is the ball set rolling? Somehow or other the handler function must be invoked for the first time
in order to log system call tracing. **As noted above, the kernel also sends `SIGCHLD` signals to the tracer
process when a signal is sent to the traced process** — in doing so,
it invokes the same handler function activated when a system call occurs.
The fact that the kernel automatically sends a `STOP` signal to the traced
process when tracing is initiated ensures that the handler function is invoked when tracing starts — even
if the process receives no other signals. This sets the ball — that is, system call tracing — rolling.
> strace 的原理 : traced process 被 STOP 的时候，tracer 会被发送一个 `SIGCHLD` ，从此之后，每次收到 syscall , 都会为下一次 syscall 注册 ptrace syscall 消息

* ***Kernel-Side Implementation***

If a different action from PTRACE_ATTACH was requested, ptrace_check_attach first checks whether
a tracer is attached to the process, and the code splits depending on the particular ptrace operation.
This is handled in arch_ptrace; the function is defined by every architecture and cannot be provided
by the generic code. However, this is not entirely true: Some requests can, in fact, be handled by
architecture-independent code, and they are handled in ptrace_request (from kernel/ptrace.c) called
by `arch_ptrace`. Only very simple requests are processed by this function. For example, PTRACE_DETACH
to detach a tracer from a process is one of them.

Usually, a large case structure that deals separately with each case (depending on the request
parameter) is employed for this purpose. I discuss only some important cases: `PTRACE_ATTACH` and
`PTRACE_DETACH`, `PTRACE_SYSCALL`, `PTRACE_CONT` as well as `PTRACE_PEEKDATA` and `PTRACE_POKEDATA`. 
The implementation of the remaining requests follows a similar pattern.

* Implementation of `PTRACE_CONT` and `_SYSCALL`

* Stopping Tracing

* Reading and Modifying Target Process Data


## 13.4 Summary
System calls are a **synchronous** mechanism to change from user into kernel mode. The next chapter
introduces you to interrupts that require **asynchronously** changing between the modes.

1. Syscall 的开销体现在什么地方 ?
2. 是如何实现内核空间和用户空间的切换的 ?
3. stack 是如何管理的 ?
4. 自我修养中间的 关于 vdso 是怎么回事 ?
5. 参数是如何传递的 ?
