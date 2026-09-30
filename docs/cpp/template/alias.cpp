#include <iostream>

template <typename T> struct Pair {
	T first{};
	T second{};
};

// Here's our alias template
// Alias templates must be defined in global scope
template <typename T> using Coord = Pair<T>; // Coord is an alias for Pair<T>

// Our print function template needs to know that Coord's template parameter T is a type template parameter
template <typename T> void print(const Pair<T> &c)
{
	std::cout << c.first << ' ' << c.second << '\n';
}

int main()
{
	// 简单来说，其实就是可以用 Coord 来代替 Pair
	Coord<int> p1{
		1, 2
	}; // Pre C++-20: We must explicitly specify all type template argument
	Pair p2{
		1, 2
	}; // In C++20, we can use alias template deduction to deduce
	   // the template arguments in cases where CTAD works

	std::cout << p1.first << ' ' << p1.second << '\n';
	print(p2);

	return 0;
}
