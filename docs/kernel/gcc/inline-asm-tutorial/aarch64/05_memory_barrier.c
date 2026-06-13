/*
 * 示例 05: 内存屏障 (Memory Barriers) (ARM64/AArch64)
 *
 * 知识点:
 * - 编译器屏障: 防止编译器重排序内存操作
 * - ARM64 内存屏障指令: dmb (Data Memory Barrier)
 *                       dsb (Data Synchronization Barrier)
 *                       isb (Instruction Synchronization Barrier)
 * - 内存 clobber ("memory") 的作用
 */

#include <stdio.h>
#include <assert.h>

/* 简单的编译器屏障 */
#define compiler_barrier() __asm__ __volatile__("" ::: "memory")

/* ARM64 内存屏障指令 */
#define dmb() __asm__ __volatile__("dmb sy" ::: "memory")  /* 系统级 DMB */
#define dsb() __asm__ __volatile__("dsb sy" ::: "memory")  /* 系统级 DSB */
#define isb() __asm__ __volatile__("isb" ::: "memory")     /* 指令同步屏障 */

/* 特定类型的 DMB */
#define dmb_ish() __asm__ __volatile__("dmb ish" ::: "memory")  /* Inner Shareable */
#define dmb_ishst() __asm__ __volatile__("dmb ishst" ::: "memory")  /* Store only */
#define dmb_ishld() __asm__ __volatile__("dmb ishld" ::: "memory")  /* Load only */

int main(void)
{
	/* ===== 编译器屏障演示 ===== */
	int x = 0;
	int y = 0;

	x = 1;
	y = 2;

	compiler_barrier();  /* 阻止重排序 */

	assert(x == 1 && y == 2);

	printf("Compiler barrier test passed\n");

	/* ===== Memory Clobber 阻止优化 ===== */
	int value = 0;

	value = 42;
	__asm__ __volatile__("" ::: "memory");

	assert(value == 42);
	printf("Memory clobber test passed\n");

	/* ===== 实际场景: 确保写入顺序 ===== */
	struct {
		int value;
		int valid;
	} shared = {0, 0};

	/* 生产者线程的逻辑 */
	shared.value = 100;      /* 步骤 1: 写入数据 */
	dmb_ishst();             /* 步骤 2: Store 内存屏障 */
	shared.valid = 1;        /* 步骤 3: 设置标志 */

	assert(shared.value == 100 && shared.valid == 1);
	printf("Ordering test passed\n");

	/* ===== ARM64 内存屏障类型测试 ===== */
	int arr[4] = {0};

	arr[0] = 1;
	arr[1] = 2;
	dmb_ish();  /* 全内存屏障 */
	arr[2] = 3;
	arr[3] = 4;

	printf("Array: [%d, %d, %d, %d]\n", arr[0], arr[1], arr[2], arr[3]);

	/* ===== 原子性与内存屏障 ===== */
	volatile int counter = 0;

	int temp = counter;
	compiler_barrier();
	temp++;
	compiler_barrier();
	counter = temp;

	printf("Counter: %d (expected: 1)\n", counter);
	assert(counter == 1);

	/* ===== 指令同步屏障 ===== */
	printf("Before ISB\n");
	isb();  /* 确保所有前面的指令完成 */
	printf("After ISB\n");

	/* ===== 验证内存 clobber 的效果 ===== */
	int test_var = 0;

	__asm__ __volatile__(
		"add %w0, %w0, #1"
		: "+r" (test_var)
		);

	__asm__ __volatile__("" ::: "memory");

	printf("test_var after possible modification: %d\n", test_var);
	assert(test_var == 1);

	printf("\n=== All memory barrier tests passed! ===\n");
	return 0;
}
