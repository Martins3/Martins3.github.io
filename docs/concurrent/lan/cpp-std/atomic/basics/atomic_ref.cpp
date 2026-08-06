// # cpp atomic_ref 和 atomic 的关系
// <!-- 80415529-5e66-4fa7-9403-293f14820eb5 -->
//
// 注: 这个文件最容易绕晕的点就在这里:
// std::atomic<int> counter{ value }; 的意思是 “拿 value 当前的值, 拷贝出一个新的 atomic 对象”。
// 所以后面对 counter 做 ++, 改到的是 counter 自己, 不是外面的 value。
// 即使函数参数写成 int &value, 只要你内部 new 了一个 std::atomic<int>, 它也已经和原变量脱钩了。
// std::atomic_ref<int> counter{ value }; 才是真的 “给现有 value 套一个原子引用视图”。
// 之后对 counter 的 ++, 本质上就是原子地修改 value 本身, 这就是名字里 ref 的含义。
// 可以把它粗暴理解成:
// - std::atomic<T>    => 我自己存一份原子值
// - std::atomic_ref<T> => 我不存值, 我只是原子地操作别人那份值
// https://mariusbancila.ro/blog/2020/04/21/cpp20-atomic_ref/
#include <chrono>
#include <iostream>
#include <syncstream>
#include <thread>
#include <atomic>

int do_count(int value)
{
	std::atomic<int> counter{ value };

	std::vector<std::thread> threads;
	for (int i = 0; i < 10; ++i) {
		threads.emplace_back([&counter]() {
			for (int i = 0; i < 10; ++i) {
				++counter;
				{
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(50ms);
				}
			}
		});
	}

	for (auto &t : threads)
		t.join();

	return counter;
}

void do_count_ref(int &value)
{
	// 虽然参数是 int&, 但下面这一句仍然会“拷贝出一个新的 atomic”,
	// 所以线程修改的是 counter, 不是外面的 value。
	std::atomic<int> counter{ value };

	std::vector<std::thread> threads;
	for (int i = 0; i < 10; ++i) {
		threads.emplace_back([&counter]() {
			for (int i = 0; i < 10; ++i) {
				++counter;
				{
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(50ms);
				}
			}
		});
	}

	for (auto &t : threads)
		t.join();
}

void array_inc(std::vector<int> &arr, size_t const i)
{
	std::atomic<int> elem{ arr[i] };
	elem++;
}

int test3()
{
	std::vector<int> arr{ 0, 0, 0 };
	array_inc(arr, 0);
	std::cout << arr[0] << '\n'; // prints 1
	return 0;
}

void do_count_ref_real(int &value)
{
	// 这里没有新建一份值, 而是直接把 value 本身当作原子对象来看待。
	// 所以线程里的 ++counter 会真实反映到 value 上。
	std::atomic_ref<int> counter{ value };

	std::vector<std::thread> threads;
	for (int i = 0; i < 10; ++i) {
		threads.emplace_back([&counter]() {
			for (int i = 0; i < 10; ++i) {
				++counter;
				{
					using namespace std::chrono_literals;
					std::this_thread::sleep_for(50ms);
				}
			}
		});
	}

	for (auto &t : threads)
		t.join();
}

// 注意: 这个 demo 的核心不是“线程有没有跑起来”, 而是“最后写回的是谁”。
// 也就是: std::atomic 是拷贝一份来改, std::atomic_ref 是直接改原对象。
int main()
{
	int result = do_count(0);
	std::cout << "do_count : " << result << '\n'; // prints 100

	int value = 0;
	do_count_ref(value);
	std::cout << "do_count_ref : " << value << '\n'; // prints 100

	test3();

	do_count_ref_real(value);
	std::cout << "do_count_ref_real : " << value << '\n'; // prints 100
}
