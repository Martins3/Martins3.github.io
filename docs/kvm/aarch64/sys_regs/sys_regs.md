# sys_regs 基础

```c
struct sys_reg_desc {
	/* Sysreg string for debug */
	const char *name;

	/* MRS/MSR instruction which accesses it. */
	u8	Op0;
	u8	Op1;
	u8	CRn;
	u8	CRm;
	u8	Op2;

	/* Trapped access from guest, if non-NULL. */
	bool (*access)(struct kvm_vcpu *,
		       struct sys_reg_params *,
		       const struct sys_reg_desc *);

	/* Initialization for vcpu. */
	void (*reset)(struct kvm_vcpu *, const struct sys_reg_desc *);

	/* Index into sys_reg[], or 0 if we don't need to save it. */
	int reg;

	/* Value (usually reset value) */
	u64 val;

	/* Custom get/set_user functions, fallback to generic if NULL */
	int (*get_user)(struct kvm_vcpu *vcpu, const struct sys_reg_desc *rd,
			const struct kvm_one_reg *reg, void __user *uaddr);
	int (*set_user)(struct kvm_vcpu *vcpu, const struct sys_reg_desc *rd,
			const struct kvm_one_reg *reg, void __user *uaddr);

	/* Return mask of REG_* runtime visibility overrides */
	unsigned int (*visibility)(const struct kvm_vcpu *vcpu,
				   const struct sys_reg_desc *rd);
};
```

基于这个原则，一共定义了这些 table:
```c
int __init kvm_sys_reg_table_init(void)
{
  // ...
	valid &= check_sysreg_table(sys_reg_descs, ARRAY_SIZE(sys_reg_descs), false);
	valid &= check_sysreg_table(cp14_regs, ARRAY_SIZE(cp14_regs), true);
	valid &= check_sysreg_table(cp14_64_regs, ARRAY_SIZE(cp14_64_regs), true);
	valid &= check_sysreg_table(cp15_regs, ARRAY_SIZE(cp15_regs), true);
	valid &= check_sysreg_table(cp15_64_regs, ARRAY_SIZE(cp15_64_regs), true);
	valid &= check_sysreg_table(sys_insn_descs, ARRAY_SIZE(sys_insn_descs), false);
  // ...
}
```

几张表都在 arch/arm64/kvm/sys_regs.c 里，是 KVM 用来分发“guest 访问系统寄存器/系统指令”的陷阱处理表。当 VM 里执行 MRS/MSR 或某些系统指令触发
trap 到 EL2 时，KVM 会根据 Op0/Op1/CRn/CRm/Op2 到这些表里查找对应的 sys_reg_desc，再调用它的 .access 处理函数。

简单说每个表的分工：
```txt
┌────────────────┬───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┐
│ 表名           │ 对应的内容                                                                                                                │
├────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ sys_reg_descs  │ AArch64 架构系统寄存器（MRS/MSR）。这是最大的一张表，包含 debug 寄存器、各种 ID                                           │
│                │ 寄存器（ID_*_EL1）、系统控制（SCTLR_EL1、TCR_EL1、TTBR*_EL1）、异常相关（ESR/FAR/SPSR/ELR）、RAS、MTE、PointerAuth、PMU、 │
│                │ generic timer、GIC 等。                                                                                                   │
├────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ cp14_regs      │ AArch32 CP14 调试寄存器（32 位访问）。比如 DBG_BCR/BVR/WCR/WVR、DBGDSCR、DBGVCR、DBGOSLAR 等，大多直接 trap/忽略或映射到  │
│                │ AArch64 debug 寄存器。                                                                                                    │
├────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ cp14_64_regs   │ AArch32 CP14 的 64 位访问。目前只有 DBGDRAR 和 DBGDSAR。                                                                  │
├────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ cp15_regs      │ AArch32 CP15 系统寄存器（32 位访问）。包括 SCTLR、TTBR0/1（32 位形式）、TTBCR、DFSR/DFAR、ACTLR、ICC_PMR、CSSELR、cache   │
│                │ maintenance 等。                                                                                                          │
├────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ cp15_64_regs   │ AArch32 CP15 的 64 位访问。例如 64 位形式的 TTBR0/1、ICC_SGI1R/ASGI1R/SGI0R、generic timer（CNTPCT、CNTVCT、CNTP_CVAL     │
│                │ 等）、PMU 64-bit counter。                                                                                                │
├────────────────┼───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┤
│ sys_insn_descs │ AArch64 系统指令（不是 MRS/MSR，而是 SYS/SYSL 指令）。主要是 cache 维护（DC_*SW）、地址转换（AT_*）和各种 TLBI（EL1/EL2   │
│                │ TLB 失效指令，包括 nested virt 相关变体）。                                                                               │
└────────────────┴───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

check_sysreg_table() 做的两件事：
1. 检查表是否按 Op0/Op1/CRn/CRm/Op2 升序排列，因为后续用二分查找。
2. 对 sys_reg_descs（第三个参数 reset_check = true）额外检查：如果表项指定了 .reg，就必须有 .reset，否则启动时报错。

之后 populate_sysreg_config() 会把 sys_reg_descs 和 sys_insn_descs 注册到每个 vCPU 的陷阱配置里，
决定哪些寄存器/指令需要 trap 到 KVM 的。

显然， sys_insn_descs 和 sys_reg_descs 就是关键了，一共是访问寄存器，一个是使用指令，
定义都非常规范:

```c
static struct sys_reg_desc sys_insn_descs[] = {
	{ SYS_DESC(SYS_DC_ISW), access_dcsw },
	{ SYS_DESC(SYS_DC_IGSW), access_dcgsw },
	{ SYS_DESC(SYS_DC_IGDSW), access_dcgsw },
	// ...
```

```c
static const struct sys_reg_desc sys_reg_descs[] = {
	// ...
	{ SYS_DESC(SYS_CTR_EL0), access_ctr },
  	// 通过 ioctl 访问，那么就是走默认的，
  	// 因为没有定义 get_user 和 set_user
  	// ...
	{ SYS_DESC(SYS_PMMIR_EL1), trap_raz_wi },
}
```

正如注释显示的那样:
- sys_reg_desc::access : 来自 guest 的访问
- sys_reg_desc::get_user / sys_reg_desc::set_user : 来自 user 的访问，也就是 qemu

至于 cp ，是 arm 中的 coprocesor 的概念:

- https://developer.arm.com/documentation/ddi0406/c/Application-Level-Architecture/The-Instruction-Sets/Coprocessor-instructions

- Transfer ARM core registers to and from coprocessor registers. For details, see:
  - MCR, MCR2
  - MCRR, MCRR2
  - MRC, MRC2
  - MRRC, MRRC2.
- Load or store the values of coprocessor registers. For details, see:
  - LDC, LDC2 (immediate)
  - LDC, LDC2 (literal)
  - STC, STC2.

## sys_reg_desc 中的常见定义

### ID_SANITISED
只读，reset 直接读 sanitized HW 值
```c
static const struct sys_reg_desc sys_reg_descs[] = {
	// ...
	ID_SANITISED(ID_AA64MMFR1_EL1),
```

```c
/* sys_reg_desc initialiser for known cpufeature ID registers */
#define ID_SANITISED(name) {			\
	SYS_DESC(SYS_##name),			\
	.access	= access_id_reg,		\
	.get_user = get_id_reg,			\
	.set_user = set_id_reg,			\
	.visibility = id_visibility,		\
	.reset = kvm_read_sanitised_id_reg,	\
	.val = 0,				\
}
```

### ID_WRITABLE
可写，reset 读 sanitized HW 值，set 用通用 set_id_reg

ID_WRITABLE(name, mask)
```c
/* sys_reg_desc initialiser for writable ID registers */
#define ID_WRITABLE(name, mask) {		\
	SYS_DESC(SYS_##name),			\
	.access	= access_id_reg,		\
	.get_user = get_id_reg,			\
	.set_user = set_id_reg,			\
	.visibility = id_visibility,		\
	.reset = kvm_read_sanitised_id_reg      \
	.val = mask,				\
}
```

## SYS_ID_AA64DFR0_EL1

SYS_ID_AA64DFR0_EL1 实在是太经典了，由于各种失误，导致需要很多特殊的操作
```c
  { SYS_DESC(SYS_ID_AA64DFR0_EL1),
    .access = access_id_reg,
    .get_user = get_id_reg,
    .set_user = set_id_aa64dfr0_el1,
    .reset = read_sanitised_id_aa64dfr0_el1,
    .val = ID_AA64DFR0_EL1_DoubleLock_MASK |
           ID_AA64DFR0_EL1_WRPs_MASK |
           ID_AA64DFR0_EL1_PMUVer_MASK |
           ID_AA64DFR0_EL1_DebugVer_MASK,
  },
```

其实很容易理解:
1. access : 从 Guest 访问的时候调用
2. get_user 和 set_user : 从 QEMU 访问
3. reset 就是默认数值
4. val 是可写位掩码（write mask）。1 表示 userspace 可以改这些位，其余位强制为硬件 sanitize 后的值。

为什么 ID_AA64DFR0_EL1 要单独注册？

### reset

read_sanitised_id_aa64dfr0_el1() 做的工作：

1. 限制 DebugVer 上限：
	ID_REG_LIMIT_FIELD_ENUM(val, ID_AA64DFR0_EL1, DebugVer, V8P8)
	把 DebugVer 限制到 V8P8，不会暴露超过 KVM 支持能力的新 debug 版本。
2. PMUVer 按 vCPU 配置重写：
	只有在 kvm_vcpu_has_pmu(vcpu) 时才写入正确的 PMUv3 版本，否则清 0。
3. 隐藏 SPE / BRBE：
	KVM 目前不向 guest 暴露 SPE 和 BRBE。
```c
    val &= ~ID_AA64DFR0_EL1_PMSVer_MASK;  // SPE
    val &= ~ID_AA64DFR0_EL1_BRBE_MASK;    // BRBE
```
4. Realm / CVM 特殊处理：
	调用 kvm_realm_reset_id_aa64dfr0_el1(vcpu, val)，针对 confidential VM 再做 BRP/WRP 限制。
	这些逻辑其他 ID 寄存器没有，所以必须自定义 .reset。

### set_user
userspace 写时需要额外校验（.set_user 专用函数）

set_id_aa64dfr0_el1() 做的工作：

1. PMUVer IMP_DEF 兼容旧 KVM：
老版本 KVM 曾错误地把 IMP_DEF PMU 暴露给 userspace；这里把它改写为 0（Not implemented），避免旧 userspace 拿到非法值。

2. 校验 DebugVer、BRPs、WRPs、CTX_CMPs：
  ```c
    if (debugver < ID_AA64DFR0_EL1_DebugVer_IMP || !bps || !wps || ctx_cmps > bps)
        return -EINVAL;
  ```
  DebugVer 必须 ≥ IMP，BRP/WRP 数量不能为 0，CTX_CMPs 不能多于 BRPs。
3. 非 CVM 场景下禁止乱改 BRP/WRP/CTX_CMPs：
结合 .val 掩码，只允许改 DoubleLock / WRPs / PMUVer / DebugVer 等字段。

通用 set_id_reg 只做“是否超过硬件能力”的检查，不会针对 ID_AA64DFR0_EL1 的字段关系做上述业务校验。

## sys_reg_desc::val

struct sys_reg_desc.val 在 arch/arm64/kvm/sys_regs.h 中是一个复用字段，
含义取决于这条描述的是普通系统寄存器还是 ID/feature 寄存器：

1. 普通寄存器：reset value

对普通 EL1/EL2 系统寄存器，val 就是该寄存器的复位值（reset value）。
典型使用函数是 reset_val()：

```c
  static inline u64 reset_val(struct kvm_vcpu *vcpu, const struct sys_reg_desc *r)
  {
      __vcpu_sys_reg(vcpu, r->reg) = r->val;
      return __vcpu_sys_reg(vcpu, r->reg);
  }
```

例如：

```c
  { SYS_DESC(SYS_SCTLR_EL1), access_vm_reg, reset_val, SCTLR_EL1, 0x00C50078 },
```

这里 0x00C50078 就是 SCTLR_EL1 的初始值。

2. ID/feature 寄存器：write mask

对 ARM64 的 cpufeature ID 寄存器，val 不是复位值，而是用户空间可写字段掩码（writable mask）。

arch/arm64/kvm/sys_regs.c 里的注释写得很清楚：

```c
  /*
   * Since reset() callback and field val are not used for idregs, they will be
   * used for specific purposes for idregs.
   * The reset() would return KVM sanitised register value.
   * The val would be used as a mask indicating writable fields for the idreg.
   * Only bits with 1 are writable from userspace.
   */
```

在 arm64_check_features() 中，rd->val 被直接当作 writable_mask：

```c
  u64 writable_mask = rd->val;
  u64 limit = rd->reset(vcpu, rd);
  ...
  if ((ftr_mask & writable_mask) != ftr_mask)
      continue;
```

逻辑是：
• 掩码中为 1 的位：表示该字段允许用户空间修改，KVM 只检查新值是否是硬件支持值的“安全子集”。
• 掩码中为 0 的位：表示不可写，用户写入的值必须等于 KVM 内部 sanitize 后的值。

例如：

```c
  { SYS_DESC(SYS_ID_AA64PFR0_EL1),
    .reset = read_sanitised_id_aa64pfr0_el1,
    .val = ID_AA64PFR0_EL1_CSV2_MASK | ID_AA64PFR0_EL1_CSV3_MASK, },
```

表示 ID_AA64PFR0_EL1 里只有 CSV2 和 CSV3 两个字段允许 userspace 改。

而大多数 ID 寄存器用 ID_SANITISED() 宏初始化：

```c
  #define ID_SANITISED(name) {    \
      ...
      .reset = kvm_read_sanitised_id_reg, \
      .val = 0,                \
  }
```

.val = 0 表示目前不允许 userspace 修改任何字段。

总结
┌───────────────────┬────────────────────────┬───────────────────────────────────┐
│ 寄存器类型        │ val 含义               │ 典型例子                          │
├───────────────────┼────────────────────────┼───────────────────────────────────┤
│ 普通系统寄存器    │ reset value            │ SCTLR_EL1 的 0x00C50078           │
├───────────────────┼────────────────────────┼───────────────────────────────────┤
│ ID/feature 寄存器 │ userspace 可写字段掩码 │ ID_AA64PFR0_EL1 的 CSV2/CSV3_MASK │
└───────────────────┴────────────────────────┴───────────────────────────────────┘

所以注释
```txt
/* Value (usually reset value), or write mask for idregs */
```
里的 “or” 就是表示这种二义性复用。

### sys_reg_desc::val 和 reset 的作用前后

set_id_aa64dfr0_el1 在前，.val 的校验在后。

```txt
  kvm_sys_reg_set_user()          // 处理 KVM_SET_ONE_REG
    └── r->set_user(vcpu, r, val) // 即 set_id_aa64dfr0_el1，先拿到 userspace 原始值
          └── set_id_reg(vcpu, rd, val)
                └── arm64_check_features(vcpu, rd, val)
                      └── writable_mask = rd->val;   // 这里才用到 .val
                          limit = rd->reset(vcpu, rd);
```

1. 先执行：set_id_aa64dfr0_el1

set_id_aa64dfr0_el1 是第一个拿到 userspace 写入值的函数，做针对该寄存器的特殊业务校验：

• 把老 KVM 误暴露的 PMUVer_IMP_DEF 改写为 0；
• 检查 DebugVer、BRPs、WRPs、CTX_CMPs 的合法性；
• 非 CVM 场景下拒绝不合理的 debug 配置。

它不会直接用 .val 去 mask 输入值；它只负责“这个寄存器特有的语义检查”，然后把（可能已被修正的）val 传给 set_id_reg。

2. 后执行：.val 作为可写位掩码

.val 在 arm64_check_features() 中被当作 writable_mask 使用：

```c
  u64 writable_mask = rd->val;        // 即 ID_AA64DFR0_EL1 的 .val
  u64 limit = rd->reset(vcpu, rd);    // 即 read_sanitised_id_aa64dfr0_el1 的返回值
```

这其实很合理，set_id_aa64dfr0_el1 本来就是对于用户态的内容来做修正的，
做了修正之后，然后和 hw sanitised val 做比对

## kvm exit 如何处理 sysreg
```c
static exit_handle_fn arm_exit_handlers[] = {
	[ESR_ELx_EC_CP15_32]	= kvm_handle_cp15_32,
	[ESR_ELx_EC_CP15_64]	= kvm_handle_cp15_64,
	[ESR_ELx_EC_CP14_MR]	= kvm_handle_cp14_32,
	[ESR_ELx_EC_CP14_LS]	= kvm_handle_cp14_load_store,
	[ESR_ELx_EC_CP10_ID]	= kvm_handle_cp10_id,
	[ESR_ELx_EC_CP14_64]	= kvm_handle_cp14_64,

	// ...
	[ESR_ELx_EC_SYS64]	= kvm_handle_sys_reg,
```

- kvm_handle_sys_reg
	- emulate_sys_reg
		- perform_access
			- sys_reg_desc::access : 调用对应寄存器的，一般就是 access_id_reg ，最后其实就是 read_id_reg

为什么会切分出来发这些不同的 handler ，这个可能是需要仔细研究下 arm 架构了，
不过最常用的 sys_insn_descs 和 sys_reg_descs
是走到 kvm_handle_sys_reg 的

### access_id_reg

access_id_reg — ID 特性寄存器的读回调

处理 只读 ID registers 的 MRS 访问，例如 ID_AA64PFR0_EL1、ID_AA64MMFR1_EL1、CTR_EL0 等。

ARM 手册里把下面这类寄存器统称为 ID registers：
┌─────────────────────────────────────────────────────────┬────────────────────────────────────────────────────────────────────────────────┐
│ 寄存器                                                  │ 含义                                                                           │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ MIDR_EL1                                                │ Main ID Register — CPU 厂商、型号、修订号                                      │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ MPIDR_EL1                                               │ Multiprocessor Affinity ID — 核号、cluster 号                                  │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ REVIDR_EL1                                              │ Revision ID                                                                    │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ CTR_EL0                                                 │ Cache Type Register — cache line 大小等                                        │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64PFR0_EL1                                         │ AArch64 Processor Feature Register 0 — 例如有没有 SVE、FP、AdvSIMD、EL3        │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64PFR1_EL1                                         │ 更多处理器特性，比如 MTE、SME、NMI                                             │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64MMFR0/1/2/3/4_EL1                                │ Memory Model Feature Register — 页表能力、ASID 位数、NV、VMID 位数             │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64ISAR0/1/2/3_EL1                                  │ Instruction Set Attribute Register — 指令集扩展，比如 AES、SHA、CRC、LSE、LSFE │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64DFR0/1_EL1                                       │ Debug Feature Register                                                         │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64AFR0/1_EL1                                       │ Auxiliary Feature Register                                                     │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64ZFR0_EL1                                         │ SVE 特性                                                                       │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64FPFR0_EL1                                        │ FP8 特性                                                                       │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_AA64SMFR0_EL1                                        │ SME 特性                                                                       │
├─────────────────────────────────────────────────────────┼────────────────────────────────────────────────────────────────────────────────┤
│ ID_PFR0/1_EL1、ID_DFR0、ID_MMFR0~4、ID_ISAR0~6、MVFR0~2 │ AArch32 对应的特性寄存器                                                       │
└─────────────────────────────────────────────────────────┴────────────────────────────────────────────────────────────────────────────────┘


### access_vm_reg

处理 EL1 虚拟内存控制寄存器的写访问，例如 SCTLR_EL1、TTBR0_EL1、TTBR1_EL1、TCR_EL1、MAIR_EL1、ESR_EL1、FAR_EL1 等。

为什么只有写？ 因为 HCR_EL2.TVM 只 trap EL1 对这些寄存器的写，读是直接在硬件上完成的（除非有其他 trap 位）。
所以 access_vm_reg 一上来就是：
```c
BUG_ON(!p->is_write);
```

3. RAZ / WI / HIDDEN
参考: https://developer.arm.com/documentation/aeg0014/g/Glossary

```c
#define REG_HIDDEN		(1 << 0) /* hidden from userspace and guest */
#define REG_RAZ			(1 << 1) /* RAZ from userspace and guest */
#define REG_USER_WI		(1 << 2) /* WI from userspace only */
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
