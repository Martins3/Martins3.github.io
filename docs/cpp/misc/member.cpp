#include <iostream>
#include <functional>

class Foo {
    public:
	void hello(int x)
	{
		std::cout << x << '\n';
	}
};

class Bar {
    public:
	void f(int);
	void f(int) const;
};

// 如果成员函数重载，需要用 `static_cast` 指定具体类型：
void (Bar::*p1)(int) = static_cast<void (Bar::*)(int)>(&Bar::f);
void (Bar::*p2)(int) const = static_cast<void (Bar::*)(int) const>(&Bar::f);

int main()
{
	// 指向 Foo 中返回 void、参数为 int 的成员函数
	void (Foo::*pmf)(int) = &Foo::hello;

	Foo foo;

	// 通过对象调用
	(foo.*pmf)(42);

	// 通过对象指针调用
	Foo *p = &foo;
	(p->*pmf)(42);

	// 也可以这么调用
	std::invoke(pmf, foo, 42);
	std::invoke(pmf, &foo, 42);
}
