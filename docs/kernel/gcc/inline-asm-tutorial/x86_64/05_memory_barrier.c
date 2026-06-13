/*
 * 示例 05: 内存屏障 (Memory Barriers)
 *
 * 知识点:
 * - 编译器屏障: 防止编译器重排序内存操作
 * - 内存 clobber ("memory") 的作用
 * - volatile 与 memory barrier 的关系
 * - 实际应用场景: 多线程同步、驱动程序
 */

#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>

/* 简单的编译器屏障 */
#define compiler_barrier() __asm__ __volatile__("" ::: "memory")

/* 内存屏障 (也阻止 CPU 重排序) */
#define mfence() __asm__ __volatile__("mfence" ::: "memory")

/* 写内存屏障 */
#define sfence() __asm__ __volatile__("sfence" ::: "memory")

/* 读内存屏障 */
#define lfence() __asm__ __volatile__("lfence" ::: "memory")

/* 共享数据 */
volatile int ready = 0;
volatile int data = 0;

int main(void)
{
	/* ===== 编译器屏障演示 ===== */
	int x = 0;
	int y = 0;

	/* 没有屏障时，编译器可能重排序这些操作 */
	x = 1;
	y = 2;

	compiler_barrier();  /* 阻止重排序 */

	/* 屏障后的操作不会被重排序到屏障前 */
	assert(x == 1 && y == 2);

	printf("Compiler barrier test passed\n");

	/* ===== Memory Clobber 阻止优化 ===== */
	int value = 0;

	/* 编译器可能认为 value 没有被使用而优化掉赋值 */
	value = 42;

	/* 但有了 memory clobber，编译器必须假设 value 被读取 */
	__asm__ __volatile__("" ::: "memory");

	/* 所以这里 value 一定是 42 */
	assert(value == 42);
	printf("Memory clobber test passed\n");

	/* ===== 实际场景: 确保写入顺序 ===== */
	struct {
		int value;
		int valid;
	} shared = {0, 0};

	/* 生产者线程的逻辑 */
	shared.value = 100;      /* 步骤 1: 写入数据 */
	compiler_barrier();      /* 步骤 2: 屏障，确保顺序 */
	shared.valid = 1;        /* 步骤 3: 设置标志 */

	assert(shared.value == 100 && shared.valid == 1);
	printf("Ordering test passed\n");

	/* ===== 内存屏障 vs 编译器屏障 ===== */
	int arr[4] = {0};

	/* 写入数组 */
	arr[0] = 1;
	arr[1] = 2;

	/* 仅编译器屏障: 确保前面的写入在代码顺序上先于后面的写入
	 * 但不保证 CPU 执行顺序
	 */
	compiler_barrier();

	arr[2] = 3;
	arr[3] = 4;

	mfence();  /* 内存屏障: 确保所有之前的写入全局可见 */

	printf("Array: [%d, %d, %d, %d]\n", arr[0], arr[1], arr[2], arr[3]);

	/* ===== 原子性与内存屏障 ===== */
	volatile int counter = 0;

	/* 模拟原子递增 (实际应用中应使用原子操作) */
	int temp = counter;
	compiler_barrier();
	temp++;
	compiler_barrier();
	counter = temp;

	printf("Counter: %d (expected: 1)\n", counter);
	assert(counter == 1);

	/* ===== 驱动程序风格示例 ===== */
	volatile unsigned int *device_reg = (void *)0x1000;  /* 假设的设备寄存器 */

	/* 模拟设备寄存器访问
	 * 注意: 这里不会真的访问 0x1000，只是演示代码结构
	 */
	(void)device_reg;  /* 抑制未使用警告 */

	/* 写入设备寄存器 */
	/* *device_reg = 0x1; */  /* 启动操作 */

	compiler_barrier();  /* 确保启动操作先完成 */

	/* 轮询等待 (实际代码) */
	/* do { reg_value = *device_reg; } while (!(reg_value & 0x80)); */

	printf("Device register pattern demo (not executed)\n");

	/* ===== 验证内存 clobber 的效果 ===== */
	int test_var = 0;

	/* 这个函数可能被内联，没有 memory clobber 时
	 * 编译器可能假设 test_var 没有被修改
	 */
	extern void might_modify_memory(void);

	__asm__ __volatile__(
		"incl %0"
		: "+m" (test_var)
		);

	__asm__ __volatile__("" ::: "memory");

	/* 由于 memory clobber，编译器必须重新加载 test_var */
	printf("test_var after possible modification: %d\n", test_var);
	assert(test_var == 1);

	printf("\n=== All memory barrier tests passed! ===\n");
	return 0;
}
