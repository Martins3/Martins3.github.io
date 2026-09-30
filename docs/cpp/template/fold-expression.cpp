#include <iostream>

// 如果没有折叠表达式，那么可以使用什么方法来判断
namespace Basic
{

template <typename T> void print(T value)
{
	std::cout << value << std::endl;
}
template <typename T, typename... Args> void print(T value, Args... args)
{
	std::cout << value << std::endl;
	print(args...);
}

static void test()
{
	print(2, 3, 4, 5);
}

}

// 使用来折叠表达式的效果
namespace Fold
{

template <typename... Args> void print_lines(const Args &...args)
{
	// 一元右折叠：(pack op ...)
	// 逗号运算符保证每个输出表达式从左到右执行。
	((std::cout << args << '\n'), ...);
}

template <typename... Numbers> auto sum(Numbers... numbers)
{
	// 二元左折叠：(init op ... op pack)
	// 展开后类似于：((0 + numbers1) + numbers2) + numbers3。
	return (0 + ... + numbers);
}

template <typename... Numbers> auto subtract_left(Numbers... numbers)
{
	// 一元左折叠：(... op pack)
	return (... - numbers);
}

template <typename... Numbers> auto subtract_right(Numbers... numbers)
{
	// 一元右折叠：(pack op ...)
	return (numbers - ...);
}

static void test()
{
	print_lines("不同类型也可以放进同一个参数包：", 42, 3.14, "hello");

	std::cout << "sum(1, 2, 3, 4) = " << sum(1, 2, 3, 4) << '\n';
	std::cout << "左折叠 (... - numbers) = " << subtract_left(10, 3, 2)
		  << '\n';
	std::cout << "右折叠 (numbers - ...) = " << subtract_right(10, 3, 2)
		  << '\n';
}

}

int main(int argc, char *argv[])
{
	Basic::test();
	Fold::test();

	return 0;
}
