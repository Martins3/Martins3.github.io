// https://en.cppreference.com/w/cpp/atomic/atomic_exchange
// 果然和 std::atomic_flag_test_and_set 非常类似

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

std::atomic<bool> lock(false); // holds true when locked
	// holds false when unlocked

int new_line = 1; // the access is synchronized via atomic lock variable

void f(int n)
{
	for (int cnt = 0; cnt < 100; ++cnt) {
		// 仔细想想，atomic_exchange_explicit 其实是两个动作
		// m = lock
		// lock = true
		//
		// 如果有多个 thread 同时 set 一个 flag 为 true ，所有的 thread 尝试 set
		// 最后只有一个 thread 可以设置上
		while (std::atomic_exchange_explicit(&lock, true,
						     std::memory_order_acquire))
			; // spin until acquired
		std::cout << n << (new_line++ % 80 ? "" : "\n");
		std::atomic_store_explicit(&lock, false,
					   std::memory_order_release);
	}
}

int main()
{
	std::vector<std::thread> v;
	for (int n = 0; n < 8; ++n)
		v.emplace_back(f, n);
	for (auto &t : v)
		t.join();
}
