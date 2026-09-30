#include <iostream>

constexpr int product(int x, int y)
{
	return (x * y);
}

int main(int argc, char *argv[])
{
	int x = product(10, 20);
	// 不可以，因为 product 需要其是可以静态求解的
	/* std::cout << product(arc, 20); */
	std::cout << x;
	return 0;
}
