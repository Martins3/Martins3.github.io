// cpp mutex 基本了解
// <!-- 70e011d7-8f83-4c96-adbc-9d5e78d98df9 -->
// 这个 demo 只演示 std::mutex 相关的常见 lock/unlock 写法。
// 重点先记住一句:
// 1. 有 std::lock(...) 这个自由函数, 用来一次性锁多个 mutex。
// 2. 类似 std::lock ，也存在 std::try_lock
// 2. 但没有 std::unlock(...) 这个自由函数。
// 3. 解锁通常靠三种方式:
//    - m.unlock()                -> 手动解锁 mutex
//    - lk.unlock()               -> 手动解锁 unique_lock
//    - 离开作用域自动析构解锁      -> lock_guard / unique_lock / scoped_lock
// 不提供 std::unlock 是非常好的，不然释放锁真的很繁琐，容易出现错误
//
// 这几个 tag 的意思:
// - std::defer_lock: 先创建锁对象, 但现在先别锁
// - std::try_to_lock: 现在试一下能不能锁, 锁不到就算了
// - std::adopt_lock: 锁已经提前拿到了, 你现在只负责“接管并在析构时解锁”
//
// 三个最常用 RAII 锁对象的横向对比:
// 1. std::lock_guard
//    - 最简单, 最轻量, 语义就是“进作用域加锁, 出作用域解锁”
//    - 没有 unlock(), 也不负责 defer/try 这类灵活控制
//    - 适合最普通的单锁临界区
//
// 2. std::unique_lock
//    - 功能最多, 可以手动 lock()/unlock(), 可以看 owns_lock(), 也支持 defer/try/adopt
//    - 适合需要“先不锁, 稍后再锁” 或 “中途先放锁再继续” 的场景
//    - 也因为更灵活, 所以更容易把锁状态写复杂
//
// 3. std::scoped_lock
//    - 可以看成“多锁场景下更省心的 RAII 写法”
//    - 构造时会安全地拿下多把 mutex, 省掉自己写 std::lock + adopt_lock
//    - 但它不像 unique_lock 那样支持手动 unlock() 这种细粒度控制
//
// 一句粗暴选择规则:
// - 单锁, 代码很直白                -> lock_guard
// - 需要手动控制加锁/解锁时机       -> unique_lock
// - 多锁, 且只想作用域内自动管理    -> scoped_lock

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

void print_title(const char *title)
{
	std::cout << "\n=== " << title << " ===\n";
}

void demo_manual_lock_unlock()
{
	print_title("1. 手动 mutex.lock() / mutex.unlock()");

	std::mutex m;
	int value = 0;

	std::cout << "加锁前 value = " << value << '\n';
	m.lock();
	std::cout << "调用 m.lock() 之后进入临界区\n";
	++value;
	std::cout << "临界区内 ++value, 现在 value = " << value << '\n';
	m.unlock();
	std::cout << "调用 m.unlock() 之后离开临界区\n";
}

void demo_lock_guard()
{
	print_title("2. std::lock_guard 自动解锁");
	// 观察点:
	// - 构造时加锁, 析构时解锁
	// - 没有 guard.unlock()
	// - 如果你只需要一小段最普通的互斥代码, 它通常就是首选

	std::mutex m;
	int value = 0;

	{
		std::lock_guard<std::mutex> guard(m);
		std::cout << "构造 lock_guard 时自动加锁\n";
		++value;
		std::cout << "临界区内 value = " << value << '\n';
	}

	std::cout << "离开作用域后, lock_guard 析构并自动解锁\n";
}

void demo_unique_lock_basic()
{
	print_title("3. std::unique_lock: 能手动 unlock()/lock()");
	// 观察点:
	// - 仍然有 RAII, 但比 lock_guard 多了“锁状态可编程”
	// - 可以中途 unlock(), 之后再 lock()
	// - 所以适合复杂流程, 但不适合为了炫技而滥用

	std::mutex m;
	int value = 0;

	std::unique_lock<std::mutex> lock(m);
	std::cout
		<< "构造 unique_lock 时自动加锁, owns_lock = " << std::boolalpha
		<< lock.owns_lock() << '\n';

	++value;
	std::cout << "第一次进入临界区后 value = " << value << '\n';

	lock.unlock();
	std::cout << "调用 lock.unlock() 后, owns_lock = " << lock.owns_lock()
		  << '\n';

	lock.lock();
	std::cout << "再次调用 lock.lock() 后, owns_lock = " << lock.owns_lock()
		  << '\n';

	++value;
	std::cout << "第二次进入临界区后 value = " << value << '\n';
	std::cout << "函数结束时如果 lock 仍持有锁, 析构时会自动解锁\n";
}

void demo_defer_lock()
{
	print_title("4. std::defer_lock: 先创建, 后加锁");

	std::mutex m;
	int value = 0;

	std::unique_lock<std::mutex> lock(m, std::defer_lock);
	std::cout << "刚构造时并没有真的加锁, owns_lock = " << lock.owns_lock()
		  << '\n';

	lock.lock();
	std::cout << "调用 lock.lock() 后才真正拿到锁, owns_lock = "
		  << lock.owns_lock() << '\n';

	++value;
	std::cout << "临界区内 value = " << value << '\n';

	lock.unlock();
	std::cout << "手动 unlock 之后, owns_lock = " << lock.owns_lock()
		  << '\n';
}

void demo_try_to_lock()
{
	print_title("5. std::try_to_lock: 尝试加锁, 不阻塞");

	std::mutex m;
	std::atomic<bool> ready{ false };

	std::thread holder([&]() {
		std::lock_guard<std::mutex> guard(m);
		ready.store(true, std::memory_order_release);
		std::this_thread::sleep_for(200ms);
	});

	while (!ready.load(std::memory_order_acquire))
		std::this_thread::yield();

	std::unique_lock<std::mutex> try_lock_1(m, std::try_to_lock);
	std::cout << "别的线程持有锁时尝试加锁, owns_lock = "
		  << try_lock_1.owns_lock() << '\n';

	holder.join();

	std::unique_lock<std::mutex> try_lock_2(m, std::try_to_lock);
	std::cout << "对方释放后再次 try_to_lock, owns_lock = "
		  << try_lock_2.owns_lock() << '\n';
}

void demo_adopt_lock_single()
{
	print_title("6. std::adopt_lock: 锁已提前拿到, 交给 RAII 接管");

	std::mutex m;

	m.lock();
	std::cout << "这里是先手动 m.lock()\n";

	{
		std::lock_guard<std::mutex> guard(m, std::adopt_lock);
		std::cout << "guard 使用 adopt_lock 接管这把已加锁的 mutex\n";
		std::cout << "因此这里不能再 lock 一次, 否则就是重复加锁\n";
	}

	std::cout << "离开作用域后, guard 析构并自动把刚才那把锁解掉\n";
}

void demo_std_lock_with_adopt_lock()
{
	print_title("7. std::lock(m1, m2) + adopt_lock");

	std::mutex m1;
	std::mutex m2;
	int a = 10;
	int b = 20;

	std::lock(m1, m2);
	std::cout << "std::lock(m1, m2) 已经一次性拿到两把锁\n";

	{
		std::lock_guard<std::mutex> guard1(m1, std::adopt_lock);
		std::lock_guard<std::mutex> guard2(m2, std::adopt_lock);
		++a;
		++b;
		std::cout << "guard1/guard2 接管两把锁, a = " << a
			  << ", b = " << b << '\n';
	}

	std::cout << "两个 lock_guard 析构后, m1/m2 都自动解锁\n";
}

void demo_std_lock_with_defer_lock()
{
	print_title("8. unique_lock + defer_lock + std::lock");

	std::mutex m1;
	std::mutex m2;
	int a = 1;
	int b = 2;

	std::unique_lock<std::mutex> lock1(m1, std::defer_lock);
	std::unique_lock<std::mutex> lock2(m2, std::defer_lock);

	std::cout << "此时 lock1/lock2 只是锁句柄, 还没真正加锁\n";
	std::lock(lock1, lock2);
	std::cout << "std::lock(lock1, lock2) 之后, 两把锁一起安全拿到\n";

	a += 10;
	b += 20;
	std::cout << "修改后 a = " << a << ", b = " << b << '\n';
	std::cout << "函数结束时, 两个 unique_lock 会自动解锁\n";
}

void demo_scoped_lock()
{
	print_title("9. std::scoped_lock: C++17 推荐的多锁 RAII 写法");
	// 观察点:
	// - 它最像“多锁版 lock_guard”
	// - 重点不是灵活, 而是让多把锁一起管理时更简单更不容易写错
	// - 如果你不需要手动 unlock(), 多锁优先考虑它

	std::mutex m1;
	std::mutex m2;
	int total = 0;

	{
		std::scoped_lock<std::mutex, std::mutex> guard(m1, m2);
		std::cout << "scoped_lock 构造时一次性安全锁住多把 mutex\n";
		total += 3;
		std::cout << "临界区内 total = " << total << '\n';
	}

	std::cout << "离开作用域后, scoped_lock 自动解锁所有持有的 mutex\n";
}

int another_example()
{
	int foo_count = 0;
	std::mutex foo_count_mutex;

	auto increment = [](int &counter, std::mutex &m, const char *desc) {
		for (int i = 0; i < 10000000; ++i) {
			//这里我在说明一个例子，就是对于这种简单场景，unique_lock 和 lock_guard 是没什么区别的
			std::unique_lock<std::mutex> lock(m);
			/* std::lock_guard<std::mutex> lock(m); */
			++counter;
			// 我靠，这个有没有居然都是可以的 !
			/* lock.unlock(); */
		}
	};

	std::thread increment_foo(increment, std::ref(foo_count),
				  std::ref(foo_count_mutex), "foo");
	std::thread increment_bar(increment, std::ref(foo_count),
				  std::ref(foo_count_mutex), "bar");

	increment_foo.join();
	increment_bar.join();
	std::cout << "ok : " << foo_count << std::endl;
	return 0;
}

int main()
{
	std::cout << std::boolalpha;

	demo_manual_lock_unlock();
	demo_lock_guard();
	demo_unique_lock_basic();
	demo_defer_lock();
	demo_try_to_lock();
	demo_adopt_lock_single();
	demo_std_lock_with_adopt_lock();
	demo_std_lock_with_defer_lock();
	demo_scoped_lock();
	another_example();

	std::cout
		<< "\n总结:\n"
		<< "1. 单 mutex, 想最简单自动解锁 -> lock_guard\n"
		<< "2. 需要手动 unlock()/lock() 或延迟加锁 -> unique_lock\n"
		<< "3. 需要同时锁多把 mutex, 且不想自己管顺序 -> scoped_lock\n"
		<< "4. 需要多锁且还想手动控制时机 -> unique_lock + defer_lock + std::lock\n"
		<< "5. 没有 std::unlock(...), 解锁靠成员函数或析构\n";
}
