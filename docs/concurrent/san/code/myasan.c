/*
 * myasan.c —— 极简 "KASAN 风格" 的 AddressSanitizer runtime
 *
 * 这个文件扮演内核里 mm/kasan/ 的角色:
 *   - 编译器 (clang -fsanitize=kernel-address) 只负责插桩,
 *     把每次内存访问变成对 __asan_load/storeN_noabort() 的调用;
 *   - shadow memory 的布局、poison 编码、越界判定和报告,
 *     全部在这里自己实现, 和 KASAN generic 的结构一一对应.
 *
 * 对应关系:
 *   myasan_poison()                 -> kasan_poison()
 *   myasan_region_is_poisoned()     -> memory_is_poisoned()
 *   __asan_load/storeN_noabort()    -> mm/kasan/generic.c 的同名函数
 *   __asan_register_globals()       -> mm/kasan/common.c 的同名函数
 *   myasan_malloc()/myasan_free()   -> kmalloc/kfree 里调 kasan_poison 的位置
 *
 * 注意: 本文件必须不带 -fsanitize 编译, 否则自己检查自己, 无限递归.
 */
#include <execinfo.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/* ---- shadow memory: 和 KASAN 一样, 应用内存每 8 字节对应 1 字节 shadow ---- */
#define SHADOW_SHIFT 3
#define SHADOW_OFFSET 0x7fff8000ULL /* 用户态 ASan 的默认 offset, 借用一下 */
#define SHADOW_ADDR(addr) (((addr) >> SHADOW_SHIFT) + SHADOW_OFFSET)

/*
 * 16TB 虚拟空间, 覆盖 [0, 2^47) 全部应用地址的 shadow.
 * MAP_NORESERVE + 匿名映射, 不触 page 就不占物理内存.
 */
#define SHADOW_MMAP_SIZE 0x100000000000ULL

/* shadow 字节的编码, 和 KASAN 一致 */
#define MYASAN_GLOBAL_REDZONE 0xfa /* KASAN_GLOBAL_REDZONE */
#define MYASAN_HEAP_FREED 0xfd     /* KASAN_KMALLOC_FREED    */
#define MYASAN_HEAP_REDZONE 0xfe   /* KASAN_KMALLOC_REDZONE  */
/* 0   : 整个 8 字节 granule 都可访问
 * 1~7 : 该 granule 只有前 N 个字节可访问 (对象尾部不满 8 字节时)
 */

static void myasan_init_shadow(void)
{
	static int done;

	if (done)
		return;
	void *p = mmap((void *)SHADOW_OFFSET, SHADOW_MMAP_SIZE,
		       PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE |
			       MAP_NORESERVE,
		       -1, 0);
	if (p != (void *)SHADOW_OFFSET) {
		fprintf(stderr, "myasan: mmap shadow failed\n");
		_exit(1);
	}
	done = 1;
}

/*
 * 把 [ptr, ptr+size) 的 shadow 设为 value; ptr 必须 8 字节对齐.
 * size 不是 8 的倍数时, 最后一个 granule 记为 "前 size%8 字节可访问".
 * value 传 0 就是 unpoison.
 */
static void myasan_poison(const void *ptr, size_t size, uint8_t value)
{
	uint8_t *shadow = (uint8_t *)SHADOW_ADDR((uintptr_t)ptr);

	myasan_init_shadow();
	memset(shadow, value, size / 8);
	if (size % 8)
		shadow[size / 8] = size % 8;
}

/* 对应 KASAN 的 memory_is_poisoned(): [addr, addr+size) 里有毒就返回 true */
static bool myasan_region_is_poisoned(uintptr_t addr, size_t size)
{
	uintptr_t end = addr + size;

	for (uintptr_t cur = addr & ~7UL; cur < end; cur += 8) {
		uint8_t s = *(uint8_t *)SHADOW_ADDR(cur);

		if (s == 0)
			continue;
		if (s <= 7) {
			/* 该 granule 从 cur+s 开始是毒, 看访问是否够到 */
			if (cur + s < end)
				return true;
		} else {
			return true;
		}
	}
	return false;
}

static const char *myasan_describe(uint8_t s)
{
	switch (s) {
	case MYASAN_GLOBAL_REDZONE:
		return "global redzone";
	case MYASAN_HEAP_FREED:
		return "freed heap (use-after-free)";
	case MYASAN_HEAP_REDZONE:
		return "heap redzone (heap-buffer-overflow)";
	case 0:
		return "addressable";
	default:
		if (s <= 7)
			return "partially addressable (尾部队不齐)";
		return "unknown poison";
	}
}

static void myasan_report(uintptr_t addr, size_t size, bool is_write,
			  void *caller)
{
	uint8_t s = *(uint8_t *)SHADOW_ADDR(addr);
	void *bt[16];
	int n;

	fprintf(stderr,
		"\n==myasan== ERROR: %s of size %zu at %p (pc %p)\n",
		is_write ? "WRITE" : "READ", size, (void *)addr, caller);
	fprintf(stderr, "==myasan== shadow byte: 0x%02x (%s)\n", s,
		myasan_describe(s));
	n = backtrace(bt, 16);
	/* bt[0] 是 callback 自身 (report 可能被内联进去), 从 bt[1] 开始打印 */
	backtrace_symbols_fd(bt + 1, n - 1, 2);
	/* 和 KASAN 的 *_noabort 语义一样: 报告完继续跑 */
}

/* ---- 编译器插桩要求的 callback, 用宏把 1/2/4/8/16 全生成出来 ---- */
#define DEFINE_ASAN_LOAD_STORE(size)                                       \
	void __asan_load##size##_noabort(uintptr_t addr)                   \
	{                                                                  \
		if (myasan_region_is_poisoned(addr, size))                 \
			myasan_report(addr, size, false,                   \
				      __builtin_return_address(0));        \
	}                                                                  \
	void __asan_store##size##_noabort(uintptr_t addr)                  \
	{                                                                  \
		if (myasan_region_is_poisoned(addr, size))                 \
			myasan_report(addr, size, true,                    \
				      __builtin_return_address(0));        \
	}

DEFINE_ASAN_LOAD_STORE(1)
DEFINE_ASAN_LOAD_STORE(2)
DEFINE_ASAN_LOAD_STORE(4)
DEFINE_ASAN_LOAD_STORE(8)
DEFINE_ASAN_LOAD_STORE(16)

/* ---- 全局变量 redzone: 结构体布局见 clang SanitizerDefs.h ---- */
struct __asan_global {
	void *beg;
	size_t size;
	size_t size_with_redzone;
	const char *name;
	const char *module_name;
	unsigned long has_dynamic_init;
	void *location;
	unsigned long odr_indicator;
};

/* .init_array 里的 asan.module_ctor 会调进来, 给每个全局变量围上 redzone */
void __asan_register_globals(struct __asan_global *globals, size_t size)
{
	myasan_init_shadow();
	for (size_t i = 0; i < size; i++) {
		struct __asan_global *g = &globals[i];

		/* 先 poison redzone, 再 unpoison 本体:
		 * size 不满 8 字节时末尾 granule 的 "部分可访问" 标记由
		 * unpoison 最后写入, 不会被 redzone 覆盖 */
		myasan_poison(g->beg, g->size_with_redzone,
			      MYASAN_GLOBAL_REDZONE);
		myasan_poison(g->beg, g->size, 0);
	}
}

void __asan_unregister_globals(struct __asan_global *globals, size_t size)
{
	for (size_t i = 0; i < size; i++)
		myasan_poison(globals[i].beg, globals[i].size_with_redzone, 0);
}

/*
 * ---- demo 用的分配器, 扮演 "调用 kasan_poison 的 kmalloc/kfree" ----
 * 布局: [32B 前 redzone][payload 对齐到 8][32B 后 redzone]
 * 前 redzone 里藏 {raw, size}, free 时不真释放 (相当于 KASAN 的 quarantine),
 * 这样 use-after-free 的写不会踩坏 glibc malloc 的 freelist.
 */
#define MYASAN_REDZONE 32

void *myasan_malloc(size_t size)
{
	size_t aligned = (size + 7) & ~7UL;
	char *raw = malloc(MYASAN_REDZONE + aligned + MYASAN_REDZONE);
	char *p;

	if (!raw)
		return NULL;
	p = raw + MYASAN_REDZONE;
	((void **)p)[-1] = raw;
	((size_t *)p)[-2] = size;
	/* 顺序不能反: unpoison payload 最后写, 保住末尾的 partial granule 标记 */
	myasan_poison(raw, MYASAN_REDZONE, MYASAN_HEAP_REDZONE);
	myasan_poison(p + aligned, MYASAN_REDZONE, MYASAN_HEAP_REDZONE);
	myasan_poison(p, size, 0);
	return p;
}

void myasan_free(void *ptr)
{
	size_t size;
	char *p = ptr;

	if (!p)
		return;
	size = ((size_t *)p)[-2];
	myasan_poison(p, (size + 7) & ~7UL, MYASAN_HEAP_FREED);
	/* 故意不 free(): quarantine */
}
