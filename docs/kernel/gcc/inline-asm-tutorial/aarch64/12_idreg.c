/*
 * 示例 12: ARM64 ID 寄存器 (系统寄存器读取)
 *
 * 知识点:
 * - MRS 指令: 读取系统寄存器
 * - ID 寄存器: MIDR_EL1, MPIDR_EL1, REVIDR_EL1
 * - 特性检测: ID_AA64* 寄存器
 * - 缓存信息: CTR_EL0, CLIDR_EL1, CCSIDR_EL1
 * - 实际应用: 特性检测、优化选择
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

/* ID 寄存器读取辅助宏 */
#define READ_SYSREG(name) ({ \
	uint64_t _val; \
	__asm__ __volatile__("mrs %0, " #name : "=r" (_val)); \
	_val; \
})

/* 主 ID 寄存器 */
static inline uint64_t read_midr_el1(void)
{
	return READ_SYSREG(midr_el1);
}

static inline uint64_t read_mpidr_el1(void)
{
	return READ_SYSREG(mpidr_el1);
}

static inline uint64_t read_revidr_el1(void)
{
	return READ_SYSREG(revidr_el1);
}

/* 缓存类型寄存器 */
static inline uint64_t read_ctr_el0(void)
{
	return READ_SYSREG(ctr_el0);
}

/* 特性检测寄存器 (AArch64) */
static inline uint64_t read_id_aa64pfr0_el1(void)
{
	return READ_SYSREG(id_aa64pfr0_el1);
}

static inline uint64_t read_id_aa64isar0_el1(void)
{
	return READ_SYSREG(id_aa64isar0_el1);
}

static inline uint64_t read_id_aa64mmfr0_el1(void)
{
	return READ_SYSREG(id_aa64mmfr0_el1);
}

/* 获取实现者 (Implementer) */
static const char *get_implementer(uint64_t midr)
{
	uint8_t implementer = (midr >> 24) & 0xFF;

	switch (implementer) {
	case 0x41: return "ARM";
	case 0x42: return "Broadcom";
	case 0x43: return "Cavium";
	case 0x44: return "DEC";
	case 0x46: return "Fujitsu";
	case 0x48: return "HiSilicon";
	case 0x49: return "Infineon";
	case 0x4D: return "Motorola";
	case 0x4E: return "NVIDIA";
	case 0x50: return "APM";
	case 0x51: return "Qualcomm";
	case 0x53: return "Samsung";
	case 0x56: return "Marvell";
	case 0x61: return "Apple";
	case 0x66: return "Faraday";
	case 0x68: return "HXT";
	default: return "Unknown";
	}
}

/* 获取架构名称 */
static const char *get_architecture(uint64_t midr)
{
	uint8_t arch = (midr >> 16) & 0xF;

	switch (arch) {
	case 0x0: return "ARMv4";
	case 0x1: return "ARMv4T";
	case 0x2: return "ARMv5";
	case 0x3: return "ARMv5T";
	case 0x4: return "ARMv5TE";
	case 0x5: return "ARMv5TEJ";
	case 0x6: return "ARMv6";
	case 0x7: return "ARMv6T2";
	case 0xF: return "ARMv7+ or defined by ID registers";
	default: return "Unknown";
	}
}

/* 解析 CTR_EL0 缓存信息 */
static void print_cache_info(void)
{
	uint64_t ctr = read_ctr_el0();

	printf("\n=== Cache Information (CTR_EL0) ===\n");

	/* IminLine: 指令缓存最小行大小 */
	uint32_t iminline = 4 << ((ctr >> 0) & 0xF);
	printf("Instruction cache line size: %u bytes\n", iminline);

	/* DminLine: 数据缓存最小行大小 */
	uint32_t dminline = 4 << ((ctr >> 16) & 0xF);
	printf("Data cache line size: %u bytes\n", dminline);

	/* L1Ip: L1 指令缓存策略 */
	uint8_t l1ip = (ctr >> 14) & 0x3;
	const char *l1ip_str;
	switch (l1ip) {
	case 0: l1ip_str = "VIPT (VPIPT on ARMv8.2+)"; break;
	case 1: l1ip_str = "AIVIVT (obsolete)"; break;
	case 2: l1ip_str = "VIPT"; break;
	case 3: l1ip_str = "PIPT"; break;
	default: l1ip_str = "Unknown";
	}
	printf("L1 instruction cache policy: %s\n", l1ip_str);

	/* CWG: Cache Writeback Granule */
	uint32_t cwg = (ctr >> 24) & 0xF;
	if (cwg != 0)
		printf("Cache writeback granule: %u words (16 bytes each)\n", cwg);

	/* ERG: Exclusive Reservation Granule */
	uint32_t erg = (ctr >> 20) & 0xF;
	if (erg != 0)
		printf("Exclusive reservation granule: %u words (16 bytes each)\n", erg);
}

/* 特性检测 */
static void print_features(void)
{
	uint64_t isar0 = read_id_aa64isar0_el1();

	printf("\n=== Feature Detection (ID_AA64ISAR0_EL1) ===\n");

	/* AES: AES 指令支持 */
	uint8_t aes = (isar0 >> 4) & 0xF;
	printf("AES: %s\n", aes ? "Yes" : "No");

	/* PMULL: PMULL/PMULL2 支持 */
	uint8_t pmull = (isar0 >> 4) & 0xF;
	printf("PMULL: %s\n", pmull >= 2 ? "Yes (PMULL/PMULL2)" :
	                    pmull == 1 ? "Yes (PMULL only)" : "No");

	/* SHA1: SHA1 指令支持 */
	uint8_t sha1 = (isar0 >> 8) & 0xF;
	printf("SHA1: %s\n", sha1 ? "Yes" : "No");

	/* SHA2: SHA2 指令支持 */
	uint8_t sha2 = (isar0 >> 12) & 0xF;
	printf("SHA2-256: %s\n", sha2 ? "Yes" : "No");
	printf("SHA2-512: %s\n", sha2 >= 2 ? "Yes" : "No");

	/* CRC32: CRC32 指令支持 */
	uint8_t crc32 = (isar0 >> 16) & 0xF;
	printf("CRC32: %s\n", crc32 ? "Yes" : "No");

	/* Atomic: 原子指令支持 */
	uint8_t atomic = (isar0 >> 20) & 0xF;
	printf("Atomic instructions (CAS/SWP/LDADD): %s\n",
	       atomic ? "Yes" : "No");

	/* RDM: Rounding Double Multiply 支持 */
	uint8_t rdm = (isar0 >> 28) & 0xF;
	printf("SQRDMLAH/SQRDMLSH: %s\n", rdm ? "Yes" : "No");

	/* SHA3: SHA3 指令支持 */
	uint8_t sha3 = (isar0 >> 32) & 0xF;
	printf("SHA3: %s\n", sha3 ? "Yes" : "No");

	/* SM3: SM3 指令支持 */
	uint8_t sm3 = (isar0 >> 36) & 0xF;
	printf("SM3: %s\n", sm3 ? "Yes" : "No");

	/* SM4: SM4 指令支持 */
	uint8_t sm4 = (isar0 >> 40) & 0xF;
	printf("SM4: %s\n", sm4 ? "Yes" : "No");

	/* DP: Dot Product 支持 */
	uint8_t dp = (isar0 >> 44) & 0xF;
	printf("UDOT/SDOT: %s\n", dp ? "Yes" : "No");
}

/* 读取当前异常级别 (只能在内核中读取，这里仅作演示) */
#if 0
static inline uint64_t read_current_el(void)
{
	return READ_SYSREG(currentel);
}
#endif

int main(void)
{
	/* ===== 测试 1: 主 ID 寄存器 ===== */
	uint64_t midr = read_midr_el1();
	uint64_t mpidr = read_mpidr_el1();
	uint64_t revidr = read_revidr_el1();

	printf("=== Processor Identification ===\n");
	printf("MIDR_EL1:  0x%016" PRIx64 "\n", midr);
	printf("MPIDR_EL1: 0x%016" PRIx64 "\n", mpidr);
	printf("REVIDR_EL1: 0x%016" PRIx64 "\n", revidr);

	/* 解析 MIDR */
	printf("\n=== CPU Details ===\n");
	printf("Implementer: %s (0x%02x)\n",
	       get_implementer(midr), (unsigned)((midr >> 24) & 0xFF));
	printf("Architecture: %s\n", get_architecture(midr));
	printf("Variant: %u\n", (unsigned)((midr >> 20) & 0xF));
	printf("Part Number: 0x%03x\n", (unsigned)((midr >> 4) & 0xFFF));
	printf("Revision: %u\n", (unsigned)(midr & 0xF));

	/* 解析 MPIDR */
	printf("\n=== Multiprocessor Affinity ===\n");
	printf("Aff3: %u\n", (unsigned)((mpidr >> 32) & 0xFF));
	printf("Aff2: %u\n", (unsigned)((mpidr >> 16) & 0xFF));
	printf("Aff1: %u\n", (unsigned)((mpidr >> 8) & 0xFF));
	printf("Aff0: %u (CPU ID)\n", (unsigned)(mpidr & 0xFF));
	printf("MT: %s\n", (mpidr >> 24) & 1 ? "Multi-threading" : "Single-threaded");
	printf("U: %s\n", (mpidr >> 30) & 1 ? "Uniprocessor" : "Multiprocessor");

	/* ===== 测试 2: 缓存信息 ===== */
	print_cache_info();

	/* ===== 测试 3: 特性检测 ===== */
	print_features();

	/* ===== 测试 4: MMU 特性 ===== */
	uint64_t mmfr0 = read_id_aa64mmfr0_el1();
	printf("\n=== MMU Features (ID_AA64MMFR0_EL1) ===\n");
	printf("PARange: %u bits\n", (unsigned)((mmfr0 >> 0) & 0xF) * 4 + 32);

	/* ===== 测试 5: 处理器特性 ===== */
	uint64_t pfr0 = read_id_aa64pfr0_el1();
	printf("\n=== Processor Features (ID_AA64PFR0_EL1) ===\n");
	printf("EL0: %s\n",
	       ((pfr0 >> 0) & 0xF) == 0 ? "None" :
	       ((pfr0 >> 0) & 0xF) == 1 ? "AArch64 only" :
	       ((pfr0 >> 0) & 0xF) == 2 ? "AArch64 + AArch32" : "Unknown");
	printf("EL1: %s\n",
	       ((pfr0 >> 4) & 0xF) == 0 ? "None" :
	       ((pfr0 >> 4) & 0xF) == 1 ? "AArch64 only" :
	       ((pfr0 >> 4) & 0xF) == 2 ? "AArch64 + AArch32" : "Unknown");
	printf("FP: %s\n", ((pfr0 >> 16) & 0xF) == 0 ? "Yes" : "No");
	printf("AdvSIMD: %s\n", ((pfr0 >> 20) & 0xF) == 0 ? "Yes" : "No");
	printf("GIC: %s\n", ((pfr0 >> 24) & 0xF) ? "Yes" : "No");

	printf("\n=== All ID register tests passed! ===\n");
	return 0;
}
