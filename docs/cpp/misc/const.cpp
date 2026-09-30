#include <bits/stdc++.h>
using namespace std;

// c 语言中就是如此的
// 1. const 函数返回值
// 2. const 指针修饰指针
// 如果 const 位于`*`的左侧，则const就是用来修饰指针所指向的变量，
// 即指针指向为常量；如果const位于`*`的右侧，
// const 就是修饰指针本身，即指针本身是常量。
//
const int abc()
{
	int m;
	const int *const pm = &m;

	// *pm = 12;
	// pm ++;

	int const *p2 = &m;
	p2++;

	const int *p4 = &m;
	p4++;

	int *const p3 = &m;
	// p3++; // 这是不可以的

	return 123;
}

class A {
    public:
	A()
	{
	}

	void name() const
	{
		std::cout << "fafafa" << std::endl;
	}

	// 不是很难理解，这两个 const 的含义，一个修饰对象，一个修饰返回值
	const int age() const
	{
		std::cout << "fafafa" << std::endl;
		return 123;
	}

	// 构造函数两个位置都是不可以添加的
	// 1. A(int num) const { }
	// 成员函数是不可以定义为 const 的，某种意义上来说，也是合理的
	// 因为直接创建对象的时候设置为 const 就可以了
	//
	// 而且如果严格按照 const 成员语义，当前对象是不可以修改的
	//
	// 2. 由于构造函数没有返回值，所以 const 前面也是不可以访问到的
	// const A(int num) { }



	// 3. static const member 可以定义在 class 里面，但是 static member 不可以
	//   - 因为 nonconst static member 是占用内存， 在多个 compile unit 出现就是一个变量占据多个内存了
	//   - https://stackoverflow.com/questions/47882456/why-do-non-constant-static-variables-need-to-be-initialized-outside-the-class
	// 这对于 const 和 static 的故事，似乎一直都是如此
	// 如果一个 static 变量定义在头文件中，那么这个头文件每被 include 一次，这个 static 变量多一份
	// 所以，绝对不要让 static 变量放到头文件中，考虑到 class 是定义在头文件中的。所以，把 static 变量
	// 定义到 source 中去
	//
	// 而 const static ，显然，所有的文件都是只有一份。
	static const int m = 12;
	// static int g = 13;
};
int main(int argc, char *argv[])
{
	return 0;
}
