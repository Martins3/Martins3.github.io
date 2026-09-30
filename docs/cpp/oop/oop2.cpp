#include <iostream>

class Base {
    public:
	int value1;
	int value2;
	Base()
	{
		value1 = 1;
	}
	Base(int value)
		: Base()
	{
		value2 = 2;
	}
};
class Subclass : public Base {
    public:
	// 如果是 c++ 11 ，必须显示的使用这句话，但是 c++20 不需要
	using Base::Base; // 继承构造
};
int main()
{
	Subclass s(3);
	std::cout << s.value1 << std::endl;
	std::cout << s.value2 << std::endl;
}
