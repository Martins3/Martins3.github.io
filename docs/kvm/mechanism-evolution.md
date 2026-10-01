# KVM 机制演进与源码阅读索引

这里整理的是：早期 KVM
为了实现虚拟化付出了哪些额外开销，后来用什么办法减少这些开销，以及旧路径今天为什么还存在。重点是
x86 的 MMU 和中断，再补充脏页记录、I/O、调度和 ARM64。

先按当前实现状态读：**旧 PV MMU、legacy device assignment、lazy guest FPU、
APF page-ready 的旧异常通知和 x86 MMU shrinker 已移除；Shadow MMU、传统 direct
MMU、interrupt window、软件 timer 等仍与后续方案并存。** 另将已删除的接口、
调试设施、恢复过的实现和互补优化分别列出，再解释各项优化的开销。

源码基准是本机 `/home/martins3/data/kernel/linux-drm`，HEAD 为
`eb5a10dc0e0026d8144f904b4b98b742a335a1d9`，`git describe` 为
`v7.2-1-geb5a10dc0e00`。下文的“当前”指这份源码。旧笔记中的调用栈来自其他版本和不同运行配置，不能直接当成这份源码的默认行为。定位使用文件、函数和宏名称。

## 核查标准

以下分类直接检查了 `linux-drm` 的当前代码和本地 Git 历史。删除提交均在当前
HEAD 的祖先链中，随后核对当前入口、调用分支、module 参数和兼容处理，而不只搜索旧函数名。

- **实现已移除**：当前不再实现旧机制，也没有选择旧实现的回退分支。保留 UAPI
  编号或文档中的 deprecated 标记，不等于实现仍然存在。
- **实现仍并存**：旧、新两条路径当前都有可达的调用分支；是否选择旧路径由能力、
  配置或运行状态决定。旧路径也可能服务于 nested 等不同用途。
- **互补优化**：优化的是不同环节，可以同时工作，不能当成一条替代链。

历史中出现过删除也不能直接推断当前状态：下面单独列出软件 vNMI 恢复和 PFN cache
同名重写的例子。这里核查的是具体实现，不把函数改名、封装调整归为机制删除。

## 已经移除的运行机制

| 旧实现 | 当前状态 | 删除或替换提交 | 当前方案和删除原因 |
| --- | --- | --- | --- |
| x86 PV MMU 的批量 PTE 操作 hypercall | host 和 guest 旧实现均已删除；不是回退路径 | `fb92045843a8`、`5202397df819` | 普通 Shadow MMU 或硬件 TDP；旧实现没有使用者，提交说明其性能和维护成本不合算 |
| KVM 自己管理 PCI/IOMMU 的 legacy device assignment | 旧直通实现已删除 | `ad6260da1e23` | 使用 VFIO；删除的是旧 KVM 直通后端，不是设备直通能力 |
| KVM guest FPU 的 lazy activation/deactivation | 基于首次使用再激活的旧路径已删除 | `bd7e5b0899a4` | 按 guest 运行边界加载、保存 FPU 状态；删除时 guest FPU 已经始终 active |
| APF page-ready 的 synthetic `#PF` 通知 | 旧 page-ready 投递方式不再实现，也无旧方式回退 | `2635b5c4a0e4` | 使用可屏蔽中断和共享 token；避免与真正 `#PF` 冲突，APF 本身仍存在 |
| x86 KVM MMU shrinker | shrinker 回调与注册已删除 | `fe140e611d34` | 不再由这个 shrinker 在 host 内存压力下驱逐 KVM 页表；删除没有收益且会干扰 VM 的路径 |

日期以下统一使用提交的 author date，避免和合入版本、committer date 混淆。

### PV MMU 批量页表操作

host 删除提交为 `fb92045843a8cd99c7b843d9b567a680a3854ba1`，标题为
`KVM: MMU: remove KVM host pv mmu support`；guest 删除提交为
`5202397df819d3c5a3f201bd4af6b86542115fb6`，标题为
`KVM guest: remove KVM guest pv mmu support`，author date 均为 2011 年 11 月 1 日。

当前 `arch/x86/kvm/x86.c` 的 `____kvm_emulate_hypercall()` 没有
`KVM_HC_MMU_OP` 的处理分支，未知操作按 `-KVM_ENOSYS` 处理；
`arch/x86/kernel/kvm.c` 也不再实现旧 guest PV MMU 操作。
`arch/x86/include/uapi/asm/kvm_para.h` 中仍有 `KVM_FEATURE_MMU_OP` 和相关
payload，文档仍写 deprecated。这是 **保留历史 ABI 名称，但实现已经删除** 的例子，
与仍实现的 PV TLB flush、PV IPI、PV EOI 不同。

### Legacy device assignment

`ad6260da1e23cf937806e42c8490af3ff4530474`，
`KVM: x86: drop legacy device assignment`，author date 为 2017 年 3 月 27 日。
diff 删除了 `arch/x86/kvm/assigned-dev.c`、`assigned-dev.h` 和
`arch/x86/kvm/iommu.c`，也删除了对应的 ioctl 分发。这些文件和旧分发当前均不存在。

VFIO 是后续的设备直通方案；当前 KVM 侧的桥接代码在 `virt/kvm/vfio.c`，
例如 `kvm_vfio_create()`、`kvm_vfio_file_add()`，注册
`KVM_DEV_TYPE_VFIO`。VFIO 设备控制和 DMA 映射由对应 VFIO/IOMMU 子系统承担。
因此旧笔记若出现 `KVM_ASSIGN_PCI_DEVICE` 一类接口，应归到历史实现，
而不是把当前 VFIO、irqfd 或 PI 当成仍在使用该旧后端。[原始删除补丁](https://lkml.rescloud.iu.edu/hypermail/linux/kernel/1703.3/01776.html)

### Lazy guest FPU

`bd7e5b0899a429445cc6e3037c13f8b5ae3be903`，
`KVM: x86: remove code for lazy FPU handling`，author date 为 2017 年 2 月 3 日。
diff 删除了 `vmx_fpu_activate()`、`vmx_fpu_deactivate()` 等旧实现，
以及 `fpu_active` 判断和 `KVM_REQ_DEACTIVATE_FPU` 路径。

旧思路是利用 CR0.TS 和 `#NM` 等在首次 FPU 使用时激活 guest FPU，争取省掉
暂时不用 FPU 的状态处理。当前 `kvm_arch_vcpu_ioctl_run()` 在运行前调用
`kvm_load_guest_fpu()`，结束时调用 `kvm_put_guest_fpu()`，内部使用
`fpu_swap_kvm_fpstate()`。旧 lazy activation 不再是可选择的后端。

这里删除的是 KVM 这一层的 lazy FPU 路径，不能推导出 guest 自己的 CR0.TS、
`#NM` 语义或所有 host FPU 优化也都消失，更不能推导出每次硬件 VM exit 都必须
完整交换 FPU。后续保留上下文等优化属于另一层。

### APF page-ready 的旧异常通知

`2635b5c4a0e407b84f68e188c719f28ba0e9ae1b`，
`KVM: x86: interrupt based APF 'page ready' event delivery`，
author date 为 2020 年 5 月 25 日。diff 将
`kvm_arch_async_page_present()` 中构造 `PF_VECTOR` 并调用
`kvm_inject_page_fault()` 的分支替换为 `kvm_apic_set_irq()`。

当前 `arch/x86/kvm/x86.h` 的 `__kvm_pv_async_pf_enabled()` 同时要求
`KVM_ASYNC_PF_ENABLED` 和 `KVM_ASYNC_PF_DELIVERY_AS_INT`：
只设置旧 enable 位不会启用旧的异常投递协议。当前
`kvm_arch_async_page_present()` 写入 ready token 后投递 LAPIC IRQ，
没有选择旧 page-ready `#PF` 的分支。

**删除的只有 page-ready 的旧投递方式**。page-not-present 仍可使用 synthetic
`#PF`，普通缺页异常和 APF worker 机制也都保留。guest 侧在
`b1d405751cd5792856b1b8333aafaca6bf09ccbb` 切换到中断 ready 通知。
原笔记：[async-pf.md](mmu/async-pf.md)。

### MMU shrinker

`fe140e611d3450708a962d937546c7bd164183ea`，
`KVM: x86/mmu: Remove KVM's MMU shrinker`，author date 为 2024 年 11 月 1 日。
diff 删除了 `mmu_shrink_scan()`、`mmu_shrink_count()`、`mmu_shrinker`
及注册、释放逻辑。当前 `arch/x86/kvm/mmu/mmu.c` 没有这些实现，
`kvm_mmu_vendor_module_init()` 也不再注册 MMU shrinker。

提交说明旧实现会干扰 VM，又没有明显收益；TDP MMU 原本也没有这个 shrinker
支持。删除不是“现在改由 TDP MMU shrinker 回收”，而是取消这条回收机制。
维护者的合入说明将其列入 6.13 的 MMU 变更。[合入说明](https://lkml.indiana.edu/hypermail/linux/kernel/2411.1/05462.html)

**取消 shrinker 不等于页表永远不释放**。当前
`make_mmu_pages_available()` 仍可按传统 MMU 的页数限制调用
`kvm_mmu_zap_oldest_mmu_pages()`；映射失效、root 失效和 VM 销毁也有各自的
zap/free 路径，TDP 页表另有自己的生命周期管理。
旧笔记中的“host 内存压力通过这个 shrinker 释放 shadow 页表”是历史行为。

## 已经移除的接口和调试设施

这些确实已经删除，但不应混写成“由新硬件机制替代的运行路径”。

| 项目 | 删除提交 | 当前核查及边界 |
| --- | --- | --- |
| `KVM_GET_NR_MMU_PAGES` ioctl 的实现 | `c5edd753a0bd`，2023 年 2 月 8 日 | `kvm_arch_vm_ioctl()` 没有 GET 分支，UAPI 编号标为 deprecated；`KVM_SET_NR_MMU_PAGES` 分支仍在，不能说 MMU 页数控制整体删除 |
| 旧 MMU auditing | `1bbc60d0c7e5`，2022 年 2 月 18 日 | `mmu_audit.c`、`CONFIG_KVM_MMU_AUDIT` 和审计挂点已删除；当前仍有 `CONFIG_KVM_PROVE_MMU` 等检查，不能说所有 MMU 调试检查删除 |

GET 接口删除的提交说明是未被使用及返回值截断等接口问题。
MMU auditing 删除的提交说明是实现失修、普通 guest 转向 TDP 后收益下降。
这两项都不能仅凭提交标题解释为性能提升。

## 当前仍然并存的机制

以下旧、新实现均有当前可达入口。“并存”不要求它们在同一个 vCPU、同一个时刻一起使用。

| 机制组合 | 当前选择条件或保留用途 | 源码证据 |
| --- | --- | --- |
| Shadow Paging 与 EPT/NPT | 普通 guest 按 `tdp_enabled` 选择；nested EPT/NPT 仍有 shadow 合成翻译 | `mmu.c` 的 `kvm_init_mmu()`、`kvm_init_shadow_ept_mmu()`、`kvm_init_shadow_npt_mmu()` |
| 传统 direct MMU 与专用 TDP MMU | EPT/NPT 开启后，再按 `tdp_mmu_enabled` 选择软件实现 | `mmu.c` 的 `kvm_tdp_page_fault()` 分发到 `direct_page_fault()` 或 `kvm_tdp_mmu_page_fault()` |
| 页表写跟踪与 leaf unsync | 上层或显式跟踪的 guest 页表仍保护；满足条件的 leaf 可暂时 unsync | `mmu_try_to_unsync_pages()`、`kvm_mmu_track_write()` |
| 软件访问采样与硬件 A/D | A/D 可用时清 A 位；不可用时撤销权限，以后续 fault 采样 | `spte.c` 的 `mark_spte_for_access_track()` |
| 软件首写 dirty logging 与 PML | 按 CPU dirty logging 能力及该映射的保护要求选择 | `kvm_arch_mmu_enable_log_dirty_pt_masked()`、`vmx_flush_pml_buffer()` |
| Bitmap、manual protect 与 dirty ring | 用户态选择 ABI；ring 可与辅助 bitmap 共同使用 | `mark_page_dirty_in_slot()`、`kvm_dirty_ring_reset()` |
| 软件 IRQ 注入、interrupt window 与 VID/PI | 普通非 nested 且 APICv active 的 LAPIC IRQ 绕过软件注入；ExtInt 优先保留软件路径 | `irq.c` 的 `kvm_cpu_has_injectable_intr()`；`vmx_enable_irq_window()`、`handle_interrupt_window()` |
| CR8/TPR 软件处理与硬件 APIC 虚拟化 | 按 TPR shadow、APICv 实际状态和具体寄存器操作选择 | `vmx_exec_control()`、`kvm_lapic_update_cr8_intercept()`、`vmx_update_msr_bitmap_x2apic()` |
| 软件 EOI、PV EOI 与硬件 EOI | 按 PV 配置、IRR/ISR 和 IOAPIC 通知要求选择，指定向量仍有 EOI exit | `apic_sync_pv_eoi_to_guest()`、`handle_apic_eoi_induced()` |
| 软件 IPI、PV IPI 与 IPI virtualization | PV IPI 批量减少退出；硬件 IPIV 另查 CPU 能力与 APICv 状态 | `kvm_pv_send_ipi()`、`vmx_tertiary_exec_control()` |
| 用户态、split 与内核 irqchip | VMM 选择模式；split 的 LAPIC 在内核，PIC/IOAPIC 在用户态 | `KVM_CREATE_IRQCHIP`、`KVM_CAP_SPLIT_IRQCHIP` 的处理分支 |
| 普通 MMIO/PIO、coalesced I/O 与 ioeventfd | 按注册区域、匹配条件、是否允许延迟及 ring 空间选择 | `coalesced_mmio_write()`、`ioeventfd_write()`，失败后仍有常规处理 |
| Intel 受限 guest 与 unrestricted guest | 无能力或未启用 unrestricted guest 时保留 vm86、identity map 等路径 | `vmx_set_cr0()`、`vmx_set_tss_addr()`、`vmx_load_mmu_pgd()` |
| 软件 LAPIC timer 与 VMX hardware timer | 依运行状态、计时范围、posted timer 配置选择和切换 | `start_hv_timer()`、`start_sw_timer()`、`kvm_lapic_switch_to_sw_timer()` |
| 普通自旋、PLE/pause filter 与 PV spinlock | 能力、guest 配置和超配负载不同；PV spinlock 不保证 PLE 从此消失 | `handle_pause()`、`kvm_vcpu_on_spin()`、`kvm_spinlock_init()` |
| 直接阻塞与 host halt polling | 依 polling 配置和历史等待时间，先轮询再按需阻塞 | `kvm_vcpu_halt()`、`kvm_vcpu_block()` |
| 软件 vNMI 回退与硬件 virtual NMI | 按 `enable_vnmi` 选择软件 mask 或 VMCS NMI-blocking 状态 | `vmx_get_nmi_mask()`、`vmx_set_nmi_mask()`，以及 `soft_vnmi_blocked` |
| VMREAD/VMWRITE 软件模拟与 VMCS shadowing | 依硬件、`enable_shadow_vmcs` 和字段 bitmap 选择；仍有软件 handler | `nested.c` 的 `handle_vmread()`、`handle_vmwrite()`、`copy_vmcs12_to_shadow()` |
| ARM64 nVHE 与 VHE | host 所处模式及隔离要求不同；protected KVM 仍需独立 hyp | `arm.c` 的 `early_kvm_mode_cfg()`，两套 `__kvm_vcpu_run()` |

表中 `mmu.c`、`spte.c` 在 `arch/x86/kvm/mmu/`；VMX 函数在
`arch/x86/kvm/vmx/`；APIC 状态处理在 `arch/x86/kvm/lapic.c`；
通用 eventfd、dirty ring 和阻塞代码在 `virt/kvm/`。
下文按这些入口解释优化解决的开销，读源码时以这里的保留分支为判断依据。

## 不能归为单向替代的组合

- **VID 与 PI**：前者决定 pending IRQ 的交付，后者优化新 IRQ 的 posting 和通知。
- **irqfd 与 APICv/AVIC**：前者连接后端事件和 KVM 路由，后者优化 vCPU 侧的交付。
- **PML 与 dirty ring**：前者发现脏页，后者向用户态交付记录。
- **VPID/ASID 与 PV TLB flush**：前者减少无谓丢弃翻译缓存，后者减少对被抢占 vCPU 的 shootdown 等待。
- **PLE 与 PV spinlock**：前者由硬件识别自旋，后者由 guest 明确表达等待；适用条件和介入环节不同。

## 曾被删除后恢复或重写的实现

### 软件 vNMI 已经恢复

`2c82878b0cb38fd516fd612c67852a6bbf282003` 在 2017 年 3 月 27 日要求硬件
virtual NMI 并删除软件模拟；但
`8a1b43922d0d1279e7936ba85c4c2a870403c95f` 在同年 11 月 6 日恢复，
提交说明发现部分 Core 2 Duo 型号也没有该能力。
`d02fcf50779ec9d8eb7a81473fd76efe3f04b3a5` 随后允许用 `vnmi=0` 测试回退。

当前 `vmx_get_nmi_mask()`、`vmx_set_nmi_mask()` 明确根据
`enable_vnmi` 使用 `soft_vnmi_blocked` 或 VMCS 的 NMI-blocking 状态。
因此 **它属于当前并存，不能因为搜到一次删除提交就列为完全移除**。
`vmx_enable_nmi_window()` 在不能使用硬件 NMI window 的相关条件下还会请求 IRQ window，
NMI window 与可屏蔽 IRQ window 也不能混为一谈。

### 旧 PFN cache 已删除但同名能力被重写

`357a18ad230f0867791b788d2b1d6f280f6f6e61` 在 2021 年 11 月 15 日删除
`kvm_map_gfn()`、`kvm_unmap_gfn()` 和旧 `gfn_to_pfn_cache`：
旧缓存未正确追踪 host 映射失效，不能安全复用 PFN。
`982ed0de4753ed6e71dbd40f82a5a066baf133ed` 在同年 12 月 10 日重新引入带
invalidation 支持的实现。

当前代码在 `virt/kvm/pfncache.c`，有
`gfn_to_pfn_cache_invalidate_start()`、`kvm_gpc_activate()`、`kvm_gpc_refresh()`。
这是 **旧实现被替换，但缓存机制以新实现保留**，不是旧、新两套 cache 后端同时存在，
也不是整个 GFN/PFN 缓存能力从 KVM 消失。原笔记：[todo-map-cache.md](todo-map-cache.md)。

## Deprecated 不等于实现删除

除了前述“MMU OP、GET ioctl 的编号保留而实现已删除”，还存在另一种情况：
`MSR_KVM_WALL_CLOCK`、`MSR_KVM_SYSTEM_TIME` 的旧编号被文档标为 deprecated，
但当前 `kvm_set_msr_common()` 和 `kvm_get_msr_common()` 仍有明确 case，
并按相应 guest PV feature 判断处理。

因此判断 deprecated 接口，必须逐个检查处理分支。文档状态、UAPI 是否定义、
运行实现是否支持，是三个不同的问题。

## 地址翻译和 Shadow MMU

### Shadow Paging 到 EPT 和 NPT

先把“硬件要走的页表”和“KVM 用来管理它的 C 结构体”分开。

```text
没有硬件 TDP，guest 已开启分页
  guest 页表给出 GVA -> GPA
  KVM 将它与 GPA -> HPA 合成为 shadow 页表
  CPU 实际使用 shadow 页表执行 GVA -> HPA

有 EPT 或 NPT
  CPU 使用 guest 页表执行 GVA -> GPA
  CPU 使用 KVM 建立的 EPT/NPT 执行 GPA -> HPA
  KVM 不必为了普通 guest 页表副本的一致性捕获每次 PTE 修改
```

Shadow Paging 的难点是：guest 修改自己的 PTE 时，KVM
合成的映射可能立即变旧。因此有 guest 页表写保护、写入模拟、shadow page
查找、unsync、sync、zap 等一整套机制。EPT/NPT
将两个地址空间交给硬件分别翻译，消除了普通 guest 的这类页表副本同步需求。

决定路径的是 `arch/x86/kvm/mmu/mmu.c` 中的 `kvm_init_mmu()`：先处理
`mmu_is_nested()`，普通路径再根据 `tdp_enabled` 选择 `init_kvm_tdp_mmu()` 或
`init_kvm_softmmu()`。`arch/x86/kvm/vmx/vmx.c` 中 `vmx_exec_control()` 在启用
EPT 时清除普通路径的 CR3 load/store 和 INVLPG exiting；nested
等条件仍可能要求拦截。

`kvm_mmu_sync_roots()`
的第一个判断也直接回答了旧笔记中的疑问：`root_role.direct`
为真就返回。因此调用栈里出现这个函数，并不说明本次正在同步 guest 的影子页表。

EPT 的代价也存在：TLB 未命中时，硬件的两阶段 page walk
会增加内存访问。它主要避免 guest 页表修改导致的退出与软件维护开销；大页、TLB 和
page walk cache 仍然重要。

原笔记：[shadow-page.md](mmu/shadow-page.md)、[page-track.md](mmu/page-track.md)、[paging_tmpl.md](mmu/paging_tmpl.md)、[hugepage.md](mmu/hugepage.md)。官方文档的
`Translation` 和 `Shadow pages` 章节也说明了 direct 与 shadow
的区别。[KVM MMU 文档](https://docs.kernel.org/virt/kvm/x86/mmu.html)

### 为什么 EPT 代码里仍然到处是 shadow 和 SPTE

`struct kvm_mmu_page`、`spt`、`spte` 和 `to_shadow_page()` 被复用来管理多种 KVM
页表。它们的名字不等于传统的 GVA 到 HPA Shadow Paging。

| 页表用途                          | 页表实际覆盖的翻译                        | CPU 中装载的位置                    | 软件管理方式                          |
| --------------------------------- | ----------------------------------------- | ----------------------------------- | ------------------------------------- |
| 普通分页 guest 的传统 shadow 页表 | GVA 到 HPA                                | VMX 的 `GUEST_CR3` 使用 shadow root | 传统 Shadow MMU                       |
| 普通 guest 的 EPT                 | GPA 到 HPA                                | `EPT_POINTER`                       | 传统 MMU 的 direct 路径或专用 TDP MMU |
| nested VMX 的 shadow EPT02        | L2 GPA 到 L0 HPA，由 EPT12 和 L0 映射合成 | 运行 L2 时的 `EPT_POINTER`          | Shadow MMU 的 guest MMU 上下文        |

`vmx_load_mmu_pgd()` 明确区分这两种硬件装载：`enable_ept` 时将 `root_hpa` 写入
`EPT_POINTER`；否则用它构造 `GUEST_CR3`。所以“`root_hpa` 总是 guest CR3”以及“EPT
与普通 x86 页表格式相同”都不准确，EPT 有自己的权限位和编码。

nested 还需要保留 shadow 的原因是：L2 的地址翻译比单层 guest
多一层，硬件通常不能把 L2 页表、L1 EPT12 和 L0 的内存映射全部独立走完。L0
因而需要合成 EPT02。源码入口是 `kvm_init_shadow_ept_mmu()` 和
`kvm_init_shadow_npt_mmu()`。这与在普通 guest 中关闭 EPT 后影子化 GVA
页表，是两种不同用途。

注意 `root_role.direct` 本身也不等于“硬件支持 EPT”：无 guest 分页时的 GPA 到 HPA
映射也可以是 direct。判断普通硬件 TDP 要结合 `tdp_enabled` 和 MMU 上下文。

原笔记：[nested.md](mmu/nested.md)、[mmu.rst.md](mmu/mmu.rst.md)、[kvm.md](kvm.md)。

### 传统 direct MMU 到专用 TDP MMU

这是 **EPT/NPT 已经存在之后，KVM 软件实现的另一次改进**。

| 普通非嵌套 guest 的配置             | 硬件翻译                        | 软件维护路径                                                              |
| ----------------------------------- | ------------------------------- | ------------------------------------------------------------------------- |
| `ept=0`                             | guest 开启分页时使用传统 shadow | `init_kvm_softmmu()`                                                      |
| `ept=1`，`tdp_mmu=0`                | 仍然是 EPT                      | `kvm_tdp_page_fault()` → `direct_page_fault()` → `direct_map()`           |
| `ept=1`，`tdp_mmu=1` 且实际允许启用 | 仍然是 EPT                      | `kvm_tdp_page_fault()` → `kvm_tdp_mmu_page_fault()` → `kvm_tdp_mmu_map()` |

对应 AMD 将 `ept` 换成 NPT 的配置；nested 要另看上下文。`kvm_configure_mmu()`
还会执行 `tdp_mmu_enabled = tdp_mmu_allowed && tdp_enabled`，所以关闭硬件 TDP
后，不能仅靠请求 `tdp_mmu=1` 得到专用 TDP MMU。

源码中两条慢 fault 路径的关键差别是：

- `direct_page_fault()` 持有 `mmu_lock` 写锁，再执行
  `direct_map()`，沿用传统页表管理和 rmap。
- `kvm_tdp_mmu_page_fault()` 持有 `mmu_lock` 读锁，再执行
  `kvm_tdp_mmu_map()`。`arch/x86/kvm/mmu/tdp_mmu.c` 中
  `__tdp_mmu_set_spte_atomic()` 使用 `try_cmpxchg64()` 更新 SPTE，RCU
  保护并发访问时的页表生命周期。

这样多个 vCPU 可以同时更新页表，减少多核 fault 时的大锁串行化。TDP MMU
还采用自己的 root、GFN range
遍历和回收方式，因此不能只概括为“把写锁改成读锁”。也不能称它完全无锁：root
失效等操作仍需写锁，还有专门的 spinlock。

TDP MMU 页表本身不维护传统的 memslot GFN 到 SPTE rmap，但一台 VM 可以同时有 TDP
页表和 nested shadow 页表。`arch/x86/kvm/mmu.h` 中 `kvm_memslots_have_rmaps()`
返回
`!tdp_mmu_enabled || kvm_shadow_root_allocated(kvm)`；`kvm_unmap_gfn_range()`
可以先清理传统 rmap，再清理 TDP 页表。所以“启用 TDP MMU 后整个 VM 就没有
rmap”不成立。

原笔记：[tdp_mmu.md](mmu/tdp_mmu.md)、[rmap.md](mmu/rmap.md)、[lock.md](lock.md)、[notifier.md](mmu/notifier.md)。

### Shadow 页表的 unsync

unsync 是 Shadow MMU 内部减少退出的优化。guest 修改 leaf PTE
后，在按架构要求使旧 TLB 映射失效之前，本来就不能要求 CPU 立即采用新映射。KVM
利用这个语义，在条件允许时让 leaf shadow page 暂时
unsync，之后在相应的同步点重新同步；不是每次 PTE 写都退出并逐项更新。

源码是 `mmu_try_to_unsync_pages()`、`kvm_unsync_page()` 和
`kvm_mmu_sync_roots()`。上层 guest
页表、被显式跟踪的页等仍需要写保护。新源码处理捕获到的页表写入使用
`kvm_mmu_track_write()`；旧笔记中的 `kvm_mmu_pte_write()`
名称已经变化，不能据此说页表写跟踪被删除。

unsync 和页表写跟踪当前都保留。它们与已经删除的 `KVM_HC_MMU_OP` 批量页表
操作 hypercall 是不同机制；后者的删除证据见前面的“已经移除的运行机制”。

## A/D bits 和脏页记录

### 软件访问跟踪到硬件 A/D bits

EPT/NPT、EPT A/D bits 和 PML 是不同能力。支持 EPT 并不自动保证支持 EPT
A/D，也不保证支持 PML。

以访问采样为例，`arch/x86/kvm/mmu/spte.c` 的 `mark_spte_for_access_track()`
有两条路径：

- A/D 可用时，清除 Accessed 位，之后由硬件访问重新置位。
- A/D 不可用时，保存部分权限并撤销访问权限，让后续 fault 表示页面再次被访问。

所以硬件 A/D 可以减少为采样访问状态而刻意制造的
fault；没有它仍然可以通过软件实现。guest 自己页表里的 A/D 和第二阶段 EPT 的
A/D，也要分开看。

### 写保护首写记录到 PML

迁移时需要知道“一轮之后哪些 guest RAM 又被写过”。经典办法是清除 SPTE
的写权限，guest 第一次写产生 fault，KVM 记脏并恢复允许的写权限。下轮再重新保护。

Intel PML 利用 EPT Dirty 位从 0 变成 1 的事件，把对应 GPA 写进硬件 PML
buffer，从而减少很多单纯用于脏页记录的首写退出。它不是每次写都记一条，也不是彻底消除
VM exit；buffer 满等事件仍要处理。

源码证据：

- `arch/x86/kvm/vmx/vmx.c` 的 `vmx_hardware_setup()` 只有在 EPT、EPT A/D 和 PML
  硬件能力都满足时才保留 `enable_pml`。
- `vmx_flush_pml_buffer()` 将缓冲里的 GPA 交给
  `kvm_vcpu_mark_page_dirty()`。运行 L2 等上下文还要看具体 PML 管理，不能把 host
  的 PML 路径直接套进去。
- `arch/x86/kvm/mmu/mmu.c` 的 `kvm_arch_mmu_enable_log_dirty_pt_masked()` 根据
  `cpu_dirty_log_size` 选择清 Dirty 位或写保护；即使启用 PML，另有 shadow
  页表保护要求的映射仍可能需要清 Writable 位。

**有 A/D 位不等于已经解决高效的迁移日志采集；没有 PML 也不妨碍使用软件 dirty
logging 进行热迁移。**

### Bitmap 到 manual protect 和 dirty ring

这里有两个维度，不能画成 `write protect → bitmap → PML → dirty ring`
这样的一条升级链。

| 维度                   | 可选机制                                     | 决定什么                   |
| ---------------------- | -------------------------------------------- | -------------------------- |
| 如何发现页面写脏       | 软件写保护 fault；硬件 PML                   | 脏页事件从哪里来           |
| 如何把事件提供给 VMM   | memslot dirty bitmap；per-vCPU dirty ring    | 用户态怎样收集和消费脏页   |
| 何时重新开启下一轮跟踪 | GET 自带清位和保护；manual CLEAR；ring RESET | 页被再次写脏后能否重新报告 |

因此软件首写 fault 可以填 ring，PML 也可以填 bitmap。`virt/kvm/kvm_main.c` 的
`mark_page_dirty_in_slot()` 明确根据 `dirty_ring_size`、当前 vCPU 和 bitmap
是否存在选择记录位置。

manual protect 将读取 bitmap 与重新保护分开，使 VMM 可以按发送批次安排
`KVM_CLEAR_DIRTY_LOG`，减少过早重新保护造成的额外 fault。dirty ring
用条目报告脏页，通常适合稀疏脏页，减少遍历整个 slot bitmap 的工作；VMM
必须回收条目并调用 `KVM_RESET_DIRTY_RINGS`，还要处理 ring 满。它不是无条件胜过
bitmap。

源码入口为 `virt/kvm/dirty_ring.c` 的 `kvm_dirty_ring_push()` 和
`kvm_dirty_ring_reset()`；ABI 定义在 `Documentation/virt/kvm/api.rst` 的
`KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2`、`KVM_CAP_DIRTY_LOG_RING`、`KVM_CAP_DIRTY_LOG_RING_WITH_BITMAP`
等章节。

原笔记：[ad.md](mmu/ad.md)。接口语义可对照
[KVM API 文档](https://docs.kernel.org/virt/kvm/api.html)。

## Interrupt window 和 APIC 加速

### Interrupt window 到 Virtual Interrupt Delivery

interrupt window 自己也是硬件提供的优化：KVM 不必不停退出检查 guest 的 IF 和
interrupt shadow，而是设置控制位，在 guest 可以接受可屏蔽中断时得到通知。

软件注入路径大致是：

```text
有待处理 IRQ
  -> 能注入时：设置 VM-entry injection
  -> 因 IF、STI/MOV SS interrupt shadow 等不能注入时：请求 irq window
  -> 窗口打开：VM exit
  -> 清除窗口请求，重新评估事件并注入
```

窗口条件和 APIC 优先级是不同问题。TPR/PPR 阻止某个向量投递时，还要看 LAPIC
的优先级判断和 CR8/TPR 相关机制，不能把 interrupt window
简化成“任何中断阻塞都用它解决”。

Virtual Interrupt Delivery 把普通 LAPIC
虚拟中断的待处理状态和优先级判定交给硬件。在满足投递条件时由 CPU
交付，不必为了窗口打开再退出给 KVM 做注入。它仍然遵守 guest 的 IF、interrupt
shadow 和优先级规则，没有强行穿过屏蔽状态。

最直接的代码证据在 `arch/x86/kvm/irq.c` 的 `kvm_cpu_has_injectable_intr()`：

```c
if (kvm_cpu_has_extint(v))
    return 1;

if (!is_guest_mode(v) && kvm_vcpu_apicv_active(v))
    return 0;

return kvm_apic_has_interrupt(v) != -1;
```

这里返回 0 的含义是“普通 LAPIC IRQ
不需要这条软件注入路径”，不是“没有中断”。判断特意先检查 ExtInt，而且要求当前不在
L2 guest mode。

源码链：`arch/x86/kvm/x86.c` 的 `kvm_check_and_inject_events()` →
`enable_irq_window`；VMX 的 `vmx_enable_irq_window()` 设置
`CPU_BASED_INTR_WINDOW_EXITING`；`handle_interrupt_window()` 清除该位并请求
`KVM_REQ_EVENT`。这些实现当前都存在。

### Virtual Interrupt Delivery 与 Posted Interrupt 的分工

这两项分别解决不同的问题：

| 机制                       | 主要解决的问题                                                              |
| -------------------------- | --------------------------------------------------------------------------- |
| Virtual Interrupt Delivery | 目标 vCPU 已有 pending virtual IRQ 时，CPU 何时以及怎样交付给 guest         |
| Posted Interrupt           | 新 IRQ 怎样在目标 vCPU 运行期间写入待处理状态并通知它，减少为通知而强制退出 |
| IPI virtualization         | guest 发送 IPI 时，如何减少发送方的退出和软件投递                           |

软件产生 PI 的证据是 `vmx_deliver_posted_interrupt()` 调用
`arch/x86/kvm/vmx/common.h` 的 `__vmx_deliver_posted_interrupt()`：设置 PID 的
PIR 位和 ON 位，然后根据目标状态通知或唤醒目标 vCPU。CPU 消费 PI 时将 PIR 汇入
virtual APIC 状态；KVM 在必要的软件同步路径上也通过 `vmx_sync_pir_to_irr()`
同步。

因此 **PI 不只属于 VT-d**。KVM 可以软件 posting；设备直通时，支持的 IOMMU
也能作为 posting 的生产者。`arch/x86/kvm/vmx/posted_intr.c` 的
`vmx_pi_update_irte()` 把目标 PID 和向量传给 `irq_set_vcpu_affinity()`，是设备
IRQ 与 PI 结合的入口。Intel 的说明也明确允许软件产生 posted
interrupt。[Intel APICv 说明](https://edc.intel.com/content/www/us/en/design/ipla/software-development-platforms/client/platforms/alder-lake-desktop/12th-generation-intel-core-processors-datasheet-volume-1-of-2/007/intel-apic-virtualization-technology-intel-apicv/)

目标已经被调度出去或阻塞时，还要唤醒、调度和重入；PI
无法把这个工作全部省掉。发送方的 IPI 写入也不自动因为目标支持 PI
就变成无退出，要另看 IPI virtualization。

### 哪些场景仍会用到 Interrupt window

1. **ExtInt**：例如 PIC 输出或用户态 `KVM_INTERRUPT`
   对应的外部中断，不能直接套用普通 LAPIC pending vector 的硬件 VID
   路径。`kvm_cpu_has_injectable_intr()` 为它优先返回真。
2. **用户态请求窗口**：`dm_request_for_irq_injection()` 检查
   `run->request_interrupt_window` 和 `!pic_in_kernel()`，`vcpu_enter_guest()`
   结合 `kvm_cpu_accept_dm_intr()` 决定请求。它与“内核有一个已经阻塞的
   IRQ”不是同一个来源。
3. **nested**：给 L1 的中断在 L2 运行期间，可能要合成退出给 L1 或按 nested
   规则注入；不能假定普通 L1 的 VID 路径直接适用。L1 自己对 L2
   配置的窗口退出也必须被虚拟化。
4. **加速不可用或被抑制**：无相关能力、用户态 LAPIC、调试阻止 IRQ、某些 Hyper-V
   AutoEOI 配置等，会影响实际启用状态。

split irqchip 需要特别纠正：**LAPIC 在内核，PIC/IOAPIC 在用户态**。它可以使用
APICv，不能归类为“QEMU 模拟 LAPIC，因此所有中断都需要窗口”。应继续按具体路由区分
MSI/LAPIC IRQ 和用户态 ExtInt。源码文档是 `Documentation/virt/kvm/api.rst` 的
`KVM_CAP_SPLIT_IRQCHIP`
章节。[KVM split irqchip 接口](https://docs.kernel.org/virt/kvm/api.html#kvm-cap-split-irqchip)

原笔记：[interrupt-window.md](interrupt-window.md)、[event-delivery.md](event-delivery.md)、[apicv.md](vmx/apicv.md)、[legacy.md](vmx/legacy.md)。

### CR8 和 TPR 到硬件 APIC 虚拟化

CR8 表示 TPR 的优先级部分。早期模拟 CR8 或访问 APIC
TPR，容易因频繁调优先级产生退出。TPR shadow 把这部分状态放在硬件可以使用的
virtual APIC page 中，减少 CR8/TPR 访问退出；TPR threshold
仍可在特定优先级变化时通知 KVM。

源码里 `vmx_exec_control()` 在使用 `CPU_BASED_TPR_SHADOW` 时清除 CR8 load/store
exiting；`vmx_update_msr_bitmap_x2apic()` 的注释明确说，即使没有使用
VID，也可以虚拟化 TPR 读写。因此 TPR shadow 不能简单等同于完整 APICv。

完整 APICv 可以进一步处理 APIC 读、EOI 和中断优先级状态。`arch/x86/kvm/lapic.c`
的 `kvm_lapic_update_cr8_intercept()` 在 `apicv_active`
时直接返回，就是早期软件优化被后续硬件接管的例子。但它只覆盖硬件支持的操作，不是全部
APIC 寄存器访问从此都不退出。

这里三个名字要区分：

- QEMU 的 VAPIC 优化和 KVM 的 `vapic_addr` 是 guest 内存共享的旧优化接口。
- VMCS 的 `VIRTUAL_APIC_PAGE_ADDR` 是硬件 virtual APIC page。
- APICv 是一组硬件加速能力，guest 使用 x2APIC 也只是接口模式，不能证明 APICv
  正在使用。

原笔记：[cr.md](cr.md)、[qemu-vapic.md](vmx/qemu-vapic.md)、[apicv.md](vmx/apicv.md)。

### 软件 EOI 到 PV EOI 和硬件 EOI

PV EOI 让支持它的 guest 在安全场景下清除共享内存中的标志，代替一次 APIC EOI
写，host 随后完成相应处理。硬件 APICv 则可以直接处理虚拟 EOI，省掉常见的 EOI
访问退出。二者是减少同类开销的不同办法。

但 level-triggered IOAPIC 等场景可能需要 EOI 反馈给软件。APICv 的
`EOI_EXIT_BITMAP` 可以要求指定向量退出，`handle_apic_eoi_induced()` 调用
`kvm_apic_set_eoi_accelerated()`；split irqchip 还可能将 IOAPIC EOI 报给用户态。

PV EOI 同样有条件。`apic_sync_pv_eoi_to_guest()` 遇到 pending IRR、没有可用 ISR
cache 或需要通知 IOAPIC 等情况就不启用本次 PV EOI。它没有以 `enable_apicv`
为条件一律返回。函数调用还可能存在，只是收益和实际工作量取决于状态。

所以“启用 APICv 后 `KVM_REQ_EVENT` 不再发生”“所有 EOI
软件函数都不会再调用”都不成立。`KVM_REQ_EVENT` 还承担异常、NMI 等其他事件，硬件
EOI 也保留明确的退出路径。

原笔记：[pv-eoi.md](features/pv-eoi.md)、[apicv-yes.md](vmx/apicv-yes.md)。

### IPI 的软件投递到 PV IPI 和 IPI virtualization

普通软件路径中，guest 写 ICR 后退出，KVM 解析目标、找到 vCPU
并投递。这包括发送方退出和目标方通知两个环节。

PV IPI 通过 `KVM_HC_SEND_IPI` 一次提供多个目标，减少多目标发送的退出次数。guest
侧在 `arch/x86/kernel/kvm.c` 的 `kvm_send_ipi_mask()`；host 侧在
`____kvm_emulate_hypercall()` 和 `arch/x86/kvm/lapic.c` 的
`kvm_pv_send_ipi()`。它仍然需要 hypercall 退出。

Intel IPI virtualization 进一步让符合硬件规则的 IPI 由硬件处理。当前
`vmx_update_msr_bitmap_x2apic()` 在相应模式且 `enable_ipiv` 时放开 ICR MSR
拦截；`vmx_tertiary_exec_control()` 和 `vmx_refresh_apicv_exec_ctrl()` 管理
`TERTIARY_EXEC_IPI_VIRT`。它依赖 CPU 能力和实际 APICv 状态，不能只看 guest 有
x2APIC 就认定开启。

这也解释了“APICv 开了，为什么 IPI 仍有 exit”：可能目标通知已被 PI 优化，而发送方
ICR 写仍由软件处理。

### AMD AVIC 和 x2AVIC

AVIC/x2AVIC 也优化 APIC 访问和中断投递，但不是 Intel 控制位的另一种拼写。KVM
共用 `apicv_active` 等状态名称，需要继续查看 VMX 或 SVM 的具体实现。

`arch/x86/kvm/svm/svm.c` 中的 `svm_enable_irq_window()` 有很有价值的反例：AVIC
无法沿自身路径投递 ExtInt，而且启用 AVIC 时硬件忽略这里用来检测窗口的 V_IRQ。KVM
因此暂时抑制 AVIC，等待窗口，再处理
ExtInt。这表明新机制与旧窗口机制确实在同一套实现里协作。

当前 `arch/x86/kvm/svm/avic.c` 的 `avic_want_avic_enabled()` 在 auto 模式下检查
x2AVIC 和 Zen4 或更新家族，并另外检查 NPT 与硬件条件。旧笔记的“AVIC
默认关闭”不适合作为跨版本结论。`APICV_INHIBIT_REASON_NESTED`、`IRQWIN`、`PIT_REINJ`
等部分原因只属于 AMD，不能全部套到 Intel。

原笔记：[avic.md](svm/avic.md)、[svm.md](svm/svm.md)。

## TLB 缓存和 PV TLB flush

Intel VPID 和 AMD ASID
给虚拟机翻译上下文打标签，减少切换时不必要的缓存丢弃。**它们没有免除页表修改后的失效责任。**

VMX 的 `vmx_flush_tlb_all()` 特别说明：启用 EPT 后，不论 VPID
如何，都必须按需执行 INVEPT；INVVPID 并不保证使 EPT 的 guest-physical
映射缓存失效。`vmx_flush_tlb_current()` 也在 EPT root flush 和 VPID context
flush 之间明确区分。

| 要维护的状态                             | 对应工作                                                            |
| ---------------------------------------- | ------------------------------------------------------------------- |
| guest 第一阶段地址空间的翻译             | guest 自己的 INVLPG、CR3、INVPCID 等语义，以及 KVM 必要的上下文失效 |
| EPT/NPT 的 GPA 到 HPA 映射               | host 改变映射、权限或回收页面后，KVM 必须使相应缓存失效             |
| guest 向其他 vCPU 发起远程 TLB shootdown | 普通 IPI，或者 PV TLB flush 等协作机制                              |

PV TLB flush 解决第三项中的调度问题：普通 shootdown 可能同步等待一个已经被 host
换出的 vCPU。`arch/x86/kernel/kvm.c` 的 `kvm_flush_tlb_multi()`
给被抢占的目标设置 `KVM_VCPU_FLUSH_TLB`，把它从本次 IPI mask 移除；host 的
`record_steal_time()` 在恢复执行前兑现 flush。

因此 PV TLB flush 和 VPID/EPT 是互补关系。前者减少对未运行 vCPU
的同步等待，后者维护和复用硬件翻译缓存，不能互相替代。

原笔记：[tlb-flush-virt.md](mmu/tlb-flush-virt.md)、[tlb-flush.md](mmu/tlb-flush.md)、[pv-tlb-flush.md](features/pv-tlb-flush.md)。

## 用户态往返和 I/O 通知

分析这部分时，要区分 **硬件 VM exit** 和 **`KVM_RUN` 返回用户态**。一次 VM exit
可以被 KVM 处理后直接重入，未必返回 QEMU。

### 用户态 irqchip 到内核 irqchip 和 irqfd

内核 irqchip 将 LAPIC、以及按模式选择的 PIC/IOAPIC 模拟留在 KVM
中，减少控制器模拟的用户态往返，也给 APICv/AVIC 提供所需的内核 LAPIC 状态。split
irqchip 只将部分控制器留在用户态。

irqfd 把 eventfd 事件接到 KVM 的中断路由，后端不用每次通过普通注入 ioctl
通知。`virt/kvm/eventfd.c` 的 `irqfd_wakeup()`
尝试在当前上下文注入，不能直接处理时调度 `irqfd_inject()`。

它减少的是后端到 KVM 的通知成本；APICv/AVIC 减少的是目标 vCPU 接受通知和 guest
中断投递的成本。它们可以组合。irqfd 的生产者也可以在用户态，因此注册 irqfd
本身不等于整个设备已经变成内核后端。

### 逐笔 MMIO 到 coalesced I/O 和 ioeventfd

coalesced I/O 将允许延迟的寄存器写记录到共享
ring，之后由用户态批量消费。`virt/kvm/coalesced_mmio.c` 的
`coalesced_mmio_write()` 在 ring 有空间时记录地址和值，ring
满等情况返回不支持，回到常规处理。它保留了寄存器写序列，不是直接完成所有设备操作。

ioeventfd 更适合队列 doorbell：VMM 注册地址、宽度和可选数据匹配条件；KVM
遇到匹配的写，在 `ioeventfd_write()` 中调用 `eventfd_signal()`。配合 vhost
等后端，可以避免每次队列通知都让 vCPU 回用户态设备模型。

二者通常仍由 guest I/O 导致的硬件退出进入
KVM；主要省掉的是返回用户态及重新进入的成本。普通 MMIO
读取、要求即时语义的写、未注册或未匹配的通知仍然要走原路径。MMIO SPTE cache
又是另一项优化：减少重复地址识别，不等于设备访问已由硬件直通。

原笔记：[mmio.md](mmio.md)、[qemu.md](qemu.md)。

## Guest 运行状态和定时器

### Intel 受限 guest 到 unrestricted guest

早期 VMX 对 CR0.PE/PG 等状态有约束，guest 的实模式、分页关闭状态需要 KVM
绕行。例如 vm86 状态管理、为部分场景建立 identity page table，或者使用指令模拟。

unrestricted guest 与 EPT 配合，使这类 guest
状态可以直接运行，减少绕行路径。`vmx_hardware_setup()` 在缺少硬件能力或没有 EPT
时关闭 `enable_unrestricted_guest`；`vmx_set_cr0()`、`vmx_set_tss_addr()` 和
`vmx_load_mmu_pgd()` 都保留了受限 guest 的分支。

但 unrestricted guest 没有替代整个 `emulate.c`。MMIO、被拦截的指令、nested
要求和异常语义仍可能需要模拟。

原笔记：[emulate.md](emulate/emulate.md)、[cr.md](cr.md)、[vmx.md](vmx/vmx.md)。

### Host hrtimer 到 VMX preemption timer 和 posted timer IRQ

VMX preemption timer 可以在 vCPU 运行时，到 guest LAPIC deadline 触发 VM
exit，再由 KVM 处理定时器。这样减少某些 host hrtimer
和异步通知路径，但**到期仍然退出**。

源码是 `vmx_set_hv_timer()`、`handle_preemption_timer()`，以及
`arch/x86/kvm/lapic.c` 的 `start_hv_timer()`、`start_sw_timer()` 和
`kvm_lapic_switch_to_sw_timer()`。目标不能用该计时方式、期限超出硬件可表示范围，或
vCPU 的运行状态改变时，需要软件 timer 路径。

另一个选择是软件 timer 到期后通过 PI 通知 guest，减少运行中目标的退出。当前
`kvm_can_post_timer_interrupt()` 要求 `pi_inject_timer`、实际 APICv
active，并允许 guest 内 HLT 或 MWAIT；`kvm_can_use_hv_timer()`
则显式避开部分这类情况。

所以它们是有条件的不同选择，不是“硬件 timer 出现以后 hrtimer 废弃”。guest
TSC-deadline 接口也不等于把 host 实体 LAPIC timer 直接交给 guest。

原笔记：[exit-reason.md](exit-reason.md)、[qemu-nullblk-performance.md](fun/qemu-nullblk-performance.md)、[KVM 定时器笔记](../kernel/time/kvm-timer.md)。

## 超配下的自旋和等待

### PAUSE 退出到 PLE 和 PV spinlock

每次 PAUSE 都退出太贵，完全不退出又可能让等待被换出锁持有者的 vCPU 空转。Intel
PLE 和 AMD pause filter 用硬件识别持续自旋，再给 KVM 一次调整运行机会的入口。

VMX 的 `vmx_exec_control()` 清除普通的
`CPU_BASED_PAUSE_EXITING`；`handle_pause()` 的注释说明当前 KVM 使用 PLE，再调用
`virt/kvm/kvm_main.c` 的 `kvm_vcpu_on_spin()`。这个路径尝试让合适的其他 vCPU
运行，不保证一定找到锁持有者。

PV spinlock 则由 guest 直接知道自己正在等锁，通过 `kvm_wait()` 停等，并通过
`KVM_HC_KICK_CPU` 唤醒。它减少超配场景下的盲目自旋，不保证使 PLE
从此不再出现。`kvm_spinlock_init()` 还会在单 vCPU、`nopvspin`、专用 CPU
的相关提示等条件下选择不启用 PV spinlock。

原笔记：[ple.md](ple.md)、[pv-spinlock.md](features/pv-spinlock.md)、[pv-sched-yield.md](features/pv-sched-yield.md)。

### HLT 后阻塞到 halt polling

host halt polling 在 guest
已经退出后，先短暂检查唤醒条件，避免很快到来的事件造成一次完整的阻塞和唤醒。`kvm_vcpu_halt()`
根据等待情况调整 polling 时长，最终仍可调用 `kvm_vcpu_block()`。

它用 host CPU 时间换短等待延迟；这是负载相关的策略，不是越长越高效。PV
spinlock、guest halt polling 和 host halt polling 也不能当成同一个机制。

若某个 HLT 在硬件中直接等待，没有退出到 KVM，本次就不会走 host 的
`kvm_vcpu_halt()`。但配置、其他等待路径及是否能在 guest
等待状态正确接收事件，仍决定软件阻塞路径是否需要保留。APICv 本身不能推出“所有
HLT 都不退出”。

原笔记：[apicv-yes.md](vmx/apicv-yes.md)、[host-freq.md](fun/host-freq.md)、[debugfs.md](debugfs.md)。

## ARM64 的 nVHE 和 VHE

VHE 与 x86 EPT、APICv 是不同层面的能力。nVHE 中 host 内核在 EL1，KVM 的 hyp 在
EL2；VHE 中 host 内核可以运行在 EL2，减少一部分 host 与 hyp 切换和状态管理成本。

当前入口是 `arch/arm64/kvm/arm.c` 的 `kvm_arm_vcpu_enter_exit()`，通过
`kvm_call_hyp_ret(__kvm_vcpu_run, vcpu)` 进入相应实现。具体对照
`arch/arm64/kvm/hyp/vhe/switch.c` 与 `arch/arm64/kvm/hyp/nvhe/switch.c` 的
`__kvm_vcpu_run()`。

旧笔记的“guest 发起系统调用时，从 Guest EL1/EL0 到 Host EL1 再到
EL2”需要纠正：普通 guest 应用的系统调用通常从 guest EL0 进入 guest EL1，不需要到
host；真正 trap 到 hypervisor 的事件直接进入 EL2。nVHE 的额外成本主要来自 host
EL1 与 EL2 hyp 的交互及上下文管理。

nVHE 也没有因为 VHE 出现而成为废代码。当前 `early_kvm_mode_cfg()` 仍处理 nVHE 和
protected 模式；pKVM 用独立 hyp 隔离 host，并不追求把 host 与 hyp
合并。这是性能与隔离目标不同的设计选择。

原笔记：[vhe.md](aarch64/vhe.md)、[code-overview.md](aarch64/code-overview.md)、[aarch64.md](aarch64/aarch64.md)。ARM
的 VGIC、Stage-2 和 timer 另读相应目录，不要直接套用 Intel 的控制位。

## 读旧笔记时优先纠正的判断

| 旧笔记中容易形成的判断                       | 当前应如何理解                                                   |
| -------------------------------------------- | ---------------------------------------------------------------- |
| `tdp_mmu=0` 就没有 EPT                       | 只关闭专用 TDP MMU 软件实现；只要 EPT 仍开着，就是硬件两阶段翻译 |
| `to_shadow_page()` 证明正在用 Shadow Paging  | 这是共用的页表管理结构，要看页表映射和 MMU role                  |
| `root_role.direct=1` 就证明有 EPT            | 没开启 guest 分页等场景也可以 direct，要结合上下文               |
| 新 TDP MMU 只是更换锁                        | 页表组织、遍历、原子更新和生命周期管理也不同                     |
| 开了 APICv 就不用 irq window                 | 普通非 nested LAPIC IRQ 可绕过；ExtInt 等仍保留                  |
| Posted Interrupt 必须由 VT-d 产生            | KVM 软件也可以 posting；IOMMU 是设备直通时的另一种生产者         |
| split irqchip 的 LAPIC 在用户态              | LAPIC 在内核，PIC/IOAPIC 在用户态                                |
| 所有用户态模拟设备 IRQ 都不能用 APICv        | 要看路由；模拟设备的 MSI 也可以进入内核 LAPIC 加速路径           |
| APICv 打开后没有 `KVM_REQ_EVENT` 或 EOI exit | 其他事件继续使用该 request；指定向量仍可要求 EOI exit            |
| 有 A/D 位就不需要软件 dirty logging          | A/D、PML、日志采集 ABI 和重新保护是不同问题                      |
| PML 和 dirty ring 谁替代谁                   | 前者发现脏页，后者向用户态交付条目，可以组合                     |
| VPID、PV TLB flush、EPT flush 是同一类升级   | 分别涉及缓存标签、远程等待和第二阶段映射一致性                   |

源码名称变化也应单独记录。例如旧 `kvm_mmu_pte_write()` 对应当前的
`kvm_mmu_track_write()`，旧调用栈里 `static_call(kvm_x86_...)` 的分发在当前常用
`kvm_x86_call(...)` 表达。改名或换封装本身不等于机制被替换。

## 建议的源码阅读顺序

先沿当前常用路径建立主线，再带着明确条件看保留机制：

1. **普通内存访问**：`kvm_init_mmu()` → `kvm_tdp_page_fault()` →
   `kvm_tdp_mmu_page_fault()` → `kvm_tdp_mmu_map()`；然后看
   `kvm_unmap_gfn_range()` 和 SPTE 生命周期。
2. **需要 Shadow MMU
   的原因**：`init_kvm_softmmu()`、`kvm_init_shadow_ept_mmu()`、`mmu_try_to_unsync_pages()`、`kvm_mmu_track_write()`，把普通
   shadow 与 nested shadow 分开。
3. **普通 LAPIC
   IRQ**：`vmx_deliver_posted_interrupt()`、`__vmx_deliver_posted_interrupt()`、`vmx_sync_pir_to_irr()`，然后回头看
   `kvm_cpu_has_injectable_intr()` 为什么绕过软件路径。
4. **仍需窗口的事件**：`kvm_check_and_inject_events()`、`vmx_enable_irq_window()`、`handle_interrupt_window()`；AMD
   再看 `svm_enable_irq_window()` 的 AVIC 抑制。
5. **迁移脏页**：`kvm_arch_mmu_enable_log_dirty_pt_masked()`、`vmx_flush_pml_buffer()`、`mark_page_dirty_in_slot()`、`kvm_dirty_ring_reset()`，分别跟踪发现、交付和重新开启跟踪。

`emulate.c`、`make_request`、MMU notifier、memslot、nested
状态机仍是当前核心机制，不能因为某条热点路径有硬件加速，就整体归为历史包袱。

## 做一个总结
记录那些虚拟化的内容已经被硬件实现了:

- kvm-clock : MSR TSC AUX 的
- 中断虚拟化 : APICv

其实可以去检查初始化的 feature 的判断，大致可以知道有那些硬件 offload

带着这个视角来分析代码，理解起来会更加容易一些

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
