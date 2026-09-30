#include <string>
#include <iostream>

// 基本语法
void func_with_auto_inline(const std::convertible_to<std::string> auto &x)
{
	std::string v = x;
}
void func_with_auto_postfix(
	const auto &x) requires std::convertible_to<decltype(x), std::string>
{
	std::string v = x;
}

template <std::convertible_to<std::string> T>
void func_with_template_inline(const T &x)
{
	std::string v = x;
}
template <typename T>
requires std::convertible_to<T, std::string> void
func_with_template_postfix(const T &x)
{
	std::string v = x;
}

void test1()
{
}

// 基本运算
// 表示参数只是接受整数，以及返回为整数的函数
template <typename T>
	requires std::integral<T> ||
	(std::invocable<T> &&
	 std::integral<
		 typename std::invoke_result<T>::type>)void function(const T &x)
{
	if constexpr (std::invocable<T>) {
		std::cout << "Result of call is " << x() << "\n";
	} else {
		std::cout << "Value is " << x << "\n";
	}
}
void test2()
{
	function(1); // OK, integral
	function([]() { return 2; }); // OK, invocable, returns integral
	/* function(2.0); // Fails */
}

template <typename T>
concept maybe_invokable_integral_v1 =
	std::is_integral<T>::value ||
	(std::is_invocable<T>::value &&
	 std::is_integral<typename std::invoke_result<T>::type>::value);
template <typename T>
concept maybe_invokable_integral_v2 =
	std::integral<T> ||
	(std::invocable<T> &&
	 std::integral<typename std::invoke_result<T>::type>);

// requires 的含义，就是只要可以执行这里的函数，那么就可以了
template <typename T> concept addable = requires(T a, T b)
{
	a + b;
};

template <class T> concept what = requires
{
	new int[(int)sizeof(T)];
	// invalid for every T: ill-formed, no diagnostic required
};

void function3(maybe_invokable_integral_v1 auto x)
{
	std::cout << x << std::endl;
}

void function3_1(addable auto x)
{
}

void function3_2(what auto x)
{
}

struct X {};

void test3()
{
	function3(1); // OK
	function3_1(1); // OK
	function3_2(1); // OK
	/* function3(X{}); // Fails */
}

int main()
{
	test1();
	test2();
	test3();
	return 0;
}
