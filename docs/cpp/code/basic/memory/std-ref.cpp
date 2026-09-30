// https://en.cppreference.com/w/cpp/utility/functional/ref
//
// https://stackoverflow.com/questions/11833070/when-is-the-use-of-stdref-necessary/45823556#45823556
// 	一个例子
// https://stackoverflow.com/questions/76398544/why-is-ref-cref-needed-for-reference-arguments-to-a-function-passed-to-stdth
// 	说明 thread 的参数为什么需要是 std::ref

#include <functional>
#include <iostream>
#include <thread>

// 可以产生一个新的 object 出来
void f(int &n1, int &n2, const int &n3)
{
	std::cout << "In function: " << n1 << ' ' << n2 << ' ' << n3 << '\n';
	++n1; // increments the copy of n1 stored in the function object
	++n2; // increments the main()'s n2
	// ++n3; // compile error
}

void test1()
{
	int n1 = 1, n2 = 2, n3 = 3;
	// std::ref 和类似 std::bind 的配置使用
	// 在函数定义的时候就开始捕获
	std::function<void()> bound_f =
		std::bind(f, n1, std::ref(n2), std::cref(n3));
	n1 = 10;
	n2 = 11;
	n3 = 12;
	std::cout << "Before function: " << n1 << ' ' << n2 << ' ' << n3
		  << '\n';
	// 可以看到 ref 的结果始终使用的是同一个变量，而普通的传递当时已经拷贝过
	bound_f();
	std::cout << "After function: " << n1 << ' ' << n2 << ' ' << n3 << '\n';
}

void update(int &data) //expects a reference to int
{
	data = 15;
}

void update_copy(int data) //expects a reference to int
{
	data = 15;
}
void test2()
{
	int data = 10;
	int data2 = 10;

	// This doesn't compile as the data value is copied when its reference is expected.
	// std::thread t2(update, data);

	std::thread t1(update, std::ref(data)); // works
	std::thread t2(update_copy, data2); // works

	t1.join();
	std::cout << data << std::endl;
	std::cout << data2 << std::endl;
}

int main()
{
	test1();
	std::cout << "---" << std::endl;
	test2();
}
