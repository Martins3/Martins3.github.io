# 从 cpu feature 到 kvm sys_reg_desc

<!-- e943dfb8-40ba-4cb4-a1f0-ff2db9f38e66 -->

这个机制和 x86 的机制非常类似的， 可以看到，arm64
作为后来者，整个结构从硬件实现上就定义的好很多， 代码也清晰很多。

arch/arm64/kernel/cpufeature.c 中定义的

```c
static const struct __ftr_reg_entry {
	u32			sys_id;
	struct arm64_ftr_reg 	*reg;
} arm64_ftr_regs[] = {

	// ...
	/* Op1 = 0, CRn = 0, CRm = 4 */
	ARM64_FTR_REG_OVERRIDE(SYS_ID_AA64PFR0_EL1, ftr_id_aa64pfr0,
			       &id_aa64pfr0_override),
```

arch/arm64/kvm/sys_regs.c 中定义的:

```c
static const struct sys_reg_desc sys_reg_descs[] = {
	// ...
	ID_FILTERED(ID_AA64PFR0_EL1, id_aa64pfr0_el1,
		    ~(ID_AA64PFR0_EL1_AMU |
		      ID_AA64PFR0_EL1_MPAM |
		      ID_AA64PFR0_EL1_SVE |
		      ID_AA64PFR0_EL1_AdvSIMD |
		      ID_AA64PFR0_EL1_FP)),
```

这个定义主要用来限制来自于用户态的写操作的，

所以，可以看到经典的这种代码:

```c
struct arm64_ftr_reg *regp = get_arm64_ftr_reg(sys_id);
```

## arm64_cpu_capabilities

```c
static const struct arm64_cpu_capabilities arm64_features = {
	// ...
	{
		.desc = "GICv3 CPU interface",
		.capability = ARM64_HAS_GICV3_CPUIF,
		.type = ARM64_CPUCAP_STRICT_BOOT_CPU_FEATURE,
		.matches = has_useable_gicv3_cpuif,
		ARM64_CPUID_FIELDS(ID_AA64PFR0_EL1, GIC, IMP)
	},
}


static const struct arm64_cpu_capabilities arm64_elf_hwcaps[];
```

arm64_ftr_regs 是基础，而 arm64_cpu_capabilities 是对于寄存器
结果的解析，相当于是更加结构化的结果。

例如 kvm 大量的调用 cpus_have_cap

## 调用细节

```c
#define ID_DESC_DEFAULT_CALLBACKS		\
	.access	= access_id_reg,		\
	.get_user = get_id_reg,			\
	.set_user = set_id_reg,			\
	.visibility = id_visibility,		\
	.reset = kvm_read_sanitised_id_reg
```

read_sanitised_ftr_reg

```c
u64 read_sanitised_ftr_reg(u32 id)
{
	struct arm64_ftr_reg *regp = get_arm64_ftr_reg(id);

	if (!regp)
		return 0;
	return regp->sys_val;
}
```

read_sanitised_ftr_reg 是如何被调用的:

```txt
@[
        read_sanitised_id_aa64pfr0_el1+0
        set_id_reg+96
        set_id_aa64pfr0_el1+80
        kvm_sys_reg_set_user+148
        kvm_arm_sys_reg_set_reg+196
        kvm_arm_set_reg+692
        kvm_arch_vcpu_ioctl+1540
        kvm_vcpu_ioctl+1404
        __do_sys_ioctl+380
        __arm64_sys_ioctl+40
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+60
        el0t_64_sync_handler+288
        el0t_64_sync+420
]: 16
```

- kvm_read_sanitised_id_reg 中的工作:
  - __kvm_read_sanitised_id_reg
    - read_sanitised_ftr_reg : 首先从这里获取，然后对于具体的 sysreg
      打补丁做修正
    - sanitise_id_aa64pfr0_el1

## kimi : struct arm64_ftr_reg 和 sys_reg_desc 各个 value 的对比

### struct arm64_ftr_reg（cpufeature 子系统）

位于 arch/arm64/include/asm/cpufeature.h，用来描述一颗 CPU 的 feature/ID
寄存器在多核、异构系统下的“安全值”：

```c
struct arm64_ftr_reg {
    const char *name;
    u64        strict_mask;   // 跨 CPU 必须一致的位
    u64        user_mask;     // 对用户空间可见/可差异化的位
    u64        sys_val;       // 系统（内核）视角的 sanitized 值
    u64        user_val;      // 用户空间视角的基础值
    struct arm64_ftr_override *override;  // 命令行覆盖
    const struct arm64_ftr_bits *ftr_bits;// 各字段语义
};
```

- sys_val：内核看到的“所有 CPU 都兼容”的安全值。启动时 update_cpu_ftr_reg()
  会对每颗 CPU 读到的寄存器值做交集/取安全值，最终写入这里。
- user_val：用户空间视角的基础值。
- 用户空间实际读到的值 = arm64_ftr_reg_user_value(reg)，即：

```c
return reg->user_val | (reg->sys_val & reg->user_mask);
```

### struct sys_reg_desc::val

这是KVM 子系统

位于 arch/arm64/kvm/sys_regs.h，用来描述 KVM guest 可见的某条系统寄存器：

```c
struct sys_reg_desc {
    ...
    u64 val;        /* Value (usually reset value), or write mask for idregs */
    ...
};
```

这里的 .val 通常是该寄存器的复位默认值。KVM 初始化 vCPU 时会调用 .reset()，把
.val 写到 vcpu->arch.sys_regs[reg] 里，guest 随后 MRS/MSR
访问的就是这个数组中的值。

### 它们的关系

对于普通系统寄存器（如 SCTLR_EL1、TCR_EL1 等），sys_reg_desc::val 和
arm64_ftr_reg 没有直接关系， .val 就是 KVM 硬编码的 reset 值。

对于 ID/feature 寄存器（如 ID_AA64PFR0_EL1、CTR_EL0、ID_AA64MMFR*_EL1
等），二者就关联起来了：

1. KVM 的 sys_reg_desc 表里，这些 ID 寄存器的 .access 通常是只读陷阱函数。
2. 它们的复位值不是直接来自 .val，而是通过 read_sanitised_ftr_reg(SYS_xxx) 或
   arm64_ftr_reg_user_value() 从对应的 arm64_ftr_reg 中获取。
3. 也就是说：KVM 把 cpufeature 子系统已经算好的、跨核兼容的 sys_val/user_val
   作为 guest 看到的 ID 寄存器值。

例如 arch/arm64/kvm/sys_regs.c 中有大量：

```c
u64 ctr = read_sanitised_ftr_reg(SYS_CTR_EL0);
val = read_sanitised_ftr_reg(id);
```

## kimi : cpufeature 机制 + kvm sys_regs 的层次结构

内核自己要用它来 根据 CPU 特性决定行为，例如：

1. 启用特性
   - 读 ID_AA64PFR0_EL1 判断有没有 SVE、FP、EL3；
   - 读 ID_AA64MMFR0_EL1 决定 ASID 位数、PARange；
   - 读 ID_AA64ISAR0_EL1 决定是否启用原子指令、RNDR、SHA、AES 等。
2. Errata / workaround
   - 很多 CPU bug workaround 是通过 capability 系统触发的，而 capability
     的检测条件就是读这些 ID 寄存器。
3. HWCAP / /proc/cpuinfo
   - 用户空间看到的 ISA 扩展能力（如 hwcap 里的 atomics、asimdrdm 等）是从
     arm64_ftr_reg 的 user_val 里算出来的。
4. 异构 CPU 一致性检查
   - big.LITTLE 系统里，secondary CPU 启动时要用 check_update_ftr_reg() 和 boot
     CPU 比较，不一致就打 taint。
5. 启动参数覆盖
   - 你甚至可以在 cmdline 里写 id_aa64pfr0=...
     之类的参数来覆盖特性位，这套框架负责解析和应用。

KVM 额外做的工作主要是：

- read_sanitised_ftr_reg() 被 EXPORT_SYMBOL_GPL，KVM 模块可以调用；
- KVM 的 arm64_check_features() 用 get_arm64_ftr_reg() 来校验用户空间设置的 ID
  寄存器值；
- cpufeature.c 里有少量 #ifdef CONFIG_KVM 的代码（比如 protected KVM
  能力检测）。

但 核心的表、字段描述、合并逻辑、override 机制，都是内核本来就有的。没有 KVM
时它们照样跑。

例如在 drivers/irqchip/irq-gic-v4.c 就不是 kvm 相关的:

```c
bool gic_cpuif_has_vsgi(void)
{
	unsigned long fld, reg = read_sanitised_ftr_reg(SYS_ID_AA64PFR0_EL1);

	fld = cpuid_feature_extract_unsigned_field(reg, ID_AA64PFR0_EL1_GIC_SHIFT);

	return fld >= ID_AA64PFR0_EL1_GIC_V4P1;
}
```

1. arm64_ftr_regs[] 是底层来源

以 ID_AA64PFR0_EL1 为例：

```c
ARM64_FTR_REG_OVERRIDE(SYS_ID_AA64PFR0_EL1, ftr_id_aa64pfr0, &id_aa64pfr0_override)
```

它绑定的是：

- SYS_ID_AA64PFR0_EL1：寄存器编码。
- ftr_id_aa64pfr0[]：描述了每个字段的可见性、严格性、安全值等。
- id_aa64pfr0_override：允许通过命令行或 DT 覆盖该寄存器值。

后续内核通过 read_sanitised_ftr_reg(SYS_ID_AA64PFR0_EL1)
读取这个消毒后的值，供调度、能力检测、KVM 等使用。

2. sys_reg_descs[] 是 KVM 的 Guest 视角

```c
ID_FILTERED(ID_AA64PFR0_EL1, id_aa64pfr0_el1,
        ~(ID_AA64PFR0_EL1_AMU |
          ID_AA64PFR0_EL1_MPAM |
          ID_AA64PFR0_EL1_SVE |
          ID_AA64PFR0_EL1_AdvSIMD |
          ID_AA64PFR0_EL1_FP)),
```

它的含义是：

- 这是一个 ID 寄存器，基础行为用 ID_DESC_DEFAULT_CALLBACKS（Guest 读取时会返回
  KVM 消毒后的值）。
- id_aa64pfr0_el1 对应一个自定义的 set_id_aa64pfr0_el1()
  函数，用于校验用户空间（QEMU）写入的值。
- val 是“可写掩码”：这里用 ~(...) 表示 AMU、MPAM、SVE、AdvSIMD、FP
  这几个字段不允许用户空间改写，其余字段可写。

## kimi : 那些 sys_reg_descs 定义了但是 arm64_ftr_regs 没定义的

有，而且数量还不少。但需要区分两类寄存器：

1. 非 ID 寄存器：sys_reg_descs[] 里大量的系统寄存器（如
   SCTLR_EL1、TTBR0_EL1、定时器、GIC、debug 等）本来就不会出现在
   arm64_ftr_regs[] 里，因为后者只关心 CPU feature ID 寄存器。
2. ID 寄存器：即使只在 ID 寄存器范围内看，也有几个是 sys_reg_descs[] 定义了但
   arm64_ftr_regs[] 里没有的。

ID 寄存器里的差异

我用脚本对比了两个表里的 ID 寄存器，结果如下：

在 sys_reg_descs[] 里，但不在 arm64_ftr_regs[] 里的 ID 寄存器：

```text
SYS_ID_AA64AFR0_EL1
SYS_ID_AA64AFR1_EL1
SYS_ID_AFR0_EL1
SYS_LORID_EL1
```

在 arm64_ftr_regs[] 里，但不在 sys_reg_descs[] 里的：

```text
SYS_DCZID_EL0
SYS_GMID_EL1
```

为什么这几个 ID 寄存器可以“破例”？

KVM 里的注释说的是：

```c
/*
 * ID regs: all ID_SANITISED() entries here must have corresponding
 * entries in arm64_ftr_regs[].
 */
```

注意它限定了是 ID_SANITISED() 这种条目，而不是所有 ID
条目。那四个“破例”的寄存器在 KVM 里是这样处理的：

```c
ID_HIDDEN(ID_AFR0_EL1),              /* CRm=1 */
...
ID_HIDDEN(ID_AA64AFR0_EL1),
ID_HIDDEN(ID_AA64AFR1_EL1),          /* CRm=5 */
...
{ SYS_DESC(SYS_LORID_EL1), trap_loregion },
```

- ID_AA64AFR0_EL1、ID_AA64AFR1_EL1、ID_AFR0_EL1 都是 ID_HIDDEN，对 Guest
  直接返回 RAZ。
- LORID_EL1 有专门的 trap_loregion 处理函数。

因为它们不需要从 arm64_ftr_regs[] 里取“消毒后”的值，所以不需要在
arm64_ftr_regs[] 里有对应条目。

这一点在 cpufeature.c 的 MRS 模拟代码里也能得到印证：

```c
regp = get_arm64_ftr_reg_nowarn(id);
if (regp)
    *valp = arm64_ftr_reg_user_value(regp);
else
    /*
     * The untracked registers are either IMPLEMENTATION DEFINED
     * (e.g, ID_AFR0_EL1) or reserved RAZ.
     */
    *valp = 0;
```

也就是说，内核自己对未跟踪的 ID 寄存器也是按 IMPLEMENTATION DEFINED 或 RAZ
处理的。

反过来呢？

SYS_DCZID_EL0 和 SYS_GMID_EL1 在 arm64_ftr_regs[] 里有，但不在 sys_reg_descs[]
里：

- DCZID_EL0：是用户态可见的 CPU feature 寄存器，通过 FTR_VISIBLE
  暴露给用户空间，KVM 不需要单独在 sys_reg_descs[] 里模拟。
- GMID_EL1：内核内部用于 MTE tag granule 大小，KVM 不把它暴露给 Guest。

总结

- 非 ID 寄存器：sys_reg_descs[] 比 arm64_ftr_regs[] 多得多，这很正常。
- ID 寄存器：严格来说，只有需要取“消毒值”的 ID_SANITISED() / ID_FILTERED() /
  ID_WRITABLE() 等才必须在 arm64_ftr_regs[] 里有对应条目；ID_HIDDEN
  或特殊处理的可以没有。
- 实际代码中“违规”的 ID 寄存器是 ID_AA64AFR0_EL1、ID_AA64AFR1_EL1、ID_AFR0_EL1
  和 LORID_EL1，它们因为直接返回 RAZ 或有专用处理函数，所以不需要进入
  arm64_ftr_regs[]。

┌─────────────────────┬─────────────────┬────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ 宏名（Linux 写法） │ 寄存器名 │ 作用 │
├─────────────────────┼─────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SYS_ID_AA64AFR0_EL1 │ ID_AA64AFR0_EL1 │ AArch64 辅助特性寄存器
0。内容完全由具体实现定义（IMPLEMENTATION DEFINED），用于暴露芯片自定义的
AArch64 辅助特性。 │
├─────────────────────┼─────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SYS_ID_AA64AFR1_EL1 │ ID_AA64AFR1_EL1 │ AArch64 辅助特性寄存器 1。与 AFR0
类似，也是实现自定义的预留寄存器。 │
├─────────────────────┼─────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SYS_ID_AFR0_EL1 │ ID_AFR0_EL1 │ AArch32 辅助特性寄存器 0。用于暴露 AArch32
状态下的实现自定义特性。 │
├─────────────────────┼─────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SYS_LORID_EL1 │ LORID_EL1 │ 有限顺序域 ID 寄存器。报告 Limited Ordering
Regions（LOR）支持情况，比如支持多少个 LOR 区域。LOR │ │ │ │
用于在某些弱序内存模型下对特定地址范围施加更严格的顺序约束。 │
├─────────────────────┼─────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SYS_DCZID_EL0 │ DCZID_EL0 │ Data Cache Zero ID 寄存器。从 EL0 可读，描述 DC
ZVA 指令一次清零的缓存块大小，以及该指令是否允许使用。 │
├─────────────────────┼─────────────────┼────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ SYS_GMID_EL1 │ GMID_EL1 │ MTE Granule Maximum ID 寄存器。报告 Memory Tagging
Extension（MTE）支持的 tag granule 信息，比如 GCR_EL1 中 PMID/PMEC │ │ │ │
字段的最大值。 │
└─────────────────────┴─────────────────┴────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘

共同特点：

- 都是 只读 寄存器，用于软件探测硬件能力。
- 前缀 ID_ 的寄存器（AFR0/AFR1 等）通常属于 ID group，在 CPU 特性探测代码（如
  Linux read_cpuid_*()）中读取。
- DCZID_EL0 是少数几个可在 EL0 访问的识别寄存器，方便用户态库代码判断 DC ZVA
  能力。

## 经典案例

在操作系统添加一些 feature 的支持:

commit 011e5f5bf529 ("arm64/cpufeature: Add remaining feature bits in
ID_AA64PFR0 register")

```diff
commit 011e5f5bf529f8ec2988ef7667d1a52f83273c36
Author: Anshuman Khandual <anshuman.khandual@arm.com>
Date:   Tue May 19 15:10:47 2020 +0530

    arm64/cpufeature: Add remaining feature bits in ID_AA64PFR0 register

    Enable MPAM and SEL2 features bits in ID_AA64PFR0 register as per ARM DDI
    0487F.a specification.

    Cc: Catalin Marinas <catalin.marinas@arm.com>
    Cc: Will Deacon <will@kernel.org>
    Cc: Mark Rutland <mark.rutland@arm.com>
    Cc: Suzuki K Poulose <suzuki.poulose@arm.com>
    Cc: linux-arm-kernel@lists.infradead.org
    Cc: linux-kernel@vger.kernel.org

    Suggested-by: Will Deacon <will@kernel.org>
    Signed-off-by: Anshuman Khandual <anshuman.khandual@arm.com>
    Link: https://lore.kernel.org/r/1589881254-10082-11-git-send-email-anshuman.khandual@arm.com
    [will: Make SEL2 a NONSTRICT feature per Suzuki]
    Signed-off-by: Will Deacon <will@kernel.org>

diff --git a/arch/arm64/include/asm/sysreg.h b/arch/arm64/include/asm/sysreg.h
index ea075cc08c8f..638f6108860f 100644
--- a/arch/arm64/include/asm/sysreg.h
+++ b/arch/arm64/include/asm/sysreg.h
@@ -645,6 +645,8 @@
 #define ID_AA64PFR0_CSV2_SHIFT		56
 #define ID_AA64PFR0_DIT_SHIFT		48
 #define ID_AA64PFR0_AMU_SHIFT		44
+#define ID_AA64PFR0_MPAM_SHIFT		40
+#define ID_AA64PFR0_SEL2_SHIFT		36
 #define ID_AA64PFR0_SVE_SHIFT		32
 #define ID_AA64PFR0_RAS_SHIFT		28
 #define ID_AA64PFR0_GIC_SHIFT		24
diff --git a/arch/arm64/kernel/cpufeature.c b/arch/arm64/kernel/cpufeature.c
index 41f6e9b26d18..68744871a65d 100644
--- a/arch/arm64/kernel/cpufeature.c
+++ b/arch/arm64/kernel/cpufeature.c
@@ -222,6 +222,8 @@ static const struct arm64_ftr_bits ftr_id_aa64pfr0[] = {
 	ARM64_FTR_BITS(FTR_HIDDEN, FTR_NONSTRICT, FTR_LOWER_SAFE, ID_AA64PFR0_CSV2_SHIFT, 4, 0),
 	ARM64_FTR_BITS(FTR_VISIBLE, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64PFR0_DIT_SHIFT, 4, 0),
 	ARM64_FTR_BITS(FTR_HIDDEN, FTR_NONSTRICT, FTR_LOWER_SAFE, ID_AA64PFR0_AMU_SHIFT, 4, 0),
+	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64PFR0_MPAM_SHIFT, 4, 0),
+	ARM64_FTR_BITS(FTR_HIDDEN, FTR_NONSTRICT, FTR_LOWER_SAFE, ID_AA64PFR0_SEL2_SHIFT, 4, 0),
 	ARM64_FTR_BITS(FTR_VISIBLE_IF_IS_ENABLED(CONFIG_ARM64_SVE),
 				   FTR_STRICT, FTR_LOWER_SAFE, ID_AA64PFR0_SVE_SHIFT, 4, 0),
 	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64PFR0_RAS_SHIFT, 4, 0),
```


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
