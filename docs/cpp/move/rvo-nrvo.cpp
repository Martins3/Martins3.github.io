// # cpp rvo nrvo
// <!-- 2b14d9c5-70d9-4b2a-b00a-9e72ef4aeeb9 -->
//
// https://stackoverflow.com/questions/60546268/how-can-i-ensure-rvo-instead-of-copy-is-performed
// https://stackoverflow.com/questions/48955310/how-does-c-abi-deal-with-rvo-and-nrvo
//
// - RVO: Return Value Optimization，返回值优化。它泛指编译器省略函数返回
//   对象时的拷贝或移动；有些资料也用 RVO 特指返回未命名临时对象的情形。
// - NRVO: Named Return Value Optimization，命名返回值优化。例如下面的
//   `return a;`：a 是函数内有名字的局部对象，编译器可以直接在调用方为
//   返回值预留的存储位置中构造 a。
// - URVO: Unnamed Return Value Optimization，未命名返回值优化，例如
//   `return A{};`。从 C++17 开始，这种 prvalue（pure rvalue，纯右值）场景
//   保证直接构造，不再只是一项可选的拷贝省略优化；NRVO 到 C++23 仍不是强制行为。
//
// 本例的 make_nrvo() 返回命名局部变量 result，因此展示的是 NRVO。选项
// -fno-elide-constructors 可以关闭 GCC（GNU Compiler Collection）或 Clang
// 的可选拷贝省略，便于观察 NRVO 未发生时的 copy；但它不能取消 C++17 保证的 prvalue 直接构造。
//
// RVO/NRVO 和 move 的关系：返回值优化成功时，对象直接在最终位置构造，
// 不调用 copy 或 move；NRVO 没有发生时，return 会优先尝试隐式 move，
// 没有可用的移动构造函数时才退回 copy。不要写 return std::move(result)，
// 因为这通常会阻止 NRVO。

#include <stdio.h>
#include <utility>

class A {
    public:
	explicit A(int value)
		: i(value)
	{
		printf("constructor: value=%d\n", i);
	}

	A(const A &other)
		: i(other.i)
	{
		printf("copy constructor: value=%d\n", i);
	}

	A(A &&other) noexcept : i(other.i)
	{
		// -1 只是本 demo 用来标记 moved-from 对象的值，并非 C++ 的通用保证。
		other.i = -1;
		printf("move constructor: take value=%d\n", i);
	}

	~A()
	{
		printf("destructor: value=%d\n", i);
	}

	A &operator=(const A &other)
	{
		i = other.i;
		printf("copy assignment: value=%d\n", i);
		return *this;
	}

	A &operator=(A &&other) noexcept
	{
		i = other.i;
		// 被移动对象仍然有效，但其具体状态由类型自己的移动操作决定。
		other.i = -1;
		printf("move assignment: take value=%d\n", i);
		return *this;
	}

	void show(const char *name) const
	{
		printf("%s: value=%d\n", name, i);
	}

	int i;
};

A make_rvo()
{
	// C++17 起保证直接在调用方构造，即使使用 -fno-elide-constructors。
	return A(10);
}

A make_nrvo()
{
	A result(20);
	return result;
}

A make_after_explicit_move()
{
	A source(30);
	A result(std::move(source));
	return result;
}

void title(const char *text)
{
	printf("\n=== %s ===\n", text);
}

int main()
{
	title("1. C++17 guaranteed RVO: return A(10)");
	{
		A result = make_rvo();
		result.show("result");
	}

	title("2. optional NRVO: return named result");
	{
		A result = make_nrvo();
		result.show("result");
	}

	title("3. explicit std::move construction");
	{
		A result = make_after_explicit_move();
		result.show("result");
	}

	title("4. copy construction for comparison");
	{
		A source(40);
		A result(source);
		result.show("result");
		source.show("source");
	}

	title("5. move assignment into an existing object");
	{
		A target(50);
		target = make_nrvo();
		target.show("target");
	}

	/**
	 * make             同时构建并运行两种版本，方便逐段比较（默认目标）
	 * make run-default 运行允许 NRVO 的版本
	 * make run-no-elide 运行关闭可选拷贝省略的版本
	 * make clean       删除生成的 .out 文件
	 *
	 * 重点比较第 2 段：默认版本只有 constructor；no-elide 版本还会出现
	 * move constructor，说明 NRVO 未发生后，return result 隐式执行了 move。
	 */
}
