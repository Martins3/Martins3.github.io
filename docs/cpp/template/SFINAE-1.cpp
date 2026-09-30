// https://stackoverflow.com/questions/982808/what-are-good-uses-of-sfinae
//
// TODO 这个例子太复杂了，template/SFINAE-3.cpp 实现的简单多了

#include <iostream>
template <typename T> struct TypeSink {
	using Type = void;
};
template <typename T> using TypeSinkT = typename TypeSink<T>::Type;

// use case
template <typename T, typename = void>
struct HasBarOfTypeInt : std::false_type {
};
template <typename T>
struct HasBarOfTypeInt<T, TypeSinkT<decltype(std::declval<T &>().*(&T::bar))> >
	: std::is_same<typename std::decay<decltype(std::declval<T &>().*
						    (&T::bar))>::type,
		       int> {
};

struct S {
	int bar;
};
struct K {};

template <typename T, typename = TypeSinkT<decltype(&T::bar)> > void print(T)
{
	std::cout << "has bar" << std::endl;
}
void print(...)
{
	std::cout << "no bar" << std::endl;
}

int main()
{
	print(S{});
	print(K{});
	std::cout << "bar is int: " << HasBarOfTypeInt<S>::value << std::endl;
}
