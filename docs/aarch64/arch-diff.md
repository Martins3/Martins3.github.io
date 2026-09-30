# aarch64 和 x86 的简单对比
<!-- 716982c0-d3de-47d7-96e6-b89782b12079 -->

一共从一下几个角度来说明下 aarch64 和 x86-64
- cpu model 的实现
- kvm 实现的差别
- 寄存器对比
- tlb 和 cache 的 flush
- 物理地址空间 / 虚拟机空间的划分
- kernel stack 的使用

说明:
1. 寄存器对比完全 ChatGPT 生成的，我删掉了其中部分内容，感觉其中的结果非常不错，直接引用了

总体来说 在 pc 和 server 领域
但是x86 整个体系比较零散了
但是 x86 的功能更加完善

## 1. x86-64 寄存器分类

这里不考虑 32-bit legacy mode，只按现代 x86-64 + VMX/KVM 视角。

### A. 通用执行状态

| 类别                     | 典型寄存器                               | 说明                                                                     |
| ------------------------ | ---------------------------------------- | ------------------------------------------------------------------------ |
| GPR                      | `RAX RBX RCX RDX RSI RDI RBP RSP R8-R15` | 通用整数寄存器                                                           |
| IP                       | `RIP`                                    | 指令指针                                                                 |
| FLAGS                    | `RFLAGS`                                 | 条件码、中断开关、方向位等                                               |
| Segment visible selector | `CS DS ES SS FS GS`                      | long mode 下大多被弱化，但 `CS`、`SS`、`FS`、`GS` 仍重要                 |
| Segment hidden state     | base、limit、attributes                  | 架构上存在，VMCS 里也要保存 guest/host selector/base/limit/access-rights |
| FPU                      | `ST0-ST7`、x87 control/status/tag        | x87 历史包袱仍属于架构状态                                               |
| SIMD/FP                  | `XMM/YMM/ZMM`、`MXCSR`                   | SSE/AVX/AVX-512 状态，数量取决于 feature                                 |
| AVX-512 mask             | `K0-K7`                                  | AVX-512 opmask                                                           |
| AMX                      | tile config / tile data                  | 新一些的扩展状态，靠 XSAVE 体系管理                                      |
| Extended state control   | `XCR0`、`IA32_XSS`                       | 控制 XSAVE 管理哪些扩展状态                                              |

### B. Control registers

现代 x86-64 主要看：

| 寄存器    | 用途                                                            |
| --------- | --------------------------------------------------------------- |
| `CR0`     | 基本 CPU 控制：paging、write protect、cache disable 等          |
| `CR2`     | page fault linear address                                       |
| `CR3`     | 当前地址空间根页表，x86-64 下通常指向 PML4/PML5，含 PCID 相关位 |
| `CR4`     | 一堆扩展开关：PAE、PGE、PSE、OSXSAVE、SMEP、SMAP、UMIP、LA57 等 |
| `CR8`     | task priority register，和 APIC interrupt priority 相关         |
| `CR1`     | 不可用/保留                                                     |
| `CR5-CR7` | 不作为现代通用架构控制寄存器使用                                |

你写的 `cr0 cr1` 里，**`CR1` 是典型“编号存在但保留”的坑**。实际内核/KVM
里经常处理的是 `CR0/CR2/CR3/CR4/CR8`。

### C. Debug registers

| 寄存器    | 用途                                    |
| --------- | --------------------------------------- |
| `DR0-DR3` | hardware breakpoint address             |
| `DR6`     | debug status                            |
| `DR7`     | debug control                           |
| `DR4/DR5` | legacy/reserved，不作为正常现代状态使用 |

对应到 KVM，就是 guest hardware breakpoint/watchpoint 状态。

### D. Descriptor/table/task 相关寄存器

| 寄存器 | 用途                                   |
| ------ | -------------------------------------- |
| `GDTR` | GDT base/limit                         |
| `IDTR` | IDT base/limit                         |
| `LDTR` | LDT selector + hidden descriptor state |
| `TR`   | TSS selector + hidden descriptor state |

即使 long mode 弱化 segmentation，IDT/GDT/TSS 仍是 x86 异常、中断、栈切换、权限切换的重要部分。

### E. MSR

MSR 是 x86 非常特殊的设计。它通过 `RDMSR/WRMSR` 访问，数量很多，和 CPU
vendor/model/feature 强相关。Intel 官方手册 Volume 4 就是专门描述 model-specific
registers。([Intel][1])

常见 MSR 类别：

| 类别             | 例子                                                                        | 说明                             |
| ---------------- | --------------------------------------------------------------------------- | -------------------------------- |
| 基础架构控制     | `IA32_EFER`                                                                 | long mode、NX、SYSCALL/SYSRET 等 |
| syscall/sysenter | `IA32_STAR`、`IA32_LSTAR`、`IA32_FMASK`、`IA32_SYSENTER_CS/EIP/ESP`         | 系统调用入口                     |
| FS/GS base       | `IA32_FS_BASE`、`IA32_GS_BASE`、`IA32_KERNEL_GS_BASE`                       | TLS/per-cpu 常用                 |
| 时间             | `IA32_TSC`、`IA32_TSC_ADJUST`、`IA32_TSC_AUX`                               | TSC、vDSO、虚拟化时间            |
| APIC/x2APIC      | `IA32_APIC_BASE`、x2APIC MSRs                                               | local APIC 控制                  |
| 内存类型         | `IA32_PAT`、MTRR MSRs                                                       | cacheability/memory type         |
| 性能计数         | `IA32_PERFEVTSELx`、`IA32_PMCx`、`IA32_FIXED_CTRx`、`IA32_PERF_GLOBAL_CTRL` | PMU                              |
| 安全缓解         | `IA32_SPEC_CTRL`、`IA32_PRED_CMD`、`IA32_ARCH_CAPABILITIES`                 | Spectre/Meltdown 后大量出现      |
| VMX capability   | `IA32_VMX_BASIC`、`IA32_VMX_PINBASED_CTLS`、`IA32_VMX_PROCBASED_CTLS` 等    | 告诉 VMM 支持哪些 VMX 控制       |
| MCE/RAS          | `IA32_MCG_*`、`IA32_MC*_STATUS`                                             | machine check                    |
| Debug/trace      | `IA32_DEBUGCTL`、Intel PT MSRs                                              | 调试/trace                       |
| XSAVE            | `IA32_XSS`                                                                  | supervisor xstate                |

### F. CPUID

x86 CPU model 很大一部分是：

```text
CPUID leaves + MSR availability + control bits + microcode behavior
```

QEMU/KVM 做 x86 CPU model 时，核心就是组合：

```text
CPUID feature bits
MSR exposure/filtering
VMX controls
XSAVE state size/layout
cache/topology leaves
```

所以在虚拟化里，`CPUID` 虽然不是 register file，但它和 CPU model 绑定非常紧。

### G. VMCS：也不是普通寄存器

VMCS 是 VMX 的核心。Intel 手册明确说，VMX non-root operation 和 VMX transitions
由 **Virtual-Machine Control Structure, VMCS** 控制；VMCS 通过 VMCS pointer
选择，并用 `VMREAD/VMWRITE/VMCLEAR/VMPTRLD/VMPTRST`
等指令访问。([Intel CDRD][3])

VMCS 字段大致分成：

| VMCS 类别              | 例子                                                                                                              | 说明                      |
| ---------------------- | ----------------------------------------------------------------------------------------------------------------- | ------------------------- |
| Guest-state fields     | guest `CR0/CR3/CR4`、`RSP/RIP/RFLAGS`、segment state、`GDTR/IDTR`、`SYSENTER` MSRs、`IA32_EFER` 等                | VM entry 后加载给 guest   |
| Host-state fields      | host `CR0/CR3/CR4`、host `RSP/RIP`、host segment selectors、host `IA32_EFER` 等                                   | VM exit 后恢复给 VMM      |
| Execution controls     | pin-based、primary/secondary processor-based controls、exception bitmap、I/O bitmap、MSR bitmap、CR masks/shadows | 控制 guest 哪些行为 trap  |
| VM-exit controls       | exit 时是否保存/加载 MSR、host address size、ack interrupt 等                                                     | 控制 VM exit 行为         |
| VM-entry controls      | entry 时是否加载 EFER、PAT、debug controls、guest IA-32e mode 等                                                  | 控制 VM entry 行为        |
| Read-only VM-exit info | exit reason、exit qualification、guest linear/physical address、instruction length/info                           | 类似 Arm 的 syndrome 信息 |
| EPT/VPID 相关          | `EPTP`、`VPID`、EPT violation info                                                                                | 二阶段地址转换和 TLB tag  |

Intel VMX 把 VMM 和 guest 分成 VMX root / VMX non-root，VM entry 进入 guest，VM
exit 回到 VMM；这和 Arm 的 EL2/EL1
模型在目标上相似，但机制差别很大。([Intel CDRD][3])

---

## 2. AArch64 寄存器分类

AArch64 的核心思路是：

```text
普通执行寄存器 + PSTATE + 大量 System Registers + Exception Level 分层
```

Arm A-profile 的异常/权限模型用 `EL0 EL1 EL2 EL3` 表示，Arm 文档也明确把 AArch64
exception model、virtualization、stage-2 translation、trapping
等作为核心内容。([Arm][2])

### A. 通用执行状态

| 类别          | 寄存器                                         | 说明                                                    |
| ------------- | ---------------------------------------------- | ------------------------------------------------------- |
| GPR           | `X0-X30`                                       | 31 个 64-bit 通用寄存器                                 |
| 32-bit view   | `W0-W30`                                       | `Xn` 的低 32 位视图                                     |
| link register | `X30 / LR`                                     | 函数返回地址                                            |
| stack pointer | `SP`，以及 `SP_EL0/SP_EL1/SP_EL2` 等           | Arm 有按 EL 区分的 SP                                   |
| zero register | `XZR/WZR`                                      | 读为 0，写丢弃；编码上和 SP 有重叠语义                  |
| PC            | `PC`                                           | 不是普通 GPR，不能像 x86 `RIP` 那样直接当普通寄存器读写 |
| PSTATE        | `NZCV`、`DAIF`、`PAN`、`UAO`、`DIT`、`SSBS` 等 | 类似 `RFLAGS` 的一部分 + interrupt mask + 安全状态控制  |

### B. FP/SIMD/SVE/SME

| 类别              | 寄存器                      | 说明                          |
| ----------------- | --------------------------- | ----------------------------- |
| FP/NEON           | `V0-V31`                    | 32 个 128-bit SIMD/FP 寄存器  |
| views             | `Bn/Hn/Sn/Dn/Qn`            | 同一个 `Vn` 的不同宽度视图    |
| FP control/status | `FPCR`、`FPSR`              | 浮点控制和状态                |
| SVE               | `Z0-Z31`、`P0-P15`、`FFR`   | scalable vector，长度实现相关 |
| SME               | `ZA`、`SVCR`、`SMCR_ELx` 等 | matrix/tile 类扩展            |

大致对应 x86 的 `XMM/YMM/ZMM + MXCSR + opmask + AMX`，但 SVE/SME 是 Arm 自己的
scalable/vector/matrix 模型。

### C. AArch64 System Registers：Arm 版“CR + MSR + VMCS 控制面”的大集合

AArch64 里大量系统状态通过 `MRS/MSR` 访问。命名上通常是：

```text
<name>_EL1
<name>_EL2
<name>_EL3
```

含义是这个寄存器属于哪个 Exception Level 的控制面。

#### EL1：guest kernel / host kernel 的 OS 控制面

| 类别             | 典型寄存器                                                | x86 直觉                                                 |
| ---------------- | --------------------------------------------------------- | -------------------------------------------------------- |
| 基本系统控制     | `SCTLR_EL1`                                               | 类似部分 `CR0/CR4`                                       |
| 页表根           | `TTBR0_EL1`、`TTBR1_EL1`                                  | 类似 `CR3`，但 Arm 分用户/内核两个 base                  |
| 地址转换控制     | `TCR_EL1`                                                 | 类似 `CR4` 里分页模式位 + x86 paging mode 配置，但更集中 |
| memory attribute | `MAIR_EL1`                                                | 类似 `PAT`，但 Arm 风格更干净                            |
| exception vector | `VBAR_EL1`                                                | 类似 `IDTR`，但不是 descriptor table                     |
| exception return | `ELR_EL1`、`SPSR_EL1`                                     | 类似保存 guest `RIP/RFLAGS` 的异常返回状态               |
| fault info       | `ESR_EL1`、`FAR_EL1`                                      | 类似 `#PF error code + CR2`，但更通用                    |
| thread/context   | `TPIDR_EL0`、`TPIDRRO_EL0`、`TPIDR_EL1`、`CONTEXTIDR_EL1` | 类似 `FS/GS base`、per-cpu、TLS、ASID/context 辅助       |
| access control   | `CPACR_EL1`                                               | 控制 FP/SIMD/SVE 等访问                                  |
| timer            | `CNTKCTL_EL1`、`CNTP_*`、`CNTV_*`                         | 类似 TSC/APIC timer 的一部分角色                         |

#### EL2：Hypervisor / KVM 控制面

这是你看 KVM/虚拟化时最重要的部分。

| 类别                        | 典型寄存器                     | 说明                                       |
| --------------------------- | ------------------------------ | ------------------------------------------ |
| hypervisor 主控制           | `HCR_EL2`                      | Arm 虚拟化最核心控制寄存器                 |
| 扩展 hypervisor 控制        | `HCRX_EL2`                     | 新 feature 的扩展控制                      |
| stage-2 页表                | `VTTBR_EL2`                    | guest IPA → host PA 的 stage-2 root        |
| stage-2 translation control | `VTCR_EL2`                     | stage-2 地址宽度、granule、shareability 等 |
| exception vector            | `VBAR_EL2`                     | EL2 trap/exception 入口                    |
| trap syndrome               | `ESR_EL2`                      | VM exit reason 的主要来源                  |
| fault address               | `FAR_EL2`、`HPFAR_EL2`         | fault VA / IPA 信息                        |
| exception return            | `ELR_EL2`、`SPSR_EL2`          | 从 EL2 返回 EL1 guest 的目标状态           |
| timer virtualization        | `CNTVOFF_EL2`、`CNTHCTL_EL2`   | 虚拟计时器/计数器控制                      |
| feature trap                | `CPTR_EL2`、`MDCR_EL2`         | FP/SIMD/debug/PMU 等 trap 控制             |
| virtual CPU ID              | `VPIDR_EL2`、`VMPIDR_EL2`      | 给 guest 看到的 CPU ID / affinity          |
| fine-grained trap           | `HFG*TR_EL2`、`HDFG*TR_EL2` 等 | 更细粒度 trap system register/instruction  |
| GIC virtualization          | `ICH_*_EL2`                    | virtual interrupt controller 控制          |

Arm 官方 A-profile 页面把 AArch64 virtualization 描述为包含 stage-2
translation、virtual exceptions、trapping、nested virtualization、VHE、Secure
EL2、VMID 等主题；GICv3/v4 virtualization 也有独立主题，覆盖 hypervisor
生成和管理虚拟中断的控制。([Arm][2])

#### EL3：secure monitor 控制面

| 类别         | 典型寄存器                                              | 说明                           |
| ------------ | ------------------------------------------------------- | ------------------------------ |
| secure 配置  | `SCR_EL3`                                               | Secure/Non-secure 世界切换控制 |
| EL3 系统控制 | `SCTLR_EL3`、`TCR_EL3`、`TTBR0_EL3`                     | EL3 自己的 MMU/异常环境        |
| exception    | `VBAR_EL3`、`ELR_EL3`、`SPSR_EL3`、`ESR_EL3`、`FAR_EL3` | EL3 exception handling         |

普通 Linux KVM 主要跑在 Non-secure EL2，很多时候你可以先把 EL3 放一边。

#### ID registers：Arm 的 CPUID 类似物

| 类别                 | 典型寄存器                                                 |
| -------------------- | ---------------------------------------------------------- |
| CPU identity         | `MIDR_EL1`、`MPIDR_EL1`                                    |
| processor feature    | `ID_AA64PFR0_EL1`、`ID_AA64PFR1_EL1`                       |
| memory model feature | `ID_AA64MMFR0_EL1`、`ID_AA64MMFR1_EL1`、`ID_AA64MMFR2_EL1` |
| instruction feature  | `ID_AA64ISAR0_EL1`、`ID_AA64ISAR1_EL1`、`ID_AA64ISAR2_EL1` |
| debug feature        | `ID_AA64DFR0_EL1`、`ID_AA64DFR1_EL1`                       |
| SVE/SME feature      | `ID_AA64ZFR0_EL1`、`ID_AA64SMFR0_EL1`                      |

这些非常像 x86 `CPUID leaves`。做 Arm CPU model 时，guest 能看到哪些
feature，很大程度就是这些 ID registers 的问题。

[1]: https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html "Manuals for Intel® 64 and IA-32 Architectures"
[2]: https://www.arm.com/architecture/learn-the-architecture/a-profile "Learn the Architecture - A-profile – Arm®"
[3]: https://cdrdv2-public.intel.com/671506/326019-sdm-vol-3c.pdf "Intel® 64 and IA-32 Architectures Software Developer’s Manual, Volume 3C: System Programming Guide, Part 3"


## cpu model
- aarch64 通过 cpu feature 机制来实现的，用户态访问，触发 exception
  ，内核中来模拟 x86 中，这部分功能勉强对于的是 cpuid 和 msr 内核模块 但是 cpuid
  在内核态和用户态执行，结果都是一样的，无法体现 用户态可以使用那些功能



调查了下 kvm forum ，aarch64 的 cpu model 还是在开发中:

| 年份      | Talk                                                                                  | 方向                                                                                               |
| --------- | ------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------- |
| 2025      | **Arm and QEMU cpu models - where are we right now?** — Cornelia Huck, Sebastian Ott  | 这是 2023 Arm CPU model 问题的后续/现状总结，很相关。([kvm-forum.qemu.org][1])                     |
| 2024      | **The Road to Optimal CPU Virtualization on Hybrid Platform** — Zhao Liu, Zhenyu Wang | 偏 Intel hybrid/P-core/E-core 这类异构平台上的 CPU virtualization。([kvm-forum.qemu.org][4])       |
| 2023      | **QEMU Arm CPU models and KVM** — Cornelia Huck                                       | 你提到的这个，核心是 Arm 上 KVM 基本只能用 `host` CPU model，迁移不安全。([kvm-forum.qemu.org][2]) |
| 2023      | **Live control of (most) CPU features via hybrid vCPU model** — Like Xu               | 更偏 CPU feature 动态控制 / hybrid vCPU model。([kvm-forum.qemu.org][2])                           |
| 2022      | **CPU Feature Management: Lessons from Trenches** — Shivam Kumar, Soham Ghosh         | 很像 x86/云厂商视角的 CPU feature 管理经验。([kvm-forum.qemu.org][3])                              |
| 2018      | **What Did Spectre and Meltdown Teach about CPU Models?** — Paolo Bonzini             | CPU model 和安全漏洞暴露/屏蔽的关系，x86 背景很强。([kvm-forum.qemu.org][5])                       |
| 2022      | **QEMU/KVM Upgrade Test (Stable Guest ABI and in Place Upgrade)** — Min Deng          | 不直接叫 CPU model，但和 stable guest ABI、升级兼容性相关。([kvm-forum.qemu.org][3])               |
| 2020/2023 | **Virtual CPU Hotplug on SoC/ARM64** 系列                                             | 不是 CPU model，但和 vCPU 拓扑、ARM64 虚拟 CPU 生命周期强相关。([kvm-forum.qemu.org][6])           |

[1]: https://kvm-forum.qemu.org/2025/ "KVM Forum 2025"
[2]: https://kvm-forum.qemu.org/2023/ "KVM Forum 2023"
[3]: https://kvm-forum.qemu.org/2022/ "KVM Forum 2022"
[4]: https://kvm-forum.qemu.org/2024/ "KVM Forum 2024"
[5]: https://kvm-forum.qemu.org/2018/ "KVM Forum 2018"
[6]: https://kvm-forum.qemu.org/2020/ "KVM Forum 2020"

在 2026-06-26 ，QEMU 中还在讨论 cpu model 相关的主题
https://patchew.org/QEMU/20260616132625.1732031-1-eric.auger@redhat.com/

## kvm

### 系统寄存器访问
x86 msr + cpuid + vmcs 相关的处理大致等价于 aarch64 的 sys_regs 的处理

- x86 msr 的功能太多，太乱了
  - 例如 x2apic 可以通过 mmio 控制，也可以通过 msr 控制

不过可以发现一些共同点:

### VMCS
aarch64 没有 VMCS ，其状态是保存在寄存器中的:

struct kvm_vcpu_arch 里嵌了一个 struct kvm_cpu_context ctxt，系统寄存器就存在这里：

arch/arm64/include/asm/kvm_host.h
```c
struct kvm_cpu_context {
        struct user_pt_regs regs;       /* sp = sp_el0 */
        ...
        u64 sys_regs[NR_SYS_REGS];      // <--- 对应 x86 VMCS 里的系统寄存器状态
        struct kvm_vcpu *__hyp_running_vcpu;
        u64 *vncr_array;
};

struct kvm_vcpu_arch {
        struct kvm_cpu_context ctxt;    // <--- 就是这里
        ...
};
```
具体可以看 __sysreg_restore_el1_state

从这里也可以看到 aarch64 和 x86 之前的设计思路的差别

### 其他
- x86 有很多历史包袱，例如
    - irq window / nmi window
    - shadow page table

- x86 的嵌套虚拟化早就完成了，aarch64 时至今日，我跑通过

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
