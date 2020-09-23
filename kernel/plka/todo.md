# 关键问题

#### copy_to_user 实现机制

#### 锁: RCU 

#### 多核意味着什么
1. percpu 单核percpu 是没有价值的
2. percpu 如何实现，amd64 中间使用 为什么使用fs(也许是gs 寄存器实现) 来支持percpu
3. 多核为锁, 调度器带来何种挑战
4. 为什么会出现从一个CPU 中被调度出去，从另一个恢复，会出现什么特殊的情况。
5. 多核让PIC 升级成为了APIC，我们开始需要分析如何正确负载
6. 多核出现形成了一个新的学科，memory consistency and cache coherency
7. WDNMD,虽然多核已经非常复杂了，但是我们还拥有更加复杂的大核和小核机制。

#### io 映射
1. 我怀疑 inb 在amd64 中间消失了, 而且inb 的实现机制是什么 ? inb 的端口分配是通过什么实现的
2. 端口映射和内存映射，但是据说端口映射其实 实现基础 是 内存映射
3. PA 显存的映射首先应该被看懂，( )

#### DMA
https://www.kernel.org/doc/Documentation/DMA-API-HOWTO.txt

1. 是那些 subsystem 最后调用到 DMA, 是不是所有的文件系统的操作

#### IO 地址映射实现原理
> 首先，理解inb 的实现是什么啊!
1. CPU 如何知道把消息告诉谁，端口事先规定好，通过什么总线，数据总线，内存和外设都接入到数据总线
2. CPU 是如何和总线系统打交道的 ? 总线控制器是放到什么位置上的 ?

> 然后理解 memory-map IO
> 1. 我怀疑，地址空间划分(哪一个区间是正常的内存，哪里是device) 是内存控制器决定 ? 操作系统能做的事情就是读取配置，然后加以分配吗 ?

https://en.wikipedia.org/wiki/Memory-mapped_I/O

Each I/O device monitors the CPU's address bus and responds to any CPU access of an address assigned to that device, connecting the data bus to the desired device's hardware register. To accommodate the I/O devices, areas of the addresses used by the CPU must be reserved for I/O and must not be available for normal physical memory. The reservation may be permanent, or temporary (as achieved via bank switching)

Different CPU-to-device communication methods, such as memory mapping, do not affect the direct memory access (DMA) for a device, because, by definition, DMA is a memory-to-device communication method that bypasses the CPU.
> DMA 和 memory port 没有关联 ? 

Hardware interrupts are another communication method between the CPU and peripheral devices, however, for a number of reasons, interrupts are always treated separately. An interrupt is device-initiated, as opposed to the methods mentioned above, which are CPU-initiated
> interrupt 也关系不大 ?
> @question 这应该就是device 和 CPU 打交道的全部三种方式吧!

AMD did not extend the port I/O instructions when defining the x86-64 architecture to support 64-bit ports, so 64-bit transfers cannot be performed using port I/O


#### 动态链接库
vdso 技术首先搞清楚再说 ?

一个动态链接库在ssd上，当一个程序运行的时候，将其加载到内存中间，然后


# 操作性试验
一个有意思的实践
https://stackoverflow.com/questions/36346835/active-inactive-list-in-linux-kernel?rq=1

重写类似的模块玩一下:
https://stackoverflow.com/questions/56097946/get-cpu-var-put-cpu-var-not-updating-the-per-cpu-variable

问一个问题: pg_data_t 中间 pg 是什么？ ask the stackoverflow


回答这一个问题:
https://unix.stackexchange.com/questions/297591/swap-cache-of-vmstat-vs-swapcached-of-proc-meminfo

gerrit kernel
1. 看懂了 ？ x86  初始化 probe 中断 bottom half (fifo & 串口)
2. 调试方法，分析方法。
