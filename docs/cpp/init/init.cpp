#include <bits/stdc++.h>
using namespace std;
class A {
	int a;

    public:
	explicit A()
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

	A &operator=(const A &)
	{
		std::cout << "const assign copy" << std::endl;
		return *this;
	}

	A &operator=(A &&) noexcept
	{
		std::cout << "assign move" << std::endl;
		return *this;
	}

	~A()
	{
		std::cout << "delete" << std::endl;
	}
};

struct B {
	int a;
	int b;
	// 首先，到底什么是 aggregate 类型
	// explicit B(int a, int b) { }
};

struct Aggregate {
	int a;
	int b;
};

void test_B_implicit(class B b)
{
}

void test_class()
{
	// A a();
	A a{};
	A a2;
	A{};
	A();

	// 这也是两个不同的语法
	B(1, 2);
	B{ 1, 2 };
	B b(1, 2);
	B c{ 1, 2 };

	// TODO 应该是只有这种写法了吧?
	A * n1 = new A;
	A * n2 = new A();
	A * n3 = new A{};
	B * n4 = new B(1, 2);
	B * n5 = new B{1, 2};

	// TODO 难道这个 cppreference 中说明的四个方法，没有覆盖到?
	B c1{ .a = 1, .b = 2 };
	B{ .a = 1, .b = 2 };

	// 这两个不受到 explicit 影响，本质就是 B{1, 2} 而已
	B e = B{ 1, 2 };
	B e2 = B(1, 2);
	// 但是这个受到 explicit 的影响了
	B f = { 1, 2 };

	// 这是错误的:
	// conversion from ‘int’ to non-scalar type ‘B’ requested
	// B g = (1, 2);

	// 如果是单独的 () ，那么就会不断的结果
	// test_B_implicit((1, 2));
	test_B_implicit({ 1, 2 });

	// 我们来仔细的回忆一下这个，make_shared 是一个模板函数，这就是完美转发的经典案例
	//
	//  template<typename _Tp, typename... _Args>
	//    inline shared_ptr<_NonArray<_Tp>>
	//    make_shared(_Args&&... __args)
	//    {
	//      using _Alloc = allocator<void>;
	//      _Alloc __a;
	//      return shared_ptr<_Tp>(_Sp_alloc_shared_tag<_Alloc>{__a},
	//        		     std::forward<_Args>(__args)...);
	//    }

	// 这些参数最后都会 forward 到 A 或者 B 的构造中
	auto x1 = std::make_shared<A>();
	auto x2 = std::make_shared<B>();

	// 忽然意识到，这个和上面的 make_shared 不是一个东西
	// 上面是 template 函数，下面是 vector class 的初始化
	// 显然 vector 不是 aggregate class ，之所以支持 {3, 9}
	// 是因为他支持 initializer_list 的初始化方法啊
	std::vector<int> x3(3, 9); // 三个元素：9, 9, 9
	std::vector<int> x4{ 3, 9 }; // 两个元素：3, 9
	// std::vector<int> x5(3, 9, 10); // 不存在这个写法，因为 vector 没有这种 class 了

	// 如果说，aggregate 只是为了支持 () 括号，这里我感觉
	// 所以，其实全部都是为了在这里的情况
	auto x5 = std::make_shared<Aggregate>(1, 2);
	// 不存在这个写法，函数调用只能使用 () ，不存在其他的方法
	// auto x8 = std::make_shared<Aggregate>{1, 2};

	// 这两个都会失败，因为 Aggregate 没有定义一个 Aggregate(int a)
	// 的构造函数
	// std::vector<Aggregate> x6{ 1, 2, 4 };
	// std::vector<Aggregate> x7(1, 2);
}

void test_int()
{
	int i1;
	int i2 = 1;
	int i3 = { 1 };
	int i4{ 1 };
	int i5(1);

	// TODO 那么只有只有这两种变化了吧
	// 那么这种语法的名称是什么?
	int j2 = int(1);
	int j3 = int{ 1 };

	int *p4 = new int{ 1 };
	int *p5 = new int(1);
}

void test_arr()
{
	int i1[2];
	// int i2[2] = 1; // 这个不行
	int i3[2] = { 1 };
	int i4[2]{ 1 };
	int i5[2](1);

	// 这是错误的，这两个都是错误的
	// error: array must be initialized with a brace-enclosed initializer
	// int j2[2] = int(1);
	// int j3[2] = int{ 1 };

	int *p4 = new int{ 1 };
	int *p5 = new int(1);
}

int main(int argc, char *argv[])
{

	return 0;
}
