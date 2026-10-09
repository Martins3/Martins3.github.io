#include <bits/stdc++.h>
#include <type_traits>
#include <source_location>

template <class T> const char *vc()
{
	if constexpr (std::is_lvalue_reference_v<T>)
		return "lvalue";
	if constexpr (std::is_rvalue_reference_v<T>)
		return "xvalue";
	return "prvalue";
}

#define VC(expr) vc<decltype((expr))>()

class A {};

static inline void
log(std::source_location loc = std::source_location::current())
{
	std::cout << loc.function_name() << " ";
}
// 如果已经定义了 reference 版本，这个可以优先匹配上这个
void logged_process(A &s)
{
	log();
	std::cout << "T &" << std::endl;
}

void logged_process(A &&s)
{
	log();
	std::cout << "T &&" << std::endl;
}

// 可以定义出来，但是存在调用语句的时候就会有
// void logged_process(A s)
// {
// 	log();
// 	std::cout << "T" << std::endl;
// }

using namespace std;
int main(int argc, char *argv[])
{
	int a = 123;
	const int &b = std::move(a);
	// 这样是不可以的
	// int &b = std::move(a);
	// TODO 除去了函数的参数传递，类似这种的表达式有什么意义
	// 既然 c 还是 lvalue ，那么 c 和 d 有什么区别?
	int &&c = std::move(a);
	int d = std::move(a);

	// TODO 怎么感觉还是非常难以捕获这个东西
	// 只要捕获了，那么就变成了 lvalue 了

	std::cout << VC(b) << std::endl;
	std::cout << VC(c) << std::endl;
	std::cout << VC(std::move(a)) << std::endl;
	std::cout << VC(123) << std::endl;
	std::cout << VC(std::move(123)) << std::endl;

	std::cout << "object : " << std::endl;

	// 不知道为什么逐渐形成了一个印象，就是 prvalue 都是 43 这种数值
	// 但是其实 object 也完全是一样的
	A foo;
	std::cout << VC(foo) << std::endl;
	std::cout << VC(A()) << std::endl;
	std::cout << VC(std::move(A())) << std::endl;

	// 参数传递
	// 显然，参数传递也都是一样的传递规则
	// 不过函数有重载的问题
	std::cout << "func: " << std::endl;
	logged_process(foo);
	logged_process(A());
	logged_process(std::move(A()));

	return 0;
}
