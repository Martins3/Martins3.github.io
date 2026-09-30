// https://github.com/wuye9036/CppTemplateTutorial
//
// 理解 C++ 中的 "特化"
// 1. 当模板实例化时提供的模板参数不能匹配到任何的特化形式的时候，它就会去匹配类模板的“原型”形式。
// 2.和继承不同，类模板的“原型”和它的特化类在实现上是没有关系的，并不是在类模板中写了 ID 这个Member，那所有的特化就必须要加入 ID 这个Member，或者特化就自动有了这个成员
#include <iostream>
// 首先，要写出模板的一般形式（原型）
template <typename T> class AddFloatOrMulInt {
    public:
	static T Do(T a, T b)
	{
		// 在这个例子里面一般形式里面是什么内容不重要，因为用不上
		// 这里就随便给个0吧。
		return T(0);
	}
};

// 其次，我们要指定T是int时候的代码，这就是特化：
template <> class AddFloatOrMulInt<int> {
    public:
	static int Do(int a, int b)
	{
		return a * b;
	}
};

// 再次，我们要指定T是float时候的代码：
template <> class AddFloatOrMulInt<float> {
    public:
	static float Do(float a, float b)
	{
		return a + b;
	}
};

void test1()
{
	AddFloatOrMulInt<int> a;
	std::cout << a.Do(1, 2) << std::endl;

	AddFloatOrMulInt<float> b;
	std::cout << b.Do(1, 2) << std::endl;

	AddFloatOrMulInt<double> c;
	std::cout << c.Do(1, 2) << std::endl;
}

template <typename T> class TypeToID {
    public:
	static int const ID = -1;
};

template <> class TypeToID<long> {
    public:
	static int const ID = 0;
};

class ClassB {};
template <> class TypeToID<void()> {
    public:
	static int const ID = 2;
};

template <> class TypeToID<int[3]>; // 数组的TypeID
template <> class TypeToID<int(int[3])>; // 这是以数组为参数的函数的TypeID
template <>
class TypeToID<int (ClassB::*[3])(
	void *, float[2])>; // 我也不知道这是什么了，自己看着办吧。
using func = void(void);
void test2()
{
	TypeToID<int> i;
	std::cout << i.ID << std::endl;
	TypeToID<long> l;
	std::cout << l.ID << std::endl;

	TypeToID<func> f;
	std::cout << f.ID << std::endl;
}

template <typename T> class TypeToID3 {
    public:
	static int const NotID = -2;
};

template <> class TypeToID3<float> {
    public:
	static int const ID = 1;
};

void test3()
{
	std::cout << "ID of float: " << TypeToID3<float>::ID << std::endl; // Print "1"
	std::cout << "ID of int: " << TypeToID3<int>::NotID << std::endl; // Print "1"

	/* std::cout << "NotID of float: " << TypeToID3<float>::NotID << std::endl; */
	// Error! TypeToID<float>使用的特化的类，这个类的实现没有NotID这个成员。
	/* std::cout << "ID of double: " << TypeToID3<double>::ID << std::endl; */
	// Error! TypeToID<double>是由类模板实例化出来的，它只有NotID，没有ID这个成员。
}

// 我要对所有的指针类型特化，所以这里就写T*
template <typename T> class TypeToID<T *> {
    public:
	static int const ID = 1111; // 用最高位表示它是一个指针
};

// 这里还是可以对于类型进行在做特化
template <> class TypeToID<long *> {
    public:
	static int const ID = 2222;
};

void test4()
{
	std::cout << "ID of float: " << TypeToID<float *>::ID << std::endl;
	std::cout << "ID of float: " << TypeToID<long *>::ID << std::endl;
}

int main(int argc, char *argv[])
{
	test1();
	test2();
	test3();
	test4();
	return 0;
}
