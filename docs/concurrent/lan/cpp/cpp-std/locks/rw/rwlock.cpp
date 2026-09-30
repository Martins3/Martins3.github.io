// # cpp rwlock
// <!-- 355ba41f-b490-454b-b884-cd243e21550e -->
// shared_mutex / shared_lock 的基本用法
//
// 这套机制用来实现典型的“读者-写者锁”模型：
// 1. 多个读线程可以同时持有共享锁并发读取
// 2. 写线程必须拿到独占锁，写入期间不能和其他读写并发
//
// 适合“读很多、写很少”的场景。
// 如果普通 mutex 也能满足需求，那么 mutex 通常更简单；
// 只有当读并发确实有价值时，shared_mutex 才更合适。
//
// 这里的角色是：
// 1. std::shared_mutex: 同时支持共享锁和独占锁的互斥量
// 2. std::shared_lock : 用来拿共享锁，适合只读路径
// 3. std::unique_lock : 用来拿独占锁，适合修改路径
//
// https://en.cppreference.com/w/cpp/thread/shared_mutex
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <syncstream>
#include <thread>

class ThreadSafeCounter {
    public:
	ThreadSafeCounter() = default;

	// Multiple threads/readers can read the counter's value at the same time.
	unsigned int get() const
	{
		// 只读路径拿共享锁。
		// 这样多个 reader 可以并发执行 get()，不会互相阻塞。
		std::shared_lock lock(mutex_);
		return value_;
	}

	// Only one thread/writer can increment/write the counter's value.
	void increment()
	{
		// 写路径拿独占锁。
		// 一旦有 writer 持锁，其他 reader / writer 都要等待。
		std::unique_lock lock(mutex_);
		++value_;
	}

	// Only one thread/writer can reset/write the counter's value.
	void reset()
	{
		// reset() 也是写操作，所以同样需要独占锁。
		std::unique_lock lock(mutex_);
		value_ = 0;
	}

    private:
	mutable std::shared_mutex mutex_;
	unsigned int value_{};
};

int main()
{
	ThreadSafeCounter counter;

	auto increment_and_print = [&counter]() {
		for (int i{}; i != 3; ++i) {
			counter.increment();
			// osyncstream 用来把一整段输出原子地刷新到 cout，
			// 避免多个线程同时打印时相互穿插。
			std::osyncstream(std::cout)
				<< std::this_thread::get_id() << ' '
				<< counter.get() << '\n';
		}
	};

	// 两个线程并发执行：
	// 写入通过 unique_lock 串行化，读取通过 shared_lock 受保护。
	std::thread thread1(increment_and_print);
	std::thread thread2(increment_and_print);

	thread1.join();
	thread2.join();
}
