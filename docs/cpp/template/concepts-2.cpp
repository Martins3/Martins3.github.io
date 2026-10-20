#include <concepts>
#include <iostream>

// concepts 的概念很容易，就是给 Typename 添加一个约束
template <typename T> concept Addable = requires(T a, T b)
{
	{ a + b }->std::convertible_to<T>;
};

template <typename T> concept AddableOny = requires(T a, T b)
{
	a + b;
};

template <typename T> concept Integral = std::is_integral_v<T>;

// 第一种写法:
template <class T> requires Integral<T> void f(T x)
{
}
// 第二种写法:
template <Integral T> void f(T x);
// 第三种写法
void f(Integral auto x);

// 怎么理解这个东西?
//
// 这个会报错为: return-type-requirement is not a type-constraint
// template <typename T> concept AddableInt = requires(T a, T b)
// {
// 	{a + b} -> int;
// };
template <typename T> T add(T a, T b)
{
	return a + b;
}

template <Addable T> T add(T a, T b)
{
	std::cout << "fine" << std::endl;
	return a + b;
}

// 很多时候，是可以不用先定义 concepts 的
template <class T> requires(sizeof(T) <= 8) void f(T x)
{
}

// 注意，这里定义了两个 requires ，而且是必须的!
// 这就是为什么 requires 采用两个语句的原因
template <class T> requires requires(const T &x)
{
	x.size();
}
void f(const T &x)
{
}

// 两个 requires 太难受了，所以就需要定义出来 concepts 出来
template <class T> concept HasSize = requires(T x)
{
	x.size();
};

template <class T> requires HasSize<T> void f(T x)
{
}

int main(int argc, char *argv[])
{
	add(12, 1);
	add(12, 1);

	return 0;
}
