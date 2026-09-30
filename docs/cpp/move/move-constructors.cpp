// 主题: 编译器何时自动生成 move constructor，何时不会
//
// 参考:
// - https://en.cppreference.com/w/cpp/language/move_constructor
// - https://stackoverflow.com/questions/33932824/why-does-destructor-disable-generation-of-implicit-move-methods
//
// C++11 规则总结:
// 1. 如果类没有用户声明的 copy constructor / copy assignment / move assignment / destructor，
//    编译器会自动生成 move constructor。
// 2. 一旦用户声明了 destructor（即使是 =default），编译器就不再自动生成 move constructor。
//    此时 std::move 会 fallback 到 copy constructor。
// 3. 可以显式定义 move constructor，或用 = default 强制编译器生成。

#include <iostream>
#include <string>
#include <utility>

struct HasAutoMove {
	std::string data;
	// 编译器自动生成 move constructor:
	// HasAutoMove(HasAutoMove&& other) noexcept
	//     : data(std::move(other.data)) {}
};

struct HasDestructor {
	std::string data;
	~HasDestructor() = default; // 用户声明了 destructor
	// 编译器不再自动生成 move constructor!
	// std::move 会 fallback 到 copy constructor
};

struct HasExplicitMove {
	std::string data;
	HasExplicitMove() = default;
	~HasExplicitMove() = default;
	HasExplicitMove(HasExplicitMove &&other) noexcept
		: data(std::move(other.data))
	{
		std::cout << "  [HasExplicitMove] move constructor called\n";
	}
};

struct HasDefaultMove {
	std::string data;
	HasDefaultMove() = default;
	~HasDefaultMove() = default; // 用户声明了 destructor
	HasDefaultMove(HasDefaultMove &&) = default; // 强制编译器生成 move
};

void test_auto_move()
{
	std::cout << "=== Test 1: Compiler-generated move ===\n";
	HasAutoMove a;
	a.data = "hello";
	std::cout << "  before move: a.data = '" << a.data << "'\n";
	HasAutoMove b = std::move(a);
	std::cout << "  after move:  a.data = '" << a.data << "' (unspecified)\n";
}

void test_destructor_blocks_move()
{
	std::cout << "\n=== Test 2: User-declared destructor blocks implicit move ===\n";
	HasDestructor a;
	a.data = "hello";
	std::cout << "  before 'move': a.data = '" << a.data << "'\n";
	// 注意: 这里调用的是 copy constructor，不是 move constructor!
	HasDestructor b = std::move(a);
	std::cout << "  after 'move':  a.data = '" << a.data << "' (copy preserves source)\n";
}

void test_explicit_move()
{
	std::cout << "\n=== Test 3: Explicit move constructor ===\n";
	HasExplicitMove a;
	a.data = "hello";
	std::cout << "  before move: a.data = '" << a.data << "'\n";
	HasExplicitMove b = std::move(a);
	std::cout << "  after move:  a.data = '" << a.data << "' (unspecified)\n";
}

void test_default_move()
{
	std::cout << "\n=== Test 4: =default to restore implicit move ===\n";
	HasDefaultMove a;
	a.data = "hello";
	std::cout << "  before move: a.data = '" << a.data << "'\n";
	HasDefaultMove b = std::move(a); // 调用编译器生成的 move
	std::cout << "  after move:  a.data = '" << a.data << "' (unspecified)\n";
}

int main()
{
	test_auto_move();
	test_destructor_blocks_move();
	test_explicit_move();
	test_default_move();
	return 0;
}
