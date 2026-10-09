// 通常是：copy constructor 用 const A&，move constructor 用 A&&，不加 const。
//
// A(const A& a);  // 拷贝构造
// A(A&& a);       // 移动构造
//
// - 拷贝构造不需要修改源对象，因此通常加 const，也能接受 const 对象。A(A&) 也合法，但不能拷贝 const 对象，一般不这样写。
// - 移动构造通常需要修改源对象，比如接管它的指针后将源指针置空，因此不加 const。
// - A(const A&&) 语法合法，也属于移动构造函数，但源对象不可修改，通常无法转移资源，所以很少使用。
//
// 但是，实际上，这些都是可以随便修改的
#include <bits/stdc++.h>
using namespace std;
class A {
    public:
	A()
	{
		std::cout << "default constructor" << std::endl;
	}

	A(const A &a)
	{
		std::cout << "copy constructor" << std::endl;
	}

	A(A &&a)
	{
		std::cout << "move" << std::endl;
	}

	// 添加 const 其实没有意义，TODO 不过，我对于 move 的含义没有理解
	A(const A &&a)
	{
		std::cout << "const move" << std::endl;
	}


	A operator=(const A &)
	{
		std::cout << "const assign copy" << std::endl;
		return *this;
	}

	A operator=(A &)
	{
		std::cout << "assign copy" << std::endl;
		return *this;
	}

	// 这个也是可以的，但是修饰 this 为 const ，那么赋值有什么意义呢?
	// 当然，cpp 也是可以的
	void operator=(const A &&) const
	{
		std::cout << "const assign move" << std::endl;
	}

	A & operator=(A &&)
	{
		std::cout << "assign move" << std::endl;
		return *this;
	}
};

int main(int argc, char *argv[])
{
	A a;
	return 0;
}
