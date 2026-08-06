#include "internal.h"
#include <linux/spinlock.h>

// TODO spinlock 的实现似乎有很多变种
// 1. raw_spin_lock_init
// 2. 测试下 pv spinlock 吧

static DEFINE_SPINLOCK(test_lock);
static long protected_counter;
static long naked_counter;

#define SPINLOCK_LOOP_NUM 1000000

/*
 * 4 个 worker 同时对共享计数器做加法:
 * id < 2  : 使用 spinlock 保护 protected_counter ，结果必须精确等于 2 * SPINLOCK_LOOP_NUM
 * id >= 2 : 不加锁裸写 naked_counter ，由于并发丢失更新，结果几乎必然小于 2 * SPINLOCK_LOOP_NUM
 */
static void spinlock_worker(struct work_struct *work)
{
	struct work *test = (struct work *)work;
	ktime_t start = ktime_get();

	if (test->id < 2) {
		for (long i = 0; i < SPINLOCK_LOOP_NUM; i++) {
			spin_lock(&test_lock);
			protected_counter++;
			spin_unlock(&test_lock);
		}
	} else {
		/*
		 * 必须用 WRITE_ONCE ，不然编译器会把整个循环优化成
		 * 一条 naked_counter += SPINLOCK_LOOP_NUM ，竞争窗口就消失了
		 */
		for (long i = 0; i < SPINLOCK_LOOP_NUM; i++)
			WRITE_ONCE(naked_counter, READ_ONCE(naked_counter) + 1);
	}
	pr_info("worker %d cost %lld ns\n", test->id, ktime_get() - start);
}

/*
 * 基础 API:
 * 1. spin_lock / spin_unlock
 * 2. spin_trylock : 已经持锁的时候返回 0
 * 3. spin_lock_irqsave : 关本地中断并保存 flags
 */
static void basic_test(void)
{
	unsigned long flags;
	bool locked;

	spin_lock(&test_lock);
	locked = spin_trylock(&test_lock);
	pr_info("trylock while holding : %d (expect 0)\n", locked);
	spin_unlock(&test_lock);

	locked = spin_trylock(&test_lock);
	pr_info("trylock while free    : %d (expect 1)\n", locked);
	if (locked)
		spin_unlock(&test_lock);

	spin_lock_irqsave(&test_lock, flags);
	pr_info("irqsave : irq disabled=%d\n", irqs_disabled());
	spin_unlock_irqrestore(&test_lock, flags);
	pr_info("irqrestore : irq disabled=%d\n", irqs_disabled());
}

int test_spinlock(long action)
{
	switch (action) {
	case 0:
		basic_test();
		break;
	case 1:
		// 4 个 worker，前两个持锁后两个裸写，对比耗时和结果
		protected_counter = 0;
		naked_counter = 0;
		batch_queue_works(spinlock_worker, 4, sizeof(struct work));
		pr_info("protected_counter=%ld (expect %d)\n", protected_counter,
			2 * SPINLOCK_LOOP_NUM);
		pr_info("naked_counter=%ld (expect < %d)\n", naked_counter,
			2 * SPINLOCK_LOOP_NUM);
		break;
	}
	return 0;
}
