// # cpp decltype
// <!-- bbf66c06-6868-411e-936d-c8156519c4bf -->
// - decltype 关键字是为了解决 auto 关键字只能对变量进行类型推导的缺陷而出现
// https://stackoverflow.com/questions/24109737/what-are-some-uses-of-decltypeauto
//
// 尾置返回类型 (trailing return type) vs auto 返回类型推导
//
//   g++ -std=c++20 -Wall -Wextra a.cpp -o a.out && ./a.out
//
// 两个可选的失败演示 (打开宏就故意编译不过):
//   g++ -std=c++20 -DSHOW_HARD_ERROR a.cpp           # auto 推导不参与 SFINAE
//   g++ -std=c++20 -DSHOW_DEDUCED_FORWARD_DECL a.cpp # auto 推导不能只声明
#include <bits/stdc++.h>
using namespace std;

// ============================================================
// 0. 基本形式
// ============================================================

auto add(int a, int b) -> int
{
	return a + b;
}

// C++14 起也可以完全省略 -> T，编译器从 return 语句推导
auto add_deduced(int a, int b)
{
	return a + b;
}

// ============================================================
// 1. 返回类型是否参与 SFINAE
// ============================================================

struct WithFoo {
	int foo() const { return 42; }
};

struct NoFoo {
};

// (A) -> decltype(t.foo())
//     t.foo() 的无效发生在 "替换模板参数" 的 immediate context 里，
//     属于 SFINAE：这个重载被静默丢弃，而不是编译错误。
template <typename T>
auto call_foo_trailing(T t) -> decltype(t.foo())
{
	return t.foo();
}

// (B) auto 推导
//     要确定返回类型，编译器必须先实例化函数体；函数体里的错误
//     不在 immediate context，是 hard error，直接让整个编译失败。
template <typename T>
auto call_foo_deduced(T t)
{
	return t.foo();
}

// 经典 sizeof-SFINAE 探测：call_foo_trailing<T>(declval<T>()) 合法就选中
// 返回 true_type 的重载，否则退到返回 false_type 的 ... 重载。
//
// TODO 有些无法理解这个东西了
template <typename T>
static auto probe_trailing(int) -> decltype(call_foo_trailing(std::declval<T>()), std::true_type {});
template <typename T>
static std::false_type probe_trailing(...);

template <typename T>
constexpr bool has_foo_trailing()
{
	// TODO 这个句子还是无法理解
	return decltype(probe_trailing<T>(0))::value;
}

// C++20 的替代方案：同样的探测用 requires 表达式表达，直接读参数，
// 不再需要把返回类型写进 immediate context，也不需要 probe 重载。
template <typename T>
concept HasFoo = requires(T t) { t.foo(); };

#ifdef SHOW_HARD_ERROR
// 把探测底层换成 auto 推导版本。
template <typename T>
static auto probe_deduced(int) -> decltype(call_foo_deduced(std::declval<T>()), std::true_type {});
template <typename T>
static std::false_type probe_deduced(...);

static void demo_hard_error()
{
	// probe_deduced<NoFoo>(0) 要实例化 call_foo_deduced<NoFoo> 的函数体，
	// 而这个函数体是 ill-formed 的。SFINAE 救不了它。
	cout << "probe_deduced<NoFoo> = " << decltype(probe_deduced<NoFoo>(0))::value << "\n";
}
#endif

// ============================================================
// 2. 返回类型显式写出 -> 可以先声明后定义、可以相互递归
// ============================================================

auto is_even(int n) -> bool; // 只有声明就够了，返回类型此时已确定
auto is_odd(int n) -> bool { return n == 0 ? false : is_even(n - 1); }
auto is_even(int n) -> bool { return n == 0 ? true : is_odd(n - 1); }

#ifdef SHOW_DEDUCED_FORWARD_DECL
// 换成 auto 推导：声明时编译器不知道返回类型，
// is_odd 的函数体里就出现了 "use of 'is_even' before deduction of 'auto'"
auto is_even_deduced(int n);
auto is_odd_deduced(int n) { return n == 0 ? false : is_even_deduced(n - 1); }
auto is_even_deduced(int n) { return n == 0 ? true : is_odd_deduced(n - 1); }
#endif

// ============================================================
// 3. auto 推导会丢引用和 const，尾置返回类型不会
// ============================================================

auto at_deduced(vector<int> &v, size_t i) { return v.at(i); }

// 显式 -> int& 保留引用，调用方可以写回
auto at_trailing(vector<int> &v, size_t i) -> int & { return v.at(i); }

int main()
{
	cout << "[0] add(1, 2)       = " << add(1, 2) << "\n";
	cout << "[0] add_deduced(1, 2) = " << add_deduced(1, 2) << "\n";
	cout << "[0] add 的类型: " << typeid(decltype(add)).name() << "\n";

	cout << "\n[1] 同一个 \"调用 t.foo()\" 的两种写法\n";
	cout << "    has_foo_trailing<WithFoo>() = " << has_foo_trailing<WithFoo>() << "\n";
	cout << "    has_foo_trailing<NoFoo>()   = " << has_foo_trailing<NoFoo>()
	     << "   (替换失败 -> 退到 fallback，编译继续)\n";
	cout << "    HasFoo<WithFoo> = " << HasFoo<WithFoo> << " (C++20 concept 版)\n";
	cout << "    HasFoo<NoFoo>   = " << HasFoo<NoFoo> << "\n";
#ifdef SHOW_HARD_ERROR
	demo_hard_error();
#endif

	cout << "\n[2] 相互递归: is_even(10) = " << is_even(10) << ", is_odd(10) = " << is_odd(10) << "\n";

	cout << "\n[3] 引用丢失\n";
	vector<int> v {1, 2, 3};
	auto &r = at_trailing(v, 1); // 引用，能写
	r = 100;
	cout << "    at_trailing 改回后 v = " << v[0] << " " << v[1] << " " << v[2] << "\n";
	// 编译期就能看出两者的区别
	static_assert(std::is_same_v<decltype(at_deduced(v, 0)), int>);
	static_assert(std::is_same_v<decltype(at_trailing(v, 0)), int &>);

	auto copy = at_deduced(v, 0); // 拷贝，改了不影响 v
	copy = -1;
	cout << "    at_deduced 改 copy 后 v[0] = " << v[0] << " (v 没变), copy = " << copy << "\n";

	return 0;
}
