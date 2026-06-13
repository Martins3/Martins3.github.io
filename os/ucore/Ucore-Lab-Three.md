1. 概述
本次实验主要完成ucore内核对虚拟内存的管理工作.
code
即首先完成初始化虚拟内存管理机制，即需要**设置好哪些页需要放在物理内存中**，哪些页不需要放在物理内存中，而是可被换出到硬盘上，并**涉及完善建立页表映射、页访问异常处理操作等函数实现。**
test
然后就执行一组访存测试，看看我们建立的页表项是否能够正确完成虚实地址映射，是否正确描述了虚拟内存页在物理内存中还是在硬盘上，是否能够正确把虚拟内存页在物理内存和硬盘之间进行传递，是否正确实现了页面替换算法等。lab3的总体执行流程如下。

details for this line:

lab3中才有的新函数vmm_init、ide_init和swap_init

wired function, doing nothing at all.
```
void vmm_init(void) {
    check_vmm();
}
```

为了表述不在物理内存中的“合法”虚拟页，需要有数据结构来描述这样的页，为此ucore建立了mm_struct和vma_struct数据结构


this is a address space decriptor ?


2. 两个关键的数据结构
by reading the struct, we find that `mm_struct` and `vma_struct`

so why so we create this line.
```
// the control struct for a set of vma using the same PDT(page directory table)
struct mm_struct {
  list_entry_t mmap_list;        // linear list link which sorted by start addr of vma(mm 只是vma的控制者而已)
  struct vma_struct *mmap_cache; // current accessed vma, used for speed purpose
  pde_t *pgdir;                  // the PDT of these vma
  int map_count;                 // the count of these vma
  void *sm_priv;                 // the private data for swap manager(神奇)
};
```

```
// pre define
struct mm_struct;

// the virtual continuous memory area(vma), [vm_start, vm_end),
// addr belong to a vma means  vma.vm_start<= addr <vma.vm_end
struct vma_struct {
  struct mm_struct *vm_mm; // the set of vma using the same PDT
  uintptr_t vm_start;      // start addr of vma(存储的虚拟地址)
  uintptr_t vm_end;        // end addr of vma, not include the vm_end itself
  uint32_t vm_flags;       // flags of vma
  list_entry_t list_link;  // linear list link which sorted by start addr of vma
};
```
使用相同的Page directory table 难道不就是相同的进程吗?

如果不使用vma的空间来管理所有的磁盘，而是采用如果当前进程需要某个地址，不存在就加载，
进程退出的时候，然后就将所有遍历一遍，然后清空。


3. 好的，第一次预见了kmalloc函数，所以kmalloc和alloc_page是如何共同绑定起来的呢?
试验三的文档中表示说　使用时的default_pmm_manager

kern/mm/default_pmm.[ch]：实现基于struct pmm_manager类框架的Fist-Fit物理内存分配参考实现（分配最小单位为页，即4096字节），相关分配页和释放页等实现会间接被kmalloc/kfree等函数使用。

slob算法

先看slob_alloc函数
The core of SLOB is a traditional K&R style heap allocator, with support for returning aligned objects. The granularity of this
那不就是，基于和分配pages的算法类似，只是粒度更加小。

```
 * Above this is an implementation of kmalloc/kfree. Blocks returned
 * from kmalloc are 8-byte aligned and prepended with a 8-byte header.
 * If kmalloc is asked for objects of PAGE_SIZE or larger, it calls
 * __get_free_pages directly so that it can return page-aligned blocks
 * and keeps a linked list of such pages and their orders. These
 * objects are detected in kfree() by their page alignment.
```
一个细节，但是不是很关键，接下来的一段中间描述slab 和 slob 的区别。

```
spin_lock_irqsave(&slob_lock, flags);
```
对的，又是那个神奇锁


4. 熟悉vmm和mm 的函数是什么

两个create都是使用kmalloc简单调用和初始化
```
struct vma_struct *
find_vma(struct mm_struct *mm, uintptr_t addr) {
```
遍历mm的链表，然后查找其中的包含addr的


```
static void check_vma_struct(void) {
```
这是一个自说自话的检查，和我无关
> 这里到处都是分配空间和释放空间，所以rust在书写操作系统的代码的时候，如何处理内存问题，除了stack上的空间可以回收，其余的根本没有办法处理。



7. swap的流程是什么?

`kern/fs/*`：定义和实现了内存页swap机制所需从磁盘读数据到内存页和写内存数据到磁盘上去的函数 swapfs_read/swapfs_write。在本实验中会涉及使用这两个函数。

8.  struct Page 中间的 pra_page_link　是用来做什么的。

kern/mm/memlayout.h：修改了struct Page，增加了两项`pra_*`成员结构，其中pra_page_link可以用来建立描述各个页访问情况（比如根据访问先后）的链表。在本实验中会涉及使用这两个成员结构，以及le2page等宏。


9. page_fault 的触发
到底什么会触发page_fault?
1. 是page_walk的时候，发现需要的page_table 或者page_entry的项目不存在
2. 注意，这一步是在硬件中间完成，硬件在page walk出现问题，保存必要信息，trap
3. 最后，就会调用到当前的do_pagefault中间、

在check_page_fault中间，由于使用的地址是0x100，而该地址并没有被映射，所以会触发page_fault

```
page_remove(pgdir, ROUNDDOWN(addr, PGSIZE));
free_page(pde2page(pgdir[0]));
pgdir[0] = 0;
```
恢复现场，清理用于装载0x100的page, 清理page table,以及padir的项

10. 观察do_pagefault函数
1. 为什么page fault 函数需要和vmm 相关联起来
因为所有的page都是通过page_fault加载进来的，所以再次可以加以管理

```
if (vma == NULL || vma->vm_start > addr) {
    cprintf("not valid addr %x, and  can not find it in vma\n", addr);
    goto failed;
}
```
看来，一直对于内存空间的理解含有问题，看来进程首先需要规定自己使用的区间和权限。

10. page_fault的过程中间是需要获取新的page的，所以其中来处理swap　in/out

```
// pgdir_alloc_page - call alloc_page & page_insert functions to
//                  - allocate a page size memory & setup an addr map
//                  - pa<->la with linear address la and the PDT pgdir
struct Page * pgdir_alloc_page(pde_t *pgdir, uintptr_t la, uint32_t perm) {
    struct Page *page = alloc_page();
    if (page != NULL) {
        if (page_insert(pgdir, page, la, perm) != 0) {
            free_page(page);
            return NULL;
        }
        if (swap_init_ok){
            swap_map_swappable(check_mm_struct, la, page, 0);
            page->pra_vaddr=la;
            assert(page_ref(page) == 1);
            //cprintf("get No. %d  page: pra_vaddr %x, pra_link.prev %x, pra_link_next %x in pgdir_alloc_page\n", (page-pages), page->pra_vaddr,page->pra_page_link.prev, page->pra_page_link.next);
        }

    }

    return page;
}
```
`swap_map_swappable(check_mm_struct, la, page, 0);`最终调用实现的策略函数

```
static int _fifo_map_swappable(struct mm_struct *mm, uintptr_t addr, struct Page *page, int swap_in) {
    list_entry_t *head=(list_entry_t*) mm->sm_priv;
    list_entry_t *entry=&(page->pra_page_link);
    assert(entry != NULL && head != NULL);
    list_add_before(head, entry);
    return 0;
}
```
函数的作用应该是，将新的使用空间放置到swap 管理队列中间来

有几点可以关注一下:
-. alloc_page总是会触发swap_out，不过是，首先分配，然后swap_out(有点奇怪，为什么不是首先swap_out然后swap_in)
-. 在swap_fifo的实现中间，为什么使用sm_priv 而不是使用`list_entry_t mmap_list; // linear list link which sorted by start addr of vma`,
显然两者处理的内容并没有相同的地方，一个是表示目前拥有的底盘，一个表示在内存中间的pages
-. get_pte获取la地址对应的page table entry, 当获取了该entry，就可以实现la对应的物理地址了. 与insert_page相配合之后，两者正好可以形成一个添加新的空间的操作。

-. emmmmmm 这些管理的空间是不是已经早已被映射了? 
在boot_map_segment中间做的是事情是什么?　将0-KERNSIZE的空间和KERNBASE-KERNBASE + KERNSIZE的空间映射起来。之后所有访问该范围的空间都不会触发错误，但是由于该空间的内存中间大部分又是被管理的，
可能出现多个虚拟地址同时访问相同的物理地址。
> 这样做，既没有必要，又是可能造成不必要的干扰。

唯一可能的原因就是,那就是kernel image的大小是可能大于4M的，但是绝对小于kernsize的，在boot的汇编代码中间没有添加全部，

可以使用的空间都是被pmm 装载起来的

> 这里一回顾，我曹，发现了更加多的问题! 前面讲过，get_pte -> alloc_page -> fifo_swap_out_victim 而swap_out中间需要使用mm_struct, 而在使用boot_map_segment的时候，根本就没有初始化mm_struct，
> 错觉，alloc_page的代码　只是非常临时的代码，中间swap_init_ok 会做出检查，所以似乎只有开始检查文件的才会出现


```
//boot_alloc_page - allocate one page using pmm->alloc_pages(1) 
// return value: the kernel virtual address of this allocated page
//note: this function is used to get the memory for PDT(Page Directory Table)&PT(Page Table)
static void *
boot_alloc_page(void) {
    struct Page *p = alloc_page();
    if (p == NULL) {
        panic("boot_alloc_page failed.\n");
    }
    return page2kva(p);
}
```
page2kva是做什么用的东西（应该隐藏一个bug)

12. struct page已经被悄悄修改了
```
list_entry_t pra_page_link; // used for pra (page replace algorithm)
uintptr_t pra_vaddr;        // used for pra (page replace algorithm)
```
第一个用于在swap 中间，第二个用于保存la,所以为什么需要保存la?
如果不保存la, 那么在磁盘的swap_in和out 的时候是没有办法实现确定正确的磁盘块的
> 但是为什么不使用pte中间保存的

在swap_out函数中间，包含了`page->pra_vaddr/PGSIZE+1`的表示磁盘的编号，至于如何实现磁盘的管理，以后再说该问题。
```
cprintf("swap_out: i %d, store page in vaddr 0x%x to disk swap entry %d\n", i, v, page->pra_vaddr/PGSIZE+1);
*ptep = (page->pra_vaddr/PGSIZE+1)<<8;
```



#### 练习1：给未被映射的地址映射上物理页
完成do_pgfault（mm/vmm.c）函数，给未被映射的地址映射上物理页。设置访问权限 的时候需要参考页面所在 VMA 的权限，同时需要注意映射物理页时需要操作内存控制 结构所指定的页表，而不是内核的页表。注意

One thing keeps tortues me, what is vma

#### 练习2：补充完成基于FIFO的页面替换算法（需要编程）
哪些页可以被换出？
一个虚拟的页如何与硬盘上的扇区建立对应关系？
何时进行换入和换出操作？
如何设计数据结构以支持页替换算法？
如何完成页的换入换出操作？

User and Kernel:
所以我们在实验三中仅仅通过执行check_swap函数在内核中分配一些页，模拟对这些页的访问，然后通过do_pgfault来调用swap_map_swappable函数来查询这些页的访问情况并间接调用相关函数，换出“不常用”的页到磁盘上


## Adventure
#### What is the meaing of KADDR
why should we use that map, only set one page, why it seems every pages are mapped ?

这个问题不断的浮现在脑海中间，其实使用KADDR的位置一共只有5个，一部分

#### One qeustion which cann't be overlooked, how to manage swap disk ?

## rethink

#### 进入到磁盘的领域中间
1. 磁盘读写结构是什么样子的 ? 物理页面没有共享的 swap 的机制，存储磁盘序列号就可以吧 !　是存储在pte 中间还是 page struct 中间，如果说是存储在pte 中间，
page struct 中间同样为什么存储其对应的虚拟地址 ? 

2. 磁盘空闲的位置如何确定 ?

swap_in : 就是通过pte确定的 在swap 中间的地址
swap_out : 
    1. swapfs_write
    2. `*ptep = (page-> pra_vaddr/PGSIZE+1)<<8;`

```c
int
swapfs_read(swap_entry_t entry, struct Page *page) {
    return ide_read_secs(SWAP_DEV_NO, swap_offset(entry) * PAGE_NSECT, page2kva(page), PAGE_NSECT);
}

int
swapfs_write(swap_entry_t entry, struct Page *page) {
    return ide_write_secs(SWAP_DEV_NO, swap_offset(entry) * PAGE_NSECT, page2kva(page), PAGE_NSECT);
}
```
> 物理地址采用的机制就是物理地址和磁盘号一一对应，根本不需要管理空闲磁盘块之类的事情。


swap_init:

max_swap_offset 的作用:

swap_manager 的作用 : 

```c
struct swap_manager swap_manager_fifo =
{
     .name            = "fifo swap manager",
     .init            = &_fifo_init,  // doing nothing
     .init_mm         = &_fifo_init_mm, // 初始化数据 pra_list 并且和mm 挂钩起来! 如果是一个mm 管理一个swap list 很诡异的
     .tick_event      = &_fifo_tick_event, // nothing
     .map_swappable   = &_fifo_map_swappable, // 将 page 添加到 list 中间
     .set_unswappable = &_fifo_set_unswappable, // nothing
     .swap_out_victim = &_fifo_swap_out_victim, // swap out 关键函数, 从链表中间删除一个 entry
     .check_swap      = &_fifo_check_swap, 
};
```

5. page struct 中间需要存储 virtual address ?
    1. `page->pra_vaddr` 在page 被加入就会告知其对应的反向的地址
    2. 因为在处理swap 的时候，已经和虚拟地址没有关系了，只能获取到 struct page , 没有虚拟地址，那么就没有办法将 page 写入到 disk 中间，注意，disk write 中间需要的虚拟地址。

check_swap 是什么东西 ?
1. 初始化环境 : 设置free page 只有四个 并且实现分配好 pte 和 page frame
2. swap 机制数量上如何处理的: 分配失败的时候才会调用swap out 机制，换一句话说



#### page fault 机制
1. 如果对于一个页面不是被替换出去的，page fault 有含义吗 ?
2. 测试中间的模拟环境是什么样子的 ?
3. 以前认为page fault 出现的位置是读磁盘，然后加载到磁盘中间，但是其实只是 pde pte 没有对应的注册项目就会触发page fault
    1. 线性地址空间只是映射了从KERNBASE 到 KERNBASE + maxpa 之间内容，并没有映射用户地址空间
    2. page fault 被触发的时候，提供只是访问权限，读写，以及需要访问的虚拟地址, 怎么可能实现读取磁盘啊!

4. page fault　是检查pte 中间的标志位是否存在，当pte 没有和pte 中间存储的内容是 swap_entry_t 的时候都会导致 page fault


vmm_init : vma在mm中间的链表结构和 page fault 机制

swap_init :  

而且还增加了一个 ide 设备初始化


一般不能重复释放吧! 但是default_free_pages 的实现其实是含有问题的，只要释放的page 没有到达下一个 frer area 就可以，开始的位置没有限制。

PG_reserved 在初始化物理内存的时候，设置之后就再也没有处理过了。


pgdir_alloc_page : la perm 创建
page_insert(pgdir, page, la, perm)
page_remove_pte pte 对应的page 释放并且清空pte就可以了

get_pte : 获取la 对应的 pte
    0. 解释物理地址和虚拟地址纠结的地方
    1. 首先检查是否存在pde是否存在，不存在，那么注册
    2. 返回的还是虚拟地址

page_fault 的含义: 当其中没有pte是一种情况，当实现依旧含有pte 又是一种情景(我们一般见到的和期待的)
    1. 如何触发的 page fault ?


#### 中断
1. 为什么仅仅出现在 console.c 和 pmm.c
2. 无法解释　alloc_pages 就是链表的操作，如果非要出现问题，那就是因为这一个代码是中断的入口，导致这些代码重复执行的时候出现错误，但是。
```c
         local_intr_save(intr_flag);
         {
              page = pmm_manager->alloc_pages(n);
         }
         local_intr_restore(intr_flag);
```

3. amd64 v2 section 8.5 中间，32-255的都是maskable 的，page fault 显然不在其中，所以，上面的解释不能说服!
4. 能够屏蔽的只有键盘，时钟和磁盘，除非处理键盘的过程中间含有内存分配
5. 而且如果在处理page fault 的过程中间出现了 page fault ，怎么办！

