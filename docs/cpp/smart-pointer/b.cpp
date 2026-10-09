#include <forward_list>
#include <list>
#include <iostream>
#include <memory>
class A {
    public:
	int val;
	A(int a, int b)
	{
		val = a;
	}
	~A()
	{
		std::cout << "delete" << std::endl;
	}
};

struct B {
	int a;
	int b;
};

void some_random_test()
{
	std::forward_list<A> fl;
	// TODO 这个语法叫做什么?
	// 这个叫做隐式转换吧，隐式转换可以发生的地方?
	// fl.push_front(12);

	std::forward_list<B> foo;
	// 想不到这个也是不行的
	// foo.push_front(1,2);
	// 这么想，

	// 圆括号有很多限制
	B x(1, 23);
	B y{ 1, 23 };
}

