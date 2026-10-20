// - explicit 的含义:
//   - explicit 修饰构造函数时，可以防止隐式转换和copy初始化
//       - https://stackoverflow.com/questions/34912221/is-list-initialization-an-implicit-conversion
//   - explicit 修饰转换函数时，可以防止隐式转换，但按语境转换除外
//   - /home/maritns3/core/CPlusPlusThings/basic_content/explicit/explicit.cpp 的例子
//
// explicit 关键字 : only declarations of constructors and conversion operators can be ‘explicit’
//
// 什么时候，使用 implict 是好的做法
// 想到了，就是在 vector 的 push 的时候，应该是一个很不错的例子

#include <iostream>

class A {
    public:
	// 我理解 implict 构造出现的地方
	// 1. 需要有构造函数和参数匹配
	// 2. 做类型转换
	A(int a)
	{
		std::cout << a << std::endl;
	};

	// 即便是两个参数，也是可以参与到 explict 中的
	A(int a, int b)
	{
		std::cout << "a b : " << a << " " << b << std::endl;
	};

	inline A &operator=(A &&) noexcept
	{
		std::cout << "operator move =" << std::endl;
		return *this;
	}

	inline A &operator=(A &)
	{
		std::cout << "operator copy =" << std::endl;
		return *this;
	}
};

class B {
    public:
	explicit B(int a)
	{
		std::cout << "a" << std::endl;
	};

	explicit B(int a, int b)
	{
		std::cout << "a b" << std::endl;
	};
};

void test(A &&a)
{
}

void test_implicit()
{
	A a = 12;
	A b = { 1, 2 };
	b = { 7, 8 };
	test({ 3, 4 });
	// 这个实际调用的是 A(int)，传入 2。 (1, 2) 是逗号表达式，结果为右操作数 2；它不是向构造函数传递两个参数。
	// 幸好 A 是有警告的
	// A b = (1, 2);
}

void test(B &&b)
{
}

void test_explict()
{
	// 这两个编译失败
	// B a = 12;
	// B b = { 1, 2 };
	// 这样都是没问题的
	//
	// TODO 还是没理解，现在想象就感觉奇怪了，为什么这个 initialize list 初始化
	// 和 implict 有关系?
	B a{ 12 };
	B b{ 1, 2 };
	// 这个会触发错误，即便是 B 是 explict ，让这里不存在
	// explict.cpp:71:13: error: call of overloaded ‘test(<brace-enclosed initializer list>)’ is ambiguous
	// test({ 3, 4 });
}

int main(int argc, char *argv[])
{
	test_implicit();
	std::cout << "----------" << std::endl;
	test_explict();
	return 0;
}
