# KVM TLB Flush 机制分析

> [!NOTE]
> 参考神奇海螺的意见，有待验证

## 1. TLB 失效指令对比

### 1.1 指令概览

| 指令 | 平台 | 标识符 | 作用层次 | 说明 |
|------|------|--------|----------|------|
| `invlpg` | Intel/AMD | - | 当前地址空间 | 使指定虚拟地址的 TLB 项失效 |
| `invlpga` | AMD | ASID | 指定地址空间 | 可选择其他 ASID，主要用于 SVM |
| `invvpid` | Intel | VPID | Guest | 刷新指定 VPID 的 TLB 项 |
| `invpcid` | Intel/AMD | PCID | Host | 刷新指定 PCID 的 TLB 项 |

### 1.2 指令详解

#### invlpg (Invalidate TLB Entry)
```asm
invlpg [addr]
```
- 使**当前地址空间**中虚拟地址 `addr` 对应的 TLB 项失效
- 不区分 PCID/ASID（除非在特殊模式下）
- 是 KVM 中需要重点监控的指令

#### invlpga (AMD SVM)
```asm
invlpga rAX, rCX
; rAX: 虚拟地址
; rCX: ASID (Address Space ID)
```
- 只失效**指定 ASID + 指定 VA** 的 TLB 项
- 仅 AMD 支持，主要用于在不切换 CR3 的状态下精准清除 Guest TLB

#### invvpid (Intel VMX)
```c
// arch/x86/kvm/vmx/vmx_ops.h
static inline void __invvpid(unsigned long ext, u16 vpid, gva_t gva)
{
    struct {
        u64 vpid;
        u64 gva;
    } operand = { vpid, gva };
    asm volatile (__ex("invvpid %1, %2")
                  : : "m" (operand), "r" ((u64)ext), "m" (*addr));
}
```

INVVPID 类型：
| 类型 | 说明 |
|------|------|
| `VMX_VPID_EXTENT_INDIVIDUAL_ADDR` | 使指定 VPID + GVA 的 TLB 项失效 |
| `VMX_VPID_EXTENT_SINGLE_CONTEXT` | 使指定 VPID 的所有 TLB 项失效 |
| `VMX_VPID_EXTENT_ALL_CONTEXT` | 使所有 VPID 的 TLB 项失效 |

#### invpcid (Intel/AMD)
用于处理客户机发出的 INVPCID 指令：
- Intel: `arch/x86/kvm/vmx/vmx.c:handle_invpcid`
- AMD: `arch/x86/kvm/svm/svm.c:invpcid_interception`

## 2. KVM TLB Flush 架构抽象

### 2.1 核心接口

KVM 通过 `kvm_x86_ops` 抽象了 TLB flush 操作：

```c
// arch/x86/include/asm/kvm_host.h
struct kvm_x86_ops {
    // ...
    void (*flush_tlb_all)(struct kvm_vcpu *vcpu);
    void (*flush_tlb_current)(struct kvm_vcpu *vcpu);
    void (*flush_tlb_gva)(struct kvm_vcpu *vcpu, gva_t addr);
    void (*flush_tlb_guest)(struct kvm_vcpu *vcpu);
    // ...
};
```

### 2.2 VMX 实现 (Intel)

```c
// arch/x86/kvm/vmx/vmx.c

void vmx_flush_tlb_all(struct kvm_vcpu *vcpu)
{
    struct vcpu_vmx *vmx = to_vmx(vcpu);

    /*
     * INVEPT 必须在 EPT 启用时执行，无论 VPID 是否启用。
     * 因为 CPU 不保证在 VM-Entry 时自动失效 guest-physical 映射。
     */
    if (enable_ept) {
        ept_sync_global();           // INVEPT (All-Context)
    } else if (enable_vpid) {
        if (cpu_has_vmx_invvpid_global()) {
            vpid_sync_vcpu_global(); // INVVPID (All-Context)
        } else {
            vpid_sync_vcpu_single(vmx->vpid);
            vpid_sync_vcpu_single(vmx->nested.vpid02);
        }
    }
}

void vmx_flush_tlb_current(struct kvm_vcpu *vcpu)
{
    struct kvm_mmu *mmu = vcpu->arch.mmu;
    u64 root_hpa = mmu->root.hpa;

    if (!VALID_PAGE(root_hpa))
        return;

    if (enable_ept)
        vmx_flush_tlb_ept_root(root_hpa);  // INVEPT (Single-Context)
    else
        vpid_sync_context(vmx_get_current_vpid(vcpu));  // INVVPID
}

void vmx_flush_tlb_gva(struct kvm_vcpu *vcpu, gva_t addr)
{
    /*
     * 如果 vpid==0，vpid_sync_vcpu_addr() 是空操作。
     * 这是因为当 VPID 禁用时，VM-Enter/VM-Exit 会自动刷新 TLB。
     */
    vpid_sync_vcpu_addr(vmx_get_current_vpid(vcpu), addr);
}

void vmx_flush_tlb_guest(struct kvm_vcpu *vcpu)
{
    vpid_sync_context(vmx_get_current_vpid(vcpu));
}
```

### 2.3 SVM 实现 (AMD)

```c
// arch/x86/kvm/svm/svm.c

static void svm_flush_tlb_asid(struct kvm_vcpu *vcpu)
{
    struct vcpu_svm *svm = to_svm(vcpu);

    /*
     * ASID 管理：通过切换 ASID 来隐式刷新 TLB
     * 如果 ASID 已经用完，则执行显式的 TLB 刷新
     */
    if (!kvm_vcpu_flush_tlb_guest(vcpu))
        svm->vmcb->control.tlb_ctl = TLB_CONTROL_FLUSH_ASID;
}

static void svm_flush_tlb_current(struct kvm_vcpu *vcpu)
{
    struct vcpu_svm *svm = to_svm(vcpu);

    /* 仅刷新当前 VCPU 的 TLB */
    svm->vmcb->control.tlb_ctl = TLB_CONTROL_FLUSH_ASID;
}

static void svm_flush_tlb_all(struct kvm_vcpu *vcpu)
{
    struct vcpu_svm *svm = to_svm(vcpu);

    /* 刷新所有 TLB */
    svm->vmcb->control.tlb_ctl = TLB_CONTROL_FLUSH_ALL_ASID;
}

static void svm_flush_tlb_gva(struct kvm_vcpu *vcpu, gva_t gva)
{
    struct vcpu_svm *svm = to_svm(vcpu);

    /* 使用 invlpga 精确刷新指定 ASID + GVA */
    invlpga(gva, svm->vmcb->control.asid);
}
```

## 3. KVM_REQ_TLB_FLUSH 请求机制

### 3.1 四种 TLB Flush 请求

| 请求类型 | 说明 | 处理函数 |
|----------|------|----------|
| `KVM_REQ_TLB_FLUSH` | 全刷新，所有上下文 | `kvm_vcpu_flush_tlb_all()` |
| `KVM_REQ_TLB_FLUSH_CURRENT` | 仅当前 MMU 上下文 | `kvm_vcpu_flush_tlb_current()` |
| `KVM_REQ_TLB_FLUSH_GUEST` | 刷新 Guest 的 TLB | `kvm_vcpu_flush_tlb_guest()` |
| `KVM_REQ_HV_TLB_FLUSH` | Hyper-V 精确刷新 | `kvm_hv_vcpu_flush_tlb()` |

### 3.2 请求处理流程

```c
// arch/x86/kvm/x86.c

void kvm_service_local_tlb_flush_requests(struct kvm_vcpu *vcpu)
{
    if (kvm_check_request(KVM_REQ_TLB_FLUSH_CURRENT, vcpu))
        kvm_vcpu_flush_tlb_current(vcpu);

    if (kvm_check_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu))
        kvm_vcpu_flush_tlb_guest(vcpu);
}

/* 在 vcpu_enter_guest() 中处理 */
if (kvm_check_request(KVM_REQ_TLB_FLUSH, vcpu))
    kvm_vcpu_flush_tlb_all(vcpu);

kvm_service_local_tlb_flush_requests(vcpu);
```

### 3.3 典型触发场景

```c
// 1. vCPU 调度到不同 pCPU 时
// arch/x86/kvm/vmx/vmx.c:vmx_vcpu_load
if (vmx->loaded_vmcs->cpu != cpu) {
    /*
     * 刷新所有 EPTP/VPID 上下文，新的 pCPU 可能有
     * 之前关联此 vCPU 的陈旧 TLB 项。
     */
    kvm_make_request(KVM_REQ_TLB_FLUSH, vcpu);
}

// 2. 影子页表需要同步时
// arch/x86/kvm/x86.c
if (!tdp_enabled) {
    kvm_mmu_sync_roots(vcpu);
    kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
}

// 3. 嵌套 VM-Exit 时
// arch/x86/kvm/vmx/nested.c
kvm_make_request(KVM_REQ_TLB_FLUSH_GUEST, vcpu);
```

## 4. VPID vs ASID

### 4.1 VPID (Virtual-Processor Identifier) - Intel

```c
// arch/x86/kvm/vmx/vmx.c

static inline int vmx_get_current_vpid(struct kvm_vcpu *vcpu)
{
    if (is_guest_mode(vcpu) && nested_cpu_has_vpid(get_vmcs12(vcpu)))
        return nested_get_vpid02(vcpu);  // 嵌套虚拟机的 VPID
    return to_vmx(vcpu)->vpid;            // L1 的 VPID
}
```

VPID 特点：
- 每个 vCPU 分配唯一的 VPID
- 嵌套虚拟化时需要维护 vpid02 (L2 的 VPID)
- VPID=0 表示禁用，此时 VM-Enter/Exit 自动刷新 TLB

### 4.2 ASID (Address Space ID) - AMD

```c
// arch/x86/kvm/svm/svm.h
struct vcpu_svm {
    // ...
    u32 asid;           // 当前使用的 ASID
    // ...
};

struct svm_cpu_data {
    u64 asid_generation;
    u32 max_asid;
    u32 next_asid;
    u32 min_asid;
};
```

ASID 管理策略：
```c
// arch/x86/kvm/svm/svm.c

static int svm_flush_asid(struct kvm_vcpu *vcpu)
{
    struct vcpu_svm *svm = to_svm(vcpu);
    struct svm_cpu_data *sd = per_cpu_ptr(&svm_data, cpu);

    /*
     * ASID 分配策略：
     * 1. 如果 ASID 还有剩余，直接分配新的 ASID
     * 2. 如果 ASID 用完，增加 generation，重新分配
     * 3. 当 generation 不匹配时，需要执行 TLB 刷新
     */
    if (sd->asid_generation != svm->asid_generation) {
        /* 需要刷新 TLB，因为 ASID 可能被其他虚拟机使用过 */
        svm->vmcb->control.tlb_ctl = TLB_CONTROL_FLUSH_ASID;
    }
}
```

## 5. TLB Flush 性能监控

### 5.1 perf 监控

```bash
# 监控 TLB flush 事件
sudo perf top -e tlb:tlb_flush
```

典型输出解析：
```
Overhead  Trace output
  23.78%  pages=1 reason=remote shootdown (1)
  10.90%  pages=-1 reason=remote shootdown (1)
   6.37%  pages=2 reason=remote shootdown (1)
```

### 5.2 常见调用链分析

#### 场景 1: 写保护页处理 (COW)
```
kvm_flush_tlb_multi
  -> flush_tlb_mm_range
    -> ptep_clear_flush
      -> do_wp_page              # 写时复制
        -> handle_mm_fault
```

#### 场景 2: 内存迁移 (NUMA)
```
kvm_flush_tlb_multi
  -> flush_tlb_mm_range
    -> tlb_flush_mmu
      -> change_prot_numa        # NUMA 页面迁移
        -> task_numa_work
```

#### 场景 3: 取消映射
```
kvm_flush_tlb_multi
  -> flush_tlb_mm_range
    -> unmap_page_range
      -> exit_mmap               # 进程退出
```

### 5.3 KVM 特定场景

#### vCPU 调度
```
vmx_vcpu_load_vmcs
  -> kvm_make_request(KVM_REQ_TLB_FLUSH)
    -> vcpu_enter_guest
      -> kvm_vcpu_flush_tlb_all
```

#### EPT 页表更新
```
kvm_tdp_page_fault
  -> kvm_mmu_do_page_fault
    -> set_pmd_migration_entry
      -> pmdp_invalidate
        -> tlb_flush_mmu
```

## 6. 优化建议

### 6.1 减少不必要的 TLB Flush

1. **使用 VPID/ASID**：避免每次 VM-Enter/Exit 都刷新 TLB
2. **批量刷新**：累积多个修改后一次性刷新
3. **范围刷新**：优先使用 `invvpid`/`invlpga` 精确刷新，避免全局刷新

### 6.2 监控指标

```bash
# 查看 KVM 统计信息
cat /sys/kernel/debug/kvm/<vm-id>/stats

# 关键指标：
# - remote_tlb_flush: 远程 TLB flush 次数
# - tlb_flush: vCPU 本地 TLB flush 次数
# - exits: VM-Exit 次数（包括 TLB 相关的 exit）
```

## 7. 待办事项

- [ ] 验证 invpcid 在 AMD 上的支持情况
- [ ] 研究 invlpga 是否可以替代 invpcid 在 AMD 中的作用
- [ ] 深入分析 TLB 刷新对性能的影响
- [ ] 研究 ASID 耗尽时的处理策略

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
