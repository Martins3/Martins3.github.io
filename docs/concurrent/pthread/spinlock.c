// pthread_spin_lock 分析
//
// 运行: gcc spinlock.c -o spinlock.out -pthread && ./spinlock.out
//
// ===================== 1. glibc 的实现 =====================
//
// glibc (nptl/pthread_spin_lock.c) 的实现非常短，是纯用户态的:
//
//   int pthread_spin_lock(pthread_spinlock_t *lock)
//   {
//       // 第一次尝试直接用 atomic_exchange, 假设大概率能拿到锁
//       if (atomic_exchange_acquire(lock, 1) != 0) {
//           do {
//               atomic_spin_nop();  // x86 上是 pause 指令
//           } while (atomic_compare_and_exchange_val_acq(lock, 1, 0) != 0);
//       }
//       return 0;
//   }
//
// 对应的 pthread_spin_unlock 只是一条释放语义的写:
//
//   int pthread_spin_unlock(pthread_spinlock_t *lock)
//   {
//       atomic_store_release(lock, 0);
//       return 0;
//   }
//
// 关键点:
// - x86 上 atomic_exchange_acquire 编译为一条 `lock xchg` (xchg 带 lock 前缀
//   是原子的总线操作), CAS 则是 `lock cmpxchg`。
// - aarch64 上对应 LDAXR/STLXR (load-acquire/store-release 的独占访问指令)。
// - 全程没有任何系统调用: 拿不到锁就在用户态死循环, 不会陷入内核,
//   不会像 mutex 那样通过 futex 挂起等待者。
// - acquire/release 语义保证临界区内的读写不会被重排锁外。
//
// ===================== 2. 和 pthread_mutex 的本质区别 =====================
//
// pthread_mutex_lock 在抢不到锁时, 会调用 futex(FUTEX_WAIT) 陷入内核,
// 把线程挂到等待队列上, 让出 CPU (类型设为 PTHREAD_MUTEX_ADAPTIVE_NP
// 时会先在用户态自旋一小段时间再挂起)。持有者优先级低时, futex 的
// PI (priority inheritance) 变体还能做优先级继承。
//
// pthread_spin_lock 什么都没有:
// - 不挂起, 不让出 CPU, 优先级翻转时也没有优先级继承;
// - 锁里不记录 owner, 内核完全不知道谁在等这把锁。
//
// ===================== 3. 用户态自旋锁的经典坑 =====================
//
// (a) 持锁线程被换出 (holder preemption) -- 最致命的问题
//     线程 A 拿到锁, 刚进入临界区, 时间片用完被调度器换出;
//     其它核上的线程 B、C、D 都在 spin, 它们要白白烧掉一整个时间片
//     (毫秒级), 直到 A 被重新调度、执行完临界区、释放锁。
//     临界区本来可能只有几十纳秒, 却因为一次抢占放大成毫秒级浪费。
//     线程数超过 CPU 核数 (oversubscription) 时几乎必然触发。
//
// (b) 优先级翻转
//     低优先级线程持锁被换出, 高优先级线程在低优先级线程所在核之外
//     空转, 甚至会抢占持锁者的 CPU, 进一步推迟锁的释放。
//
// (c) 和内核 spinlock 的本质差异
//     内核态 spinlock 持锁时会关闭抢占 (preempt_disable), 从根本上
//     消除了 (a); 用户态做不到这一点, 这是用户态 spin lock 的先天缺陷。
//
// ===================== 4. 什么时候才值得用 =====================
//
// - 临界区极短 (几条指令, 绝不包含系统调用/内存分配/IO);
// - 线程数 <= CPU 核数, 最好还做了 CPU 亲和性绑定 (pthread_setaffinity_np);
// - 争用概率低。
// 否则老老实实用 pthread_mutex: 无竞争时同样是一条原子指令拿锁,
// 有竞争时通过 futex 挂起, 不会烧 CPU。想要 mutex 也先自旋一段再挂起,
// 可以把类型设为 PTHREAD_MUTEX_ADAPTIVE_NP。
//
// 缓解措施 (不根治): 自旋若干次后 sched_yield() 主动让出 CPU,
// 让持锁者有机会被调度回来。

#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 4
#define NUM_INCREMENTS 100000

long counter = 0; // 被自旋锁保护的共享变量
pthread_spinlock_t lock;

void *worker(void *arg)
{
	for (int i = 0; i < NUM_INCREMENTS; ++i) {
		// 1. pthread_spin_lock(): 加锁
		// 拿不到锁时在用户态自旋等待, 不会陷入内核
		pthread_spin_lock(&lock);

		// --- 临界区开始 ---
		// 注意: 这里故意保持极短。如果临界区里出现 sleep/系统调用,
		// 其它线程会在自旋中空转, 白白浪费 CPU
		counter++;
		// --- 临界区结束 ---

		// 2. pthread_spin_unlock(): 解锁
		pthread_spin_unlock(&lock);
	}
	return NULL;
}

int main()
{
	pthread_t threads[NUM_THREADS];

	// 3. pthread_spin_init(): 初始化自旋锁
	// 第二个参数 pshared:
	//   PTHREAD_PROCESS_PRIVATE - 只在当前进程的线程间共享
	//   PTHREAD_PROCESS_SHARED  - 可以放在共享内存中跨进程使用
	if (pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE) != 0) {
		printf("无法初始化自旋锁\n");
		return -1;
	}

	for (long i = 0; i < NUM_THREADS; i++) {
		if (pthread_create(&threads[i], NULL, worker, (void *)i)) {
			printf("无法创建线程\n");
			return -1;
		}
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("counter = %ld (期望 %d)\n", counter,
	       NUM_THREADS * NUM_INCREMENTS);

	// 4. pthread_spin_destroy(): 销毁自旋锁
	pthread_spin_destroy(&lock);
	return 0;
}
