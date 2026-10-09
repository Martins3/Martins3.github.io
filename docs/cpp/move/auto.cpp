#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// decltype((expr)) 为 T& / T&& / T 时，表达式分别为左值 / 将亡值 / 纯右值。
template <class T> const char* vc()
{
    if constexpr (std::is_lvalue_reference_v<T>)
        return "lvalue";
    if constexpr (std::is_rvalue_reference_v<T>)
        return "xvalue";
    return "prvalue";
}

#define VC(expr) vc<decltype((expr))>()

// 这里只展示本例中四个 int 变量的声明类型。
template <class T> const char* int_type()
{
    if constexpr (std::is_same_v<T, const int&>)
        return "const int&";
    if constexpr (std::is_same_v<T, int&>)
        return "int&";
    if constexpr (std::is_same_v<T, int&&>)
        return "int&&";
    return "int";
}

void target(const std::string&)
{
    std::cout << "target(const std::string&): 左值版本\n";
}

void target(std::string&&)
{
    std::cout << "target(std::string&&): 右值版本\n";
}

void named_reference_demo()
{
    std::cout << "=== 有名字的右值引用，表达式仍是左值 ===\n";
    auto&& value = std::string{"hello"}; // 声明类型是 std::string&&。
    std::cout << "value: " << VC(value) << '\n';
    target(value);                               // 传左值。
    target(std::forward<decltype(value)>(value)); // 恢复为右值。
}

void lambda_demo()
{
    std::cout << "\n=== 泛型 lambda：保留调用者的传参方式 ===\n";
    auto relay = [](auto&& value) {
        // auto&& 是转发引用；用 decltype(value) 代替函数模板中的 T。
        // 不要写 decltype((value))：那永远是左值引用类型。
        target(std::forward<decltype(value)>(value));
    };

    std::string name = "hello";
    std::cout << "relay(name): ";
    relay(name);            // decltype(value) = std::string&。
    std::cout << "relay(std::move(name)): ";
    relay(std::move(name)); // decltype(value) = std::string&&。
}

void auto_type()
{
    int x = 10;
    const int cx = 20;

    auto&& a = x;
    auto&& b = cx;
    auto&& c = 30;
    auto&& d = std::move(x);

    std::cout << "变量  声明类型     表达式值类别\n";
    std::cout << "a     " << int_type<decltype(a)>() << "           " << VC(a) << '\n';
    std::cout << "b     " << int_type<decltype(b)>() << "     " << VC(b) << '\n';
    std::cout << "c     " << int_type<decltype(c)>() << "          " << VC(c) << '\n';
    std::cout << "d     " << int_type<decltype(d)>() << "          " << VC(d) << '\n';
}

int main()
{
    auto_type();
    named_reference_demo();
    lambda_demo();
}
