#include <iostream>
template <typename T> void func(T m)
{
	std::cout << "nice" << m.x << std::endl;
	// std::cout << "nice" << y << std::endl;
}

struct A {
	using value_type = int;
};

struct B {
	static constexpr int value_type = 42;
};

template <typename T> void foo()
{
	// 这里的 typename 是不可以少的
	// `typename` 告诉编译器，把这个 Dependent Qualified Name
	typename T::value_type x;
}

template <typename T> void foo2()
{
	T::value_type;
}

template <typename T> void foo(T &obj)
{
	obj.template convert<int>(); // Error
}

int main(int argc, char *argv[])
{
	foo<A>();
	// foo<B>();
	return 0;
}
