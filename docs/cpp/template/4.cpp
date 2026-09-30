#include <iostream>
template <typename T> class MyTemplate {
    public:
	int a;
	int met();
};

// 定义一个 T 的实现，格式是 template <typename T>
template <typename T> int MyTemplate<T>::met()
{
	return 1;
}

// 可以直接定义一个具体类型的函数
template <> int MyTemplate<int>::met()
{
	return 2;
}

void test1()
{
	MyTemplate<int> foo;
	std::cout << foo.met() << std::endl;
	;

	MyTemplate<double> bar;
	std::cout << bar.met() << std::endl;
	;
}

// 可以有默认参数的
template <typename T, typename U = int> struct Para2 {
	T a;
	U b;
};

void test2()
{
	Para2<int> a;
	std::cout << a.a << std::endl;
	Para2<int, double> b;
	std::cout << b.b << std::endl;
}

// 这里的 typename = int 没有用，但是给 SFINAE 用的
// https://stackoverflow.com/questions/34459640/what-does-typename-enable-void-mean
template <typename T, typename = int> struct Para {
	T a;
};

void test3()
{
	Para<int> a;
	std::cout << a.a << std::endl;
	Para<int, double> b;
	std::cout << a.a << std::endl;
}

// TODO typename = void ，这里的 void 是一个空类型
template <typename T, typename = void> struct Para3 {
	T a;
};

int main(int argc, char *argv[])
{
	test2();
	return 0;
}
