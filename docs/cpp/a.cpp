#include <iostream>

template <typename T> class A {
    public:
	int a;
};

template <> class A<int> {};

// 继续犯用这里来思考 template 机制吧

int main(int argc, char *argv[])
{
	A<int> a;
	A<double> b;
	std::cout << sizeof(a) << std::endl;
	std::cout << sizeof(b) << std::endl;

	return 0;
}
