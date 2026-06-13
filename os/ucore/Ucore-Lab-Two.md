## How to begin

## how paging and segment works

1. segment
```
.p2align 2                                          # force 4 byte alignment
gdt:
    SEG_NULLASM                                     # null seg
    SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)           # code seg for bootloader and kernel
    SEG_ASM(STA_W, 0x0, 0xffffffff)                 # data seg for bootloader and kernel
```
```
#define SEG_ASM(type, base, lim)                                               \
  .word(((lim) >> 12) & 0xffff), ((base)&0xffff);                              \
  .byte(((base) >> 16) & 0xff), (0x90 | (type)),                               \
      (0xC0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)
```
此时的segment应该只是一个临时的，又bootloader使用的,



2. 在设置完成segment之后，然后加载image进来，然后就马上enable了paging

```
.align PGSIZE
__boot_pgdir:
.globl __boot_pgdir
    # map va 0 ~ 4M to pa 0 ~ 4M (temporary)
    .long REALLOC(__boot_pt1) + (PTE_P | PTE_U | PTE_W)
    .space (KERNBASE / PGSIZE / 1024 * 4) - (. - __boot_pgdir) # pad to PDE of KERNBASE
    # map va KERNBASE + (0 ~ 4M) to pa 0 ~ 4M
    .long REALLOC(__boot_pt1) + (PTE_P | PTE_U | PTE_W)
    .space PGSIZE - (. - __boot_pgdir) # pad to PGSIZE
```
内核被加载到1M开始，而且物理地址和虚拟地址就是相等，此处保证kernel img 不会大于3M


似乎再次之前，所有的访存就已经进入到了physical addresss + KERNBASE = vritual addresss


**所以为什么需要做一个KERNBASE的映射?**


区分boot和kern 的代码相互分开的，

空闲内存空间的起始地址在哪里？
1. 不是在bootloader中间的代码，而是在kernel image中间的
2. 在其中paging之后代码都是不需要

也就是说，只有在kernel中间，paging之前需要手动切换


3. 然后就到了米奇的妙妙屋
```
void pmm_init(void) {
    // We've already enabled paging
    boot_cr3 = PADDR(boot_pgdir);
```

4. 前面的先放着， 第一句话的含义是什么:
```
boot_pgdir[PDX(VPT)] = PADDR(boot_pgdir) | PTE_P | PTE_W;
```
如果访问VPT空间，用掉最高10, 然后使用　中间10 位作为index 访问 boot_pgdir的，然后得到是page table的地址。


```
boot_map_segment(boot_pgdir, KERNBASE, KMEMSIZE, 0, PTE_W);
```
以前是直接实现所有的内核空间一一映射，然后加载的过程中间，将新的空间逐渐添加映射

现在，依旧在内核中间，开启了segment之后，然后添加paging的映射，由于kernel image小于4M, 并且在编译期间的时候，前面4M 已经实现了映射

5. 回到前面的内容中间
```
init_pmm_manager()
```
处理的内容大概是将

6.
```
static void
page_init(void) {
    struct e820map *memmap = (struct e820map *)(0x8000 + KERNBASE);
```
所以这是哪里搞定的:

物理内存探测是在bootasm.S中实现的，相关代码很短


**为什么加载内存segread是在内存探测之前就可以完成的?**

首先实现page_init函数，将所有的空间全部探测出来，然后使用将KERNSIZE的空间重新映射出来，
在映射的时候，使用的空间就是从pmm_manager中间分配的

```
 local_intr_save(intr_flag); {
      page = pmm_manager->alloc_pages(n);
 }
 local_intr_restore(intr_flag);
```
所以，为什么local_intr_resore的是做什么，如果没有，会触发什么问题吗?

7. 来分析一下page_init 函数

```
// virtual address of physicall page array
struct Page *pages;
// amount of physical memory (in pages)
size_t npage = 0;
```

maxpa是个谜，但是pages的开始位置表示为kernel代码段之后第一个pagesize对其的位置就是
也就是管理内存空间code + page struct arrays + pages的布局
```
extern char end[];

npage = maxpa / PGSIZE;
pages = (struct Page *)ROUNDUP((void *)end, PGSIZE);
```
似乎，当使用了page_init之后，就没有办法实现buddy_system了

居然，探测出来的内存空间是不连续的，你敢信?
而且中间的空洞位置，采用的方法就是不去映射，白白的浪费struct page的空间。

8. 难道不觉标志位可以影响page table的地址吗?

不会，由于page table总是page size对其的 和　以及page table 中间的项的最后12位是没有用途，
所以最后的12位置都是可以用于作为标志位的，只是访问的时候需要屏蔽一下。

9. 为什么需要重新load gdt，而不是boot的时候就处理好?

https://en.wikipedia.org/wiki/Task_state_segment

关键问题，内核的stack空间在哪里，对的，又是链接处理的
```
extern char bootstack[], bootstacktop[];
```
那么，用户的栈空间在哪里?
```
.data
.align PGSIZE
    .globl bootstack
bootstack:
    .space KSTACKSIZE
    .globl bootstacktop
bootstacktop:
```
也就是说，kernel中间的size只有一个pagesize那么大.

**先打住，看来stack还是含有一堆东西的**

10. kmalloc的实现基础是什么？ 
```
void slob_init(void) {
  cprintf("use SLOB allocator\n");
}

inline void kmalloc_init(void) {
    slob_init();
    cprintf("kmalloc_init() succeeded!\n");
}
```



#### 1
管理页级物理内存空间所需的Page结构的内存空间从哪里开始，占多大空间？

#### 2
the first alloc physical pages, and this part map them.

逻辑地址通过段式管理的地址映射可以得到线性地址，线性地址通过页式管理的地址映射得到物理地址。

在 ucore 中段式管理只起到了一个过渡作用，它将逻辑地址不加转换直接映射成线性地址，所以我们在下面的讨论中可以对这两个地址不加区分（目前的 OS 实现也是不加区分的）

ucore OS是基于80386 CPU实现的，所以CPU在进入保护模式后，就直接使能了段机制，并使得ucore OS需要在段机制的基础上建立页机制
> because before entering the protected mode, we have already set the gdt and ldt



in pmm.c
explaining following pages:

A small question: why lab1 can work even there is no memory map ?
because it work on physical memory !

understand pmm_init, and this link:
https://chyyuu.gitbooks.io/ucore_os_docs/content/lab2/lab2_3_3_6_self_mapping.html

#### 可不可以注意一下，什么时候使用macro来加减KERNBASE, 什么时候使用硬件完成的


## Adventures
#### why should map physical and virtual memory as p - 0xC0000000 = v

调用boot_map_segment函数建立一一映射关系，具体处理过程以页为单位进行设置，即
virt addr = phy addr + 0xC0000000

check ics:
1. different process have different cr3
2. how we create

#### sync is used for what ?


#### what's relation between GDT and cr3

在lab1中，我们已经碰到到了简单的段映射，即对等映射关系，保证了物理地址和虚拟地址相等，也就是通过建立全局段描述符表，让每个段的基址为0，从而确定了对等映射关系
```
# Switch from real to protected mode, using a bootstrap GDT
# and segment translation that makes virtual addresses
# identical to physical addresses, so that the
# effective memory map does not change during the switch.
lgdt gdtdesc
movl %cr0, %eax
orl $CR0_PE_ON, %eax
```

```
set_cr3(kpdirs);
set_cr0(get_cr0() | CR0_PG);
  1014bb:	0f 22 d8             	mov    %eax,%cr3

  1014be:	0f 20 c0             	mov    %cr0,%eax
  1014c1:	89 45 f4             	mov    %eax,-0xc(%ebp)
  1014c4:	8b 45 f4             	mov    -0xc(%ebp),%eax
```

A new problem: what is TSS ?

Task State Segment:
The TSS may reside anywhere in memory. A special segment register called
the Task Register (TR) holds a **segment selector** that points a valid TSS
segment descriptor which resides in the GDT. Therefore, to use a TSS
the following must be done in function `gdt_init`:
  - create a TSS descriptor entry in GDT
  - add enough information to the TSS in memory as needed
  - load the TR register with a segment selector for that segment

There are several fileds in TSS for specifying the new stack pointer when a
privilege level change happens. But only the fields SS0 and ESP0 are useful
in our os kernel.

The field SS0 contains the stack segment selector for CPL = 0, and the ESP0
contains the new ESP value for CPL = 0. When an interrupt happens in protected
mode, the x86 CPU will look in the TSS for SS0 and ESP0 and load their value
into SS and ESP respectively.

TR -> segment selector -> TSS
TSS in GDT


```
static void gdt_init(void) {
    // set boot kernel stack and default SS0
    load_esp0((uintptr_t)bootstacktop);
    ts.ts_ss0 = KERNEL_DS;

    // initialize the TSS filed of the gdt
    gdt[SEG_TSS] = SEGTSS(STS_T32A, (uintptr_t)&ts, sizeof(ts), DPL_KERNEL);

    // reload all segment registers
    lgdt(&gdt_pd);

    // load the TSS
    ltr(GD_TSS);
}
```


A new question: 
what is **segment selector** ?

Global Descriptor Table:
The kernel and user segments are identical (except for the DPL). To load
the %ss register, the CPL must equal the DPL. Thus, we must duplicate the
segments for the user and the kernel. Defined as follows:
  - 0x0 :  unused (always faults -- for trapping NULL far pointers)
  - 0x8 :  kernel code segment
  - 0x10:  kernel data segment
  - 0x18:  user code segment
  - 0x20:  user data segment
  - 0x28:  defined for tss, initialized in gdt_init

kern_entry函数的主要任务是为执行kern_init
建立一个良好的C语言运行环境（设置堆栈），
而且临时建立了一个段映射关系，
为之后建立分页机制的过程做一个准备
（细节在3.5小节有进一步阐述）

Notice:
```
bootloader -> | kern\_entry -> kern\_init
              | ucore
```

Let me think:
the address in eip is virtual or physical ?
virtual 

when will we should 

`.space`
http://cs.lmu.edu/~ray/notes/x86assembly/


Below code means what ?
```
.set i, 0
__boot_pt1:
.rept 1024
    .long i * PGSIZE + (PTE_P | PTE_W)
    .set i, i + 1
.endr
```
page table provide really address

  so who is using the first 4M,

why need temporay map:
from the time enable map to unmap, there are only several instructions:
```
# update eip
# now, eip = 0x1.....
leal next, %eax
# set eip = KERNBASE + 0x1.....
jmp *%eax
```
if there is no tempray map, the eip = 0x1......
instructions, it seems impossible for us to change eip, the reason why we didn't
handle this problem in ics is that copy kernel map when creating the a address space

a new quesiton: how to manage stack ?

## kernel
1. bootasm.S 中间的探测内存 和 ?? 对应 ?


2. sync.c 中间函数的作用 ?

3. init 中间设置了 0~4M 的映射，很好的处理了 ip 的切换，内核的 enable cr0 有没有类似的操作 ?

4. 还是两次lgdt，其实我觉得第一次加载完成所有就可以了啊!

```S
    # Jump to next instruction, but in 32-bit code segment.
    # Switches processor into 32-bit mode.
    ljmp $PROT_MODE_CSEG, $protcseg
```

```c
    // reload cs
    asm volatile ("ljmp %0, $1f\n 1:\n" :: "i" (KERNEL_CS)); // 巧妙的加载cs的方法
```
> 可能其中需要设置的内容不相同吧!
