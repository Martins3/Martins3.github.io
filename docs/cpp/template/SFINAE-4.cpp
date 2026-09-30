#include <iostream>
struct X {
	using type = int;
};

struct Y {
	using type2 = int;
};

template <typename T> void foo(typename T::type)
{
	std::cout << "FOO1" << std::endl;
}
template <typename T> void foo(typename T::type2)
{
	std::cout << "FOO2" << std::endl;
}
template <typename T> void foo(T)
{
	std::cout << "FOO3" << std::endl;
}

template <typename T> void meow()
{
	T::a * 12;
}


void callFoo()
{
	foo<X>(5);
	foo<Y>(10);
	foo<int>(15);
}

int main(int argc, char *argv[])
{
	callFoo();
	return 0;
}
