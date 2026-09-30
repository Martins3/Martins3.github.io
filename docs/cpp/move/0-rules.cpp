/*
 * Rule of Zero:用 std::vector 表达"拥有所有权"，编译器默认生成的
 * 析构/拷贝/移动就都是正确的，一个特殊成员函数都不用写。
 *
 * 反面教材见 docs/cpp/move/3-5-rules.cpp:裸指针成员导致浅拷贝 -> double free。
 *
 * 编译运行:
 *   g++ -std=c++17 -Wall -Wextra -Wpedantic rule-of-zero.cpp -o rule-of-zero.out
 *   ./rule-of-zero.out
 * 想验证没有内存错误可以再加 ASan:
 *   g++ -std=c++17 -fsanitize=address,undefined -g rule-of-zero.cpp \
 *       -o rule-of-zero-asan.out && ./rule-of-zero-asan.out
 */
#include <cstddef>
#include <cstring>
#include <iostream>
#include <string>
#include <utility>
#include <vector>


/* ------------------------------------------------------------------ */

/*
 * 正解:用 std::vector<char> 作为成员。
 *
 * std::vector 内部自己持有堆内存，并且完整、正确地实现了五法则:
 *   - 拷贝构造/拷贝赋值:分配新内存 + 深拷贝元素
 *   - 移动构造/移动赋值:窃取内部指针,源对象置空
 *   - 析构:释放自己的内存
 *
 * 于是 Buffer 什么都不用写，编译器默认生成的五个特殊成员函数
 * 逐成员地调用 vector 自己的版本，整体就是对的。这就是 Rule of Zero。
 */
class Buffer {
    public:
	explicit Buffer(std::size_t n, char fill)
		: data_(n, fill)
	{
	}

	/* 故意不写任何特殊成员函数:
	 *   Buffer(const Buffer &)            -> 隐式生成
	 *   Buffer(Buffer &&)                 -> 隐式生成
	 *   Buffer &operator=(const Buffer &) -> 隐式生成
	 *   Buffer &operator=(Buffer &&)      -> 隐式生成
	 *   ~Buffer()                         -> 隐式生成
	 * 全部都是正确的。
	 */

	void set(std::size_t i, char c)
	{
		data_[i] = c;
	}

	char at(std::size_t i) const
	{
		return data_[i];
	}

	std::size_t size() const
	{
		return data_.size();
	}

	const char *data() const
	{
		return data_.data();
	}

	std::string str() const
	{
		return std::string(data_.begin(), data_.end());
	}

    private:
	std::vector<char> data_;
};

void demo_vector_rule_of_zero()
{
	std::cout << "=== 正解:std::vector<char> 成员 ===\n";

	Buffer a(8, 'a');
	std::cout << "a        : " << a.str()
		  << " data=" << static_cast<const void *>(a.data()) << "\n";

	/* 1. 拷贝构造:编译器生成的版本 -> vector 深拷贝,地址不同 */
	Buffer b = a;
	std::cout << "b = a    : " << b.str()
		  << " data=" << static_cast<const void *>(b.data()) << "\n";
	std::cout << "两份数据地址不同 -> 深拷贝\n";

	/* 2. 拷贝之后两个对象完全独立,改一个不影响另一个 */
	b.set(0, 'b');
	std::cout << "b.set(0, 'b'): a=" << a.str() << ", b=" << b.str()
		  << "\n";
	std::cout << "改 b 不影响 a -> 没有共享内存\n\n";

	/* 3. 拷贝赋值:同样由编译器生成,底层是 vector 的拷贝赋值 */
	Buffer c(4, 'c');
	c = a;
	std::cout << "c = a    : " << c.str()
		  << " data=" << static_cast<const void *>(c.data()) << "\n";
	std::cout << "c 原来的 'cccc' 被正确替换,也没泄漏\n\n";

	/* 4. 移动构造:窃取 vector 的内部指针,不发生元素拷贝
	 *    被移动后的对象处于"有效但未指定"状态,这里通常为空。 */
	const char *old = a.data();
	Buffer d = std::move(a);
	std::cout << "d = move(a): " << d.str()
		  << " data=" << static_cast<const void *>(d.data()) << "\n";
	std::cout << "a 移动前 data=" << static_cast<const void *>(old)
		  << ", 移动后 size=" << a.size()
		  << " -> 指针被窃取,源对象不再拥有这块内存\n\n";

	/* 5. 析构:每个对象各自析构自己的 vector,不重不漏。
	 *    用 ASan 跑就能确认没有 leak / double free。 */
	std::cout << "作用域结束,析构全部由编译器生成,无 double free\n\n";
}

/* ------------------------------------------------------------------ */

/*
 * 如果你就是想"把五个特殊成员函数明确写出来"(比如为了可读性)，
 * 那么 = default 就够了，不需要手写任何函数体。
 * 注意:显式写出这五个，语义和 Rule of Zero 完全一致。
 */
class ExplicitBuffer {
    public:
	explicit ExplicitBuffer(std::size_t n, char fill)
		: data_(n, fill)
	{
	}

	ExplicitBuffer(const ExplicitBuffer &) = default;
	ExplicitBuffer(ExplicitBuffer &&) noexcept = default;
	ExplicitBuffer &operator=(const ExplicitBuffer &) = default;
	ExplicitBuffer &operator=(ExplicitBuffer &&) noexcept = default;
	~ExplicitBuffer() = default;

	std::string str() const
	{
		return std::string(data_.begin(), data_.end());
	}

    private:
	std::vector<char> data_;
};

int main()
{
	demo_vector_rule_of_zero();
	return 0;
}
