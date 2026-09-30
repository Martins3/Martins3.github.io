#include <bits/stdc++.h>
using namespace std;

class Foo {
    public:
	virtual void a();
};

class Bar : public Foo {
    public:
	void a() override
	{
		std::cout << "Bar" << std::endl;
	}
};

int main(int argc, char *argv[])
{
	Bar b;
	Foo f;
	Foo *f1 = new Bar();

	// 这两个函数调用都是会出现错误的，只要完全没有调用，那么就可以仅仅声明
	// 不用实现，这个和普通的函数相同的
	// b.a();
	// f1->a();
	return 0;
}
