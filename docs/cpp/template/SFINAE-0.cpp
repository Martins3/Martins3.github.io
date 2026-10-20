#include <iostream>

template <class T> void zoo(T)
{
	std::cout << "zoo template\n";
}

// bar
template <class T> void bar(T)
{
	std::cout << "bar template\n";
}

void bar(int)
{
	std::cout << "bar int overload\n";
}

// foo 的实现
struct A {
	using value_type = int;
};

// 注意第二个参数有默认值，因此只传一个实参也能考虑该模板；但编译器仍然必须检查第二个参数的类型。
template <class T> void foo(T, typename T::value_type * = nullptr)
{
	std::cout << "foo: template : value_type\n";
}

template <class T> void foo(T)
{
	std::cout << "foo : template\n";
}

struct Test {
	using foo = double;
};

template <typename T> void f(typename T::foo)
{
	std::cout << "typename T::foo" << std::endl;
}

template <typename T> void f(T)
{
	std::cout << "T" << std::endl;
}

int main()
{
	// 虽然 foo 是 template ，但是可以根据参数类型来自动推导
	// 所以就不用写成 foo<int>(124) 了
	foo(124);

	// 先比较实参转换；在这里匹配同样好时，非模板函数优先。
	bar(12); // 两者都是精确匹配，选择非模板 bar(int)。
	bar<int>(12); // 显式指定模板实参，只考虑函数模板。
	bar(12.0); // 模板精确匹配 double，胜过需要 double -> int 转换的非模板。

	// foo<A>(A{}); // 两个候选都可行且无法选出更好的一个，产生歧义。
	// int 没有 value_type，带第二个参数的候选替换失败，只留下 foo(T)。
	foo<int>(12);

	// 这个不会出现 ambiguous 的错误
	f<Test>(10);
	f<int>(10);
}
