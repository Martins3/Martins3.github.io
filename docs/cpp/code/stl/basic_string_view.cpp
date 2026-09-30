#include <iostream>
#include <string_view>

int main()
{
	constexpr std::string_view unicode[]{ "▀▄─", "▄▀─", "▀─▄", "▄─▀" };

	for (int y{}, p{}; y != 6; ++y, p = ((p + 1) % 4)) {
		for (int x{}; x != 16; ++x)
			std::cout << unicode[p];
		std::cout << '\n';
	}

	constexpr std::basic_string_view abc{ "12", 12 };
	// TODO 为什么这么声明会有问题
	// constexpr std::basic_string_view<char, int> abc{ "12", 12 };
	std::cout << abc << std::endl;

	return 0;
}
