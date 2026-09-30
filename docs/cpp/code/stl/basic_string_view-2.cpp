// https://en.cppreference.com/w/cpp/string/basic_string_view/operator%22%22sv

#include <iostream>
#include <string_view>
#include <typeinfo>

void print_each_character(const std::string_view sw)
{
    for (char c : sw)
        std::cout << (c == '\0' ? '@' : c);
    std::cout << '\n';
}

int main()
{
    using namespace std::literals;

    std::string_view s1 = "abc\0\0def";
    std::string_view s2 = "abc\0\0def"sv;

    std::cout << "s1.size(): " << s1.size() << "; s1: ";
    print_each_character(s1);
    std::cout << "s2.size(): " << s2.size() << "; s2: ";
    print_each_character(s2);

    std::cout << "substr(1, 4): " << "abcdef"sv.substr(1, 4) << '\n';

    // 太牛了，lambda 加上 template 的
    auto value_type_info = []<typename T>(T)
    {
	// 获取到 string_view 类型的  value_type
        using V = typename T::value_type;
        std::cout << typeid(T).name() << " sizeof " << typeid(V).name() << ": " << sizeof(V) << '\n';
    };

    value_type_info("char A"sv);
    value_type_info(L"wchar_t ∀"sv);
    value_type_info(u8"char8_t ∆"sv);
    value_type_info(u"char16_t ∇"sv);
    value_type_info(U"char32_t ∃"sv);
    value_type_info(LR"(raw ⊞)"sv);
}
