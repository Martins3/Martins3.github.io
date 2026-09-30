#include <iostream>
class A {
    public:
	void func()
	{
		std::cout << "A" << std::endl;
	}
};

class B : public A {
    public:
	virtual void func()
	{
		std::cout << "B" << std::endl;
	}
};

int main()
{
	// 只有 A 添加上 virtual 之后，才可以被继承的
	A * b = new B;
	b->func();

	return 0;
}
