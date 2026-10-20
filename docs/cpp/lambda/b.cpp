#include <functional>
#include <iostream>

std::function<int()> counter()
{
	int count = 0;
	return [count]() mutable { return ++count; };
}

int main()
{
	auto c1 = counter();
	auto c2 = counter();
	std::cout << c1() << c1() << c1() << std::endl; // 123
	std::cout << c2(); // 1
}
