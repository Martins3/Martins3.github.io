## highmem
1. 为什么内核虚拟地址需要映射所有的物理地址 ?
猜测: a. gup 很容易实现，利用 kmap 可以很容易访问所有的。
2. 真的所有物理地址的 page table 全部填充了吗 ?


2. 为什么需要将内核地址空间和用户地址空间分离开 ?
A 32-bit system has the ability to address 4GB of virtual memory; while user space and the kernel could have distinct 4GB address spaces, arranging things that way imposes a significant performance cost resulting from the need for frequent translation lookaside buffer flushes. [^2]

To avoid paying this cost, Linux used the same address space for both kernel and user mode, with the memory protections set to prevent user space from accessing the kernel's portion of the shared space. This arrangement saved a great deal of CPU time — at least, until the **Meltdown vulnerability** hit and forced the isolation of the kernel's address space.
> 什么 ?

为什么曾经需要 highmen ?
内核的虚拟地址空间 和 用户的虚拟地址空间不能重叠，
在 32bit 的地址空间，面对 4G 的物理内存的时候，内核虚拟地址空间无法覆盖所有的物理内存。
linux 规定 KVAS 映射 3G~4G 的虚拟地址空间，那么如果需要访问 0G-3G 的物理内存，那么就需要修改 page table
1. 为什么内核不可以使用 0G-3G 的虚拟地址空间 ? (只要出现一丢丢重叠，那么同一个虚拟地址就是可以映射到两个物理地址上的，那么用户和内核之间的切换就必须进行 TLB flush)
2. 虽然 0G-3G 的空间不可以映射，但是通过在 KVAS 中间的内核虚拟地址空的 page struct 数组，还是可以对于所有的物理内存进行管理。

gup 的作用 : 将用户的 page 直接固定到内核态，不用使用 copy_from_user 之类智障操作，
从用户提供的虚拟地址(get_user_pages 的参数 mm 说明是哪一个用户)得到物理页面。
然后获取该物理页面在内核虚拟地址空间的地址，然后就可以直接访问了。

Consider, for example, all of the calls to kmap() and kmap_atomic(); they do nothing on 64-bit systems, but are needed to access high memory on smaller systems. And, sometimes, high memory affects development decisions being made today. [^2]

kmap 和 kmap_atomic 在 64bit 是不是完全相同的:
```c
 static inline void *kmap(struct page *page)
 {
  might_sleep();
  return page_address(page);
 }

 static inline void kunmap(struct page *page)
 {
 }

 static inline void *kmap_atomic(struct page *page)
 {
  preempt_disable();
  pagefault_disable();
  return page_address(page);
 }
 #define kmap_atomic_prot(page, prot) kmap_atomic(page)

 static inline void __kunmap_atomic(void *addr)
 {
  pagefault_enable();
  preempt_enable();
 }
```
并不是完全相同的，应该只是历史遗留产物吧 !
