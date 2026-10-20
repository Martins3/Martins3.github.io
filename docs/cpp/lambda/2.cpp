#include <iostream>

void a()
{
	// 最简单的 lamba
	auto m = []() { return 12; };
	std::cout << m() << std::endl;

	// 带上一个参数
	auto m2 = [](int gg) { return 12 + gg; };
	std::cout << m2(12) << std::endl;
}

void b()
{
	// 捕获，而且继续修改
	int gg = 0;
	auto m3 = [&gg](int m) { gg += m; };

	m3(1);
	m3(2);
	std::cout << gg << std::endl;

	auto m4 = [&](int m) { gg += m; };
}

void c()
{
	int gg = 0;
	auto m5 = [](int &&m) { return 2; };
	std::cout << m5(12) << std::endl;
	// 这显然不行，不过也没什么特别的了
	// std::cout << m5(gg) << std::endl;
}

class A {
    public:
	A()
	{
		std::cout << "create" << std::endl;
	}

	~A()
	{
		std::cout << "delete" << std::endl;
	}

	void show()
	{
		std::cout << "show" << std::endl;
	}
};

// lambda 本身就是对象，这意味着 lambda 可以被函数返回
// local reference 都是可以直接检查到的内容，c++ 并没有报错，
// 这是构建的出来的经典例子，还是程序员来保证了。
auto check()
{
	A a;
	auto m5 = [&a](int &&m) {
		a.show();
		return 2;
	};
	return m5;
}
auto rl()
{
	int gg = 0;
	auto f = check();
	std::cout << "call check now" << std::endl;
	f(12);
}

//  TODO 和 class 的耦合问题?
//
// 当然可以定义在 global 环境中定义，但是现在不可以捕获任何全部变量
// 因为根本就不需要捕获
int abc;
auto m4 = [](int m) { return 12 + abc; };

int main(int argc, char *argv[])
{
	// b();
	rl();
	return 0;
}
