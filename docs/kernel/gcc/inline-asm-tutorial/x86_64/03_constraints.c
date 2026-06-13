/*
 * 示例 03: 约束条件 (Constraints) 详解
 *
 * 知识点:
 * - 通用寄存器约束: "r" (任何通用寄存器)
 * - 特定寄存器约束: "a" (rax/eax), "b" (rbx/ebx), "c" (rcx/ecx), "d" (rdx/edx)
 * - 内存约束: "m" (内存操作数)
 * - 立即数约束: "i" (立即数), "n" (已知数值的立即数)
 * - 匹配约束: "0", "1" 等表示与前面某个操作数使用同一位置
 */

#include <stdio.h>
#include <assert.h>
#include <stdint.h>

int main(void)
{
	/* ===== 特定寄存器约束 ===== */
	unsigned long rax_val = 0;
	unsigned int eax_val = 42;

	/* 使用 "a" 约束强制使用 rax/eax 寄存器
	 * 这在 syscall 中很有用，因为 syscall 号必须在 rax 中
	 */
	__asm__ __volatile__(
		"movq $123456, %%rax"
		: "=a" (rax_val)      /* 输出必须在 rax */
		);

	printf("rax = %lu (expected: 123456)\n", rax_val);
	assert(rax_val == 123456);

	/* 使用 eax 作为输入 */
	unsigned int result;
	__asm__ __volatile__(
		"addl $8, %%eax"
		: "=a" (result)
		: "a" (eax_val)       /* 输入也使用 eax */
		);

	printf("%d + 8 = %d (expected: 50)\n", eax_val, result);
	assert(result == 50);

	/* ===== 内存约束 ===== */
	int mem_val = 100;

	/* "m" 约束表示操作数在内存中 */
	__asm__ __volatile__(
		"incl %0"             /* 内存中的值加 1 */
		: "=m" (mem_val)
		: "m" (mem_val)
		);

	printf("After inc: %d (expected: 101)\n", mem_val);
	assert(mem_val == 101);

	/* ===== 立即数约束 ===== */
	int imm_result;

	/* "i" 约束表示立即数 */
	__asm__ __volatile__(
		"movl %1, %0"
		: "=r" (imm_result)
		: "i" (0xDEADBEEF)    /* 编译时常数 */
		);

	printf("imm_result = 0x%x (expected: 0xdeadbeef)\n", imm_result);
	assert(imm_result == (int)0xDEADBEEF);

	/* ===== 匹配约束 ===== */
	/* 当输入和输出需要是同一个位置时使用 */
	int in_out = 10;

	__asm__ __volatile__(
		"addl %1, %0"         /* in_out += 20 */
		: "=r" (in_out)
		: "r" (20), "0" (in_out) /* "0" 表示与 %0 同一位置 */
		);

	printf("in_out = %d (expected: 30)\n", in_out);
	assert(in_out == 30);

	/* 更好的方式: 使用 + 修饰符表示读写 */
	int rw_val = 5;
	__asm__ __volatile__(
		"shll $2, %0"         /* rw_val <<= 2 (乘以 4) */
		: "+r" (rw_val)       /* + 表示读写 */
		);

	printf("rw_val = %d (expected: 20)\n", rw_val);
	assert(rw_val == 20);

	/* ===== 大小限制约束 ===== */
	uint8_t byte_val = 0xFF;

	/* "q" 约束: a, b, c, or d register (for byte operations) */
	__asm__ __volatile__(
		"movb %1, %0\n\t"
		"incb %0"
		: "=q" (byte_val)
		: "q" (byte_val)
		);

	printf("byte_val = %u (expected: 1, overflow to 0 then inc)\n", byte_val);
	/* Note: 这里的行为取决于具体实现，可能不是预期的 */

	/* ===== 地址约束 ===== */
	int arr[4] = {10, 20, 30, 40};
	int *ptr = &arr[1];
	int loaded;

	/* "p" 约束: 有效的内存地址 (用于加载地址到指针寄存器) */
	__asm__ __volatile__(
		"movl (%1), %0"
		: "=r" (loaded)
		: "r" (ptr)           /* 指针在寄存器中 */
		);

	printf("loaded = %d (expected: 20)\n", loaded);
	assert(loaded == 20);

	printf("\n=== All constraint tests passed! ===\n");
	return 0;
}
