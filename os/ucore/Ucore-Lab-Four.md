1. 概述
本次实验将首先接触的是内核线程的管理。内核线程是一种特殊的进程，内核线程与用户进程的区别有两个：内核线程只运行在内核态而用户进程会在在用户态和内核态交替运行；所有内核线
使用共同的ucore内核内存空间，不需为每个内核线程维护单独的内存空间而用户进程需要维护各自的用户内存空间

kern_init -> proc_init ==> idleproc内核线程和initproc内核线程的创建或复制工作

参考cpu_idle函数的实现

所以uCore OS内核中的所有线程都不需要再建立各自的页表，只需共享这个内核虚拟空间就可以访问整个物理内存了

这是由于在uCore OS启动后，已经对整个内核内存空间进行了管理，通过设置页表建立了内核虚拟空间（即boot_cr3指向的二级页表描述的空间）
> which parition of code handle this procedure

2. struct proc的含义
为什么需要将context 和 trapframe 相互分离开来，
为什么一个直接包含，而一个则是使用的指针

3. 关键函数一: kernel_thread
> 感觉这就是普通C 创建thread的方法

做的事情主要是创建一个tf 然后，调用do_fork函数
tf 包含内容: 入口函数地址，函数的参数，参数个数以及
```
tf.tf_cs = KERNEL_CS;
tf.tf_ds = tf.tf_es = tf.tf_ss = KERNEL_DS;
```

tf trap frame
tf 中断帧的指针，总是指向内核栈的某个位置：
当进程从用户空间跳到内核空间时，
中断帧记录了进程在被中断前的状态。
当内核需要跳回用户空间时，
需要调整中断帧以恢复让进程继续执行的各寄存器值。
除此之外，uCore内核允许嵌套中断。因此为了保证嵌套中断发生时tf 总是能够指向当前的trapframe，uCore 在内核栈上维护了 tf 的链，
可以参考`trap.c::trap`函数做进一步的了解

几乎所有的内容都在此处proc.c中间， `entry.s` 以及 `switch.s` 是简单辅助内容


4. 关键函数二: copy_thread
> nemu 中间熟悉的部分

为什么需要将trapframe放置在kernstack中，具体来说是的尾端，也就是stack的底部

* context 中间eip esp 和 trapframe 中间的eip esp 分别表示什么?
* copy thread 中间的设置proc-> tf -> esp  以及　proc-> tf -> eax 的原因是什么
*

5. 所以schedule函数到底在搞什么
在proc_list 中间一顿操作之后，然后使用了调用proc_run

6. proc_run 函数在搞什么
```
  load_esp0(next->kstack + KSTACKSIZE);
  lcr3(next->cr3);
  switch_to(&(prev->context), &(next->context));
```
既然这一个地方设置esp ，那么之前似乎只是随便设置esp 为0, 此时才是真的设置。


7. tss 到底是什么
```
/* *
 * load_esp0 - change the ESP0 in default task state segment,
 * so that we can use different kernel stack when we trap frame
 * user to kernel.
 * */
void load_esp0(uintptr_t esp0) {
    ts.ts_esp0 = esp0;
}
```
所以之前的为什么从来没有注意到ts 的存在。
```
The field SS0 contains the stack segment selector for CPL = 0, and the ESP0
contains the new ESP value for CPL = 0. When an interrupt happens in protected
mode, the x86 CPU will look in the TSS for SS0 and ESP0 and load their value
into SS and ESP respectively.
```
tr(寄存器)保存segment selector 指向gdt 中间某一项，然后该部分描述了TSS的信息，而TSS则是可以保存在内存的任意位置
在ucore 中间，是在编译期间确定的。

8. 首先，搞清楚调用的流程是什么?
```

```



8. context 中间保存eip,所以eip是如何设置的?
在copy_thread 的时候实现的赋值的，esp 和eip 同时赋值

> 关键问题trapframe中间保存`tf_regs`的内容和context中间保存的内容的区别是什么?
> 而且esp 出现在了三个位置, 虽然tf_esp 没有用途?


9. switch.s 的细节是什么？

所以正常的情况下，到底是谁在调用schedule() 函数，switch.s 中间保存的返回值是schedule()函数的返回值位置。
> 回想Nemu 中间实现调度的方法，总是 时钟---> trap ---> 保存上下文 ---> sche



## Adventure

#### how to implement volatile


#### Understand the PCB
kernel address space and stack space
user address space and stack space

hash, why need hash

#### summarize the stack
最开始使用的stack在哪里：（有待观察，但是应该在lgdt的时候确定)

kernel stack: kernel fork的时候使用

用户的stack在哪里: 下一个试验

#### Memory map, how does this worked
```
memset(proc->name, 0, sizeof(proc->name));
```
memset proc-> name是绝对不会造成page_fault 的，sizeof已经表示不会越界，
kmalloc申请的空间在kernbase ~ kernbase + kernsize，这个空间一旦获取就可以使用，不会触发page_fault
除非调用者出现了问题!

为什么需要使用hash来管理函数：





#### Debug A
1. 当删除代码kstack的内容，可以解决swap_test中间的问题
2. alloc_pages 中间含有修改代码的位置，前面的试验中间含有错误

```
static inline ppn_t page2ppn(struct Page *page) { return page - pages; }

static inline uintptr_t page2pa(struct Page *page) {
  return page2ppn(page) << PGSHIFT;
}

static inline void *page2kva(struct Page *page) { return KADDR(page2pa(page)); }
```

所以page2kva得到结果是什么? 

通过访问pages直接得到对应的pages, 物理内存地址
在pmm中间，struct page就是和物理内存使用数组一一对应的
p1p2p3p4----P1----- -----P2----- ------P3----- ------P4-----

flags表示是否存在，property表示包含空闲项目数


pte2page 为什么pte可以获取对应struct page
pte 中间含有物理地址，pte中间包含page 
```
static inline struct Page *pte2page(pte_t pte) {
  if (!(pte & PTE_P)) {
    panic("pte2page called with invalid pte");
  }
  return pa2page(PTE_ADDR(pte));
}
```

```
static inline struct Page *pa2page(uintptr_t pa) {
  if (PPN(pa) >= npage) {
    panic("pa2page called with invalid pa");
  }
  return &pages[PPN(pa)];
}
```

```
#define PTXSHIFT 12 // offset of PTX in a linear address
// page number field of address
#define PPN(la) (((uintptr_t)(la)) >> PTXSHIFT)
```
看来物理地址和pages数组是线性对应的。

在KERN的部分，虚拟地址和物理地址　也是一一对应的。
所以出现了，page pa va都是可以互相装换的，但是操蛋的事情是物理
> **如果将地址空间不限于KERN, 这个结论大错特错** 多个la 是可以对应同一个pa的


#### Debug B
还有的一个大问题就是 get_pte　的实现很可能有问题
1. 有一个没有使用的函数boot_alloc_page
2. page_remove_pte函数的参数也没有全部使用

重新理解page_insert函数： 
以前以为page_insert函数是用于物理地址和虚拟地址互相映射才使用的，如今才发现是Page 和la 互相映射的
```
int page_insert(pde_t *pgdir, struct Page *page, uintptr_t la, uint32_t perm) {
    pte_t *ptep = get_pte(pgdir, la, 1);
    if (ptep == NULL) {
        return -E_NO_MEM;
    }
    page_ref_inc(page);
    if (*ptep & PTE_P) {
        struct Page *p = pte2page(*ptep);
        if (p == page) {
            page_ref_dec(page);
        } else {
            page_remove_pte(pgdir, la, ptep);
        }
    }
    *ptep = page2pa(page) | PTE_P | perm;
    tlb_invalidate(pgdir, la);
    return 0;
}
```
1. page 和物理地址是绝对一一对应，无论是否在KERN 的空间
2. 所以，使用page和la 对应就是，建立物理和虚拟地址对应
3. 插入之前需要检查　两者是不是　pte已经使用　已经有关系正好联系上，
4. 当已经使用pte已经使用，也就是虚拟地址已经被映射，

在一个pgdir中，虚拟地址显然只能被映射一次

似乎从此处再次说明 page_remove_pte(pgdir, la, ptep);中间的参数就是含有重复的page_remove_pte(pgdir, la, ptep);。

#### Debug C
几乎没有经历什么，free_list的位置忽然大幅度增加，使用gdb 观察这一个位置是更加可信的事情。

#### Debug D
显然，并没有彻底搞清楚pmm是如何使用的,在default_pmm 中间，好吧，找到了bug, 原来是前面的试验留下来的bug
不要过于依赖于注释，注释是含有误导性的
```
  SetPageProperty(p);
  list_entry_t * before_page = list_prev(&(page->page_link));
  list_add(before_page, &(p->page_link));
```
> 其他的部分似乎是没有问题。


## 关键位置
1. 如何给内核线程分配stack 的
2. 如何实现线程切换的
3. fork 机制是如何实现的 ?
4. 为了启动init 进程的准备是什么?



#### stack 到底是如何处理的
* proc_run : schedule 调度的
```c
            load_esp0(next->kstack + KSTACKSIZE);
            lcr3(next->cr3);
            switch_to(&(prev->context), &(next->context));
```
context_switch 的时候，发生在内核空间中间，提前加载cr3其实问题不大，因为只有切入到用户空间的时候，才会发现不同。
复制内核空间比想象的简单，只需要复制pgdir 的部分即可。

1. 但是 load_esp0 的作用如果导致 stack 立刻切换，在load_esp0 返回的时候都是在新的stack 上进行的!
2. switch_to 永远都不会返回，作用，保存当前的context 然后切换到下一个 context
3. 当switch_to 返回的时候，之前的进程执行完成，就像switch_to 什么都没有发生一样. (其实也不完全是，之前设置的各种stack load_esp0 应该就像是没有发生一样!)
4. switch_to 如何写:
    1. 保存到自己的stack 上
    2. 恢复新的stack
    3. iret 来恢复新的stack
    4. @todo 其实恢复的eip 能不能不保存eip的数值，如果函数入口总是switch 中间，返回也是switch 所以切换的时候保存和恢复的eip都是相同的，所以没有必要切换，除非 ?

5. context 中间保存内容其实没有想象的那么多，和trapframe 对比，差别是什么 ?

4. context_switch 的两种情况
    1. 时钟中断 : 时钟中断 => trap_dispatch => schedule => context_switch
    2. yield 函数主动放弃 : @todo 检查yield函数的内容

5. load_esp0 没有改变特权级，但是还是切换了stack的位置，没有切换esp 和 ebp 的切换stack 的是什么意思，还是一个偏移量
ESP0 gets the value the stack-pointer shall get at a system call

For each CPU which executes processes possibly wanting to do system calls via interrupts, **one TSS is required**.
The only interesting fields are SS0 and ESP0. Whenever a system call occurs, the CPU gets the SS0 and ESP0-value in its TSS and assigns the stack-pointer to it.
So one or more kernel-stacks need to be set up for processes doing system calls. Be aware that a thread's/process' time-slice may end during a system call, 
passing control to another thread/process which may as well perform a system call, ending up in the same stack.
**Solutions are to create a private kernel-stack for each thread/process and re-assign esp0 at any task-switch or to disable scheduling during a system-call.**
https://wiki.osdev.org/Task_State_Segment

难道是说，esp0 就是给syscall 使用的 ?

在boot 阶段的确使用 0x7c00 之前的作为stack, 然后之后在init 设置好映射之后，马上设置新的esp，如果真的esp0 的作用是在syscall 的时候将 esp 

文档中间说的对，注释也说的很对，其实切换esp0 只是用来处理用户进入到内核时候需要的stack，如果用户syscall进入到内核中间，可以不断exception int 但是都是在其所在的stack 中间
而在stack 的最上层一定是 trap frame !

内核中间还是每一个线程对应一个stack

load_esp0 并不是用来切换 stack 的，因为esp0 是所有process 的全局变量，所以切换process 的时候必须切换一下。

为什么context switch 的时候不用处理: trap frame 中间的gs es ss 之类的东西。



#### fork机制牵涉的内容
* copy_thread 复制context 和 trapframe 但是诡异的赋值:
    1. 

do_fork 完成最后，从设置好的trap frame 中间返回

如何实现两个返回值的 ? 从parent process 的角度来说，只是创建一堆新的数据，然后添加一个新的proc 到 scheduler 中间，但是从child 的角度，其返回值就放到 ? 

kernel_thread 通过手动创建一个假的 trapframe 来实现创建的新的kernel thread，其中指定的 edx ebx 和 eip 都是entry.S 设置的，可以导致其从context_switch 中间进入到制定的函数中间

do_fork 完成之后，thread 进入到 scheduler 中间，当程序开始执行的时候，context_switch 执行, 设置esp 和 eip , 导致进入到forkret 中间

#### trap 中间如何支持fork
1. 才发现，trap 需要保存这么多东西
2. forkret 正好对应一个 syscall 的返回，只是返回之前有一个小动作


```c
    proc->tf->tf_regs.reg_eax = 0; // @todo 不知道为什么 ?
    proc->tf->tf_esp = esp; // 这是用户态的stack 由硬件保存
    proc->tf->tf_eflags |= FL_IF; // 由于进入的之后关中断了，所以fork的需要开中断来

    proc->context.eip = (uintptr_t)forkret; // 保证context switch 进入到 forkret 中间，同时
    proc->context.esp = (uintptr_t)(proc->tf);  // 到达 forkret　的时候此时的stack 内容
```
