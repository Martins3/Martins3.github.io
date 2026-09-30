#include <atomic>
#include <chrono>
#include <iostream>
#include <syncstream>
#include <thread>

class Student {
	int a;
	int b;
};

class Student2 {
	double a;
	double b;
	int c[100];
};

using namespace std::chrono_literals;

std::atomic<int> counter = 0;

void foo()
{
	std::thread::id this_id = std::this_thread::get_id();

	std::osyncstream(std::cout) << "thread " << this_id << " sleeping...\n";
	for (size_t i = 0; i < 1000000; i++) {
		// 都是可以正确保证的，看来 atomic class 将这些符号重载了
		/* counter++; */
		counter += 10;
	}
}

void test_counter()
{
	counter = 0;
	std::jthread t1{ foo };
	std::jthread t2{ foo };

	t1.join();
	t2.join();
	std::cout << counter.load() << std::endl;
}

void test_lock_free()
{
	std::atomic<int> i{ 1 };
	std::cout << decltype(i)::is_always_lock_free << std::endl;

	std::atomic<float> f{ 1.0f };
	std::cout << decltype(f)::is_always_lock_free << std::endl;

	std::atomic<Student> s;
	std::cout << decltype(s)::is_always_lock_free << std::endl;
	std::cout << s.is_always_lock_free << std::endl;

	std::atomic<Student2> s2;
	std::cout << decltype(s2)::is_always_lock_free << std::endl;
	std::cout << s2.is_always_lock_free << std::endl;
	// 使用 c++ 20 这两个会构建失败的
	/* std::cout << s2.is_lock_free() << std::endl; */
	/* std::cout << std::atomic_is_lock_free(&s2) << std::endl; */
}

int main()
{
	test_counter();
	test_lock_free();
	return 0;
}
