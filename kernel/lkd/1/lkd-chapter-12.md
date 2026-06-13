# Linux Kernel Development : 
Modern virtual memory operating systems generally have a flat memory model and not a segmented one.


> so, what's the purpose of `lgdt`?

> ics2018中间只有lidt也就是加载中断，没有使用lgdt
> 使用cr0寄存器分别设置paging 和 加载paging 的目录地址
> 而在ucore中间，首先设置的内容就是lgdt, 并且同时设置为保护模式

> 有点怀疑，所谓的虚拟地址　是　表示　使用段式地址和页式地址都是可以表示虚拟地址
> 是不是，在现代的操作系统中间，两者不是共存的，当使用页式地址地址之后，段式地址就没有用途了。

Segment addressing
In real mode each logical address points directly into physical memory location, 
every logical address consists of two 16 bit parts: 
The segment part of the logical address contains the base address of a segment with a granularity of 16 bytes, i.e. a segment may start at physical address 0, 16, 32, ..., 220-16.
The offset part of the logical address contains an offset inside the segment, i.e. the physical address can be calculated as physical_address : = segment_part × 16 + offset (if the address line A20 is enabled), respectively (segment_part × 16 + offset) mod 220 (if A20 is off)[clarification needed] Every segment has a size of 216 bytes. 

> 操作系统关于内存的处理:
> 1. 释放和分配物理内存(slab first-fit and buddy-system)
> 2. 虚拟内存的映射

## 参考资料
1. [详细的解释了所有的问题](https://stackoverflow.com/questions/18431261/how-does-x86-paging-work)
2. [segment and paging](https://stackoverflow.com/questions/24358105/do-modern-oss-use-paging-and-segmentation)
解释了为什么paging和segment 是可以混合使用的，但是仅仅限于在内核和用户态的切换而已。



