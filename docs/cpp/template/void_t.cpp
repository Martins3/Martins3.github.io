// https://en.cppreference.com/w/cpp/types/void_t

#include <iomanip>
#include <iostream>
#include <map>
#include <type_traits>
#include <vector>

// Variable template that checks if a type has begin() and end() member functions
template <typename, typename = void> constexpr bool is_iterable = false;

template <typename T>
constexpr bool is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
					  decltype(std::declval<T>().end())> > =
	true;

// An iterator trait those value_type is the value_type of the iterated container,
// supports even back_insert_iterator (where value_type is void)

template <typename T, typename = void>
struct iterator_trait : std::iterator_traits<T> {
};

template <typename T>
struct iterator_trait<T, std::void_t<typename T::container_type> >
	: std::iterator_traits<typename T::container_type::iterator> {
};

class A {};

#define SHOW(...)                                                           \
	std::cout << std::setw(34) << #__VA_ARGS__ << " == " << __VA_ARGS__ \
		  << '\n'

int main()
{
	std::cout << std::boolalpha << std::left;
	SHOW(is_iterable<std::vector<double> >);
	SHOW(is_iterable<std::map<int, double> >);
	SHOW(is_iterable<double>);
	SHOW(is_iterable<A>);

	using container_t = std::vector<int>;
	container_t v;

	static_assert(std::is_same_v<
		      container_t::value_type,
		      iterator_trait<decltype(std::begin(v))>::value_type>);

	static_assert(std::is_same_v<container_t::value_type,
				     iterator_trait<decltype(std::back_inserter(
					     v))>::value_type>);
}
