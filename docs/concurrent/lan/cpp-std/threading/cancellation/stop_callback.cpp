// cpp stop_callback 的基本用法
// <!-- 090a5798-1363-49e6-a563-679b741a7c6b -->
//
// stop_callback 是 C++20 协作式停止机制的一部分。
// 它允许你把一个回调函数绑定到 stop_token 上：
// 一旦有人对关联的 stop_source / jthread 发出停止请求，
// 这个回调就会被自动执行。
//
// 它常见的用途不是“结束线程本身”，而是在线程准备退出时做一些配套动作，
// 比如唤醒等待中的线程、打日志、通知其他对象、做轻量清理等。
//
// 需要注意：
// 1. stop_callback 通常运行在“发出 request_stop() 的那个线程”里
// 2. 如果注册回调时停止请求已经发生，那么回调会立刻执行
// 3. 如果 stop_callback 对象先析构了，那么这个回调就不会再触发
//
// https://en.cppreference.com/w/cpp/thread/stop_callback
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

using namespace std::chrono_literals;

// Use a helper class for atomic std::cout streaming.
class Writer {
	std::ostringstream buffer;

    public:
	~Writer()
	{
		std::cout << buffer.str();
	}
	Writer &operator<<(auto input)
	{
		buffer << input;
		return *this;
	}
};

int main()
{
	// A worker thread.
	// It will wait until it is requested to stop.
	std::jthread worker([](std::stop_token stoken) {
		Writer() << "Worker thread's id: " << std::this_thread::get_id()
			 << '\n';
		std::mutex mutex;
		std::unique_lock lock(mutex);
		// condition_variable_any 提供了支持 stop_token 的 wait 重载。
		// 这样线程既可以等待条件成立，也可以在收到 stop 请求时被唤醒。
		std::condition_variable_any().wait(lock, stoken, [&stoken] {
			return stoken.stop_requested();
		});
	});

	// Register a stop callback on the worker thread.
	// 这里注册的 callback 和 worker 共享同一份停止状态。
	// 只要 worker 对应的 stop 状态被请求停止，这个回调就会执行。
	std::stop_callback callback(worker.get_stop_token(), [] {
		Writer() << "Stop callback executed by thread: "
			 << std::this_thread::get_id() << '\n';
	});

	// Stop_callback objects can be destroyed prematurely to prevent execution.
	{
		// 这个回调只在当前作用域内有效。
		// 由于它在真正 request_stop() 之前就析构了，所以后面不会执行。
		std::stop_callback scoped_callback(worker.get_stop_token(), [] {
			// This will not be executed.
			Writer() << "Scoped stop callback executed by thread: "
				 << std::this_thread::get_id() << '\n';
		});
	}

	// Demonstrate which thread executes the stop_callback and when.
	// Define a stopper function.
	auto stopper_func = [&worker] {
		// request_stop() 只有第一次调用会真正把停止状态从 false 改成 true。
		// 因此多个线程竞争时，只有一个线程会返回 true。
		if (worker.request_stop())
			Writer() << "Stop request executed by thread: "
				 << std::this_thread::get_id() << '\n';
		else
			Writer() << "Stop request not executed by thread: "
				 << std::this_thread::get_id() << '\n';
	};

	// Let multiple threads compete for stopping the worker thread.
	std::jthread stopper1(stopper_func);
	std::jthread stopper2(stopper_func);
	stopper1.join();
	stopper2.join();

	sleep(2);

	// After a stop has already been requested,
	// a new stop_callback executes immediately.
	Writer() << "Main thread: " << std::this_thread::get_id() << '\n';
	// 因为这里 stop 状态已经是“已请求停止”，
	// 所以新构造的 stop_callback 会在构造过程中立刻执行。
	std::stop_callback callback_after_stop(worker.get_stop_token(), [] {
		Writer() << "Stop callback executed by thread: "
			 << std::this_thread::get_id() << '\n';
	});
}
