// 主题: noexcept move constructor 的重要性 与 moved-from 状态
//
// 参考:
// - https://en.cppreference.com/w/cpp/language/noexcept_spec
// - https://stackoverflow.com/questions/10787766/when-should-i-really-use-noexcept
//
// 核心问题:
// 1. STL 容器(如 std::vector)在重新分配内存时，如果元素的 move constructor 不是 noexcept，
//    容器会选择 copy 而不是 move。原因是: 如果 move 过程中抛出异常，
//    原容器中的元素可能已经被部分移动，无法恢复，导致数据丢失。
//    copy 则安全得多，因为原数据始终完整保留。
// 2. 被 move 后的对象处于 "valid but unspecified state":
//    - 可以安全地析构
//    - 可以安全地赋值
//    - 不可以假设其成员的具体值

#include <iostream>
#include <string>
#include <utility>
#include <vector>

struct SafeMove {
	std::string data;
	static int move_count;
	static int copy_count;

	SafeMove() : data(1024, 'x')
	{
	} // 1KB data

	// noexcept move constructor
	SafeMove(SafeMove &&other) noexcept
		: data(std::move(other.data))
	{
		++move_count;
	}

	SafeMove(const SafeMove &other) : data(other.data)
	{
		++copy_count;
	}
};

int SafeMove::move_count = 0;
int SafeMove::copy_count = 0;

struct UnsafeMove {
	std::string data;
	static int move_count;
	static int copy_count;

	UnsafeMove() : data(1024, 'x')
	{
	}

	// move constructor WITHOUT noexcept
	UnsafeMove(UnsafeMove &&other)
		: data(std::move(other.data))
	{
		++move_count;
	}

	// 因为 move 不是 noexcept，vector reallocation 时会优先用 copy!
	UnsafeMove(const UnsafeMove &other) : data(other.data)
	{
		++copy_count;
	}
};

int UnsafeMove::move_count = 0;
int UnsafeMove::copy_count = 0;

struct Simple {
	int value;
	std::string name;

	Simple(int v, const std::string &n) : value(v), name(n)
	{
	}

	Simple(Simple &&other) noexcept
		: value(other.value)
		, name(std::move(other.name))
	{
	}

	Simple &operator=(Simple &&other) noexcept
	{
		value = other.value;
		name = std::move(other.name);
		return *this;
	}
};

void demo_vector_reallocation()
{
	std::cout << "=== Demo 1: vector reallocation chooses copy vs move ===\n";

	{
		std::cout << "\n-- SafeMove (noexcept move): --\n";
		std::vector<SafeMove> vec;
		vec.reserve(2);
		for (int i = 0; i < 4; ++i) {
			SafeMove s;
			vec.push_back(std::move(s));
		}
		SafeMove::move_count = 0;
		SafeMove::copy_count = 0;
		vec.reserve(100); // 强制 reallocation
		std::cout << "  during reallocation: moves=" << SafeMove::move_count
			  << ", copies=" << SafeMove::copy_count << "\n";
	}

	{
		std::cout << "\n-- UnsafeMove (non-noexcept move): --\n";
		std::vector<UnsafeMove> vec;
		vec.reserve(2);
		for (int i = 0; i < 4; ++i) {
			UnsafeMove u;
			vec.push_back(std::move(u));
		}
		UnsafeMove::move_count = 0;
		UnsafeMove::copy_count = 0;
		vec.reserve(100); // 强制 reallocation
		std::cout << "  during reallocation: moves=" << UnsafeMove::move_count
			  << ", copies=" << UnsafeMove::copy_count << "\n";
		std::cout << "  -> copies > 0 because vector avoids non-noexcept moves\n";
	}
}

void demo_moved_from_state()
{
	std::cout << "\n=== Demo 2: moved-from state ===\n";
	Simple s1(42, "hello");
	std::cout << "  before move: value=" << s1.value << ", name='" << s1.name
		  << "'\n";

	Simple s2 = std::move(s1);
	std::cout << "  after move:  value=" << s1.value << ", name='" << s1.name
		  << "'\n";
	std::cout << "  -> s1 is in 'valid but unspecified state'\n";
	std::cout << "     - safe to destruct s1\n";
	std::cout << "     - safe to assign to s1\n";
	std::cout << "     - do NOT read s1.value or s1.name expecting specific values\n";

	s1 = Simple(100, "reassigned");
	std::cout << "  after reassignment: value=" << s1.value << ", name='"
		  << s1.name << "'\n";
}

int main()
{
	demo_vector_reallocation();
	demo_moved_from_state();
	return 0;
}
