// # cpp stop_token / stop_source
// <!-- d666ab8e-40c0-40aa-9e37-5730fdd3d87f -->
//
// stop_token 是 C++20 提供的一套“协作式停止”机制，用来通知线程：
// “应该准备结束了”。
// 它不是像 kill 那样把线程强行打断，而是由线程自己在合适的位置检查
// stop_requested()，然后主动清理资源并退出。
//
// 通常分成两个角色：
// 1. stop_source: 用来发出停止请求
// 2. stop_token : 用来观察是否已经收到停止请求
//
// 这种方式适合有循环、阻塞等待、后台 worker 的线程函数，
// 可以让线程退出过程更安全、可控，也更容易和 RAII 配合。
//
// https://en.cppreference.com/w/cpp/thread/stop_source
// https://en.cppreference.com/w/cpp/thread/
#include <iostream>
#include <thread>
#include <chrono>
#include <iostream>
#include <stop_token>
#include <thread>

using namespace std::literals::chrono_literals;
using namespace std::chrono_literals;

void func(std::stop_token stop_token, int value)
{
	// jthread 会把自己的 stop_token 作为第一个参数传进来。
	// 线程函数只需要周期性检查 stop_requested()，就能实现协作式停止。
	while (!stop_token.stop_requested()) {
		std::cout << value++ << ' ' << std::flush;
		std::this_thread::sleep_for(200ms);
	}
	// 收到停止请求后主动退出，而不是被强行中断。
	std::cout << std::endl;
}

void test_stop_token()
{
	// 这里没有显式传 stop_token。
	// 因为 f 的第一个参数是 std::stop_token，jthread 会自动补上它。
	std::jthread thread(func, 5);
	// prints 5 6 7 8... for approximately 3 seconds
	std::this_thread::sleep_for(10s);
	// jthread 析构时会自动 request_stop() + join()。
	// 所以离开作用域后，f 中的 stop_requested() 会变成 true，线程随后退出。
}

void worker_fun(int id, std::stop_token stoken)
{
	for (int i = 10; i; --i) {
		std::this_thread::sleep_for(300ms);
		// stop_token 本身只是一份“只读停止视图”：
		// 别的线程调用 request_stop() 后，这里就能观察到请求。
		if (stoken.stop_requested()) {
			std::printf("  worker%d is requested to stop\n", id);
			return;
		}
		std::printf("  worker%d goes back to sleep\n", id);
	}
}

int test_main()
{
	std::jthread threads[4];
	std::cout << std::boolalpha;
	auto print = [](const std::stop_source &source) {
		std::printf(
			"stop_source stop_possible = %s, stop_requested = %s\n",
			source.stop_possible() ? "true" : "false",
			source.stop_requested() ? "true" : "false");
	};

	// 一个 stop_source 可以派生出多个 stop_token。
	// 这样就能把“发出停止请求”和“观察停止请求”分离开。
	std::stop_source stop_source;

	print(stop_source);

	// 为 4 个 worker 显式传入同一个 stop_token。
	// 这些线程共享同一份停止状态，谁都可以同时看到 stop 请求。
	for (int i = 0; i < 4; ++i)
		threads[i] = std::jthread(worker_fun, i + 1,
					  stop_source.get_token());

	std::this_thread::sleep_for(1000ms);

	std::puts("Request stop");
	// request_stop() 会把共享停止状态标记为 true。
	// 之后所有关联 token 的 stop_requested() 都会尽快观察到这个变化。
	stop_source.request_stop();

	print(stop_source);

	// jthread 析构时还会自动 join，因此这里不需要手动等待线程结束。
	return 0;
}

int main(int argc, char *argv[])
{
	test_stop_token();
	test_main();
	return 0;
}
