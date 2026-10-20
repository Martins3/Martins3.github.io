#include "dynamic-order.hpp"

// Ordered：本翻译单元内保证 first → second。
int ordered_a_first = trace_init("ordered a.cpp", 1);
int ordered_a_second = trace_init("ordered a.cpp", 2);

// 故意按 2、1 的顺序引用模板实例；这不构成动态初始化顺序的保证。
Order variables_a()
{
	return {variable<2>, variable<1>};
}

Order members_a()
{
	return {Box<2>::value, Box<1>::value};
}
