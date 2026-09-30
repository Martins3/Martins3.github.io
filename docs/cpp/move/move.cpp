#include <iostream>
#include <vector>
using namespace std;

class A {
    public:
	A()
	{
		cout << "default constructor" << endl;
	}

	A(const A &x)
	{
		cout << "copy constructor" << endl;
	}

	const A &operator=(const A &x)
	{
		cout << "operator =" << endl;
		return *this;
	}
};

void move_item()
{
	vector<int> a(1000, 0);
	vector<int> b = a;
	std::cout << a.back() << std::endl;

	// 这里 a 后面还可能继续使用，因此 b 必须申请自己的内存，并复制一百万个元素。
	//
	// 但如果我们明确表示“不再需要 a 的内容”：
	//
	// c 就可以接管 a 的堆内存。概念上类似：
	//
	// c.data = a.data;
	// a.data = nullptr;
	//
	// 所以：
	//
	// - copy：源对象和目标对象都保留资源。
	// - move：目标对象接管源对象的资源。
	// - std::move：只表达“允许移动”，自身不搬运任何东西。
	vector<int> c = std::move(a);
	std::cout << a.back() << std::endl;
}

int main()
{
	A a; // default constructor
	A b(a); // copy constructor
	A c = a; // copy constructor
	c = b; // operator =

	move_item();
	return 0;
}
