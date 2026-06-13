#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <syncstream>
#include <thread>

using namespace std::chrono_literals;

// # cpp thread thread 和 async 的关系
// <!-- 4843a80e-1107-4aba-a94e-dc48f90a22fa -->
// 这个 demo 的目标不是“把线程 API 全背下来”, 而是建立一个选择直觉:
//
// 1. std::thread
//    - 核心语义: 我明确要启动一个线程
//    - 你自己负责 join()/detach()
//    - 函数返回值不会自动带回主线程, 通常要靠共享变量 / promise / packaged_task
//
// 2. std::jthread
//    - 还是“启动一个线程”, 但析构时会自动 join
//    - 额外支持 stop_token, 更适合“可协作停止”的线程函数
//    - 可以理解成“更不容易忘记收尾的 std::thread”
//
// 3. std::async
//    - 核心语义: 我想异步执行一个任务, 然后通过 future 拿结果
//    - 重点是结果和等待方式, 不是线程对象本身
//    - 它不一定真的立刻创建新线程; 如果用 deferred, 会等到 get()/wait() 才执行

void print_title(const char *title)
{
	std::cout << "\n=== " << title << " ===\n";
}

int slow_double(const std::string &name, int value, std::chrono::milliseconds delay)
{
	std::osyncstream(std::cout)
		<< name << ": start on thread " << std::this_thread::get_id() << '\n';
	std::this_thread::sleep_for(delay);
	int result = value * 2;
	std::osyncstream(std::cout)
		<< name << ": finish, result = " << result << '\n';
	return result;
}

void thread_worker(int input, int &output)
{
	// std::thread 不会帮你自动收集返回值, 所以这里把结果写回引用参数。
	output = slow_double("std::thread worker", input, 150ms);
}

void demo_std_thread()
{
	print_title("1. std::thread: 明确创建线程, 手动 join");

	int result = -1;

	// 观察点:
	// - 这里拿到的是线程对象, 不是 future
	// - 结果需要你自己想办法带回来
	// - 如果忘了 join()/detach(), 程序会直接 terminate
	std::thread worker(thread_worker, 21, std::ref(result));

	std::cout << "主线程还能继续做别的事, 此时 result 还没准备好 = "
		  << result << '\n';

	worker.join();
	std::cout << "join() 之后线程结束, result = " << result << '\n';
}

void jthread_worker(std::stop_token token)
{
	int round = 0;

	while (!token.stop_requested()) {
		std::osyncstream(std::cout)
			<< "std::jthread worker: round " << round
			<< ", thread id = " << std::this_thread::get_id() << '\n';
		std::this_thread::sleep_for(80ms);
		++round;
	}

	std::osyncstream(std::cout)
		<< "std::jthread worker: observe stop request, exit loop\n";
}

void demo_std_jthread()
{
	print_title("2. std::jthread: 自动 join, 可协作停止");

	// 观察点:
	// - 这里仍然是线程对象, 不是 future
	// - 但离开作用域时会自动 join, 不容易忘
	// - stop_token 让线程函数可以“被请求停止”
	{
		std::jthread worker(jthread_worker);
		std::this_thread::sleep_for(220ms);
		std::cout << "主线程调用 request_stop()\n";
		worker.request_stop();
		std::cout << "即将离开作用域, jthread 会自动 join\n";
	}

	std::cout << "作用域结束, jthread 已自动完成 join\n";
}

void demo_std_async_async()
{
	print_title("3. std::async(std::launch::async): 关注 future 和结果");

	// 观察点:
	// - std::async 返回的是 future<int>
	// - 你关心的是“结果何时能 get() 到”, 而不是线程对象本身
	// - 这里显式指定 launch::async, 让任务异步执行
	std::future<int> fut =
		std::async(std::launch::async, []() {
			return slow_double("std::async task", 30, 180ms);
		});

	std::cout << "主线程此时没有线程对象可 join, 只有 future 可等待\n";
	std::cout << "主线程先做一点别的事...\n";
	std::this_thread::sleep_for(50ms);

	int result = fut.get();
	std::cout << "fut.get() 拿回结果 result = " << result << '\n';
}

void demo_std_async_deferred()
{
	print_title("4. std::async(std::launch::deferred): 不一定立刻新建线程");

	// 观察点:
	// - deferred 的任务不会马上运行
	// - 它会等到 wait()/get() 时, 才在“调用者线程”里执行
	std::future<int> fut =
		std::async(std::launch::deferred, []() {
			return slow_double("deferred task", 50, 100ms);
		});

	std::cout << "future 已拿到, 但 deferred task 现在其实还没开始跑\n";
	std::cout << "下面调用 get(), 这时任务才真正开始执行\n";

	int result = fut.get();
	std::cout << "deferred 的结果 result = " << result << '\n';
}

int main()
{
	unsigned int n = std::thread::hardware_concurrency();
	std::cout << n << " concurrent threads are supported.\n";

	demo_std_thread();
	demo_std_jthread();
	demo_std_async_async();
	demo_std_async_deferred();

	std::cout << "\n总结:\n"
		  << "1. 想明确管理一个线程对象 -> std::thread\n"
		  << "2. 想少犯忘记 join 的错, 还想支持停止请求 -> std::jthread\n"
		  << "3. 想异步跑任务并拿结果 -> std::async + std::future\n"
		  << "4. async 关注的是结果, thread/jthread 关注的是线程对象\n";
}
