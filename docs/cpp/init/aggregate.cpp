// https://cppreference.com/index.php?title=cpp/language/direct_initialization&variant=zh-hant
#include <bits/stdc++.h>
#include <iostream>
using namespace std;

class A {
	// public 是可以使用 {} 来构造的前提
    public:
	int a;
	int b;

	void show()
	{
		std::cout << this->a << " " << this->b << std::endl;
	}
};

class B {
    public:
	int a;
	int b;

	B(int _a, int _b)
		: a(_a)
		, b(_b)
	{
		std::cout << a << " " << b << std::endl;
	}
	void show()
	{
		std::cout << a << " " << b << std::endl;
	}
};

int main()
{
	// A 没有用户定义的构造函数。这是 C++20 支持的聚合类圆括号初始化：
	// 成员按照声明顺序初始化，效果分别类似 A{12, 0} 和 A{14, 15}。
	A a(12);
	A a1(13);
	A a2(14, 15);
	// std::cout << a.num << " " << a1.num2 << " " << a2.num2 << std::endl;

	// 这是不可以的，因为不存在这样的构造函数了
	// B b(12);
	// B b1(13);

	B b2(14, 15);
	B b3{ 1, 12 };
	b2.show();
	b3.show();

	return EXIT_SUCCESS;
}
