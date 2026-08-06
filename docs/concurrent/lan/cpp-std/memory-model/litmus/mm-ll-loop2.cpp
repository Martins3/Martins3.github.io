#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <semaphore>
#include <cassert>

int xy;
int yx;
std::atomic<bool> x{ false }, y{ false };

std::atomic<bool> go3{ true };
std::atomic<bool> go4{ true };

std::atomic<int> done{ 0 };
std::atomic<int> target{ 2 };

int m = 0;

// Change these to memory_order_seq_cst to fix the algorithm
/* static const auto store_ordering = std::memory_order_release; */
/* static const auto load_ordering = std::memory_order_acquire; */

/* static const auto store_ordering = std::memory_order_seq_cst; */
/* static const auto load_ordering = std::memory_order_seq_cst; */

// TODO 他喵的，为什么所有的都是 xy yx 都不为 0
static const auto store_ordering = std::memory_order_relaxed;
static const auto load_ordering = std::memory_order_relaxed;

void work_done()
{
	while (true) {
		// 开启新的 epoch
		// TODO 这个先搞一下
		/* std::atomic_compare_exchange_strong(&done, &target, 0); */

		y.store(false, std::memory_order_seq_cst);
		x.store(false, std::memory_order_seq_cst);

		go4.store(true, std::memory_order_seq_cst);
		go3.store(true, std::memory_order_seq_cst);

		/* using namespace std::chrono_literals; */
		/* std::this_thread::sleep_for(10ns); */
		x.store(true, store_ordering);
		y.store(true, store_ordering);
		m++;
	}
}

void read_x_then_y()
{
	while (true) {
		if (go3) {
			if (x.load(load_ordering) && !y.load(load_ordering))
				xy++;
			go3.store(false);
			done++;
		}
	}
}

void read_y_then_x()
{
	while (true) {
		if (go4) {
			if (!x.load(load_ordering) && y.load(load_ordering))
				yx++;
			go4.store(false);
			done++;
		}
	}
}

void show()
{
	while (true) {
		std::cout << "m      : " << m << std::endl;
		std::cout << "xy     : " << xy << std::endl;
		std::cout << "yx     : " << yx << std::endl;
		std::cout << "2m - xy - yx : " << 2 * m - xy - yx << std::endl;
		std::cout << std::endl;
		sleep(1);
	}
}

int main()
{
	std::thread c(read_x_then_y), d(read_y_then_x);
	std::thread e(work_done);
	std::thread f(show);

	c.join(), d.join(), e.join();
}
