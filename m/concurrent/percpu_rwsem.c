#include "internal.h"
#include <linux/rwsem.h>
#include <linux/delay.h>
#include <linux/percpu-rwsem.h>

/*
 * percpu rwsem 的核心卖点: reader 路径只需要操作 percpu 计数器，
 * 不需要任何 atomic 操作和内存屏障，所以大量 reader 并发的时候
 * 不会存在 cache line 竞争。代价是 writer 路径非常的慢。
 *
 * 参考: struct super_block::s_writers
 */
DEFINE_STATIC_PERCPU_RWSEM(percpu_rwsem_demo);
static DECLARE_RWSEM(normal_rwsem_demo);

#define RWSEM_BENCH_LOOP 10000000

/*
 * 对比 percpu_rwsem 和 rwsem 的 read 路径开销
 */
static void bench_read_path(void)
{
	ktime_t start;
	long i;

	start = ktime_get();
	for (i = 0; i < RWSEM_BENCH_LOOP; i++) {
		percpu_down_read(&percpu_rwsem_demo);
		percpu_up_read(&percpu_rwsem_demo);
	}
	pr_info("percpu_rwsem read : %lld ns\n", ktime_get() - start);

	start = ktime_get();
	for (i = 0; i < RWSEM_BENCH_LOOP; i++) {
		down_read(&normal_rwsem_demo);
		up_read(&normal_rwsem_demo);
	}
	pr_info("rwsem        read : %lld ns\n", ktime_get() - start);
}

/*
 * 两个 reader 并发持锁: 两个 worker 同时进入读临界区，
 * 证明 reader 之间不会互相阻塞。
 *
 * 注意不要在同一个任务里嵌套两次 percpu_down_read:
 * 如果两次 down_read 之间来了 writer ，第二次 down_read 会排在
 * writer 后面，而 writer 又在等第一次读释放，形成自我死锁，
 * lockdep 会直接报 recursive locking DEADLOCK 。
 */
static void percpu_rwsem_reader(struct work_struct *work)
{
	struct work *test = (struct work *)work;
	static atomic_t readers_inside;

	percpu_down_read(&percpu_rwsem_demo);
	pr_info("reader %d entered, %d readers inside\n", test->id,
		atomic_inc_return(&readers_inside));
	msleep(1000);
	atomic_dec(&readers_inside);
	percpu_up_read(&percpu_rwsem_demo);
}

int test_percpu_rwsem(long action)
{
	switch (action) {
	case 0:
		batch_queue_works(percpu_rwsem_reader, 2, sizeof(struct work));
		break;
	case 1:
		// writer 会等待所有已经进来的 reader 离开
		percpu_down_write(&percpu_rwsem_demo);
		pr_info("writer entered\n");
		percpu_up_write(&percpu_rwsem_demo);
		break;
	case 2:
		bench_read_path();
		break;
	}
	return 0;
}
