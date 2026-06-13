# tdp_mmu

- https://lwn.net/Articles/832835/
- https://static.sched.com/hosted_files/kvmforum2019/25/MMU%20improvements%20KVM%20Forum%20Presentation%20-%20short.pdf

1. spte 是需要 rmap 的?

handle_mmio_page_fault


```c
static inline struct kvm_mmu_page *spte_to_child_sp(u64 spte)
{
	return to_shadow_page(spte & SPTE_BASE_ADDR_MASK);
}
```
-  到底有多少个 kvm_mmu_page


- kvm_tdp_page_fault
  - kvm_tdp_mmu_page_fault
    - kvm_tdp_mmu_map

- handle_mmio_page_fault
  - get_mmio_spte
    - kvm_tdp_mmu_get_walk

- fast_page_fault
  - kvm_tdp_mmu_fast_pf_get_last_sptep
    - kvm_tdp_mmu_get_walk


## 问题

### [x] kvm_tdp_mmu_map 中 为什么这里也有 shadow page table，只是统一的名称

```c
#define tdp_mmu_for_each_pte(_iter, _mmu, _start, _end)		\
	for_each_tdp_pte(_iter, to_shadow_page(_mmu->root.hpa), _start, _end)

/*
 * Iterates over every SPTE mapping the GFN range [start, end) in a
 * preorder traversal.
 */
#define for_each_tdp_pte_min_level(iter, root, min_level, start, end) \
	for (tdp_iter_start(&iter, root, min_level, start); \
	     iter.valid && iter.gfn < end;		     \
	     tdp_iter_next(&iter))

#define for_each_tdp_pte(iter, root, start, end) \
	for_each_tdp_pte_min_level(iter, root, PG_LEVEL_4K, start, end)
```

1. 参数: gfn
2. `to_shadow_page(_mmu->root.hpa)` : ept page table 的根
3. 只有 ept page table 才会去调用 to_shadow_page 的 ？
```c
static inline struct kvm_mmu_page *to_shadow_page(hpa_t shadow_page)
{
	struct page *page = pfn_to_page((shadow_page) >> PAGE_SHIFT);

	return (struct kvm_mmu_page *)page_private(page);
}
```

设置时间，这是错误的，这个函数从来不会被使用

## shadow_page
```c
static void link_shadow_page(struct kvm_vcpu *vcpu, u64 *sptep,
			     struct kvm_mmu_page *sp)
{
	__link_shadow_page(vcpu->kvm, &vcpu->arch.mmu_pte_list_desc_cache, sptep, sp, true);
}
```

直接访问:
- kvm_mmu_alloc_shadow_page : 不开嵌套虚拟化的时候，不会调用
- tdp_mmu_init_sp
  - 实际上，每一个 ept page 都需要建立一个 kvm_mmu_page 的

```txt
@[
    tdp_mmu_init_sp+1
    kvm_tdp_mmu_get_vcpu_root_hpa+223
    kvm_mmu_load+1527
    kvm_arch_vcpu_ioctl_run+4482
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 54
@[
    tdp_mmu_init_sp+1
    kvm_tdp_mmu_map+296
    direct_page_fault+862
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 2192
```

## 清理
```txt
    kvm_tdp_mmu_zap_all+5
    kvm_mmu_zap_all+224
    kvm_mmu_notifier_release+47
    __mmu_notifier_release+116
    exit_mmap+656
    __mmput+67
    do_exit+723
    do_group_exit+49
    get_signal+2390
    arch_do_signal_or_restart+46
    exit_to_user_mode_prepare+267
    syscall_exit_to_user_mode+27
    do_syscall_64+76
    entry_SYSCALL_64_after_hwframe+114
```
- [ ] 非常奇怪，当 L2 结束的时候，没有跟踪到这个

## kvm_tdp_mmu_write_protect_gfn
- 没人调用，我谢天谢地哦

## 似乎操作 spte 比想想的复杂

* set_spte_gfn
* `__tdp_mmu_set_spte`
* tdp_mmu_set_spte_atomic
  - `__handle_changed_spte` : handle bookkeeping associated with an SPTE change
    - 处理 dirty 了之类的情况
    - handle_removed_pt : 专门来处理移除 page 的情况

```txt
@[
    __handle_changed_spte+1
    kvm_tdp_mmu_map+829
    direct_page_fault+862
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 447750
@[
    __handle_changed_spte+1
    __handle_changed_spte+905
    __handle_changed_spte+905
    __tdp_mmu_zap_root+238
    tdp_mmu_zap_root_work+65
    process_one_work+482
    worker_thread+84
    kthread+233
    ret_from_fork+41
]: 614912
```

## 此外，关于 zero page ref 的问题，为什么会走这个路径

## 理解 kvm_mmu_page_role


## 完全无法理解 kvm_tdp_page_fault 中下面这部分
```c
#ifdef CONFIG_X86_64
	if (tdp_mmu_enabled)
		return kvm_tdp_mmu_page_fault(vcpu, fault);
#endif

	return direct_page_fault(vcpu, fault);
```
https://lore.kernel.org/lkml/20220726165748.76db5284@rosa.proxmox.com/T/#mbffa2249164668617811ca1cb301ef7c6232ded9

似乎 tdp_mmu_enabled 5.10 现在才打开，4.19 直接没有这个选项。
难道 4.19 的代码中走的 shadow page table 吗? 还是说使用硬件加强的 shaow page table ?

其实  `kvm_tdp_mmu_page_fault` 和  `direct_page_fault` 的差别很小:
1. `direct_page_fault` 存在
```txt
	/* Dummy roots are used only for shadowing bad guest roots. */
	if (WARN_ON_ONCE(kvm_mmu_is_dummy_root(vcpu->arch.mmu->root.hpa)))
		return RET_PF_RETRY;
```
2. lock 修改为 write -> read (TODO ????)

```diff
- r = direct_map(vcpu, fault);
+ r = kvm_tdp_mmu_map(vcpu, fault);
```

等下，是不是 direct

### 关键在于 role.direct 是否为 true

```c
int kvm_mmu_load(struct kvm_vcpu *vcpu)
{
  // ...
	if (vcpu->arch.mmu->root_role.direct)
		r = mmu_alloc_direct_roots(vcpu);
	else
		r = mmu_alloc_shadow_roots(vcpu);
  // ...
}
```

当前 role.direct 都是

#### npt=N 的时候，这个是 direct 为 0

居然一时有点难以验证


## 测试下四种默认的结果吧

测试验证 tdp_mmu_enabled 的效果：
```txt
sudo rmmod kvm_intel
sudo rmmod kvm
sudo modprobe kvm tdp_mmu=N
sudo modprobe kvm_intel ept=N
```

## 原始 patch
https://lore.kernel.org/all/20201009105924.GF1042@kadam/T/

### 看看报告吧
- https://www.youtube.com/watch?v=iQwO2PudbNY&t=1095s


Exploring an architecture-neutral MMU

- https://kvm-forum.qemu.org/2022/KVM%20Forum%202022_%20Exploring%20an%20architecture-neutral%20MMU.pdf

## tdp_enabled 和 tdp_mmu_enabled 的区别
<!-- b6faa70c-e250-48d2-9dcc-9a18263d1a0e -->

tdp_enabled 是硬件机制
tdp_mmu_enabled 是 lock 机制改进

打开关闭的方法分别是:
```sh
sudo rmmod kvm_intel
sudo rmmod kvm
sudo modprobe kvm tdp_mmu=N
sudo modprobe kvm_intel ept=N
```

这个地方的命名最后都是统一的，也就是 tdp_mmu 就是锁机制了。

```c
/*
 * When setting this variable to true it enables Two-Dimensional-Paging
 * where the hardware walks 2 page tables:
 * 1. the guest-virtual to guest-physical
 * 2. while doing 1. it walks guest-physical to host-physical
 * If the hardware supports that we don't need to do shadow paging.
 */
bool tdp_enabled = false;

static bool __ro_after_init tdp_mmu_allowed;

#ifdef CONFIG_X86_64
bool __read_mostly tdp_mmu_enabled = true;
module_param_named(tdp_mmu, tdp_mmu_enabled, bool, 0444);
EXPORT_SYMBOL_FOR_KVM_INTERNAL(tdp_mmu_enabled);
#endif
```

tdp_mmu_enabled 的区别体现:
```c
int kvm_tdp_page_fault(struct kvm_vcpu *vcpu, struct kvm_page_fault *fault)
{
#ifdef CONFIG_X86_64
	if (tdp_mmu_enabled)
		return kvm_tdp_mmu_page_fault(vcpu, fault);
#endif

	return direct_page_fault(vcpu, fault);
}
```

tdp_enabled 的区别体现:
```c
void kvm_init_mmu(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu_role_regs regs = vcpu_to_role_regs(vcpu);
	union kvm_cpu_role cpu_role = kvm_calc_cpu_role(vcpu, &regs);

	if (mmu_is_nested(vcpu))
		init_kvm_nested_mmu(vcpu, cpu_role);
	else if (tdp_enabled)
		init_kvm_tdp_mmu(vcpu, cpu_role); // 不过这里为什么不去进一步的做区分?
	else
		init_kvm_softmmu(vcpu, cpu_role);
}
```

或者说的再清楚点:
```c
/*
 * When setting this variable to true it enables Two-Dimensional-Paging
 * where the hardware walks 2 page tables:
 * 1. the guest-virtual to guest-physical
 * 2. while doing 1. it walks guest-physical to host-physical
 * If the hardware supports that we don't need to do shadow paging.
 */
bool tdp_enabled = false;

static bool __ro_after_init tdp_mmu_allowed;

#ifdef CONFIG_X86_64
bool __read_mostly tdp_mmu_enabled = true;
module_param_named(tdp_mmu, tdp_mmu_enabled, bool, 0444);
#endif
```

注意，再看几个经典例子:
```c
void kvm_init_mmu(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu_role_regs regs = vcpu_to_role_regs(vcpu);
	union kvm_cpu_role cpu_role = kvm_calc_cpu_role(vcpu, &regs);

	if (mmu_is_nested(vcpu))
		init_kvm_nested_mmu(vcpu, cpu_role);
	else if (tdp_enabled)
		init_kvm_tdp_mmu(vcpu, cpu_role);
	else
		init_kvm_softmmu(vcpu, cpu_role);
}
EXPORT_SYMBOL_GPL(kvm_init_mmu);
```

```c
int kvm_tdp_page_fault(struct kvm_vcpu *vcpu, struct kvm_page_fault *fault)
{
#ifdef CONFIG_X86_64
	if (tdp_mmu_enabled)
		return kvm_tdp_mmu_page_fault(vcpu, fault);
#endif

	return direct_page_fault(vcpu, fault);
}
```

## 4.19 内核启动嵌套
整个机器的全部都是红色的，可以看到大部分的时间都是卡在
native_queued_spin_lock_slowpath ，其

perf record -a -g
```txt
  Children      Self  Command          Shared Object                    Symbol
+   91.33%     0.00%  qemu-system-x86  libc.so.6                        [.] __GI___ioctl
+   89.90%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] entry_SYSCALL_64_after_hwframe
+   89.90%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] do_syscall_64
+   89.78%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] __x64_sys_ioctl
+   89.78%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] ksys_ioctl
+   89.78%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] do_vfs_ioctl
+   89.77%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_vcpu_ioctl
+   89.77%     0.06%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_arch_vcpu_ioctl_run
+   89.04%     0.21%  qemu-system-x86  [kernel.kallsyms]                [k] vcpu_enter_guest
+   86.53%     0.36%  qemu-system-x86  [kernel.kallsyms]                [k] _raw_spin_lock
+   86.17%    86.17%  qemu-system-x86  [kernel.kallsyms]                [k] native_queued_spin_lock_slowpath
+   43.70%     0.03%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_mmu_load
+   41.97%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_mmu_unload
+   41.96%     0.02%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_mmu_free_roots
+   41.70%     0.02%  qemu-system-x86  [kernel.kallsyms]                [k] nested_svm_vmexit
+   41.49%     0.04%  qemu-system-x86  [kernel.kallsyms]                [k] handle_exit
+   22.21%     0.02%  qemu-system-x86  [kernel.kallsyms]                [k] vmrun_interception
+   21.63%     0.03%  qemu-system-x86  [kernel.kallsyms]                [k] enter_svm_guest_mode
+   20.53%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_mmu_reset_context
+    7.07%     0.00%  swapper          [kernel.kallsyms]                [k] secondary_startup_64
+    7.07%     0.00%  swapper          [kernel.kallsyms]                [k] cpu_startup_entry
+    7.07%     0.00%  swapper          [kernel.kallsyms]                [k] do_idle
+    7.03%     0.00%  swapper          [kernel.kallsyms]                [k] start_secondary
+    6.98%     0.00%  swapper          [kernel.kallsyms]                [k] cpuidle_enter_state
+    6.98%     0.00%  swapper          [kernel.kallsyms]                [k] acpi_idle_enter
+    6.97%     6.97%  swapper          [kernel.kallsyms]                [k] acpi_processor_ffh_cstate_enter
+    1.66%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_mmu_page_fault
+    1.46%     0.01%  qemu-system-x86  [kernel.kallsyms]                [k] kvm_mmu_sync_roots.part.140
+    1.21%     0.00%  qemu-system-x86  [kernel.kallsyms]                [k] tdp_page_fault
+    1.09%     0.07%  qemu-system-x86  [kernel.kallsyms]                [k] apic_timer_interrupt
+    1.03%     0.01%  qemu-system-x86  [kernel.kallsyms]                [k] smp_apic_timer_interrupt
+    0.89%     0.01%  qemu-system-x86  [kernel.kallsyms]                [k] hrtimer_interrupt
```

不知道为什么，只是运行了一个虚拟机，而且虚拟机的容量只有 20G 而已，就有问题。
真的服气啊，怪不得嵌套慢啊

如果只是启动一个虚拟机，结果为:
```txt
+   58.78%     0.01%  qemu-system-x86  [kernel.kallsyms]                                            [k] entry_SYSCALL_64_after_hwframe
+   58.77%     0.01%  qemu-system-x86  [kernel.kallsyms]                                            [k] do_syscall_64
+   57.92%     0.01%  qemu-system-x86  libc.so.6                                                    [.] __GI___ioctl
+   53.34%     0.00%  qemu-system-x86  [kernel.kallsyms]                                            [k] __x64_sys_ioctl
+   53.34%     0.00%  qemu-system-x86  [kernel.kallsyms]                                            [k] ksys_ioctl
+   53.32%     0.01%  qemu-system-x86  [kernel.kallsyms]                                            [k] do_vfs_ioctl
+   52.89%     0.00%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_vcpu_ioctl
+   52.88%     0.49%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_arch_vcpu_ioctl_run
+   44.38%     1.50%  qemu-system-x86  [kernel.kallsyms]                                            [k] vcpu_enter_guest
+   25.91%     0.08%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_mmu_load
+   25.14%     0.02%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_mmu_sync_roots.part.140
+   25.12%     0.02%  qemu-system-x86  [kernel.kallsyms]                                            [k] mmu_sync_children
+   21.93%     0.00%  qemu-system-x86  [kernel.kallsyms]                                            [k] __kvm_sync_page
+   21.93%     0.96%  qemu-system-x86  [kernel.kallsyms]                                            [k] paging64_sync_page
+   17.30%     0.00%  swapper          [kernel.kallsyms]                                            [k] secondary_startup_64
+   17.30%     0.01%  swapper          [kernel.kallsyms]                                            [k] cpu_startup_entry
+   17.29%     0.09%  swapper          [kernel.kallsyms]                                            [k] do_idle
+   17.21%     0.00%  swapper          [kernel.kallsyms]                                            [k] start_secondary
+   14.82%     0.09%  swapper          [kernel.kallsyms]                                            [k] cpuidle_enter_state
+   14.69%     0.05%  swapper          [kernel.kallsyms]                                            [k] acpi_idle_enter
+   10.59%    10.59%  swapper          [kernel.kallsyms]                                            [k] acpi_processor_ffh_cstate_enter
+    9.46%     4.19%  qemu-system-x86  [kernel.kallsyms]                                            [k] set_spte
+    9.15%     0.02%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_mmu_page_fault
+    9.03%     0.66%  qemu-system-x86  [kernel.kallsyms]                                            [k] __kvm_read_guest_atomic
+    8.64%     8.64%  qemu-system-x86  [kernel.kallsyms]                                            [k] copy_user_generic_string
+    7.43%     0.11%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_vcpu_block
+    7.05%     0.12%  qemu-system-x86  [kernel.kallsyms]                                            [k] tdp_page_fault
+    5.90%     0.03%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_flush_remote_tlbs
+    5.87%     0.01%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_make_all_cpus_request
+    5.79%     3.00%  qemu-system-x86  [kernel.kallsyms]                                            [k] kvm_make_vcpus_request_mask
+    4.95%     0.00%  qemu-system-x86  [unknown]                                                    [k] 0x0531abb0000001f3
```

```txt
        tdp_mmu_link_sp+1
        kvm_tdp_mmu_map+800
        kvm_tdp_page_fault+196
        kvm_mmu_do_page_fault+473
        kvm_mmu_page_fault+134
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
```

## tdp 的经典问题

也就是，现在其实存在三种模式?

1. shadow 模式
```
	if (vcpu->arch.mmu->root_role.direct)
		r = mmu_alloc_direct_roots(vcpu);
	else
		r = mmu_alloc_shadow_roots(vcpu);
	if (r)
		goto out;
```
2. kvm_tdp_mmu_alloc_root : tdp

3. mmu_alloc_root ? 不是 shadow ，也不是 tdp mmu


```c
static int mmu_alloc_direct_roots(struct kvm_vcpu *vcpu)
{
	struct kvm_mmu *mmu = vcpu->arch.mmu;
	u8 shadow_root_level = mmu->root_role.level;
	hpa_t root;
	unsigned i;
	int r;

	if (tdp_mmu_enabled) {
		if (kvm_has_mirrored_tdp(vcpu->kvm) &&
		    !VALID_PAGE(mmu->mirror_root_hpa))
			kvm_tdp_mmu_alloc_root(vcpu, true);
		kvm_tdp_mmu_alloc_root(vcpu, false);
		return 0;
	}
```

再看一个问题:

```txt
int kvm_mmu_load(struct kvm_vcpu *vcpu)
{
    r = mmu_topup_memory_caches(vcpu,
        !vcpu->arch.mmu->root_role.direct);  // indirect 需要更多缓存
    r = mmu_alloc_special_roots(vcpu);

    if (vcpu->arch.mmu->root_role.direct)
        r = mmu_alloc_direct_roots(vcpu);    // TDP
    else
        r = mmu_alloc_shadow_roots(vcpu);    // Shadow paging
}
```

```
┌─────────────────────────────────────────────────────────────┐
│                      KVM MMU 实现                           │
└─────────────────────────────────────────────────────────────┘
                          │
                          ├─────────────────────────────────┐
                          ▼                                 ▼
              ┌──────────────────────┐          ┌──────────────────────┐
              │  Shadow Paging       │          │  Direct Mapping      │
              │  (传统影子页表)      │          │  (TDP/EPT/NPT)       │
              │                      │          │                      │
              │  root_role.direct=0  │          │  root_role.direct=1  │
              └──────────────────────┘          └──────────────────────┘
                                                            │
                                        ┌───────────────────┴───────────────────┐
                                        ▼                                       ▼
                            ┌──────────────────────┐              ┌──────────────────────┐
                            │  旧 TDP 实现         │              │  新 TDP MMU          │
                            │  (mmu.c)             │              │  (tdp_mmu.c)         │
                            │                      │              │                      │
                            │  tdp_mmu_enabled=0   │              │  tdp_mmu_enabled=1   │
                            └──────────────────────┘              └──────────────────────┘
```


### 5.1 Shadow Paging 路径

```
kvm_mmu_load()
    └─ mmu_alloc_shadow_roots()
        ├─ 读取 Guest 页表（CR3）
        ├─ 分配影子页表页
        ├─ 建立 GVA→GPA 映射
        └─ 需要持续同步 Guest 页表变化

页面故障处理：
    └─ FNAME(page_fault)()
        ├─ 遍历 Guest 页表（GVA→GPA）
        ├─ 遍历影子页表（GVA→HPA）
        └─ 同步更新
```

### 5.2 旧 TDP 路径

```
kvm_mmu_load()
    └─ mmu_alloc_direct_roots()
        └─ tdp_mmu_enabled = false
            ├─ write_lock(&kvm->mmu_lock)  ← 写锁
            ├─ mmu_alloc_root()
            └─ write_unlock(&kvm->mmu_lock)

页面故障处理：
    └─ direct_page_fault()
        └─ direct_map()
            ├─ 使用 for_each_shadow_entry 遍历
            ├─ 使用 RMAP 管理反向映射
            └─ 写锁保护
```
这里为什么要用 rmap ? 看看这个锁在哪里的

### 5.3 新 TDP MMU 路径

```
kvm_mmu_load()
    └─ mmu_alloc_direct_roots()
        └─ tdp_mmu_enabled = true
            └─ kvm_tdp_mmu_alloc_root()  ← 无需写锁
                ├─ 分配根页表
                ├─ refcount = 2
                └─ 添加到 tdp_mmu_roots 链表

页面故障处理：
    └─ kvm_tdp_mmu_page_fault()
        └─ kvm_tdp_mmu_map()
            ├─ rcu_read_lock()  ← RCU 保护
            ├─ for_each_tdp_pte() 遍历
            ├─ 原子操作更新 SPTE
            └─ rcu_read_unlock()
```

是这样的

### 6.1 旧 TDP 实现的问题

```
问题 1：并发性能差
    - 使用 write_lock(&kvm->mmu_lock)
    - 多个 vCPU 页面故障时串行处理
    - 在多核 VM 中性能瓶颈明显

问题 2：代码复杂
    - 与 Shadow Paging 共享大量代码
    - RMAP（反向映射）开销大
    - 维护困难

问题 3：可扩展性差
    - 无法利用现代硬件特性
    - 难以支持大页优化
```

### 6.2 新 TDP MMU 的优势

```
优势 1：并发性能好 ✅
    - 使用 read_lock(&kvm->mmu_lock)
    - RCU 保护允许多读者
    - 原子操作更新 SPTE
    - 多核性能显著提升

优势 2：代码简洁 ✅
    - 独立实现（tdp_mmu.c）
    - 不需要 RMAP（部分场景）
    - 专门为 TDP 优化

优势 3：易于优化 ✅
    - 支持大页恢复
    - 支持 eager page split
    - 易于添加新特性
```

## 这是为什么?
```txt
struct kvm {
#ifdef KVM_HAVE_MMU_RWLOCK
	rwlock_t mmu_lock;
#else
	spinlock_t mmu_lock;
#endif /* KVM_HAVE_MMU_RWLOCK */

	struct mutex slots_lock;
	// ...
}
```

## 访问 page table 的函数都是不一样的
old tdp 使用:
```c
static void __set_spte(u64 *sptep, u64 spte)
{
	KVM_MMU_WARN_ON(is_ept_ve_possible(spte));
	WRITE_ONCE(*sptep, spte);
}

static void __update_clear_spte_fast(u64 *sptep, u64 spte)
{
	KVM_MMU_WARN_ON(is_ept_ve_possible(spte));
	WRITE_ONCE(*sptep, spte);
}

static u64 __update_clear_spte_slow(u64 *sptep, u64 spte)
{
	KVM_MMU_WARN_ON(is_ept_ve_possible(spte));
	return xchg(sptep, spte);
}

static u64 __get_spte_lockless(u64 *sptep)
{
	return READ_ONCE(*sptep);
}
```

tdp 的使用:
```txt
@[
    xueshi+70
    xueshi+70
    tdp_iter_next+160
    kvm_tdp_mmu_map+343
    kvm_tdp_page_fault+225
    kvm_mmu_do_page_fault+372
    kvm_mmu_page_fault+209
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 7753
```

真正使用的都是在这个地方，
```c
static inline u64 kvm_tdp_mmu_read_spte(tdp_ptep_t sptep)
{
	return READ_ONCE(*rcu_dereference(sptep));
}

static inline u64 kvm_tdp_mmu_write_spte_atomic(tdp_ptep_t sptep, u64 new_spte)
{
	KVM_MMU_WARN_ON(is_ept_ve_possible(new_spte));
	return xchg(rcu_dereference(sptep), new_spte);
}

static inline void __kvm_tdp_mmu_write_spte(tdp_ptep_t sptep, u64 new_spte)
{
	KVM_MMU_WARN_ON(is_ept_ve_possible(new_spte));
	WRITE_ONCE(*rcu_dereference(sptep), new_spte);
}
```

## kvm_mmu_slot_try_split_huge_pages
<!-- a7d5c9dc-69d5-4126-afdf-0fd2775425dc -->

为什么只有 tdp mmu 需要小心处理 hugepage 的 split ?
```c
void kvm_mmu_slot_try_split_huge_pages(struct kvm *kvm,
					const struct kvm_memory_slot *memslot,
					int target_level)
{
	u64 start = memslot->base_gfn;
	u64 end = start + memslot->npages;

	if (!tdp_mmu_enabled)
		return;

	if (kvm_memslots_have_rmaps(kvm)) {
		write_lock(&kvm->mmu_lock);
		kvm_shadow_mmu_try_split_huge_pages(kvm, memslot, start, end, target_level);
		write_unlock(&kvm->mmu_lock);
	}

	read_lock(&kvm->mmu_lock);
	kvm_tdp_mmu_try_split_huge_pages(kvm, memslot, start, end, target_level, true);
	read_unlock(&kvm->mmu_lock);

	/*
	 * No TLB flush is necessary here. KVM will flush TLBs after
	 * write-protecting and/or clearing dirty on the newly split SPTEs to
	 * ensure that guest writes are reflected in the dirty log before the
	 * ioctl to enable dirty logging on this memslot completes. Since the
	 * split SPTEs retain the write and dirty bits of the huge SPTE, it is
	 * safe for KVM to decide if a TLB flush is necessary based on the split
	 * SPTEs.
	 */
}
```

## 当 ept=1 的时候，

### tdp_mmu_enabled=0

将 tdp_mmu_enabled=0 ，可以得到:
```txt
@[
    mmu_spte_update_no_track+306
    mmu_spte_update_no_track+306
    mmu_spte_update+22
    mmu_set_spte+441
    direct_map+989
    direct_page_fault+252
    kvm_mmu_do_page_fault+372
    kvm_mmu_page_fault+209
    vmx_handle_exit+1273
    kvm_arch_vcpu_ioctl_run+6688
    kvm_vcpu_ioctl+1565
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 1628
```

### tdp_mmu_enabled=1

才有这个
```txt
sudo bpftrace -e "rawtracepoint:kvm_tdp_mmu_spte_changed { @[kstack] = count(); }"
```

```txt
@[
    handle_changed_spte+1140
    __tdp_mmu_zap_root+431
    tdp_mmu_zap_root+86
    kvm_tdp_mmu_zap_invalidated_roots+58
    kvm_set_memslot+323
    kvm_vm_ioctl_set_memory_region+65
    kvm_vm_ioctl+679
    __se_sys_ioctl+110
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 26112
```

## 这里的 kvm_memslots_have_rmaps 和 tdp_mmu_enabled 不是互斥的

合理的
```c
bool kvm_unmap_gfn_range(struct kvm *kvm, struct kvm_gfn_range *range)
{
	bool flush = false;

	if (kvm_memslots_have_rmaps(kvm))
		flush = kvm_handle_gfn_range(kvm, range, kvm_zap_rmap);

	if (tdp_mmu_enabled)
		flush = kvm_tdp_mmu_unmap_gfn_range(kvm, range, flush);

	if (kvm_x86_ops.set_apic_access_page_addr &&
	    range->slot->id == APIC_ACCESS_PAGE_PRIVATE_MEMSLOT)
		kvm_make_all_cpus_request(kvm, KVM_REQ_APIC_PAGE_RELOAD);

	return flush;
}
```


## ept=0 之后，tdp_mmu_enabled 参数自动消失

```txt
sudo insmod arch/x86/kvm/kvm.ko tdp_mmu=1
sudo insmod arch/x86/kvm/kvm-intel.ko 'dump_invalid_vmcs=1 ept=0 enable_apicv=1 '

cat /sys/module/kvm/parameters/tdp_mmu
cat /sys/module/kvm_intel/parameters/ept
N
N
```
这是自然!

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
