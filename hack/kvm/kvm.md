# KVM

## shadow page table 严重的干扰了视线，有必要使用 kcov 来覆盖一下

## 使用 `kvm_stat` 可以观测最核心的函数

```txt
Event                                         Total %Total CurAvg/s
kvm_entry                                    337793   15.4    26107
kvm_exit                                     337787   15.4    26107
kvm_ack_irq                                  457205   20.9    25548
kvm_emulate_insn                             192824    8.8    16726
kvm_fast_mmio                                192514    8.8    16709
kvm_apic_accept_irq                          168566    7.7    15209
kvm_apicv_accept_irq                         168559    7.7    15209
kvm_msi_set_irq                              151693    6.9    13865
kvm_eoi                                       91441    4.2     5110
kvm_hv_timer_state                            22948    1.0     1818
kvm_msr                                       18642    0.9     1495
kvm_wait_lapic_expire                         14466    0.7     1166
kvm_pv_tlb_flush                               5097    0.2      384
kvm_pic_set_irq                                4832    0.2      369
kvm_set_irq                                    4788    0.2      369
kvm_ioapic_set_irq                             4788    0.2      369
kvm_fpu                                        3718    0.2      268
kvm_vcpu_wakeup                                3178    0.1      243
kvm_userspace_exit                             1860    0.1      134
kvm_pio                                        1600    0.1      119
kvm_hypercall                                  1188    0.1       83
kvm_mmio                                        484    0.0       27
vcpu_match_mmio                                 274    0.0       15
kvm_apic                                       1524    0.1        8
kvm_pvclock_update                               13    0.0        4
kvm_halt_poll_ns                                 42    0.0        3
Total                                       2187824          167463
```
## 有趣的 patch
- a54d806688fe1e482350ce759a8a0fc9ebf814b0

从这个 patch 开始，将内核从 memory slot 从 array 修改为 rbtree 组织形式

## 过一下官方文档
https://www.kernel.org/doc/html/latest/virt/kvm/index.html

## [ ] kvm ring
https://kvmforum2020.sched.com/event/eE4R/kvm-dirty-ring-a-new-approach-to-logging-peter-xu-red-hat

顺便理解一下:
```c
static const struct vm_operations_struct kvm_vcpu_vm_ops = {
	.fault = kvm_vcpu_fault,
};
```

## TODO
- kvm_vcpu_unmap : 调用的好多位置在 nested, 是做啥的
- cpu hotplug

- kvm_arch_vcpu_ioctl_run+4975
  - kvm_mmu_load+1151
    - mmu_alloc_direct_roots
    - kvm_mmu_sync_roots+1

我认为 kvm_mmu_sync_roots 虽然被调用了，但是应该立刻开始返回。
```c
	if (vcpu->arch.mmu->root_role.direct)
		return;
```

### 整理关键的数据结构
- Each virtual CPU has an associated struct `kvm_run` data structure,
used to communicate information about the CPU between the kernel and user space.

## 整理一下路径

## TODO
1. VMPTRST 和 VMPTRLD
3. rsp_rdx
4. vmcs_config vmcs 中间的具体内容是什么用于管控什么东西
5. cpuid

MSR 来 check vmx 的能力:
setup_vmcs_config 的中间，来分析其中的作用

Before system sftware can enter VMX operation, it enables VMX by setting CR4.VMXE[bit 13] = 1
`__vmx_enable`

想不到 : vmx_init_syscall 动态添加 syscall, 可以动态的修改 vcpu 的属性.

vmcs 的格式:
IA32_VMX_BASIC :

VPID 在内核中的操作方法 ?

## 记录
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

## 函数调用路径

- kvm_arch_vcpu_ioctl_run
  - vcpu_run
    - vcpu_enter_guest
        - static_call(kvm_x86_vcpu_run)(vcpu)
          - svm_vcpu_run
            - svm_exit_handlers_fastpath
              - handle_fastpath_set_msr_irqoff
        - vmx_handle_exit
          - kvm_vmx_exit_handlers
            - `__kvm_get_msr`
              - `vmx_get_msr`

- vmx_handle_exit
  - kvm_mmu_page_fault
    - direct_page_fault
      - kvm_tdp_mmu_map

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

## intel processor tracing

- patch : https://lwn.net/Articles/741093/
- https://man7.org/linux/man-pages/man1/perf-intel-pt.1.html

#### `vmx_x86_ops`

- `struct x86_kvm_ops` : `vmx_x86_ops` 也是其中一种
- `x86_kvm_ops` : 一个经常访问的变量


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

## hyperv.c
模拟 HyperV 的内容, 但是为什么需要模拟 HyperV ?

- kvm_hv_hypercall
- stimer

实在是有点看不懂:
https://docs.microsoft.com/en-us/virtualization/hyper-v-on-windows/reference/hyper-v-architecture

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

#### kvm_vm_ioctl_set_memory_region

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


## shadow page table 的坏处
- Simplified VMM design. 需要处理 shadow page table 和两级翻译的同步问题
- Guest page table modifications need not be trapped, hence VM exits reduced. 同步
- Reduced memory footprint compared to shadow page table algorithms. shadow table 会占用空间


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

    if (kvm_hv_hypercall_enabled(vcpu->kvm))
        return kvm_hv_hypercall(vcpu);

```

## manual notes
- Table C-1. Basic Exit Reasons
