// 这里演示了 copy 是 exception 安全的，
//
// 首先 copy 的过程中，如果出现了 exception ，还是需要处理的
// 这里的问题关键在于，如果是 move 触发了 exception ，是不知道怎么回退的
// 而 copy 的回退是很容易的，仅仅让代码直接新构建的资源删掉就可以了
#include <iostream>
#include <stdexcept>
#include <vector>

struct Item {
	int value;
	static inline int copy_count;

	explicit Item(int value)
		: value(value)
	{
	}

	Item(const Item &other)
		: value(other.value)
	{
		std::cout << "copy " << value << '\n';
		copy_count ++;
		if (copy_count >= 2)
			throw std::runtime_error("copy failed");
	}

	Item(Item &&other) noexcept(false)
		: value(other.value)
	{
	}
};

int main()
{
	std::vector<Item> items;
	items.reserve(2);
	items.emplace_back(1);
	items.emplace_back(2);
	const Item *old_data = items.data();

	try {
		items.emplace_back(3); // 扩容，复制第二个旧元素时抛异常。
	} catch (const std::runtime_error &error) {
		std::cout << "caught: " << error.what() << '\n';
	}

	// 这里不去捕获异常，那么程序将会直接退出的
	// items.emplace_back(3);

	std::cout << "same storage: " << std::boolalpha
		  << (items.data() == old_data) << '\n';
	std::cout << "size: " << items.size() << '\n';
	std::cout << "values: " << items[0].value << ' ' << items[1].value
		  << '\n';
}
