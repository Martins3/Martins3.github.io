// unique_ptr：只有一个拥有者
// 终于知道了，unique_ptr 和 move 合并使用，从而表示只有一个 owner
// 而且这个例子中，需要注意，unique_ptr 自动的解决了 exception 捕获问题
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

struct Object {
    Object() { std::cout << "创建 Object\n"; }
    ~Object() { std::cout << "释放 Object\n"; }
};

void work()
{
    auto owner = std::make_unique<Object>();
    Object* observer = owner.get(); // 借用，不负责 delete。
    std::cout << "借用指针有效：" << (observer != nullptr) << '\n';

    // auto another = owner; // 编译失败：不允许复制所有权。
    auto another = std::move(owner);
    std::cout << "转移后 owner 为空：" << (owner == nullptr) << '\n';
    std::cout << "对象地址没变：" << (another.get() == observer) << '\n';

    throw std::runtime_error("模拟处理中途失败");
    // 即使到不了函数末尾，another 也会在异常展开时析构并释放 Object。
}

int main()
{
    try {
        work();
    } catch (const std::exception& e) {
        std::cout << "捕获异常：" << e.what() << '\n';
    }
}
