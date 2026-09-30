// https://stackoverflow.com/questions/58694521/what-is-stdfalse-type-or-stdtrue-type
// https://stackoverflow.com/questions/20368187/when-would-i-use-stdintegral-constant-over-constexpr

#include <type_traits>
#include <iostream>

template <typename T> void use_impl(const T &, std::false_type)
{
	std::cout << "use_impl(false)" << std::endl;
}

template <typename T> void use_impl(const T &, std::true_type)
{
	std::cout << "use_impl(true)" << std::endl;
}

template <typename T> void use(const T &v)
{
	std::cout << std::is_integral<int>::type() << std::endl;
	use_impl(v, typename std::is_integral<T>::type());
}

int main()
{
	use(1);
	use(1.2);
}
