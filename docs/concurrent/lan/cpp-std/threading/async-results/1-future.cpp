// # cpp future 功能是做什么的
// <!-- 76be221f-fd37-4c5a-8d13-693e93402756 -->
//
// 了解这些东西还是非常有必要的，例如 rust 中其实也是这些的东西
// https://news.ycombinator.com/item?id=48019163
//
// 这个文件想解决一个最常见的困惑:
// “future 到底是什么? 为什么它总是和 promise / packaged_task / async 混在一起?”
//
// 最短答案:
// 1. std::future<T> 不是“启动任务”的工具, 它只是“将来某个结果会到这里来”的收件箱。
// 2. 你可以把 future 理解成“读结果的一端”。
// 3. promise / packaged_task / async 则是“生产这个结果的不同方式”。
//
// 粗暴类比:
// - future          -> 取快递的取件码和取件口
// - promise         -> 某个线程承诺以后把包裹放进去
// - packaged_task   -> 把一个可调用对象打包起来, 它跑完后自动把结果塞进 future
// - async           -> 最省事, 帮你启动异步任务, 并直接把 future 给你
//
// 这个 demo 会分 4 步:
// 1. 先看 future 最基本的 wait()/get() 语义
// 2. 再看 promise 怎么往 future 里送结果
// 3. 再看 packaged_task 怎么把“函数返回值”接到 future
// 4. 最后看 async 这个最省事的写法

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <syncstream>
#include <thread>

using namespace std::chrono_literals;

void print_title(const char *title)
{
	std::cout << "\n=== " << title << " ===\n";
}

int slow_square(const std::string &name, int value, std::chrono::milliseconds delay)
{
	std::osyncstream(std::cout)
		<< name << ": start on thread " << std::this_thread::get_id() << '\n';
	std::this_thread::sleep_for(delay);
	int result = value * value;
	std::osyncstream(std::cout)
		<< name << ": finish, result = " << result << '\n';
	return result;
}

void demo_future_basics()
{
	print_title("1. future 基本直觉: wait 是等, get 是取结果");

	std::promise<int> promise;
	std::future<int> future = promise.get_future();

	// 这里用 promise 只是为了制造一个 future。
	// 真正想说明的是:
	// - future.wait() 只负责“等结果准备好”
	// - future.get() 会“取结果”, 同时 future 自己也会被消费掉
	std::thread producer([promise = std::move(promise)]() mutable {
		std::this_thread::sleep_for(120ms);
		promise.set_value(42);
	});

	auto status = future.wait_for(10ms);
	std::cout << "刚开始 wait_for(10ms), ready? "
		  << (status == std::future_status::ready) << '\n';

	future.wait();
	std::cout << "future.wait() 返回, 表示结果已经准备好了\n";

	int result = future.get();
	std::cout << "future.get() 取到结果 result = " << result << '\n';
	std::cout << "注意: 同一个 future 的 get() 只能调用一次\n";

	producer.join();
}

void demo_promise()
{
	print_title("2. promise -> future: 手动承诺以后给结果");

	// promise 是“写结果的一端”, future 是“读结果的一端”。
	// 适合这样的场景:
	// “任务怎么执行我自己决定, 但最后想把结果发给另一个线程”
	std::promise<int> promise;
	std::future<int> future = promise.get_future();

	std::thread worker([promise = std::move(promise)]() mutable {
		int result = slow_square("promise worker", 7, 150ms);
		promise.set_value(result);
	});

	std::cout << "主线程现在手里只有 future, 所以它只能等和取结果\n";
	int result = future.get();
	std::cout << "future.get() 收到 promise 发来的结果 = " << result << '\n';

	worker.join();
}

void demo_packaged_task()
{
	print_title("3. packaged_task -> future: 把函数执行结果接到 future");

	// packaged_task 可以理解成:
	// “把一个可调用对象包装起来, 之后不管谁来执行它,
	//  它的返回值都会自动流进对应的 future”
	std::packaged_task<int()> task([]() {
		return slow_square("packaged_task worker", 8, 170ms);
	});

	std::future<int> future = task.get_future();

	// 这里把 task 移动进 thread, 让它在线程里执行。
	// 但它也不一定非要配 thread, 你甚至可以在当前线程里直接 task()。
	std::thread worker(std::move(task));

	std::cout << "主线程不需要知道函数细节, 只需要 future.get()\n";
	int result = future.get();
	std::cout << "packaged_task 对应的结果 = " << result << '\n';

	worker.join();
}

void demo_async()
{
	print_title("4. async -> future: 最省事的异步返回值写法");

	// async 是最像“帮你一条龙打包好”的:
	// 你给它一个函数, 它返回 future。
	// 你通常不直接操作 promise / packaged_task。
	std::future<int> future =
		std::async(std::launch::async, []() {
			return slow_square("async worker", 9, 180ms);
		});

	std::cout << "调用 async 后直接拿到 future\n";
	std::cout << "主线程继续做别的事...\n";
	std::this_thread::sleep_for(50ms);

	int result = future.get();
	std::cout << "async 对应的结果 = " << result << '\n';
}

int main()
{
	demo_future_basics();
	demo_promise();
	demo_packaged_task();
	demo_async();

	std::cout << "\n总结:\n"
		  << "1. future 只负责“等结果/取结果”, 它不是启动线程的工具\n"
		  << "2. promise 是手动把结果塞给 future\n"
		  << "3. packaged_task 是把“函数返回值”自动接给 future\n"
		  << "4. async 是最省事的 future 来源\n";
}
