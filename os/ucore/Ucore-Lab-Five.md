## 阅读文档
用户进程的地址空间，保护机制和用户内核的相互切换。

基于功能分析，我们就可以把这个用户环境定义为如下组成部分：
* 建立用户虚拟空间的页表和支持页换入换出机制的用户内存访存错误异常服务例程：提供地址隔离和超过物理空间大小的虚存空间。
* 应用程序执行的用户态CPU特权级：在用户态CPU特权级，应用程序只能执行一般指令，如果特权指令，结果不是无效就是产生“执行非法指令”异常；
* 系统调用机制：给用户进程提供“服务窗口”；
* 中断响应机制：给用户进程设置“中断窗口”，这样产生中断后，当前执行的用户进程将被强制打断，CPU控制权将被操作系统的中断服务例程使用。

所以操作系统虽然是一个软件，但其实是一个基于事件的软件，这里操作系统需要响应的事件包括三类：外设中断、CPU执行异常（比如访存错误）、陷入（系统调用）(trap)

最终通过do_execve函数来完成用户进程的创建工作。此函数的主要工作流程如下：
首先为加载新的执行码做好用户态内存空间清空准备。如果mm不为NULL，则设置页表为内核空间页表，且进一步判断mm的引用计数减1后是否为0，如果为0，则表明没有进程再需要此进程所占用的内存空间，为此将根据mm中的记录，释放进程所占用户空间内存和进程页表本身所占空间。最后把当前进程的mm内存管理指针为空。由于此处的initproc是内核线程，所以mm为NULL，整个处理都不会做。
接下来的一步是加载应用程序执行码到当前进程的新创建的用户态虚拟空间中。这里涉及到读ELF格式的文件，申请内存空间，建立用户态虚存空间，加载应用程序执行码等。load_icode函数完成了整个复杂的工作。

load_icode函数的主要工作就是给用户进程建立一个能够让用户进程正常运行的用户环境。此函数有一百多行，完成了如下重要工作：
* 调用mm_create函数来申请进程的内存管理数据结构mm所需内存空间，并对mm进行初始化；
* 调用setup_pgdir来申请一个页目录表所需的一个页大小的内存空间，并把描述ucore内核虚空间映射的内核页表（boot_pgdir所指）的内容拷贝到此新目录表中，最后让`mm->pgdir`指向此页目录表，这就是进程新的页目录表了，且能够正确映射内核虚空间；
* 根据应用程序执行码的起始位置来解析此ELF格式的执行程序，并调用mm_map函数根据ELF格式的执行程序说明的各个段（代码段、数据段、BSS段等）的起始位置和大小建立对应的vma结构，并把vma插入到mm结构中，从而表明了用户进程的合法用户态虚拟地址空间；
* 调用根据执行程序各个段的大小分配物理内存空间，并根据执行程序各个段的起始位置确定虚拟地址，并在页表中建立好物理地址和虚拟地址的映射关系，然后把执行程序各个段的内容拷贝到相应的内核虚拟地址中，至此应用程序执行码和数据已经根据编译时设定地址放置到虚拟内存中了；
* 需要给用户进程设置用户栈，为此调用mm_mmap函数建立用户栈的vma结构，明确用户栈的位置在用户虚空间的顶端，大小为256个页，即1MB，并分配一定数量的物理内存且建立好栈的虚地址<-->物理地址映射关系；
* 至此,进程内的内存管理vma和mm数据结构已经建立完成，于是把`mm->pgdir`赋值到cr3寄存器中，即更新了用户进程的虚拟内存空间，此时的initproc已经被hello的代码和数据覆盖，成为了第一个用户进程，但此时这个用户进程的执行现场还没建立好；
* 先清空进程的中断帧，再重新设置进程的中断帧，使得在执行中断返回指令“iret”后，能够让CPU转到用户态特权级，并回到用户态内存空间，使用用户态的代码段、数据段和堆栈，且能够跳转到用户进程的第一条指令执行，并确保在用户态能够响应中断；

## 阅读代码
> ◆ kern/debug/
> kdebug.c：修改：解析用户进程的符号信息表示（可不用理会）

那就不理会吧!

> ◆ kern/mm/ （与本次实验有较大关系）
> 1. memlayout.h：修改：增加了用户虚存地址空间的图形表示和宏定义 （需仔细理解）。
> 1. pmm.[ch]：修改：添加了用于进程退出（do_exit）的内存资源回收的page_remove_pte、unmap_range、exit_range函数和用于创建子进程（do_fork）中拷贝父进程内存空间的copy_range函数，修改了pgdir_alloc_page函数
> 1. vmm.[ch]：修改：扩展了mm_struct数据结构，增加了一系列函数 
>     1. mm_map/dup_mmap/exit_mmap：设定/取消/复制/删除用户进程的合法内存空间
>     1. copy_from_user/copy_to_user：用户内存空间内容与内核内存空间内容的相互拷贝的实现
>     1. user_mem_check：搜索vma链表，检查是否是一个合法的用户空间范围

memlayout.h 用户虚拟地址空间的定义用户stack,代码和heap开始的位置,和之前理解的相同，那就是stack 在上，向下生长，heap 在代码段的上方，向上生长的。

既然，用户和kernel使用的pgdir不相同，那么为什么还是需要故意将用户的地址空间和内核的地址空间放在同一张表中间，从虚拟地址上看，用户的地址空间难道不是为所欲为的，反正实际申请的空间还是那些位置, 原因是fork 的时候需要拷贝pgdir,*如果不复制pgdir,应该大量的页面无法使用*。


pmm.c的逐个分析: page_remove_pte、unmap_range、exit_range copy_rangep　gdir_alloc_page函数.
1. 为什么之前不需要这些函数，因为copy_range函数是由于复制父进程的内存空间，由于内核线程都是使用整个KERN空间的，所以内核之间并没有这一个操作。但是内存资源的回收早就使用过了啊。
2. 鸡儿，page_remove_pte 早就添加了，回顾一下，为什么需要让pte和page 相连起来，即使在使用内核的空间的时候，所有的动态运行空间也是需要申请的，也就是KERNSIZE的映射只有静态代码有用的，其余部分的映射没有意义，虽然get_pte可以返回一个数值，但是该数值
在page_insert的时候被重写为动态分配的page
3. unmap_range　循环调用 page_remove_pte，根据pgdir 和线性地址　释放物理页，清空pte
4. exit_range 应该是用于回收页表，首先调用unmap_range回收所有的物理页，然后再清空页表
5. copy_range 因为不是很麻烦
6. page_alloc_page 就是分配一个page 并且让线性地址和物理地址相对应

vmm.c 增加的函数
1. mm_map 根据start end创建一个vma 并且插入到mm 中间，注意内核的初始化中间，mm 是必须初始化为NULL,　只有内核线程，似乎是随意申请空间就可以，但是用户必须申请空间
2. dup_mmap  深度拷贝mm
3. exit_mmap  回收
2. copy_from_user/copy_to_user　从内核角度复制，
3. user_mem_check 上面两个复制函数的关键位置


> ◆ kern/process/ （与本次实验有较大关系）
> proc.[ch]：修改：扩展了proc_struct数据结构。增加或修改了一系列函数
> 1. setup_pgdir/put_pgdir：创建并设置/释放页目录表
> 1. copy_mm：复制用户进程的内存空间和设置相关内存管理（如页表等）信息
> 1. do_exit：释放进程自身所占内存空间和相关内存管理（如页表等）信息所占空间，唤醒父进程，好让父进程收了自己，让调度器切换到其他进程
> 1. load_icode：被do_execve调用，完成加载放在内存中的执行程序到进程空间，这涉及到对页表等的修改，分配用户栈
> 1. do_execve：先回收自身所占用户空间，然后调用load_icode，用新的程序覆盖内存空间，形成一个执行新程序的新进程
> 1. do_yield：让调度器执行一次选择新进程的过程
> 1. do_wait：父进程等待子进程，并在得到子进程的退出消息后，彻底回收子进程所占的资源（比如子进程的内核栈和进程控制块）
> 1. do_kill：给一个进程设置PF_EXITING标志（“kill”信息，即要它死掉），这样在trap函数中，将根据此标志，让进程退出
> 1. `KERNEL_EXECVE/__KERNEL_EXECVE/__KERNEL_EXECVE2`：被user_main调用，执行一用户进程


1. setup_pgdir/put_pgdir alloc一个page,然后将boot_pgdir中间的内容复制过来，由于地址空间是复制的，所以用户的空间不可以访问KERN的空间，应该是通过mm 来管理，如果alloc_page 成功，默认就是所有的空间拿出来就是可以使用的,所以清空page的时候需要恢复内核映射的空间
所有的物理空间都在内核映射上，使用空间都需要经过pmm_manager的申请，映射可以自行修改。**注意** `free_page`需要不可以`*pte=0`
2. copy_mm 将current 的内存复制到参数proc 中间
3. load_icode 将“磁盘”中间的内容加载到内存中间，具体位置由链接器确定，**trapframe** 需要填写都是什么狗东西
4. do_wait函数 https://chyyuu.gitbooks.io/ucore_os_docs/content/lab5/lab5_3_3_process_exit_wait.html　含有描述

> TODO 完成剩下的函数的阅读

实际上只有127M的空间

> ◆ kern/trap/
> trap.c：修改：在idt_init函数中，对IDT初始化时，设置好了用于系统调用的中断门（idt[T_SYSCALL]）信息。这主要与syscall的实现相关

似乎以前就处理了该问题

> ◆ `user/*`
> 新增的用户程序和用户库

了解该库和程序员的自我修养同时阅读

## 问题
1. proc中间的wait_state 的具体是什么
2. proc中的flags 
3. 什么时候会设置成为 PF_EXITING


## 重点关注内容
1. 用户如何通过syscall 进入内核态中间
2. 加载用户程序
3. 用户的库包括的内容有什么
4. 如何通过一个kernel thread 获取到一个用户进程
5. 获取用户进程和用户线程的区别是什么
6. mm_map 的实现到底是什么
7. 有没有升级swap 机制来file based 的内容 ?

8. 如何创建第一个用户进程的

8. 用户态和内核态的退出有什么不同 ?


kern/debug : 以前处理过kern/debug 吗 ?


#### 从fork分析函数开始
1. do_fork 机制处理sys_fork 和 kernel 的fork


2. copy_thread 之所以需要单独处理stack 是因为内核的esp 需要，而用户态则是完全照抄
3. parent 和 child 共用 stack ?
    * 无论是内核态的每一个thread都是需要自己的stack
    * 在forkret 的时候，会设置将stack 进行设置
    * fork 的 setup_kstack 可以保证所有人获取的内核态的stack都是不同的
    * 当进行context switch 的时候，就设置过esp是什么,　如果中间出现多次函数调用最后到 forkret 之后，其实此时esp 不是指向 tf 的，所以清空一下stack 而已
    * 所以的确，所有的线程都是含有内核stack 的

4. 但是用户态的stack 也是不可以重复的啊! 而且需要复制整个用户态的stack 才可以
    * 用户态的stack 被mm 管理，所以在copy_mm 的位置处理

4. 用户进程的context switch 的流程是什么:
    1. 首先保存 trap frame
    2. trap frame 的保存的位置由 esp0 确定，而且会保存用户态的esp和ss eip 等东西
    3. 调用scheduler => context switch
    4. 由于context switch 的上下文总是

* 如果trap frame 保存了用户态的 gs es fs 等等，那么又是在什么位置设置成为内核的fs es gs 之类的东西啊!
    * 理解错了，整个trap frame 的保存是在 trapentry.S 中间进行的
    * 的确有一部分是硬件保存的
    * es ds 切换软件完成，但是cs eip设置需要硬件完成，并且指向发vector 的
    * 至于fs gs 是什么状况就不知道了

* 如何获取fork 的返回值 ? 通过tf.eax
    * 同时，tf.eax 也是syscall 的参数个数
    * 由于子进程拷贝的context 直接返回，同时设置tf.eax 为 0
    * 父进程调用syscall 返回，所以返回值是syscall 的内容

* fork 之后child 的context 是什么 ? context 本身在proc 结构体中间，这一个context 被设置成为从context switch 中间跳转出来，那么立刻进行跳转即可。

* vector.c 生成的数值:
    1. 为什么有的生成不要添加push 01

* fork 机制如何实现复制cs ds ss 的，从而实现内核和用户态的不同
    * 复制trapframe 的时候会自动复制

#### 跟踪一下copy_mm的内容
1. 只有一个是否复制 mm_struct 的
2. 用户态将会出现复制，而内核太不用复制
    1. 似乎用户态只有进程，而内核态只有线程
    2. 内核产生新的stack 不是依赖fork, 不会调用函数到forkret 中间
    3. 而是直接调用新的函数，而且stack 也是设置好的
    4. 依旧无法解释，为什么需要单独设置内核态tf.esp = 0
        1. alloc_proc 应该会把tf 设置为 0

3. 复制为什么需要pgdir ?
    1. 因为需要get_pte，当前在to 的地址空间中间的，需要做的事情只是:
    2. alloc pte 设置，然后复制空间(需要虚拟地址)
    3. 再一次说明，为什么我们需要线性映射了!
    4. 需要from 的pgdir 的原因:
        1. 需要复制pte 的权限

2. 复制的时候仅仅限于 存在的，如果当前的page 在swap 中间，那不是gg ?
    1. 新的mm_struct 没有swap，所以一定出现问题

3. 由于拷贝发生在用户地址空间，虽然大家的虚拟地址相同，但是实际上物理地址不同。

#### do_exit
1. 居然根本不处理kernel thread 的退出问题
2. 主要处理两个事情: 释放mm 和 scheduler , 然后然后设置当前的状态为ZOMBIE，之后等待parent 收割返回值
2. exit_mmap
    1. unmap_range exit_range 一个回收pte 和 对应memory 一个回收二级列表

几个指针，older younger parent child !


如何配合wait_pid 机制使用:

是不是如果parent 死掉之后，child 一定会死掉, 显然不是的。
1. child 是直接提交到init上面，还是提交给parent 的 parent ?

zombie 的含义是什么 : 事情做完之后，当parent 需要返回值的时候，parent 通过wait 机制获取返回值。

his occurs for child processes,
where the entry is still needed to allow the parent process to read its child's exit status:
once the exit status is read via the **wait** system call, 
the zombie's entry is removed from the process table and it is said to be "reaped".

https://en.wikipedia.org/wiki/Zombie_process


do_wait 最后的部分保证 : 当一个进程进入到ZOMBIE的时候，那么几乎该释放的都释放了才可以!

set_links 和 remove_links 并不是，而是挂载init 上面了，最后还是通过init 擦屁股，使用link 的机制最后也是如此。



#### do_yield
在当前的模型下，进程的调度规则是什么 ?

```c
int do_yield(void) {
    current->need_resched = 1; // need_resched 的作用是什么 ?
    return 0;
}
```
和 PROC_RUNNABLE　之类的比较:


cpu_idle 真的会被使用吗 ?

* current 什么时候会等于 NULL => current 被使用的位置 ?
    * current 只要在idle 被初始化之后就不会设置为 NULL　对于current == NULL 的检查其实都是最早期的操作而已

* trap() 中间的问题:
    * otf 和 tf 的切换
        * 其实进入内核态，到底是用户态打断还是本身就是内核态，根本没有人在乎，只有从current-> tf 中间查出来，而已otf 是必须的，如果想要进行多级中断
    * 解释 其中调用 schedule 函数的原因是 : 这是配合 yield syscall 使用的，只要两个地方对于yeild 赋值为1，结论显然.
    * 解释 E_KILLED 和含义类似，配合yeild 使用！才发现exit 和 kill 是两个指令!

创建的新的proc添加到 scheduler 的方法，proc 的链表。

当用户态程序syscall 进入到内核态，在内核态中间遇到中断，pgfault(内核态中间可能pgfault 吗 ? 还是说pgfault 的地址不能出现在内核地址空间)

找到内核态trap 返回的操作 中间是如何处理之前自动push 进去的esp 的
https://stackoverflow.com/questions/6892421/switching-to-user-mode-using-iret 完全一致的描述

1. iret 虽然切换了ss:esp cs:ip 但是并没有切换数据段啊 ?
    1. 在 trapentry.S 中间，手动切换的
    2. 并不是说，用户态使用的三种段选择子和内核不同，而是大家使用都是cs ds ss，这意味者只要是进行ring切换，三者就需要切换
    3. iret 总是保证切换回来的时候的stack 就是之前发生中断的时候的stack，由于在内核态发生exception的时候，

3. 一个用户进程，正在运行，发生中断，被切换到内核态 ? 完全可能的! 就像是该用户使用int 指令一样，实际上，基于这种模型，如果PIC不停的发送中断，岂不是根本没有办法正常工作 ? 存在一种可以推迟响应的中断的办法，还是说，只能尽快切换，context switch 还是没有办法省略!

3. **如果访问了内核地址空间，硬件是如何检查出来的? 触发错误的整个过程是什么** ?

#### load_icode
1. load_icode 被execve 调用，当前的模型仅仅支持fork + execve 的方法实现。
2. do_execve 的作用，清空当前的 mm 然后填充新的mm，保留的内容，trapframe 和 进程控制块的内容
3. do_execve 导致stack 被清空了，load_icode 中间重做一份。

3. 二进制文件显然不可能原封不动的加载到内存中间，BSS，部分不用加载的内容

4. 拷贝函数名称使用 memcpy 而不是 copy_from_user ? 应该是特殊机制!
    1. 不仅仅是函数名，而且整个binary 都是不知道怎么被拷贝的，以及加工的

6. lcr3 并不会导致问题，因为当前执行在内核空间中间，而所有进程都会复制这一部分，这和启动的部分不同!
    1. 从 setup_pgdir 可知，其实只是需要复制pgdir table 这一个page 就可以了，由于是从boot_pgdir 中间复制的，用户空间绝对安全。

#### copy_from_user
copy_from_user 复制能力有限，更多是让用户持有文件描述符，fread 之类的操作又会导致syscall，其实也不难，因为current 指针没有装换，通过get_pte 之类的函数，可以轻易获取对应的物理地址，然后memcpy

1. 我猜测 copy_from_user 调用的情况为，用户中断进入到内核，此时current 依旧指向用户的进程控制块
2. 我猜测 并不是用户不能访问用户空间，支持不能直接访问，需要手动计算一下实际的物理地址然后转换为内核地址空间
    1. 一个物理地址显然是会被内核映射的，但是可能没有被用户映射，所以其实需要检查一下

实际上 copy_from_user 没有被ref，sys_execve 的参数实际上就是说明用户程序直接被加载内核中间的。
#### 一生之敌逐渐靠近
sync.c 中间又增加lock

#### 忽然发现kmalloc 和 kfree 使用的是 slob 分配器了!


#### panic 机制的含义
1. user 的 panic 实现很简单，各种警告，然后 exit(err)
2. 各种输出，然后内核的发命令行模式!
3. print_stackframe 实验一内容
4. 并没有神奇的内容
    1. 通过user.ld 将所有的string table 和 symbol table 各自放在一起，并且提供strtab 和　symbtab 的入口地址
    2. 获取eip 地址通过二分查找得到该eip 对应描述符


#### 用户态支持
1. 用户态的链接脚本，将程序连接到的特定位置开始
2. stack 和 heap 的设置
3. malloc 实现

2. umain.c 和 initcode.S 的作用: 分别在开始和结尾设置 stack 和 exit
    1. esp 为什么需要向下20 在backtrace 中间分析!

4. initcode.S 编译，clang 本来就可以编译.S 文件

5. 解决grad.sh 的bug


2. 按照当前的链接脚本 kernel.ld 难道不会导致将用户的所有的img 放置到同一个位置吗?
    1. https://stackoverflow.com/questions/20278489/gcc-linker-options-format-binary
    2. 由于相当于直接吧所有的二进制文件拷贝，所以就会有虽然每一个二进制文件都和ulib link 过，但是依旧可以放进去


为什么 forktree 又出现了bug !
1. 之前修复的bug 没有迁移, 部分初始值没有设置
2. 没有 set_proc_name 的使用

3. 当前的scheduler 的设置中间，总是从左向右扫描的，所以导致一旦scheule 就是 init 执行了 ?
    1. wait_state 机制 

#### wait_state 的作用是什么
> 将答案分析了一下，感觉非常的蛇皮，至少在本试验中间，没有什么意义!
