#include <iostream>

// - `0` 的类型是 `int`，与 `T` 完全无关。
// - 所以 `f(0)` 是一个 Non-dependent Call。
// - 编译器在定义 `foo` 时，就完成了 `f` 的 Name Lookup。
// - 后来定义的 `f(double)` 不会进入这次查找的结果。
//
// 即使 `foo<double>` 是后来才实例化的，也不会重新查找 `f`。
// 这正是 Phase 1 的意义：模板中那些与模板参数无关的名称，
// 不会因为实例化位置不同而随意改变含义。

void f(int)
{
	std::cout << "f(int)\n";
}

template <typename T> void foo(T x)
{
	f(0);
}

void f(double)
{
	std::cout << "f(double)\n";
}

int main()
{
	foo(3.14);
}
