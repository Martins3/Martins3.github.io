# Understand Linux Kernel : Program Execution

## KeyNote
1. process credential 在 fork 中间都是怎么变化的 ?
2. 各种 syscall 实现调查一下


While it may not seem
like a big problem to load a bunch of instructions into memory and point the CPU to
them, the kernel has to deal with flexibility in several areas:

- Different executable formats

  Linux is distinguished by its ability to run binaries that were compiled for other
  operating systems. In particular, Linux is able to run an executable created for a
  32-bit machine on the 64-bit version of the same machine. For instance, an executable created on a Pentium can run on a 64-bit AMD Opteron.

- Shared libraries

  Many executable files don’t contain all the code required to run the program but
  expect the kernel to load in functions from a library at runtime.

- Other information in the execution context

  This includes the command-line arguments and environment variables familiar
  to programmers.

## 1 Executable Files

An executable file is a regular file that describes how to initialize a new execution context (i.e., how to start a new computation).

#### 1.1 Process Credentials and Capabilities

The process’s credentials are stored in several fields of the process descriptor, listed
in Table 20-1. These fields contain identifiers of users and user groups in the system,
which are usually compared with the corresponding identifiers stored in the inodes
of the files being accessed.
Table 20-1. Traditional process credentials
| Name         | Description                                                                                   |
|--------------|-----------------------------------------------------------------------------------------------|
| uid, gid     | User and group real identifiers
| euid, egid   | User and group effective identifiers
| fsuid, fsgid | User and group effective identifiers for file access groups Supplementary group identifiers
| suid, sgid   | User and group saved identifiers
> @todo 到底什么是 effective 和 real

A UID of 0 specifies the superuser (root), while a user group ID of 0 specifies the root
group. If a process credential stores a value of 0, the kernel bypasses the permission
checks and allows the privileged process to perform various actions, such as those
referring to system administration or hardware manipulation, that are not possible to
unprivileged processes.
> @todo 可以检查分析一下

When a process is created, it always inherits the credentials of its parent. However,
these credentials can be modified later, either when the process starts executing a
new program or when it issues suitable system calls. **Usually, the uid, euid, fsuid,
and suid fields of a process contain the same value.** When the process executes a setuid program—that is, an executable file whose setuid flag is on—the euid and fsuid
fields are set to the identifier of the file’s owner. **Almost all checks involve one of
these two fields: fsuid is used for file-related operations, while euid is used for all
other operations.** Similar considerations apply to the `gid`, `egid`, `fsgid`, and `sgid` fields
that refer to group identifiers.

The process
descriptor includes an suid field, which stores the values of the effective identifiers
(euid and fsuid) at the `setuid` program startup. The process can change the effective
identifiers by means of the `setuid()`, `setresuid()`, `setfsuid()`, and `setreuid()` system calls.

Table 20-2 shows how these system calls affect the process’s credentials. Be warned
that if the calling process does not already have superuser privileges—**that is, if its
`euid` field is not null**—these system calls can be used only to set values already
included in the process’s credential fields. For instance, an average user process can
store the value 500 into its `fsuid` field by invoking the `setfsuid()` system call, but
only if one of the other credential fields already holds the same value.
> @todo Table 20-2 似乎描述的内容并不是如此，修改的限制不是只能使用现有的数值。

If the `euid` field is not 0, the `setuid()` system call modifies only the value stored in
`euid` and `fsuid`, leaving the other two fields unchanged. This behavior of the system
call is useful when implementing a `setuid` program that scales up and down the effective process’s privileges stored in the euid and fsuid fields.
> @todo 并不可以理解为什么 setuid 需要这种功能

* ***Process capabilities***

Neither the VFS nor the Ext2 filesystem currently supports the capability model, so
there is no way to associate an executable file with the set of capabilities that should
be enforced when a process executes that file. Nevertheless, a process can explicitly
get and lower its capabilities by using, respectively, the `capget()` and `capset()` system calls. For instance, it is possible to modify the login program to retain a subset of
the capabilities and drop the others.

The kernel checks the value of this flag by invoking the `capable()` function and passing the `CAP_SYS_NICE` value to it.

This approach works, thanks to some “compatibility hacks” that have been added to
the kernel code: each time a process sets the euid and fsuid fields to 0 (either by
invoking one of the system calls listed in Table 20-2 or by executing a setuid program owned by the superuser), the kernel sets all process capabilities so that all
checks will succeed. When the process resets the euid and fsuid fields to the real
UID of the process owner, the kernel checks the keep_capabilities flag in the process descriptor and drops all capabilities of the process if the flag is set. A process can
set and reset the keep_capabilities flag by means of the Linux-specific `prctl()` system call.

* ***The Linux Security Modules framework***
> one page, skip

A widely known example is Security-Enhanced Linux (SELinux), developed by the United
State’s National Security Agency.

#### 1.2 Command-Line Arguments and Shell Environment
They(environment variables)are used to customize the
execution context of a process, to provide general information to a user or other processes,
or to allow a process to keep some information across an `execve()` system call.

Command-line arguments and environment strings are placed on the User Mode
stack, right before the return address (see the section “Parameter Passing” in
Chapter 10). The bottom locations of the User Mode stack are illustrated in
Figure 20-1. Notice that the environment variables are located near the bottom of the
stack, right after a 0 long integer.
> @todo 所以，这和内核有什么关系吗 ?

#### 1.3 Libraries
A process can also load additional shared libraries at runtime by using the `dlopen()` library function.
> @todo 谁需要使用 dlopen 函数
> @todo 程序加载的过程中间，如何利用 mmap 实现动态库的加载的 ?

#### 1.4 Program Segments and Process Memory Regions
The linear address space of a Unix program is traditionally partitioned, from a logical point of view, in several linear address intervals called segments:

Each `mm_struct` memory descriptor (see the section “The Memory Descriptor” in
Chapter 9) includes some fields that identify the role of a few crucial memory regions
of the corresponding process:
- start_code, end_code

Store the initial and final linear addresses of the memory region that includes the
native code of the program—the code in the executable file.

- start_data, end_data

Store the initial and final linear addresses of the memory region that includes the
native initialized data of the program, as specified in the executable file. The
fields identify a memory region that roughly corresponds to the data segment.

- start_brk, brk

Store the initial and final linear addresses of the memory region that includes the
dynamically allocated memory areas of the process (see the section “Managing
the Heap” in Chapter 9). This memory region is sometimes called the heap.

- start_stack
Stores the address right above that of main( )’s return address; as illustrated in
Figure 20-1, higher addresses are reserved (recall that stacks grow toward lower
addresses).

- arg_start, arg_end

Store the initial and final addresses of the stack portion containing the command-line arguments.

- env_start, env_end

Store the initial and final addresses of the stack portion containing the environment strings.
> @todo 为什么 mm_struct 需要这些蛇皮东西 ?

*Notice that shared libraries and file memory mapping have made the classification of
the process’s address space based on program segments obsolete, because each of the
shared libraries is mapped into a different memory region from those discussed in
the preceding list.*

* ***Flexible memory region layout***
The kernel typically uses the flexible layout when it can get a limit on the size of
the User Mode stack by means of the `RLIMIT_STACK` resource limit (see the section
“Process Resource Limits” in Chapter 3).
This limit determines the size of the linear address space reserved for the stack; 
however, this size cannot be smaller than
128 MB or larger than 2.5 GB.
> @todo interesting 可以检查相关代码一下

> skip 吃饭了

#### 1.5 Execution Tracing
> skip 以前看过，再说吧!

## 2 Executable Formats
An executable format is described by an object of type `linux_binfmt`, which essentially provides three methods:

- load_binary

Sets up a new execution environment for the current process by reading the
information stored in an executable file.

- load_shlib

Dynamically binds a shared library to an already running process; it is activated
by the uselib( ) system call.

- core_dump

Stores the execution context of the current process in a file named core. This file,
whose format depends on the type of executable of the program being executed,
is usually created when a process receives a signal whose default action is
“dump” (see the section “Actions Performed upon Delivering a Signal” in Chapter 11).
> @todo 非常有意思，可以了解一下

All `linux_binfmt` objects are included in a singly linked list, and the address of the
first element is stored in the formats variable. Elements can be inserted and removed
in the list by invoking the `register_binfmt()` and `unregister_binfmt()` functions.
The register_binfmt( ) function is executed during system startup for each executable format compiled into the kernel. This function is also executed when a module
implementing a new executable format is being loaded, while the `unregister_binfmt()` function is invoked when the module is unloaded.

> skip 剩下的部分懒得看了，只能说没有什么神奇的地方。
