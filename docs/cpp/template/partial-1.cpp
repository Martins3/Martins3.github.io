// 最基本的 partial-template-specialization 例子
//
// https://stackoverflow.com/questions/6138439/understanding-simple-c-partial-template-specialization
//
#include <iostream>

// 当第二个模板实参为 int 时，选择下面更具体的偏特化。
template <typename A, typename B>
class Thing // 主模板
{
    public:
	int doSomething();
};

template <typename A>
class Thing<A, int> //partial specialization of the class template
{
    public:
	int doSomething2();
};

template <class A, class B> int Thing<A, B>::doSomething()
{
	return 1;
}

template <class A> int Thing<A, int>::doSomething2()
{
	return 2;
}

int main()
{
	// 很显然，由于 partial-specialization ，所以会优先匹配到    class Thing<A, int>
	Thing<int, int> a;
	std::cout << "partial specialization: " << a.doSomething2() << '\n';

	Thing<int, float> b;
	std::cout << "primary template: " << b.doSomething() << '\n';

	return 0;
}
