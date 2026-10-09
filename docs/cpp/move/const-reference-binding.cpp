// const std::string& 可以绑定 std::string 的左值和右值。 为什么这么设计?
//
// • 因为 C++ 需要一种参数形式，能够避免拷贝地读取对象，同时接受已有变量和临时对象。const T& 正好满足这个需求：
//
//  // 只读取字符串，不必复制整个字符串。
//  void print(const std::string& s) {
//      std::cout << s << '\n';
//  }
//
//  std::string name = "Alice";
//  print(name);                  // 已有对象：可以传
//  print(std::string("Bob"));     // 临时对象：也可以传
//
//  如果只能用 T&，第二种调用就不允许；如果用 T 按值传参，传入已有对象时通常需要拷贝。const T& 让同一个只读接口能处理这两种
//  情况。
//
//  为什么普通的 T& 不允许绑定临时对象？这样可以让可修改的引用参数要求调用者提供一个可持续使用的对象，避免修改落在一个马上就
//  会销毁的临时对象上。例如：
//
//  void normalize(std::string& s) {
//      s += "!";
//  }
//
//  normalize(name);               // 修改结果保存在 name 中。
//  // normalize(std::string("Bob")); // 不允许：修改结果随临时对象一起消失。
//
//  const T& 禁止通过引用修改对象，所以允许接收临时对象，适合“读取输入”的接口。
//
//  C++11 又增加了 T&&，用于明确接收右值、修改它或移动其资源。因此三种参数形式表达了不同意图：
//
//   参数形式    常见意图
//  ━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
//   T&          修改调用者提供的对象
//  ──────────  ────────────────────────────────
//   const T&    读取对象，接受左值和右值
//  ──────────  ────────────────────────────────
//   T&&         接收右值，允许修改或移动其资源
//
//   当然，


#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

// const 左值引用：既能接收左值，也能接收右值。
void k(const std::string& c) {
    std::cout << "k reads: " << c << '\n';
    // c += " modified"; // 编译错误：不能通过 const 引用修改字符串。
}

// 编译期验证：非 const / const 左值、非 const / const 右值都能传给 k。
static_assert(std::is_invocable_v<decltype(k), std::string&>);
static_assert(std::is_invocable_v<decltype(k), const std::string&>);
static_assert(std::is_invocable_v<decltype(k), std::string&&>);
static_assert(std::is_invocable_v<decltype(k), const std::string&&>);

int main() {
    std::string text = "mutable lvalue";
    const std::string constant = "const lvalue";

    // 具名变量表达式是 lvalue，非 const 和 const 对象都能绑定。
    k(text);
    k(constant);

    // 临时字符串表达式是 prvalue，同样能绑定到 const 左值引用。
    k(std::string("temporary prvalue"));

    // std::move 产生 xvalue，非 const 和 const 对象都能绑定。
    // 这里的 k 只读取字符串；std::move 本身不会移动资源。
    k(std::move(text));
    k(std::move(constant));
}
