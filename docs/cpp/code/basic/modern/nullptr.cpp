
// C++11 引入了 nullptr 关键字，专门用来区分空指针、0 。而 nullptr 的类型为 nullptr_t，
// 能够隐式的转换为任何指针或成员指针的类型，也能和他们进行相等或者不等的比较。
// `#include<cstddef>`
// C++ 不允许直接将 void * 隐式转换到其他类型
#include <iostream>
int main(int argc, char *argv[])
{
	char *m = nullptr; // 隐式转换
	/* int * g = (void *)0; */ // 这个错误，但是 c 中可以
	return 0;
}
