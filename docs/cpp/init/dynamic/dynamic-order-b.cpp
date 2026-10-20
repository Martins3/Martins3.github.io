#include "dynamic-order.hpp"

// Ordered：本翻译单元内保证 first → second；不保证与 a.cpp 谁先谁后。
int ordered_b_first = trace_init("ordered b.cpp", 1);
int ordered_b_second = trace_init("ordered b.cpp", 2);

// 与 a.cpp 相反，按 1、2 的顺序引用相同的模板实例。
Order variables_b()
{
	return {variable<1>, variable<2>};
}

Order members_b()
{
	return {Box<1>::value, Box<2>::value};
}
