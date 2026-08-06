#include <thread>
#include <atomic>
#include <assert.h>

// # seq_cst 和 acq_rel 的区别
// <!-- d4e5a2c2-0595-4ea5-8ff8-e47c8b58ef04 -->
//
// 这个 demo 试图只用两个 flag 实现一个“两线程互斥”算法：
// 1. 先把自己的 flag 设为 true，表示“我想进入临界区”
// 2. 再检查对方的 flag
// 3. 如果对方是 false，就认为自己拿到了“锁”
// 4. 如果对方也是 true，就把自己的 flag 清回 false，然后重试
//
// 注意， 每一个 thread 只是写自己的 flag ，然后观察到其他的 thread 的 flag 就可以了
// 这个方法不高效，尤其是 thread 数量多的时候， 它还容易导致活锁。
//
// 因为 release 语义控制 store 操作的顺序，而 acquired 语义控制 load 的顺序，而这个
// 顺序要求这两个操作 store 和 release 操作的顺序不可以翻过来，所以会有问题的。
std::atomic<bool> flag1{ false };
std::atomic<bool> flag2{ false };

std::atomic<int> counter{ 0 };

// Change these to memory_order_seq_cst to fix the algorithm
// static const auto store_ordering = std::memory_order_release;
// static const auto load_ordering = std::memory_order_acquire;

// static const auto store_ordering = std::memory_order_seq_cst;
// static const auto load_ordering = std::memory_order_seq_cst;

// 这个会被 g++ 警告，但是 clang++ 不会，原因也很简单，只有 rmw 的操作才需要对于指令前后都屏蔽
// https://stackoverflow.com/questions/77992063/why-does-stdmemory-order-acq-rel-always-trigger-warnings-in-c11
// static const auto store_ordering = std::memory_order_acq_rel;
// static const auto load_ordering = std::memory_order_acq_rel;

static const auto store_ordering = std::memory_order_relaxed;
static const auto load_ordering = std::memory_order_relaxed;

void busy(int n)
{
	auto &me = (n == 1) ? flag1 : flag2;
	auto &him = (n == 1) ? flag2 : flag1;

	for (;;) {
		for (;;) {
			// 先声明“我准备进入临界区”。
			me.store(true, store_ordering);
			// 再观察对方是否也在竞争。
			if (him.load(load_ordering) == false) {
				// got the 'lock'
				break;
			}

			// retention, no wait period -> busy loop
			me.store(false, store_ordering);
		}

		// 这里已经进入到了 critical region 了
		// counter 只是一个调试探针，用来检查是否真的实现了互斥。
		int tmp = counter.fetch_add(1, std::memory_order_relaxed);
		assert(tmp == 0);
		tmp = counter.fetch_sub(1, std::memory_order_relaxed);
		assert(tmp == 1);

		me.store(false, store_ordering);
	}
}

int main()
{
	std::thread t1{ busy, 1 };
	std::thread t2{ busy, 2 };

	t1.join();
	t2.join();
}
