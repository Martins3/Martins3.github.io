- 内核模块的冲突规则是什么?
- mark_oom_victim -> `__thaw_task`
  - 什么 uninterruptable sleep 之类的哇


## TODO
- driver/base 下的代码需要分析一下

openeuler 总结的关于 5.10 内核的提升:
1. 支持调度器优化：优化 CFS Task 的公平性，新增
NUMA-Aware 异步调用机制，在 NVDIMM 初始
化方面有明显的提升；优化 SCHED_IDLE 的调度
策略，可以显著改善高优先级任务的调度延迟，
降低对其他任务的干扰。优化 NUMA balancing
机制，带来更好的亲和性、更高的使用率和更少
的无效迁移。
2. CPU 隔离机制增强：支持中断隔离，支持
unbound kthreads 隔离，增强 CPU 核的隔离性，
可以更好的避免业务间的相互干扰。
3. 进程间通信优化：pipe_wait、epoll_wait 唤醒机
制优化，解决唤醒多个等待线程的性能问题。
4. 内存管理增强：优化内存初始化、内存控制、统
计、异构内存、热插拔等功能，并提供更有效的
用户控制接口。热点锁及信号量优化，激进内存
和碎片整理，优化 VMAP、vmalloc 机制，显著
提升内存申请效率。
5. cgroup 优化单线程迁移性能：消除对 Thread
Group 读写信号量的依赖；引入 Time
Namespace 方便容器迁移。
6. 系统容器支持对容器内使用文件句柄数进行限制：
文件句柄包括普通文件句柄和网络套接字。启动
容器时，可以通过指定 --files-limit 参数限制容器
内打开的最大句柄数。
7. 支持 PSI ：提供了一种评估系统资源 CPU、内存、
数据读写压力的方法。准确的检测方法可以帮资
源使用者确定合适的工作量，帮助系统制定高效
的资源调度策略，最大化利用系统资源，改善用
户体验。
8. TCP 发包切换到了 Early Departure Time 模型：
解决原来 TCP 框架的限制，根据调度策略给数据
包设置 Early Departure Time 时间戳，避免大的
队列缓存带来的时延，同时大幅提升 TCP 性能。
9. 支持 MultiPath TCP 可在移动与数据场景提升性
能和可靠性：支持在负载均衡场景多条子流并行
传输。
10. Ext4 引入一种新的、更轻量级的日志方法：- fast
commit，可以大大减少 fsync 等耗时操作，带来
更好的性能。
11. dm-writecache 特性：提升 SSD 大块顺序写性能， 提高 DDR 持久性内存的性能。
13. IMA 商用增强：在开源 IMA 方案基础上，增强安全性、提升性能、提高易用性，助力商用落地。
14. 支持 per-task 栈检查：增强对 ROP 攻击的防护 能力。
16. MPAM 资源管控：支持 ARM64 架构 Cache QoS 以及内存带宽控制技术。
17. 支持基于 SEDI 和 PMU 的 NMI 机制：使能 hard lockup 检测。使能 perf nmi，能更精确的进行性 能分析。

## 整理一个 blog ，作为一个导航的存在

1. percpu 如何实现，amd64 中间使用 为什么使用 fs(也许是 gs 寄存器实现) 来支持 percpu
3. 多核为锁, 调度器带来何种挑战
4. 为什么会出现从一个 CPU 中被调度出去，从另一个恢复，会出现什么特殊的情况。
5. 多核让 PIC 升级成为了 APIC，我们开始需要分析如何正确负载
6. 多核出现形成了一个新的学科，memory consistency and cache coherency


- [ ] /sys/devices/system/node/node0/ 是如何创建出来的?

## tracing 是自动挂载到 /sys/kernel/debug 下的，还是 kernel 启动之后，手动挂载到上面的

## sar 是如何监控 zram 和 loop devices 的 io 速度的

## 测试了下文件系统和裸盘的性能

这样居然可以达到 400K
```txt
[global]
ioengine=libaio
# ioengine=psync
iodepth=128

[trash]
bs=4k
direct=1
filename=/home/martins3/hack/mnt/x
size=10G
rw=randread
runtime=30000
time_based
```
所以文件系统的极限在哪里?


## 才发现 xfs_address_space_operations 的 read 可以不走 aops 的

```c
const struct address_space_operations xfs_address_space_operations = {
	.read_folio		= xfs_vm_read_folio,
	.readahead		= xfs_vm_readahead,
	.writepages		= xfs_vm_writepages,
	.dirty_folio		= filemap_dirty_folio,
	.release_folio		= iomap_release_folio,
	.invalidate_folio	= iomap_invalidate_folio,
	.bmap			= xfs_vm_bmap,
	.migrate_folio		= filemap_migrate_folio,
	.is_partially_uptodate  = iomap_is_partially_uptodate,
	.error_remove_page	= generic_error_remove_page,
	.swap_activate		= xfs_iomap_swapfile_activate,
};
```

## 逐个选项查看 kernel hacking 的选项的作用

## youtube 上有很多教程
https://www.youtube.com/playlist?list=PLbzoR-pLrL6o8cdq_JLTwsLfe2_DhNsDf

例如如何调试内存，以及各种 rcu 的设计

## 可以阅读的内容
https://lwn.net/Articles/569635/
https://book.douban.com/subject/6799412/ : 内容应该是太老了，但是可以查漏补缺


https://www.uninformativ.de/blog/postings/2022-06-11/0/POSTING-en.html

https://github.com/dynup/kpatch

https://mp.weixin.qq.com/s?__biz=Mzg2MjE0NDE5OA==&mid=2247487871&idx=1&sn=1f320c3043d1cf785320ff3c5d2b6a1e&chksm=ce0d02d6f97a8bc0af97768a575baad61eecd49de2a213e7a7e3e730b527ca9f6b0d169ba0ef&scene=126&sessionid=1696827223#rd

https://mp.weixin.qq.com/s/DDwAAfID8D6h0md9jYAJ6w : GCC features to help harden the kernel

https://occlum.io/

https://mp.weixin.qq.com/s/2kChzwgU-k-DLJdPQROijQ

https://news.ycombinator.com/item?id=37376369

https://news.ycombinator.com/item?id=36580006

https://github.com/HUSTSeclab/crash_deduplication


## 这个写的比较一般，但是用的命令需要看下
https://zhuanlan.zhihu.com/p/463433198
https://zhuanlan.zhihu.com/p/610314077
http://liujunming.top/tags/

## 内核和链接中间的内容
1. `__init`
2. 内核的链接脚本

#### (fs) mpage_readpage 在 chapter 16 介绍过 ?

#### (fs) 找到文件系统对应的设备
在 superblock 中间，在 inode ，以及在 file 中间似乎都是持有 dev_t 的变量，但是
1. 为什么 dev_t 中间为什么不是仅仅放到 superblock 中间，其他变量需要使用直接查询不就可以了吗 ? 检查一下是不是
2. 和设备打交道的也就是 page cache 层次，那么应该在 address_space 中间持有即可
3. 如果块设备需要 page cache 来作为缓冲区，那么字符设备需要 page cache 来缓冲吗 ? 字符设备需要缓冲吗 ?(现在是需要的，printf 换行实现刷新缓冲区)

#### (fs) fs 中间进行查询最简单的实现就是递归算法，但是显然内核是拒绝的
内核避免使用递归的通用方法是什么

#### (fs) inode 和 struct file 是什么关系 ? 是不是 process 打开每一个文件都是需要一个 struct file, 那么，通过 file 可以找到 inode, 但是反向操作不可以实现

#### (dev) 6.5.2 说了什么 plka

Recall from Chapter 6.5.2 that the function
claims a block device for a specific holder (in this case the swap implementation) and signalizes
to other parts of the kernel that the device is already attached to it.

#### (proc) 每一个用户进程是不是对应一个内核线程为其提供 syscall 的服务，是不是每一个用户态线程在内核中间都有其对应的 kernel stack, 同时这些 kernel stack 的生命周期是怎么样的 ?
1. 内核使用进程调用的基本单位是线程还是进程
2. 内核线程调度会将 kernel thread 和 user thread 分开管理吗 ?

#### (misc) 从 do_page_fault 中间的 exception_enter，到 context switch 中间的对于 track 的处理，内核中间处理 track 的一般方法是什么 ?

#### (smp) bootstrap processor 启动的时候，显然是从单个 CPU 中间启动的，我想知道从 scheduler 的角度，其他的 CPU 都是如何启动起来的

#### (smp) possible CPU
If the possible processor is the new terminology for you, you can read more about it the CPU masks chapter. In short words, possible cpus is the set of processors that can be plugged in anytime during the life of that system boot. All possible processors stored in the cpu_possible_bits bitmap, you can find its definition in the kernel/cpu.c:

#### (open) 到底什么信息需要 register( 或者硬件支持, 比如各种 CR 寄存器)，什么只是需要放置到内存中间就可以了
1. 寄存器可以实现高速访问，对于最常用内容，放在寄存器中间 ?
2. 访存会有虚拟地址空间的问题，cr3 这种本身处理地址映射的需要 放到寄存中间, 不过分吧!


#### (misc) 为了让 64 位内核可以运行 32 位的用户程序，内核做出了怎么样的丧心病狂的努力
从 file_operation 定义的 compat 函数到 syscall 中间的
After we have set the entry point for system calls, we need to set the following model specific registers:
- MSR_CSTAR - target rip for the compatibility mode callers;
- MSR_IA32_SYSENTER_CS - target cs for the sysenter instruction;
- MSR_IA32_SYSENTER_ESP - target esp for the sysenter instruction;
- MSR_IA32_SYSENTER_EIP - target eip for the sysenter instruction.

The values of these model specific register depend on the CONFIG_IA32_EMULATION kernel configuration option

#### (int) 中断屏蔽的含义是什么？是 delay 还是 ignore ?
如果 ignore 那么可以写一个程序将所有的 CPU 的中断屏蔽掉，然后测试鼠标和键盘是否响应的操作 ?
if delay, 延迟的信号保存在什么位置了 ?

#### (proc) 当用户进程切换到内核态的时候，从 scheduler 的观点，此时 syscall 产生的
syscall 产生的内核线程
1. 需要被 scheduler 调用吗 ?
2. 可以被 interrupt 打断吗 ?　显然是可以被 exception 处理的，但是内核态的 exception 应该很严重的


更多关于 process 的问题
1. 如何从内核线程产生用户进程的
2. 如果说是通过 fork 来产生进程的，那么我们是通过什么方法产生线程的（应该也是 fork 吧，只是不去复制 mm_struct 之类的东西)，
3. fork 复制的选项有什么东西？
4. 通过 fork 进程之间含有 父子关系，那么，进程和线程之间父子关系有什么不同的地方 ?

更多关于 scheduler 的问题
1. 当一个进程被 interrupt 掉，exception 或者 syscall 的时候， 会通知 scheduler 吗 ?  应该不会，不然不符合中断的设计思想，使用调度器，那就是为了用 ms 规划的(时钟精度 1/HZ), 既然这样，而 interrupt 等都是需要快速回来

内核不是动态分配 CPU 资源，动态分配内存， 而是动态分配计算资源: 分配一个线程，线程中间包含线程执行需要的 stack，head，code(function) 以及 CPU 时间片，各种事情都是需要内核分配计算资源，
从 swap 机制(自己的管理) 到响应硬件(interrupt) 到 brk(用户申请的)，但是不是所有计算都是需要 scheduler 的，只有长时间的，或者需要长时间存在的才需要，比如 work queue ，但是 perishable 瞬间结束，显然没有必要.

设计 syscall 的 philosophy 是什么: 用户态调用内核态函数，内核态的函数返回结果，中间是非常短暂的时间，只是一个裂缝，不会为此为用户进程或者每一个线程特地设置一个对应的内核空间，真实的情况就是，有需求
然后就去申请(stack page frame)，函数调用结束之后，这些资源释放. @todo 我自己的想法，有没有专门给 syscall 使用的内存池就不知道了.

brk 的实现我觉得应该是怎么样子的:
1. 确定 brk 的最低地址，通过查询 mm_struct 之类的实现
2. 向 buddy system 申请空间内存
3. 获取用户进程的 cr3 然后修改其 page table
4. 返回用户进程的虚拟地址
> 应该不是非常难以实现的 ! 虚实地址映射现在唯一的问题在于不知道 : 硬件细节(segment 寄存器的作用到底是什么) 过度过程(ucore 很简单，但是 kernel 似乎不简单)


#### (proc) 程序的启动参数应该放到什么地方 ?
1. 为什么需要程序启动参数 ? 因为 main 函数需要参数
2. 环境变量放置的位置在什么地方，和 main 函数的参数是不是放到一起的(应该是在一起的，不然只会让问题更加复杂化)
3. 为什么程序运行需要 PATH，在什么地方使用过，PATH 中间包含什么内容

> 程序启动的问题难道不能在程序员的自我修养中间仔细分析过吗 ?
从 miniCRT 的过程，编译器可以制定程序的入口为 mini_crt_entry，但是再次的准备活动，esp 和 ebp 数值设置。
感觉初始化 stack 的设置为 :

> 整个 stack 向左行驶:
growing stack |bp |argc| argv poninter | argv | PATH |

所以启动设置细节是什么:
1. mm_struct 创建新 stack 的 region
2. 从 parent 哪里继承 PATH (你确定)
3. 程序开始执行的时候，syscall 比如 execvp 之类必定含有包含 main 函数的参数
4. 将参数复制到 stack 上

#### (block) 显然 disk 和固态使用的驱动不可能是同一个驱动，都使用 gendisk 表示岂不是很尴尬
1. 典型的 block 设备有什么 ? disk sdd floppy 网卡(?)
2. block layer 真的对于抽象和封装　block 设备有效吗 ? 他的假设是什么(推测预取数据，将多次读取合并)
3. 但是似乎 ssd 不像 ? (也许是谣言，也许理解不正确，数据被放到一起不应该是正确的操作吗 ?)


#### (fs) fread 实现机制
fread 需要参数 用户地址空间的指针 和 文件描述符
1. 如果 fread 需要穿过 page cache 层次，那么意味着数据首先拷贝到 page cache 然后才可以拷贝到用户中间
2. 如何将数据传送到用户的地址空间中间，应该也是 copy_to_user 实现的方法了:
    1. 猜测第一个: 当一个函数的参数列表中间含有 `__user` 的时候，意味着该函数一定在 syscall 的调用上，此时，current ,就是正在调用 syscall 的用户
    2. 猜测第一个: `__user` 务必需要指向其中 用户已经映射好的地址空间中，比如分配好的 heap 中间
    3. 由于知道 current , 找到 page director ， 所以可以知道写入的物理地址，然后将事情交给 dma 之类的就可以了

并不是所有的函数都可以持有 `__user` 的参数，它们应该具有什么样子的特点。
#### (int) 在 context switch 的过程中间允许中断吗 ? 应该不允许，因为时间非常的短暂
如果允许，那么应该如何设计 ?

用户程序出现 context switch ，也就是 exception syscall 和 int :
1. 上下文信息保存用户 stack 上 ?
2. 执行上下文切换时候，当时处于内核态 ?

执行上下文切换的代码如何防止破坏上下文，都是一些 push 指令，只会破坏 esp 最后保存 esp 的数值可以计算出来。


#### (proc) set-user-ID 和 set-group-ID 是什么东西 ?
real UID and real GID
effective uesr ID and effective group ID

#### (arch) CS 寄存器，到底是通过什么位置表示 DPL

```c
/*
 * user_mode(regs) determines whether a register set came from user
 * mode.  On x86_32, this is true if V8086 mode was enabled OR if the
 * register set was from protected mode with RPL-3 CS value.  This
 * tricky test checks that with one comparison.
 *
 * On x86_64, vm86 mode is mercifully nonexistent, and we don't need
 * the extra check.
 */
static inline int user_mode(struct pt_regs *regs)
{
#ifdef CONFIG_X86_32
	return ((regs->cs & SEGMENT_RPL_MASK) | (regs->flags & X86_VM_MASK)) >= USER_RPL;
#else
	return !!(regs->cs & 3);
#endif
}
```

segment selector : cs fs gs 之类的东西 o
segment descriptor : cs 在 table 中间项目

segment selector 中间的确含有 rpl 之类的东西，也许是用来作为防备的



#### (process) context switch 的基本单位是 process 还是 thread ? 如果是相同的 process 下的 thread 切换，很多内容是相同，是不是切换可以简化一下？

#### (misc) https://www.linuxjournal.com/article/8023
搜索 context switch 的时候顺便发现的

#### (todo) https://www.cs.kent.ac.uk/people/staff/srk21/research/papers/kell16missing-preprint.pdf


#### (misc) section 在内核的作用是什么 ?
当我们使用链接脚本的时候，现在能够体会的是设置装载的虚拟地址位置从而实现虚拟地址和物理地址做的内核线性映射
但是，链接器的作用显然不仅仅如此，各种类似功能的代码被放到同一个 section 中间，
但是这样做的好处是什么，尚且不清楚


#### (misc) 引入 early_param 机制的作用是什么 ?

#### () 不同时期的日志系统是如何变化的，比如 pr_info 和 printk 的内容
   pr_info("NR_CPUS:%d nr_cpumask_bits:%d nr_cpu_ids:%u nr_node_ids:%d\n",
        NR_CPUS, nr_cpumask_bits, nr_cpu_ids, nr_node_ids);

#### (todo) 抢占的含义是什么？
preempt 现在理解的含义: 时钟中断，计数，到达特定值，切换!

如果想要屏蔽抢断，关中断，岂不是用户程序可以拒绝被抢占 ?

#### (proc) 内核是如何启动第一个内核线程以及用户进程的

```c
static int run_init_process(const char *init_filename)
{
	argv_init[0] = init_filename;
	pr_info("Run %s as init process\n", init_filename);
	return do_execve(getname_kernel(init_filename),
		(const char __user *const __user *)argv_init,
		(const char __user *const __user *)envp_init);
}
```
> init/main.c 中间偶遇一次，跟踪一下，很有意思的

参数是文件，作为 init 进程，再次之前的提供何种支持来保证此时的文件系统是可用的。

#### (misc) 什么 syscall 会访问 user id 和 group id ?

#### (sched) 调度启如何实现两个高运行的出现在不同的 CPU 中间

#### (device) module device device_node 之间的关系都是什么 ?

#### (syscall) 为什么 syscall 可以保证安全，没有替换的方法吗 ?

#### (fs dev) 既然不同的文件系统可以加载到设备上，也就是访问设备提供了一套 interface，那么这个 interface 是什么 ?
1. 能不能利用 /dev/　下的获取到设备之后，绕过文件系统，直接访问设备
    1. 这一个操作只能在内核态完成或者也可以在用户态完成

2. 感觉 fs/block_dev.c 似乎会告诉我们很多关键的信息点。

#### (fs block) block 和 sector 之间是如何装换的 ? block = n * sector 的时候，n 的取值有什么讲究吗 ?


#### (io) iopl ioperm 了解一下

#### (fs) 各种值得分析的文件系统
d_tmpfile


## qemu
在 cpu dump 的时候无法调用 get msr 的 ioctl ，qemu 会卡到内核中，而且啥不掉

/home/martins3/core/qemu/target/i386/cpu-dump.c

```txt
enum pid_type {
	PIDTYPE_PID,
	PIDTYPE_TGID,
	PIDTYPE_PGID,
	PIDTYPE_SID,
	PIDTYPE_MAX,
};
```

## 时不时检索一次
https://zhuanlan.zhihu.com/c_1108400140804726784

## 超线程，可以仔细分析下
- https://stackoverflow.com/questions/23078766/is-hyperthreading-smt-a-flawed-concept

考虑一下，难道这里

- https://github.com/gregkh/kernel-development
- https://github.com/gregkh/kernel-tutorial
- https://www.youtube.com/playlist?list=PLVsQ_xZBEyN0daQRmKO4ysrjkSzaLI6go
- http://www.hackdig.com/01/hack-881330.htm
- https://www.redhat.com/en/topics/microservices/what-is-a-service-mesh

- https://blog.goodaudience.com/understanding-zero-knowledge-proofs-through-simple-examples-df673f796d99

## 有时间回答这个问题
https://stackoverflow.com/questions/77852699/how-does-irqfd-trigger-interrupt-in-the-guest

## 非主线 patch
https://github.com/CachyOS/kernel-patches

## 如果可以用 golang 实现一个操作系统吗?
golang 的 gc 如何执行?

## 这个工作有趣的，要是可以直接集成到 ci 就可以了
https://ieeexplore.ieee.org/document/10179356

几个作者在 kernel 中有很多提交 fix

还有类似的工作吗?

## 这个做什么的
```txt
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
overlayfs: unrecognized mount option "volatile" or missing value
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
skbuff: enp177s0f1np1: received packets cannot be forwarded while LRO is enabled
```
https://access.redhat.com/solutions/20201

## 有趣的错误排查
```txt
[12574.685163] NOHZ: local_softirq_pending 10
[15152.484041] NOHZ: local_softirq_pending 08
[17174.874028] perf: interrupt took too long (4924 > 4920), lowering kernel.perf_event_max_sample_rate to 40000
[18287.494149] NOHZ: local_softirq_pending 08
[42880.729182] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[42893.063327] NOHZ: local_softirq_pending 10
[44171.647017] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[44489.266846] NOHZ: local_softirq_pending 10
[44845.383857] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[45883.452037] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[47356.953685] NOHZ: local_softirq_pending 202
[51824.083572] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[56673.137690] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[57201.874517] NOHZ: local_softirq_pending 10
[61393.867015] NOHZ: local_softirq_pending 10
[69815.534417] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[69815.534446] openvswitch: ovs-system: deferred action limit reached, drop recirc action
[74170.557353] NOHZ: local_softirq_pending 202
[76646.089523] NOHZ: local_softirq_pending 08
```

```diff
commit bca37119c57bdc2c68c84b313a5118005e8693cf
Author: Paul E. McKenney <paulmck@kernel.org>
Date:   Fri Jun 26 13:39:41 2020 -0700

    tick-sched: Clarify "NOHZ: local_softirq_pending" warning

    Currently, can_stop_idle_tick() prints "NOHZ: local_softirq_pending HH"
    (where "HH" is the hexadecimal softirq vector number) when one or more
    non-RCU softirq handlers are still enabled when checking to stop the
    scheduler-tick interrupt.  This message is not as enlightening as one
    might hope, so this commit changes it to "NOHZ tick-stop error: Non-RCU
    local softirq work is pending, handler #HH".

    Reported-by: Andy Lutomirski <luto@kernel.org>
    Cc: Frederic Weisbecker <fweisbec@gmail.com>
    Cc: Thomas Gleixner <tglx@linutronix.de>
    Cc: Ingo Molnar <mingo@kernel.org>
    Signed-off-by: Paul E. McKenney <paulmck@kernel.org>
```

## 过于有趣的问题
- https://www.reddit.com/r/linux/comments/1f3q0l8/one_of_the_rust_linux_kernel_maintainers_steps/

## pual 的 blog
https://paulmck.livejournal.com/

## dune 的同门项目
https://www.usenix.org/system/files/conference/osdi14/osdi14-paper-belay.pdf

ifso 中提到的:

> Operating-system kernels adapted for networking,
where each connection (also called flow [DKS89,
Zha89, McK90]) is assigned to a specific thread. One
recent example of this approach is the IX operating
system [BPP+16]. IX does have some shared data
structures, which use synchronization mechanisms
to be described in Section 9.5.


## 到底什么是 auxiliry bus ?
https://docs.kernel.org/driver-api/auxiliary_bus.html
到时候这里整理到那里吧!

## 不容易的
http://liujunming.top/2022/03/29/Introduction-to-Intel-I-OAT/

## 将这个整理到哪里去
- https://wiki.osdev.org/I/O_Ports
- https://stackoverflow.com/questions/14194798/is-there-a-specification-of-x86-i-o-port-assignment

总算是知道了

## 有趣的东西，都需要看看
https://www.openeuler.org/whitepaper/en/openEuler%2022.03%20LTS%20SP2%20Technical%20White%20Paper.pdf

## 这个写的真的很好
https://os.phil-opp.com/

至少解释 irq 的部分:
用于理解 CPU 这边的底层，一共三篇文章

## 想到了一个问题，如果没 vduse 之类的，那么 k8s 的存储如何接入
也是通过 fuse 吧，直接使用文件系统的功能

## 这个做什么的?
https://github.com/kernkonzept/fiasco

## 看看
https://github.com/bcoles/kasld

## kernel bugzilla 中的东西也是需要时不时浏览一下的
https://bugzilla.kernel.org/show_bug.cgi?id=218267


## 艺术欣赏
https://news.ycombinator.com/item?id=41600756

## https://github.com/skiffos/SkiffOS

## 有趣的文档，这里显示了 intel 对于未来的展望
- https://software.intel.com/sites/default/files/managed/c5/15/architecture-instruction-set-extensions-programming-reference.pdf

https://book.douban.com/subject/26820213/

https://news.ycombinator.com/item?id=42636086

## GPU 中为什么会有 i2c ？
```txt
vn on  master 🥝
🤒  find /sys -name "i2c*"
find: ‘/sys/kernel/tracing’: Permission denied
find: ‘/sys/kernel/debug’: Permission denied
/sys/class/i2c-adapter
/sys/class/i2c-adapter/i2c-1
/sys/class/i2c-adapter/i2c-0
/sys/devices/platform/HISI02A2:00/i2c-0
/sys/devices/pci0000:00/0000:00:11.0/0000:03:00.0/i2c-1
find: ‘/sys/fs/pstore’: Permission denied
find: ‘/sys/fs/bpf’: Permission denied
/sys/bus/platform/drivers/i2c_designware
/sys/bus/i2c
/sys/bus/i2c/devices/i2c-1
/sys/bus/i2c/devices/i2c-0
/sys/module/i2c_designware_platform
/sys/module/i2c_designware_core
/sys/module/i2c_designware_core/holders/i2c_designware_platform
/sys/module/i2c_algo_bit
/sys/module/ipmi_ssif/drivers/i2c:ipmi_ssif
vn on  master 🥝
🤒  lspci -s 0000:03:00.0
03:00.0 VGA compatible controller: Huawei Technologies Co., Ltd. Hi171x Series [iBMC Intelligent Management system chip w/VGA support] (rev 01)
```

## 不相关的
- 将 fun.md 和 fav.md 中链接都检查一下
- https://www.usenix.org/conference/osdi24/presentation/chen-haibo : 中式英语，太好了
  - https://zhuanlan.zhihu.com/p/708244292
  - https://www.zhihu.com/question/661246661/answer/3558041765
    - 这个也是需要看看的

## https://aquasecurity.github.io/tracee/v0.21/docs/events/builtin/syscalls/fsopen/

## 有一个想法，可以按照一个文件最后被修改的时间来排序文件或者目录
例如对于 Documentation 中内容排序，最后看下哪些 doc 都是过期的

## 总结的很好
https://mp.weixin.qq.com/s/nwwCReagfzmby491BSF6KA

## qemu gdb server 需要添加一个 lx-current 的功能才好

## 这个做什么的?
tools/verification/

## 无则加勉
https://docs.qualcomm.com/bundle/publicresource/topics/80-70015-12/debugging_linux_kernel.html

## 提一个需求

这种 callchain 调用 callchain ，有办法把他们都连到一起吗?
```txt
@[
    kvmclock_update_fn+97
    process_one_work+399
    worker_thread+560
    kthread+220
    ret_from_fork+49
    ret_from_fork_asm+26
]: 192
```
## 这个日志是什么意思
```txt
[    1.503602] input: AT Translated Set 2 keyboard as /devices/platform/i8042/serio0/input/input1
[    1.529372] RAPL PMU: API unit is 2^-32 Joules, 0 fixed counters, 10737418240 ms ovfl timer
[    1.918150] input: ImExPS/2 Generic Explorer Mouse as /devices/platform/i8042/serio1/input/input3
```

## 还不错，可以收集一下
https://www.zhihu.com/people/nobody_know/columns

## 思考一下，asahi linux 是如何调试硬件的，既然他们都是没有串口

## 有趣的
https://lore.kernel.org/kvm/20250313064743.GA10198@lst.de/T/#m61c7b8e571122a1de42b79487d4910a3fe44f567

## 他的内容都看下
https://mp.weixin.qq.com/s/Eyeq3UBBr1pZY_qgxLPdkA

https://mp.weixin.qq.com/s/qMd_xCB4D_hmoGi_ERuR8Q
https://mp.weixin.qq.com/s/NRn7vYazmVvNIOaMvJ-o8Q
https://mp.weixin.qq.com/s/jhA2pk8cj7QKeFadLB-efQ

## 这个手册好啊
https://software.intel.com/content/www/us/en/develop/download/intel-architecture-instruction-set-extensions-programming-reference.html

一篇论文讲透Cache优化 - 红星闪闪的文章 - 知乎
https://zhuanlan.zhihu.com/p/608663298

## 测试一下 drivers/vhost/scsi.c

## 这几个模块都看看做什么的?
- kernel/trace/rv/monitors/sco/Kconfig
- tools/tracing/rtla/src/osnoise.c
- tools/verification/dot2/dot2k

## 还是应该搞一个板子玩玩，看看 ejtag 是如何工作的

他的两个 blog 都不错:

https://blog.74ls74.org/2025/02/01/20250201_openocd_debug_raspi5_by_raspi1/
https://blog.74ls74.org/2022/03/30/20220330_debug_raspberry_pi_bcm2835_armv6_linux_kernel_J-Link_jtag_gdb/


## 有的函数带有这个后缀是什么意思?
ping_lookup.isra.0 这里的 isra 是什么意思?

## 有趣的东西
https://github.com/cowtoolz/webcamize

理解一下 /dev/video0 是做什么的?

## 有趣的分析
https://www.zhihu.com/question/1908531885698683097

## 32 bit 的支持
https://mp.weixin.qq.com/s/3KMs2Qf0xIysNQeE61It_A

难道嵌入式中不是全部都是 32bit 的吗？

## 现在发现两个问题
如果是由于 selinux 或者 firewalld 导致的问题，一般都不知道咋解决

## 看看
https://mp.weixin.qq.com/s/PN8NKA8-XnHqUyQjDk7o3Q

## 使用这个，执行会有这个告警
/home/martins3/data/vn/code/src/hpc/avx2.S
[  372.641921] process 'code/src/hpc/a.out' started with executable stack

## robot os
https://mp.weixin.qq.com/s/XAEYiS86D5wJlTarFv5mZA

https://mp.weixin.qq.com/s/mynYtDWiVLytYXRxHiQP7Q

ros2
## 原来 apple os 的内核是开源的
https://github.com/apple-oss-distributions/xnu

## lkrg
https://mp.weixin.qq.com/s/erWoY2KugM_AXSYjAmOHqA


https://mp.weixin.qq.com/s/jbLCTvRBuECmfx9K24hQIQ

## 看看这个问题

|--------------------|-----|-----|-----|-------------------------------------------------------------------------------------------------------------------------------------------|
| padata.c           | 207 | 186 | 719 | 应该是并行计算的                                                                                                                          |
| relay.c            | 180 | 290 | 861 | See Documentation/filesystems/relay.txt for an overview.                                                                                  |
| profile.c          | 75  | 70  | 420 | http://www.pixelbeat.org/programming/profiling/　具体内容不知道，很有可能是 sched 的 debug 工具                                           |
| umh.c              | 85  | 184 | 417 | https://kernelnewbies.org/KernelProjects/usermode-helper-enhancements @todo 尚且没有阅读                                                  |
| jump_label.c       | 127 | 116 | 557 | https://lwn.net/Articles/412072/                                                                                                          |
| tsacct.c           | 21  | 45  | 119 | System accounting over taskstats interface 　进程的统计                                                                                   |
| extable.c          | 22  | 61  | 93  | exception table 处理的辅助函数                                                                                                            |
| freezer.c          | 30  | 61  | 90  | Function to freeze a process                                                                                                              |
| cpu_pm.c           | 21  | 106 | 82  | cpu 相关的电池管理                                                                                                                        |
| dma.c              | 29  | 43  | 77  | A DMA channel allocator https://www.kernel.org/doc/html/v4.19/driver-api/dmaengine/provider.html @todo 难道 dma 全部的 interface 在此处 ? |
| kcmp.c             | 40  | 29  | 187 | compare two processes to determine if they share a kernel resource                                                                        |
| context_tracking.c | 29  | 73  | 116 | 感觉是一个 debug 机制                                                                                                                     |
| ucount.c           | 28  | 13  | 205 | 应该是处理 namespace 的相关的，不清楚                                                                                                     |
| latencytop.c       | 46  | 78  | 182 | @todo 有注释                                                                                                                              |
| cred.c             | 101 | 190 | 527 | Task credentials management                                                                                                               |
| acct.c             | 64  | 146 | 396 | acct 系统调用                                                                                                                             |
| delayacct.c        | 26  | 27  | 118 | 进程统计                                                                                                                                  |
| sys_ni.c           | 100 | 102 | 235 | ni 是什么意思，提供统一的 syscall 接口 ?                                                                                                  |
| user_namespace.c   | 199 | 290 | 838 | https://en.wikipedia.org/wiki/Linux_namespaces　@todo 所以 user namespace 实现什么东西的虚拟化 ?                                          |

## ai 自动 review 代码
https://sashiko.dev/

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
