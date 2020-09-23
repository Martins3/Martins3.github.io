# KVM
1. 看看 kvm 的 ioctl 的实现
2. 求求了，什么时候学一下 x86 汇编吧，然后出一个利用 kvm 给别人写一个教程
3. 在 kvm 中间运行 unikernel ?
4. [^6] 虚拟化入门，各种 hypervisor 分类
5. 我心心念念的 TLB 切换在哪里啊 ?
6. virtio 的两个文章:
    1. https://www.redhat.com/en/blog/introduction-virtio-networking-and-vhost-net
    2. https://www.redhat.com/en/blog/virtio-devices-and-drivers-overview-headjack-and-phone
7. 所以 kvm 是怎么和 virtio 产生联系的 ？
8. virtio 如何处理 GPU 的 ?


## 记录
[^1] lwn 给出了一个超级入门的介绍，值得学习 :

Each virtual CPU has an associated struct `kvm_run` data structure, 
used to communicate information about the CPU between the kernel and user space. 

he VCPU also includes the processor's register state, broken into two sets of registers: standard registers and "special" registers. These correspond to two architecture-specific data structures: `struct kvm_regs` and `struct kvm_sregs`, respectively. On x86, the standard registers include general-purpose registers, as well as the instruction pointer and flags; the "special" registers primarily include segment registers and control registers.

**This sample virtual machine demonstrates the core of the KVM API, but ignores several other major areas that many non-trivial virtual machines will care about.**

1. Prospective implementers of memory-mapped I/O devices will want to look at the `exit_reason` `KVM_EXIT_MMIO`, as well as the `KVM_CAP_COALESCED_MMIO` extension to reduce vmexits, and the `ioeventfd` mechanism to process I/O asynchronously without a vmexit.

2. For hardware interrupts, see the `irqfd` mechanism, using the `KVM_CAP_IRQFD` extension capability. This provides a file descriptor that can inject a hardware interrupt into the KVM virtual machine without stopping it first. A virtual machine may thus write to this from a separate event loop or device-handling thread, and threads running `KVM_RUN` for a virtual CPU will process that interrupt at the next available opportunity.

3. x86 virtual machines will likely want to support CPUID and model-specific registers (SRs), both of which have architecture-specific ioctl()s that minimize vmexits.M
> TODO 这几个进阶，值得关注

While they can support other devices and `virtio` hardware, if you want to emulate a completely different type of system that shares little more than the instruction set architecture, you might want to implement a new VM instead. 

[^2]: 配置的代码非常详尽
TODO : 内核切换到 long mode 的方法比这里复杂多了, 看看[devos](https://wiki.osev.org/Setting_Up_Long_Moded)

The two modes are distinguished by the `dpl` (descriptor privilege level) field in segment register `cs.dpl=3`  in `cs` for user-mode, and zero for kernel-mode (not sure if this "level" equivalent to so-called ring3 and ring0).

In real mode kernelshould handle the segment registers carefully, while in x86-64, instructions syscall and sysret will properly set segment registers automatically, so we don't need to maintain segment registers manually.


This is just an example, we should *NOT* set user-accessible pages in hypervisor, user-accessible pages should be handled by our kernel.
> 这些例子 `mv->mem` 的内存是 hypervisor 的，到底什么是 hypervisor ?

Registration of syscall handler can be achieved via setting special registers named `MSR (Model Specific Registers)`. We can get/set MSR in hypervisor through `ioctl` on `vcpufd`, or in kernel using instructions `rdmsr` and `wrmsr`.

> 其实代码的所有的细节应该被仔细的理解清楚 TODO
> 1. 经典的 while(1) 循环，然后处理各种情况的结构在哪里
> 2. 似乎直接介绍了内核的运行方式而已


## container
kata 和 firecracker :


[^3] 的记录，clearcontainer 停止维护，只是一个宣传的文章，关于 memory overhead 的使用 DAX 有点意思。


## virtio
问题 : 
2. 利用 virtqueue 解决了高效传输的数据的问题，那么中断虚拟化怎么办 ?


[^7] 的记录:
动机:
Linux supports 8 distinct virtualization systems:
- Xen, KVM, VMWare, ...
- Each of these has its own block, console, network, ... drivers

VirtIO – The three goals
- Driver unification
- Uniformity to provide a common ABI for general publication and use of buffers
- Device probing and configuration

Virtqueue 
- It is a part of the memory of the
guest OS
- A channel between front-end and back-end
- It is an interface Implemented as
Vring 
  - Vring is a memory mapped region between QEMU and guest OS
  - Vring is the memory layout of the virtqueue abstraction




[^4] 的记录:
The end goal of the process is to try to create a straightforward, efficient, and extensible standard.

- "Straightforward" implies that, to the greatest extent possible, devices should use existing bus interfaces. Virtio devices see something that looks like a standard PCI bus, for example; there is to be no "boutique hypervisor bus" for drivers to deal with. 
-  "Efficient" means that batching of operations is both possible and encouraged; interrupt suppression is supported, as is notification suppression on the device side. 
- "Extensible" is handled with feature bits on both the device and driver sides with a negotiation phase at device setup time; this mechanism, Rusty said, has worked well so far. And the standard defines a common ring buffer and descripor mechanism (a "virtqueue") that is used by all devices; the same devices can work transparently over different transports.
> changes for virtio 1.0 之后没看，先看个更加简单的吧!

[^5] 的记录:
Linux offers a variety of hypervisor solutions with different attributes and advantages. Examples include the Kernel-based Virtual Machine (KVM), lguest, and User-mode Linux
> @todo 忽然不知道什么叫做 hypervisor 了

Rather than have a variety of device emulation mechanisms (for network, block, and other drivers), virtio provides a common front end for these device emulations to standardize the interface and increase the reuse of code across the platforms.

> paravirtualization 和 virtualization 的关系
In the full virtualization scheme, the hypervisor must emulate device hardware, which is emulating at the lowest level of the conversation (for example, to a network driver). Although the emulation is clean at this abstraction, it’s also the most inefficient and highly complicated. In the paravirtualization scheme, the guest and the hypervisor can work cooperatively to make this emulation efficient. The downside to the paravirtualization approach is that the operating system is aware that it’s being virtualized and requires modifications to work.
![](https://developer.ibm.com/developer/articles/l-virtio/images/figure1.gif)

Here, the guest operating system is aware that it’s running on a hypervisor and includes drivers that act as the front end. The hypervisor implements the back-end drivers for the particular device emulation. These front-end and back-end drivers are where virtio comes in, providing a standardized interface for the development of emulated device access to propagate code reuse and increase efficiency.

![](https://developer.ibm.com/developer/articles/l-virtio/images/figure2.gif)

> 代码结构
![](https://developer.ibm.com/developer/articles/l-virtio/images/figure4.gif)


Guest (front-end) drivers communicate with hypervisor (back-end) drivers through buffers. For an I/O, the guest provides one or more buffers representing the request.

Linking the guest driver and hypervisor driver occurs through the `virtio_device` and most commonly through `virtqueues`. The `virtqueue` supports its own API consisting of five functions. 
1. add_buf
2. kick
3. get_buf
4. enable_cb
5. disable_cb

> 具体的例子 : blk 大致 1000 行，net 大致 3000 行，在 virtio 中间大致 6000 行
You can find the source to the various front-end drivers within the ./drivers subdirectory of the Linux kernel. 
1. The virtio network driver can be found in ./drivers/net/virtio_net.c, and 
2. the virtio block driver can be found in ./drivers/block/virtio_blk.c. 
3. The subdirectory ./drivers/virtio provides the implementation of the virtio interfaces (virtio device, driver, virtqueue, and ring). 

## Intel VT-x
[wiki](https://en.wikipedia.org/wiki/X86_virtualization#Intel_virtualization_(VT-x))


## TODO
几个中文的 blog 讲解原理还是比较清楚的，已经保存

资料之所以没有办法搜索到，是因为没有结合具体的技术搜索 kvm mmu 试试.

https://github.com/cloudius-systems/osv/wiki/Running-OSv-image-under-KVM-QEMU : 有意思，可以测试一下
## 待处理的资源
https://github.com/google/novm : 快速开发，然后忽然停止, go 语言写的，10000行左右



[^1]: https://lwn.net/Articles/658511/
[^2]: https://github.com/kvmtool/kvmtool
[^3]: [An Introduction to Clear Containers](https://lwn.net/Articles/644675/)
[^4]: [Standardizing virtio](https://lwn.net/Articles/580186/)
[^5]: https://developer.ibm.com/articles/l-virtio/
[^6]: https://developer.ibm.com/tutorials/l-hypervisor/
[^7]: https://www.cs.cmu.edu/~412/lectures/Virtio_2015-10-14.pdf
[^8]: https://david942j.blogspot.com/2018/10/noe-learning-kvm-implement-your-own.htmlt
[^9]: https://binarydebt.wordpress.com/201810/14/intel-virtualisation-how-vt-x-kvm-and-qemu-work-together//
[^10]: https://www.kernel.org/doc/html/latest/virt/kvm/index.html
