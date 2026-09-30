#include <iostream>
class A {
	int *data;

    public:
	A()
		: data(new int)
	{
	}

	~A()
	{
		delete data;
	}
};

int main(int argc, char *argv[])
{
	A a;
	A b = a;
	// 需要析构函数的类必须 定义拷⻉和赋值函数
	// 这里用默认的赋值函数，导致两个 object 指向一个 data 中。
	// 释放的时候，出现了 double free 。
	std::cout << "good" << std::endl;
	return 0;
}
