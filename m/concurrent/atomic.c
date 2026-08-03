#include "internal.h"
#include <linux/slab.h>

/*
 * 测试学习: https://docs.kernel.org/core-api/wrappers/atomic_t.html
 *
 * 务必仔细理解 include/linux/atomic/ 下的三个文件，每一个文件都长的不得了
 */

// TODO 补充一下这几个测试的反汇编结果
static atomic_t a;
static int test_atomic_read(void)
{
	return atomic_read(&a);
}

static void test_atomic_write(long action)
{
	atomic_set(&a, action);
}

static void test_atomic_inc(void)
{
	atomic_inc(&a);
}

/*
 * io_req_local_work_add 是一个有趣的使用案例，如果使用 try_cmpxchg 来
 * 来实现一个无锁操作，如何增加了元素到链表中，并且访问一些信息。
 * */
static atomic_t v;
static int func(int old)
{
	/* TODO 参考 io_req_local_work_add 补充成为真正的例子吧
	 * */
	return old;
}
static void test_try_cmpxchg(void)
{
	int old = atomic_read(&v);
	int new;

	/*
	 * try_cmpxchg 和 atomic_try_cmpxchg 实际上没区别，只是参数类型不一样而已
	 */
	do {
		new = func(old);
	} while (!atomic_try_cmpxchg(&v, &old, new));
}

static void test_xchg(void)
{
	u8 m = 1;
	u16 n = 2;
	/*
	 * mov    $0xc,%al
	 * xchg   %al,0x7(%rsp)
         * movzbl %al,%esi
         * mov    $0x0,%rdi
	 * call   0x12a <test_atomic+282> # call printf 
	 *
         * mov    $0xc,%ax
         * xchg   %ax,0x4(%rsp)
         * movzwl %ax,%esi
         * mov    $0x0,%rdi
	 * call   0x12a <test_atomic+282> # 构建为 call printf 
	 *
	 */

	/*
	 * 位宽是 xchg 的寄存器决定的 al 或者 ax ，而结果也是放到 al / ax 上的。
	 *
	 * 显然，没有处理 xchg 遇到 page fault 的可能
	 */
	pr_info("%d", xchg(&m, 12));
	pr_info("%d", xchg(&n, 12));
}

/*
 * 一个有趣的问题，如果 atomic 指令操作的 page 实际上 page fault 了。
 * 1. CPU 的实现中是基于 cache coherence 的
 * 2. 操作系统必须要保证在 page fault 的互斥，也就是只有一个 thread 会 page fault ，
 */

struct kvm_steal_time {
	u8 preempted;
};

static unsigned long lock[2];
static inline int bitmap_lock(long bit, unsigned long *lock)
{
	return test_and_set_bit_lock(bit, lock);
}

static inline void bitmap_unlock(long bit, unsigned long *lock)
{
	clear_bit_unlock(bit, lock);
}

struct test {
	bool use_lock;
	bool use_atomic;

	int which_bit;
	long *counter;
	int id;
	struct work_struct work;
};

static void do_inc(struct work_struct *work)
{
	// 故意使用第二个 long 来测试
	struct test *test = container_of(work, struct test, work);

	ktime_t start = ktime_get();
	size_t limit = 0x100000;
	if (test->use_lock) {
		for (size_t i = 0; i < limit; i++) {
			// 4 个 worker:
			//
			// 3 cost 1234198975 ns
			// 0 cost 1367247834 ns
			// 2 cost 1395892314 ns
			// 1 cost 1396143246 ns
			//
			// 2 个:
			// 1 cost 269342736 ns
			// 0 cost 278901740 ns
			//
			// 一个:
			// 0 cost 23194467 ns
			while (bitmap_lock(sizeof(long) * 8 + test->which_bit,
					   lock))
				;
			(*(test->counter))++;
			bitmap_unlock(sizeof(long) * 8 + test->which_bit, lock);
		}
	}

	else {
		for (size_t i = 0; i < limit; i++)
			(*(test->counter))++;
	}
	pr_info("%d cost %lld ns\n", test->id, ktime_get() - start);
}

// system_unbound_wq 才是我们需要的，参考 workqueue_init_early 可以 2048 并发
static int test_atomic_bitmap_hack(bool use_lock)
{
	const int works = 1;
	struct test *bit1_tests;
	bit1_tests = kvmalloc_array(works, sizeof(struct test), GFP_KERNEL);
	if (!bit1_tests)
		return -ENOMEM;

	long counter1 = 0;
	long counter2 = 0;
	for (size_t i = 0; i < works; i++) {
		INIT_WORK(&bit1_tests[i].work, do_inc);
		bit1_tests[i].use_lock = use_lock;
		bit1_tests[i].which_bit = i % 2;
		bit1_tests[i].counter = i % 2 == 0 ? &counter1 : &counter2;
		bit1_tests[i].id = i;
	}

	for (size_t i = 0; i < works; i++)
		if (!queue_work(system_unbound_wq, &bit1_tests[i].work))
			return -ENAVAIL;

	for (size_t i = 0; i < works; i++)
		flush_work(&bit1_tests[i].work);

	pr_info("result\n");
	pr_info("counter 1 : %lx\n", counter1);
	pr_info("counter 2 : %lx\n", counter2);

	return 0;
}

static void test_atomic_bitmap_basic(void)
{
	unsigned long m = 0;
	pr_info("0: %d", test_and_set_bit(0, &m));
	pr_info("1: %d", test_and_set_bit(1, &m));
	pr_info("2: %d", test_and_set_bit(2, &m));
	pr_info("2: %d", test_and_set_bit(2, &m));
	pr_info("clear result : %lx\n", m);
	clear_bit(2, &m);
	pr_info("clear result : %lx\n", m);
}

/**
 *
 * 检查 flags 的时候我发现
 * static __always_inline void folio_clear_swapbacked(struct folio *folio) {
 *   clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
 * }
 * static __always_inline void __folio_clear_swapbacked(struct folio *folio) {
 *   __clear_bit(PG_swapbacked, folio_flags(folio, FOLIO_PF_NO_TAIL));
 * }
 */
static unsigned long bench_flags;
static void bench_atomic_clear_bit(void)
{
	ktime_t begin = ktime_get();
	for (size_t i = 0; i < 1000 * 1000 * 1000; i++) {
		__clear_bit(i % sizeof(unsigned long), &bench_flags);
	}
	pr_info("__clear_bit %lld\n", ktime_get() - begin);

	begin = ktime_get();
	for (size_t i = 0; i < 1000 * 1000 * 1000; i++) {
		clear_bit(i % sizeof(unsigned long), &bench_flags);
	}
	pr_info("clear_bit %lld\n", ktime_get() - begin);
	/*
	 * [  206.301778] __clear_bit 1364171651
	 * [  209.513112] clear_bit   3211082257
	 *
	 * 差不多三倍的差别了，一个是 1.3 ns ，一个是 3.2 ns 的
	 */
}

int test_atomic(long action)
{
	switch (action) {
	case 0:
		pr_info("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__,
			atomic_read(&a));
		/* atomic_set(&a, 1); */
		atomic_add_unless(&a, 100, 0);
		// 只有不等于 0 的时候才会增加 100
		pr_info("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__,
			atomic_read(&a));

		break;
	case 1:
		return test_atomic_read();
	case 2:
		test_atomic_write(action);
		break;
	case 3:
		test_atomic_inc();
		break;
	case 4:
		test_try_cmpxchg();
		break;
	case 5:
		test_xchg();
		break;
	case 6:
		// 基础 api 测试，就是我们想要的效果
		test_atomic_bitmap_basic();
		break;
	case 7:
		test_atomic_bitmap_hack(false);
		break;
	case 8:
		test_atomic_bitmap_hack(true);
		break;
	case 9:
		bench_atomic_clear_bit();
		break;
	}
	return 0;
}
