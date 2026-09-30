// https://stackoverflow.com/questions/5211205/template-partial-specialization

template <class T> struct B {
	using type = T;

    public:
	int a = 1;
};

template <class T> struct X {};

template <class T> struct X<B<T> > {};

// 如下两个编译会报错，其内容为:
// partial-specialization.cpp:14:27: error: template parameters not deducible in partial specialization:
// template <class T> struct X<typename B<T>::type *> { };
// template <class T> struct X<typename B<T>::type> { };
// 似乎因为，如果 B partial-specialization 来其他的类型，其实 B<T> 中到底是什么，其实高不清楚的
//
//  假设现在要匹配 X<int>，编译器就得反过来问：
//
//  > 哪个 T 能使 B<T>::type 等于 int？
//
//  你的代码特意构造了三个答案：
//
//  B<int>::type  // int，来自主模板
//  B<Foo>::type  // int，来自全特化
//  B<Bar>::type  // int，来自全特化
//
//  知道 T 可以求出 type，知道 type 却不能反推出 T。 类型别名也不会保留“这个 int 来自 B<Foo>”的信息。
//
//  严格来说，C++ 直接规定这里 B<T>:: 属于 非推导上下文（non-deduced context）；即使删掉 Foo、Bar 的特化，也不能从这个位置推导 T。加上 *
//  同样无法解决。标准草案规则 (https://eel.is/c++draft/temp.deduct.type#5.1)

class Foo;
class Bar;
template <> struct B<Foo> {
	using type = int;
};

template <> struct B<Bar> {
	using type = int;
};

void test1()
{
	//  因为这里已经明确给出了 int 或 Foo，只需要正向查出 type：
	//
	// struct X<typename B<int>::type> x;     // 等价于 X<int> x;
	// struct X<typename B<int>::type *> x2;  // 等价于 X<int*> x2;
	// struct X<typename B<Foo>::type *> x3;  // 等价于 X<int*> x3;
	//
	// 这三个声明用到的都是 X 主模板；没有一个参数长得像 B<T>。其中 x2、x3 的类型完全相同。
	struct X<typename B<int>::type> x;
	struct X<typename B<int>::type *> x2;
	struct X<typename B<Foo>::type *> x3;
}

int main()
{
	test1();
	return 0;
}
