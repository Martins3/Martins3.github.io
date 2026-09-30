// effective c++ 中的 43 中顺便提到的 explicit qualification 会关闭 "virtual 绑定行为"
// 那个 explicit 指的是 A::hello 就是写死了会调用谁，没有多态了
//
// 和 explicit 关键字那个没什么关系了
#include <iostream>

class A {
    public:
	virtual void hello()
	{
		std::cout << "A" << std::endl;
	}
};

// TODO 深入理解这个问题
// 为什么这里必须是 public 才可以
// explicit.cpp:29:21: error: ‘A’ is an inaccessible base of ‘B’
// 29 |         A * a = new B;
class B : public A {
    public:
	void hello()
	{
		// A:: 这个限定符，这次调用就变成静态绑定，不查虚表，直接进 A::hello()。
		A::hello();
		std::cout << "B" << std::endl;
	}
};

int main(int argc, char *argv[])
{
	B b;
	b.hello();

	A *a = new B;
	a->hello();
	return 0;
}
