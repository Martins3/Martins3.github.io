#include <iostream>
/*
 * # cpp delete
 * <!-- 6daef6c4-4ac5-4c9d-bb63-087dbc4c74bf -->
 *
 * 显式禁用默认函数 : 为了能够让程序员显式的禁用某个函数，
 * C++11 标准引入了一个新特性：deleted 函数。
 * 程序员只需在函数声明后加上“=delete;”，就可将该函数禁用。
 * 例如，我们可以将类 X 的拷贝构造函数以及拷贝赋值操作符声明为 deleted 函数，
 * 就可以禁止类 X 对象之间的拷贝和赋值
 */

class X {
    public:
	X() {};
	X(const X &) = delete; // 声明拷贝构造函数为 deleted 函数
	X &operator=(const X &) = delete; // 声明拷贝赋值操作符为 deleted 函数
};

/* Deleted 函数特性还可用于禁用类的某些转换构造函数，从而避免不期望的类型转换 */

class N {};
class M {
	double a;

    public:
	M(double);
	M(N x)
	{
		this->a = 10;
	}
	M(int) = delete;
};

M::M(double m)
{
	this->a = m;
}

/* Deleted 函数特性还可以用来禁用某些用户自定义的类的 new 操作符，
 * 从而避免在自由存储区创建类的对象 */
class J {
	int a;

    public:
	void *operator new(size_t) = delete;
	void *operator new[](size_t) = delete;
};

void test1()
{
	X x1;
	/* X x2 = x1; // 错误，拷贝构造函数被禁用 */
	X x3;
	/* x3 = x1; */
	// 错误，拷贝赋值操作符被禁用
}

void test2()
{
	M x1(1.2);
	M x2 = 1.2;
	N n;
	M m3 = n;
	/* M x3 = 2; */
	/* M x2(2); */
	// 错误，参数为整数 int 类型的转换构造函数被禁用
}

void test3()
{
	/* X *pa = new J; // 错误，new 操作符被禁用 */
	/* X *pb = new J[10]; // 错误，new[] 操作符被禁用 */
}

int main(int argc, char *argv[])
{
	test1();
	test2();
	test3();
	std::cout << "cpp delete 用于屏蔽掉自动构建的函数" << std::endl;
	return 0;
}
