#include <cassert>
#include <iostream>
#include <type_traits>

// 编译运行：g++ -std=c++17 -Wall -Wextra -Wpedantic scoope.cpp -o scoope.out && ./scoope.out
// 注释掉的错误示例可以逐个取消注释，观察编译器如何指出作用域边界。

int global_value = 1; // 全局命名空间作用域

namespace N
{
int namespace_value = 2; // 命名空间作用域

struct A {
	int member = 3; // 类作用域

	void f(int parameter)
	{ // parameter：函数参数作用域
		int local = 4; // local：函数体的块作用域
		std::cout << "global / namespace / member / parameter / local: "
			  << global_value << " / " << namespace_value << " / "
			  << member << " / " << parameter << " / " << local << '\n';

		{
			int inner = 5; // 更内层的块作用域
			std::cout << "inner: " << inner << '\n';
		}

		for (int i = 0; i < 3; ++i) {
			// i 属于 for 引入的块作用域
			std::cout << "loop i: " << i << '\n';
		}
		// 此处不能再使用 inner 和 i
	}
};
}

// 1. Function parameter scope：参数名不属于函数外面的命名空间。
// 声明中的名字到该声明符结束就不再可用；定义可以使用另一个参数名。
int parameter_demo(int declared_name);
// int leaked_parameter = declared_name; // 错误：这里找不到 declared_name

int parameter_demo(int value)
{
	// int value = 99; // 错误：函数体最外层不允许重新声明同名参数
	{
		int value = 99; // 正确：再嵌套一层块，可以遮蔽参数
		std::cout << "inner value: " << value << '\n';
	}
	std::cout << "parameter value: " << value << '\n'; // 仍然是传入的值
	return value;
}

// 参数作用域不仅覆盖函数体：在进入 {} 之前，尾置返回类型就能使用 value。
auto echo(int value) -> decltype(value)
{
	return value;
}

// 2. Template parameter scope：T 是这份模板的参数，不会自动成为类成员。
template <class T> struct Box {
	using value_type = T; // value_type 属于 class scope；T 属于模板参数作用域
	T value;
};
// T leaked_type; // 错误：Box 模板参数的名字 T 在这里不可用

// 另一个 template 引入的是独立的 T，可以在不同实例中代表不同类型。
template <class T> T identity(T value)
{
	return value;
}

void template_parameter_demo()
{
	Box<int> box{42};
	Box<int>::value_type number = box.value; // 正确：显式声明的成员类型别名
	// Box<int>::T invalid = 0; // 错误：T 不是 Box 的成员类型
	static_assert(std::is_same_v<decltype(identity(3.5)), double>);
	assert(number == 42);
	std::cout << "Box<int> / identity<double>: " << number << " / "
		  << identity(3.5) << '\n';
}

// 3. Enumeration scope：枚举类型名和枚举项的名字，属于不同层次。
// Color、Traffic 属于全局命名空间；各自的 Red 属于各自的枚举作用域。
enum class Color { Red = 1, Green = Red + 1 }; // 内部可以直接查找到 Red
enum class Traffic { Red = 10, Yellow }; // 两个 Red 不冲突

// 普通 enum 的枚举项还会绑定到外层作用域，所以外面可以直接写 OldRed。
enum OldColor { OldRed = 20, OldGreen };

void enumeration_demo()
{
	Color color = Color::Red;
	Traffic light = Traffic::Red;
	// auto invalid = Red; // 错误：这里没有单独的 Red 名字
	OldColor old = OldRed; // 正确：普通 enum 的枚举项可以在外层直接找到
	assert(old == OldColor::OldRed); // 普通 enum 也支持枚举名限定
	static_assert(static_cast<int>(Color::Green) == 2);
	std::cout << "Color::Red / Traffic::Red / OldRed: "
		  << static_cast<int>(color) << " / " << static_cast<int>(light)
		  << " / " << old << '\n';
}

// 4. Lambda scope：初始化捕获会引入新名字；它和参数、局部变量分属三种作用域。
void lambda_demo()
{
	int base = 10; // 外层 block scope
	auto add = [base = base + 1](int x) {
		// 左侧 base：新引入的捕获名，属于 lambda scope。
		// 右侧 base + 1：查找外层 base，所以捕获值是 11。
		// x：function parameter scope；result：函数体的 block scope。
		int result = base + x;
		return result;
	};
	base = 100; // 修改外层变量，不会改变按值初始化的捕获
	int result = add(2); // 11 + 2 = 13
	assert(result == 13);
	std::cout << "outer base / add(2): " << base << " / " << result << '\n';

	auto only_inside = [secret = 42] { return secret; };
	assert(only_inside() == 42);
	// std::cout << secret; // 错误：初始化捕获的名字不会泄漏到外层
	// std::cout << add.base; // 错误：捕获名不是可通过闭包对象访问的公开成员名
	// auto bad = [base = 1](int base) { return base; }; // 错误：参数与捕获重名
	// 注意：以上针对 [name = expression] 初始化捕获。
	// 普通 [base] 捕获已有变量，没有在源码层面引入一个新名字。
}

int main()
{
	N::A{}.f(6);
	int parameter = parameter_demo(7);
	assert(parameter == 7);
	assert(echo(8) == 8);
	template_parameter_demo();
	enumeration_demo();
	lambda_demo();
}
