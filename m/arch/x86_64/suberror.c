#include "internal.h"
#include <asm/io.h>

#define FLDS_MEM_EAX ".byte 0xd9, 0x00"

static inline void flds(uint64_t address)
{
	__asm__ __volatile__(FLDS_MEM_EAX ::"a"(address));
}

// 参考 tools/testing/selftests/kvm/x86_64/flds_emulation.h
// 2025-06-13 测试这个东西，并没有效果
// 但是在 4.19 内核中测试还是有效果的
static void suberror1(void)
{
	// 顺便说明一下 ioremap 和 phys_to_virt 的区别是什么?
	// ioremap 是给一个物理地址，然后给分配一个新的虚拟地址的
	// 所以可以看到 m2 和 m3 结果不同。
	//
	// 而 phys_to_virt 就是单纯的做一个减法。
	// 由于这里的  0x380000000000l 是随便找的，
	// 所以落到了一个 page table 没有建立的地址上
	long *m = phys_to_virt(0x380000000000l);
	long *m2 = ioremap(0x380000000000l, 0x10);
	long *m3 = ioremap(0x380000000000l, 0x10);
	pr_info("%lx %lx %lx\n", (long)m, (long)m2, (long)m3);
	flds((uint64_t)m2);
}

// TODO 这个可以帮忙解答一下 https://stackoverflow.com/questions/76691901/kvm-internal-error-suberror-1-when-filling-the-cr3-register
int test_suberror(long action)
{
	switch (action) {
	case 0:
		suberror1();
		break;
	case 1:
		// 参考这个，只是 guest os crash ，没有我们想要的效果
		// tools/testing/selftests/kvm/x86_64/vmx_exception_with_invalid_guest_state.c
		//
		// XXX 用的时候在取消掉，不然编译的时候有
		// martins3.o: warning: objtool: test_suberror+0x9c: unreachable instruction
		asm volatile("ud2");
		break;
	case 2:
		// 按道理，我们可以制作出来一个汇编，让 kvm 无法解析，
		// 然后就有 emulation failed 的操作
		break;
	}
	return 0;
}
