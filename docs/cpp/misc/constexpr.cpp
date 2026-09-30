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
int main(int argc, char *argv[])
{

	// 注意，const 表示这个位置是赋值之后，不可以继续修改
	// constexpr 表示该位置是在编译器就可以确定了
	const int a =  argc;
	// 这个显然是不行的
	// constexpr int b =  argc;
	return 0;
}
