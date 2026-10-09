#include <iostream>
#include <string>
#include <utility>

// 只打印选中了哪个重载，不实际移动字符串。
void process(const std::string&)
{
    std::cout << "左值版本\n";
}

void process(std::string&&)
{
    std::cout << "右值版本\n";
}

template <typename T>
void plain(T&& s)
{
    process(s);                  // s 有名字，表达式是左值。
}

template <typename T>
void move_all(T&& s)
{
    process(std::move(s));       // 无论原来是什么，都转成右值。
}

template <typename T>
void forward_original(T&& s)
{
    // 传左值：T = std::string&，转发为左值。
    // 传右值：T = std::string，转发为右值。
    process(std::forward<T>(s)); // 保留调用者原来的传参方式。
}

int main()
{
    std::string name = "hello";

    std::cout << "直接调用：\n";
    process(name);                   // 左值版本
    process(std::move(name));        // 右值版本

    std::cout << "\n包装后直接传 s：\n";
    plain(name);                     // 左值版本
    plain(std::move(name));          // 左值版本：丢失了原来的右值类别

    std::cout << "\n包装后用 std::move：\n";
    move_all(name);                  // 右值版本：把调用者的左值也变成右值
    move_all(std::move(name));       // 右值版本

    std::cout << "\n包装后用 std::forward：\n";
    forward_original(name);          // 左值版本
    forward_original(std::move(name)); // 右值版本：与直接调用一致
}
