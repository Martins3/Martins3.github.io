## kvm_vcpu_arch 中的 5 个 MMU 的含义
<!-- 5e8c439f-ecae-4e55-9ec0-b86f4a6dde51 -->

```c
struct kvm_vcpu_arch {
	/*
	 * Paging state of the vcpu
	 *
	 * If the vcpu runs in guest mode with two level paging this still saves
	 * the paging mode of the l1 guest. This context is always used to
	 * handle faults.
	 */
	struct kvm_mmu *mmu;

	/* Non-nested MMU for L1 */
	struct kvm_mmu root_mmu;

	/* L1 MMU when running nested */
	struct kvm_mmu guest_mmu;

	/*
	 * Paging state of an L2 guest (used for nested npt)
	 *
	 * This context will save all necessary information to walk page tables
	 * of an L2 guest. This context is only initialized for page table
	 * walking and not for faulting since we never handle l2 page faults on
	 * the host.
	 */
	struct kvm_mmu nested_mmu;

	/*
	 * Pointer to the mmu context currently used for
	 * gva_to_gpa translations.
	 */
	struct kvm_mmu *walk_mmu;
```

1. root_mmu (实例)
- 用途：L1 非嵌套模式的 MMU
- 场景：L1 直接运行，无嵌套虚拟化

2. guest_mmu (实例)
- 用途：处理 L2 的 page faults（通过 Shadow EPT）
- 场景：L1 运行 L2 nested guest
- 关键：构建 shadow EPT02，将 L2 GPA 直接映射到 L0 HPA

3. nested_mmu (实例)
- 用途：L2 GVA → L2 GPA 的地址转换
- 场景：Walk L2 的 page tables
- 关键：不处理 page faults，只用于地址转换

4. mmu (指针)
- 用途：当前活动的 MMU，指向上述三个之一
- 非嵌套：mmu = &root_mmu
- 嵌套：mmu = &guest_mmu

5. walk_mmu (指针)
- 用途：当前用于 GVA→GPA 转换的 MMU
- 非嵌套：walk_mmu = &root_mmu
- 嵌套：walk_mmu = &nested_mmu

为什么需要 guest_mmu 和 nested_mmu 两个？

关键原因：两级地址转换需要两个 MMU context

L2 GVA --[nested_mmu]--> L2 GPA --[guest_mmu]--> L0 HPA
         (walk L2 CR3)            (Shadow EPT02)

- nested_mmu：Walk L2 的页表（L2 GVA → L2 GPA）
- guest_mmu：Walk L1 的 EPT12，构建 Shadow EPT（L2 GPA → L0 HPA）

### vCPU nested 状态切换，导致 kvm_vcpu_arch 中mmu 和 walk_mmu 的切换
<!-- 3ce9855f-51a5-4e25-b6dd-7e7fa6526191 -->

```c
static void nested_ept_init_mmu_context(struct kvm_vcpu *vcpu)
{
	WARN_ON(mmu_is_nested(vcpu));

	vcpu->arch.mmu = &vcpu->arch.guest_mmu;
	nested_ept_new_eptp(vcpu);
	vcpu->arch.mmu->get_guest_pgd     = nested_ept_get_eptp;
	vcpu->arch.mmu->inject_page_fault = nested_ept_inject_page_fault;
	vcpu->arch.mmu->get_pdptr         = kvm_pdptr_read;

	vcpu->arch.walk_mmu              = &vcpu->arch.nested_mmu;
}

static void nested_ept_uninit_mmu_context(struct kvm_vcpu *vcpu)
{
	vcpu->arch.mmu = &vcpu->arch.root_mmu;
	vcpu->arch.walk_mmu = &vcpu->arch.root_mmu;
}
```

当 vCPU 切换 guest mode 的时候，同样的需要切换 mmu ，也就是:
* **非 nested / L1 运行时**
  * `mmu == root_mmu`
  * `walk_mmu == root_mmu`
* **L2 运行时（nested EPT 打开）**
  * `mmu == guest_mmu`
  * `walk_mmu == nested_mmu`（或特定场景下不同）

## kvm_mmu 中的函数指针
<!-- de311f0f-7e04-436a-a769-5e87bf6bb0d7 -->

```c
struct kvm_mmu {
	unsigned long (*get_guest_pgd)(struct kvm_vcpu *vcpu);
	u64 (*get_pdptr)(struct kvm_vcpu *vcpu, int index);
	int (*page_fault)(struct kvm_vcpu *vcpu, struct kvm_page_fault *fault);
	void (*inject_page_fault)(struct kvm_vcpu *vcpu,
				  struct x86_exception *fault);
	gpa_t (*gva_to_gpa)(struct kvm_vcpu *vcpu, struct kvm_mmu *mmu,
			    gpa_t gva_or_gpa, u64 access,
			    struct x86_exception *exception);
	int (*sync_spte)(struct kvm_vcpu *vcpu,
			 struct kvm_mmu_page *sp, int i);
	// ...
```

找到赋值的方法:

```txt
rg "get_guest_pgd\s+=" arch/x86/kvm
rg "get_pdptr\s+=" arch/x86/kvm
rg "page_fault\s+=" arch/x86/kvm
rg "inject_page_fault\s+=" arch/x86/kvm
rg "gva_to_gpa\s+=" arch/x86/kvm
rg "sync_spte\s+=" arch/x86/kvm
```

struct kvm_mmu 的 6 个核心函数指针

1. get_guest_pgd - 获取页表根指针
get_guest_cr3              // 非嵌套：读 guest CR3
nested_ept_get_eptp        // 嵌套 EPT：读 EPT12 指针 (VMCS12)

2. page_fault - 最重要，处理 page faults
kvm_tdp_page_fault         // TDP/EPT/NPT
paging64_page_fault        // 64-bit Shadow Paging
ept_page_fault             // Shadow EPT (嵌套虚拟化)
NULL                       // nested_mmu (不处理 page faults！)

3. gva_to_gpa - GVA → GPA 地址转换
paging64_gva_to_gpa        // Walk guest 64-bit 页表
paging32_gva_to_gpa        // Walk guest 32-bit 页表
nonpaging_gva_to_gpa       // Real mode (GVA == GPA)
ept_gva_to_gpa             // Walk EPT12 (Shadow EPT)

4. inject_page_fault - 向 guest 注入异常
kvm_inject_page_fault      // 注入标准 #PF
nested_ept_inject_page_fault  // 注入 EPT violation 给 L1

5. sync_spte - 同步 shadow SPTE（仅 shadow paging）
paging64_sync_spte         // 64-bit shadow paging
ept_sync_spte              // Shadow EPT
NULL                       // TDP 和 nested_mmu 不需要

6. get_pdptr - 获取 PDPTR（PAE 模式）
kvm_pdptr_read             // 几乎总是这个

关键洞察

1. TDP 仍需 gva_to_gpa
// TDP 初始化 (mmu.c:5729-5734)
context->page_fault = kvm_tdp_page_fault;  // 处理 GPA→HPA
context->gva_to_gpa = paging64_gva_to_gpa; // 仍需 walk guest 页表！
context->sync_spte = NULL;                 // 不需要同步

TDP 只处理 GPA→HPA，仍需 walk guest 的页表做 GVA→GPA！

2. nested_mmu 不处理 page faults
// nested_mmu 初始化
g_context->page_fault = NULL;          // ❌ 不处理！
g_context->gva_to_gpa = paging64_...;  // ✅ 只做地址转换
g_context->sync_spte = NULL;

3. 完整的赋值矩阵
┌────────────────────────┬─────────────────────┬─────────────────────┬────────────────────┐
│      MMU Context       │     page_fault      │     gva_to_gpa      │     sync_spte      │
├────────────────────────┼─────────────────────┼─────────────────────┼────────────────────┤
│ root_mmu (TDP)         │ kvm_tdp_page_fault  │ paging64_gva_to_gpa │ NULL               │
├────────────────────────┼─────────────────────┼─────────────────────┼────────────────────┤
│ root_mmu (Shadow)      │ paging64_page_fault │ paging64_gva_to_gpa │ paging64_sync_spte │
├────────────────────────┼─────────────────────┼─────────────────────┼────────────────────┤
│ guest_mmu (Shadow EPT) │ ept_page_fault      │ ept_gva_to_gpa      │ ept_sync_spte      │
├────────────────────────┼─────────────────────┼─────────────────────┼────────────────────┤
│ nested_mmu             │ NULL                │ paging64_gva_to_gpa │ NULL               │
└────────────────────────┴─────────────────────┴─────────────────────┴────────────────────┘
嵌套虚拟化的地址转换链

L2 GVA
   |
   | [nested_mmu.gva_to_gpa]  (Walk L2 页表)
   v
L2 GPA
   |
   | [guest_mmu.gva_to_gpa]   (Walk EPT12)
   v
L1 GPA
   |
   | [L1's EPT01]
   v
L0 HPA

### root_mmu (非嵌套 L1)

1. TDP/EPT 模式 (init_kvm_tdp_mmu)

```c
get_guest_pgd       = get_guest_cr3              // 读 guest CR3
get_pdptr           = kvm_pdptr_read             // 读 PDPTR
page_fault          = kvm_tdp_page_fault         // TDP page fault 处理
inject_page_fault   = kvm_inject_page_fault      // 注入 #PF 给 guest
gva_to_gpa          = paging64_gva_to_gpa 等     // Walk guest 页表 (GVA→GPA)
sync_spte           = NULL                       // TDP 不需要同步
```

**关键点**: 即使用 TDP，仍需 `gva_to_gpa` 来 walk guest 的页表！

2. Shadow Paging 64-bit (paging64_init_context)

```c
get_guest_pgd       = get_guest_cr3              // 读 guest CR3
get_pdptr           = kvm_pdptr_read             // 读 PDPTR
page_fault          = paging64_page_fault        // Shadow page fault 处理
inject_page_fault   = kvm_inject_page_fault      // 注入 #PF 给 guest
gva_to_gpa          = paging64_gva_to_gpa        // Walk guest 页表
sync_spte           = paging64_sync_spte         // 同步 shadow PTE
```

### guest_mmu (嵌套，处理 L2 page faults)
Shadow EPT (kvm_init_shadow_ept_mmu)

```c
get_guest_pgd       = nested_ept_get_eptp        // 读 EPT12 指针 (VMCS12)
get_pdptr           = kvm_pdptr_read             // 读 PDPTR
page_fault          = ept_page_fault             // Shadow EPT page fault
inject_page_fault   = nested_ept_inject_page_fault  // 注入 EPT violation 给 L1
gva_to_gpa          = ept_gva_to_gpa             // Walk EPT12 (L2 GPA → L1 GPA)
sync_spte           = ept_sync_spte              // 同步 shadow EPT entry
```

### nested_mmu (嵌套，L2 GVA→GPA 转换)

init_kvm_nested_mmu
```c
get_guest_pgd       = get_guest_cr3              // 读 L2 的 CR3
get_pdptr           = kvm_pdptr_read             // 读 PDPTR
page_fault          = NULL                       // ❌ 不处理 page faults
inject_page_fault   = kvm_inject_page_fault      // 注入 #PF
gva_to_gpa          = paging64_gva_to_gpa 等     // Walk L2 页表 (L2 GVA → L2 GPA)
sync_spte           = NULL                       // 不处理 shadow
```

**关键注释**
```c
/*
 * L2 page tables are never shadowed, so there is no need to sync
 * SPTEs.
 */
g_context->sync_spte = NULL;
```

嵌套虚拟化 (Shadow EPT) 的基本流程

```
L2 访问地址
    |
    +-- EPT violation (L2 GPA 在 EPT02 中不存在)
    |
    +-- guest_mmu.page_fault (ept_page_fault)
    |   |
    |   +-- 调用 guest_mmu.gva_to_gpa (ept_gva_to_gpa)
    |   |   Walk EPT12: L2 GPA → L1 GPA
    |   |
    |   +-- 如果 EPT12 中不存在映射:
    |   |   调用 guest_mmu.inject_page_fault
    |   |   → nested_ept_inject_page_fault
    |   |   → Inject EPT violation 给 L1
    |   |
    |   +-- 如果 EPT12 中存在映射:
    |   |   将 L1 GPA → L0 HPA (通过 L1's EPT01)
    |   |   构建 shadow EPT entry: L2 GPA → L0 HPA
    |   |   调用 guest_mmu.sync_spte (ept_sync_spte)
    |
    +-- 映射成功，返回
```


#### 测试 ept 关闭的场景

结果是这样的:
```sh
sudo rmmod kvm_intel
sudo rmmod kvm
sudo modprobe kvm
sudo modprobe kvm_intel ept=N
```

```txt
@[
        paging64_page_fault+5
        kvm_mmu_do_page_fault+280
        kvm_mmu_page_fault+134
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1337979
```

```txt
@[
        paging64_gva_to_gpa+5
        vcpu_mmio_gva_to_gpa+124
        emulator_read_write_onepage+132
        emulator_read_write+192
        x86_emulate_insn+1412
        x86_emulate_instruction+749
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 1699
@[
        paging64_gva_to_gpa+5
        kvm_read_guest_virt_helper+149
        kvm_handle_invpcid+111
        handle_invpcid+374
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 2088
```

INVPCID

注意这里的 exit reason 为 INVPCID 指令，这个是修改 TLB 的操作


```txt
@[
        paging64_sync_spte+5
        __kvm_mmu_invalidate_addr+522
        kvm_mmu_invalidate_addr+230
        kvm_mmu_invlpg+33
        handle_invlpg+74
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 11196
```

默认情况下，也就是打开 tdp 和 tdp_mmu，对三个函数的观测结果，只能看到这个
paging64_gva_to_gpa 也就是来 walk page table:
```txt
@[
        paging64_gva_to_gpa+5
        kvm_fetch_guest_virt+91
        __do_insn_fetch_bytes+342
        x86_decode_insn+476
        x86_decode_emulated_instruction+58
        x86_emulate_instruction+1997
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 710
```

## nested_ept_init_mmu_context vs kvm_init_shadow_ept_mmu
<!-- c8056e5f-11f6-485c-8e26-352afbd40ce8 -->

三层调用关系：

nested_ept_init_mmu_context()     ← 外层（VMX 特定）
    |
    +-- 切换 mmu 和 walk_mmu 指针
    |   vcpu->arch.mmu = &guest_mmu
    |   vcpu->arch.walk_mmu = &nested_mmu
    |
    +-- 设置回调函数
    |   get_guest_pgd = nested_ept_get_eptp
    |   inject_page_fault = nested_ept_inject_page_fault
    |
    +-- nested_ept_new_eptp()     ← 中层（参数提取）
            |
            +-- 从 VMCS12 提取 EPT 参数
            |
            +-- kvm_init_shadow_ept_mmu()  ← 内层（通用 MMU）
                    |
                    +-- 初始化 **guest_mmu** 核心参数
                    +-- page_fault = ept_page_fault
                    +-- gva_to_gpa = ept_gva_to_gpa
                    +-- sync_spte = ept_sync_spte

```txt
@[
    kvm_init_shadow_ept_mmu+5
    prepare_vmcs02.constprop.0+3599
    nested_vmx_enter_non_root_mode+4489
    nested_vmx_run+264
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 1125613
```

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
			-> kvm_init_shadow_mmu
				-> shadow_mmu_init_context
					- nonpaging_init_context
					- paging64_init_context
					- paging32_init_context
}
```

如果打开嵌套模式，这个东西切换的非常频繁:
```txt
@[
    kvm_init_mmu+5
    nested_vmx_load_cr3+185
    nested_vmx_enter_non_root_mode+1273
    nested_vmx_run+293
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 313711
```

如果不去开启嵌套，仅仅在虚拟机启动的时候，可以观测到这些:
```txt
@[
        kvm_init_mmu+5
        kvm_set_cr0+331
        handle_cr+1976
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 63
@[
        kvm_init_mmu+5
        kvm_post_set_cr4+165
        kvm_set_cr4+267
        handle_cr+1960
        vmx_handle_exit+18
        vcpu_enter_guest.constprop.0+973
        vcpu_run+50
        kvm_arch_vcpu_ioctl_run+736
        kvm_vcpu_ioctl+279
        __x64_sys_ioctl+151
        do_syscall_64+126
        entry_SYSCALL_64_after_hwframe+118
]: 98
```

所以，通过这些函数，正好初始化 nested guest root_mmu

## handle_ept_violation 可以覆盖 l1 的 ept 和 l0 的 ept 的
<!-- eeed29be-27e9-425b-8d4f-8bb369cc283e -->

最开始的路径是相同的:
```txt
sudo bpftrace -e 'kfunc:kvm_intel:handle_ept_violation { @ = hist(args->vcpu->arch.hflags & 1) }'
[0]                 1444 | |
[1]               320026 |@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@|
```

```c
static inline bool is_guest_mode(struct kvm_vcpu *vcpu)
{
	return vcpu->arch.hflags & HF_GUEST_MASK;
}
```

```txt
sudo bpftrace -e 'kprobe:ept_page_fault { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'

@[
    ept_page_fault+5
    kvm_mmu_do_page_fault+275
    kvm_mmu_page_fault+130
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+407
    kvm_vcpu_ioctl+563
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 320011
```
发现，如果 vCPU 是 guest mode exit ，那么最后处理的路线就是 ept_page_fault

(补充一下，后面的路径差别导致的差别吧)

## 每次出现 mmu context 变化，那么就会有 mmu 的变化
测试一个 4 CPU 的虚拟机启动:
```txt
@[
    kvm_init_mmu+5
    kvm_arch_vcpu_create+638
    kvm_vm_ioctl+2073
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 4
@[
    kvm_init_mmu+5
    kvm_mmu_after_set_cpuid+77
    kvm_set_cpuid+1703
    kvm_vcpu_ioctl_set_cpuid2+79
    kvm_arch_vcpu_ioctl+3888
    kvm_vcpu_ioctl+896
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 4
@[
    kvm_init_mmu+5
    kvm_set_msr_common+4536
    __kvm_set_msr+145
    kvm_emulate_wrmsr+81
    vmx_handle_exit+1898
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 5
@[
    kvm_init_mmu+5
    kvm_post_set_cr0+255
    kvm_set_cr0+290
    handle_cr+1711
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 7
@[
    kvm_init_mmu+5
    kvm_post_set_cr4+188
    kvm_set_cr4+162
    handle_cr+1730
    vmx_handle_exit+301
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 14
```
太有趣了，每一个 backtrace 正好是对应一个 x86 的设置模式。

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
