/*
 * kernel doc:
 * https://www.kernel.org/doc/html/latest/arch/arm64/cpu-feature-registers.html
 */
#include <asm/hwcap.h>
#include <stdio.h>
#include <sys/auxv.h>

// 从 linux/arch/arm64/include/generated/asm/sysreg-defs.h 拷贝一些字段过来
#define ID_AA64ISAR1_EL1_LRCPC_SHIFT 20

#define get_cpu_ftr(id)                                  \
	({                                               \
		unsigned long __val;                     \
		asm("mrs %0, " #id : "=r"(__val));       \
		printf("%-20s: 0x%016lx\n", #id, __val); \
		__val;                                   \
	})

int main(void)
{
	if (!(getauxval(AT_HWCAP) & HWCAP_CPUID)) {
		fputs("CPUID registers unavailable\n", stderr);
		return 1;
	}

	unsigned long val;
	val = get_cpu_ftr(ID_AA64ISAR0_EL1);
	val = get_cpu_ftr(ID_AA64ISAR1_EL1);
	// 不太理解，m2 上这个输出是 2 ，但是手册规定，要么是 0 要么是 1
	printf("%lx\n", (val >> ID_AA64ISAR1_EL1_LRCPC_SHIFT) & 0xf);
	get_cpu_ftr(ID_AA64MMFR0_EL1);
	get_cpu_ftr(ID_AA64MMFR1_EL1);
	get_cpu_ftr(ID_AA64PFR0_EL1);
	get_cpu_ftr(ID_AA64PFR1_EL1);
	get_cpu_ftr(ID_AA64DFR0_EL1);
	get_cpu_ftr(ID_AA64DFR1_EL1);

	get_cpu_ftr(MIDR_EL1);
	get_cpu_ftr(MPIDR_EL1);
	get_cpu_ftr(REVIDR_EL1);

#if 0
      /* Unexposed register access causes SIGILL */
      get_cpu_ftr(ID_MMFR0_EL1);
#endif

	return 0;
}
