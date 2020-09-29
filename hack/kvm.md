# 分析一下
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

## nested

指的是 KVM 的 nested, 还是  page table nested ?


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

#### kvm_inject_page_fault

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



## link_shadow_page
难道还有反向映射:
1. mmu_page_add_parent_pte

```c
static bool is_mmio_spte(u64 spte)
{
	return (spte & SPTE_SPECIAL_MASK) == SPTE_MMIO_MASK;
}
```

## mmu

#### mmu.rst

> Guest memory (gpa) is part of the user address space of the process that is
using kvm.  Userspace defines the translation between guest addresses and user
addresses (`gpa->hva`); note that two gpas may alias to the same hva, but not
vice versa.

> These hvas may be backed using any method available to the host: anonymous
memory, file backed memory, and device memory.  Memory might be paged by the
host at any time.

映射的空间是 mmio 的话，如何配置，如何捕获 ?

The principal data structure is the shadow page, 'struct kvm_mmu_page'.

> shadow page
```c
struct kvm_mmu_page {
	struct list_head link;
	struct hlist_node hash_link;
	struct list_head lpage_disallowed_link;

	bool unsync;
	u8 mmu_valid_gen;
	bool mmio_cached;
	bool lpage_disallowed; /* Can't be replaced by an equiv large page */

	/*
	 * The following two entries are used to key the shadow page in the
	 * hash table.
	 */
	union kvm_mmu_page_role role;
	gfn_t gfn;

	u64 *spt;
	/* hold the gfn of each spte inside spt */
	gfn_t *gfns;
	int root_count;          /* Currently serving as active root */
	unsigned int unsync_children;
	struct kvm_rmap_head parent_ptes; /* rmap pointers to parent sptes */
	DECLARE_BITMAP(unsync_child_bitmap, 512);

#ifdef CONFIG_X86_32
	/*
	 * Used out of the mmu-lock to avoid reading spte values while an
	 * update is in progress; see the comments in __get_spte_lockless().
	 */
	int clear_spte_count;
#endif

	/* Number of writes since the last time traversal visited this page.  */
	atomic_t write_flooding_count;
};
```
A shadow page contains 512 sptes, which can be either leaf or nonleaf sptes.  A
shadow page may contain a mix of leaf and nonleaf sptes.
> shadow page : 一个 page，存在 512 bytes
> **leaf and nonleaf**

A nonleaf spte allows the hardware mmu to reach the leaf pages and
is not related to a translation directly.  It points to other shadow pages.

A leaf spte corresponds to either one or two translations encoded into
one paging structure entry.  These are always the lowest level of the
translation stack, with optional higher level translations left to NPT/EPT.
*Leaf ptes point at guest pages.*

> *TODO* : kvm_mmu_page_role 的每一项目的含义

reverse map :
> The mmu maintains a reverse mapping whereby all ptes mapping a page can be
reached given its gfn.  This is used, for example, when swapping out a page.

在 kvm 下的 reverse map 有什么不同之处吗 ?


> *TODO* synchronized and unsynchronized pages
> A special case is when a guest page table is reachable from the current
guest cr3.  In this case, the guest is obliged to issue an invlpg instruction
before using the translation. 

> reachable 为什么反而需要 cr3 来处理


mmio 的 generation number ?

> When KVM finds an MMIO spte, it checks the generation number of the spte.
If the generation number of the spte does not equal the global generation
number, it will ignore the cached MMIO information and handle the page
fault through the slow path.

#### role
>  role.level:
>    The level in the shadow paging hierarchy that this shadow page belongs to.
>    1=4k sptes, 2=2M sptes, 3=1G sptes, etc.


- [] shadow paging 机制似乎是独立于 guest 的类型，不然 for_each_shadow_entry 需要很多种吧
  - [ ] 对于 ept，其实模拟 gpa 的大小
  - [ ] 对于 shadow, 取决于 guest。找到默认设定 guest 的 paging 方式代码的位置


> role.direct:
>   If set, leaf sptes reachable from this page are for a linear range.
>   Examples include real mode translation, large guest pages backed by small
>   host pages, and gpa->hpa translations when NPT or EPT is active.
>   The linear range starts at (gfn << PAGE_SHIFT) and its size is determined
>   by role.level (2MB for first level, 1GB for second level, 0.5TB for third
>   level, 256TB for fourth level)
>   If clear, this page corresponds to a guest page table denoted by the gfn
>   field.

- [] direct 表示 linear range 的，如果 memslot 在用户地址空间，那么其实这很科学

> role.quadrant:
>   When role.gpte_is_8_bytes=0, the guest uses 32-bit gptes while the host uses 64-bit
>   sptes.  That means a guest page table contains more ptes than the host,
>   so multiple shadow pages are needed to shadow one guest page.
>   For first-level shadow pages, role.quadrant can be 0 or 1 and denotes the
>   first or second 512-gpte block in the guest page table.  For second-level
>   page tables, each 32-bit gpte is converted to two 64-bit sptes
>   (since each first-level guest page is shadowed by two first-level
>   shadow pages) so role.quadrant takes values in the range 0..3.  Each
>   quadrant maps 1GB virtual address space.

linux KVM的内存虚拟化源码中存在一种物理页框对EPT spte的反向映射，具体实现在rmap_add()



#### shadow page's gfn

## general mmu

TDP : two dimensional paging

KVM Forum :
- https://static.sched.com/hosted_files/kvmforum2019/25/MMU%20improvements%20KVM%20Forum%20Presentation%20-%20short.pdf
- https://stackoverflow.com/questions/49042280/reverse-mapping-in-kvm

问题是:
1. mmu 的设计不是在 vmx 中间吗 ? mmu.c 中间存放什么内容的 ?

mmu.c 
```c
/*
 * x86 supports 4 paging modes (5-level 64-bit, 4-level 64-bit, 3-level 32-bit,
 * and 2-level 32-bit).  The kvm_mmu structure abstracts the details of the
 * current mmu mode.
 */
struct kvm_mmu {
```

```c
static inline int kvm_mmu_do_page_fault(struct kvm_vcpu *vcpu, gpa_t cr2_or_gpa,
					u32 err, bool prefault)
{
#ifdef CONFIG_RETPOLINE
	if (likely(vcpu->arch.mmu->page_fault == kvm_tdp_page_fault))
		return kvm_tdp_page_fault(vcpu, cr2_or_gpa, err, prefault);
#endif
	return vcpu->arch.mmu->page_fault(vcpu, cr2_or_gpa, err, prefault);
}
```

> 1. 初始化的顺序
> 2. create CPU 的时候调用

```c
void kvm_init_mmu(struct kvm_vcpu *vcpu, bool reset_roots)
{
	if (reset_roots) {
		uint i;

		vcpu->arch.mmu->root_hpa = INVALID_PAGE;

		for (i = 0; i < KVM_MMU_NUM_PREV_ROOTS; i++)
			vcpu->arch.mmu->prev_roots[i] = KVM_MMU_ROOT_INFO_INVALID;
	}

	if (mmu_is_nested(vcpu))
		init_kvm_nested_mmu(vcpu);
	else if (tdp_enabled)
		init_kvm_tdp_mmu(vcpu);
	else
		init_kvm_softmmu(vcpu); // softmmu 就是 shadowmmu
}
EXPORT_SYMBOL_GPL(kvm_init_mmu);
```


```c
#ifdef KVM_ARCH_NR_OBJS_PER_MEMORY_CACHE
/*
 * Memory caches are used to preallocate memory ahead of various MMU flows,
 * e.g. page fault handlers.  Gracefully handling allocation failures deep in
 * MMU flows is problematic, as is triggering reclaim, I/O, etc... while
 * holding MMU locks.  Note, these caches act more like prefetch buffers than
 * classical caches, i.e. objects are not returned to the cache on being freed.
 */
struct kvm_mmu_memory_cache {
	int nobjs;
	gfp_t gfp_zero;
	struct kmem_cache *kmem_cache;
	void *objects[KVM_ARCH_NR_OBJS_PER_MEMORY_CACHE];
};
#endif
```

## async_pf
Asynchronous page fault is a way to try and use guest vcpu more efficiently by allowing it to execute other tasks while page is brought back into memory[1].


路径1:
kvm_mmu_do_page_fault =>
kvm_tdp_page_fault =>
direct_page_fault => try_async_pf => kvm_arch_setup_async_pf => kvm_setup_async_pf : 在其中初始化一个 workqueue 任务，并且放在队列中间

路径2:
```c
kvm_handle_page_fault : 入口函数, 靠近 exit handler 
  - kvm_mmu_page_fault
  - kvm_async_pf_task_wait_schedule
```


#### https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/03/24/kvm-async-page-fault
1. 需要修改内核kvm 外面的代码 ? 不然怎么来识别从 host inject 的
2. 内核如何调度 host 的另一个 task 过来运行的


> 算是最清楚的代码了，TODO

#### https://lwn.net/Articles/817239/


#### ept page fault
1. where is  paging64_gva_to_gpa ?
2. mtrr : https://zhuanlan.zhihu.com/p/51023864 : 还是很迷，这个到底是做什么

```c
kvm_tdp_page_fault
  direct_page_fault: // 重点关注
```

https://stackoverflow.com/questions/60694243/how-does-kvm-qemu-and-guest-os-handles-page-fault

> while the EPT tables are used to translate Guest Physical Addresses into Host Physical Addresses.
>
> If a page is present in the guest page tables but not present in the EPT, it causes an EPT violation VM exit, so the VMM can handle the missing page.



## vpclock
https://opensource.com/article/17/6/timekeeping-linux-vms

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/virtualization_host_configuration_and_guest_installation_guide/chap-virtualization_host_configuration_and_guest_installation_guide-kvm_guest_timing_management

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

#### kvm_arch_end_assignment
- [ ] vfio 和 virtual IO 的关系 ?
- [ ] kvm 下面也有 eventfd 


## memslot
https://blog.csdn.net/sdulibh/article/details/83306327

memslot : HPA 到 GVA 之间的映射关系

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


## nested.c
https://www.kernel.org/doc/Documentation/virtual/kvm/nested-vmx.txt

https://www.usenix.org/legacy/events/osdi10/tech/full_papers/Ben-Yehuda.pdf

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

## eventfd

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


#### sync page
TODO: 解释其中 第 8 页的所有情况:
file:///Users/admin/Downloads/07IBM%20XiaoGuangrong%20kvm-mmu.pdf

TODO : 似乎还有 sync 和 mark unsync 之分


kvm_sync_page : 
kvm_mmu_get_page : 获取 gfn 对应的 shadow page，如果没有，那么创建

=> `__kvm_sync_page` => FNAME(sync_name)

可以确定，更新是从 host 到 shadow 的

- [ ] 保护的 page 应该是 guest 的 page

mmu_spte_update : spte 指向的是物理地址，对于 high level 的 shadow page table，其中的 spte 指向是下一级的 shadow page table

```c
/*
 * Using the cached information from sp->gfns is safe because:
 * - The spte has a reference to the struct page, so the pfn for a given gfn
 *   can't change unless all sptes pointing to it are nuked first.
 *
 * Note:
 *   We should flush all tlbs if spte is dropped even though guest is
 *   responsible for it. Since if we don't, kvm_mmu_notifier_invalidate_page
 *   and kvm_mmu_notifier_invalidate_range_start detect the mapping page isn't
 *   used by guest then tlbs are not flushed, so guest is allowed to access the
 *   freed pages.
 *   And we increase kvm->tlbs_dirty to delay tlbs flush in this case.
 */
static int FNAME(sync_page)(struct kvm_vcpu *vcpu, struct kvm_mmu_page *sp)

	first_pte_gpa = FNAME(get_level1_sp_gpa)(sp);
  // 获取 sp 指向的所有 gfn

	for (i = 0; i < PT64_ENT_PER_PAGE; i++) {
    // pte_gpa 是 spte 持有的 gfn
		pte_gpa = first_pte_gpa + i * sizeof(pt_element_t);
    // 通过 copy_from_user 来获取的 gpte
		if (kvm_vcpu_read_guest_atomic(vcpu, pte_gpa, &gpte,
					       sizeof(pt_element_t)))
    // 通过 gpte 获取 gfn
		gfn = gpte_to_gfn(gpte);
    // 更新 spte
		set_spte_ret |= set_spte(vcpu, &sp->spt[i],
					 pte_access, PG_LEVEL_4K,
					 gfn, spte_to_pfn(sp->spt[i]),
					 true, false, host_writable);
    }
}
```




- [ ] kvm_mmu_notifier_invalidate_range_start
- [ ] kvm_mmu_notifier_invalidate_page

- [ ] rvsd


## kvm_mmu_alloc_page
分配 kvm_mmu_page 和 shadow page，对于非 direct 甚至需要分配一个 gfn 映射数组

```c
	if (!direct) {
		/*
		 * we should do write protection before syncing pages
		 * otherwise the content of the synced shadow page may
		 * be inconsistent with guest page table.
		 */
		account_shadowed(vcpu->kvm, sp); // 对于 non-leaf，使用 page table
		if (level == PG_LEVEL_4K && rmap_write_protect(vcpu, gfn)) // 对于 leaf, 使用 rmap_write_protect
			kvm_flush_remote_tlbs_with_address(vcpu->kvm, gfn, 1);

		if (level > PG_LEVEL_4K && need_sync)
			flush |= kvm_sync_pages(vcpu, gfn, &invalid_list);
	}
```

现在理解一下 rmap_write_protect : 

#### rmap_write_protect

=> kvm_mmu_slot_gfn_write_protect
```c
bool kvm_mmu_slot_gfn_write_protect(struct kvm *kvm,
				    struct kvm_memory_slot *slot, u64 gfn)
{
	struct kvm_rmap_head *rmap_head;
	int i;
	bool write_protected = false;

  // TODO : 为什么 rmap 需要处理多个 levels, 现在的假设是为了
  //  考虑 huge page 的因素
	for (i = PG_LEVEL_4K; i <= KVM_MAX_HUGEPAGE_LEVEL; ++i) {
    // 通过 rmap 获取 guest page table 指向其的 spte
    // 这样的也是可以说清楚，通过在 spte 上添加保护位
    // 就可以实现保护 spte
		rmap_head = __gfn_to_rmap(gfn, i, slot);
		write_protected |= __rmap_write_protect(kvm, rmap_head, true);
	}

	return write_protected;
}
```

- [ ] rmap 为什么是反向的, spte 是 page walk 的结果, 从 spte 到 gfn 才是 rmap 啊
  - [ ] rmap 的真正作用是可以让 gfn 可以同时持有多个 spte
  - [ ] 但是实际上只有一个 gfn 可以到达 sptep 啊

从 mmu_set_spte 中间的关于 `was_rmapped` 变量的分析，
rmap 机制是针对于 2mb 的 PMD 之类的

```c
static int rmap_add(struct kvm_vcpu *vcpu, u64 *spte, gfn_t gfn)
{
	struct kvm_mmu_page *sp;
	struct kvm_rmap_head *rmap_head;

	sp = sptep_to_sp(spte);
  // 从这一句说明，就是 spte 指向 gfn
	kvm_mmu_page_set_gfn(sp, spte - sp->spt, gfn);
	rmap_head = gfn_to_rmap(vcpu->kvm, gfn, sp);
	return pte_list_add(vcpu, spte, rmap_head);
}
```

#### functions of rmap
```c
static inline struct kvm_mmu_page *sptep_to_sp(u64 *sptep)
{
	return to_shadow_page(__pa(sptep));
}

// 如果传入 pte，那么返回的是 pte 指向的 page table 所在的 page frame
static inline struct kvm_mmu_page *to_shadow_page(hpa_t shadow_page)
{
	struct page *page = pfn_to_page(shadow_page >> PAGE_SHIFT);

	return (struct kvm_mmu_page *)page_private(page);
}

static void drop_parent_pte(struct kvm_mmu_page *sp,
			    u64 *parent_pte)
{
	mmu_page_remove_parent_pte(sp, parent_pte);
	mmu_spte_clear_no_track(parent_pte);
}

static void mmu_page_remove_parent_pte(struct kvm_mmu_page *sp,
				       u64 *parent_pte)
{
	__pte_list_remove(parent_pte, &sp->parent_ptes);
}
```

rmap : 多个 parent page table 会指向 同一个下一级 page table

- [ ] why

- kvm_mmu_unlink_parents 和 kvm_mmu_page_unlink_children 可以增强理解 mmu


## write protect
- [x] 到底是保护谁 ?
- [ ] 如何触发, 一个普通的 write page table 的 protection 凭什么惊动 host ?
- [ ] 触发之后的处理机制在哪里 ?

为什么设置保护机制是在 shadow page table 上的 ?

Guest 想要修改 GPTE 的时候，其实也是需要 page walk 的，此时 page walk 经过的就是
shadow page table. 设置保护就是更新一下 spte 的标志位。当 page walk 失败的原因是 
write protect 的时候，显然可以知道是在哪一个 spte 上的，更新 spte 指向的内容。
还用一个可能性是，guest 更新 page table 的时候，总是会首先进行 invlpg


mmu_need_write_protect 的注释:
CPU1: 正确的顺序是 `sp->unsync`，然后 spte writable

- [ ] 是不是host 使用的页面，只有页表被设置为 write protect 的 ?

**当修改 host page table 的时候，该页面由于被 write protection, 
退出，根据 gfn(其实是页表的虚拟地址) 可以找到其对应的 spte,
设置 unsync ，writable，然后继续修改，invlpg，sync page table**

#### mmu_need_write_protect
如果 unsync，那么就是不需要 write protect

- [ ] 为什么会调用 kvm_unsync_page



## two cr3
似乎存在 Guest cr3 和 shadow Cr3

## https://rayanfam.com/topics/hypervisor-from-scratch-part-4/
> TODO intel 手册 Chapter 28 – (VMX SUPPORT FOR ADDRESS TRANSLATION)

> According to a VMware evaluation paper: “EPT provides performance gains of up to 48% for MMU-intensive benchmarks and up to 600% for MMU-intensive microbenchmarks”.

- [ ] 是 shadow table 需要使用 Complicated reverse map 的吗 ?

> **EPT mechanism that treats your Guest Physical Address like a virtual address and the EPTP is the CR3.**

- [ ] cr3 中间存放 eptp 的地址，找到对应的代码 ?

> Note that PAE stands for **Physical Address Extension** which is a memory management feature for the x86 architecture that extends the address space and PSE stands for **Page Size Extension** that refers to a feature of x86 processors that allows for pages larger than the traditional 4 KiB size.

## 一般的 page walk 事情总结一下
1. 那些地方需要修改 page table 的，check 一下代码

Level 0 is PML4E(Page-Map Level-4 Offset)
Level 1 is PDPE(Page-Directory- Pointer Offset)
Level 2 is PDP(Page-Directory Offset)
Level 3 is PTE (Page Table Offset)

> An important note is almost all the above structures have a 36-bit Physical Address which means our hypervisor supports only 4-level paging. It is because every page table (and every EPT Page Table) consist of 512 entries which means you need 9 bits to select an entry and as long as we have 4 level tables, we can’t use more than 36 (4 * 9) bits. Another method with wider address range is not implemented in all major OS like Windows or Linux. I’ll describe EPT PML5E briefly later in this topic but we don’t implement it in our hypervisor as it’s not popular yet!
- https://en.wikipedia.org/wiki/X86-64 : 中间描述的是，寻址范围是 48bit，采用的是 4 个 level 的，甚至有 5 level 的
- [ ] host 的寻址模式和 Guest 的寻址模式一致吗 ?


## root_level 和 shadow_root_level


## page walk

```c
static void shadow_walk_init(struct kvm_shadow_walk_iterator *iterator,
			     struct kvm_vcpu *vcpu, u64 addr)
{
	shadow_walk_init_using_root(iterator, vcpu, vcpu->arch.mmu->root_hpa,
				    addr);
}
```

```c
struct kvm_shadow_walk_iterator {
    u64 addr;            // gfn << PAGE_SHIFT   Guest物理页基址
    hpa_t shadow_addr;   // 当前VM的EPT页表项的物理页机制
    u64 *sptep;          // 指向下一级EPT页表的指针
    int level;           // 当前所处的页表级别，逐级递减
    unsigned index;      // 页表的索引      *sptep + index = 下下一级EPT页表的指针
};
```

- [ ] sptep 是物理地址吗 ?

翻译 shadow page table :
- addr : Guest 虚拟地址
- shadow_addr : Host 物理地址


- [x] 似乎哪里有点问题，direct_map 被 shadow 使用过 ? 并没有
  - [x] 只是都是使用相同的 page walk 机制而已吧

shadow 的存在两种 page walk:
1. guest page table :  FNAME(walk_addr_generic)
2. shadow page table : FNAME(fetch)

```c
static int FNAME(page_fault)(struct kvm_vcpu *vcpu, gpa_t addr, u32 error_code,
			     bool prefault)

	/*
	 * Look up the guest pte for the faulting address.
	 */
	r = FNAME(walk_addr)(&walker, vcpu, addr, error_code);

	r = FNAME(fetch)(vcpu, addr, &walker, write_fault, max_level, pfn,
			 map_writable, prefault, lpage_disallowed);
```
FNAME(page_fault) 存在一个误导，gpa_t addr 指出来地址实际上是 pva 地址
因为 tdp 使用的就是 gpa

```c
static void shadow_walk_init(struct kvm_shadow_walk_iterator *iterator,
			     struct kvm_vcpu *vcpu, u64 addr)
{
	shadow_walk_init_using_root(iterator, vcpu, vcpu->arch.mmu->root_hpa,
				    addr);
}
```

root_hpa
- shadow : shadow page table 的 pdt (根地址)
- ept : ept 根地址

#### shadow_walk_init_using_root
- [ ] 仅仅捕获了两种情况，其他的 ?
- [ ] PT32E_ROOT_LEVEL 的原理是什么 ?

```c
static void shadow_walk_init_using_root(struct kvm_shadow_walk_iterator *iterator,
					struct kvm_vcpu *vcpu, hpa_t root,
					u64 addr)
{
	iterator->addr = addr;
	iterator->shadow_addr = root;
	iterator->level = vcpu->arch.mmu->shadow_root_level;

	if (iterator->level == PT64_ROOT_4LEVEL &&
	    vcpu->arch.mmu->root_level < PT64_ROOT_4LEVEL &&
	    !vcpu->arch.mmu->direct_map)
		--iterator->level;

	if (iterator->level == PT32E_ROOT_LEVEL) {
		/*
		 * prev_root is currently only used for 64-bit hosts. So only
		 * the active root_hpa is valid here.
		 */
		BUG_ON(root != vcpu->arch.mmu->root_hpa);

		iterator->shadow_addr
			= vcpu->arch.mmu->pae_root[(addr >> 30) & 3];
		iterator->shadow_addr &= PT64_BASE_ADDR_MASK;
		--iterator->level;
		if (!iterator->shadow_addr)
			iterator->level = 0;
	}
}
```
## tlb
kvm_flush_remote_tlbs

## zap
- [x] 好像是 free 的意思 ? 是的
  - kvm_mmu_prepare_zap_page
  - kvm_mmu_commit_zap_page

#### make_mmu_pages_available
- 检测 `kvm->arch.n_max_mmu_pages` 和 `kvm->arch.n_used_mmu_pages`
- 然后靠 kvm_mmu_zap_oldest_mmu_pages 维持生活

kvm_mmu_alloc_page : 导致 `kvm->arch.n_used_mmu_pages` ++

- mmu page 就是 页表的意思啊，shadow page 或者 ept page

-  kvm_mmu_zap_oldest_mmu_pages 其实就是根据 active_mmu_pages 的头部的 sp 去掉，知道满足足够的数量





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

## for_each_shadow_entry_lockless

- [ ] lockless 的实现方法
- [ ] 为什么需要 lock
- [ ] fast_page_fault 是怎么回事

## mmu_notifier

- [ ] mmu_notifier 机制的原理是什么
- [ ] mmu_notifier 和 gfn_track 存在什么关联吗 ?

- [ ] 检测 kvm_page_track_notifier_node:node 的使用位置
- [ ] 似乎多个 kvm_page_track_notifier_node 可以挂到一个 kvm_page_track_notifier_head 上

```c
struct kvm_page_track_notifier_node {
	struct hlist_node node;

	/*
	 * It is called when guest is writing the write-tracked page
	 * and write emulation is finished at that time.
	 *
	 * @vcpu: the vcpu where the write access happened.
	 * @gpa: the physical address written by guest.
	 * @new: the data was written to the address.
	 * @bytes: the written length.
	 * @node: this node
	 */
	void (*track_write)(struct kvm_vcpu *vcpu, gpa_t gpa, const u8 *new,
			    int bytes, struct kvm_page_track_notifier_node *node);
	/*
	 * It is called when memory slot is being moved or removed
	 * users can drop write-protection for the pages in that memory slot
	 *
	 * @kvm: the kvm where memory slot being moved or removed
	 * @slot: the memory slot being moved or removed
	 * @node: this node
	 */
	void (*track_flush_slot)(struct kvm *kvm, struct kvm_memory_slot *slot,
			    struct kvm_page_track_notifier_node *node);
};
```

#### track_write




1. 注册函数

kvm_mmu_pte_write : 
调用 for_each_gfn_indirect_valid_sp 获取 gpa 对应的 spte(kvm_mmu_page)，
然后，mmu_pte_write_new_pte, 最后
```c
	vcpu->arch.mmu->update_pte(vcpu, sp, spte, new);

static void FNAME(update_pte)(struct kvm_vcpu *vcpu, struct kvm_mmu_page *sp,
			      u64 *spte, const void *pte)
{
	pt_element_t gpte = *(const pt_element_t *)pte;

	FNAME(prefetch_gpte)(vcpu, sp, spte, gpte, false);
}
```

- [ ] 似乎只是完成对于 guest page table 的模拟，但是 shadow page table 没有被同步




从 FNAME(fetch) 看，for_each_gfn_indirect_valid_sp 给定的 gpa 是 guest page table 的 gpa，而不是普通页面

2. 调用位置

push
=> segmented_write 
=> emulator_write_emulated : 
```c
static const struct read_write_emulator_ops write_emultor = {
	.read_write_emulate = write_emulate,
	.read_write_mmio = write_mmio,
	.read_write_exit_mmio = write_exit_mmio,
	.write = true,
};
```
完全的迷茫啊!


#### https://lwn.net/Articles/266320/
Guest systems maintain their own page tables, but they are not the tables used by the memory management unit. Instead, whenever the guest makes a change to its tables, the host system intercepts the operation, checks it for validity, then mirrors the change in the real page tables, which "shadow" those maintained by the guest.
> - 物理上的是 shadow page table，guest 无法访问 shadow page table，否则存在安全问题

In particular, if the host system decides that it wants to push a given page out to swap, it can't tell the guest that the page is no longer resident. 
> 通过 notifier 告诉他的物理地址被换出了

#### https://lwn.net/Articles/732952/


#### https://www.linux-kvm.org/images/3/33/KvmForum2008%24kdf2008_15.pdf
> mmu notifier 解决 swap 的问题 ?

## write_flooding_count

> write_flooding_count:
>    A guest may write to a page table many times, causing a lot of
>    emulations if the page needs to be write-protected (see "Synchronized
>    and unsynchronized pages" below).  Leaf pages can be unsynchronized
>    so that they do not trigger frequent emulation, but this is not
>    possible for non-leafs.  This field counts the number of emulations
>    since the last time the page table was actually used; if emulation
>    is triggered too frequently on this page, KVM will unmap the page
>    to avoid emulation in the future.

- [ ] 为什么 leaf page 可以 unsynchronized ?
- [ ] avoid emulation, 但是实际上存在作用怎么办


## mmu_set_spte

pte_access :
1. `__direct_map` : 使用参数 ACC_ALL
2. 其他都是按照 gpte

mmu_set_spte 的参数 spte 是通过 shadow page walk 获取的，
也就是，无论是是什么页面(普通，或者 page table)，
spte 决定该页面是被保护的。注意，guest 想要修改 page table 的时候，还是要经过 shadow page walk，
因此就会被识别出来。

## set_spte

当 pte 存在 ACC_WRITE_MASK 的时候，调用
mmu_need_write_protect 检查，如果 page 被 tracked，那么设置为 RO，否认 unsync



```c
		/*
		 * Optimization: for pte sync, if spte was writable the hash
		 * lookup is unnecessary (and expensive). Write protection
		 * is responsibility of mmu_get_page / kvm_sync_page.
		 * Same reasoning can be applied to dirty page accounting.
		 */
```

## gfn
在 FNAME(fetch) 中间，出现了将 base_pfn 作为 pfn 传递的情况，进而
kvm_mmu_get_page 下面调用链的都是 发生 #PF 的 guest host page table


shadow 和 guest 的 hash 添加映射关系，是因为创建的 shadow page table 总是存在关联的 guest page table 的
此时 gfn 是 page table 地址，在 FNAME(fetch) 上。

FNAME(fetch):
```c
		if (!is_shadow_present_pte(*it.sptep)) {
      // 虽然不知道为什么 -2, 但是可以确定
      // 就是为了将 table 勾连起来
			table_gfn = gw->table_gfn[it.level - 2];
			sp = kvm_mmu_get_page(vcpu, table_gfn, addr, it.level-1,
					      false, access);
		}
```





```diff
 [PATCH] KVM: MMU: Shadow page table caching

 Define a hashtable for caching shadow page tables. Look up the cache on
 context switch (cr3 change) or during page faults.

 The key to the cache is a combination of
 - the guest page table frame number
 - the number of paging levels in the guest
    * we can cache real mode, 32-bit mode, pae, and long mode page
      tables simultaneously.  this is useful for smp bootup.
 - the guest page table table
    * some kernels use a page as both a page table and a page directory.  this
      allows multiple shadow pages to exist for that page, one per level
 - the "quadrant"
    * 32-bit mode page tables span 4MB, whereas a shadow page table spans
      2MB.  similarly, a 32-bit page directory spans 4GB, while a shadow
      page directory spans 1GB.  the quadrant allows caching up to 4 shadow page
      tables for one guest page in one level.
 - a "metaphysical" bit
    * for real mode, and for pse pages, there is no guest page table, so set
      the bit to avoid write protecting the page.

```




## direct_pte_prefetch
- [ ] 只有 direct 才有 prefetch
- [ ] 应该是将连续的 GPA 装配上 ept table 吧

## fast page fault

## https://events.static.linuxfound.org/sites/events/files/slides/Guangrong-fast-write-protection.pdf

## FNAME(fetch)
- [ ] 为什么需要进行两次 while 循环进行 shadow_walk
  - [ ] 调用 FNAME(fetch) 之前，FNAME(walk_addr) 进行的循环是走到最下面一个级别的，所以，实际上只是单独处理最后一个级别的内容吧!

## Generation 
> As mentioned in "Reaction to events" above, kvm will cache MMIO
information in leaf sptes.

- [ ] cache 什么 MMIO information

> Since only 19 bits are used to store generation-number on mmio spte, all
pages are zapped when there is an overflow.

- [ ] 找到对应的 zap 代码

- [ ] 看不懂的 lock 机制


```c
static inline bool vcpu_match_mmio_gen(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.mmio_gen == kvm_memslots(vcpu->kvm)->generation;
}
```
> TODO mmu.rst 中间算是很清楚的吧，检查一下.

## dirty
kvm_mmu_slot_set_dirty : 将 slot 对应 spte 全部设置为 dirty

```c
static void vmx_slot_disable_log_dirty(struct kvm *kvm,
				       struct kvm_memory_slot *slot)
{
	kvm_mmu_slot_set_dirty(kvm, slot);
}


static void vmx_slot_enable_log_dirty(struct kvm *kvm,
				     struct kvm_memory_slot *slot)
{
	if (!kvm_dirty_log_manual_protect_and_init_set(kvm))
		kvm_mmu_slot_leaf_clear_dirty(kvm, slot);
  // TODO largepage 并不知道指的是什么东西
  // 但是本函数只是简单的 对于 memslot 调用 slot_rmap_write_protect
	kvm_mmu_slot_largepage_remove_write_access(kvm, slot);
}
```
这时候的想法是没有问题的 : 通过将所有的 page 全部 write protection, 从而可以知道
那些是 dirty 的

## spte_write_protect

```c
/*
 * Write-protect on the specified @sptep, @pt_protect indicates whether
 * spte write-protection is caused by protecting shadow page table.
 *
 * Note: write protection is difference between dirty logging and spte
 * protection:
 * - for dirty logging, the spte can be set to writable at anytime if
 *   its dirty bitmap is properly set.
 * - for spte protection, the spte can be writable only after unsync-ing
 *   shadow page.
 *
 * Return true if tlb need be flushed.
 */
static bool spte_write_protect(u64 *sptep, bool pt_protect)
{
	u64 spte = *sptep;

	if (!is_writable_pte(spte) &&
	      !(pt_protect && spte_can_locklessly_be_made_writable(spte)))
		return false;

	rmap_printk("rmap_write_protect: spte %p %llx\n", sptep, *sptep);

	if (pt_protect)
		spte &= ~SPTE_MMU_WRITEABLE;
	spte = spte & ~PT_WRITABLE_MASK;

	return mmu_spte_update(sptep, spte);
}
```
- [x] dirty logging 和 spte protection 如何实现区分
  - [ ] pt_protect 参数就是用于区分到底谁才是 pt_protect
- [ ] 



```c
/*
 * Currently, we have two sorts of write-protection, a) the first one
 * write-protects guest page to sync the guest modification, b) another one is
 * used to sync dirty bitmap when we do KVM_GET_DIRTY_LOG. The differences
 * between these two sorts are:
 * 1) the first case clears SPTE_MMU_WRITEABLE bit.
 * 2) the first case requires flushing tlb immediately avoiding corrupting
 *    shadow page table between all vcpus so it should be in the protection of
 *    mmu-lock. And the another case does not need to flush tlb until returning
 *    the dirty bitmap to userspace since it only write-protects the page
 *    logged in the bitmap, that means the page in the dirty bitmap is not
 *    missed, so it can flush tlb out of mmu-lock.
 *
 * So, there is the problem: the first case can meet the corrupted tlb caused
 * by another case which write-protects pages but without flush tlb
 * immediately. In order to making the first case be aware this problem we let
 * it flush tlb if we try to write-protect a spte whose SPTE_MMU_WRITEABLE bit
 * is set, it works since another case never touches SPTE_MMU_WRITEABLE bit.
 *
 * Anyway, whenever a spte is updated (only permission and status bits are
 * changed) we need to check whether the spte with SPTE_MMU_WRITEABLE becomes
 * readonly, if that happens, we need to flush tlb. Fortunately,
 * mmu_spte_update() has already handled it perfectly.
 *
 * The rules to use SPTE_MMU_WRITEABLE and PT_WRITABLE_MASK:
 * - if we want to see if it has writable tlb entry or if the spte can be
 *   writable on the mmu mapping, check SPTE_MMU_WRITEABLE, this is the most
 *   case, otherwise
 * - if we fix page fault on the spte or do write-protection by dirty logging,
 *   check PT_WRITABLE_MASK.
 *
 * TODO: introduce APIs to split these two cases.
 */
static inline int is_writable_pte(unsigned long pte)
{
	return pte & PT_WRITABLE_MASK;
}
```

- [ ] 看不懂的注释，两个 flags 还是不知道如何使用


## PT_WRITABLE_MASK
```c
static bool is_dirty_spte(u64 spte)
{
  // 存在一些 ad bit 的奇怪东西
  // 但是没有 ad bit 的支持，似乎 dirty log 是没有办法进行的
	u64 dirty_mask = spte_shadow_dirty_mask(spte);
	return dirty_mask ? spte & dirty_mask : spte & PT_WRITABLE_MASK;
}
```

- [ ] 找到传输 dirty map 的内容

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

  // 核心函数
	r = kvm_get_dirty_log_protect(kvm, log);

	mutex_unlock(&kvm->slots_lock);
	return r;
}
```

```c
/**
 * kvm_get_dirty_log_protect - get a snapshot of dirty pages
 *	and reenable dirty page tracking for the corresponding pages.
 * @kvm:	pointer to kvm instance
 * @log:	slot id and address to which we copy the log
 *
 * We need to keep it in mind that VCPU threads can write to the bitmap
 * concurrently. So, to avoid losing track of dirty pages we keep the
 * following order:
 *
 *    1. Take a snapshot of the bit and clear it if needed.
 *    2. Write protect the corresponding page.
 *    3. Copy the snapshot to the userspace.
 *    4. Upon return caller flushes TLB's if needed.
 *
 * Between 2 and 4, the guest may write to the page using the remaining TLB
 * entry.  This is not a problem because the page is reported dirty using
 * the snapshot taken before and step 4 ensures that writes done after
 * exiting to userspace will be logged for the next call.
 *
 */
 // TODO 上面的注释
static int kvm_get_dirty_log_protect(struct kvm *kvm, struct kvm_dirty_log *log)

	dirty_bitmap = memslot->dirty_bitmap;

	kvm_arch_sync_dirty_log(kvm, memslot);

	n = kvm_dirty_bitmap_bytes(memslot);
	flush = false;
	if (kvm->manual_dirty_log_protect) { // TODO 这一行的 git message
		/*
		 * Unlike kvm_get_dirty_log, we always return false in *flush,
		 * because no flush is needed until KVM_CLEAR_DIRTY_LOG.  There
		 * is some code duplication between this function and
		 * kvm_get_dirty_log, but hopefully all architecture
		 * transition to kvm_get_dirty_log_protect and kvm_get_dirty_log
		 * can be eliminated.
		 */
		dirty_bitmap_buffer = dirty_bitmap;
```

```c
void kvm_arch_sync_dirty_log(struct kvm *kvm, struct kvm_memory_slot *memslot)
{
	/*
	 * Flush potentially hardware-cached dirty pages to dirty_bitmap.
	 */
	if (kvm_x86_ops.flush_log_dirty)
		kvm_x86_ops.flush_log_dirty(kvm);
}
```

==> 实际上注册的函数
```c
static void vmx_flush_log_dirty(struct kvm *kvm)
{
	kvm_flush_pml_buffers(kvm);
}
```

- [ ] PML buffer 是什么 ?
  - [ ] PMP buffer 为什么在 VMEXITs 的时候被刷新出去了
```c
/*
 * Flush all vcpus' PML buffer and update logged GPAs to dirty_bitmap.
 * Called before reporting dirty_bitmap to userspace.
 */
static void kvm_flush_pml_buffers(struct kvm *kvm)
{
	int i;
	struct kvm_vcpu *vcpu;
	/*
	 * We only need to kick vcpu out of guest mode here, as PML buffer
	 * is flushed at beginning of all VMEXITs, and it's obvious that only
	 * vcpus running in guest are possible to have unflushed GPAs in PML
	 * buffer.
	 */
	kvm_for_each_vcpu(i, vcpu, kvm)
		kvm_vcpu_kick(vcpu);
}
```


## ad bit


## PFERR_WRITE_MASK
1. page_fault_can_be_fast 自己就是 error_code 的判断，其中包含的注释:

```c
	/*
	 * #PF can be fast if:
	 * 1. The shadow page table entry is not present, which could mean that
	 *    the fault is potentially caused by access tracking (if enabled).
	 * 2. The shadow page table entry is present and the fault
	 *    is caused by write-protect, that means we just need change the W
	 *    bit of the spte which can be done out of mmu-lock.
	 *
	 * However, if access tracking is disabled we know that a non-present
	 * page must be a genuine page fault where we have to create a new SPTE.
	 * So, if access tracking is disabled, we return true only for write
	 * accesses to a present page.
	 */
	return shadow_acc_track_mask != 0 ||
	       ((error_code & (PFERR_WRITE_MASK | PFERR_PRESENT_MASK))
		== (PFERR_WRITE_MASK | PFERR_PRESENT_MASK));
```
- [ ] spte 不存在是因为 access tracking 机制 ?
- [ ] write-protect 只需要修改一下 W bit of  spte ?
- [ ] if access tracking is disabled we know that a non-present page must be a genuine page fault

```c
/*
 * SPTEs used by MMUs without A/D bits are marked with SPTE_AD_DISABLED_MASK;
 * shadow_acc_track_mask is the set of bits to be cleared in non-accessed
 * pages.
 */
static u64 __read_mostly shadow_acc_track_mask;
```

- [ ] TODO: 查看这一行的 commit messsage, TODO: 调查 ept A bits 是什么
- [ ] acc-track PTE

## acc_track_mask
- [ ] 阅读一下 fast_page_fault 中的代码，似乎被 track 的代码会去掉 flags, 而 fast page fault 只是 restore 上 flags 而已
  - [ ]


```c
/* Restore an acc-track PTE back to a regular PTE */
static u64 restore_acc_track_spte(u64 spte)
{
	u64 new_spte = spte;
	u64 saved_bits = (spte >> shadow_acc_track_saved_bits_shift)
			 & shadow_acc_track_saved_bits_mask;

	WARN_ON_ONCE(spte_ad_enabled(spte));
	WARN_ON_ONCE(!is_access_track_spte(spte));

	new_spte &= ~shadow_acc_track_mask;
	new_spte &= ~(shadow_acc_track_saved_bits_mask <<
		      shadow_acc_track_saved_bits_shift);
	new_spte |= saved_bits;

	return new_spte;
}
```


## lockless
- [ ] SPTE_HOST_WRITEABLE 和 SPTE_MMU_WRITEABLE


```c
static bool spte_can_locklessly_be_made_writable(u64 spte)
{
	return (spte & (SPTE_HOST_WRITEABLE | SPTE_MMU_WRITEABLE)) ==
		(SPTE_HOST_WRITEABLE | SPTE_MMU_WRITEABLE);
}
```

## ad bit
https://stackoverflow.com/questions/32093036/setting-of-intel-ept-accessed-and-dirty-flags-for-guest-page-tables

https://stackoverflow.com/questions/55589131/what-is-the-relation-between-ept-pte-and-host-pte-entry
> 当访问的时候，host pte 和 ept 都会设置 access / dirty bit (前提是 enable 了)

- [ ] 那么 shadow page table 的 ad bit 是如何处理的

#### ad bit manual
IA32_VMX_EPT_VPID_CAP

When accessed and dirty flags for EPT are enabled, software can track writes to guest-physical addresses using a 
feature called page-modification logging.

*Software can enable page-modification logging by setting the “enable PML” VM-execution control (see Table 24-7 
in Section 24.6.2).*

**If the processor updated a dirty flag for EPT (changing it from 0 to 1), it then operates 
as follows:**
> 当 update a dirty flag for EPT 的自动将地址写入到 PML 中间



## page fault
handle_exception_nmi => kvm_handle_page_fault

handle_ept_violation / handle_ept_misconfig / kvm_handle_page_fault
=> kvm_mmu_page_fault
=> kvm_mmu_do_page_fault


## ept misconfig

## PML white paper
page modification logging

https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/page-modification-logging-vmm-white-paper.pdf

> extended page-table mechanism (EPT) by allowing that software to
monitor the guest-physical pages modified during guest virtual machine (VM) execution more efficiently.

- [ ] 在没有翻译的时候，ad bit 在什么位置 ?

> This feature(AD bit) enables VMMs
to implement memory-management and page-classification algorithms efficiently so
as to optimize VM memory operations such as defragmentation, paging, etc. Without
accessed and dirty flags, *VMMs may need to emulate them (by marking EPT pagingstructures as not-present or read-only)
and incur the overhead of the resulting EPT violations: VM exits and associated software processing.*

- [ ] 为什么通过 ept 的 read only 可以 intercept
  - [ ] read only 不可以吧，访问没有遇到任何阻碍，还是对于 EPT pagingstructures 存在疑惑
- [ ] 通过 intercept 可以实现什么东西

> For some usage models, VMMs may benefit from additional support to monitor the
working set size. As accessed and dirty flags for EPT are set without invoking the
VMM, there is no opportunity for the VMM to accumulate working-set statistics during
operation. To calculate such statistics the VMM must scan the EPT paging structures
to aggregate the information from the accessed and dirty flags. Such scans impose
both latency and bandwidth costs that may be unacceptable in some circumstances.

- [ ] 获取 working set 真的需要对于 ept 的 dirty bit 进行访问吗 ?
  - [ ] 不就是统计 vm 的内存使用量吗 ?

PML builds upon the processor’s support for accessed and dirty flags for EPT,
extending the processing that occurs when dirty flags for EPT are set. *When PML is
active each write that sets a dirty flag for EPT also generates an entry in an inmemory log, reporting the guest-physical address of the write (aligned to 4 KBytes).
When the log is full, a VM exit occurs, notifying the VMM.*


#### PML code

```c
static int handle_pml_full(struct kvm_vcpu *vcpu)
{
	unsigned long exit_qualification;

	trace_kvm_pml_full(vcpu->vcpu_id);

	exit_qualification = vmx_get_exit_qual(vcpu);

	/*
	 * PML buffer FULL happened while executing iret from NMI,
	 * "blocked by NMI" bit has to be set before next VM entry.
	 */
	if (!(to_vmx(vcpu)->idt_vectoring_info & VECTORING_INFO_VALID_MASK) &&
			enable_vnmi &&
			(exit_qualification & INTR_INFO_UNBLOCK_NMI))
		vmcs_set_bits(GUEST_INTERRUPTIBILITY_INFO,
				GUEST_INTR_STATE_NMI);

	/*
	 * PML buffer already flushed at beginning of VMEXIT. Nothing to do
	 * here.., and there's no userspace involvement needed for PML.
	 */
	return 1;
}
```
- [ ] PML buffer 在 VMEXIT 的时候被 flush 到哪里去了
- [ ] white paper 中间提到 exit_qualification 和 vmcs_set_bits 的原因

## hugepage
- [ ] 如果 guest 使用 huge page, ept 可以感知到吗 ?

- [ ] 如果 guest 使用 shadow page table, 并且使用 huge page tabel, 如何办 ?
  - [ ] 直接禁止使用
  - [ ] host 建立对应的 huge page table
    - 如果 guest 建议 huge page table，但是 host 根本没有这么多空间，如何 ?
 
- [ ] 不考虑 kvm 虚拟化，为了支持 THP 需要实现什么东西 ?
  - TLB 设置标志位，告诉一个一个 2M 的虚拟地址空间都是可以翻译
  - page walk 机制，当检测到是 page entry THP 的时候，可以得到物理地址了

使用 THP 的时候，Guest 的 page walk 可以少走一次，可以直接导致 ept 的 page walk 可以少走一次
Host 使用 page walk，可以让 ept 少走一次

#### kvm_mmu_hugepage_adjust

kvm_mmu_hugepage_adjust 的两个调用位置:
- `__direct_map`
- `FNAME(fetch)`

用于计算 host 的 page walk 的 level，实际上，前提是 host 是 huge page，
否则一般返回 PG_LEVEL_4K，但是实际上，







给定 max_level 以及 gfn, pfn，来判断，最多可以 level up 到何种大小的 huge

```c
/*
 * Return the pointer to the large page information for a given gfn,
 * handling slots that are not large page aligned.
 */
static struct kvm_lpage_info *lpage_info_slot(gfn_t gfn,
					      struct kvm_memory_slot *slot,
					      int level)
{
	unsigned long idx;

	idx = gfn_to_index(gfn, slot->base_gfn, level);
	return &slot->arch.lpage_info[level - 2][idx];
}
```
- [ ] kvm_lpage_info 的 level - 2 的原因是什么



在 kvm_alloc_memslot_metadata 中间，初始化 rmap 和 lpage_info 的信息
- 实际上，对于每一个 page 都会初始化一个 lpage
- 对其，等因素，会让其 disallow_lpage






```diff
 KVM: MMU: rename has_wrprotected_page to mmu_gfn_lpage_is_disallowed

 kvm_lpage_info->write_count is used to detect if the large page mapping
 for the gfn on the specified level is allowed, rename it to disallow_lpage
 to reflect its purpose, also we rename has_wrprotected_page() to
 mmu_gfn_lpage_is_disallowed() to make the code more clearer

 Later we will extend this mechanism for page tracking: if the gfn is
 tracked then large mapping for that gfn on any level is not allowed.
 The new name is more straightforward
```



## pat
https://www.kernel.org/doc/Documentation/x86/pat.txt


#### disallowed_hugepage_adjust 
- 可以查看该函数所在 git blame，就是对于 host huge page 的再次检查，
如果不可以，那么需要 break up 成为普通 page


```c
static void disallowed_hugepage_adjust(struct kvm_shadow_walk_iterator it,
				       gfn_t gfn, kvm_pfn_t *pfnp, int *levelp)
{
	int level = *levelp;
	u64 spte = *it.sptep;

	if (it.level == level && level > PG_LEVEL_4K &&
	    is_nx_huge_page_enabled() &&
	    is_shadow_present_pte(spte) &&
	    !is_large_pte(spte)) {
		/*
		 * A small SPTE exists for this pfn, but FNAME(fetch)
		 * and __direct_map would like to create a large PTE
		 * instead: just force them to go down another level,
		 * patching back for them into pfn the next 9 bits of
		 * the address.
		 */
		u64 page_mask = KVM_PAGES_PER_HPAGE(level) - KVM_PAGES_PER_HPAGE(level - 1);
		*pfnp |= gfn & page_mask;
		(*levelp)--;
	}
}
```



## ZONE_DEVICE

```c
bool kvm_is_reserved_pfn(kvm_pfn_t pfn)
{
	/*
	 * ZONE_DEVICE pages currently set PG_reserved, but from a refcounting
	 * perspective they are "normal" pages, albeit with slightly different
	 * usage rules.
	 */
	if (pfn_valid(pfn))
		return PageReserved(pfn_to_page(pfn)) &&
		       !is_zero_pfn(pfn) &&
		       !kvm_is_zone_device_pfn(pfn);

	return true;
}
```


## nx huge page

```diff
History:   #0
Commit:    b8e8c8303ff28c61046a4d0f6ea99aea609a7dc0
Author:    Paolo Bonzini <pbonzini@redhat.com>
Committer: Thomas Gleixner <tglx@linutronix.de>
Date:      Mon 04 Nov 2019 11:22:02 AM UTC

kvm: mmu: ITLB_MULTIHIT mitigation

With some Intel processors, putting the same virtual address in the TLB
as both a 4 KiB and 2 MiB page can confuse the instruction fetch unit
and cause the processor to issue a machine check resulting in a CPU lockup.

Unfortunately when EPT page tables use huge pages, it is possible for a
malicious guest to cause this situation.

Add a knob to mark huge pages as non-executable. When the nx_huge_pages
parameter is enabled (and we are using EPT), all huge pages are marked as
NX. If the guest attempts to execute in one of those pages, the page is
broken down into 4K pages, which are then marked executable.

This is not an issue for shadow paging (except nested EPT), because then
the host is in control of TLB flushes and the problematic situation cannot
happen.  With nested EPT, again the nested guest can cause problems shadow
and direct EPT is treated in the same way.

[ tglx: Fixup default to auto and massage wording a bit ]

Originally-by: Junaid Shahid <junaids@google.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
Signed-off-by: Thomas Gleixner <tglx@linutronix.de>
```

#### mmu_topup_memory_caches
- 添加(topup) cache

- [ ] 什么 cache 

#### shadow page table 的 dirty bit

他们甚至使用上了 : get_user_page
