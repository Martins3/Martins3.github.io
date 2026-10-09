// C++11 引入了 nullptr 关键字，专门用来区分空指针、0 。而 nullptr 的类型为 nullptr_t，
// 能够隐式的转换为任何指针或成员指针的类型，也能和他们进行相等或者不等的比较。
// 不谈各种细节问题，例如 template 或者函数重载容易区分，仅仅作为一个合格的程序，
// 是什么就写什么，不要引入模糊的东西
//
int main(int argc, char *argv[])
{
	char *m = nullptr; // 隐式转换
	// C++ 不允许直接将 void * 隐式转换到其他类型
	// int * g = (void *)0;
	return 0;
}
