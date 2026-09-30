#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

struct A {
	std::string s;
	int k;

	A()
	{
		std::cout << "construct!\n";
	}
	A(const A &o)
	{
		std::cout << "copy construct!\n";
	}
	/*
	 * 如果返回 reference 就可以了:
	 * construct!
	 * construct!
	 * construct!
	 * copy assign!
	 * copy assign!
	 * deconstructor!
	 * deconstructor!
	 * deconstructor!
	 *
	 * 如果返回的是 value :
	 *
	 * construct!
	 * construct!
	 * construct!
	 * copy assign!
	 * copy construct!
	 * copy assign!
	 * copy construct!
	 * deconstructor!
	 * deconstructor!
	 * deconstructor!
	 * deconstructor!
	 * deconstructor!
	 *
	 * 可以看到，直接多了两个 copy construct 和 deconstructor
	 *
	 * 因为 operator= 的返回的时候，需要构造一个新的函数出来
	 * */
	A &operator=(const A &o)
	{
		std::cout << "copy assign!\n";
		return *this;
	}

	~A()
	{
		std::cout << "deconstructor!\n";
	}
};

void test()
{
	// 区分一下: 拷⻉构造函数，拷⻉赋值函数
	A d;
	A c;
	c = d; // 这个才是拷贝赋值，两个都已经初始化了，不是看等于号，笑了

	A e;
	A f = e; // 也是拷贝构造
}

struct Accumulator {
	int counter = -10;
	int counter2 = -10;
	int operator()(int i)
	{
		std::cout << "hi" << std::endl;
		return counter += i;
	}
};
void test2()
{
	Accumulator acc;
	std::cout << acc(10) << std::endl;
	std::cout << acc(20) << std::endl;
	// 这个并不会调用 operator() ，用 cpp inside ，这个等价于
	//   Accumulator m(12, {-10}); 感觉误导性很强
	Accumulator m(12);
	std::cout << m.counter << std::endl;
	std::cout << m.counter2 << std::endl;
}

void test3()
{
	// 配合测试 assign operator 到底是返回 reference 还是 value
	A a;

	A b;
	A c;
	b = a;
}

int main(int argc, char *argv[])
{
	test2();
	return 0;
}
