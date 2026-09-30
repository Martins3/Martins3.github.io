// 首先，constructor 是不能成为 virtual 的，语法限制，因为构造函数没有
// 这里讨论一下 copy assignment / move assignment 如果实现 override 的情况。
//
// 我认为，这个问题的本质是: 什么时候需要 Base = Derive 或者 Derive = Base 的操作。
// 尤其是，Derive = Base 会导致多余的部分没有初始化，我想不到这样的使用场景。
//
// https://stackoverflow.com/questions/669818/virtual-assignment-operator-c
// https://stackoverflow.com/questions/31423632/virtual-assignment-operator

#include <iostream>
class A {
    public:
	virtual A &operator=(const A &a_)
	{
		std::cout << "Calling A" << std::endl;
		return *this;
	}
};

class B : public A {
    public:
	// 注意，这个函数根本和 virtual A &operator=(const A &a_) 没有关系
	// 在这个函数后面也无法添加 override
	// virtual B &operator=(const B &b_)
	// {
	// 	std::cout << "Calling B" << std::endl;
	// 	return *this;
	// }

	// 首先，这样定义这样的函数不是非常奇怪吗?
	virtual B &operator=(const A &b_) override
	{
		std::cout << "Calling B" << std::endl;
		return *this;
	}
};

void test1()
{
	B b1;
	B b2;
	A a1;

	A &a = b1;

	a = b2; // Calling B
	a = a1; // Calling B
}

void test2()
{
	B b1;
	B b2;
	A a1;
	A a2;

	A &a = a1;

	a = b2; // Calling A
	a = b1; // Calling A
	a = a2; // Calling A
}

int main()
{
	test1();
	std::cout << "---" << std::endl;
	test2();
	return 0;
}
