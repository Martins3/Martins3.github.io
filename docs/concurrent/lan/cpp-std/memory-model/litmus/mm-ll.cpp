// https://en.cppreference.com/w/cpp/atomic/memory_order 最下的案例
#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>

std::atomic<int> z{ 0 };
std::atomic<bool> x{ false }, y{ false };

// 这个测试想要说明一个事情，如果到达了 cache 之后，cache 必须保证顺序
//
// 如果不可以保证顺序，即便是 thread A 的两个 store 保持顺序，thread B 的两个 load 保持顺序
// 也会发现其实 thread B 可以加载的也是乱序的。
//
// 如果两个指令是一个 core 提交的，其他的 core 必须看到的顺序是一样的。
//
// 所以问题是，如果是两个 core 提交的，其他的 core 有必要看到的是一样吗?
// 我认为是没有必要的吧！
//
// 经过 cpp/mm-ll-loop.cpp 的测试，的确就是一旦提交给 cache ，那么 cache 就是需要保证顺序的
void thread1()
{
	x.store(true, std::memory_order_seq_cst);
}

void thread2()
{
	y.store(true, std::memory_order_seq_cst);
}

void read_x_then_y()
{
	while (!x.load(std::memory_order_seq_cst))
		;
	if (y.load(std::memory_order_seq_cst))
		++z;
}

void read_y_then_x()
{
	while (!y.load(std::memory_order_seq_cst))
		;
	if (x.load(std::memory_order_seq_cst))
		++z;
}

int main()
{
	std::thread c(read_x_then_y), d(read_y_then_x);
	std::thread a(thread1), b(thread2);

	a.join(), b.join(), c.join(), d.join();
	std::cout << z.load() << std::endl;
}
