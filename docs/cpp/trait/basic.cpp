#include <iostream>

struct Temperature {
	double celsius;
};

// 主模板：声明有这样一套接口，本例不提供默认实现。
template <typename T> struct value_traits;

// int 的说明书：数值类型是 int，读取数值就是返回它自己。
template <> struct value_traits<int> {
	using value_type = int;
	static constexpr const char *name = "integer";

	static value_type get(const int &value)
	{
		return value;
	}
};

// Temperature 的说明书：数值类型是 double，读取 celsius 成员。
template <> struct value_traits<Temperature> {
	using value_type = double;
	static constexpr const char *name = "temperature (C)";

	static value_type get(const Temperature &value)
	{
		return value.celsius;
	}
};

// 只使用 traits 提供的统一接口，不需要知道对象内部如何存储数值。
template <typename T> void print_value(const T &object)
{
	using traits = value_traits<T>;
	typename traits::value_type value = traits::get(object);
	std::cout << traits::name << ": " << value << '\n';
}

int main()
{
	const int count = 42;
	const Temperature room{ 23.5 };

	// 先直接访问 traits，观察它提供的信息和操作。
	value_traits<int>::value_type number = value_traits<int>::get(count);
	value_traits<Temperature>::value_type degrees =
		value_traits<Temperature>::get(room);
	std::cout << "direct: " << number << ", " << degrees << '\n';

	// 再让模板函数根据参数类型选择对应的 traits。
	print_value(count);
	print_value(room);
}
