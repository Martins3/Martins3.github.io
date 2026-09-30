// https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
#include <bits/stdc++.h>
using namespace std;

// 这是一个精心构造的例子，如果不用 typename 来区分，这里的东西来确认
//
// typename T::foo *x;    声明指针变量 x，指向 T::foo 类型
// T::foo *x;             表达式：T::foo 乘以 x
//
// 如果不在编译的清理掉这个差别，而是让 cpp 动态确定，
// 对于代码的可读性和编译器的实现都是一个很大的负担

int x = 12;

// 如果你真的需要可以处理两种情况，可以利用两个特性:
// - if constexpr（C++17）：编译期条件分支，实例化时只实例化选中的分支。
// - requires 表达式（C++20）：检查某些类型或表达式是否有效，结果为 bool。

template <typename T> void adaptive()
{
	if constexpr (requires { typename T::foo; }) {
		typename T::foo *x = nullptr;
		// foo 是类型时的逻辑
	} else {
		auto result = T::foo * ::x;
		// foo 是值时的逻辑
	}
}

template <typename T> void value()
{
	T::foo *x;
}

template <typename T> void type()
{
	typename T::foo *x;
}

struct X {
	using foo = int;
};

struct Y {
	static int const foo = 123;
};

int main(int argc, char *argv[])
{
	type<X>();
	value<Y>();
	adaptive<X>();
	adaptive<Y>();
	return 0;
}
