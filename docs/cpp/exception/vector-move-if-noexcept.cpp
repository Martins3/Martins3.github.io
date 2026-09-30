#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

struct Item {
	int value;

	explicit Item(int value)
		: value(value)
	{
	}

	Item(const Item &other)
		: value(other.value)
	{
		std::cout << "copy " << value << '\n';
	}

	Item(Item &&other) noexcept(false)
		: value(std::exchange(other.value, -1))
	{
		std::cout << "move " << value << ", then throw\n";
		throw std::runtime_error("move failed");
	}
};

int main()
{
	std::vector<Item> items;
	// 容量
	items.reserve(1);
	items.emplace_back(1);

	std::cout << "vector reallocation:\n";
	// move 可能抛且可以 copy，所以 vector 使用 copy。
	items.emplace_back(2);
	std::cout << "values: " << items[0].value << ' ' << items[1].value
		  << '\n';

	std::cout << "direct move:\n";
	try {
		Item moved(std::move(items[0]));
	} catch (const std::runtime_error &) {
		std::cout << "source after failed move: " << items[0].value
			  << '\n';
	}
}
