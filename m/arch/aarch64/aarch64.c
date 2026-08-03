#include "internal.h"
#include "asm/pgtable.h"
int test_aarch64_init(void)
{
	return 0;
}

int test_aarch64_exit(void)
{
	return 0;
}

static void get_el(void)
{
	u32 kernel_mode = read_sysreg(CurrentEL);
	pr_info("CurrentEL : %x\n", kernel_mode);
}

/*
 * 一共 8 个 cpu ，输出的结果完美符合预期:
 *
 * [  516.058958] mpidr: 80000000
 * [  516.059522] mpidr: 80000001
 * [  516.059896] mpidr: 80000002
 * [  516.060816] mpidr: 80000003
 * [  516.062492] mpidr: 80000004
 * [  516.063569] mpidr: 80000005
 * [  516.065417] mpidr: 80000006
 * [  516.068286] mpidr: 80000007
 */
static void handler(void *data)
{
	u32 mpidr = read_sysreg(MPIDR_EL1);
	pr_info("mpidr: %x\n", mpidr);
}

static void smp_mpidr(void)
{
	int cpu;
	u32 midr = read_sysreg(MIDR_EL1); // 61 1 f 032 0
	u32 implementer = (midr & SMIDR_EL1_IMPLEMENTER) >>
			  SMIDR_EL1_IMPLEMENTER_SHIFT;
	pr_info("midr : %x %x \n", midr, implementer);

	/* 和 cat /proc/cpuinfo 中的内容基本是对应的
	 *
	 * 字段解释参考这个:
	 * https://developer.arm.com/documentation/ddi0595/2020-12/AArch64-Registers/MIDR-EL1--Main-ID-Register
	 *
	 * CPU implementer : 0x61
	 * CPU architecture: 8
	 * CPU variant     : 0x1
	 * CPU part        : 0x032
	 * CPU revision    : 0
	 */

	for_each_possible_cpu(cpu)
		smp_call_function_single(cpu, handler, NULL, 1);
}

/*
 * 测试直接访问高地址 0x3000000000000010
 * 这个地址在 AArch64 的高内存区域，直接访问应该会导致缺页异常或内核崩溃
 *
 * 注意，在 panic 日志中显示 fault 的地址是 0x10 ，因为高位被截断掉了。
 *
 *
 * [ 1378.939948] 尝试访问高地址: 0x3000000000000010
 * [ 1378.940050] Unable to handle kernel access to user memory outside uaccess routines at virtual address 0000000000000010
 * [ 1378.940061] Mem abort info:
 * [ 1378.940062]   ESR = 0x0000000096000004
 * [ 1378.940065]   EC = 0x25: DABT (current EL), IL = 32 bits
 * [ 1378.940067]   SET = 0, FnV = 0
 * [ 1378.940069]   EA = 0, S1PTW = 0
 * [ 1378.940070]   FSC = 0x04: level 0 translation fault
 * [ 1378.940072] Data abort info:
 * [ 1378.940074]   ISV = 0, ISS = 0x00000004, ISS2 = 0x00000000
 * [ 1378.940076]   CM = 0, WnR = 0, TnD = 0, TagAccess = 0
 * [ 1378.940078]   GCS = 0, Overlay = 0, DirtyBit = 0, Xs = 0
 * [ 1378.940080] user pgtable: 4k pages, 48-bit VAs, pgdp=0000000104e49000
 * [ 1378.940083] [0000000000000010] pgd=0000000000000000, p4d=0000000000000000
 * [ 1378.940142] Internal error: Oops: 0000000096000004 [#1]  SMP
 */
static void access_high_addr(void)
{
	u64 addr = 0x3000000000000010ULL;
	u64 value;

	pr_info("尝试访问高地址: 0x%llx\n", addr);

	/*
	 * 直接读取该地址，这应该会触发异常
	 * 注意：这可能会导致内核 oops 或 panic
	 */
	value = *(volatile u64 *)addr;

	pr_info("读取成功，值: 0x%llx\n", value);
}


int test_aarch64(long action)
{
	switch (action) {
	case 0:
		get_el();
		break;
	case 1:
		smp_mpidr();
		break;
	case 2:
		// 如果是 64KiB 页，这个输出为 13
		pr_info("HUGETLB_PAGE_ORDER=%d\n", HUGETLB_PAGE_ORDER);
		break;
	case 3:
		access_high_addr();
		break;
	}
	return 0;
}
