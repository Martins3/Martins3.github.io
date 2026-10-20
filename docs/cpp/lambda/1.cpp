#include <bits/stdc++.h>
#include <iostream>
using namespace std;

class A {
    public:
	int num;
	A &operator=(const A &rhs)
	{
		std::cout << "=:\t" << rhs.num << std::endl;
		this->num = rhs.num;
		return *this;
	}

	A(int n)
	{
		this->num = n;
	};

	A(const A &foo)
	{
		std::cout << "copy:\t" << foo.num << std::endl;
		this->num = foo.num;
	}
};

void test1()
{
	A a(11);
	A b(22);

	// 自动捕获 a 和 m
	auto value_lambda = [a, b] { return 1; };
	std::cout << "1 : " << value_lambda() << std::endl;

	// 捕获使用 reference 的
	auto ref_lambda = [&a, b] { return 2; };
	std::cout << "2 : " << ref_lambda() << std::endl;

	// 捕获所有的
	auto automatic_value = [=] { return 3; };
	std::cout << "4 : " << automatic_value() << std::endl;

	// 捕获所有，但是按照 ref 的模式
	auto automatic_ref = [&] { return 4; };
	std::cout << "4 : " << automatic_ref() << std::endl;
}

// 解释说明一下 mutable 的含义:
// 可以理解为， 默认加 const 的是 lambda 生成的 operator() 成员函数，而不是参数或捕获变量本身。
auto counter2()
{
	int count = 0;
	auto f = [count]() mutable { return ++count; };
	// 这个是不行的，没有定义 mutable
	// auto g = [count]() { return ++count; };
	f();
	f();
	// 这里是拷贝，所以输出为 0
	std::cout << count << std::endl;
}

auto counter3()
{
	int count = 0;
	// 使用 [&count]，修改的是外部变量，不是闭包自身的成员状态，所以不需要 mutable
	auto f = [&count]() { return ++count; };
}

int main(int argc, char *argv[])
{
	counter2();
	return 0;
}
