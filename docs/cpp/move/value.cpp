// # cpp lvalue rvalue
// <!-- a9fbcfcd-8db3-4abd-96d7-be3dc4a943f0 -->
//
// 1.  `lvalue` `rvalue` `lvalue reference` `rvalue reference`
// 	https://en.cppreference.com/w/cpp/language/value_category
// 2. ref 和 move
//
// 左值和右值描述的是表达式，不是对象
//
// 可以先用这个不完全严格、但很好用的模型：
// - 左值：代表一个仍有稳定身份、通常还会继续使用的对象。
// - 右值：临时值，或者明确表示即将被放弃的对象。
//
// 现代 C++ 更准确的分类是：
//
//           expression
//          /          \
//     glvalue          rvalue
//     /      \         /     \
// lvalue      \       /     prvalue
//              \     /
//               xvalue
//
//
// 注意这不是一棵树：xvalue 有两个父节点，既是 glvalue 又是 rvalue。
// 本体是一张二维表：
//
//                 有身份        无身份
//   不可移动      lvalue        （不存在）
//   可移动        xvalue        prvalue
//
// glvalue = “有身份”一列 = lvalue + xvalue
// rvalue  = “可移动”一行 = xvalue + prvalue
// xvalue  = 行列交点，唯一同时属于两边的类别
//
// 这里：
//
// - lvalue：普通的、有身份的对象表达式。
// - prvalue：用于产生一个值，例如 std::string("hello")。
// - xvalue：有身份，但资源允许被复用，例如 std::move(s)。
// - glvalue：lvalue 和 xvalue 的合称。
// - rvalue：prvalue 和 xvalue 的合称。
//
// 重要的是，同一个对象可以通过不同类别的表达式访问：
//
// std::string s = "hello";
//
// s;            // 访问同一个对象，表达式是左值
// std::move(s); // 仍是同一个对象，但表达式是右值
//
// std::move(s) 没有创建新对象，也没有立即修改 s。
//
// 为什么可以这样分类：把“有身份 identity”和“可被移动 movable”
// 看成两个正交的属性，2 x 2 = 4 种组合里，“无身份且不可移动”这
// 一格不存在——一个连身份都没有的值，没有任何表达式能再次访问
// 它指向的对象，它马上就要销毁，因此资源总是可以安全地搬走。
// 于是叶子类别恰好是三个：lvalue / xvalue / prvalue。
//
// 本文件用 decltype((expr)) 在编译期把值类别探测出来：
//   表达式是 lvalue  -> decltype((expr)) 是 T&
//   表达式是 xvalue  -> decltype((expr)) 是 T&&
//   表达式是 prvalue -> decltype((expr)) 是 T
//
// 双括号必不可少：decltype(x) 求的是变量 x 的声明类型，
// decltype((x)) 才是把 (x) 当成一个表达式，按值类别给出编码。
// 另外 decltype 的操作数是 unevaluated context，所以
// SHOW(std::move(s)) 里的 std::move(s) 并不会真的执行。

#include <stdio.h>
#include <string>
#include <utility>

// TODO 才注意到这个例子就是我一直无法理解的东西
template <typename T> struct Category {
	static constexpr const char *name = "prvalue";
	static constexpr bool identity = false;
	static constexpr bool movable = true;
};

template <typename T> struct Category<T &> {
	static constexpr const char *name = "lvalue";
	static constexpr bool identity = true;
	static constexpr bool movable = false;
};

template <typename T> struct Category<T &&> {
	static constexpr const char *name = "xvalue";
	static constexpr bool identity = true;
	static constexpr bool movable = true;
};

#define SHOW(expr)                                                           \
	printf("%-38s => %-8s identity=%d movable=%d\n", #expr,                 \
	       Category<decltype((expr))>::name,                                \
	       Category<decltype((expr))>::identity,                            \
	       Category<decltype((expr))>::movable)

// 按值返回：调用表达式是 prvalue
std::string make();
// 按右值引用返回：调用表达式是 xvalue
int &&expiring();

void bind(std::string &) { puts("bind(std::string&)"); }
void bind(std::string &&) { puts("bind(std::string&&)"); }
void bind(const std::string &) { puts("bind(const std::string&)"); }

void take(std::string &&s)
{
	// 形参 s 的声明类型是 std::string&&，但表达式 s 有名字，是 lvalue
	SHOW(s);
	bind(s); // -> bind(std::string&)
	bind(std::move(s)); // -> bind(std::string&&)
}

int main()
{
	int i = 0;
	int *p = &i;
	std::string s = "hello";
	const std::string cs = "const";

	printf("== leaf categories ==\n");
	SHOW(i); // 变量名
	SHOW(*p); // 解引用
	SHOW(++i); // 前置自增返回引用
	SHOW(s);
	SHOW(s[0]); // operator[] 返回 char&
	SHOW("hello"); // 字符串字面量是 lvalue，能取地址
	SHOW(i++); // 后置自增返回临时值
	SHOW(i + 1);
	SHOW(42);
	SHOW(std::string("temporary"));
	SHOW(make());
	SHOW(std::move(s));
	SHOW(static_cast<std::string &&>(s));
	SHOW(expiring());

	printf("\n== xvalue denotes the same object as the lvalue ==\n");
	printf("s.data()             = %p\n", (void *)s.data());
	printf("std::move(s).data()  = %p\n", (void *)std::move(s).data());
	// 成员访问是合法的，而且两个指针相同：std::move(s) 和 s 访问的是
	// 同一个对象，std::move 没有创建任何新东西。
	//
	// 注意两个编译错误：
	//   &std::string("temporary");              // prvalue 没有地址
	//   &std::move(s);                          // xvalue 也没有！
	// 内置 & 的操作数要求是 lvalue（[expr.unary.op]: "The operand of
	// the unary & operator shall be an lvalue"）。xvalue 虽然是 glvalue
	// （有身份），但不是 lvalue。“有身份”体现在能通过成员访问等方式
	// 确认它和 s 指同一个对象，而不是内置 & 能直接作用于它。

	printf("\n== a named rvalue reference is an lvalue ==\n");
	int &&r = 42; // r 的声明类型是 int&&，但表达式 r 是 lvalue
	SHOW(r);
	printf("r = %d\n", r);
	take(std::string("source"));
	// take(s); // 编译错误：右值引用形参不能绑定 lvalue

	printf("\n== overload resolution ==\n");
	std::string t;
	bind(t); // lvalue        -> std::string&
	bind(std::move(t)); // xvalue        -> std::string&&
	bind(std::string("tmp")); // prvalue       -> std::string&&
	bind(cs); // const lvalue  -> const std::string&
	bind(std::move(cs));   // const xvalue  -> const std::string&
	// std::move(cs) 的类型是 const std::string&&，不是 std::string&&，
	// 右值引用重载不可行，最终落到 const 左值引用上。

	return 0;
}
