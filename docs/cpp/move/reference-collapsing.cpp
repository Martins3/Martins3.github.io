// 引用折叠是类型组合规则，不是 forwarding reference 专属的规则。
// 不能直接声明 int& &，但模板替换、类型别名可以间接组合出两层引用。
// 折叠规则：& + & -> &，& + && -> &，&& + & -> &，&& + && -> &&。
// 即：只要有 & 就得到 &，只有两层都是 && 才得到 &&。
//
//
// 但通过模板参数、类型别名等组合类型时，可能间接出现这种情况。
// C++ 会把两层引用合并成一层，这就是引用折叠：
//
//  &  + &  → &
//  &  + && → &
//  && + &  → &
//  && + && → &&
//
//  记忆方法是：只要有一层是左值引用 &，结果就是 &；只有两层都是 &&，结果才是 &&。
//
//  TODO 那么，问题是，为什么这么设计?


#include <cmath>
#include <iostream>
#include <type_traits>
#include <utility>
#include <source_location>

static inline void
log(std::source_location loc = std::source_location::current())
{
	std::cout << loc.function_name() << "\n";
}

void alias_collapsing()
{
	using L = int &;
	using R = int &&;
	static_assert(std::is_same_v<L &, int &>);
	static_assert(std::is_same_v<L &&, int &>);
	static_assert(std::is_same_v<R &, int &>);
	static_assert(std::is_same_v<R &&, int &&>);
	std::cout << "类型别名：四种引用折叠规则均通过 static_assert\n";
}

void inspect(int &)
{
	std::cout << "int&：lvalue\n";
}

void inspect(int &&)
{
	std::cout << "int&&：rvalue\n";
}

// T 是此函数模板调用时推导的、没有 const 修饰的类型参数，T&& 是 forwarding reference。
// 分清两个步骤：
// 1. 特殊推导规则：传入 int 左值时，T 推导为 int&。
// 2. 类型替换和折叠：T&& 变成 int& &&，然后折叠成 int&。
// 传入 int 右值时，T 推导为 int，参数直接成为 int&&。
template <typename T> void logged_process(T &&s)
{
	log();

	if constexpr (std::is_lvalue_reference_v<T>) {
		static_assert(std::is_same_v<T, int &>);
		static_assert(std::is_same_v<decltype(s), int &>);
		std::cout << "T = int&，参数类型 = int&\n";
	} else {
		static_assert(std::is_same_v<T, int>);
		static_assert(std::is_same_v<decltype(s), int &&>);
		std::cout << "T = int，参数类型 = int&&\n";
	}

	// decltype(s) 得到参数声明的类型；decltype((s)) 检查表达式的值类别。
	// 无论参数声明是 int& 还是 int&&，有名字的表达式 s 都是左值。
	static_assert(std::is_same_v<decltype((s)), int &>);
	std::cout << "直接使用 s：";
	inspect(s);

	// std::forward<T> 利用 T 保存的信息恢复实参的值类别。
	// T = int&：返回 int&；T = int：返回 int&&。
	std::cout << "使用 std::forward<T>(s)：";
	inspect(std::forward<T>(s));
}

// 这里没有待推导的 T，是普通右值引用，只能接收 rvalue。
// 这里的 rvalue 就是定义中的，包括 prvalue + xvalue
void ordinary_rvalue_reference(int &&s)
{
	std::cout << "普通 int&& 参数进入函数后，s 仍是左值：";
	inspect(s);
}

// 只能接受 lvalue ，xvalue 和 prvalue 都不可以接受
// 内置 `&` 的操作数必须是 lvalue
void ordinary_reference(int &s)
{
	inspect(s);
}

// 这个可以接受任何形式的，lvalue prvalue 和 xvalue
// void ordinary_reference(int s) {  }
// 这个和 cpp/move/forwarding-reference-1.cpp 中定义的结果非常类似
// 但是那个例子更加复杂，因为 template 做来类型推导，让
//
// template <typename T> void logged_process(T &&s)
// 可以变换为如下两种情况:
// void logged_process<Dog>(Dog && s)
// void logged_process<Dog &>(Dog & s)

int main()
{
	alias_collapsing();
	int value = 42;

	std::cout << "\n传入左值 value：\n";
	logged_process(value);
	std::cout << "\n传入 xvalue std::move(value)：\n";
	logged_process(std::move(value));
	std::cout << "\n传入 prvalue 42：\n";
	logged_process(42);

	std::cout << "\n普通右值引用：\n";
	// TODO 忽然发现接受
	ordinary_rvalue_reference(std::move(value));
	ordinary_rvalue_reference(12);
	// ordinary_rvalue_reference(value); // 编译错误：int&& 不能绑定到左值。
	// logged_process<int>(value); // 编译错误：显式指定 T = int 后，参数是 int&&。
	// logged_process<int&>(value); // 合法：显式指定 T = int&，仍会发生引用折叠。

	ordinary_reference(value);
	// ordinary_reference(12);
	// ordinary_reference(std::move(value));
}
