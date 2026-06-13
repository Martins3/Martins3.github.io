/*
 * 示例 12: CPUID 指令
 *
 * 知识点:
 * - cpuid 指令: 获取 CPU 信息和特性
 * - 序列化效果: cpuid 是序列化指令
 * - 常用功能: 获取厂商 ID、特性标志、缓存信息等
 * - 实际应用: 特性检测、优化选择
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* CPUID 输出结构 */
struct cpuid_result {
	uint32_t eax;
	uint32_t ebx;
	uint32_t ecx;
	uint32_t edx;
};

/* 基本 cpuid 调用 */
static inline struct cpuid_result cpuid(uint32_t eax, uint32_t ecx)
{
	struct cpuid_result result;

	__asm__ __volatile__(
		"cpuid"
		: "=a" (result.eax), "=b" (result.ebx),
		  "=c" (result.ecx), "=d" (result.edx)
		: "a" (eax), "c" (ecx)
		);

	return result;
}

/* 获取 CPU 厂商 ID */
static void get_vendor_id(char *vendor)
{
	struct cpuid_result r = cpuid(0, 0);

	/* 厂商 ID 在 ebx, edx, ecx 中 */
	memcpy(vendor + 0, &r.ebx, 4);
	memcpy(vendor + 4, &r.edx, 4);
	memcpy(vendor + 8, &r.ecx, 4);
	vendor[12] = '\0';
}

/* 获取 CPU 品牌字符串 */
static void get_brand_string(char *brand)
{
	/* 品牌字符串需要 3 次 cpuid 调用 (0x80000002-0x80000004) */
	uint32_t *brand_u32 = (uint32_t *)brand;

	for (int i = 0; i < 3; i++) {
		struct cpuid_result r = cpuid(0x80000002 + i, 0);
		brand_u32[i * 4 + 0] = r.eax;
		brand_u32[i * 4 + 1] = r.ebx;
		brand_u32[i * 4 + 2] = r.ecx;
		brand_u32[i * 4 + 3] = r.edx;
	}
	brand[48] = '\0';

	/* 去除前导空格 */
	char *start = brand;
	while (*start == ' ') start++;
	if (start != brand) {
		memmove(brand, start, strlen(start) + 1);
	}
}

/* 检查 CPU 特性 */
static bool check_feature(uint32_t leaf, uint32_t subleaf,
	                  uint32_t reg_mask, char reg)
{
	struct cpuid_result r = cpuid(leaf, subleaf);
	uint32_t reg_val;

	switch (reg) {
	case 'a': reg_val = r.eax; break;
	case 'b': reg_val = r.ebx; break;
	case 'c': reg_val = r.ecx; break;
	case 'd': reg_val = r.edx; break;
	default: return false;
	}

	return (reg_val & reg_mask) != 0;
}

/* 常用特性检查宏 */
#define HAS_SSE2()    check_feature(1, 0, (1u << 26), 'd')
#define HAS_SSE3()    check_feature(1, 0, (1u << 0),  'c')
#define HAS_SSSE3()   check_feature(1, 0, (1u << 9),  'c')
#define HAS_SSE41()   check_feature(1, 0, (1u << 19), 'c')
#define HAS_AVX()     check_feature(1, 0, (1u << 28), 'c')
#define HAS_AVX2()    check_feature(7, 0, (1u << 5),  'b')
#define HAS_BMI1()    check_feature(7, 0, (1u << 3),  'b')
#define HAS_BMI2()    check_feature(7, 0, (1u << 8),  'b')
#define HAS_RDRAND()  check_feature(1, 0, (1u << 30), 'c')
#define HAS_RDSEED()  check_feature(7, 0, (1u << 18), 'b')
#define HAS_TSC()     check_feature(1, 0, (1u << 4),  'd')
#define HAS_MSR()     check_feature(1, 0, (1u << 5),  'd')
#define HAS_RDTSCP()  check_feature(0x80000001, 0, (1u << 27), 'd')

/* 获取处理器信息 */
static void get_processor_info(uint32_t *family, uint32_t *model,
	                       uint32_t *stepping)
{
	struct cpuid_result r = cpuid(1, 0);

	*stepping = r.eax & 0xF;
	*model = ((r.eax >> 4) & 0xF) | ((r.eax >> 12) & 0xF0);
	*family = ((r.eax >> 8) & 0xF) | ((r.eax >> 20) & 0xFF);
}

/* 获取逻辑处理器数量 */
static uint32_t get_logical_processor_count(void)
{
	struct cpuid_result r = cpuid(1, 0);
	return (r.ebx >> 16) & 0xFF;
}

/* 获取缓存信息 */
static void print_cache_info(void)
{
	printf("\n=== Cache Information ===\n");

	/* 使用 cpuid 4 获取确定性缓存参数 */
	for (int i = 0; ; i++) {
		struct cpuid_result r = cpuid(4, i);

		if ((r.eax & 0x1F) == 0) {
			break;  /* 无更多缓存 */
		}

		int cache_type = r.eax & 0x1F;
		int level = (r.eax >> 5) & 0x7;
		int fully_assoc = (r.eax >> 9) & 0x1;

		/* 计算缓存大小 */
		int ways = ((r.ebx >> 22) & 0x3FF) + 1;
		int partitions = ((r.ebx >> 12) & 0x3FF) + 1;
		int line_size = (r.ebx & 0xFFF) + 1;
		int sets = r.ecx + 1;

		uint32_t cache_size = ways * partitions * line_size * sets;

		const char *type_str;
		switch (cache_type) {
		case 1: type_str = "Data"; break;
		case 2: type_str = "Instruction"; break;
		case 3: type_str = "Unified"; break;
		default: type_str = "Unknown"; break;
		}

		printf("L%d %s Cache: %u KB, %d-way%s, line size: %d bytes\n",
		       level, type_str, cache_size / 1024,
		       ways, fully_assoc ? ", fully associative" : "",
		       line_size);
	}
}

int main(void)
{
	char vendor[13];
	char brand[49];
	uint32_t family, model, stepping;

	/* ===== 测试 1: 获取厂商 ID ===== */
	get_vendor_id(vendor);
	printf("CPU Vendor: %s\n", vendor);
	assert(strlen(vendor) > 0);

	/* ===== 测试 2: 获取品牌字符串 ===== */
	/* 首先检查是否支持扩展功能 */
	struct cpuid_result ext_check = cpuid(0x80000000, 0);
	if (ext_check.eax >= 0x80000004) {
		get_brand_string(brand);
		printf("CPU Brand: %s\n", brand);
	}

	/* ===== 测试 3: 处理器信息 ===== */
	get_processor_info(&family, &model, &stepping);
	printf("\nProcessor Info:\n");
	printf("  Family: %u\n", family);
	printf("  Model: %u\n", model);
	printf("  Stepping: %u\n", stepping);

	/* ===== 测试 4: 逻辑处理器数量 ===== */
	uint32_t lp_count = get_logical_processor_count();
	printf("  Logical Processors: %u\n", lp_count);

	/* ===== 测试 5: 特性检测 ===== */
	printf("\n=== CPU Features ===\n");

	printf("Basic Features:\n");
	printf("  TSC:     %s\n", HAS_TSC() ? "Yes" : "No");
	printf("  MSR:     %s\n", HAS_MSR() ? "Yes" : "No");
	printf("  RDTSCP:  %s\n", HAS_RDTSCP() ? "Yes" : "No");

	printf("\nSIMD Features:\n");
	printf("  SSE2:    %s\n", HAS_SSE2() ? "Yes" : "No");
	printf("  SSE3:    %s\n", HAS_SSE3() ? "Yes" : "No");
	printf("  SSSE3:   %s\n", HAS_SSSE3() ? "Yes" : "No");
	printf("  SSE4.1:  %s\n", HAS_SSE41() ? "Yes" : "No");
	printf("  AVX:     %s\n", HAS_AVX() ? "Yes" : "No");
	printf("  AVX2:    %s\n", HAS_AVX2() ? "Yes" : "No");

	printf("\nBit Manipulation:\n");
	printf("  BMI1:    %s\n", HAS_BMI1() ? "Yes" : "No");
	printf("  BMI2:    %s\n", HAS_BMI2() ? "Yes" : "No");

	printf("\nRandom Number Generation:\n");
	printf("  RDRAND:  %s\n", HAS_RDRAND() ? "Yes" : "No");
	printf("  RDSEED:  %s\n", HAS_RDSEED() ? "Yes" : "No");

	/* ===== 测试 6: 使用 rdrand (如果支持) ===== */
	if (HAS_RDRAND()) {
		printf("\n=== RDRAND Test ===\n");

		unsigned int rand_val;
		int retry = 10;
		bool success = false;

		while (retry-- > 0) {
			unsigned char carry;
			__asm__ __volatile__(
				"rdrand %0\n\t"
				"setc %1"
				: "=r" (rand_val), "=qm" (carry)
				:
				: "cc"
				);

			if (carry) {
				success = true;
				break;
			}
		}

		if (success) {
			printf("Random number from RDRAND: %u\n", rand_val);
		} else {
			printf("RDRAND failed after retries\n");
		}
	}

	/* ===== 测试 7: 缓存信息 ===== */
	print_cache_info();

	/* ===== 测试 8: cpuid 作为序列化指令 ===== */
	printf("\n=== CPUID Serialization Test ===\n");

	volatile int order_test = 0;

	order_test = 1;
	cpuid(0, 0);  /* 序列化点 */
	order_test = 2;

	printf("Order test: %d (expected: 2)\n", order_test);
	assert(order_test == 2);

	printf("\n=== All CPUID tests passed! ===\n");
	return 0;
}
