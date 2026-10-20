// constexpr
// - `constexpr const int * p = &i` : p 是 constexpr 指针，指向 int 常量, i 必须定义在函数外面
// - constexpr 要求返回类型和所有形参类型都是字面值类型, 函数体中间有且仅有一个 return 语句 (可以存在一些不产生动作的语句, 比如 using 和 decltype)
//   - [ ] 可以是字面值 class 吗 ?
// - constexpr 函数不一定返回常量表达式，参数也不一定需要是 const 或者 constexpr
// - literal class
//   - 成员都是 literal type 的 aggregate class
//   - 或者满足下面的所有条件:
//     1. 成员都是 literal type
//     2. 至少一个 constexpr 构造函数
//     3. 所有的成员都可以被 "explicit" 的初始化 (通过类内初始值，或者 constructor)
//     4. 析构函数默认定义
#include <bits/stdc++.h>
using namespace std;

template <typename E> constexpr auto underlying(E e)
{
	return std::underlying_type_t<E>(e);
}

// 利用 constexpr 可以构建非常复杂的公式，然后将其转换为 const
constexpr unsigned count_bits(unsigned value)
{
	unsigned count = 0;

	while (value != 0) {
		count += value & 1u;
		value >>= 1;
	}

	return count;
}

constexpr unsigned enabled_features = 0b101101;

static_assert(count_bits(enabled_features) == 4,
	      "Exactly four features must be enabled");

void test1(int argc)
{
	// 注意，const 表示这个位置是赋值之后，不可以继续修改
	// constexpr 表示该位置是在编译器就可以确定了
	const int a = argc;
	// 这个显然是不行的
	// constexpr int b =  argc;
}

// C1 是一个 literal class
// literal class 是一个 constexpr 扩展到对象的很好的例子
class C1 {
    public:
	constexpr C1(int i)
		: x(i)
	{
	}
	int x;
};

class C2 {
	int y;

    public:
	int x;
	C1 c1;
	constexpr C2()
		: y()
		, x()
		, c1(2)
	{
	}

	C2(int x)
		: c1(12)
	{
	}
};

constexpr int get_x1(C1 c)
{
	return c.x;
}

constexpr int get_x2(C2 c)
{
	return c.x;
}

class A {
    public:
	int a;
};

void test2()
{

	// 这里展示下，object 也可以是 constexpr
	constexpr C2 a;
	// 因为 x 需要是 constexpr ，所以必须将 a 声明为 constexpr 对象
	// 如果 x 没有这个要求，那么就可以不添加

	constexpr int x = get_x2(a);

	constexpr A g = { 12 };
}

int main(int argc, char *argv[])
{
	test1(argc);
	test2();
	return 0;
}
