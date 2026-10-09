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

class C {
	std::shared_ptr<A> a;

    public:
	C()
	{
		a = std::make_shared<A>(1, 2);
	}

	void show_ref()
	{
		std::cout << a.use_count() << std::endl;
	}
};

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

void test_how_to_create()
{
	// 原来这个才是正确的构造方法
	std::make_shared<A>(1, 2);
	// 这会导致一次复制
	auto m = std::make_shared<A>(A(1, 2));
}

void test_assign()
{
	auto m = std::make_shared<A>(1, 2);
	// move 的过程，导致原来的资源被释放，
	// 所以这里的赋值就可以看到函数的析构，我终于参透了 move 的实现了
	m = std::make_shared<A>(3, 4);
	std::cout << "nice" << std::endl;
}

void test_object()
{
	// 这个就是最经典的如果资源是 shared_ptr 管理，那么就不需要了
	C c;
	C b = c;
	b.show_ref();
	C &&g = std::move(b);
	g.show_ref();
}

void operator_test()
{
	std::shared_ptr<int> a;
	// 简单看，对象都初始化出来了，怎么可能为 nullptr
	// 但是注意，对象 a 是重载了 operator bool() 和 == 的
	// 所以这里可以用来判断
	if (a)
		std::cout << "not" << std::endl;
	else
		std::cout << "nullptr" << std::endl;
	a = std::make_shared<int>(12);
	if (a)
		std::cout << "not" << std::endl;
	else
		std::cout << "nullptr" << std::endl;
}

int main(int argc, char *argv[])
{
	return 0;
}
