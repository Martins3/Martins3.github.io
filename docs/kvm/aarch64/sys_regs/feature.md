# aarch64 cpufeature
<!-- e943dfb8-40ba-4cb4-a1f0-ff2db9f38e66 -->

## 简述

arm64_ftr_bits -> arm64_ftr_reg -> arm64_ftr_regs

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

	// ...
	/* Op1 = 0, CRn = 0, CRm = 5 */
	ARM64_FTR_REG(SYS_ID_AA64DFR0_EL1, ftr_id_aa64dfr0),
```


## arm64_ftr_bits

```c
struct arm64_ftr_bits {
	bool		sign;	/* Value is signed ? */
	bool		visible;
	bool		strict;	/* CPU Sanity check: strict matching required ? */
	enum ftr_type	type;
	u8		shift;
	u8		width;
	s64		safe_val; /* safe value for FTR_EXACT features */
};
```

- sign        : 该字段按有符号数还是无符号数解释。true 表示有符号，false 表示无符号。某些 4-bit 字段用 0xf 表示“不支持”，需要解释为 -1。
- visible     : 是否向用户态暴露该字段。true 时，用户态执行受内核模拟的 MRS 读取 ID 寄存器，可以看到经过归一化的系统值；false 时返回 safe_val。这不直接表示 KVM 是否向 guest 暴露。
- strict      : 不同 CPU 的该字段是否必须严格相同。若为 true，非启动 CPU 与启动 CPU 的值不同时会打印 SANITY CHECK 警告，并把内 核标记为 TAINT_CPU_OUT_OF_SPEC。
- shift       : 该字段在寄存器中的最低位位置。
- width       : 字段宽度，单位为 bit。字段范围为 [shift + width - 1 : shift]。数组末尾用 width == 0 作为结束标志。
- safe_val    : 预定义安全值。主要用于 FTR_EXACT 字段发生不一致时；当前代码也用它作为隐藏字段向用户态返回的替代值。
- type        : 当数值不同的时候，如果选择 sanitised val

type 一共四种选择:
```c
enum ftr_type {
      FTR_EXACT,
      FTR_LOWER_SAFE,
      FTR_HIGHER_SAFE,
      FTR_HIGHER_OR_ZERO_SAFE,
};
```
- FTR_EXACT CPU : 值相同时保留原值；不同时直接使用 safe_val。
- FTR_LOWER_SAFE : 选择所有 CPU 中较小的值，常用于特性版本字段，确保只公开所有 CPU 都支持的能力。
- FTR_HIGHER_SAFE : 很少的选择了
- FTR_HIGHER_OR_ZERO_SAFE : 非零值之间选择较大的值，但只要有一个 CPU 的值为 0，最终就是 0。即该字段语义上把 0 看作最大的安全值。

大多数时候，都是 FTR_LOWER_SAFE 定义的，这个很容易理解，因为取 CPU 能力的交集

### 如果 arm64_ftr_bits 没有定义对应的 bit，那么 sanitised val 会被配置为 0
```c
static const struct arm64_ftr_bits ftr_id_aa64mmfr1[] = {
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_ECBHB_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_NONSTRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_TIDCP1_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_VISIBLE, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_AFP_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_HCX_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_ETS_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_TWED_SHIFT, 4, 0),
	// ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_XNX_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_HIGHER_SAFE, ID_AA64MMFR1_EL1_SpecSEI_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_PAN_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_LO_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_HPDS_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_VH_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_VMIDBits_SHIFT, 4, 0),
	ARM64_FTR_BITS(FTR_HIDDEN, FTR_STRICT, FTR_LOWER_SAFE, ID_AA64MMFR1_EL1_HAFDBS_SHIFT, 4, 0),
	ARM64_FTR_END,
};
```
例如，如果注释掉 ID_AA64MMFR1_EL1_XNX_SHIFT ，那么可以确认，
```c
		u64 hw = read_sysreg_s(reg);
		u64 san = read_sanitised_ftr_reg(reg);
```
hw 中的 xnx 为 1 ，但是 san 中会是 0

这非常合理，sanitised 就是用来描述操作系统对于数值的支持。

#### 例子
commit 011e5f5bf529 ("arm64/cpufeature: Add remaining feature bits in ID_AA64PFR0 register")

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


### FTR_LOWER_SAFE

前面提到了 arm64_ftr_bits::type 的功能，更多从异构的角度，也就是不同的 CPU 能力不同，
不过这个功能也适用于热迁移功能的。

最经典的例子就是，kvm_arm64_ftr_safe_value ，他禁止不同的 CPU 使用
ID_AA64DFR0_EL1_PMUVer ID_AA64DFR0_EL1_DebugVer ，但是热迁移，通过修改为 FTR_LOWER_SAFE ，
允许从版本低的热迁移过来。
```c
static s64 kvm_arm64_ftr_safe_value(u32 id, const struct arm64_ftr_bits *ftrp,
				    s64 new, s64 cur)
{
	struct arm64_ftr_bits kvm_ftr = *ftrp;

	/* Some features have different safe value type in KVM than host features */
	switch (id) {
	case SYS_ID_AA64DFR0_EL1:
		switch (kvm_ftr.shift) {
		case ID_AA64DFR0_EL1_PMUVer_SHIFT:
			kvm_ftr.type = FTR_LOWER_SAFE;
			break;
		case ID_AA64DFR0_EL1_DebugVer_SHIFT:
			kvm_ftr.type = FTR_LOWER_SAFE;
			break;
		}
		break;
	case SYS_ID_DFR0_EL1:
		if (kvm_ftr.shift == ID_DFR0_EL1_PerfMon_SHIFT)
			kvm_ftr.type = FTR_LOWER_SAFE;
		break;
	}

	return arm64_ftr_safe_value(&kvm_ftr, new, cur);
}
```

具体的检查逻辑在 arm64_check_features() 中:

在 arm64_check_features() 里，对每个 可写 字段会再调 kvm_arm64_ftr_safe_value()
检查源端值是不是 target 能安全接受的子集：
arm64_check_features() 会在第一个失败的字段处直接返回 -E2BIG（内部），
然后 set_id_reg() 把它转成 -EINVAL（ret=-22）。


## arm64_ftr_reg

arm64_ftr_reg 就是核心，
```c
/*
 * @arm64_ftr_reg - Feature register
 * @strict_mask		Bits which should match across all CPUs for sanity.
 * @sys_val		Safe value across the CPUs (system view)
 */
struct arm64_ftr_reg {
	const char			*name;
	u64				strict_mask;
	u64				user_mask;
	u64				sys_val;
	u64				user_val;
	struct arm64_ftr_override	*override;
	const struct arm64_ftr_bits	*ftr_bits;
};
```

## user_val 的使用

具体分析参考: ./feature-user.md

## sys_val 的使用

就是通过 read_sanitised_ftr_reg 接口获取的:
```c
u64 read_sanitised_ftr_reg(u32 id)
{
	struct arm64_ftr_reg *regp = get_arm64_ftr_reg(id);

	if (!regp)
		return 0;
	return regp->sys_val;
}
```

### misc

例如，当 trace 一下 read_sanitised_ftr_reg ，可以发现其调用频率极高:
```txt
@[
        read_sanitised_ftr_reg+0
        filemap_map_pages+484
        do_read_fault+232
        do_fault+256
        handle_pte_fault+152
        __handle_mm_fault+444
        handle_mm_fault+172
        do_page_fault+376
        do_translation_fault+84
        do_mem_abort+72
        el0_ia+108
        el0t_64_sync_handler+204
        el0t_64_sync+420
]: 14900
```

### arm64_cpu_capabilities

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

这些能力会更新到 system_cpucaps 中去，通过调用 .matches 注册的函数
也就是 has_useable_gicv3_cpuif ，基本上都是走向到 read_scoped_sysreg 中

例如 kvm 大量的调用 cpus_have_cap 访问的是 bitmap `system_cpucaps`

### kvm

```c
#define ID_DESC_DEFAULT_CALLBACKS		\
	.access	= access_id_reg,		\
	.get_user = get_id_reg,			\
	.set_user = set_id_reg,			\
	.visibility = id_visibility,		\
	.reset = kvm_read_sanitised_id_reg
```

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
    - read_sanitised_ftr_reg : 首先从这里获取 sys_val，然后对于具体的 sysreg 打补丁做修正
    - sanitise_id_aa64pfr0_el1

#### sys_reg_desc::val

./sys_regs.md 中已经谈到过，取决于 sys reg 的类型

```c
struct sys_reg_desc {
	/* Sysreg string for debug */
	// ...
	/* Value (usually reset value), or write mask for idregs */
	u64 val;
```

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

| 宏名（Linux 写法）   | 寄存器名        | 作用                                                                                                                                                                              |
| -------------------- | --------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| SYS_ID_AA64AFR0_EL1  | ID_AA64AFR0_EL1 | AArch64 辅助特性寄存器 0。内容完全由具体实现定义（IMPLEMENTATION DEFINED），用于暴露芯片自定义的 AArch64 辅助特性。                                                              |
| SYS_ID_AA64AFR1_EL1  | ID_AA64AFR1_EL1 | AArch64 辅助特性寄存器 1。与 AFR0 类似，也是实现自定义的预留寄存器。                                                                                                              |
| SYS_ID_AFR0_EL1      | ID_AFR0_EL1     | AArch32 辅助特性寄存器 0。用于暴露 AArch32 状态下的实现自定义特性。                                                                                                               |
| SYS_LORID_EL1        | LORID_EL1       | 有限顺序域 ID 寄存器。报告 Limited Ordering Regions（LOR）支持情况，比如支持多少个 LOR 区域。LOR 用于在某些弱序内存模型下对特定地址范围施加更严格的顺序约束。                       |
| SYS_DCZID_EL0        | DCZID_EL0       | Data Cache Zero ID 寄存器。从 EL0 可读，描述 DC ZVA 指令一次清零的缓存块大小，以及该指令是否允许使用。                                                                            |
| SYS_GMID_EL1         | GMID_EL1        | MTE Granule Maximum ID 寄存器。报告 Memory Tagging Extension（MTE）支持的 tag granule 信息，比如 GCR_EL1 中 PMID/PMEC 字段的最大值。                                               |

共同特点：

- 都是 只读 寄存器，用于软件探测硬件能力。
- 前缀 ID_ 的寄存器（AFR0/AFR1 等）通常属于 ID group，在 CPU 特性探测代码（如
  Linux read_cpuid_*()）中读取。
- DCZID_EL0 是少数几个可在 EL0 访问的识别寄存器，方便用户态库代码判断 DC ZVA
  能力。


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
