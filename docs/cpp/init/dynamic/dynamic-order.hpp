#ifndef VN_CPP_INIT_DYNAMIC_ORDER_HPP
#define VN_CPP_INIT_DYNAMIC_ORDER_HPP

#include <cstdio>

// 本例单线程、不使用 modules；每次真正执行动态初始化时分配一个序号。
// 函数内 static 的初始化规则独立于非局部变量，不存在计数器的启动顺序问题。
inline int trace_init(const char *kind, int id)
{
	static int sequence = 0;
	const int current = ++sequence;
	std::printf("%2d: %s %d\n", current, kind, id);
	return current;
}

// Partially-ordered：这个头文件在各翻译单元中的定义顺序都是 first → second。
// 因而本例保证 partial_first 的动态初始化先于 partial_second。
inline int partial_first = trace_init("partial inline", 1);
inline int partial_second = trace_init("partial inline", 2);

// TODO 我靠，这是什么 template 写法?
// Unordered：变量模板实例。即使写了 inline，也不会变成 partially-ordered。
template<int N>
inline int variable = trace_init("unordered variable", N);

// Unordered：类模板静态数据成员的实例。这里没有显式特化。
template<int N>
struct Box {
	static int value;
};

template<int N>
int Box<N>::value = trace_init("unordered member", N);

struct Order {
	int first;
	int second;
};

extern int ordered_a_first;
extern int ordered_a_second;
extern int ordered_b_first;
extern int ordered_b_second;

Order variables_a();
Order variables_b();
Order members_a();
Order members_b();

#endif
