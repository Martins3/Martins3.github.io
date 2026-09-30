// https://github.com/wuye9036/CppTemplateTutorial
//
// 这个函数用于演示 template 的参数都可以是什么
// 其中有趣的是，除了类型 T 可以作为 template 的参数，而且 int 也可以

// template 也是可以用前置申明的
template <int i> class A;

template <int i> class A {
    public:
	void foo(int)
	{
	}
};
template <int a, typename b, void *c> class B {
};
template <bool, void (*a)()> class C {
};
template <void (A<3>::*a)(int)> class D {
};

template <int i> int Add(int a) // 当然也能用于函数模板
{
	return a + i;
}

void foo()
{
	A<5> a;
	B<7, A<5>, nullptr> b;
	// 模板参数可以是一个无符号八位整数，可以是模板生成的类；可以是一个指针。
	C<false, &foo>
		c; // 模板参数可以是一个bool类型的常量，甚至可以是一个函数指针。
	D<&A<3>::foo> d; // 丧心病狂啊！它还能是一个成员函数指针！
	int x = Add<3>(
		5); // x == 8。因为整型模板参数无法从函数参数获得，所以只能是手工指定啦。
}

template <double a> class E {
};
// 如果是 c++ 11 ，这个会报错，但是 c++ 20 不会
// template2.cpp:31:18: error: ‘double’ is not a valid type for a template non-type parameter
int main(int argc, char *argv[])
{
	foo();
	E<1.0> e;
	return 0;
}
