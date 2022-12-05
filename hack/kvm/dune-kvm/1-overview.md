# 从 Dune 到 kvm : 虚拟化原理介绍

- [ ] we wanna to change the pte without ioctl : https://github.com/misc0110/PTEditor

## 内存虚拟化

## 中断虚拟化


## Dune
http://dune.scs.stanford.edu/

Dune is designed to only expose privileged CPU features in a safe **fashion**.

questions:
1. Dune => glibc VMCALL global directly
2. `dune_init`
3. chmod /dev/dune
4. signals and more robust


Notes:
We use Dune to implement three userlevel applications that can benefit from access to privileged hardware: a sandbox for untrusted code, a privilege separation facility, and a garbage collector


*Compared to ptrace in Linux*, we show that Dune can intercept a system call with 25× less overhead.

Dune extends the kernel with a module that enables VT-x,
placing the kernel in VMX root mode. Processes using
Dune are granted direct but safe access to privileged hardware by running in VMX non-root mode

However, by exposing hardware privilege modes, Dune enables additional privilege-separation techniques within a process that would not otherwise be practical

For example, *a VMM might provide a hypercall to register an interrupt handler for a virtual network device,*
whereas a Dune process would use a hypercall to call read on a TCP socket.

*In Dune, we configure
the EPT to reflect process address spaces. As a result,
the memory layout can be sparse and memory can be
coherently shared when two processes map the same
memory segment.*

*For Dune, however, the problem is that
we want to expose the full host-virtual address space and
yet the guest-physical address space is limited to a smaller
size (e.g., a 36-bit physical limit vs. a 48-bit virtual limit
on many contemporary Intel processors).*

Dune completely changes how signal handlers are invoked


One limitation inour implementation is that we cannot
efficiently detect when EPT pages have been modified or
accessed, which is needed for swapping.
> 处理 ad bit ?

- [ ] libDune 处理的内容太多了


In many ays, transitioning a process into Dune mode
is similar to booting an OS.w


- [ ] virtual machine 可以，但是多个 process 在多个 vm 中间运行，之间的沟通比较麻烦，所以，使用 Dune 的 process 为什么需要进行沟通，


Althoug the hardware features Dune exposes suffice
in supporting our motivating use cases, several other hardware features, such as cache control, debug registers, and
access to DMA-capable devices, could also be safely exposed through virtualization hardware.h
- [ ] 通过 Dune 可以 expose hardware features, 部分是 expose 的，部分没有 expose，没有 exposed 最终如何被系统使用


## 被虚拟化的内容
exception, memory

- Normally, reporting an exception to a user program requires privilege mode transitions and an upcall mechanism (e.g., signals).
 Dune can reduce exception overhead because it uses VT-x to deliver exceptions directly in hardware.
- User programs can also benefit from fast and flexible access to virtual memory
    - Dune also gives user programs the ability to manually
control TLB invalidations.

First, a Dune process is a normal Linux process, the only difference being that it uses the VMCALL instruction to invoke system calls.

Two motivating use cases for privilege modes are *privilege separation* and *sandboxing of untrusted code*, both evaluated in this paper.


## 内核模块的作用
We then explore three
key aspects of the module’s operation: managing memory, exposing access to privileged hardware, and preserving access to kernel interfaces.


那么 : vmx launch 的工作:

## 想要知道的技术细节
1. 为什么需要二进制的 loader
2. vmx_run_vcpu : 这种窒息的大段汇编代码在 KVM 中间似乎有类似的东西
3. 为什么其不需要 emulation

- [ ] 这是一个简化版本的 KVM 吗 ？ 如果是，那么相对于 KVM 的优势在于什么地方 ?
  - [x] KVM 是提供的 CPU 的模拟，而 dune 实现的是进程的虚拟化

- [ ] 之所以可以操作页表，ept 提供了支持
- [ ] 会不会导致让 qemu 依赖的所有东西全部被拉入到内核态中间
  - [ ] 并不是让 qemu 运行在内核态，只是想让其可以访问页表, TLB

- [ ] qemu 想要修改 SPT，那么就不可以 enable EPT 吧
- [ ] 进入到内核态，还是进入到

#### intercept syscall
- [ ] how to intercept syscall
- [ ] 能不能选择性的只是 intercept 部分 syscall
- [ ] 理解一下 dune fork child 的时候，child 可以不受影响的原因

*The Dune module vectors hypercalls through the kernel’s system call table.*


dune.S
`__dune_syscall`
1. Userland 中间使用 syscall : vmcall
2. kernel : dune_sycall_handler

setup_syscall :

dune.init :



https://stackoverflow.com/questions/27786602/does-vmcall-instruction-in-x86-save-the-guest-cpu-state

page.c : dune 截获了 page 管理，那么如何才可以实现 用户态分配的内存 作为 GPA ?

- [ ] 如果用户的地址空间是自己管理的，那么 fork 的时候，内核还是会复制地址空间啊 ?

## trap

- [ ] 为什么修改 idt，将 signal 重新设置为 idt 从而从硬件上加以实现吗 ?
- [ ] dune_trap_handler : 由于 ept 的建立，从 GPA 到 HPA 的映射是 ept，通过 gup 来实现分配物理页面，自己需要注册 page fault 处理其 GVA 到 GPA 之间的映射


`__dune_syscall` 和 idt : 操作路径完全不同，当 `__dune_syscall` 是覆盖 rwmsr 中间获取的地址，然后将其替换，导致 handler 处理的入口不同。

syscall 也是被 dune 设置为可以注册，


- [ ] 曾经，syscall 作为一种 int, 也是被放在 idt 上的 (ucore 就是证明操作的)

setup_idt : 将所有的 int 的处理全部转换到 dune 设置的模式


## dune boot
在 do_dune_enter 中间，似乎已经进入到内核了，而且保证，从 dune_init_and_enter 之后，原来的程序继续运行的.
- 这个设计很巧妙，在调用 `__dune_enter` 的前半段，代码是运行在 host 的用户态的，后半段就是运行在 Guest mode 的.
- 虽然进入到内核态，但是在切换 cr3 之后，使用的地址空间实际上还是同一个，所以，就是直接返回的
- [ ] dune_boot 中间的 lgdt 是什么的暂时不知道



## debug
- [ ] `__dune_go_linux`

- [ ] 完全无法理解其信号机制, 似乎其 debug 机制就是为了将其的 reg 打印出来
  - [x] notifier_sched_in 中间，将处理函数放在 pt_regs 的返回地址，从而可以正确返回到用户空间
  - [x] 信号机制是，发现存在 signal，然后就退出到用户, 然后有马上进入到内核中间, 猜测是给用户一个注册的机会，只是表示可以截获.
  - [ ] 也就是文章说，signal 使用中断处理
  - [ ] interrupt 和 signal 为什么走两条线，在内核中间 ptrace 和 signal 不是放在一起吗 ?


在 on_dune_exit() 作为用户态的程序，其需要处理的就是 vmx 的各种退出情况。

- [ ] vmx_handle_nmi_exception 似乎没有处理 nmi, 从 vmx_launch 中间，当识别出来是 NMI，那么就 int 2 (2 是 NMI 的编号),
但是为什么进行 NMI，我们是不清楚的。

## apic && ipi
https://stackoverflow.com/questions/11359021/how-should-i-read-or-write-apic-register-apic-icr
> 通过 ICR 将中断写入到指定的 CPU 中间

## send_posted_ipi
- [x] 为什么在 kern 部分和 libdune 部分都是存在
  - 在 libdune 中间访问的 msr 实现的访问，会在 vmx 中被截断，然后 emulation
- [ ] vt-d : vmx_handle_external_interrupt, 似乎从外部的来的 interrupt 成为 posted interrupt，所以和 send ipi 是一套的

## thread mode
thread 模型:
- [ ] 没有 pthread 的支持，
- [ ] 靠什么才可以实现连个 process 共享地址空间 ?
  - [ ] 每次启动的时候，都会创建一个地址空间
  - [ ] 强行使用不对外提供的函数 `_do_fork`，真的好吗 ?

- [ ] 从 wedge 来看，是存在 -lpthread 的，是表示可以支持 pthread, 还是普通模式需要使用

- [ ] 线程之间如何通信啊 ? 地址之间是相互隔离的，但是似乎是可以通过内核地址之间通信

## wedge
struct sc

- [ ] 在 ept 模式下，可以随意切换 cr3，感觉其实在非虚拟化情况下的切换没有什么区别, 但是硬件需要提供什么额外的支持吗 ?


## user
dune_jump_to_user


## vmx
vmx_handle_syscall 的作用:
- 当 vmcall 的 reason 是 syscall 的时候: 调用 vmx_handle_syscall
- 调用 vmx_create_vcpu 中间注册的 dune_syscall_tbl
- vmx_init_syscall : 将 SYSCALL_TBL 的拷贝到 dune_syscall_tbl
- SYSCALL_TBL 启动的位置的确定是 dune 加载的时候确定的，前提是内核地址空间是固定的

- [x] 最后如何进入到 user land 注册到的 syscall
  - `__dune_syscall` 理解说明，是存在两个syscall table, user mode 采用 vmcall，然后进入到 vmx_handle_syscall 中间
  - kernel mode 采用取决于 dune_register_syscall_handler 使用

- [ ] 为什么可以切换到内核态，那么这就需要 sandbox 的理解了
- [ ] 为什么需要将 user mode 的几个做出修改

- [ ] 跟踪一下 dune_tf 和 kernel 的 tf 的关系


## mapping
vma 都是什么时候建立的

dune_init_and_enter :

dune_init:
  - dune_page_init : mmap 出来 GPA，malloc : struct page
  - setup_mappings :
      - `__setup_mappings_full` : 用户地址空间 和 内核地址空间都是可以被搞定
        - setup_syscall : TODO vsyscall 的作用是什么 ?
        - `dune_vm_map_phys` : 32bit 的空间，然后利用 `__dune_vm_page_walk` 实现
      - `__setup_mappings_precise` : 和
  - setup_syscall : 将原先的 syscall table 替换掉

dune_enter:
  - do_dune_enter :
    - map_stack() : TODO
    - `__dune_enter` :调用 ioctl 为什么使用汇编 ? => vmx_launch
      - [ ] 似乎没有建立好的东西 : syscall 和 idt 只是将入口占用，但是都是没有注册的，至少让进程可以使用默认数值吧 !
      - [ ] idt 和 ipi, apic 之类的注册都是没有处理
  - create_percpu : 当其中的

对于原来的 table 的映射关系，其采用的方法是:


原来的 page table 的映射关系，让 ept violation 维护,
在 函数 ept_set_epte 中间，ept violation 会通知 hva， 通过 gup 获取实际上物理地址，
那么，就建立映射了。

- 如果，在运行过程中，user process 调用 brk 扩展了其映射范围，如何 ?
- [ ] 那么可以保证其 gup 的时候的 page fault 成功，否则 ept 无法建立空间.(有待验证)

为了保证什么关系 ?
- 之前内核给 process 分配的物理空间和虚拟空间之间的映射，现在通过 ept 之后，之间映射关系保持不变
- 在初始化的时候，不是拷贝页表，构建一个直接映射作为 guest page table
- 让 ept 去接管 host page table

- [ ] `__setup_mappings_full` 和 `__setup_mappings_precise` 实现细微区别需要理解

- [ ] dune_mmap_addr_to_pa : dune doesn't do a one to one map, in fact, he can do whatever he want, because ept page fault will handle it.

- [ ] GPA_STACK_SIZE : so dune limit user process's virtual address


- [ ] dune_page_init and setup_mappings
   - dune_page_init :
   - setup_mappings : setup mapping from GVA to GPA
    - set up a bigger mapping than need is possible, or just map



## pgfault
In many ways, *transitioning a process into Dune mode is similar to booting an OS.*
The first issue is that a valid page table must be provided before enabling Dune.
*A simple identity mapping is insufficient because, although the
goal is to have process addresses remain consistent before
and after the transition, the compressed layout of the EPT
must be taken into account.*


## pgroot
pgroot 是 mmap 简单的创建一个 page, 是 GVA

通过 dune_vm_lookup 实现 Guest page table 的装配.

- [ ] 一个 process 加载进来的时候，其地址空间的规划是谁负责的 ?
- [ ] glibc 是如何参与进来的，为什么需要修改 glibc

- [ ] page.c 的分配的页面仅仅是 gust page table 的内容吗 ?
  - [ ] 应该是，user process 无法直接控制页面的分配，只能 brk

- [x] 一个程序是不是首先已经启动了，然后被放到 dune 的 ?
  - [ ] 不会，dune 建立了物理地址才会才会启动吧


- [ ] dune 是如何维护 ept 的映射关系的 ?
  - [ ] 什么时候分配 page frame , 此时建立映射关系

- [ ] ept 的 page 的分配位置在哪里 ?
  - [ ] 从 page.c 中间吗 ? 使用用户的 mmap 的空间 ?
  - [ ] 更加可能是，内核模块的 吧


- [ ] 正如 page fault 的原因是 : page table 或者 page 不存在，ept fault 的原因也应该是这两种
- [ ] shadow page fault 处理: 在处理 page table 的时候，那么就需要被捕获，此时就需要分配 page

- [ ] 在 do_dune_enter 中间，将 pgroot 赋值 cr3
  - [ ] 没有一个 GPA 和 HVA 的映射吗 ?

- [ ] 为什么 do_dune_enter 需要使用 汇编，直接 ioctl 不香吗 ?

- [ ] ept.c 中间处理过 A/D bit

- [ ]

## TODO
1. 修复的方法:
https://gitlab.azmi.pl/azmi-open-source/linux-grsecurity/commit/4b1e54786e4862d3110bbfb27999c2c795013007
2. cpu-internals
3. https://github.com/ix-project/dune : 的一整个项目的内容是什么
4. https://medium.com/@jain.sm/hardware-enforced-containers-sandboxes-88fe3113044e : 理论介绍

## 补齐知识
- [ ] gs fs 有什么特殊的
- [ ] 在 KVM 中间，给 guest 的 host page frame 是什么时候分配的

## TODO
- fork 可以让两个 thread 共享空间吗 ? 从 flags 上看是可以的
- 使用 wedge 查看 主动注册 httpd handler 是如何使用的

## 待处理的资源
https://pdos.csail.mit.edu/6.828/2016/lec/faq-dune.txt
