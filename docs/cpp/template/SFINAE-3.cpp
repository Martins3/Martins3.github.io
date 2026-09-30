// https://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error
#include <iostream>
#include <type_traits>

// void type can only be created if the provided arguments are valid types.
template <typename... Ts> using void_t = void;
template <typename... Ts> using int_t = int;

template <typename T, typename = void>
struct has_type_foobar : std::false_type {
};

// std::void_t 可以直接替换为 void_t ，测试大致的意思就是，如果
// 如果存在，那么就是会用变为 void 类型
// 注意，这个类型是一个 partial specialization 的，必须是前面已经有定义才可以。
//
// 1. partial specialization 优先匹配
// 2. 这个 partial specialization 的存在是有前提的
template <typename T>
struct has_type_foobar<T, void_t<typename T::foobar> >
	: std::true_type {
};

struct foo {
	using foobar = float;
};

int main()
{
	std::cout << std::boolalpha;
	std::cout << has_type_foobar<int, int>::value << std::endl;
	std::cout << has_type_foobar<foo>::value << std::endl;
	std::cout << has_type_foobar<foo, void>::value << std::endl;
	std::cout << has_type_foobar<foo, int>::value << std::endl;
	std::cout << has_type_foobar<int, void>::value << std::endl;
	return 0;
}
