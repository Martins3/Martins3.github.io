#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

std::atomic_flag lock = ATOMIC_FLAG_INIT;

void f(int n)
{
	for (int cnt = 0; cnt < 100; ++cnt) {
		while (std::atomic_flag_test_and_set_explicit(
			&lock, std::memory_order_acquire))
			; // spin until the lock is acquired
		std::cout << "Output from thread " << n << '\n';
		std::atomic_flag_clear_explicit(&lock,
						std::memory_order_release);
	}
}

int main()
{
	std::vector<std::thread> v;
	for (int n = 0; n < 10; ++n)
		v.emplace_back(f, n);
	for (auto &t : v)
		t.join();
}
