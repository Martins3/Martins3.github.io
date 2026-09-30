#include <bits/stdc++.h>
#include <iostream>
using namespace std;

class Trace {
    public:
	explicit Trace(int value)
		: num(value)
	{
		std::cout << "constructor: " << num << '\n';
	}

	Trace(const Trace &other)
		: num(other.num)
	{
		std::cout << "copy constructor: " << num << '\n';
	}

	Trace &operator=(const Trace &other)
	{
		num = other.num;
		std::cout << "copy assignment: " << num << '\n';
		return *this;
	}

	int num;
};

int main()
{
	Trace source(20);

	// result 在这一行才被创建，所以调用拷贝构造函数。
	Trace result(source); // 直接初始化（direct-initialization）

	// 虽然写了等号，但 copy 也在这一行才被创建，仍调用拷贝构造函数。
	Trace copy = source; // 拷贝初始化（copy-initialization）

	// assigned 已经由上一行构造完成；这里修改已有对象，调用拷贝赋值运算符。
	Trace assigned(0);
	assigned = source;

	std::cout << "values: " << result.num << " " << copy.num << " "
		  << assigned.num << '\n';
	return 0;
}
