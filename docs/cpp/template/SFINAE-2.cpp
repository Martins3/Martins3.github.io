// https://en.wikipedia.org/wiki/Substitution_failure_is_not_an_error

#include <iostream>

template <typename T> struct has_type_foobar {
	// Types "yes" and "no" are guaranteed to have different sizes,
	// specifically sizeof(yes) == 1 and sizeof(no) == 2.
	using yes = char[1];
	using no = char[2];

	// 这里表示，只有你拥有了 foobar 类型，那么才可以匹配到这个函数
	template <typename C> static yes &test(typename C::foobar *);
	template <typename> static no &test(...);

	// If the "sizeof" of the result of calling test<T>(nullptr) is equal to
	// sizeof(yes), the first overload worked and T has a nested type named
	// foobar.
	static const bool value = sizeof(test<T>(nullptr)) == sizeof(yes);
};

struct foo {
	using foobar = float;
};

void test2()
{
	std::cout << std::boolalpha;
	std::cout << has_type_foobar<int>::value
		  << std::endl; // Prints false
	std::cout << has_type_foobar<foo>::value << std::endl; // Prints true
}

int main()
{
	test2();
	return 0;
}
