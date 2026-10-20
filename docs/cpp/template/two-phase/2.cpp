#include <iostream>

void f(int)
{
	std::cout << "f(int)\n";
}

// 取决于这个 declaration 是否存在，会有不同的结果
void f(double);

template <typename T> void foo(T x)
{
	f(0);
	f(x);

	// 这两个都是不行的，基本上都是编译器确定啊
	// normal();
	// normal(x);
}

void normal() {};

void f(double)
{
	std::cout << "f(double)\n";
}

int main()
{
	foo(3.14);
}
