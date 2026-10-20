#include "counter.hpp"
#include <iostream>

namespace
{
int count = 100; // 与 counter.cpp 中的 count 是不同实体。
int increment()
{
	return ++count;
}
}

namespace tutorial
{
// 不可以重新定义，即便是在不同的文件中，这很自然
// int next()
// {
// 	return 1;
// }
}

int main()
{
	std::cout << "tutorial::next: " << tutorial::next() << '\n';
	std::cout << "tutorial::next: " << tutorial::next() << '\n';
	std::cout << "main translation unit count: " << count << '\n';
	std::cout << increment()  << std::endl;

}
