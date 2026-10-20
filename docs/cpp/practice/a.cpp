#include <iostream>
using namespace std;

class A {
    public:
	A()
	{
		cout << "cons" << endl;
	}

	A(const A &a)
	{
		cout << "copy" << endl;
	}

	A &operator=(const A &a)
	{
		cout << "=" << endl;
		return *this;
	}
};

class B {
	A _a;

    public:
	B(A a)
		: _a(a)
	{
		cout << "B" << endl;
	}
	B(A a, int x)
	{
		_a = a;
		cout << "B = " << endl;
	}
};

int main(int argc, char *argv[])
{
	A a;

	cout << "------" << endl;
	B b(a);
	cout << "------" << endl;
	B b1(a, 1);
	return 0;
}
