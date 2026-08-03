#include "internal.h"
#include <asm/sysreg.h>
#include <uapi/linux/kvm.h>
#include <linux/version.h>
#include <linux/smp.h>

struct sys_reg_params {
	u8 Op0;
	u8 Op1;
	u8 CRn;
	u8 CRm;
	u8 Op2;
};

#define reg_to_encoding(x)                                                  \
	sys_reg((u32)(x)->Op0, (u32)(x)->Op1, (u32)(x)->CRn, (u32)(x)->CRm, \
		(u32)(x)->Op2)

// 方便测试，将参数 sys_reg_desc 变为 const struct sys_reg_params
static u64 sys_reg_to_index(const struct sys_reg_params *reg)
{
	return (KVM_REG_ARM64 | KVM_REG_SIZE_U64 | KVM_REG_ARM64_SYSREG |
		(reg->Op0 << KVM_REG_ARM64_SYSREG_OP0_SHIFT) |
		(reg->Op1 << KVM_REG_ARM64_SYSREG_OP1_SHIFT) |
		(reg->CRn << KVM_REG_ARM64_SYSREG_CRN_SHIFT) |
		(reg->CRm << KVM_REG_ARM64_SYSREG_CRM_SHIFT) |
		(reg->Op2 << KVM_REG_ARM64_SYSREG_OP2_SHIFT));
}

static bool index_to_params(u64 id, struct sys_reg_params *params)
{
	switch (id & KVM_REG_SIZE_MASK) {
	case KVM_REG_SIZE_U64:
		/* Any unused index bits means it's not valid. */
		if (id &
		    ~(KVM_REG_ARCH_MASK | KVM_REG_SIZE_MASK |
		      KVM_REG_ARM_COPROC_MASK | KVM_REG_ARM64_SYSREG_OP0_MASK |
		      KVM_REG_ARM64_SYSREG_OP1_MASK |
		      KVM_REG_ARM64_SYSREG_CRN_MASK |
		      KVM_REG_ARM64_SYSREG_CRM_MASK |
		      KVM_REG_ARM64_SYSREG_OP2_MASK))
			return false;
		params->Op0 = ((id & KVM_REG_ARM64_SYSREG_OP0_MASK) >>
			       KVM_REG_ARM64_SYSREG_OP0_SHIFT);
		params->Op1 = ((id & KVM_REG_ARM64_SYSREG_OP1_MASK) >>
			       KVM_REG_ARM64_SYSREG_OP1_SHIFT);
		params->CRn = ((id & KVM_REG_ARM64_SYSREG_CRN_MASK) >>
			       KVM_REG_ARM64_SYSREG_CRN_SHIFT);
		params->CRm = ((id & KVM_REG_ARM64_SYSREG_CRM_MASK) >>
			       KVM_REG_ARM64_SYSREG_CRM_SHIFT);
		params->Op2 = ((id & KVM_REG_ARM64_SYSREG_OP2_MASK) >>
			       KVM_REG_ARM64_SYSREG_OP2_SHIFT);
		return true;
	default:
		return false;
	}
}

static void test_index_to_params(void)
{
	size_t i;
	// 注意，这里完全是对应的
	u64 m[] = { 0x603000000013c039, 0x603000000013c020,
		    0x603000000013d801 };
	int s[] = { SYS_ID_AA64MMFR1_EL1, SYS_ID_AA64PFR0_EL1, SYS_CTR_EL0 };

	for (i = 0; i < ARRAY_SIZE(m); i++) {
		struct sys_reg_params para;
		u64 idx = m[i];
		if (!index_to_params(idx, &para)) {
			pr_info("failed, skip\n");
			continue;
		}
		u64 idx2 = sys_reg_to_index(&para);
		pr_info("%x %x %x %x %x\n", para.Op0, para.Op1, para.CRn,
			para.CRm, para.Op2);
		pr_info("%x %x\n", reg_to_encoding(&para), s[i]);
		pr_info("%llx %llx\n", idx, idx2);
	}
	// [359047.965342] 3 0 0 7 1
	// [359047.993444] 180720 180720
	// [359048.025697] 603000000013c039 603000000013c039
	// [359048.078689] 3 0 0 4 0
	// [359048.106799] 180400 180400
	// [359048.139046] 603000000013c020 603000000013c020
	// [359048.192039] 3 3 0 0 1
	// [359048.220150] 1b0020 1b0020
	// [359048.252399] 603000000013d801 603000000013d801
}

typedef struct arm64_ftr_reg *(*type_get_arm64_ftr_reg)(u32 sys_id);
type_get_arm64_ftr_reg get_reg = (type_get_arm64_ftr_reg)0xffffc122a97436e8;
typedef u64 (*type_read_sanitised_ftr_reg)(u32 id);
type_read_sanitised_ftr_reg read_reg =
	(type_read_sanitised_ftr_reg)0xffff8000800230f0;

// 测试内核对于 sysreg 的屏蔽功能
static void sanitised_val(void)
{
	// 5.4 获取到的:
	// [368065.838455] read : 211122
	// [368065.831153] read : 11111111
	//
	// 4.19 中获取到的
	// [402078.406228] read : 211122
	// [402078.394112] read : 10011111111
	//
	// 如果在 6.14 内核中，没有任何屏蔽的
	// [ 7491.979238] read : 10211122
	// [ 7491.977252] read : 10011111111
	//
	//  6.14 kernel 在虚拟机中观察到的，不过虚拟机中直接 read_reg 也是如此
	//  read : 10211122
	//  read : 1100000011111111
	//
	// 虚拟机的不同应该都是 sanitise_id_aa64pfr0_el1 的效果了
	//
	// 1. read_sanitised_ftr_reg 就是 regp->sys_val
	//
	// 2.
	// #define read_cpuid(reg)			read_sysreg_s(SYS_ ## reg)
	// 所以，这两个方法是等价的
	// u64 x = read_sysreg_s(SYS_ID_AA64MMFR1_EL1);
	// u64 v1 = read_cpuid(ID_AA64MMFR1_EL1);

	size_t i;
	int s[] = { SYS_ID_AA64MMFR1_EL1, SYS_ID_AA64PFR0_EL1,
		    SYS_ID_PFR0_EL1 };
	for (i = 0; i < ARRAY_SIZE(s); i++) {
		struct arm64_ftr_reg *regp;
		u64 san;
		pr_info("ID : %x", s[i]);
		regp = get_reg(s[i]);
		if (!regp) {
			pr_warn("get_arm64_ftr_reg failed for %x\n", s[i]);
			continue;
		}
		san = read_reg(s[i]);
		pr_info("%s %llx %llx %llx\n", regp->name, regp->user_val,
			regp->sys_val, san);
	}

	// 不知道为什么 read_sysreg_s 参数不能是 s[i]
	pr_info("%x %llx\n", SYS_ID_AA64MMFR1_EL1,
		read_sysreg_s(SYS_ID_AA64MMFR1_EL1));
	pr_info("%x %llx\n", SYS_ID_AA64PFR0_EL1,
		read_sysreg_s(SYS_ID_AA64PFR0_EL1));
	pr_info("%x %llx\n", SYS_CTR_EL0, read_sysreg_s(SYS_CTR_EL0));

	// 6.16 内核中测试结果:
	// [27260.726055] ID : 180720
	// [27260.726058] SYS_ID_AA64MMFR1_EL1 0 10211122 10211122
	// [27260.735231] ID : 180400
	// [27260.735234] SYS_ID_AA64PFR0_EL1 11 10011111111 10011111111
	// [27260.744911] ID : 180100
	// [27260.744913] SYS_ID_PFR0_EL1 0 0 0
	// [27260.752421] 180720 10211122
	// [27260.756090] 180400 10011111111
	// [27260.756090] 1b0020 84448004
}

static void fast_test(void)
{
	// 不要写 for loop ，这个 read_sysreg_s 是 macro ，其参数不可以是一个变量
	//
	// [147166.600341] 1b0020 84448004 84448004
	// [147166.643998] 180500 110305408 110305408
	pr_info("%x %llx %llx\n", SYS_CTR_EL0, read_sysreg_s(SYS_CTR_EL0),
		read_sanitised_ftr_reg(SYS_CTR_EL0));
	pr_info("%x %llx %llx\n", SYS_ID_AA64DFR0_EL1,
		read_sysreg_s(SYS_ID_AA64DFR0_EL1),
		read_sanitised_ftr_reg(SYS_ID_AA64DFR0_EL1));
}

// 对比当前 CPU 硬件原始值和内核 sanitized 系统统一值
// 注意：read_sysreg_s 是 macro，参数必须是字面量，不能用变量
//
// [152796.585013] === compare read_sysreg_s() vs read_sanitised_ftr_reg() ===
// [152796.664957] '*' 表示两者不同；同构无 cmdline override 的系统上通常全为 0
// [152796.764598]   SYS_ID_AA64PFR0_EL1    hw=0x0000010011111111 san=0x0000010011111111 diff=0x0000000000000000
// [152796.879795]   SYS_ID_AA64PFR1_EL1    hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152796.994984]   SYS_ID_AA64PFR2_EL1    hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152797.110178]   SYS_ID_AA64DFR0_EL1    hw=0x0000000110305408 san=0x0000000110305408 diff=0x0000000000000000
// [152797.225372]   SYS_ID_AA64DFR1_EL1    hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152797.340574]   SYS_ID_AA64ISAR0_EL1   hw=0x0001100010211120 san=0x0001100010211120 diff=0x0000000000000000
// [152797.455765]   SYS_ID_AA64ISAR1_EL1   hw=0x0000000000011001 san=0x0000000000011001 diff=0x0000000000000000
// [152797.570960]   SYS_ID_AA64ISAR2_EL1   hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152797.686152]   SYS_ID_AA64ISAR3_EL1   hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152797.801347]   SYS_ID_AA64MMFR0_EL1   hw=0x0000000000101125 san=0x0000000000101125 diff=0x0000000000000000
// [152797.916540]   SYS_ID_AA64MMFR1_EL1   hw=0x0000000010211122 san=0x0000000010211122 diff=0x0000000000000000
// [152798.031735]   SYS_ID_AA64MMFR2_EL1   hw=0x0000000000001011 san=0x0000000000001011 diff=0x0000000000000000
// [152798.146932]   SYS_ID_AA64MMFR3_EL1   hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152798.262122]   SYS_ID_AA64ZFR0_EL1    hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152798.377314]   SYS_ID_AA64SMFR0_EL1   hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152798.492501]   SYS_ID_AA64FPFR0_EL1   hw=0x0000000000000000 san=0x0000000000000000 diff=0x0000000000000000
// [152798.607689]   SYS_CTR_EL0            hw=0x0000000084448004 san=0x0000000084448004 diff=0x0000000000000000
//
// 也是挑选的几个寄存器基本 sanitised_val 和寄存器中本来的都是相同的
// 如果出现不同的确是需要特殊条件，例如:
// 1. 内核启动参数禁用
// 2. 异构 CPU (多个 CPU 中能力不同)
static void compare_hw_vs_sanitised(void)
{
#define COMPARE_REG(reg)                                                        \
	do {                                                                    \
		u64 hw = read_sysreg_s(reg);                                    \
		u64 san = read_sanitised_ftr_reg(reg);                          \
		char diff = (hw != san) ? '*' : ' ';                            \
		pr_info("%c %-22s hw=0x%016llx san=0x%016llx diff=0x%016llx\n", \
			diff, #reg, hw, san, hw ^ san);                         \
	} while (0)

	pr_info("=== compare read_sysreg_s() vs read_sanitised_ftr_reg() ===\n");
	pr_info("'*' 表示两者不同；同构无 cmdline override 的系统上通常全为 0\n");

	COMPARE_REG(SYS_ID_AA64PFR0_EL1);
	COMPARE_REG(SYS_ID_AA64PFR1_EL1);
	COMPARE_REG(SYS_ID_AA64PFR2_EL1);
	COMPARE_REG(SYS_ID_AA64DFR0_EL1);
	COMPARE_REG(SYS_ID_AA64DFR1_EL1);
	COMPARE_REG(SYS_ID_AA64ISAR0_EL1);
	COMPARE_REG(SYS_ID_AA64ISAR1_EL1);
	COMPARE_REG(SYS_ID_AA64ISAR2_EL1);
	COMPARE_REG(SYS_ID_AA64ISAR3_EL1);
	COMPARE_REG(SYS_ID_AA64MMFR0_EL1);
	COMPARE_REG(SYS_ID_AA64MMFR1_EL1);
	COMPARE_REG(SYS_ID_AA64MMFR2_EL1);
	COMPARE_REG(SYS_ID_AA64MMFR3_EL1);
	COMPARE_REG(SYS_ID_AA64ZFR0_EL1);
	COMPARE_REG(SYS_ID_AA64SMFR0_EL1);
	COMPARE_REG(SYS_ID_AA64FPFR0_EL1);
	COMPARE_REG(SYS_CTR_EL0);

#undef COMPARE_REG
}

// 在每个 online CPU 上读取 ID_AA64MMFR1_EL1 硬件值
static void read_mmfr1_on_cpu(void *info)
{
	u64 val = read_sysreg_s(SYS_ID_AA64MMFR1_EL1);
	pr_info("CPU%-3d ID_AA64MMFR1_EL1 = 0x%016llx\n", smp_processor_id(), val);
}

static void per_cpu_mmfr1_test(void)
{
	int cpu;
	u64 sys_san = read_sanitised_ftr_reg(SYS_ID_AA64MMFR1_EL1);

	pr_info("=== per-CPU ID_AA64MMFR1_EL1 hardware value ===\n");
	pr_info("system sanitised value = 0x%016llx\n", sys_san);

	for_each_online_cpu(cpu) {
		smp_call_function_single(cpu, read_mmfr1_on_cpu, NULL, 1);
	}
}

int test_sysreg(long action)
{
	switch (action) {
	case 1:
		fast_test();
		break;
	case 2:
		test_index_to_params();
		break;
	case 3:
		compare_hw_vs_sanitised();
		break;
	case 5:
		per_cpu_mmfr1_test();
		break;
	case 100:
		// 注意，需要提前通过 /proc/kallsyms 修改 get_reg 的地址
		sanitised_val();
		break;
	}
	return 0;
}
