// 无论是构造函数，还是析构函数，都是会继续调用 parent 的，
// 是不是 virtual 表示根据运行时的属性来确定，到底自动调用哪一个函数
//
//
// - https://stackoverflow.com/questions/461203/when-to-use-virtual-destructors
// 	- 最好是让 destructor 是 virtual 的，也就是 destructor 前需要有 virtual 参数
//
// - https://stackoverflow.com/questions/733360/why-do-we-not-have-a-virtual-constructor-in-c
// 	- 为什么构造函数不可以是 virtual 的。也就是构造函数无法添加 virtual 修饰。
// 	原理的解释很好，因为完整的信息，所以没办法。
#include <iostream>

class Base {
    public:
	Base()
	{
		std::cout << "Base con" << std::endl;
	}
	virtual ~Base()
	{
		std::cout << "Base destructor" << std::endl;
	}
};

class Derived : public Base {
    public:
	Derived()
	{
		std::cout << "Derived con" << std::endl;
	}
	~Derived()
	{
		std::cout << "Derived destructor" << std::endl;
	}
};

int main(int argc, char *argv[])
{
	/* Base Derived 都会被调用 */
	Derived a;

	Base *b = new Derived;
	delete b;

	return 0;
}
