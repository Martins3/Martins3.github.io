#include <vector>
#include <iostream>

using namespace std;

template <class T> T foo(T a)
{
	return a;
}

template <int N> void compare(const char (&a)[N])
{
	printf("the len %d :   %s\n", N, a);
}

template <typename T> class Foo {
	T t;

    public:
	using value_type = T;
	using size_type = typename vector<T>::size_type;
	value_type bar();
	T foo();
};

template <typename T> T Foo<T>::bar()
{
	return t;
}

// this factorial code is amazing !
template <unsigned int n> struct factorial {
	enum { value = n * factorial<n - 1>::value };
};

template <> struct factorial<0> {
	enum { value = 1 };
};

// 有趣，两个类型，然后用 auto 自动判断应该的返回值，不过一般是用不到了
template <typename T, typename U> auto add3(T x, U y)
{
	return x + y;
}

// `typename` 和 `class` 两个关键字, 可以交换使用，可以看懂就可以了
template <class T, class U> auto add4(T x, U y)
{
	return x + y;
}

int main()
{
	struct factorial<10> a;
	std::cout << a.value << std::endl;
	std::cout << add3(3, 3.0) << std::endl;
	return 0;
}
