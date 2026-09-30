#include <iostream>
// 如果 base 的所有的构造函数都是 private 的，那么这个 class 直接无法被继承了

class A {
	A()
	{
		std::cout << "A" << std::endl;
	}
	void hello()
	{
		std::cout << "hello" << std::endl;
	}

    public:
	A(int a)
	{
		std::cout << a << std::endl;
	}
};

class B : A {
    public:
	B()
		: A(12) // 这里是必须的
	{
		std::cout << "A" << std::endl;
	}
};
int main(int argc, char *argv[])
{
	B b;
	A a(12);
	return 0;
}
