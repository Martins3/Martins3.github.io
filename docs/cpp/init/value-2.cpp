#include <bits/stdc++.h>
using namespace std;

class A {
    public:
	int a;
	int b;
	A(int a, int b)
	{
		std::cout << "nice" << std::endl;
	}
	void show()
	{
		std::cout << a << "" << b << std::endl;
	}
};

// 和 aggregate class 的关系
class B {
	int a;
	int b;

    public:
	void show()
	{
		std::cout << a << " " << b << std::endl;
	}
};

struct Agg {
	int a;
	int b;
};

// 是 value initialization 的前提非常搞
int main(int argc, char *argv[])
{
	A a = { 1, 2 };
	A a1{ 1, 2 };
	A a3(1, 2);
	a.show();
	a1.show();
	a3.show();

	B b1;
	B b2{};
	// 正常来说，你定义了 constructor，那么就无法再去自定义 A a() 了
	// 但是， aggregate 可以多开一个口子，如果你什么都没有，那么可以 ｛｝ 也可以 () 来直接初始化每一个变量
	// 不可以
	// B b3{1, 2};
	b1.show();
	b2.show();

	// 如果定义了构造函数，那么就用构造函数，构造函数中没有 default 自动设置为 0 的低性能操作
	// 1. 如果没有定义和定义了 default 的，都可以用如下的两种初始化方法
	// 也就是 https://cppreference.com/cpp/language/value_initialization 中的
	B c{};
	B c2;

	Agg g0;
	Agg g1{};
	Agg g2{ 1 };
	Agg g3{ 1, 3 };
	std::cout << g0.a << g0.b << std::endl;
	std::cout << g1.a << g1.b << std::endl;
	std::cout << g2.a << g2.b << std::endl;
	std::cout << g3.a << g3.b << std::endl;
	return 0;
}
