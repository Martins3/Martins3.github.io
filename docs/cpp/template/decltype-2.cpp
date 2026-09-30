// https://stackoverflow.com/questions/18815221/what-is-decltype-and-how-is-it-used
#include <bits/stdc++.h>
// 如果制定了 decltype ，那么就必须，返回值就必须是 auto
auto add(int a, int b) -> decltype(a)
{
	return a + b;
}

auto add(int a, int b, int c) -> int
{
	return a + b + c;
}

auto sub(int a, int b, int c) -> int
{
	return a + b + c;
}

int foo()
{
	return 0;
}

int main(int argc, char *argv[])
{
	// 显然，decltype 一个函数是没有必要的
	/* decltype(add); */

	int n = 10;

	decltype(n) a = 20; // a is an "int"    [unparenthesized id-expression]
	decltype((n)) b = a; // b is an "int &"  [(n) is an lvalue]
	decltype((std::move(n))) c = std::move(a);
	decltype(foo()) d = foo(); // d is an "int"    [(foo()) is a prvalue]
	decltype(foo()) &&r1 = foo(); // int &&
	decltype((n)) &&r2 = n; // int & [& && collapses to &]
	return 0;
}
