// https://en.cppreference.com/w/cpp/iterator/iterator_traits
//
// 这么看，似乎方便 template 的使用
#include <iostream>
#include <iterator>
#include <list>
#include <typeinfo>
#include <vector>

// BidirIt 只是我们起的模板参数名，本身不会限制实参必须是双向迭代器。
// 下面的函数体实际要求：可多遍遍历、可 ++/--、可读写元素等。
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
	// 问题一：difference_type 不是 iterator_traits 定义的吗，为什么 A 没有？
	// traits 不会凭空给任意类型生成这些信息。对于普通类迭代器，C++17
	// 的 iterator_traits 会在所需的五个成员类型齐全时，转发该类的声明：
	//   using difference_type = typename It::difference_type;
	// 另外四个是 value_type、pointer、reference、iterator_category。
	// A 是空类，不满足条件，所以 iterator_traits<A> 没有这些成员类型。
	// 报错来自 my_reverse() 请求 iterator_traits<A>::difference_type。
	// C++20 增加了部分推导规则，但这里的空类 A 仍然不满足条件。
	//
	// int* 本身也没有成员类型，但标准库专门提供了指针的 traits 特化，
	// 其中 difference_type 是 std::ptrdiff_t，value_type 对 int* 是 int。
	// 这就是上面的数组指针能用的原因。给 A 补类型别名也不代表它就能反转：
	// 后续的解引用、递增、递减、比较、写入操作同样需要由 A 支持。

	// std::istreambuf_iterator<char> i1(std::cin), i2;
	// my_reverse(i1, i2); // compilation error: i1, i2 are input iterators

	// 报错为 error: no match for ‘operator--’ (operand type is ‘std::istreambuf_iterator<char, std::char_traits<char> >’)
	// 问题二：为什么 istreambuf_iterator 没有 operator--？
	// 这次不是 traits 缺少类型信息：这个迭代器有 difference_type、
	// value_type 等成员，iterator_category 是 std::input_iterator_tag。
	// 出错的是 my_reverse() 中的 --last；last 的类型是迭代器本身，
	// 不是 iterator_traits<迭代器>，所以编译器是在寻找迭代器的 operator--。
	// istreambuf_iterator 用于向前读取流，提供 ++，没有提供 --。
	// traits 只描述已有能力，不会替原类型添加 operator--。
	// 它也不能通过 *it 写回字符，而且是单遍迭代器：std::distance()
	// 遍历它的副本仍会消耗底层输入流。因此整个反转算法都不适用于它。
	// 报错里的 std::char_traits<char> 是 istreambuf_iterator 的默认第二
	// 模板参数，描述字符操作，与缺少 operator-- 没有因果关系。
}
