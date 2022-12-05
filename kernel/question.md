# 输出才是最好的学习方式
1. 为什么需要使用fork 机制来产生process,难道没有更加好的方法吗，比如所有的进程从init 进程获取
2. 内核和用户空间传输数据没有更好的方法吗 ? 为什么连一个文件名都是需要拷贝的。

## 内核和链接中间的内容
1. `__init`
2. 内核的链接脚本


## 锁的使用原理是什么
在单核，多核上　锁的实现机制有什么不同的地方 ?

## 为什么说内核设计的好
1. C 语言编程需要什么范式　或者　规范吗 ?

## What
1. `CONFIG_MIGRATION`到底是什么东西 ?
2. asmlinkage 　???

可以有静态的函数，以及静态的变量，但是是没有静态的typedef 当在一个文件中定义类型之后，似乎无法在其他文件重新定义类型
> 并不是，类型定义仅仅对于下文负责，也就是说，所以的类型定义都是静态，我们只是不能在同一个文件中间定义两次同名类型


## 内核的build 系统到底如何工作的
1. modules.buildin 文件是做什么用的
2. autoconf.h 是如何生成的 ?
3. scripts 文件夹下有大量好用的工具，比如生成tag　的 tags.sh
4. 如果想要向内核中间　添加一个新的　代码，如果写Makefile


## 文件和文件夹
1. samples 文件夹的代码　作用是什么
2. tools 中间　主要做什么的
3. lib 和 tools 有什么区别吗 ?


## 资源
1. 有一张内核调用的[图](https://makelinux.net/kernel_map/)，或者自己做一张这样的图。
2. 除了这一个　[子模块](https://linux-mm.org/)，还有没有其他的模块。
#### (process) struct pid  的作用是什么


#### (mem) slab cache 的作用是什么 ?
cache 的含义在此处是指预留空间吗？应该是和slab 的实现相关的，只是名字恰好相同

#### (fs) register_filesystem 做什么工作，为什么需要将filesystem 管理起来，管理的数据结构叫什么

#### (fs) proc_fs_type 其他类型的都是如何注册的 ? struct file_system_type 如何实现管理的 ？

#### (fs) 文件系统和设备的链接处在哪里 ?
从inode 中间的dev_t 到 file_system_type 中间的 struct module * owner。
为什么两者之间的connection 如此之多，如果dev 和 fs 只有一个沟通环节不是更好吗 ?
文件系统是和具体的设备挂钩的，superblock 中间持有一个 dev_t 就完全足够，但是实际上，整个文件系统的中间的所有的inode 都是需要只有dev_t(也许)
这很不优雅!


#### (proc) proc 文件系统的操作在ext4 中间含有对称的部分吗 ？

#### (vfs) vfs 实现了那些功能，那些功能是vfs 无法实现的
vfs 不仅仅提供了所有文件系统通用实现的通用功能，比如上层的查询函数，而且处理了和process 的打交道的问题:
1. 同一个文件如果被多个进程使用，怎么办 ? 通过file

#### (misc) subsystem 的具体定义是什么 ？ 在代码中间如何体现subsystem 的范围

#### (fs) 整理一下一个文件系统的关键的要素是什么，file_operations inode_operation ，和设备文件打交道的接口，mount
1. 什么开始增加了文件系统的复杂性: 锁机制，缓存，软硬链接

#### (process) tgid 和pid 是什么关系

#### (fs) filp 和 inode 是如何挂钩起来的 ?

#### (fs) dentry 是怎么回事，为什么在dcache 中间定义?
dentry 描述directory entry, 为什么其功能不直接放到inode 中间，
反正inode 同时描述 dir 和 file

#### (fs) 据说，理解debugfs 的非常的clean 而且 simple 和 well documented
这是成为其他操作系统对照的基础

#### (fs) address_space 的角色到底是什么，process 有，fs有，swap 有 !
1. 为什么我们抽象出来address_space 这一个对象出来 ?
2. 为什么在分析 proc 以及 syfs 等文件系统的时候，就没有介绍过address_space了

#### (fs) mpage_readpage在chapter 16介绍过 ?

#### (fs) address_space 实现管理page cache 和 检查page 在各个进程的位置，是如何实现此功能的

#### (fs) 找到文件系统对应的设备
在superblock 中间，在inode ，以及在file 中间似乎都是持有dev_t 的变量，但是
1. 为什么dev_t 中间为什么不是仅仅放到superblock 中间，其他变量需要使用直接查询不就可以了吗 ? 检查一下是不是
2. 和设备打交道的也就是page cache 层次，那么应该在address_space 中间持有即可
3. 如果块设备需要page cache 来作为缓冲区，那么字符设备需要page cache 来缓冲吗 ? 字符设备需要缓冲吗 ?(现在是需要的，printf 换行实现刷新缓冲区)


#### (fs) fs中间进行查询最简单的实现就是递归算法，但是显然内核是拒绝的
内核避免使用递归的通用方法是什么

#### (fs) inode 和struct file 是什么关系 ? 是不是process 打开每一个文件都是需要一个struct file, 那么，通过file 可以找到inode, 但是反向操作不可以实现。

#### (fs) file_operations 的主要作用是什么东西?
总结一下: inode_operation dir_operation 等操作的意图是什么 ?

#### (dev) def_blk_fops 和 def_blk_aosp的作用分别都是如何使用的

#### (mem) lru cache 是什么东西？

#### (fs/mem) page cache 的作用到底是什么，是不是所有文件的的读入到内中间都是基于page cache 的?

#### (fs) page cache 和 swap 的关联是什么 ? 是两套并行的机制，还是page cache 是运行在swap 上的

#### (fs) page cache 和 page fault 的关联是什么?

#### (mem) map 的类型划分到底如何定义？
private shared
file-based ?

#### (lock) 为什么执行某些代码的时候不可以被interrupt, 某些代码不可以sleep ?
1. interrupt 的本质是什么，和lock 的联系是什么 ?
2. 多核给 interrupt 带来了什么挑战 ?

#### (int) 当在interrupt 或者 kernel thread 的时候发生page fault 由于没有上下文而会发生错误?

#### (mem) lru cache 到底是什么cache

#### (mem) 为什么内核会发生page fault, 如果内核中间数据都是关键数据，那么内核中间的数据为什么会被换出啊?

#### (mem) 内核提供了丰富的操作vma的区间合并，拆分等操作的支持，那么什么时候，我们需要进行这些操作，难道不应该是只是vma的添加和删除不就可以了吗?

#### (process) 进程的切换的context switch 和 使用syscall 的　context switch 含有代码复用吗?
1. 必定复用了代码
2. 还是process switch 的过程中间的一种　为　syscall 版本的
3. context switch 的核心位置在什么地方:
    1. user kernel 切换，user user 切换，kernel kernel 切换有什么相似不同的地方
    2. user to kernel switch 的安全问题是如何解决的，以及我们是如何保护内核空间

#### (mem) reclaim 是什么含义
将数据占据的物理内存重新利用，和swap out 的含义有什么区别 ?

#### (mem) find_get_page
我在不停看到这一个函数，但是没有时间搞懂它

#### (io) async IO 在用户态内核是如何支持的实现的，在内核态是如何实现的aio的

#### (swap) 虽然page cache实现使用内存缓存disk, 但是这些page 什么时候被放到disk 和 swap　分区中间，需要swap 机制完成，所以是不是说，swap机制发

#### (swap) pagevec 和 lru list 是什么关系 ?

#### (mem) 18讲解将物理页面刷新到磁盘中间，所以ch17 中间的pdflush 的工作是什么 ?

#### (swap) swap fs 的结构是什么样子，为了 efficient 和 功能，如何调整以及和一般的fs 相区分的

#### (swap) swap partition 和 swap file 是什么关系 ?

#### (dev) swap_info_struct 同时包含file 和 bdev 两个设备，如果他想要和一个设备沟通，那么直接持有一个bdev即可，如果一直使用file 是进程用于封装inode的假设，这是不可能的 ?
重回chapter 8 中间的，将file 及其相关的描述加以分析，并且总结file inode address_space 的各种operations

#### (swap) swap_info 为什么需要建立成为一个数组，为了实现类似于per cpu 的效果，实现对于disk 的并发访问

#### (dev) 6.5.2 说了什么 plka

Recall from Chapter 6.5.2 that the function
claims a block device for a specific holder (in this case the swap implementation) and signalizes
to other parts of the kernel that the device is already attached to it.

#### (proc) 总结一下process 需要关注的内容
1. 内核线程和用户线程的生活空间有什么不同 ?
2. 切换
3. 内核守护进程添加了什么不同的东西使其成为了守护进程

#### (sysctl) ctl_table 是做什么使用的 ?
int drop_caches_sysctl_handler

#### (swap) swap 中间的reclaimer 是什么东西 ?
```
		/* wait a bit for the reclaimer. */
```
#### (smp) smp_mb() 实现原理完全看不懂啊 ?
```c
#define __smp_mb()	asm volatile("lock; addl $0,-4(%%rsp)" ::: "memory", "cc")
```

#### (mem) 在分析swap 中间的时候，以前表示page 的属性采用 active 和 inactive ，但是如今进一步被划分成为 inactive file , inactive anon
1. swap 机制为什么要区分file 和 anon 的内容，真的是因为file 是被刷新到ext4 而不是　swap fs 中间的吗 ?
2. page cache 是如何划分page cache 的　？

#### (mem) node_data 初始化细节是什么
#### (mem) migration 对于swap 的影响是什么 ?

#### (doc) Document 的中文文档进行阅读 ? 或者首先通过提交doc 维持生活 ?

#### (mem) address space 和一个inode 对应，找到其初始化的位置在什么地方 ?
In struct address_space, for example, there is a radix tree called page_tree that tracks the in-memory page-cache pages that are associated with a given inode.

#### (mem) address space 对于文件来说非常容易，但是对于anon 就并不简单，因为没有文件了，除非当同一个page frame 被 shared 的时候，其所在的vma不会被拆分，或者shared 形成的原因是fork , 地址空间相同，总之，保持索引值一致

#### (mem) page cache 中间的页面是内核态还是用户态的 ?

#### (mem) stack 的问题总结
1. 程序启动的参数放在哪里？什么栈 环境变量存放的位置在什么地方(CSAPP 提到过，PA试验, 进程初始化的过程是什么)
2. 进程切换的时候，stack 是如何切换的
3. 当syscall 的时候，stack 如何切换，和上面的切换有什么区别和不同之处
4. 如果stack不够使用的时候，通过什么方法大小。因为stack 也是vm_area 管理的，vm_area 的初始化　大小之后就是没有办法扩展吗?

到底切换stack麻烦的在于，函数执行需要stack的支持，但是切换stack 是通过调用函数实现的，实现切换的过程真空期如何处理。

#### (mem) 物理映射方法
1. 找到page frame 和 struct page 通过地址差互相访问的代码
2. 既然不同的用户进程之间的虚拟地址空间都是可以相互重叠的，那么为什么用户和内核的虚拟地址空间就是不能互相重叠的
3. 线性地址空间
    1. 如何初始化的线性地址空间 : 将预定义的pg table 进行填充，之后所有的进程全部从其中fork
    2. linear space 带来的结果 : 在内核空间中，当持有了其物理地址，那么可以知道其虚拟地址。物理地址的差值等于虚拟地址的差值。
    3. 什么机制的实现必须依赖于 linear space ? 找到哪一个实现地址宏的使用的位置即可!
4. fork 拷贝pg table，到底是拷贝整个树，还是仅仅拷贝dir 的2k地址。如果是拷贝了整个树，那么如何确定谁的内容是谁的
5. fork 拷贝dir, 内核地址空间的映射有意义吗? 反正用户无法访问!
6. 地址空间的保护靠什么实现的, pte 上面的flag 吗，寄存器中间的标志位和预设的虚拟地址空间阈值?
8. 通过linear mapping 的确可以知道虚拟地址对应的物理地址，所获取的物理地址具体数值其实没有任何意义的吧 ? 就算获取了之后，没有什么意义啊!
    1. 当没有线性映射的时候，那么，通过

9. 物理地址到虚拟地址之间的切换操作是什么?

10. 如果buddy system 和 slab 分配器开始工作的时候，它们返回的地址显然是虚拟地址，分配器
11. 内核的虚拟地址空间必须映射整个物理内存，否则，buddy System 无法管理内存，那么它是如何将整个物理内存映射的(仅仅考虑64位)
12. 从目前的分析来说，线性映射显然是需要的，内核需要连续的物理内存来提升性能，但是他应该是透明的才对啊!
    1. 如果这一种想法正确的话，那么内核中间就不该有 虚实地址转换函数 存在啊!
13. 一种说法是x86的线性地址只是为了兼容而已，在arm中间这些蛇皮限制是不存在的。
14. 如果没有线性地址空间，是不是意味着没有办法进行context switch 中间修改pgdir 的操作 ?
    1. cr3 中间存放的是物理地址还是虚拟地址, 显然是物理地址

15. 需要phys_to_virt and it's reverse 的唯一原因是: 我们需要物理地址。几乎没有需要物理地址的时间，但是在整个pgtable 填充的过程中间，
填充的内容都是物理地址，同时，page_alloc 返回空间是虚拟地址，所以需要virt_to_phys 的数值


#### (mem) https://www.kernel.org/doc/html/v4.14/dev-tools/kasan.html
KASAN 的工作原理是什么 ?

#### (arch) 同时编译一份RISC-V 的代码
1. 交叉编译
2. ccls 处理交叉编译
3. defconfig

#### (todo) https://read.seas.harvard.edu/cs161-19/
harvard 的使用C++ 的多核操作系统课程。

#### (ccls) strncpy_from_user 无法查找其定位，最后在lib 中间找到，但是该函数的引用不对

#### (proc) 每一个用户进程是不是对应一个内核线程为其提供syscall 的服务，是不是每一个用户态线程在内核中间都有其对应的kernel stack, 同时这些kernel stack 的生命周期是怎么样的 ?
1. 内核使用进程调用的基本单位是线程还是进程
2. 内核线程调度会将kernel thread 和 user thread 分开管理吗 ?


#### (proc) current 指针只能指向用户进程，当内核线程运行的时候，其为null 指针


#### (mem) 内核是如何写pde 和 pte 的, 在page_fault 中间，当每一级都不存在，将整个page walk 逐级填充的过程 ?
> 当不采用 linear address space 的时候

内核读写操作数都是虚拟地址，持有物理地址无法进行修改内存。获取了page walk 的第一级，进而获取page walk 第二级是没有办法的，


由于page_fault 的整个过程都是软件处理的，如果不是 linear address space ,　那么显然page walk 的填充过程无法完成。

#### (mem) VPT机制非常的秀，并不清楚在内核中间是含有对应的实现
在ucore 中间的唯一的用途也只是仅仅限于 print_pgdir 中间

#### (misc) 学习inline assembly
https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html

#### (mem) stack 空间切换的本质是什么，切换之后，stack push开始地址不同而已

#### (arch) 找一门x86_64和RISC-V64 的汇编课程上一下
现在在arch 层次到处碰壁

#### (arch) https://en.wikipedia.org/wiki/Long_mode
long mode，protected mode，real mode 总结一下，
https://0xax.gitbooks.io/linux-insides/content/Interrupts/linux-interrupts-2.html
中间描述，从startup_32 到 startup_64 中间变化

#### (arch) entry_64.S 和 head_64.S 简直是一生之敌

#### (arch) https://en.wikipedia.org/wiki/X86_memory_segmentation
x86_memory_segmentation 分析问题首先确定当前处于何种模式

#### ()

#### (int) 一致认为中断的来源非常多，但是这是被除0 exception 误导的，感觉一个终端号处理事情有限，但是实际上整个syscall 也只是占用一个终端号而已



#### (misc) 从do_page_fault 中间的 exception_enter，到context switch 中间的对于track 的处理，内核中间处理track 的一般方法是什么 ?

#### (io) 当读写文件阻塞了，CPU会切换进程，那么内核判断是否切换进程的标志是什么?

#### (hardware) 虽然TLB 和 cache 号称是程序员透明的，但是实际上内核提供了众多机制来处理cache 和 TLB，为什么需要操纵TLB 和 cache，操纵的时间点在什么位置，切换进程 ?　切换特权级吗？

#### (stack) https://www.kernel.org/doc/Documentation/x86/kernel-stacks
> 不是一直抱怨没有stack的总结吗?

#### (numa) first_online_code  如何定义online node ?

#### (smp) bootstrap processor 启动的时候，显然是从单个CPU中间启动的，我想知道从 scheduler 的角度，其他的CPU都是如何启动起来的

#### (int) 永远都不知道当前的代码的执行需不需要关闭中断？

#### (smp) possible CPU
If the possible processor is the new terminology for you, you can read more about it the CPU masks chapter. In short words, possible cpus is the set of processors that can be plugged in anytime during the life of that system boot. All possible processors stored in the cpu_possible_bits bitmap, you can find its definition in the kernel/cpu.c:

#### (open) 到底什么信息需要register( 或者硬件支持, 比如各种CR寄存器)，什么只是需要放置到内存中间就可以了
1. 寄存器可以实现高速访问，对于最常用内容，放在寄存器中间 ?
2. 访存会有虚拟地址空间的问题，cr3 这种本身处理地址映射的需要 放到寄存中间, 不过分吧!
3.

#### (misc) 为了让64位内核可以运行32位的用户程序，内核做出了怎么样的丧心病狂的努力
从 file_operation 定义的 compat 函数到 syscall 中间的
After we have set the entry point for system calls, we need to set the following model specific registers:
- MSR_CSTAR - target rip for the compatibility mode callers;
- MSR_IA32_SYSENTER_CS - target cs for the sysenter instruction;
- MSR_IA32_SYSENTER_ESP - target esp for the sysenter instruction;
- MSR_IA32_SYSENTER_EIP - target eip for the sysenter instruction.

The values of these model specific register depend on the CONFIG_IA32_EMULATION kernel configuration option

#### (int) 中断屏蔽的含义是什么？是delay 还是 ignore ?
如果ignore 那么可以写一个程序将所有的CPU的中断屏蔽掉，然后测试鼠标和键盘是否响应的操作 ?
if delay, 延迟的信号保存在什么位置了 ?


#### (todo) https://people.kernel.org/metan/towards-parallel-kernel-test-runs

#### (proc) 当用户进程切换到内核态的时候，从 scheduler 的观点，此时syscall 产生的
syscall 产生的内核线程
1. 需要被scheduler 调用吗 ?
2. 可以被 interrupt 打断吗 ?　显然是可以被exception 处理的，但是内核态的 exception 应该很严重的


更多关于 process 的问题
1. 如何从内核线程产生用户进程的
2. 如果说是通过fork 来产生进程的，那么我们是通过什么方法产生线程的（应该也是fork 吧，只是不去复制 mm_struct 之类的东西)，
3. fork 复制的选项有什么东西？
4. 通过fork 进程之间含有 父子关系，那么，进程和线程之间父子关系有什么不同的地方 ?

更多关于 scheduler 的问题
1. 当一个进程被 interrupt 掉，exception 或者 syscall 的时候， 会通知 scheduler 吗 ?  应该不会，不然不符合中断的设计思想，使用调度器，那就是为了用ms 规划的(时钟精度 1/HZ), 既然这样，而interrupt 等都是需要快速回来

内核不是动态分配 CPU 资源，动态分配内存， 而是动态分配计算资源: 分配一个线程，线程中间包含线程执行需要的stack，head，code(function) 以及 CPU 时间片，各种事情都是需要内核分配计算资源，
从swap 机制(自己的管理) 到响应硬件(interrupt) 到 brk(用户申请的)，但是不是所有计算都是需要 scheduler 的，只有长时间的，或者需要长时间存在的才需要，比如work queue ，但是 perishable 瞬间结束，显然没有必要.

设计syscall 的 philosophy 是什么: 用户态调用内核态函数，内核态的函数返回结果，中间是非常短暂的时间，只是一个裂缝，不会为此为用户进程或者每一个线程特地设置一个对应的内核空间，真实的情况就是，有需求
然后就去申请(stack page frame)，函数调用结束之后，这些资源释放. @todo 我自己的想法，有没有专门给syscall 使用的内存池就不知道了.

brk 的实现我觉得应该是怎么样子的:
1. 确定brk 的最低地址，通过查询mm_struct 之类的实现
2. 向buddy system 申请空间内存
3. 获取用户进程的cr3 然后修改其page table
4. 返回用户进程的虚拟地址
> 应该不是非常难以实现的 ! 虚实地址映射现在唯一的问题在于不知道 : 硬件细节(segment 寄存器的作用到底是什么) 过度过程(ucore 很简单，但是kernel 似乎不简单)


#### (proc) 程序的启动参数应该放到什么地方 ?
1. 为什么需要程序启动参数 ? 因为main 函数需要参数
2. 环境变量放置的位置在什么地方，和main 函数的参数是不是放到一起的(应该是在一起的，不然只会让问题更加复杂化)
3. 为什么程序运行需要PATH，在什么地方使用过，PATH 中间包含什么内容

> 程序启动的问题难道不能在程序员的自我修养中间仔细分析过吗 ?
> 从miniCRT 的过程，编译器可以制定程序的入口为 mini_crt_entry，但是再次的准备活动，esp 和 ebp 数值设置。
> 感觉初始化stack 的设置为 :

> 整个stack 向左行驶:
growing stack |bp |argc| argv poninter | argv | PATH |

所以启动设置细节是什么:
1. mm_struct 创建新 stack 的region
2. 从parent 哪里继承PATH (你确定)
3. 程序开始执行的时候，syscall 比如 execvp 之类必定含有包含main 函数的参数
4. 将参数复制到stack 上
#### (block) 显然disk 和固态使用的驱动不可能是同一个驱动，都使用gendisk 表示岂不是很尴尬
1. 典型的block 设备有什么 ? disk sdd floppy 网卡(?)
2. block layer 真的对于抽象和封装　block 设备有效吗 ? 他的假设是什么(推测预取数据，将多次读取合并)
3. 但是似乎ssd 不是数据分离的吗 ? (也许是谣言，也许理解不正确，数据被放到一起不应该是正确的操作吗 ?)


#### (misc) control group 是什么回事 ?
1. control group 在应用层和内核说的一个东西吗？
2. 内核中间主要出现的位置是什么, 现在知道swap 分区中间编译选项控制，恐怖的一匹
3. control group 是在控制一组thread 中间的 CPU mem 等使用量吗 ?
    1. 谁和谁应该被划分一个cgroup
    2. 资源的额度如何设置 ?
    3. 如何保证其强制性
4. cgroup 是为了解决什么问题

#### (fs) fread 实现机制
fread 需要参数 用户地址空间的指针 和 文件描述符
1. 如果fread 需要穿过page cache 层次，那么意味着数据首先拷贝到page cache 然后才可以拷贝到用户中间
2. 如何将数据传送到用户的地址空间中间，应该也是copy_to_user 实现的方法了:
    1. 猜测第一个: 当一个函数的参数列表中间含有 `__user` 的时候，意味着该函数一定在syscall的调用上，此时，current ,就是正在调用syscall 的用户
    2. 猜测第一个: `__user` 务必需要指向其中 用户已经映射好的地址空间中，比如分配好的heap 中间
    3. 由于知道 current , 找到page director ， 所以可以知道写入的物理地址，然后将事情交给dma 之类的就可以了

并不是所有的函数都可以持有 `__user` 的参数，它们应该具有什么样子的特点。
#### (int) 在context switch 的过程中间允许中断吗 ? 应该不允许，因为时间非常的短暂。
如果允许，那么应该如何设计 ?

用户程序出现context switch ，也就是exception syscall 和 int :
1. 上下文信息保存用户stack 上 ?
2. 执行上下文切换时候，当时处于内核态 ?

执行上下文切换的代码如何防止破坏上下文，都是一些push 指令，只会破坏esp 最后保存esp 的数值可以计算出来。


#### (proc) set-user-ID 和 set-group-ID 是什么东西 ?
real UID and real GID
effective uesr ID and effective group ID


#### (misc) current_user() user.h
user tracking system ?
内核不仅仅需要管理process, 而且需要管理用户和用户组:

那些内容需要内核出面:
1. 无处不在的权限检查
2. resource limit
3. ....

对于current_user() 分析，发现在 task_struct 中间含有如下内容, 以前并没有注意到的内容
```c
	/* Process credentials: */

	/* Tracer's credentials at attach: */
	const struct cred __rcu		*ptracer_cred;

	/* Objective and real subjective task credentials (COW): */
	const struct cred __rcu		*real_cred;

	/* Effective (overridable) subjective task credentials (COW): */
	const struct cred __rcu		*cred;
```






跟踪user的结构体:
```c
/*
 * Some day this will be a full-fledged user tracking system..
 */
struct user_struct {
	refcount_t __count;	/* reference count */
	atomic_t processes;	/* How many processes does this user have? */
	atomic_t sigpending;	/* How many pending signals does this user have? */
```



#### (todo) setuid
man it

#### (mem) vmalloc kmalloc kzalloc
vmalloc 到底是怎么回事 ?

#### (todo) id
https://en.wikipedia.org/wiki/Universally_unique_identifier
https://en.wikipedia.org/wiki/User_identifier#Real_user_ID

#### (todo) 虚拟化 和 容器化 有什么不同
plka 内容没有分析虚拟化之类的东西，比如kvm ，值的单独总结


#### (misc) Navida 的显卡驱动由于不开源结果和linux 社区闹很不开心，那这一个问题windows 是如何解决的啊 ?

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

segment selector : cs fs gs 之类的东西o
segment descriptor : cs 在 table 中间项目

segment selector 中间的确含有rpl之类的东西，也许是用来作为防备的



#### (process) context switch 的基本单位是process 还是 thread ? 如果是相同的process 下的thread 切换，很多内容是相同，是不是切换可以简化一下？

#### (misc) https://www.linuxjournal.com/article/8023
搜索context switch 的时候顺便发现的

#### (todo) https://www.cs.kent.ac.uk/people/staff/srk21/research/papers/kell16missing-preprint.pdf


#### (asm) head_64.S 中间直接include 了C 头文件，但是其实应该不是直接include 的，而是首先被预处理过的
> 进一步的证据需要首先了解linux 的编译系统

#### (misc) section 在内核的作用是什么 ?
当我们使用链接脚本的时候，现在能够体会的是设置装载的虚拟地址位置从而实现虚拟地址和物理地址做的内核线性映射
但是，链接器的作用显然不仅仅如此，各种类似功能的代码被放到同一个section 中间，
但是这样做的好处是什么，尚且不清楚


#### (misc) 引入 early_param 机制的作用是什么 ?

#### () 不同时期的日志系统是如何变化的，比如pr_info 和 printk 的内容
      pr_info("NR_CPUS:%d nr_cpumask_bits:%d nr_cpu_ids:%u nr_node_ids:%d\n",
        NR_CPUS, nr_cpumask_bits, nr_cpu_ids, nr_node_ids);

#### (percpu) smp_processor_id()
一个有意思的函数，值的分析?

#### (init) 内核启动让人疑惑的地方
1. bios 也会设置中断向量表 和 之后系统启动使用的idt 是两个东西
2. bios 会选择启动盘，然后读入bootloader 然后 bootloader 读入 kernel image

#### (vmm) 找到内核态中间不能进行pgfault 的证据在什么地方 ?

#### (todo) 抢占的含义是什么？
preempt 现在理解的含义: 时钟中断，计数，到达特定值，切换!

如果想要屏蔽抢断，关中断，岂不是用户程序可以拒绝被抢占 ?

#### (mem) VMAP_STACK 是做什么用的 ?

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

参数是文件，作为init 进程，再次之前的提供何种支持来保证此时的文件系统是可用的。

#### (misc) 什么syscall 会访问user id 和 group id ?

#### (misc) cgroup 和 rlimit 是一个东西吗 ?


#### (fs) debugfs 到底是做什么的 ?

#### (lock) might_sleep 作用是什么，谁会使用 ?

#### (sched) 调度启如何实现两个高运行的出现在不同的CPU中间

#### (block layer) CONFIG_BLOCK
如果这个CONFIG 被取消掉了，那么.... ?

#### (mmap) 对于文件的映射，是不是都是只读的?
不然，为什么file的反向映射需要简单的多 ?

#### (mm) 既然buddy system 限制了buddy system 的最大的分配的连续的物理地址页面，那么如果需要分配的连续物理页面大于限制，怎么办 ?
猜测，直接返回失败，没有办法返回多个不连续的物理地址来拼凑(返回值只有一个)，
现在需要重新想一下为什么需要内核分配2^n(n > 0) 的物理地址，除了huge page 之外。
一次分配一堆，减少调用alloc_page的次数。
从内存虚拟化的角度来说，内核分配的物理内存的时机总是在page fautl 之类的时候，所以应该总是分配一个大小为1.

#### (dev) 你永远都不知道 vfs_inode 的前面会放置什么额外信息 !

```c
struct bdev_inode {
	struct block_device bdev;
	struct inode vfs_inode;
};
```

#### (todo) mm/backing-dev.c 的作用是什么 ?

#### (看来可以提交新的patch了) core.c:shced_fork 中间可以减少一个判断啊 !

#### (process) set_current_state(TASK_INTERRUPTABLE)

#### (mm) PageUptodate
想知道page 上的各种 flag 的作用是什么 ?

#### (io mm) read_iter 的两个参数的作用是什么 ?
```diff
tree 89aae8e9d832199906d413dcffd5b885bcee14fe
parent 7f7f25e82d54870df24d415a7007fbd327da027b
author Al Viro <viro@zeniv.linux.org.uk> Tue Feb 11 18:37:41 2014 -0500
committer Al Viro <viro@zeniv.linux.org.uk> Tue May 6 17:36:00 2014 -0400

new methods: ->read_iter() and ->write_iter()

Beginning to introduce those.  Just the callers for now, and it's
clumsier than it'll eventually become; once we finish converting
aio_read and aio_write instances, the things will get nicer.

For now, these guys are in parallel to ->aio_read() and ->aio_write();
they take iocb and iov_iter, with everything in iov_iter already
validated.  File offset is passed in iocb->ki_pos, iov/nr_segs -
in iov_iter.

Main concerns in that series are stack footprint and ability to
split the damn thing cleanly.

// 在这一个patch中间，将 read 修改为 read_iter
```

#### (misc) subsys_initcall
1. 实现的机制是什么 ?
2. 到底存在那些 subsys 可以使用 ?

看到 subsys_initcall(init_user_reserve) 有感 !

#### (mm) 内核使用 mm_struct 和 vma 吗 ?

显然内核需要:
1. 内核中间的 vma 有那几个 ?
2. 会在运行的过程中间，动态的变化吗 ?

#### (mm) 请问vmscan.c 中间的函数中间总是需要处理的各种智障 flags 到底是干什么的 ?

比如 shrink_page_list 中间的例子:

```c
		/* Double the slab pressure for mapped and swapcache pages */
		if ((page_mapped(page) || PageSwapCache(page)) &&
		    !(PageAnon(page) && !PageSwapBacked(page)))
			sc->nr_scanned++;
```

#### (mem) 当用户访问没有 mmap 的空间，被操作系统告知 segment fault 的过程是什么 ?
1. 应该不可能是通过访问 vma 的权限吧，
1. 是通过 page table 上的权限实现的吧 ?
    1. 在创建 vma 的时候，其对应的 page table 上的权限就确定了，而 vma 上的权限只有当 page fault 进行 page walk 的时候逐渐补充的，当 page walk 的时候，该位置权限不对，或者超过范围，之类的时候，这时候 page fault 失败
然后告知用户进程的问题。
    2. 想法不错，可以检查一下。

#### (device) module device device_node 之间的关系都是什么 ?

#### (syscall) 为什么 syscall 可以保证安全，没有替换的方法吗 ?

#### (pid) pid sid gid uid 之类到底都是啥 ?

```c
struct audit_context
...
	pid_t		    pid, ppid;
	kuid_t		    uid, euid, suid, fsuid;
	kgid_t		    gid, egid, sgid, fsgid;
	unsigned long	    personality;
	int		    arch;

	pid_t		    target_pid;
	kuid_t		    target_auid;
	kuid_t		    target_uid;
	unsigned int	    target_sessionid;
	u32		    target_sid;
...
```

#### (process) 一共都有那些 kthread ，其执行方法有什么类似的地方 ?(while 循环, wakeup, sleep 之类的)
1. audit.c : kauditd_thread
2. vmscan.c : kswapd(好像这个名字)


#### (misc) 编译系统 Kconfig 那个make menu 以及各种 dep 文件
如果可以掌握这个，那么岂不是可以应对所有的挑战了

#### (misc) 我想知道 /lib 下那么多的东西都有啥 ?

#### (mm) 内存空洞

1. NUMA 的物理空洞，所以存在 spare mem 来防止出现反向映射
    1. 我觉得，其中 IO 映射 也是导致物理地址出现空洞

2. NUMA 的存在，那么内核的代码段放在那里啊 ?
    1. 不然执行syscall 的时候岂不是非常的难受, 访问另一个内存上的内核代码段

#### (sched) 什么时候使用 might_sleep，什么时候使用 need_sched

#### (misc) poll 和 select 是如何实现的 ?

#### (todo) 找到或者总结一下 SMP 和 NUMA 带来的挑战
1. 是在不行就到 reddit 或者 知乎 上去问
#### (todo) https://github.com/LearningOS/aos-lectures/blob/master/aos-course-outline.md
需要操作系统进阶的内容，现在太过于局限于细节之中了

#### (lock) seqlock 是什么东西?

#### (process) task 不可以被 interruptible 是什么意思 ? 当其运行的时候，屏蔽所有的中断吗 ?

```c
/* set task’s state to interruptible sleep */
set_current_state(TASK_INTERRUPTIBLE)
```

#### (process) wait queue 在何处实现的 ?
不是 work queue ?

#### (time) 能不能其他的CPU 时钟停止，只有有事情干的才会时钟 tick ?

#### () 我想知道uboot bootloader PMON bios 都是什么J8 东西。
https://github.com/u-boot/u-boot


#### (process) 整理一下 page fault 流程

page fault的原理非常简单，其目的也很清晰，但是，现代操作系统出于一下原因让page fault变的异常复杂:

# 问题
1. pf在中断的过程中间如何被触发?
2. pf出现内核的地址空间意味着什么?
3. pf为什么需要区分不同类型的memory，以及都是如何使用的?
4. pf处理的如果是swap和首次加载有什么不同?
5. pf为了提高性能，除了prefetch，还提供了什么方法?

#### (todo) 分析一下经典的 path
1. open 函数 : vfs,  page cache , block
2. sleep 函数 : 时钟调度器
3. kill : 信号机制 process 生命周期

经典的 syscall 函数将整体串联起来

#### (cgroup) subsystem : 额，好像这里的 subsys 不是一个东西


```
struct cgroup_subsys memory_cgrp_subsys = {
	.css_alloc = mem_cgroup_css_alloc,
	.css_online = mem_cgroup_css_online,
	.css_offline = mem_cgroup_css_offline,
	.css_released = mem_cgroup_css_released,
	.css_free = mem_cgroup_css_free,
	.css_reset = mem_cgroup_css_reset,
	.can_attach = mem_cgroup_can_attach,
	.cancel_attach = mem_cgroup_cancel_attach,
	.post_attach = mem_cgroup_move_task,
	.bind = mem_cgroup_bind,
	.dfl_cftypes = memory_files,
	.legacy_cftypes = mem_cgroup_legacy_files,
	.early_init = 0,
};
```
曾经一直以为kernel中间subsystem只是一个抽象的概念，现在才发现是一个具体的内容的支持。

#### (todo) 说明一下当键盘按下一个键，到屏幕显示结果的过程
1. 中断
2. 字符设备
3. tty 等等

#### (fs dev) 既然不同的文件系统可以加载到设备上，也就是访问设备提供了一套interface，那么这个 interface 是什么 ?
1. 能不能利用 /dev/　下的获取到设备之后，绕过文件系统，直接访问设备
    1. 这一个操作只能在内核态完成或者也可以在用户态完成

2. 感觉 fs/block_dev.c 似乎会告诉我们很多关键的信息点。
#### (fs block) block 和 sector 之间是如何装换的 ? block = n * sector 的时候，n 的取值有什么讲究吗 ?

#### (block) io scheduler

提供给 io scheduler 的interface 是什么？
1. request queue ?

#### (dma) dma 和 block layer 的关系是什么 ?

#### (device) disk 初始化的过程，为什么不咨询一下 ucore

1. boot 之后，我怎么知道存在 hard disk 的存在
2. /dev/nvme0n1 的创建时间是什么
3. fs 怎么知道自己 mount 到哪一个 partion 上面去

#### (mem) vmalloc 映射虚拟地址空间的开始位置是多少 ?

#### (dev) /dev 到底是用来做啥的 ? 和 /sys 的关系是什么 ?

/sys/block/nvme0n1 和 /dev/nvme0n1 的关系是什么 ?

```
drwxr-xr-x root root 0 B Thu Feb 20 11:07:03 2020   .
dr-xr-xr-x root root 0 B Thu Feb 20 09:52:58 2020   ..
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop0 ⇒ ../devices/virtual/block/loop0
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop1 ⇒ ../devices/virtual/block/loop1
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop10 ⇒ ../devices/virtual/block/loop10
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop11 ⇒ ../devices/virtual/block/loop11
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop12 ⇒ ../devices/virtual/block/loop12
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop13 ⇒ ../devices/virtual/block/loop13
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop14 ⇒ ../devices/virtual/block/loop14
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop15 ⇒ ../devices/virtual/block/loop15
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop16 ⇒ ../devices/virtual/block/loop16
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop17 ⇒ ../devices/virtual/block/loop17
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop18 ⇒ ../devices/virtual/block/loop18
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop19 ⇒ ../devices/virtual/block/loop19
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop2 ⇒ ../devices/virtual/block/loop2
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop20 ⇒ ../devices/virtual/block/loop20
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop21 ⇒ ../devices/virtual/block/loop21
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop22 ⇒ ../devices/virtual/block/loop22
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop23 ⇒ ../devices/virtual/block/loop23
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop24 ⇒ ../devices/virtual/block/loop24
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop25 ⇒ ../devices/virtual/block/loop25
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop3 ⇒ ../devices/virtual/block/loop3
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop4 ⇒ ../devices/virtual/block/loop4
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop5 ⇒ ../devices/virtual/block/loop5
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop6 ⇒ ../devices/virtual/block/loop6
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop7 ⇒ ../devices/virtual/block/loop7
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop8 ⇒ ../devices/virtual/block/loop8
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   loop9 ⇒ ../devices/virtual/block/loop9
lrwxrwxrwx root root 0 B Thu Feb 20 11:07:03 2020   nvme0n1 ⇒ ../devices/pci0000:00/0000:00:1d.0/0000:03:00.0/nvme/nvme0/nvme0n1
```
> 为什么全部都是链接文件 ? 为什么要这样 ?

#### (io) iopl ioperm 了解一下

#### (mem) slab_is_available
不，我无法理解 memblock 没有释放 pf 给 buddy allocator 的时候，此时 slab 是可以使用的
而且 slab 还有好几个可用级别,真的窒息。

#### (mm) struct page 中间 `_mapcount` 和 `_refcount` 比对一下 !

```c
	union {		/* This union is 4 bytes in size. */
		/*
		 * If the page can be mapped to userspace, encodes the number
		 * of times this page is referenced by a page table.
		 */
		atomic_t _mapcount;

		/*
		 * If the page is neither PageSlab nor mappable to userspace,
		 * the value stored here may help determine what this page
		 * is used for.  See page-flags.h for a list of page types
		 * which are currently stored here.
		 */
		unsigned int page_type;

		unsigned int active;		/* SLAB */
		int units;			/* SLOB */
	};

	/* Usage count. *DO NOT USE DIRECTLY*. See page_ref.h */
	atomic_t _refcount;
```

#### (mem layout) zero_user 函数的实现

#### (misc) perf 到底是什么 ?
比如这个函数 : perf_event_init_task

#### (fs) simple_read_from_buffer

```c
ssize_t simple_read_from_buffer(void __user *to, size_t count, loff_t *ppos,
				const void *from, size_t available)
```
it relys on copy_to_user:

so every file read function depends on copy_to_user eventually to copy data to user space.

there is no doubt, find the evidence !

#### (fs) 各种值得分析的文件系统
d_tmpfile
