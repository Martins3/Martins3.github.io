// 忽然发现各种构造函数的参数都是随意的
#include <iostream>

// 测试 1 : this 指针到底是什么东西
// 就是当前结构体内存地址
class A {
    public:
	void operator=(const A &a)
	{
		std::cout << "operator = " << std::endl;
	}

	A &foo()
	{
		std::cout << "foo\t" << this << std::endl;
		return *this;
	}

	A()
	{
	}
};

static void test()
{
	A a;
	std::cout << "test\t" << &a << std::endl;

	A b;
	A c = b.foo();
}

// 测试 2 : 为什么 operator= 的返回值需要时 reference
// 如果不是返回引用，那么使用 operator= 会导致一个拷贝，并且构造出来一个临时对象出来
// 这个临时对象只能是 void operator=(const B &a) 来调用

class B {
    public:
	void operator=(const B &a)
	{
		std::cout << "const operator = " << std::endl;
	}

	// 这里的返回值 B 还是 B &
	B operator=(B &a)
	{
		std::cout << "operator = " << std::endl;
		return *this;
	}

	B(B &a)
	{
		std::cout << "copy" << std::endl;
	}

	B()
	{
	}
};

static void test2()
{
	B a;
	B b;
	B c;
	// 这个地方不可以用，b = c 没问题
	// 注意，b = c 的结果是 void ，所以会导致 a = void 就很诡异了
	a = b = c;
}

// copy/move 赋值函数的返回值就大多数的时候，其实是无所谓的，但是
// 从 operator = 的语义完整，最好是返回一个 object ，而且用 reference ，最后返回 *this 是最好的
int main(int argc, char *argv[])
{
	test();
	std::cout << "test 2" << std::endl;
	test2();
	return 0;
}
