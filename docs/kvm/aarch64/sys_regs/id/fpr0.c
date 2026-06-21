/*
 *
 * # aarch64 fpr0
 * <!-- ad3e0932-f084-4396-a570-3e4dbc383186 -->
 *
 * mac 上的执行结果:
 *
 * === ID_AA64PFR0_EL1 (AArch64 Features) ===
 * Raw: 0x0001000000110011
 * EL0:                  AArch64 supported (1)
 * EL1:                  AArch64 supported (1)
 * EL2:                  Not supported (0)
 * EL3:                  Not supported (0)
 * FP:                   ARMv8.0 FP/SIMD (1)
 * AdvSIMD:              ARMv8.0 FP/SIMD (1)
 * GIC CPU interface:    None (0)
 * RAS Extension:        Not supported (0)
 * SVE:                  Not supported (0)
 * SEL2 (Secure EL2):    Not supported (0)
 * MPAM:                 Not supported (0)
 * AMU (Activity Mon):   Not supported (0)
 *
 * === ID_PFR0_EL1 ===
 * Access failed: CPU does not implement AArch32 feature registers (pure AArch64).
 *
 * kunpeng 上的执行结果:
 *
 * === ID_AA64PFR0_EL1 (AArch64 Features) ===
 * Raw: 0x0000000000110011
 * EL0:                  AArch64 supported (1)
 * EL1:                  AArch64 supported (1)
 * EL2:                  Not supported (0)
 * EL3:                  Not supported (0)
 * FP:                   ARMv8.0 FP/SIMD (1)
 * AdvSIMD:              ARMv8.0 FP/SIMD (1)
 * GIC CPU interface:    None (0)
 * RAS Extension:        Not supported (0)
 * SVE:                  Not supported (0)
 * SEL2 (Secure EL2):    Not supported (0)
 * MPAM:                 Not supported (0)
 * AMU (Activity Mon):   Not supported (0)
 *
 * === ID_PFR0_EL1 ===
 * Access failed: CPU does not implement AArch32 feature registers (pure AArch64).
 *
 */
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>

static jmp_buf sigill_jmp;

static void sigill_handler(int sig)
{
	(void)sig;
	longjmp(sigill_jmp, 1);
}

// Helper to extract 4-bit field at given bit offset
static inline uint8_t get_field_4bit(uint64_t val, int offset)
{
	return (val >> offset) & 0xF;
}

// Decode ID_AA64PFR0_EL1 (AArch64 features)
static void decode_id_aa64pfr0(uint64_t val)
{
	printf("\n=== ID_AA64PFR0_EL1 (AArch64 Features) ===\n");
	printf("Raw: 0x%016lx\n", val);

	uint8_t el0 = get_field_4bit(val, 0);
	uint8_t el1 = get_field_4bit(val, 4);
	uint8_t el2 = get_field_4bit(val, 8);
	uint8_t el3 = get_field_4bit(val, 12);
	uint8_t fp = get_field_4bit(val, 16);
	uint8_t advsimd = get_field_4bit(val, 20);
	uint8_t gic = get_field_4bit(val, 24);
	uint8_t ras = get_field_4bit(val, 28);
	uint8_t sve = get_field_4bit(val, 32);
	uint8_t sel2 = get_field_4bit(val, 36);
	uint8_t mpam = get_field_4bit(val, 40);
	uint8_t amu = get_field_4bit(val, 44);

	printf("EL0:                  %s (%u)\n",
	       el0 ? "AArch64 supported" : "Not supported", el0);
	printf("EL1:                  %s (%u)\n",
	       el1 ? "AArch64 supported" : "Not supported", el1);
	printf("EL2:                  %s (%u)\n",
	       el2 ? "AArch64 supported" : "Not supported", el2);
	printf("EL3:                  %s (%u)\n",
	       el3 ? "AArch64 supported" : "Not supported", el3);

	const char *fp_simd_str[] = {
		"Not implemented", "ARMv8.0 FP/SIMD",
		"ARMv8.2 +FP16",   "ARMv8.6 +BF16",
		"Reserved",	   "Reserved",
		"Reserved",	   "Reserved",
		"Reserved",	   "Reserved",
		"Reserved",	   "Reserved",
		"Reserved",	   "Reserved",
		"Reserved",	   "Implementation specific"
	};
	printf("FP:                   %s (%u)\n",
	       fp < 4 ? fp_simd_str[fp] : "Reserved", fp);
	printf("AdvSIMD:              %s (%u)\n",
	       advsimd < 4 ? fp_simd_str[advsimd] : "Reserved", advsimd);

	printf("GIC CPU interface:    %s (%u)\n",
	       gic ? "System register interface" : "None", gic);
	printf("RAS Extension:        %s (%u)\n",
	       ras ? (ras == 1 ?
			      "ARMv8.2" :
			      (ras == 2 ? "ARMv8.4" :
					  (ras == 3 ? "ARMv8.6" : "Unknown"))) :
		     "Not supported",
	       ras);

	// Fix: remove unused sve_str, use inline logic
	if (sve == 0) {
		printf("SVE:                  Not supported (%u)\n", sve);
	} else if (sve == 1) {
		printf("SVE:                  SVEv1 supported (%u)\n", sve);
	} else if (sve == 2) {
		printf("SVE:                  SVEv1 + SVE2 (%u)\n", sve);
	} else {
		printf("SVE:                  SVE version %u (>=3)\n", sve);
	}

	printf("SEL2 (Secure EL2):    %s (%u)\n",
	       sel2 ? "Supported" : "Not supported", sel2);
	printf("MPAM:                 %s (%u)\n",
	       mpam ? "Supported" : "Not supported", mpam);
	printf("AMU (Activity Mon):   %s (%u)\n",
	       amu ? "Supported" : "Not supported", amu);
}

int main(void)
{
	uint64_t aa64pfr0;

	// Always safe to read ID_AA64PFR0_EL1
	asm volatile("mrs %0, ID_AA64PFR0_EL1" : "=r"(aa64pfr0));
	decode_id_aa64pfr0(aa64pfr0);

	// Try to read ID_PFR0_EL1 only if AArch32 EL0 is supported
	uint8_t aarch32_el0_supported =
		(aa64pfr0 & 0xF) !=
		0; // If EL0 in AA64 mode is supported, AArch32 *might* be

	if (!aarch32_el0_supported) {
		printf("\n=== ID_PFR0_EL1 ===\n");
		printf("Skipped: AArch32 not supported (EL0=0 in ID_AA64PFR0_EL1).\n");
		return 0;
	}

	// PFR0_EL1 访问直接 panic 了
	struct sigaction sa, old_sa;
	sa.sa_handler = sigill_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGILL, &sa, &old_sa);

	uint64_t pfr0;
	if (setjmp(sigill_jmp) == 0) {
		asm volatile("mrs %0, ID_PFR0_EL1" : "=r"(pfr0));
		printf("never reach here\n");
	} else {
		printf("\n=== ID_PFR0_EL1 ===\n");
		printf("Access failed: CPU does not implement AArch32 feature registers "
		       "(pure AArch64).\n");
	}

	sigaction(SIGILL, &old_sa, NULL);
	return 0;
}
