#include <bits/stdc++.h>
#include <iostream>
using namespace std;

class A {
    public:
	int num;
	// 赋值运算符：仅当赋值双方对象都已完成构造后才被调用，例如 `d = a;`。
	// 注意区分拷贝构造：`A c = a;` 中 c 是新建对象，属于拷贝初始化，走拷贝构造函数；
	// 而 `d = a;` 中 d 已存在（A d(33) 已执行），才对已构造对象重新赋值，走本函数。
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

	// 拷贝构造函数：用一个已存在的对象初始化一个新对象时被调用，典型场景：
	// 1. 拷贝初始化：`A c = a;`（c 是新对象，虽写法带 =，但不走赋值运算符）；
	// 2. lambda 按值捕获：`[a, b]` 会把 a、b 逐一拷贝进闭包对象，输出中的 copy 即由此而来。
	A(const A &foo)
	{
		std::cout << "copy:\t" << foo.num << std::endl;
		this->num = foo.num;
	}
};

int main(int argc, char *argv[])
{
	A a(11);
	A b(22);

	A c = a;
	A d(33);
	d = a;

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

	return 0;
}
