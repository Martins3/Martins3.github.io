// # cpp decltype 用法总结
// <!-- 56d5d01a-37a4-4a7d-ba4c-a8f3af72ed5a -->
//
// decltype(expr) 在编译期推导 expr 的类型，但不会真正求值 expr。
// 与 auto 不同，decltype 会完整保留表达式的类型，包括引用和 const。
//
// decltype 的推导规则可以分成两类：
//
// 1. "特殊规则"：decltype(名字) 得到该名字的"声明类型"，不额外加引用
//    - decltype(x)       // 无括号的变量名 -> int
//    - decltype(obj.m)   // 无括号的成员访问 -> 成员的声明类型
//
// 2. "普通规则"：decltype(其他表达式) 按表达式的值类别推导
//    - 纯右值 prvalue -> T
//    - 左值   lvalue  -> T&
//    - 将亡值 xvalue  -> T&&
//
// 判断值类别的一个简单方法：
//   - 有名字的对象是左值；
//   - 字面量、算术运算结果是纯右值；
//   - std::move 的结果是将亡值。
//
// 最容易踩的坑：给名字加一对括号 (x) 之后，它就从"名字"变成了"左值表达式"，
// 于是 decltype((x)) 得到引用类型 T&，而不是 T。
//
// decltype(auto)（C++14）用在返回值时，按 decltype 规则推导返回类型，
// 从而完整保留返回表达式的类型；而 auto 会把引用和顶层 const 剥掉。

#include <cassert>
#include <iostream>
#include <type_traits>
#include <utility>

struct Widget {
	double value;
};

// ---- 常见用途 1：尾置返回类型 ----
// 模板参数之间做运算，返回类型在参数之后才能确定。
template <typename T, typename U> auto add(T t, U u) -> decltype(t + u)
{
	return t + u;
}

template <typename T, typename U> auto add_auto(T t, U u)
{
	return t + u;
}

// ---- 常见用途 2：decltype(auto) 完美转发返回类型 ----
// auto 会丢掉引用，导致按值返回（发生拷贝）；decltype(auto) 保留引用。
int &getRef(int &i)
{
	return i;
}

auto copyReturn(int &i) // 返回类型是 int，引用被剥掉
{
	return getRef(i);
}

decltype(auto) refReturn(int &i) // 返回类型是 int&，引用被保留
{
	return getRef(i);
}

int main()
{
	// ================= 规则 1：decltype(名字) -> 声明类型 =================
	int x = 0;
	int &rx = x;
	const int cx = 0;

	static_assert(std::is_same_v<decltype(x), int>);
	static_assert(
		std::is_same_v<decltype(rx), int &>); // rx 的声明类型就是 int&
	static_assert(std::is_same_v<decltype(cx), const int>);

	// ================= 规则 2：值类别决定是否加引用 =================
	// 括号让 (x) 从"名字"变成"左值表达式"，于是得到引用
	static_assert(std::is_same_v<decltype((x)), int &>);
	static_assert(std::is_same_v<decltype((cx)), const int &>);

	// 纯右值 -> T
	static_assert(std::is_same_v<decltype(42), int>);
	static_assert(std::is_same_v<decltype(x + 1), int>);

	// 左值 -> T&
	static_assert(std::is_same_v<decltype(std::declval<int &>()), int &>);

	// 将亡值 -> T&&
	static_assert(std::is_same_v<decltype(std::move(x)), int &&>);

	// 成员访问：无括号 -> 声明类型；有括号 -> 左值引用
	Widget w{ 3.14 };
	static_assert(std::is_same_v<decltype(w.value), double>);
	static_assert(std::is_same_v<decltype((w.value)), double &>);

	// ================= decltype 不求值表达式 =================
	// 即使函数从未被定义，decltype 也能工作，因为它只在编译期取类型。
	extern int evil(); // 只有声明，没有定义
	static_assert(std::is_same_v<decltype(evil()), int>);
	std::cout
		<< "decltype 不会真的调用 evil()，因此链接时也不需要它的定义\n";

	// ================= 尾置返回类型 =================
	auto sum = add(1, 2.5); // t+u 是 double，返回 double
	static_assert(std::is_same_v<decltype(sum), double>);
	std::cout << "add(1, 2.5) = " << sum << '\n';

	auto a_a = add_auto(1, 2.5); // t+u 是 double，返回 double

	// ================= decltype(auto) 与 auto 的区别 =================
	int val = 10;
	// copyReturn 按值返回，修改副本不影响原变量
	int copy = copyReturn(val);
	copy = 999;
	assert(copy == 999); // 副本被改了
	assert(val == 10); // 原变量不受影响

	// refReturn 按引用返回，修改它等于修改原变量
	int &ref = refReturn(val);
	ref = 999;
	assert(val == 999);
	std::cout << "val 经过引用修改后是 " << val << '\n';

	// ================= lambda 的类型是唯一的 =================
	// 每个 lambda 表达式都有自己独有的无名类型，decltype 可以拿到它。
	auto a = [](int n) { return n + 1; };
	static_assert(!std::is_same_v<decltype(a), int>);
	decltype(a) b = a; // 用 decltype 复制 lambda 的类型
	std::cout << "a(1) = " << a(1) << ", b(1) = " << b(1) << '\n';

	// ================= decltype 函数名得到函数类型 =================
	static_assert(std::is_same_v<decltype(getRef), int &(int &)>);

	return 0;
}
