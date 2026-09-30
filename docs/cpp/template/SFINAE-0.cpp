#include <iostream>

template <class T> void zoo(T x)
{
	std::cout << "zoo template\n";
}

// bar
template <class T> void bar(T x)
{
	std::cout << "bar template\n";
}

void bar(int x)
{
	std::cout << "bar int overload\n";
}

// foo 的实现
struct A {
	using value_type = int;
};

// 注意第二个参数有默认值，因此只传一个实参也能考虑该模板；但编译器仍然必须检查第二个参数的类型。
template <class T> void foo(T x, typename T::value_type * = nullptr)
{
	std::cout << "foo: template : value_type\n";
}

template <class T> void foo(T x)
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

	// 当然，如果定义了 foo 的非 template 版本，而且可以匹配上，优先使用非 template 的版本
	// 有点类似 partial specialization 会优先匹配
	bar(12);
	bar<int>(12);
	bar(12.0);

	// 终于到达来讲到 SFINAE 的例子了，这里
	// 这里 foo 42
	A a;
	// foo<A>(a); // 这个会触发错误，因为对于两个 foo 的定义，A 都是可以匹配上的
	// 但是 foo<int>(12) 没问题，因为 template <class T> void foo(T x, typename T::value_type * = nullptr) 而言
	// 根本就不存在
	foo<int>(12);

	// 这个不会出现 ambiguous 的错误
	f<Test>(10);
	f<int>(10);
}
