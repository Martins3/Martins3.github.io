// https://en.cppreference.com/w/cpp/thread/promise
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <thread>
#include <vector>

void accumulate(std::vector<int>::iterator first,
		std::vector<int>::iterator last,
		std::promise<int> accumulate_promise)
{
	int sum = std::accumulate(first, last, 0);
	accumulate_promise.set_value(sum); // Notify future
}

void do_work(std::promise<void> barrier)
{
	barrier.set_value();
	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	std::this_thread::sleep_for(std::chrono::seconds(3));
	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
}

int main()
{
	// Demonstrate using promise<int> to transmit a result between threads.
	std::vector<int> numbers = { 1, 2, 3, 4, 5, 6 };
	std::promise<int> accumulate_promise;
	std::future<int> accumulate_future = accumulate_promise.get_future();
	std::thread work_thread(accumulate, numbers.begin(), numbers.end(),
				std::move(accumulate_promise));

	// future::get() will wait until the future has a valid result and retrieves it.
	// Calling wait() before get() is not needed
	// accumulate_future.wait(); // wait for result
	std::cout << "result=" << accumulate_future.get() << '\n';
	work_thread.join(); // wait for thread completion

	// Demonstrate using promise<void> to signal state between threads.
	//
	// 通过 promise 机制，child thread 可以通知 parent 一个事情干完了，将结果放到其中
	std::promise<void> barrier;
	std::future<void> barrier_future = barrier.get_future();
	std::thread new_work_thread(do_work, std::move(barrier));
	// 多个 wait 无需多个 set_value ，
	barrier_future.wait();
	barrier_future.wait();
	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);


	new_work_thread.join();
}
