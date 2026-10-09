// https://en.cppreference.com/w/cpp/iterator/iterator_traits
//
// 这么看，似乎方便 template 的使用
#include <iostream>
#include <iterator>
#include <list>
#include <vector>

template <typename BidirIt> void my_reverse(BidirIt first, BidirIt last)
{

	std::cout << typeid(BidirIt).name() << '\n';
	typename std::iterator_traits<BidirIt>::difference_type n =
		std::distance(first, last);
	for (--n; n > 0; n -= 2) {
		typename std::iterator_traits<BidirIt>::value_type tmp = *first;
		*first++ = *--last;
		*last = tmp;
	}
}

class A{

};

int main()
{
	std::vector<int> v{ 1, 2, 3, 4, 5 };
	my_reverse(v.begin(), v.end());
	for (int n : v)
		std::cout << n << ' ';
	std::cout << '\n';

	std::list<int> l{ 1, 2, 3, 4, 5 };
	my_reverse(l.begin(), l.end());
	for (int n : l)
		std::cout << n << ' ';
	std::cout << '\n';

	int a[]{ 1, 2, 3, 4, 5 };
	my_reverse(a, a + std::size(a));
	for (int n : a)
		std::cout << n << ' ';
	std::cout << '\n';

	// A m, n;
	// my_reverse(m, n);
	// TODO 为什么 iterator_traits 会知道 A 类型没有 difference_type ，
	// 这个不是 std::iterator_traits 定义的内容吗?
	// no type named ‘difference_type’ in ‘struct std::iterator_traits<A>’

	// std::istreambuf_iterator<char> i1(std::cin), i2;
	// my_reverse(i1, i2); // compilation error: i1, i2 are input iterators
	// 报错为 error: no match for ‘operator--’ (operand type is ‘std::istreambuf_iterator<char, std::char_traits<char> >’)
	// TODO 同样的，为什么 std::iterator_traits 的参数是 std::istreambuf_iterator<char, std::char_traits<char> > 就没有
	// operator-- 了
}
