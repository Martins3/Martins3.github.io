# KVM

## TODO
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


## 问题
- [ ] 如果 kvm 中间跑一个支持 multicore 的 OS，kvm 从一个 cpu 中间启动，最后可以迁移到其他的 CPU 中间
    - [ ] 如果 kvm 的 CPU 数量能否动态扩展 ？

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


# kvm

- [ ] put anything understand ./virt here

## 分析一下
https://www.owalle.com/2019/02/20/kvm-src-analysis

循环依赖 ?
x86.c : 存放整个 x86 通用的函数，emulate.c 和 vmx.c 中间都会使用的代码
vmx.c : 处理各种 exit 的操作, 其中可能会调用 emulate.c 的那处理
emulate.c : 各种指令的模拟


## 关键的数据结构
```c
struct kvm_x86_ops // 难道是为了将 kvm_get_msr 对于不同的 x86 架构上 ?

struct x86_emulate_ops // 定义的函数都是给 emulate.c 使用

struct vcpu_vmx {
	struct kvm_vcpu       vcpu;
  ...
}

/*
 * x86 supports 4 paging modes (5-level 64-bit, 4-level 64-bit, 3-level 32-bit,
 * and 2-level 32-bit).  The kvm_mmu structure abstracts the details of the
 * current mmu mode.
 */
struct kvm_mmu {
```


## TODO

```c
  kvm_mmu_gva_to_gpa_read:5516
  kvm_mmu_gva_to_gpa_fetch:5523
  kvm_mmu_gva_to_gpa_write:5531
  kvm_mmu_gva_to_gpa_system:5540
```
- [ ] https://www.cnblogs.com/ck1020/p/6920765.html 其他的文章

- [ ] https://www.kernel.org/doc/ols/2007/ols2007v1-pages-225-230.pdf
    - 看看 KVM 的总体上层架构怎么回事
- [ ] x86.c :  mmio / pio 的处理
- [ ] emulate.c 中间模拟的指令数量显然是远远没有达到实际上指令数量的，而且都是各种基本指令的模拟
  - [ ] 为什么要进行这些模拟, vmx 的各种 handle 函数为什么反而不能处理这些简单的指令
  - [ ] 很多操作依赖于 vcs read / write ，但是这里仅仅是利用 `ctxt->ops` 然后读 vcpu 中的内容
- [ ] vcpu 的 regs 和 vmcs 的 regs 的关系是什么 ?
- [ ] cpuid.c 为什么有 1000 行,  kvm_emulate_cpuid  和 ioctl API
- [ ] 调查一下 kvm_vcpu_gfn_to_hva
- [x] kvm 的 host va 的地址在哪里 ? 在使用 kvm 的线程的用户空间中
- [ ] mmu 和 flush 和 zap 有什么区别 ?
- [ ] ept 和 shadow page table 感觉处理方法类似了: 都是 for_each_shadow_entry，kvm_mmu_get_page, link_shadow_page 和 mmu_set_spte
    - [ ] `FNAME(fetch)`
    - [ ] `__direct_map`

- [ ] 对于 shadow page table, 不同的 process 都有一套，不同 process 的 cr3 的加载是什么时候 ?
- [ ] 在 FNAME(page_fault) 的两个步骤判断，当解决了 guest page table 的问题之后，依旧发生 page fault, 此时添加上的 shadow page table 显然可以 track 上
- [ ] dirty log



## 函数调用路径

```c
int kvm_arch_vcpu_ioctl_run(struct kvm_vcpu *vcpu) // x86.c
  static int vcpu_run(struct kvm_vcpu *vcpu) // x86.c
    static int vcpu_enter_guest(struct kvm_vcpu *vcpu) // x86.c
      static int vmx_handle_exit(struct kvm_vcpu *vcpu, fastpath_t exit_fastpath) // vmx.c
        static int (*kvm_vmx_exit_handlers[])(struct kvm_vcpu *vcpu) = { // vmx.c
          int __kvm_get_msr(struct kvm_vcpu *vcpu, u32 index, u64 *data, bool host_initiated)
            -> static int vmx_get_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
```
当 vmx 返回值大于 0 的时候，会将结果返回给用户空间，用户空间处理。



```c
gpa_t kvm_mmu_gva_to_gpa_read(struct kvm_vcpu *vcpu, gva_t gva,
			      struct x86_exception *exception)

// 最终在 handle_exception_nmi
```


## x86.c overview
- VMCS 的 IO
- timer pvclock tsc
- ioctl

- pio mmio 和 一般的 IO 的模拟
- emulate


1. debugfs
```c
static struct kmem_cache *x86_fpu_cache;
static struct kmem_cache *x86_emulator_cache;
```
2. kvm_on_user_return :
    1. user return ?
    2. share msr

3. exception_type

4. payload

提供了很多函数访问设置 vcpu，比如 kvm_get_msr 之类的
1. 谁调用 <- vmx.c 吧 !
2. 实现的方法 : 将其放在 vmcs 中，
从 vmcs 中间读取 : 当想要访问的时候，

- [ ] vmcs 是内存区域，还会放在 CPU 中间，用 指令读写的内容

kvm_steal_time_set_preempted


## details

#### vmx_vcpu_run 
vmx_exit_handlers_fastpath : 通过 omit what 来 fast


#### kvm_read_guest_virt_helper
内核读取 guest 的内存，因为 guest 的使用地址空间是
用户态的，所以
1. gva_to_gpa 的地址切换
		gpa_t gpa = vcpu->arch.walk_mmu->gva_to_gpa(vcpu, addr, access,
2. kvm_vcpu_read_guest_page : copy_to_user 而已




#### kvm_vcpu_ioctl_x86_set_mce 
向 guest 注入错误的方法

kvm_queue_exception

So, what is bank ?


## event injection


#### kvm_vcpu_flush_tlb_all

```c
static void kvm_vcpu_flush_tlb_all(struct kvm_vcpu *vcpu)
{
	++vcpu->stat.tlb_flush;
	kvm_x86_ops.tlb_flush_all(vcpu);
}
```

## emulat.c
init_emulate_ctxt 
x86_emulate_instruction : 

```c
int kvm_emulate_instruction(struct kvm_vcpu *vcpu, int emulation_type)
{
	return x86_emulate_instruction(vcpu, 0, emulation_type, NULL, 0);
}
```

1. emulate_ctxt 的使用位置 :

	struct x86_emulate_ctxt *ctxt = vcpu->arch.emulate_ctxt;

- [x] emulate_ctxt.ops 的调用位置 ? 在 emulate.c 中间

1. 为什么会出现 emulation_instruction 的需求 ?

```c
// 将 kvm_arch_vcpu_create 被 kvm_vm_ioctl_create_vcpu 唯一 call
int kvm_arch_vcpu_create(struct kvm_vcpu *vcpu)
```

#### opcode_table 的使用位置

```c
static const struct opcode opcode_table[256] = {
```

指令编码:
```c
struct opcode {
	u64 flags : 56;
	u64 intercept : 8;
	union {
		int (*execute)(struct x86_emulate_ctxt *ctxt);
		const struct opcode *group;
		const struct group_dual *gdual;
		const struct gprefix *gprefix;
		const struct escape *esc;
		const struct instr_dual *idual;
		const struct mode_dual *mdual;
		void (*fastop)(struct fastop *fake);
	} u;
	int (*check_perm)(struct x86_emulate_ctxt *ctxt);
};
```

## direct_map
- [x] 被调用路径: tdp 的注册函数
- [ ] 做什么的

kvm_tdp_page_fault
=> direct_page_fault : 制作一些 cache
=> `__direct_map`

`__direct_map`
1. for_each_shadow_entry : 在 tdp 中间为什么为什么存在 shadow entry

#### `__direct_map`
1. for_each_shadow_entry : 因为多个 shadow page 映射一个 page table


## vmx.c

pt : https://lwn.net/Articles/741093/ : processor tracing

#### vmx_x86_ops

- struct x86_kvm_ops : vmx_x86_ops 也是其中一种
- x86_kvm_ops : 一个经常访问的变量


提供对于 vmcs 的标准访问，和 kvm_x86_ops 的关系是什么 ?

```c
static struct kvm_x86_init_ops vmx_init_ops __initdata = {
	.cpu_has_kvm_support = cpu_has_kvm_support,
	.disabled_by_bios = vmx_disabled_by_bios,
	.check_processor_compatibility = vmx_check_processor_compat,
	.hardware_setup = hardware_setup,

	.runtime_ops = &vmx_x86_ops,
};

// 在 KVM init 的时候，确定使用何种硬件设置，但是 emulate 还是存在的
int kvm_arch_hardware_setup(void *opaque)
{
  // ...
	memcpy(&kvm_x86_ops, ops->runtime_ops, sizeof(kvm_x86_ops));
  // ...
```

## emulate_ops 和 vmx_x86_ops 的操作对比
- vmx_x86_ops 提供了各种操作的硬件支持.
- vmx 的 kvm_vmx_exit_handlers 需要 emulate 的，但是 emulator 的工作需要从 emulator 中间得到数据



## hyperv.c
模拟 HyperV 的内容, 但是为什么需要模拟 HyperV ?

- kvm_hv_hypercall
- stimer

实在是有点看不懂:
https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/hyper-v-architecture


## 8254 / 8259
KVM_CREATE_IRQCHIP :

https://en.wikipedia.org/wiki/Intel_8253

## irq.c
似乎很短，但是 lapic 很长!


## 中断虚拟化
中断虚拟化的关键在于对中断控制器的模拟，我们知道x86上中断控制器主要有旧的中断控制器PIC(intel 8259a)和适应于SMP框架的IOAPIC/LAPIC两种。

https://luohao-brian.gitbooks.io/interrupt-virtualization/content/qemu-kvm-zhong-duan-xu-ni-hua-kuang-jia-fen-679028-4e2d29.html

查询 GSI 号上对应的所有的中断号:

从 ioctl 到下层，kvm_vm_ioctl 注入的中断，最后更改了 kvm_kipc_state:irr 

kvm_kipc_state 的信息如何告知 CPU ? 通过 kvm_pic_read_irq

## lapic

#### https://luohao-brian.gitbooks.io/interrupt-virtualization/content/kvmzhi-nei-cun-xu-ni531628-kvm-mmu-virtualization.html

```c
struct kvm_memory_slot {
	gfn_t base_gfn;
	unsigned long npages;
	unsigned long *dirty_bitmap;
	struct kvm_arch_memory_slot arch;
	unsigned long userspace_addr;
	u32 flags;
	short id;
};

/*
 * Note:
 * memslots are not sorted by id anymore, please use id_to_memslot()
 * to get the memslot by its id.
 */
struct kvm_memslots {
	u64 generation;
	/* The mapping table from slot id to the index in memslots[]. */
	short id_to_index[KVM_MEM_SLOTS_NUM];
	atomic_t lru_slot;
	int used_slots;
	struct kvm_memory_slot memslots[];
};
```

`hva=base_hva+(gfn-base_gfn)*PAGE_SIZE`

```c
unsigned long gfn_to_hva(struct kvm *kvm, gfn_t gfn)
{
	return gfn_to_hva_many(gfn_to_memslot(kvm, gfn), gfn, NULL);
}

// 关键 : 定位 slot
struct kvm_memory_slot *gfn_to_memslot(struct kvm *kvm, gfn_t gfn)
{
	return __gfn_to_memslot(kvm_memslots(kvm), gfn);
}

// 定位 slot 的核心函数，估计是顺着查询一遍的样子
/*
 * search_memslots() and __gfn_to_memslot() are here because they are
 * used in non-modular code in arch/powerpc/kvm/book3s_hv_rm_mmu.c.
 * gfn_to_memslot() itself isn't here as an inline because that would
 * bloat other code too much.
 *
 * IMPORTANT: Slots are sorted from highest GFN to lowest GFN!
 */
static inline struct kvm_memory_slot *
search_memslots(struct kvm_memslots *slots, gfn_t gfn)
```

> 作用：GVA直接到HPA的地址翻译,真正被VMM载入到物理MMU中的页表是影子页表；
> MMU 会在 mmu 没有命中的时候 crash

获得缺页异常发生时的CR2,及当时访问的虚拟地址；
进入
```
kvm_mmu_page_fault()(vmx.c)->
r = vcpu->arch.mmu.page_fault(vcpu, cr2, error_code);(mmu.c)->
FNAME(page_fault)(struct kvm_vcpu *vcpu, gva_t addr, u32 error_code)(paging_tmpl.h)->
FNAME(walk_addr)() 
```
查guest页表，物理地址是否存在， 这时肯定是不存在的
The page is not mapped by the guest. Let the guest handle it.
`inject_page_fault()->kvm_inject_page_fault()` 异常注入流程；

> 只要是 mmu 中间访问失败都是需要进行 vm exit 的，如果发现是 guest 的问题，那么通知 guest
> TODO 找到对于 guest 的 page table 进行 walk 的方法
> Guest 搞定之后，那么
> TODO TLB 的查找不到，被 VMM 截获应该是需要 硬件支持的吧!

为了快速检索GUEST页表所对应的的影子页表，KVM 为每个GUEST都维护了一个哈希
表，影子页表和GUEST页表通过此哈希表进行映射。对于每一个GUEST来说，GUEST
的页目录和页表都有唯一的GUEST物理地址，通过页目录/页表的客户机物理地址就
可以在哈希链表中快速地找到对应的影子页目录/页表。
> 显然不可能使用保存所有的物理地址，从虚拟机只会将虚拟机使用的物理地址处理掉

> 填充过程

mmu_alloc_root =>
`__direct_map` => kvm_mmu_get_page =>


感觉这里还是 shadow 的处理机制，那么 ept 在哪里 ?
```c
static int __direct_map(struct kvm_vcpu *vcpu, gpa_t gpa, int write,
			int map_writable, int max_level, kvm_pfn_t pfn,
			bool prefault, bool account_disallowed_nx_lpage)
{
  // TODO 是在对于谁进行 walk ? 应该不是是对于 shadow page 进行的
  // shadow page 也是划分为 leaf 和 nonleaf 的，也就是这是对于 shadow 的
  // 
  // shadow page 形成一个层次结构的目的是什么 ?
	struct kvm_shadow_walk_iterator it;
	struct kvm_mmu_page *sp;
	int level, ret;
	gfn_t gfn = gpa >> PAGE_SHIFT;
	gfn_t base_gfn = gfn;

	if (WARN_ON(!VALID_PAGE(vcpu->arch.mmu->root_hpa)))
		return RET_PF_RETRY;

  // TODO level generation 的含义
  // level : 难道 shadow page table 也是需要多个 level
	level = kvm_mmu_hugepage_adjust(vcpu, gfn, max_level, &pfn);

	for_each_shadow_entry(vcpu, gpa, it) {
		/*
		 * We cannot overwrite existing page tables with an NX
		 * large page, as the leaf could be executable.
		 */
		disallowed_hugepage_adjust(it, gfn, &pfn, &level);

		base_gfn = gfn & ~(KVM_PAGES_PER_HPAGE(it.level) - 1);
		if (it.level == level)
			break;

		drop_large_spte(vcpu, it.sptep);
		if (!is_shadow_present_pte(*it.sptep)) {
			sp = kvm_mmu_get_page(vcpu, base_gfn, it.addr,
					      it.level - 1, true, ACC_ALL);

			link_shadow_page(vcpu, it.sptep, sp);
			if (account_disallowed_nx_lpage)
				account_huge_nx_page(vcpu->kvm, sp);
		}
	}

	ret = mmu_set_spte(vcpu, it.sptep, ACC_ALL,
			   write, level, base_gfn, pfn, prefault,
			   map_writable);
	direct_pte_prefetch(vcpu, it.sptep);
	++vcpu->stat.pf_fixed;
	return ret;
}
```
==> kvm_mmu_get_page : 应该修改为 get_shadow_page
==> kvm_page_table_hashfn : 利用 gfn 作为 hash 快速定位 shadow_page
==> kvm_mmu_alloc_page : 分配并且初始化一个 shadow page table

注意 : shadow page table 似乎可以存放 shadow page table entry 的

**TODO** 调查 kvm_mmu_alloc_page 的创建的 kvm_mmu_page 的管理内容, 似乎 rule 说明了很多东西

The hypervisor computes the guest virtual to
host physical mapping on the fly and stores it in
a new set of page tables

https://www.linux-kvm.org/images/e/e5/KvmForum2007%24shadowy-depths-of-the-kvm-mmu.pdfhttps://www.linux-kvm.org/images/e/e5/KvmForum2007%24shadowy-depths-of-the-kvm-mmu.pdf

emmmm : 一个物理页面，在 host 看来是给 host 使用的，write protect  可以在 guest 中间，
也是可以放在 host 中间。

emmmm : 什么情况下，一个 hva 可以被多个 gpa 映射 ?

对于 guest 的那些 page table，需要通过 `page->private` 关联起来.

- When we shadow a guest page, we iterate over
the reverse map and remove write access

- When adding write permission to a page, we
check whether the page has a shadow

- **We can have multiple shadow pages for a
single guest page – one for each role**

#### shadow page descriptor
TODO : shadow page table 在 TLB miss 的时候，触发 exception 吗 ?

- [x] 既然 hash table 可以查询，为什么还要建立 hierarchy 的 shadow page table ?
- [x] hash page table 中间放置所有的从 gva 到 hpa 的地址 ?

- 建立 hash 是为了让 guest 的 page table 和 host 的 shadow page table 之间可以快速查找.
- shadow page table : gva 到 hpa 的映射，这个映射是一个 tree 的结构


## sync shadow page
1. 利用 generation 来实现定位 ?

```c
static bool is_obsolete_sp(struct kvm *kvm, struct kvm_mmu_page *sp)
{
	return sp->role.invalid ||
	       unlikely(sp->mmu_valid_gen != kvm->arch.mmu_valid_gen);
}
```

## trace mmu


## mmu_spte_update
TODO : 为什么会存在一个 writable spte 和 read-only spte 的区分 ?

```c
/* Rules for using mmu_spte_update:
 * Update the state bits, it means the mapped pfn is not changed.
 *
 * Whenever we overwrite a writable spte with a read-only one we
 * should flush remote TLBs. Otherwise rmap_write_protect
 * will find a read-only spte, even though the writable spte
 * might be cached on a CPU's TLB, the return value indicates this
 * case.
 *
 * Returns true if the TLB needs to be flushed
 */
static bool mmu_spte_update(u64 *sptep, u64 new_spte)
```

核心就是 WRITE_ONCE 而已，但是存在很多检查

## ept 

tdp_page_fault()->
gfn_to_pfn(); GPA到HPA的转化分两步完成，分别通过gfn_to_hva、hva_to_pfn两个函数完成
`__direct_map()`; 建立EPT页表结构

为什么 ept 也是需要建立一个 shadow page table ?


kvm_tdp_page_fault 和 ept_page_fault 的关系是什么 ?

## paging_tmpl.h

We need the mmu code to access both 32-bit and 64-bit guest ptes,
so the code in this file is compiled twice, once per pte size.

- [x] 如何实现多次编译 ? 目的应该是提供三种不同编译属性的文件，其中只是少量偏移量的修改。通过三次 include 解决.
- [ ] 如果 guest 使用 transparent huge page 的时候，其提供的 page walk 怎么办 ?


```c
static void shadow_mmu_init_context(struct kvm_vcpu *vcpu, struct kvm_mmu *context,
				    u32 cr0, u32 cr4, u32 efer,
				    union kvm_mmu_role new_role)
{
	if (!(cr0 & X86_CR0_PG))
		nonpaging_init_context(vcpu, context);
	else if (efer & EFER_LMA)
		paging64_init_context(vcpu, context);
	else if (cr4 & X86_CR4_PAE)
		paging32E_init_context(vcpu, context);
	else
		paging32_init_context(vcpu, context);

	context->mmu_role.as_u64 = new_role.as_u64;
	reset_shadow_zero_bits_mask(vcpu, context);
}
```
> 都是提供的 shadow 的情况，那么 ept 和 tdp 所以没有出现 ?

## shadow page table
- [ ] shadow page table 是放在 qemu 的空间中间，还是内核地址空间
  - guest 通过 cr3 可以来访问
  - 内核可以操控 page table
- [ ] guest 的内核 vmalloc 修改 page table，是首先修改 shadow page table 造成的异常，然后之后才修改 guest page table ?
    - [ ] shadow page table 各个级别存放的地址是什么 ? 物理地址，因为是让 cr3 使用的
    - [x] guest page table 的内容 ? GVA 也就是 host 的虚拟地址
- [x] `FNAME(walk_addr)()` 存储的地址都是 guest 的虚拟地址 ? 是的，所以应该很容易 walk.

> FNAME(walk_addr)() 查 guest页表，物理地址是否存在，这时肯定是不存在的
`inject_page_fault()->kvm_inject_page_fault()` 异常注入流程；

在 Host 中间检查发现不存在，然后在使用 inject pg 到 guest.
因为 guest page table 存在多个模型

让 Host 越俎代庖来走一遍 guest 的 page walk，shadow page table 是 CR3 中间实际使用的 page table.
-> 使用 spt ，出现 exception 是不知道到底哪一个层次出现问题的, 所以都是需要抛出来检查的
-> *那么当 guest 通过 cr3 进行修改 shadow page table 的时候，通过 write protection 可以找到 ?*
-> *好像 shadow page 只能存放 512 个 page table entry,  利用 cr3 访问真的没有问题吗 ?*

> 影子页表又是载入到CR3中真正为物理MMU所利用进行寻址的页表，因此开始时任何的内存访问操作都会引起缺页异常；导致vm发生VM Exit；进入handle_exception();

## ept page table
- [ ] ept 和 shadow page table 不应该共享结构啊

shadow page table : gva => hpa
ept : 应该是 GPA 到 HPA

- init_kvm_tdp_mmu
- kvm_mmu_alloc_page  : 申请 kvm_mmu_page 空间，该结构表示 EPT 页表项
- vmx_load_mmu_pgd : 传入的root_hpa也就直接当Guest CR3用，其实就是影子页表的基址。

- 当CPU访问EPT页表查找HPA时，发现相应的页表项不存在，则会发生EPT Violation异常，导致VM-Exit

**GPA到HPA的映射关系由EPT页表来维护**

## ept 和 shadow page table 中间的内容
- ept 和 shadow page table 的格式相同，让硬件访问可以格式相同
- 维护 ept 是使用软件的方法维护的，那么 ept 都是物理地址

pgd : page global directory


kvm_mmu_load_pgd : `vcpu->arch.mmu->root_hpa` 作为参数传递出去

kvm_init_mmu : 处理三种 mmu 初始化
  -> init_kvm_softmmu : shadow
  -> init_kvm_tdp_mmu

## 找到 shadow 以及 ept 的 page table entry


## mmu_alloc_root
调用 kvm_mmu_get_page，但是其利用 hash 来查找，说好的 hash 是用于 id 的啊

## arch.mmu->root_hpa 和 arch.mmu->root_pgd
- [x] 是不是 root_hpa 被 direct 使用，root_pgd 被 shadow 使用
  - 并不是，都依赖于 hpa 进行 page walk，而 root_pgd 就是 guest cr3 的值，这是 GPA


mmu_alloc_shadow_roots : `root_pgd = vcpu->arch.mmu->get_guest_pgd(vcpu);`
mmu_alloc_direct_roots : root_pgd = 0


get_guest_pgd 的一般注册函数:
```c
static unsigned long get_cr3(struct kvm_vcpu *vcpu)
{
	return kvm_read_cr3(vcpu);
}

// 读取 cr3 似乎不是一定会从 vmcs 中间读取
static inline ulong kvm_read_cr3(struct kvm_vcpu *vcpu)
{
	if (!kvm_register_is_available(vcpu, VCPU_EXREG_CR3))
		kvm_x86_ops.cache_reg(vcpu, VCPU_EXREG_CR3);
	return vcpu->arch.cr3;
}
```




1. `arch.mmu->root_hpa` 的初始化

mmu_alloc_direct_roots
```c
static int mmu_alloc_roots(struct kvm_vcpu *vcpu)
{
	if (vcpu->arch.mmu->direct_map)
		return mmu_alloc_direct_roots(vcpu);
	else
		return mmu_alloc_shadow_roots(vcpu);
}
```
## memory in kernel or qumu process
luohao's blog:

- [ ] rmap 字段的解释，那么 memory 是 vmalloc 分配的 ?????
  - [ ] vmalloc 的分配是 page fault 的吗 ?

```c
struct kvm_memory_slot {
    gfn_t base_gfn;                    // 该块物理内存块所在guest 物理页帧号
    unsigned long npages;              //  该块物理内存块占用的page数
    unsigned long flags;
    unsigned long *rmap;               // 分配该块物理内存对应的host内核虚拟地址（vmalloc分配）
    unsigned long *dirty_bitmap;
    struct {
        unsigned long rmap_pde;
        int write_count;
    } *lpage_info[KVM_NR_PAGE_SIZES - 1];
    unsigned long userspace_addr;       // 用户空间地址（QEMU)
    int user_alloc;
};
```

## rmap
https://www.cnblogs.com/ck1020/p/6920765.html

在KVM中，逆向映射机制的作用是类似的，但是完成的却不是从HPA到对应的EPT页表项的定位，
而是从gfn到*对应的页表项*的定位。
*理论上讲根据gfn一步步遍历EPT也未尝不可，但是效率较低*况且在EPT所维护的页面不同于host的页表，*理论上讲是虚拟机之间是禁止主动的共享内存的*，为了提高效率，就有了当前的逆向映射机制。

- rmap: from guest page to shadow ptes that map it
- Shadow hash: from guest page to its shadow
- Parent pte chain: from shaow page to upperlevel shadow page
- Shadow pte: from shadow page to lower-level shadow page
- LRU: all active shadow pages

Walk the shadow page table, instantiating page tables as necessary
- Can involve an rmap walk and *write protecting the guest page table*


```c
struct kvm_arch_memory_slot {
  // 应该是一种 page size 然后提供一种 rmap 吧
	struct kvm_rmap_head *rmap[KVM_NR_PAGE_SIZES];
	struct kvm_lpage_info *lpage_info[KVM_NR_PAGE_SIZES - 1];
	unsigned short *gfn_track[KVM_PAGE_TRACK_MAX];
};

#define KVM_MAX_HUGEPAGE_LEVEL	PG_LEVEL_1G
#define KVM_NR_PAGE_SIZES	(KVM_MAX_HUGEPAGE_LEVEL - PG_LEVEL_4K + 1)

enum pg_level {
	PG_LEVEL_NONE,
	PG_LEVEL_4K,
	PG_LEVEL_2M,
	PG_LEVEL_1G,
	PG_LEVEL_512G,
	PG_LEVEL_NUM
};
```

```c
static int kvm_alloc_memslot_metadata(struct kvm_memory_slot *slot,
				      unsigned long npages)
    // 每一个 page 都会建立一个
		slot->arch.rmap[i] =
			kvcalloc(lpages, sizeof(*slot->arch.rmap[i]),
    // ....
}

// mmu_set_spte 的地方调用
static int rmap_add(struct kvm_vcpu *vcpu, u64 *spte, gfn_t gfn)
{
	struct kvm_mmu_page *sp;
	struct kvm_rmap_head *rmap_head;

  // 通过 pte 的指针，获取 spte 指向的 pte 所在的 page 的
	sp = sptep_to_sp(spte);
  // shadow 和 direct 都是需要 rmap
  // 但是，direct 其实并不会注册
	kvm_mmu_page_set_gfn(sp, spte - sp->spt, gfn);
	rmap_head = gfn_to_rmap(vcpu->kvm, gfn, sp);
	return pte_list_add(vcpu, spte, rmap_head);
}
```

```c
static gfn_t kvm_mmu_page_get_gfn(struct kvm_mmu_page *sp, int index)
{
	if (!sp->role.direct)
		return sp->gfns[index];

  // TODO guest 的物理页面应该就是连续的啊!
  // 当 level 在最底层的时候，sp->gfn + index 就可以了啊!
	return sp->gfn + (index << ((sp->role.level - 1) * PT64_LEVEL_BITS));
}


static struct kvm_rmap_head *gfn_to_rmap(struct kvm *kvm, gfn_t gfn,
					 struct kvm_mmu_page *sp)
{
	struct kvm_memslots *slots;
	struct kvm_memory_slot *slot;

	slots = kvm_memslots_for_spte_role(kvm, sp->role);
	slot = __gfn_to_memslot(slots, gfn);
	return __gfn_to_rmap(gfn, sp->role.level, slot);
}
```


- [ ] 建立反向映射的原因是 : 当 shadow page table 进行修改之后，需要知道其所在的 gfn
  - [ ] 真的存在根据 shadow page table 到 gfn 的需求吗 ?
- [ ] direct 需要 rmap 吗 ? 显然需要，不然 direct_map 不会调用 rmap_add


```c
	kvm_mmu_page_set_gfn(sp, spte - sp->spt, gfn); // 一个 shadow page 和 gfn 的关系
	rmap_head = gfn_to_rmap(vcpu->kvm, gfn, sp);
	return pte_list_add(vcpu, spte, rmap_head); // slot 的每一个 page 都会被 rmap
```

实际上，存在两个 rmap 
- `sp->gfns` 获取每一个 pte 对应的 gfn
- `rmap_head->val` = spte : 这不是 rmap 吧

#### parent rmap
```c
static void mmu_page_add_parent_pte(struct kvm_vcpu *vcpu,
				    struct kvm_mmu_page *sp, u64 *parent_pte)
{
	if (!parent_pte)
		return;

	pte_list_add(vcpu, parent_pte, &sp->parent_ptes);
}
```

#### rmap iterator
- [x] rmap 总是构建的 rmap_head 到 sptep 吗 ?
  - rmap_add 和 mmu_page_add_parent_pte 都是的

解析 for_each_rmap_spte
```c
#define for_each_rmap_spte(_rmap_head_, _iter_, _spte_)			\
	for (_spte_ = rmap_get_first(_rmap_head_, _iter_);		\
	     _spte_; _spte_ = rmap_get_next(_iter_))
```
使用位置: 
kvm_mmu_write_protect_pt_masked : 给定 gfn_offset，将关联的所有的 spte 全部添加 flags

kvm_set_pte_rmapp : 将 rmap_head 的持有的所有的 sptep 进行设置



## parent_ptes
```c
static void kvm_mmu_mark_parents_unsync(struct kvm_mmu_page *sp)
{
	u64 *sptep;
	struct rmap_iterator iter;

	for_each_rmap_spte(&sp->parent_ptes, &iter, sptep) {
		mark_unsync(sptep);
	}
}

static void mark_unsync(u64 *spte)
{
	struct kvm_mmu_page *sp;
	unsigned int index;

	sp = sptep_to_sp(spte);
	index = spte - sp->spt;
	if (__test_and_set_bit(index, sp->unsync_child_bitmap))
		return;
	if (sp->unsync_children++)
		return;
	kvm_mmu_mark_parents_unsync(sp);
}
```
递归向上，当发现存在有人 没有 unsync 的时候，在 unsync_child_bitmap 中间设置标志位，
并且向上传导，直到发现没人检测过

link_shadow_page : mark_unsync 的唯一调用位置
kvm_unsync_page : kvm_mmu_mark_parents_unsync 唯一调用位置

mmu_need_write_protect : 对于sp 

#### mmu_need_write_protect
for_each_gfn_indirect_valid_sp : 一个 gfn 可以
同时对应多个 shadow page，原因是一个 guest page 可以对应多个 shadow page


> hash : 实现 guest page tabel 和 shadow page 的映射

> rmap_add 处理的是 :  gfn 和其对应的 pte 的对应关系


## role.quadrant
作用: 一个 guest 地址对应的 page table

get_written_sptes : 依靠 gpa 的 page_offset 计算出来，然后和 `sp->role.quadrant` 对比

#### obsolete sp

```c
static bool is_obsolete_sp(struct kvm *kvm, struct kvm_mmu_page *sp)
{
	return sp->role.invalid ||
	       unlikely(sp->mmu_valid_gen != kvm->arch.mmu_valid_gen);
}
```

#### gfn_to_rmap
RMAP_RECYCLE_THRESHOLD 居然是 1000

## gfn_track

```diff
 History:        #0
 Commit:         3d0c27ad6ee465f174b09ee99fcaf189c57d567a
 Author:         Xiao Guangrong <guangrong.xiao@linux.intel.com>
 Committer:      Paolo Bonzini <pbonzini@redhat.com>
 Author Date:    Wed 24 Feb 2016 09:51:11 AM UTC
 Committer Date: Thu 03 Mar 2016 01:36:21 PM UTC

 KVM: MMU: let page fault handler be aware tracked page

 The page fault caused by write access on the write tracked page can not
 be fixed, it always need to be emulated. page_fault_handle_page_track()
 is the fast path we introduce here to skip holding mmu-lock and shadow
 page table walking

 However, if the page table is not present, it is worth making the page
 table entry present and readonly to make the read access happy

 mmu_need_write_protect() need to be cooked to avoid page becoming writable
 when making page table present or sync/prefetch shadow page table entries

 Signed-off-by: Xiao Guangrong <guangrong.xiao@linux.intel.com>
 Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```
-  [ ] tracked 的 page 不能被 fixed, 必须被模拟，为啥 ?

gfn_track 其实没有什么特别的，告诉该 页面被 track 了，然后
kvm_mmu_page_fault 中间将会调用 x86_emulate_instruction 来处理，
似乎然后通过 mmu_notifier 使用 kvm_mmu_pte_write 来更新 guest page table

#### page_fault_handle_page_track
direct_page_fault 和 FNAME(page_fault) 调用，
似乎如果被 track，那么这两个函数会返回 RET_PF_EMULATE


## track 机制
track 和 dirty bitmap 实际上是两个事情吧! 

对于加以维护的:
kvm_slot_page_track_add_page :
kvm_slot_page_track_remove_page :
==> update_gfn_track

- [ ] 两个函数，调用 update,  都是对于 gfn_track 的加减 1 而已

分别被 account_shadowed 和 unaccount_shadowed 调用

`__kvm_mmu_prepare_zap_page` : 被各种 zap page 调用，并且配合 commit_zap 使用
=> unaccount_shadowed

kvm_mmu_get_page : 
=> account_shadowed




1. kvm_mmu_page_write

```c
void kvm_mmu_init_vm(struct kvm *kvm)
{
	struct kvm_page_track_notifier_node *node = &kvm->arch.mmu_sp_tracker;

	node->track_write = kvm_mmu_pte_write;
	node->track_flush_slot = kvm_mmu_invalidate_zap_pages_in_memslot;
	kvm_page_track_register_notifier(kvm, node);
}
```
kvm_mmu_get_page: 当不是 direct 模式，那么需要对于 kvm_mmu_alloc_page 的 page 进行 account_shadowed
=> account_shadowed : 
=> kvm_slot_page_track_add_page

**所以，保护的是 shadow page table ?**

```c
static void account_shadowed(struct kvm *kvm, struct kvm_mmu_page *sp)
{
	struct kvm_memslots *slots;
	struct kvm_memory_slot *slot;
	gfn_t gfn;

	kvm->arch.indirect_shadow_pages++;
	gfn = sp->gfn;
	slots = kvm_memslots_for_spte_role(kvm, sp->role);
	slot = __gfn_to_memslot(slots, gfn);

	/* the non-leaf shadow pages are keeping readonly. */
	if (sp->role.level > PG_LEVEL_4K)
		return kvm_slot_page_track_add_page(kvm, slot, gfn,
						    KVM_PAGE_TRACK_WRITE);

	kvm_mmu_gfn_disallow_lpage(slot, gfn);
}
```
- [ ] 为什么不保护 leaf shadow page ?

> TOBECON

## track mode

> - dirty tracking:
>    report writes to guest memory to enable live migration
>    and framebuffer-based displays

原来 tracing 是 dirty 的



```diff
 KVM: page track: add the framework of guest page tracking

 The array, gfn_track[mode][gfn], is introduced in memory slot for every
 guest page, this is the tracking count for the gust page on different
 modes. If the page is tracked then the count is increased, the page is
 not tracked after the count reaches zero

 We use 'unsigned short' as the tracking count which should be enough as
 shadow page table only can use 2^14 (2^3 for level, 2^1 for cr4_pae, 2^2
 for quadrant, 2^3 for access, 2^1 for nxe, 2^1 for cr0_wp, 2^1 for
 smep_andnot_wp, 2^1 for smap_andnot_wp, and 2^1 for smm) at most, there
 is enough room for other trackers

 Two callbacks, kvm_page_track_create_memslot() and
 kvm_page_track_free_memslot() are implemented in this patch, they are
 internally used to initialize and reclaim the memory of the array

 Currently, only write track mode is supported
```

#### gfn_to_memslot_dirty_bitmap
`slot->dirty_bitmap` 都在 kvm_main 上面访问

pte_prefetch_gfn_to_pfn


- [ ] dirty 指的是 谁 相对于 谁 是 dirty 的

```c
/**
 * kvm_vm_ioctl_get_dirty_log - get and clear the log of dirty pages in a slot
 * @kvm: kvm instance
 * @log: slot id and address to which we copy the log
 *
 * Steps 1-4 below provide general overview of dirty page logging. See
 * kvm_get_dirty_log_protect() function description for additional details.
 *
 * We call kvm_get_dirty_log_protect() to handle steps 1-3, upon return we
 * always flush the TLB (step 4) even if previous step failed  and the dirty
 * bitmap may be corrupt. Regardless of previous outcome the KVM logging API
 * does not preclude user space subsequent dirty log read. Flushing TLB ensures
 * writes will be marked dirty for next log read.
 *
 *   1. Take a snapshot of the bit and clear it if needed.
 *   2. Write protect the corresponding page.
 *   3. Copy the snapshot to the userspace.
 *   4. Flush TLB's if needed.
 */
static int kvm_vm_ioctl_get_dirty_log(struct kvm *kvm,
				      struct kvm_dirty_log *log)
{
	int r;

	mutex_lock(&kvm->slots_lock);

	r = kvm_get_dirty_log_protect(kvm, log);

	mutex_unlock(&kvm->slots_lock);
	return r;
}
```

https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2018/08/11/dirty-pages-tracking-in-migration

> So here for every gfn, we remove the write access. After return from this ioctl, the guest’s RAM has been marked no write access, every write to this will exit to KVM make the page dirty. This means ‘start the dirty log’.


- [ ] kvm_mmu_slot_apply_flags : 实际作用是 dirty log

## kvm_sync_page
kvm_sync_pages : 对于 gfn (其实是 gva 关联的 vcpu) 全部更新, 通过调用 kvm_sync_page

kvm_mmu_sync_roots : 从根节点更新更新 => (mmu_sync_children : 将整个 children 进行 sync)

最终调用 sync_page 函数指针维持生活







## mmio
- [ ] 对于 host 而言，存在 pcie 分配 mmio 的地址空间，在虚拟机中间，这一个是如何分配的 MMIO 空间的

```c
static bool is_mmio_spte(u64 spte)
{
	return (spte & SPTE_SPECIAL_MASK) == SPTE_MMIO_MASK;
}
```

- generation 只是为了 MMIO 而处理的


> - if the RSV bit of the error code is set, the page fault is caused by guest
>  accessing MMIO and cached MMIO information is available.
>
>  - walk shadow page table
>  - check for valid generation number in the spte (see "Fast invalidation of
>    MMIO sptes" below)
>  - cache the information to `vcpu->arch.mmio_gva`, `vcpu->arch.mmio_access` and
>    `vcpu->arch.mmio_gfn`, and call the emulator


## mmio generation
👇记录 mmu.rst 的内容:
虽然的确解释了 mmio 使用 generation 的原因，但是下面的问题值得理解:
- [ ] As mentioned in "Reaction to events" above, kvm will cache MMIO information in leaf sptes. 
  - [ ] 如果不 cache, 这些数据放在那里

- [ ] When a new memslot is added or an existing memslot is changed, this information may become stale and needs to be invalidated.
  - [ ] 为什么 memslot 增加，导致数据失效

Unfortunately, a single memory access might access kvm_memslots(kvm) multiple
times, the last one happening when the generation number is retrieved and
stored into the MMIO spte.  Thus, the MMIO spte might be created based on
out-of-date information, but with an up-to-date generation number.

- [ ] To avoid this, the generation number is incremented again after synchronize_srcu
returns;

- [ ] 找到访问 pte 来比较 generation, 发现 out of date，然后 slow path 的代码

## TODO : shadow flood


## kvm_main
kvm_is_zone_device_pfn

- [ ] zone device


#### vcpu_load
- [ ] check 一下使用的位置
- [ ] preempt_notifier_register : 神奇的 notifier 机制
- [ ] 和 vcpu_put 的结合分析一下

```c
/*
 * Switches to specified vcpu, until a matching vcpu_put()
 */
void vcpu_load(struct kvm_vcpu *vcpu)
{
	int cpu = get_cpu();

	__this_cpu_write(kvm_running_vcpu, vcpu);
	preempt_notifier_register(&vcpu->preempt_notifier);
	kvm_arch_vcpu_load(vcpu, cpu);
	put_cpu();
}
```

#### kvm_vm_ioctl_set_memory_region

#### kvm_vcpu_unmap

#### kvm_read_guest
- [ ] 为什么要处理 guest page 机制

#### kvm_vcpu_fault
> 配合 vcpu ioctl 
```c
static int create_vcpu_fd(struct kvm_vcpu *vcpu)
{
	char name[8 + 1 + ITOA_MAX_LEN + 1];

	snprintf(name, sizeof(name), "kvm-vcpu:%d", vcpu->vcpu_id);
	return anon_inode_getfd(name, &kvm_vcpu_fops, vcpu, O_RDWR | O_CLOEXEC);
}
```

#### kvm device ioctl
> TODO

#### kvm io bus write

kvm_io_bus_write => `__kvm_io_bus_write`

```c
struct kvm_io_bus {
	int dev_count;
	int ioeventfd_count;
	struct kvm_io_range range[];
};
```
KVM: Adds support for in-kernel mmio handlers


## unsorted resource
- Extended page-table mechanism (EPT) used to support the virtualization of physical memory.
- **Translates the guest-physical addresses used in VMX non-root operation.**
- Guest-physical addresses are translated by traversing a set of EPT paging structures to produce physical addresses that are used to access memory.


> 1. 对于 page table 的翻译 : 让硬件完成其中的插入工作，这样就不使用 shadow table
> 2. 使用 TLB 进行翻译


> TLB 被划分为两个部分，`VA->PA` 和 `PA-VA`


hspt 的想法 : 将内核中间添加一个 mmap 的空间，每一个 process 在一个虚拟地址空间中间，
这个虚拟地址空间直接映射到 host 的一个连续空间中间，那么访问就相当于直接访问了.

- Simplified VMM design. 需要处理 shadow page table 和两级翻译的同步问题
- Guest page table modifications need not be trapped, hence VM exits reduced. 同步
- Reduced memory footprint compared to shadow page table algorithms. shadow table 会占用空间

TLB miss is very costly since guest-physical address to machine address needs an extra EPT walk for each stage of guest-virtual address translation.


## kvm_make_all_cpus_request

## hypercall
https://stackoverflow.com/questions/33590843/implementing-a-custom-hypercall-in-kvm

x86.c: kvm_emulate_hypercall

```c
/* For KVM hypercalls, a three-byte sequence of either the vmcall or the vmmcall
 * instruction.  The hypervisor may replace it with something else but only the
 * instructions are guaranteed to be supported.
 *
 * Up to four arguments may be passed in rbx, rcx, rdx, and rsi respectively.
 * The hypercall number should be placed in rax and the return value will be
 * placed in rax.  No other registers will be clobbered unless explicitly
 * noted by the particular hypercall.
 */

static inline long kvm_hypercall0(unsigned int nr)
{
	long ret;
	asm volatile(KVM_HYPERCALL
		     : "=a"(ret)
		     : "a"(nr)
		     : "memory");
	return ret;
}
```
host 发送 hypercall 的之后，造成从 host 中间退出，然后 最后调用到 kvm_emulate_hypercall, 实际上支持的操作很少

```c
int kvm_emulate_hypercall(struct kvm_vcpu *vcpu)
{
	unsigned long nr, a0, a1, a2, a3, ret;
	int op_64_bit;

  // TODO hyperv 另一种虚拟化方案 ?
  // 一种硬件支持 ?
	if (kvm_hv_hypercall_enabled(vcpu->kvm))
		return kvm_hv_hypercall(vcpu);

```

## resources
- https://github.com/dpw/kvm-hello-world : a good resource to understand how real, protect, long mode in intel
- https://github.com/david942j/kvm-kernel-example

- [Watch this organization](https://github.com/rust-vmm/kvm-bindings)
>  It provides a set of virtualization components that any project can use to quickly develop virtualization solutions while focusing on the key differentiators of their product rather than re-implementing common components like KVM wrappers, virtio devices and other VMM libraries.


- https://github.com/canonical/multipass
  - write with cpp
  - include many cpp 

#### hypervisor-from-scratch-part-4
https://rayanfam.com/topics/hypervisor-from-scratch-part-4/
> TODO intel 手册 Chapter 28 – (VMX SUPPORT FOR ADDRESS TRANSLATION)

> According to a VMware evaluation paper: “EPT provides performance gains of up to 48% for MMU-intensive benchmarks and up to 600% for MMU-intensive microbenchmarks”.

- [ ] 是 shadow table 需要使用 Complicated reverse map 的吗 ?

> **EPT mechanism that treats your Guest Physical Address like a virtual address and the EPTP is the CR3.**

- [ ] cr3 中间存放 eptp 的地址，找到对应的代码 ?

> Note that PAE stands for **Physical Address Extension** which is a memory management feature for the x86 architecture that extends the address space and PSE stands for **Page Size Extension** that refers to a feature of x86 processors that allows for pages larger than the traditional 4 KiB size.
