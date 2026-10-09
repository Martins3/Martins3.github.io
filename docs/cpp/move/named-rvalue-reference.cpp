// 右值引用变量的声明类型是 T&&，但通过变量名使用它时，表达式是 lvalue。
// 引用类型与表达式的值类别是两个概念，不能看到 T&& 就认定表达式是右值。
// 这里用带日志的类展示构造函数选择；std::string 也遵循相同的规则。
//
// 为什么这样设计?
//
// • 因为 “这个引用能绑定右值” 和 “现在允许移动这个对象” 是两件不同的事。
//
//  T&& 参数允许函数接收临时对象，或调用者显式交出的对象。但进入函数后，参数可能被使用多次，函数需要决定什么时候移动它：
//
//  void f(std::string&& s) {
//      // 先保存一份副本，后续还要使用 s。
//      std::string backup = s;
//
//      // 检查原始内容。
//      std::cout << s << '\n';
//
//      // 最后才把资源交出去。
//      std::string result = std::move(s);
//  }
//
//  如果具名的右值引用表达式自动被当成右值，第一句就可能移动 s，后面的代码看到的便是移动后的状态。这样，每一次普通的变量访问
//  都可能消耗对象，代码会很难推理。
//
//  因此 C++ 用表达式的写法区分意图：
//
//  • s：普通地访问这个对象，表达式是 lvalue。
//  • std::move(s)：把这次访问标记为可供移动，表达式是 xvalue。
//
//  还有一个关键点：右值引用并不保证对象没有其他使用者。
//
//  std::string text = "hello";
//  f(std::move(text));
//
//  // text 仍然存在；如果 f 移动了它，这里访问的是移动后的对象。
//
//  std::move(text) 不会结束 text 的生命周期，也不会赋予 f 独占所有权。因此，T&& 不能被理解为“这个对象以后没人用了”。
//
//  可以把这条设计规则记成：右值引用让函数有机会移动；具名变量仍是 lvalue，让函数能控制具体在哪一次使用时移动。

#include <iostream>
#include <type_traits>
#include <utility>

struct Trace {
	Trace() = default;

	Trace(const Trace &)
	{
		std::cout << "copy constructor\n";
	}

	Trace(Trace &&) noexcept
	{
		std::cout << "move constructor\n";
	}
};

void inspect(const Trace &)
{
	std::cout << "lvalue overload: const Trace&\n";
}

void inspect(Trace &&)
{
	std::cout << "rvalue overload: Trace&&\n";
}

void f(Trace &&s)
{
	// decltype(s) 对未加括号的变量名采用特殊规则，得到变量的声明类型。
	static_assert(std::is_same_v<decltype(s), Trace &&>);

	// decltype((s)) 按表达式值类别推导：lvalue 得到 T&，xvalue 得到 T&&。
	// 因此这两个断言直接验证 s 是 lvalue，std::move(s) 是 xvalue。
	static_assert(std::is_same_v<decltype((s)), Trace &>);
	static_assert(std::is_same_v<decltype(std::move(s)), Trace &&>);

	// s 是 lvalue，不能绑定到 inspect(Trace&&)，于是选择 const Trace& 重载。
	std::cout << "inspect(s): ";
	inspect(s);

	// std::move 本身只是转换值类别，不调用移动构造函数，也不搬走资源。
	// 此处只是把引用传给另一个重载，输出中不会出现 move constructor。
	std::cout << "inspect(std::move(s)): ";
	inspect(std::move(s));

	// 虽然 s 的声明类型是 Trace&&，表达式 s 仍是 lvalue，因此这里拷贝构造。
	std::cout << "Trace t = s: ";
	[[maybe_unused]] Trace t = s;

	// std::move(s) 是 xvalue，可以绑定到移动构造函数的 Trace&& 参数。
	// 真正调用移动构造函数的是初始化 u，而不是 std::move 本身。
	std::cout << "Trace u = std::move(s): ";
	[[maybe_unused]] Trace u = std::move(s);
}

int main()
{
	Trace original;

	// std::move(original) 使实参能绑定到 f 的右值引用参数；绑定引用不构造对象。
	// 进入 f 后，通过参数名字 s 使用该对象时，表达式又是 lvalue。
	f(std::move(original));
}
