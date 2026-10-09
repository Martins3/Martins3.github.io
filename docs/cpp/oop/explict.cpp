// - explicit 的含义:
//   - explicit 修饰构造函数时，可以防止隐式转换和copy初始化
//       - https://stackoverflow.com/questions/34912221/is-list-initialization-an-implicit-conversion
//   - explicit 修饰转换函数时，可以防止隐式转换，但按语境转换除外
//   - /home/maritns3/core/CPlusPlusThings/basic_content/explicit/explicit.cpp 的例子
//
// explicit 关键字 : only declarations of constructors and conversion operators can be ‘explicit’
//
// 什么时候，使用 implict 是好的做法

#include <iostream>

class A {
    public:
	// 我理解 implict 构造出现的地方
	// 1. 需要有构造函数和参数匹配
	// 2. 做类型转换
	A(int a)
	{
		std::cout << "a" << std::endl;
	};

	// TODO 这个东西用于 implict 的构造吗?
	// 当传递两个参数的时候
	A(int a, int b)
	{
		std::cout << "a b" << std::endl;
	};
};
int main(int argc, char *argv[])
{
	A a = 12;
	// 这个没有什么意义
	// A b = (1, 2);
	return 0;
}
