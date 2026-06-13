## 似乎关联的源码
- tools/arch/arm64/include/asm/sysreg.h
	- 其实 tools/arch/arm64/include/ 目录下的内容都有重复
	- 和 arch/arm64/include/asm/sysreg.h 中内容差不多
- arch/arm64/include/asm/sysreg.h
- arch/arm64/kvm/guest.c
- arch/arm64/kvm/sys_regs.c
- arch/arm64/kernel/cpufeature.c


## sys_insn_descs 也是需要仔细看看的了

## sys_reg_descs
<!-- 1b77567b-58f8-4cf1-9a3e-f6c6925550c1 -->

首先，这是 sys reg 的标准定义:
```c
#define SYS_PMMIR_EL1			sys_reg(3, 0, 9, 14, 6)
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

正如注释显示的那样:
- sys_reg_desc::access : 来自 guest 的访问
- sys_reg_desc::get_user / sys_reg_desc::set_user : 来自 user 的访问，也就是 qemu

### 从虚拟机访问 sysreg
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

(这里自然有一个问题，为什么 cp 从有一个单独的 exit handler)

### 除去 sys_reg_descs 之外，还有 cp14_regs cp15_regs cp14_64_regs cp15_64_regs
> [!NOTE]
> 参考 Deepseeek ，有待验证

sys_reg_descs: 這是最通用的AArch64系統暫存器描述表。它包含了標準的64位元系統暫存器定義，這些暫存器可供虛擬機監視器（VMM）進行模擬。
cp14_regs 和 cp15_regs: 這些表格定義了ARM架構中的協處理器14（CP14）和協處理器15（CP15）的32位元暫存器。在ARMv7及更早的架構中，系統控制功能主要透過協處理器來實現。
cp14_64_regs 和 cp15_64_regs: 這兩張表則定義了CP14和CP15的64位元暫存器，主要用於AArch64架構。
sys_insn_descs: 這張表與前述不同，它描述的是系統 指令 而非暫存器。

ARMv7-A/R架構中，CP14和CP15是用於系統控制的協處理器。
- CP15 (System Control Coprocessor): 這是最主要的系統控制協處理器，負責控制快取、記憶體管理單元（MMU）、安全擴展（TrustZone）以及其他系統選項。
cp15_regs 表格定義了這些32位元暫存器的描述，以便在AArch32模式下進行模擬。
- CP14 (Debug, Trace, and Jazelle/ThumbEE Coprocessor): CP14主要提供除錯、追蹤以及Jazelle（Java硬體加速）和ThumbEE執行環境的系統控制暫存器介面。
cp14_regs 表格則定義了這些32位元暫存器的模擬方式。

隨著ARM架構演進到AArch64，許多原本由CP14和CP15提供的功能被整合到系統暫存器中。
然而，為了相容性和擴展性，部分64位元的CP14和CP15暫存器依然存在。cp14_64_regs和cp15_64_regs這兩個表格就是用來定義這些64位元暫存器的描述，以便在AArch64模式下進行模擬

與其他表格不同，sys_insn_descs是用來描述系統 指令 的，例如TLB（Translation Lookaside Buffer）管理相關的指令。當客戶端執行這些特定的系統指令時，KVM會攔截它們，並在sys_insn_descs中查找相應的描述，以執行模擬的操作。這對於虛擬化記憶體管理至關重要。

一共定义了这些 table
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

原来 arm 也是存在 coprocess 的概念:

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

## arm64_ftr_regs

### sys_reg_desc::val 和 struct arm64_ftr_reg 中的各个 value 是什么关系?
```c
static void get_ctr_el0(struct kvm_vcpu *v, const struct sys_reg_desc *r)
{
	((struct sys_reg_desc *)r)->val = read_sanitised_ftr_reg(SYS_CTR_EL0);
}
```

```c
#define ID_DESC_DEFAULT_CALLBACKS		\
	.access	= access_id_reg,		\
	.get_user = get_id_reg,			\
	.set_user = set_id_reg,			\
	.visibility = id_visibility,		\
	.reset = kvm_read_sanitised_id_reg
```

也就是默认为 kvm_read_sanitised_id_reg

这里的代码:
```c
u64 read_sanitised_ftr_reg(u32 id)
{
	struct arm64_ftr_reg *regp = get_arm64_ftr_reg(id);

	if (!regp)
		return 0;
	return regp->sys_val;
}
```

### read_sanitised_ftr_reg 的如何初始化的
似乎发起修改的地方只有 : init_cpu_ftr_reg

- __primary_switched
  - start_kernel
    - smp_prepare_boot_cpu
      - cpuinfo_store_boot_cpu
        - init_cpu_features
          - init_cpu_ftr_reg

```c
static const struct __ftr_reg_entry {
	u32			sys_id;
	struct arm64_ftr_reg 	*reg;
} arm64_ftr_regs[] = {
```

```c
struct arm64_ftr_reg arm64_ftr_reg_ctrel0 = {
	.name		= "SYS_CTR_EL0",
	.ftr_bits	= ftr_ctr,
	.override	= &no_override,
};
```


- kvm_read_sanitised_id_reg
  - __kvm_read_sanitised_id_reg
    - read_sanitised_ftr_reg : 首先从这里获取，然后对于具体的 sysreg 打补丁做修正
    - sanitise_id_aa64pfr0_el1 :

## sys_id 是什么东西?

例如可以看到:
```c
struct arm64_ftr_reg *regp = get_arm64_ftr_reg(sys_id);
```

从 sys_reg_desc 到 sys_id 的转换:
```c
	const struct sys_reg_desc *rd,
	u32 id = reg_to_encoding(rd);

	#define reg_to_encoding(x)						\
	sys_reg((u32)(x)->Op0, (u32)(x)->Op1,				\
		(u32)(x)->CRn, (u32)(x)->CRm, (u32)(x)->Op2)
```

从一个 SYS_ID 的 macro 到 sys_reg 的转换:
```c
#define SYS_ID_DFR0_EL1                                 sys_reg(3, 0, 0, 1, 2)
```

## access_id_reg 和 access_vm_reg 是什么意思?
(容易，以后再来)

## RAZ 的细节考虑
参考: https://developer.arm.com/documentation/aeg0014/g/Glossary

1. RAZ : read as zero
2. WI : write ignore

```c
#define REG_HIDDEN		(1 << 0) /* hidden from userspace and guest */
#define REG_RAZ			(1 << 1) /* RAZ from userspace and guest */
#define REG_USER_WI		(1 << 2) /* WI from userspace only */
```

- [ ] REG_RAZ : 是不是默认包含 WI 的功能的?
- [ ] REG_RAZ 和 REG_USER_WI 的区别是什么？

```c
static unsigned int id_visibility(const struct kvm_vcpu *vcpu,
				  const struct sys_reg_desc *r)
{
	u32 id = reg_to_encoding(r);

	switch (id) {
	case SYS_ID_AA64ZFR0_EL1:
		if (!vcpu_has_sve(vcpu))
			return REG_RAZ;
		break;
	}

	return 0;
}
```
## 继续总结一下吧，感觉还不是很清晰

- https://docs.google.com/document/d/1ILLott1RH9bl4y3e9mbFpfqniTBUKO41GZoFZ0ikxug/edit?tab=t.0#heading=h.3or91njsonsy
- https://docs.google.com/document/d/1g1bcHWlQrrP5__MUPJH1lPay1NxF-ZSD9Y7aF4N5Wt4/edit?tab=t.0#heading=h.wfhi0u4x2tzo
- https://docs.google.com/document/d/1A3OxmnXNmGKuC_m44xMEMknFz3Jykgnuc3-LmfiZNuM/edit?tab=t.0#heading=h.wfhi0u4x2tzo

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
