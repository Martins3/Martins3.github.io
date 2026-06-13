#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <semaphore>
#include <cassert>

int xy;
int yx;
std::atomic<bool> x{ false }, y{ false };

std::atomic<bool> go1{ true };
std::atomic<bool> go2{ true };
std::atomic<bool> go3{ true };
std::atomic<bool> go4{ true };

std::atomic<int> done{ 0 };

int m = 0;

// Change these to memory_order_seq_cst to fix the algorithm
static const auto store_ordering = std::memory_order_release;
static const auto load_ordering = std::memory_order_acquire;

/* static const auto store_ordering = std::memory_order_seq_cst; */
/* static const auto load_ordering = std::memory_order_seq_cst; */

/* static const auto store_ordering = std::memory_order_relaxed; */
/* static const auto load_ordering = std::memory_order_relaxed; */

void work_done()
{
	while (true) {
		if (done != 2)
			continue;
		done = 0;
		assert(!go1);
		assert(!go2);
		assert(!go3);
		assert(!go4);
		// 开启新的 epoch

		y.store(false, std::memory_order_relaxed);
		x.store(false, std::memory_order_relaxed);

		go4.store(true, std::memory_order_relaxed);
		go3.store(true, std::memory_order_relaxed);
		go1.store(true, std::memory_order_relaxed);
		go2.store(true, std::memory_order_relaxed);
		m++;
	}
}

void thread1()
{
	while (true) {
		if (go1) {
			go1.store(false);
			x.store(true, store_ordering);
		}
	}
}

void thread2()
{
	while (true) {
		if (go2) {
			go2.store(false);
			y.store(true, store_ordering);
		}
	}
}

// 如果 read x 是 ok 的，但是 y 不 ok 的时候
void read_x_then_y()
{
	while (true) {
		if (go3) {
			while (!x.load(load_ordering))
				;
			if (!y.load(load_ordering)) {
				++xy;
			}
			go3.store(false);
			done++;
		}
	}
}

void read_y_then_x()
{
	while (true) {
		if (go4) {
			while (!y.load(load_ordering))
				;
			if (!x.load(load_ordering)) {
				++yx;
			}
			go4.store(false);
			done++;
		}
	}
}

// 总体来说，这个是符合预期的
void show()
{
	while (true) {
		std::cout << "m(循环的次数)      : " << m << std::endl;
		std::cout << "xy(读到 x 后但是没有 y)     : " << xy << std::endl;
		std::cout << "yx     : " << yx << std::endl;
		std::cout << "2m - xy - yx (有多少次，已经搞了) : " << 2 * m - xy - yx << std::endl;
		std::cout << std::endl;
		sleep(1);
	}
}

int main()
{
	std::thread c(read_x_then_y), d(read_y_then_x);
	std::thread a(thread1), b(thread2);
	std::thread e(work_done);
	std::thread f(show);

	a.join(), b.join(), c.join(), d.join(), e.join();
}
