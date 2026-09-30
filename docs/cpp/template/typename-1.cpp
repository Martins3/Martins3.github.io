// https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
// 其中的一个回答的案例已经非常清晰了，A 位置的 typename 如果用了，那么 f_tmpl<Y>(); 就不可以初始化。
//
// 首先，展示一个小的 demo 例子，对于 T::foo ，取决于 T ，
// 他可能是一个数值，也可能是类型的
#include <bits/stdc++.h>
using namespace std;

template <typename T> void show_type()
{
	typename T::foo x = 12;
	std::cout << "show_type()" << std::endl;
}

template <typename T> void show_value()
{
	std::cout << T::foo << std::endl;
	std::cout << "show_value()" << std::endl;
}
struct X {
	using foo = int; // 这里将 foo 定义为类型
};
struct Y {
	static int const foo = 123; // 这里故意 foo 为数值
};

int main(int argc, char *argv[])
{
	show_type<X>();
	show_value<Y>();

	// show_type<Y>();
	// show_value<X>();
	return 0;
}
