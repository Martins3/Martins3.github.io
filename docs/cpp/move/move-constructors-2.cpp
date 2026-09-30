// 主题: move constructor 的语法与调用规则
//
// 关键知识点:
// 1. move constructor 的标准签名: T(T&&)
// 2. 可以有额外参数，但必须都有默认值: T(T&&, int = 0)
// 3. std::move 本质上只是 static_cast<T&&>（类型转换），真正执行"移动"的是 move constructor
// 4. 如果没有定义 move constructor，std::move 会 fallback 到 copy constructor

#include <iostream>
#include <string>
#include <utility>

struct Foo {
	std::string name;

	Foo() : name("default")
	{
	}

	// copy constructor
	Foo(const Foo &other) : name(other.name)
	{
		std::cout << "  [Foo] copy: '" << name << "'\n";
	}

	// move constructor
	Foo(Foo &&other) noexcept : name(std::move(other.name))
	{
		std::cout << "  [Foo] move: '" << name << "'\n";
	}
};

struct Bar {
	std::string name;
	int extra;

	Bar() : name("default"), extra(0)
	{
	}

	// move constructor with default argument (合法但少见)
	Bar(Bar &&other, int e = 42) noexcept
		: name(std::move(other.name))
		, extra(e)
	{
		std::cout << "  [Bar] move with extra=" << extra << "\n";
	}
};

struct NoMove {
	std::string name;

	NoMove() : name("default")
	{
	}

	// 只有 copy constructor，没有 move constructor
	NoMove(const NoMove &other) : name(other.name)
	{
		std::cout << "  [NoMove] copy (no move ctor): '" << name << "'\n";
	}

	// 注意: 如果用户声明了 copy constructor，编译器不会自动生成 move constructor
};

void demo_move_vs_copy()
{
	std::cout << "=== Demo 1: std::move triggers move constructor ===\n";
	Foo f1;
	f1.name = "foo1";
	std::cout << "  calling: Foo f2 = std::move(f1)\n";
	Foo f2 = std::move(f1); // 调用 move constructor
}

void demo_no_move_fallback()
{
	std::cout << "\n=== Demo 2: fallback to copy when no move constructor ===\n";
	NoMove n1;
	n1.name = "no_move1";
	std::cout << "  calling: NoMove n2 = std::move(n1)\n";
	// std::move(n1) 把 n1 转成右值引用，但 NoMove 没有 move ctor，
	// 所以 overload resolution 选择了 copy constructor
	NoMove n2 = std::move(n1);
	std::cout << "  Note: std::move is just a cast; actual move requires a move constructor\n";
}

void demo_move_with_default_arg()
{
	std::cout << "\n=== Demo 3: move constructor with default argument ===\n";
	Bar b1;
	b1.name = "bar1";
	std::cout << "  calling: Bar b2 = std::move(b1)\n";
	Bar b2 = std::move(b1); // 使用默认参数 e=42
	std::cout << "  calling: Bar b3(std::move(b1), 100)\n";
	Bar b3(std::move(b1), 100); // 显式提供参数 e=100
}

int main()
{
	demo_move_vs_copy();
	demo_no_move_fallback();
	demo_move_with_default_arg();
	return 0;
}
