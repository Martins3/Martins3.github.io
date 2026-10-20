#include <iostream>
int main(int argc, char *argv[])
{
	std::cout << std::is_integral_v<int> << std::endl;
	static_assert(std::is_integral<int>::value);
	static_assert(std::is_same<int, int>::value);
	static_assert(std::is_same<std::conditional<true, int, double>::type,
				   int>::value);
	return 0;
}
