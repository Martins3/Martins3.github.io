#include <exception>
#include <stdexcept>
#include <iostream>
#include <cassert>
int compare(int a, int b)
{
	if (a < 0 || b < 0) {
		throw std::invalid_argument("received negative value");
	}
	throw std::logic_error("logic error");
}

// 添加 noexcept 之后，不可以 throw
int compare_noexcept(int *a) noexcept
{
	return *a;
}

int compare_call(int a, int b) noexcept
{
	return compare(a, b);
}

int compare_catch(int a, int b)
{
	try {
		return compare(a, b);
	} catch (const std::invalid_argument &e) {
		std::cout << e.what() << std::endl;
	} catch (const std::exception &e) {
		std::cout << e.what() << std::endl;
	} catch (...) {
		std::cout << "unknown" << std::endl;
	}
	return -1;
}

int main(int argc, char *argv[])
{
	compare_catch(-1, 3);
	compare_catch(1, 2);

	try {
		compare_noexcept((int *)12);
	} catch (...) {
		std::cout << "catch" << std::endl;
	}
	return 0;
}
