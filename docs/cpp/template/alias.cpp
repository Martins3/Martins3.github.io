#include <iostream>

template <typename T> struct Pair {
	T first{};
	T second{};
};

// 别名模板可以定义在命名空间作用域或类作用域，不能定义在块作用域。
template <typename T> using Coord = Pair<T>; // Coord is an alias for Pair<T>

// Coord<T> 就是 Pair<T>，传给 print() 时仍可推导 T。
template <typename T> void print(const Pair<T> &c)
{
	std::cout << c.first << ' ' << c.second << '\n';
}

int main()
{
	// 简单来说，其实就是可以用 Coord 来代替 Pair
	Coord<int> p1{ 1, 2 }; // 显式指定别名模板的实参。
	Pair p2{ 1, 2 };
	// C++20 聚合 CTAD：推导出 Pair<int>，这里没有使用别名模板推导。

	print(p1);
	print(p2);

	return 0;
}
