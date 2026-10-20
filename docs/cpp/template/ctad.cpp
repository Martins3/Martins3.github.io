#include <cassert>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Basic {

template <typename T> struct Box {
	T value;

	Box(T v) : value(v) {}
};

static void test()
{
	Box b(42);
	static_assert(std::is_same_v<decltype(b), Box<int>>);
	assert(b.value == 42);

	auto explicit_type = Box<int>(42); // 只有 auto 推导。
	auto both = Box(42);               // CTAD 和 auto 一起使用。
	static_assert(std::is_same_v<decltype(explicit_type), Box<int>>);
	static_assert(std::is_same_v<decltype(both), Box<int>>);

	Box text("hello"); // 按值推导，字符串字面量退化成指针。
	static_assert(std::is_same_v<decltype(text), Box<const char *>>);
	assert(std::string(text.value) == "hello");

	std::cout << "implicit guide: Box<int>, Box<const char *>\n";
}

}

namespace Guided {

template <typename T> struct Box {
	T value;

	Box(T v) : value(v) {}
};

// 推导指引没有函数体；实际构造仍由 Box<std::string> 的构造函数完成。
Box(const char *) -> Box<std::string>;

static void test()
{
	Box text("hello");
	static_assert(std::is_same_v<decltype(text), Box<std::string>>);
	assert(text.value == "hello");

	// 显式指定模板实参时不使用 CTAD。
	Box<const char *> pointer("hello");
	static_assert(std::is_same_v<decltype(pointer), Box<const char *>>);
	assert(std::string(pointer.value) == "hello");

	std::cout << "user-defined guide: Box<std::string>, value = "
		  << text.value << '\n';
}

}

static void test_standard_library()
{
	std::pair p(1, 2.5);
	static_assert(std::is_same_v<decltype(p), std::pair<int, double>>);
	assert(p.first == 1 && p.second == 2.5);

	std::vector repeated(3, 7); // 三个 7。
	std::vector elements{3, 7}; // 两个元素：3 和 7。
	static_assert(std::is_same_v<decltype(repeated), std::vector<int>>);
	static_assert(std::is_same_v<decltype(elements), std::vector<int>>);
	assert((repeated == std::vector<int>{7, 7, 7}));
	assert((elements == std::vector<int>{3, 7}));

	std::cout << "std::pair<int, double>: (" << p.first << ", " << p.second
		  << ")\n"
		  << "vector(3, 7): size = " << repeated.size() << '\n'
		  << "vector{3, 7}: size = " << elements.size() << '\n';
}

int main()
{
	Basic::test();
	Guided::test();
	test_standard_library();
	std::cout << "All CTAD checks passed.\n";
}
