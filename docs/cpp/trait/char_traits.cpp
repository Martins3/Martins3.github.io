// https://en.cppreference.com/w/cpp/string/char_traits
//
// https://stackoverflow.com/questions/5319770/what-is-the-point-of-stl-character-traits
#include <cctype>
#include <iostream>
#include <string>
#include <string_view>

struct ci_char_traits : public std::char_traits<char> {
	static char to_upper(char ch)
	{
		return std::toupper((unsigned char)ch);
	}
	// TODO 为什么这个 compare 函数会被调用
	static int compare(const char *s1, const char *s2, std::size_t n)
	{
		while (n-- != 0) {
			if (to_upper(*s1) < to_upper(*s2))
				return -1;
			if (to_upper(*s1) > to_upper(*s2))
				return 1;
			++s1;
			++s2;
		}
		return 0;
	}
};

// 通过这个函数，将 std::basic_string_view<char8_t, ...> 转换为
// std::basic_string_view<char8_t, ci_char_traits>
// 然后调用到 compare 函数上
template <class DstTraits, class CharT, class SrcTraits>
constexpr std::basic_string_view<CharT, DstTraits>
traits_cast(const std::basic_string_view<CharT, SrcTraits> src) noexcept
{
	return { src.data(), src.size() };
}

int main()
{
	using namespace std::literals;

	// constexpr auto s1 = u8"Hello"sv;
	// constexpr auto s2 = L"heLLo"sv;

	constexpr auto s1 = "Hello"sv;
	constexpr auto s2 = "heLLo"sv;
	std::cout << typeid(s1).name() << '\n';


	// char_traits.cpp:64:45: error: no match for ‘operator==’ (operand types are ‘std::basic_string_view<char8_t, ci_char_traits>’
	// and ‘std::basic_string_view<wchar_t, ci_char_traits>'
	if (traits_cast<ci_char_traits>(s1) == traits_cast<ci_char_traits>(s2))
		std::cout << "equal" << std::endl;
}
