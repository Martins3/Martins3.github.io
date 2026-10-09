// # cpp initializer list
// <!-- 3917cfaa-cafa-441c-bf27-7c2ebcfb4740 -->
//
// 代码来自于: https://en.cppreference.com/w/cpp/utility/initializer_list
//
// std::initializer_list may be implemented as a pair of pointers or pointer and length.
// Copying a std::initializer_list does not copy the backing array of the corresponding initializer list.
//
// https://stackoverflow.com/questions/34957393/what-does-initializer-list-do
//
// 其实简单来说，就是为了用 { } 初始化而已
#include <initializer_list>
#include <iostream>
#include <vector>

template <class T> class S {
    public:
	std::vector<T> v;

	S(std::initializer_list<T> l)
		: v(l)
	{
		std::cout << "constructed with a " << l.size()
			  << "-element list\n";
	}

	void append(std::initializer_list<T> l)
	{
		v.insert(v.end(), l.begin(), l.end());
	}

	std::pair<const T *, std::size_t> c_arr() const
	{
		return { &v[0], v.size() };
		// copy list-initialization in return statement
		// this is NOT a use of std::initializer_list
	}
};

template <typename T> void templated_fn(T)
{
}

// 在增加一个函数，说明 initializer_list 也是可以用于普通的函数
template <typename T> void show(std::initializer_list<T> l)
{
	for (auto var : l) {
		std::cout << var << "\n";
	}
}

int main()
{
	S<int> s = { 1, 2, 3, 4, 5 }; // copy list-initialization
	s.append({ 6, 7, 8 }); // list-initialization in function call

	std::cout << "The vector now has " << s.c_arr().second << " ints:\n";

	for (auto n : s.v)
		std::cout << n << ' ';
	std::cout << '\n';

	std::cout << "Range-for over brace-init-list: \n";

	for (int x :
	     { -1, -2, -3 }) // the rule for auto makes this ranged-for work
		std::cout << x << ' ';
	std::cout << '\n';

	auto al = { 10, 11, 12 }; // special rule for auto

	std::cout << "The list bound to auto has size() = " << al.size()
		  << '\n';

	templated_fn<std::initializer_list<int> >({ 1, 2, 3 }); // OK
	templated_fn<std::vector<int> >({ 1, 2, 3 }); // also OK

	show({ 1, 2, 3 }); // OK
}
